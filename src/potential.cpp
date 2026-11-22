// 潜力值（ptt）：b50 计算 + 展示开关
//
// 评分面板是 UserCell：
//   panel+0x498 = user 对象，user+0x44 = ptt（乘 1000 的 int）
//   panel+0x4A0 = kind，2 = 在线形态，3 = 离线形态

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <unistd.h>

#include "game.hpp"
#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

static const char *st3Path(void)
{
    static char path[256];
    if (path[0])
        return path;

    char pkg[128] = {0};
    FILE *fp = fopen("/proc/self/cmdline", "rb");
    if (fp)
    {
        const size_t n = fread(pkg, 1, sizeof(pkg) - 1, fp);
        fclose(fp);
        if (n == 0)
            pkg[0] = 0;
    }
    if (pkg[0] == 0)
        snprintf(pkg, sizeof(pkg), "%s", PACKAGE_NAME);

    snprintf(path, sizeof(path), "/sdcard/Android/obb/%s/st3", pkg);
    return path;
}

// UserCell 字段（非指令地址，跨版本稳定）
static constexpr size_t kOffCellUser = 0x498; // panel -> user 对象
static constexpr size_t kOffCellKind = 0x4A0; // panel -> 形态
static constexpr size_t kOffCellRoot = 0x4A8; // panel -> 根节点（csb UserCell.csb 的根）
static constexpr size_t kOffUserPtt = 0x44;   // user 的 ptt，乘 1000 的 int

static constexpr int kKindOnline = 2;  // 建评分面板
static constexpr int kKindOffline = 3; // 游戏的真实形态（setupRating 直接 ret）

// 内部精度：1 ptt = kUnit 单位。取 3e6 让公式里的 /3e5 变成精确乘法，
// 只在最后换成乘 1000 的展示值时做一次整除。
static const int64_t kUnit = 3000000;
static const int64_t kUnitToShown = 180000; // (sum / 60) / 3e6 * 1000
static const int64_t kConstToUnit = 300000; // 定数存的是 10 倍，还原要乘 300000

// 单次成绩分档：满分 / 9.8e6 / 9.5e6
static const int kScoreMax = 10000000;
static const int kScoreHigh = 9800000;
static const int kScoreBase = 9500000;

// b50 = 前 50 名之和 + 前 10 名之和
static const int kBestCount = 50;
static const int kBestTopCount = 10;

// 难度上限（PST/PRS/FTR/BYD/ETR）与表容量
static const int kMaxDifficulty = 5;
static const int kMaxEntries = 4096;
static const int kMaxSongIdLen = 48;

// 算不出来，与真实结果 0 区分开
static const int64_t kPttFailed = -2;

// 游戏自己的「无数据」语义：user+0x44 == -1，setupRating 会走 off 分支
// （img/rating_off.png + rating_large/small 显示 "--"）
static const int kPttNoData = -1;

namespace
{
    struct Sqlite3
    {
        int (*open_v2)(const char *, void **, int, const char *) = nullptr;
        int (*prepare_v2)(void *, const char *, int, void **, const char **) = nullptr;
        int (*step)(void *) = nullptr;
        int (*finalize)(void *) = nullptr;
        int (*close)(void *) = nullptr;
        int (*column_int)(void *, int) = nullptr;
        const unsigned char *(*column_text)(void *, int) = nullptr;
        bool ok = false;
    };

    Sqlite3 g_sqlite;

    const int kSqliteOk = 0;
    const int kSqliteRow = 100;
    const int kSqliteDone = 101;
    const int kSqliteOpenReadOnly = 0x00000001;

    bool loadSqlite()
    {
        if (g_sqlite.ok)
            return true;

        void *sym = nullptr;
#define ARC_LOAD_SQLITE(field, name) \
    sym = dlsym(RTLD_DEFAULT, name); \
    if (!sym)                        \
        return false;                \
    g_sqlite.field = reinterpret_cast<decltype(g_sqlite.field)>(sym)

        ARC_LOAD_SQLITE(open_v2, "sqlite3_open_v2");
        ARC_LOAD_SQLITE(prepare_v2, "sqlite3_prepare_v2");
        ARC_LOAD_SQLITE(step, "sqlite3_step");
        ARC_LOAD_SQLITE(finalize, "sqlite3_finalize");
        ARC_LOAD_SQLITE(close, "sqlite3_close");
        ARC_LOAD_SQLITE(column_int, "sqlite3_column_int");
        ARC_LOAD_SQLITE(column_text, "sqlite3_column_text");

#undef ARC_LOAD_SQLITE

        g_sqlite.ok = true;
        return true;
    }
}

// 查定数（10 倍值）；0 = 难度不存在。表按 songId 升序，二分查找
static int lookupConstantX10(const char *songId, int difficulty)
{
    if (!songId || difficulty < 0 || difficulty >= kMaxDifficulty)
        return 0;

    int lo = 0;
    int hi = kSongConstantCount - 1;
    while (lo <= hi)
    {
        const int mid = lo + (hi - lo) / 2;
        const SongConstant &e = kSongConstants[mid];
        const int cmp = strcmp(e.songId, songId);
        if (cmp < 0)
        {
            lo = mid + 1;
            continue;
        }
        if (cmp > 0)
        {
            hi = mid - 1;
            continue;
        }
        switch (difficulty)
        {
        case 0:
            return e.pst;
        case 1:
            return e.prs;
        case 2:
            return e.ftr;
        case 3:
            return e.byd;
        case 4:
            return e.etr;
        default:
            return 0;
        }
    }
    return 0;
}

// 单曲 ptt，返回内部单位（1 ptt = kUnit）
//
// 官方公式：
//   分数 >= 1e7          : 定数 + 2 + 奖励
//   9.8e6 <= 分数 < 1e7  : 定数 + 1 + (分数-9.8e6)/2e5 + 奖励
//   分数 < 9.8e6         : max(0, 定数 + (分数-9.5e6)/3e5 + 奖励)

static int64_t playPttUnit(int constantX10, int score, int clearType)
{
    int64_t value = static_cast<int64_t>(constantX10) * kConstToUnit;

    if (score >= kScoreMax)
        value += 2 * kUnit; // 满分 +2.0
    else if (score >= kScoreHigh)
        // +1.0 再 +(score-9.8e6)/2e5，单位换算后即 *15
        value += kUnit + static_cast<int64_t>(score - kScoreHigh) * 15;
    else
        // (score-9.5e6)/3e5，单位换算后即 *10
        value += static_cast<int64_t>(score - kScoreBase) * 10;

    if (clearType != 0)
        value += 600000; // 通关 +0.2

    // 负值一律取 0：负数没有意义，也不参与 b50（调用方按 <= 0 淘汰）
    return value < 0 ? 0 : value;
}

struct PlayEntry
{
    int songIdOffset; // 指向 g_songIdPool
    int difficulty;
    int score;
    int clearType;
};

static char g_songIdPool[kMaxEntries * kMaxSongIdLen];
static int g_songIdPoolUsed = 0;
static int64_t g_playPtt[kMaxEntries];
static PlayEntry g_plays[kMaxEntries];

static bool readPlays(void *db, PlayEntry *plays, int &playCount)
{
    static const char kSql[] =
        "SELECT s.songId, s.songDifficulty, s.score, IFNULL(c.clearType, 0)"
        " FROM scores s"
        " LEFT JOIN cleartypes c"
        " ON c.songId = s.songId AND c.songDifficulty = s.songDifficulty";

    void *stmt = nullptr;
    if (g_sqlite.prepare_v2(db, kSql, -1, &stmt, nullptr) != kSqliteOk)
        return false;

    playCount = 0;
    int src = kSqliteRow;
    while ((src = g_sqlite.step(stmt)) == kSqliteRow && playCount < kMaxEntries)
    {
        const unsigned char *text = g_sqlite.column_text(stmt, 0);
        if (!text)
            continue;

        // songId 直接拷进池里，PlayEntry 只存偏移（避免每行都分配）
        const size_t len = strlen(reinterpret_cast<const char *>(text));
        if (len == 0 || len >= static_cast<size_t>(kMaxSongIdLen))
            continue;
        if (g_songIdPoolUsed + static_cast<int>(len) + 1 > static_cast<int>(sizeof(g_songIdPool)))
            continue;

        const int offset = g_songIdPoolUsed;
        memcpy(g_songIdPool + offset, text, len + 1);
        g_songIdPoolUsed += static_cast<int>(len) + 1;

        PlayEntry &e = plays[playCount++];
        e.songIdOffset = offset;
        e.difficulty = g_sqlite.column_int(stmt, 1);
        e.score = g_sqlite.column_int(stmt, 2);
        e.clearType = g_sqlite.column_int(stmt, 3);
    }

    // 100 = ROW（可能只是到了快照上限），101 = DONE；其它值都是真错误
    const bool stepOk = (src == kSqliteRow || src == kSqliteDone);

    g_sqlite.finalize(stmt);
    return stepOk;
}

// 返回内部单位；kPttFailed = 算不出来，否则 >= 0 是真实结果（可以为 0）
static int64_t computePttUnit(void)
{
    if (!loadSqlite())
        return kPttFailed;

    void *db = nullptr;
    if (g_sqlite.open_v2(st3Path(), &db, kSqliteOpenReadOnly, nullptr) != kSqliteOk || !db)
    {
        if (db)
            g_sqlite.close(db);
        return kPttFailed;
    }

    g_songIdPoolUsed = 0;
    int playCount = 0;
    const bool playsOk = readPlays(db, g_plays, playCount);

    g_sqlite.close(db);

    if (!playsOk)
        return kPttFailed; // 读取失败，不是没成绩

    if (playCount <= 0)
        return 0; // 库能读但没有任何成绩，真实结果 0

    int valueCount = 0;
    for (int i = 0; i < playCount; ++i)
    {
        // 不能按 score 过滤：score 可以为 0 且通关
        //  未通关的 0 分记录会算出 0，由下面的 value <= 0 自然淘汰
        // songId 未收录或该难度不存在则无视这条成绩
        const int constantX10 =
            lookupConstantX10(g_songIdPool + g_plays[i].songIdOffset, g_plays[i].difficulty);
        if (constantX10 <= 0)
            continue;

        const int64_t value = playPttUnit(constantX10, g_plays[i].score, g_plays[i].clearType);
        if (value <= 0)
            continue;
        g_playPtt[valueCount++] = value;
    }

    if (valueCount <= 0)
        return 0; // 有成绩但没有一条能参与 b50，真实结果 0

    // 降序（插入排序，实际条目数远小于上限）
    for (int i = 1; i < valueCount; ++i)
    {
        const int64_t key = g_playPtt[i];
        int j = i - 1;
        while (j >= 0 && g_playPtt[j] < key)
        {
            g_playPtt[j + 1] = g_playPtt[j];
            --j;
        }
        g_playPtt[j + 1] = key;
    }

    int64_t sum = 0;
    for (int i = 0; i < valueCount && i < kBestCount; ++i)
        sum += g_playPtt[i];
    for (int i = 0; i < valueCount && i < kBestTopCount; ++i)
        sum += g_playPtt[i];

    return sum;
}

// 计算入口（带缓存）
// 算不出来返回 -2，不锁存，下次渲染重试
// 算出 0 锁存，界面显示 0.000

static int g_cached = 0;
static bool g_attempted = false;

// 返回 ptt 乘 1000 的值；-2 = 算不出来（不缓存，下次重试），>= 0 = 真实值。
static int pttGetX1000(void)
{
    if (g_attempted)
        return g_cached;

    const int64_t unit = computePttUnit();
    if (unit == kPttFailed)
        return -2; // ← 不锁存，下次重新尝试

    // 内部单位换成展示值，直接截断小数位不四舍五入
    int64_t shown = unit / kUnitToShown;
    if (shown < 0)
        shown = 0;
    g_cached = static_cast<int>(shown);
    g_attempted = true; // ← 只有成功才锁存
    return g_cached;
}

static void pttInvalidate(void)
{
    g_attempted = false;
    g_cached = 0;
}

// 开关 + 面板接管

// 潜伏开关：读档 + 判断是否启用（getPrefs 是懒加载，必须先调）
static bool showPotentialOn(void)
{
    getPrefs::ShowPotential();
    return settingsIsShowPotential() != 0;
}

typedef uintptr_t (*PFN_Panel)(void *self);
typedef void (*PFN_OfflineResult)(void *scene, int flag);
// TopBar::init(x0=this, x1=std::string*, w2, w3, w4, x5=delegate, w6)，共 7 个参数，返回 bool。
typedef bool (*PFN_TopBarInit)(void *self, void *name, int a2, int a3, int a4, void *delegate,
                               int a6);

static Trampoline g_panel;
static Trampoline g_offlineResult;
static Trampoline g_topBarInit;

// TopBar 实例（onTopBarInit 里捕获）。RatingChange 挂到它上面，层级/坐标天然正确
static void *g_topBar = nullptr;

// 上一次展示给玩家的 ptt（结算变化量的基准）
static int g_shownPtt = -1;
// 结算时算出、等顶栏渲染那一刻再挂 UI 的变化量
static int g_pendingDelta = 0;
static int g_pendingPtt = -1;
static bool g_pendingValid = false;

// 最近一次渲染过 ptt 的面板，开关切换时直接对它重跑 setupRating
static void *g_lastPanel = nullptr;

// 被临时改过的字段，跑完原函数要原样放回
struct PanelPatch
{
    int oldPtt = 0;
    bool pttFilled = false;
};

// 游戏只以离线形态（kind=3）运行，所以只需把 kind 临时抬到 2 让 setupRating 建节点。
//
// 结算时先渲染旧值：animateRatingUpdate 的中间回调会重跑 setupRating，
// 只有那一刻 user+0x44 是新值，数字才会经历「淡出 -> 变新值 -> 淡入」。
//
// onlyOurs：结算页顶栏会建多个 UserCell（好友头像等），它们同样会进 setupRating；
// 只有我们自己的评分面板才能做旧值替换。
static void preparePanel(void *self, PanelPatch &patch, int ptt, bool onlyOurs)
{
    *reinterpret_cast<int *>(static_cast<char *>(self) + kOffCellKind) = kKindOnline;

    void *user = *reinterpret_cast<void **>(static_cast<char *>(self) + kOffCellUser);
    if (!user)
        return;

    if (onlyOurs && g_pendingValid && g_pendingDelta != 0 && g_shownPtt >= 0)
        ptt = g_shownPtt;

    int *field = reinterpret_cast<int *>(static_cast<char *>(user) + kOffUserPtt);
    patch.oldPtt = *field;
    *field = ptt;
    patch.pttFilled = true;
}

static void restorePanel(void *self, const PanelPatch &patch)
{
    *reinterpret_cast<int *>(static_cast<char *>(self) + kOffCellKind) = kKindOffline;

    if (!patch.pttFilled)
        return;

    void *user = *reinterpret_cast<void **>(static_cast<char *>(self) + kOffCellUser);
    if (!user)
        return;
    *reinterpret_cast<int *>(static_cast<char *>(user) + kOffUserPtt) = patch.oldPtt;
}

// 面板根节点（panel+0x4A8，不是 +0x498 那个 user 指针）
static void *panelRootOf(void *panel)
{
    return *reinterpret_cast<void **>(static_cast<char *>(panel) + kOffCellRoot);
}

// 面板根节点下的 ptt 文本容器（只有真显示评分的面板才有）
static void *pttContainerOf(void *panel)
{
    void *root = panelRootOf(panel);
    return root ? childByName(root, "rating_text_container") : nullptr;
}

// 隐藏 ptt 整块 UI（关闭功能时用，不显示 rating_off，直接消失）。
// 结构：panel+0x4A8 -> rating_bg（徽章）/ rating_text_container（整数+小数位）
// 恢复不需要手动打开：setupRating 一开头就无条件 setVisible(1) 这两块。
static void hideRatingUi(void *panel)
{
    if (!panel)
        return;

    void *root = panelRootOf(panel);
    if (!root)
        return;

    void *bg = childByName(root, "rating_bg");
    if (bg)
        gameVcall<void (*)(void *, int)>(bg, OFF_VT_SET_VISIBLE, 0);

    void *box = childByName(root, "rating_text_container");
    if (box)
        gameVcall<void (*)(void *, int)>(box, OFF_VT_SET_VISIBLE, 0);

    // 变化量节点可能挂在 TopBar / 容器 / 根节点，一并藏掉（不销毁，showRatingChange 会复用）
    void *candidates[3] = {g_topBar, box, root};
    for (void *p : candidates)
    {
        if (!p)
            continue;
        void *change = childByName(p, "rating_change");
        if (change)
            gameVcall<void (*)(void *, int)>(change, OFF_VT_SET_VISIBLE, 0);
    }
}

// 变化量节点：贴图 keep/up/down，"+ x.xxx" 文案，0.6 缩放，FadeIn(0.7) + MoveBy(0,-10)
typedef void *(*PFN_LoadCsb)(void *gameStr);
typedef void (*PFN_SetString)(void *node, void *gameStr);
typedef void (*PFN_SetColor)(void *node, const void *color, int flag);
typedef void *(*PFN_MakeFade)(float duration);
typedef void *(*PFN_MakeMove)(const void *pos, float duration);
typedef void *(*PFN_WrapAction)(void *action);
typedef void *(*PFN_Spawn3)(void *, void *, void *);
typedef void (*PFN_AnimateRating)(void *self, int delta);

static void showRatingChange(void *parent, int delta)
{
    if (!parent)
        return;

    // 复用同名节点：历史坑是每场结算都 addChild 一棵新树，既叠字又泄漏。
    // 本版 so 已 strip，没有可靠的 removeChild 地址，只能用复用。
    void *node = childByName(parent, "rating_change");
    const bool reused = (node != nullptr);
    if (!reused)
    {
        withGameStr("layouts/results/RatingChange.csb", [&](void *str)
                    {
            node = reinterpret_cast<PFN_LoadCsb>(g_base + OFF_CREATE_UI_NODE)(str);
            return true; });
        if (!node)
            return;

        // 加载后必须 update 整棵子树，否则锚点/尺寸/贴图都是未初始化状态
        reinterpret_cast<void (*)(void *)>(g_base + OFF_NodeUpdateTree)(node);
    }

    void *bg = childByName(node, "bg");
    void *text = childByName(node, "text");
    void *keepText = childByName(node, "keep_text");

    const char *texName = "layouts/results/rating_keep.png";
    if (delta > 0)
        texName = "layouts/results/rating_up.png";
    else if (delta < 0)
        texName = "layouts/results/rating_down.png";

    if (bg)
        withGameStr(texName, [&](void *str)
                    {
            gameVcall<void (*)(void *, void *)>(bg, OFF_VT_SET_TEXTURE, str);
            return true; });

    if (text)
    {
        if (delta == 0)
        {
            gameVcall<void (*)(void *, int)>(text, OFF_VT_SET_VISIBLE, 0);
        }
        else
        {
            char buf[32];
            const int absD = delta < 0 ? -delta : delta;
            snprintf(buf, sizeof(buf), "%c %d.%03d", delta > 0 ? '+' : '-', absD / 1000,
                     absD % 1000);
            withGameStr(buf, [&](void *str)
                        {
                reinterpret_cast<PFN_SetString>(g_base + OFF_LabelSetString)(text, str);
                return true; });

            unsigned char color[4];
            if (delta > 0)
            {
                color[0] = 0x03;
                color[1] = 0x85;
                color[2] = 0x85;
            }
            else
            {
                color[0] = 0xD4;
                color[1] = 0x48;
                color[2] = 0x5B;
            }
            color[3] = 0xFF;

            reinterpret_cast<PFN_SetColor>(g_base + OFF_LabelSetColor)(text, color, 1);
            gameVcall<void (*)(void *, int)>(text, OFF_VT_SET_VISIBLE, 1);
        }

        if (keepText)
            gameVcall<void (*)(void *, int)>(keepText, OFF_VT_SET_VISIBLE, delta == 0 ? 1 : 0);
    }

    gameVcall<void (*)(void *, float)>(node, OFF_VT_SET_SCALE, 0.6f);

    // 位置：ptt 数字右侧。Up 时再偏一点，避免和数字重叠
    float visible[2] = {0.0f, 0.0f};
    callGetVisibleSize(visible, reinterpret_cast<void *>(g_base + OFF_GetVisibleSize));
    float pos[2] = {visible[0] * 0.5f + 110.0f, visible[1] + 10.0f};
    if (parent != g_topBar && delta > 0)
    {
        pos[0] += 25.0f;
        pos[1] += -5.0f;
    }
    gameVcall<void (*)(void *, const void *)>(node, OFF_VT_SET_POSITION, pos);

    gameVcall<void (*)(void *, int)>(node, OFF_VT_SET_VISIBLE, 1);
    gameVcall<void (*)(void *, int)>(node, OFF_VT_SET_CASCADE_OPACITY, 1);

    if (reused)
    {
        // 复用不播动作：拿不到 stopAllActions，旧 MoveBy 会叠加把节点推走
        gameVcall<void (*)(void *, int)>(node, OFF_VT_SET_OPACITY, 255);
        return;
    }

    // 新建：必须在 addChild 之前设好 opacity，否则 onEnter 时会被冲掉
    gameVcall<void (*)(void *, int)>(node, OFF_VT_SET_OPACITY, 0);
    void *fade = reinterpret_cast<PFN_MakeFade>(g_base + OFF_MakeFadeIn)(0.7f);
    const float moveVec[2] = {0.0f, -10.0f};
    void *move = reinterpret_cast<PFN_MakeMove>(g_base + OFF_MakeMoveBy)(moveVec, 0.7f);

    if (fade && move)
    {
        void *wrapped = reinterpret_cast<PFN_WrapAction>(g_base + OFF_WrapAction)(move);
        void *spawn =
            reinterpret_cast<PFN_Spawn3>(g_base + OFF_SpawnCreate)(fade, wrapped, nullptr);
        gameVcall<void (*)(void *, void *)>(node, OFF_VT_RUN_ACTION, spawn ? spawn : fade);
    }
    else if (fade)
    {
        gameVcall<void (*)(void *, void *)>(node, OFF_VT_RUN_ACTION, fade);
    }

    // 压到最上层（原版 z=1 会被顶栏盖住）
    withGameStr("rating_change", [&](void *str)
                {
        gameVcall<void (*)(void *, void *, int, void *)>(parent, OFF_VT_ADD_CHILD_NAMED, node,
                                                     0x7FFFFFF, str);
        return true; });
}

static void playPttChange(void *panel, int delta, int newPtt)
{
    if (!panel)
        return;

    // restorePanel 已把 user+0x44 恢复成旧值，这里写新值，
    // 让 1 秒后 setupRating 回调读到新值，数字才会在淡出后变化。
    void *user = *reinterpret_cast<void **>(static_cast<char *>(panel) + kOffCellUser);
    if (user)
        *reinterpret_cast<int *>(static_cast<char *>(user) + kOffUserPtt) = newPtt;

    reinterpret_cast<PFN_AnimateRating>(g_base + OFF_UserCellAnimateRating)(panel, delta);
}

void potentialOnToggled(int on)
{
    pttInvalidate();
    g_pendingValid = false;

    void *panel = g_lastPanel;
    if (!panel)
        return;

    if (!on)
    {
        hideRatingUi(panel);
        return;
    }

    // 开启：算不出来同样退化成无数据形态
    const int v = pttGetX1000();
    const int ptt = (v == -2) ? kPttNoData : v;

    PanelPatch patch;
    preparePanel(panel, patch, ptt, true);
    reinterpret_cast<PFN_Panel>(g_panel.cave)(panel);
    restorePanel(panel, patch);

    // 同步结算基准，否则重开后第一场结算的变化量会拿旧值当基准
    if (ptt >= 0)
        g_shownPtt = ptt;
}

// 评分面板入口。
// 开关关闭：完全透传（kind=3 让 setupRating 提前返回），但仍要藏掉 csb 自带的节点。
// 算不出来（-2）：填 -1，显示 rating_off + "--"。
// 算出值：填真实 ptt。
static uintptr_t onPanel(void *self)
{
    if (!self)
        return reinterpret_cast<PFN_Panel>(g_panel.cave)(self);

    const bool ours = (pttContainerOf(self) != nullptr);
    if (ours || !g_lastPanel)
        g_lastPanel = self;

    if (!showPotentialOn())
    {
        hideRatingUi(self);
        return reinterpret_cast<PFN_Panel>(g_panel.cave)(self);
    }

    int ptt = pttGetX1000();
    const bool painted = (ptt == -2);
    if (painted)
        ptt = kPttNoData;

    PanelPatch patch;
    preparePanel(self, patch, ptt, ours);

    const uintptr_t ret = reinterpret_cast<PFN_Panel>(g_panel.cave)(self);

    restorePanel(self, patch);

    // 画成「无数据」时不参与动画。必须清 pending：结算时算成功、
    // 渲染时却算失败会让标记滞留，之后拿过期旧值覆盖真实 ptt。
    if (painted)
    {
        g_pendingValid = false;
        return ret;
    }

    // 此刻 ptt 数字刚被写进标签，在这一刻挂变化量节点才能和数字动画同步
    if (g_pendingValid && ours)
    {
        // 父节点必须是真正的 Node。不能传 self（UserCell 对象本身，
        // 对它 addChild 会写坏面板，导致头像无法点击）。
        void *container = pttContainerOf(self);
        void *parent = g_topBar ? g_topBar : (container ? container : panelRootOf(self));
        showRatingChange(parent, g_pendingDelta);

        if (g_pendingDelta != 0)
            playPttChange(self, g_pendingDelta, g_pendingPtt);

        // 基准推进到新值，否则「结算完直接再打一场」时 delta 会重复累加
        g_shownPtt = g_pendingPtt;
        g_pendingValid = false;
    }
    else if (ours)
    {
        // 主界面刷新：记下本次 ptt 作为下一场结算的基准（结算期间不记，否则冲掉基准）
        g_shownPtt = ptt;
    }

    return ret;
}

// 结算：st3 重算。离线结算分支会在本函数内入库，所以重算必须放在它返回之后。
static void onOfflineResult(void *scene, int flag)
{
    const int oldPtt = g_shownPtt;

    reinterpret_cast<PFN_OfflineResult>(g_offlineResult.cave)(scene, flag);

    if (!showPotentialOn())
    {
        g_pendingValid = false;
        return;
    }

    pttInvalidate();
    const int newPtt = pttGetX1000();

    // -2 = 算不出来，不能拿 0 当结果，也不挂变化量
    if (newPtt == -2)
    {
        g_pendingValid = false;
        return;
    }

    // 只记录，等结算顶栏真正渲染出数字的那一刻再挂 UI（见 onPanel）
    g_pendingPtt = newPtt;
    g_pendingDelta = (oldPtt >= 0) ? (newPtt - oldPtt) : 0;
    g_pendingValid = true;
}

// TopBar::init 入口：x0 就是 TopBar 实例。每次 init 都是新 TopBar，旧面板一并清掉。
static bool onTopBarInit(void *self, void *name, int a2, int a3, int a4, void *delegate, int a6)
{
    g_topBar = self;
    g_lastPanel = nullptr;
    return reinterpret_cast<PFN_TopBarInit>(g_topBarInit.cave)(self, name, a2, a3, a4, delegate,
                                                               a6);
}

void installPotentialHook()
{
    g_panel = installTrampoline(g_base + OFF_RatingPanel, (void *)&onPanel);
    if (!g_panel.ok)
        return; // 补丁失败就什么都不做，绝不让 hook 里跳 cave(0)

    g_offlineResult = installTrampoline(g_base + OFF_OfflineResult, (void *)&onOfflineResult);
    if (!g_offlineResult.ok)
        return;

    g_topBarInit = installTrampoline(g_base + OFF_TopBarInit, (void *)&onTopBarInit);
}
