// 潜力值（ptt）：b50 计算 + 展示开关。
//
// 评分面板是 UserCell：
//   panel+0x498 = user 对象；user+0x44 = ptt（×1000 int）
//   panel+0x4A0 = kind：2 = 在线形态，3 = 离线形态

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

static const char *const kSt3Path = OBB_DIR "st3";

// UserCell 字段（非指令地址，跨版本稳定）
static constexpr size_t kOffCellUser = 0x498; // panel -> user 对象
static constexpr size_t kOffCellKind = 0x4A0; // panel -> 形态
static constexpr size_t kOffUserPtt = 0x44;   // user -> ptt×1000

static constexpr int kKindOnline = 2; // 2 = 建评分面板

// 单次成绩分档
static const int kScoreMax = 10000000;
static const int kScoreHigh = 9800000;
static const int kScoreBase = 9500000;

// 内部精度：1 ptt = kUnitPerPtt 单位。取 3e6 是为了让公式里的 /3e5 变成精确乘法
// 只在最后换成 ×1000 展示值时做一次整除
static const int64_t kUnitPerPtt = 3000000;
static const int64_t kUnitFullBonus = 2 * kUnitPerPtt; // 满分 +2.0
static const int64_t kUnitHighBase = 1 * kUnitPerPtt;  // 9.8e6 档 +1.0
static const int64_t kUnitClearBonus = 600000;         // 通关 +0.2

// 定数表存 ×10，还原成内部单位要 ×300000。
static const int64_t kConstToUnit = 300000;

// 汇总
static const int kBestCount = 50;
static const int kBestTopCount = 10;
// (Σunit / 60) / 3e6 * 1000 = Σunit / 180000
static const int64_t kUnitToShown = 180000;

// 难度上限（PST/PRS/FTR/BYD/ETR）与表容量
static const int kMaxDifficulty = 5;
static const int kMaxEntries = 8192;
static const int kMaxSongIdLen = 48;

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

// 取第 difficulty 档的定数 ×10；越界返回 0。
static int16_t constantX10Of(const SongConstant &e, int difficulty)
{
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
    default:
        return e.etr;
    }
}

// 查定数（×10）；0 = 未收录 / 该难度不存在（游戏里定数不会为 0，可当哨兵）
static int16_t lookupConstantX10(const char *songId, int difficulty)
{
    if (!songId || difficulty < 0 || difficulty >= kMaxDifficulty)
        return 0;

    int lo = 0;
    int hi = kSongConstantCount - 1;
    while (lo <= hi)
    {
        const int mid = lo + (hi - lo) / 2;
        const int cmp = strcmp(kSongConstants[mid].songId, songId);
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
        return constantX10Of(kSongConstants[mid], difficulty);
    }
    return 0;
}

// 返回内部单位（1 ptt = 3e6）；全程整数、无截断
static int64_t playPttUnit(int16_t constantX10, int score, int clearType)
{
    int64_t value = static_cast<int64_t>(constantX10) * kConstToUnit;

    if (score >= kScoreMax)
        value += kUnitFullBonus;
    else if (score >= kScoreHigh)
        // +1.0 再 + (score-9.8e6)/2e5 → 单位: /2e5 * 3e6 = *15
        value += kUnitHighBase + static_cast<int64_t>(score - kScoreHigh) * 15;
    else
        // (score-9.5e6)/3e5 → 单位: /3e5 * 3e6 = *10（精确）
        value += static_cast<int64_t>(score - kScoreBase) * 10;

    if (value < 0)
        value = 0;

    if (clearType != 0)
        value += kUnitClearBonus;

    return value < 0 ? 0 : value;
}

namespace
{
    struct PlayEntry
    {
        int songIdOffset; // 指向 g_songIdPool
        int difficulty;
        int score;
        int clearType;
    };
}

static char g_songIdPool[kMaxEntries * kMaxSongIdLen];
static int g_songIdPoolUsed = 0;

static int internSongId(const char *id)
{
    const size_t len = strlen(id);
    if (len == 0 || len >= static_cast<size_t>(kMaxSongIdLen))
        return -1;
    if (g_songIdPoolUsed + static_cast<int>(len) + 1 > static_cast<int>(sizeof(g_songIdPool)))
        return -1;

    const int offset = g_songIdPoolUsed;
    memcpy(g_songIdPool + offset, id, len + 1);
    g_songIdPoolUsed += static_cast<int>(len) + 1;
    return offset;
}

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
    while (g_sqlite.step(stmt) == kSqliteRow && playCount < kMaxEntries)
    {
        const unsigned char *text = g_sqlite.column_text(stmt, 0);
        if (!text)
            continue;

        const int offset = internSongId(reinterpret_cast<const char *>(text));
        if (offset < 0)
            continue;

        PlayEntry &e = plays[playCount++];
        e.songIdOffset = offset;
        e.difficulty = g_sqlite.column_int(stmt, 1);
        e.score = g_sqlite.column_int(stmt, 2);
        e.clearType = g_sqlite.column_int(stmt, 3);
    }

    g_sqlite.finalize(stmt);
    return true;
}

namespace
{
    int64_t g_playPtt[kMaxEntries];
    PlayEntry g_plays[kMaxEntries];
}

static void sortDesc(int64_t *values, int count)
{
    // 插入排序
    for (int i = 1; i < count; ++i)
    {
        const int64_t key = values[i];
        int j = i - 1;
        while (j >= 0 && values[j] < key)
        {
            values[j + 1] = values[j];
            --j;
        }
        values[j + 1] = key;
    }
}

// 返回内部单位；-1 = 数据缺失（无库 / 无成绩）
static int64_t computePttUnit(void)
{
    if (!loadSqlite())
        return -1;

    void *db = nullptr;
    if (g_sqlite.open_v2(kSt3Path, &db, kSqliteOpenReadOnly, nullptr) != kSqliteOk || !db)
    {
        if (db)
            g_sqlite.close(db);
        return -1;
    }

    g_songIdPoolUsed = 0;
    int playCount = 0;
    const bool playsOk = readPlays(db, g_plays, playCount);

    g_sqlite.close(db);

    if (!playsOk || playCount <= 0)
        return -1;

    int valueCount = 0;
    for (int i = 0; i < playCount; ++i)
    {
        // 不能按 score 过滤：score 可以为 0 且通关
        //  未通关的 0 分记录会算出 0，由下面的 value <= 0 自然淘汰
        // songId 未收录 / 该难度不存在 → 无视这条成绩
        const int16_t constantX10 =
            lookupConstantX10(g_songIdPool + g_plays[i].songIdOffset, g_plays[i].difficulty);
        if (constantX10 <= 0)
            continue;

        const int64_t value = playPttUnit(constantX10, g_plays[i].score, g_plays[i].clearType);
        if (value <= 0)
            continue;
        g_playPtt[valueCount++] = value;
    }

    if (valueCount <= 0)
        return -1;

    sortDesc(g_playPtt, valueCount);

    int64_t sum = 0;
    for (int i = 0; i < valueCount && i < kBestCount; ++i)
        sum += g_playPtt[i];
    for (int i = 0; i < valueCount && i < kBestTopCount; ++i)
        sum += g_playPtt[i];

    return sum;
}

// 计算入口（带缓存）
//
// st3 唯一会被写的时机 = 结算时成绩入库（onOfflineResult），
// 那里已经显式调 pttInvalidate() ⇒ 不需要每次渲染都 stat() 一遍文件。
// （setupRating 在选歌页/顶栏/好友单元格上会被反复调用，
//   逐次 stat 会变成纯浪费的 syscall

static int g_cached = 0;
static bool g_attempted = false;

// 返回 ptt ×1000；无成绩 / 无库时返回 0（界面显示 0.000）。
static int pttGetX1000(void)
{
    if (g_attempted)
        return g_cached;

    g_attempted = true;

    const int64_t unit = computePttUnit();
    if (unit < 0)
    {
        g_cached = 0;
        return g_cached;
    }

    // 内部单位 → ×1000（直接截断小数位，不四舍五入）
    int64_t shown = unit / kUnitToShown;
    if (shown < 0)
        shown = 0;
    g_cached = static_cast<int>(shown);
    return g_cached;
}

static void pttInvalidate(void)
{
    g_attempted = false;
    g_cached = 0;
}

// 开关 + 面板接管

// 潜伏开关：读档 + 判断是否启用（getPrefs 是懒加载，必须先调）。
static bool showPotentialOn(void)
{
    getPrefs::ShowPotential();
    return settingsIsShowPotential() != 0;
}

typedef uintptr_t (*PFN_Panel)(void *self);
typedef void (*PFN_OfflineResult)(void *scene, int flag);
typedef uintptr_t (*PFN_TopBarInit)(void *self, void *name, int a3, int a4, void *delegate,
                                    int a6);

static Trampoline g_panel;
static Trampoline g_offlineResult;
static Trampoline g_topBarInit;

// TopBar 实例（onTopBarInit 里捕获）。RatingChange 挂到它上面 -> 层级/坐标天然正确。
static void *g_topBar = nullptr;

// 上一次展示给玩家的 ptt（结算变化量的基准）。
static int g_shownPtt = -1;
// 结算时算出、等顶栏渲染那一刻再挂 UI 的变化量。
static int g_pendingDelta = 0;
static int g_pendingPtt = -1;
static bool g_pendingValid = false;

// 被临时改过的字段，跑完原函数要原样放回。
struct PanelPatch
{
    int oldKind = 0;
    int oldPtt = 0;
    bool pttFilled = false;
};

// 游戏只以离线形态（kind=3）运行，所以这里只做「离线 → 在线」的临时抬升。
//
// 结算时先渲染旧值：animateRatingUpdate 的中间回调会重跑 setupRating，
//   只有那一刻 user+0x44 是新值，数字才会经历「淡出 → 变新值 → 淡入」。
//   若此处直接填新值，数字从第一帧就是新值 → 看起来完全没动画
static void preparePanel(void *self, PanelPatch &patch, int ptt)
{
    int *kind = reinterpret_cast<int *>(static_cast<char *>(self) + kOffCellKind);
    patch.oldKind = *kind;
    *kind = kKindOnline;

    void *user = *reinterpret_cast<void **>(static_cast<char *>(self) + kOffCellUser);
    if (!user)
        return; // 没有 user 对象就只能放开形态，ptt 无从补起

    if (g_pendingValid && g_pendingDelta != 0 && g_shownPtt >= 0)
        ptt = g_shownPtt; // 结算首帧用旧值

    int *field = reinterpret_cast<int *>(static_cast<char *>(user) + kOffUserPtt);
    patch.oldPtt = *field;
    *field = ptt;
    patch.pttFilled = true;
}

static void restorePanel(void *self, const PanelPatch &patch)
{
    *reinterpret_cast<int *>(static_cast<char *>(self) + kOffCellKind) = patch.oldKind;

    if (!patch.pttFilled)
        return;

    void *user = *reinterpret_cast<void **>(static_cast<char *>(self) + kOffCellUser);
    if (!user)
        return;
    *reinterpret_cast<int *>(static_cast<char *>(user) + kOffUserPtt) = patch.oldPtt;
}

// 面板根节点下是否有 ptt 文本容器
static bool hasRatingContainer(void *panel)
{
    if (!panel)
        return false;
    void *root = *reinterpret_cast<void **>(static_cast<char *>(panel) + kOffCellUser);
    if (!root)
        return false;
    return childByName(root, "rating_text_container") != nullptr;
}

// 结算变化量节点（RatingChange.csb）

typedef void *(*PFN_LoadCsb)(void *gameStr);
typedef void (*PFN_SetString)(void *node, void *gameStr);
typedef void (*PFN_SetColor)(void *node, const void *color, int flag);
typedef void *(*PFN_MakeFade)(float duration);
typedef void *(*PFN_MakeMove)(const void *pos, float duration);
typedef void *(*PFN_WrapAction)(void *action);
typedef void *(*PFN_Spawn3)(void *, void *, void *);
typedef void (*PFN_AnimateRating)(void *self, int delta);

// 变化量节点缩放（原版是 0.6f）
static const float kRatingChangeScale = 0.6f;
// 压到最上层（原版 z=1 会被顶栏盖住）。
static const int kRatingChangeZOrder = 0x7FFFFFF;

// 自建 RatingChange 节点并挂到 parent 下：
//   贴图 keep/up/down + "+ x.xxx" 文案 + 配色 + 0.6 缩放 + FadeIn(0.7) + MoveBy(0,-10)
static void showRatingChange(void *parent, int delta)
{
    if (!parent)
        return;

    // 1) 加载节点
    void *node = nullptr;
    withGameStr("layouts/results/RatingChange.csb", [&](void *str)
                {
        node = reinterpret_cast<PFN_LoadCsb>(g_base + OFF_CREATE_UI_NODE)(str);
        return true; });
    if (!node)
        return;

    // 加载后必须 update 整棵子树，否则锚点/尺寸/贴图都是未初始化状态。
    reinterpret_cast<void (*)(void *)>(g_base + OFF_NodeUpdateTree)(node);

    // 2) 子节点
    void *bg = childByName(node, "bg");
    void *text = childByName(node, "text");
    void *keepText = childByName(node, "keep_text");

    // 3) 贴图（相对名，让 FileUtils 搜索路径去解析）
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

    // 4) 文本 / 颜色 / keep_text 可见性
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

            // up = #038585FF，down = #D4485BFF
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

    // 5) 缩放 + 位置
    gameVcall<void (*)(void *, float)>(node, OFF_VT_SET_SCALE, kRatingChangeScale);

    float pos[2];
    if (parent == g_topBar)
    {
        // TopBar 的 contentSize = 屏幕尺寸 ⇒ 直接照抄游戏公式（无 up 偏移）
        float visible[2] = {0.0f, 0.0f};
        callGetVisibleSize(visible, reinterpret_cast<void *>(g_base + OFF_GetVisibleSize));
        pos[0] = visible[0] * 0.5f + 110.0f;
        pos[1] = visible[1] + 10.0f;
    }
    else
    {
        float visible[2] = {0.0f, 0.0f};
        callGetVisibleSize(visible, reinterpret_cast<void *>(g_base + OFF_GetVisibleSize));
        pos[0] = visible[0] * 0.5f + 110.0f;
        pos[1] = visible[1] + 10.0f;
        if (delta > 0)
        {
            pos[0] += 25.0f;
            pos[1] += -5.0f;
        }
    }
    gameVcall<void (*)(void *, const void *)>(node, OFF_VT_SET_POSITION, pos);

    // 6) 显示 + 淡入（必须在 addChild 之前设好 opacity，否则 onEnter 时会被冲掉）
    gameVcall<void (*)(void *, int)>(node, OFF_VT_SET_VISIBLE, 1);
    gameVcall<void (*)(void *, int)>(node, OFF_VT_SET_CASCADE_OPACITY, 1);
    gameVcall<void (*)(void *, int)>(node, OFF_VT_SET_OPACITY, 0);
    {
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
    }

    // 7) 最后挂载：先设好透明度/位姿/动作再 addChild，否则淡入会被 onEnter 冲掉。
    withGameStr("rating_change", [&](void *str)
                {
        gameVcall<void (*)(void *, void *, int, void *)>(parent, OFF_VT_ADD_CHILD_NAMED, node,
                                                     kRatingChangeZOrder, str);
        return true; });
}

// 让 ptt 数字本身播放「淡出 → 重跑 setupRating（读新值）→ 淡入」
// 直接调游戏自己的 UserCell::animateRatingUpdate
static void playPttChange(void *panel, int delta, int newPtt)
{
    if (!panel)
        return;

    // restorePanel 已把 user+0x44 恢复成旧值 ⇒ 这里写新值，
    // 让 1 秒后 setupRating 回调读到新值，数字才会在淡出后变化。
    void *user = *reinterpret_cast<void **>(static_cast<char *>(panel) + kOffCellUser);
    if (user)
        *reinterpret_cast<int *>(static_cast<char *>(user) + kOffUserPtt) = newPtt;

    reinterpret_cast<PFN_AnimateRating>(g_base + OFF_UserCellAnimateRating)(panel, delta);
}

// 评分面板入口：关闭时完全透传（kind=3 ⇒ 游戏自己就不建评分节点）
static uintptr_t onPanel(void *self)
{
    if (!self || !showPotentialOn())
        return reinterpret_cast<PFN_Panel>(g_panel.cave)(self);

    PanelPatch patch;
    preparePanel(self, patch, pttGetX1000());

    const uintptr_t ret = reinterpret_cast<PFN_Panel>(g_panel.cave)(self);

    restorePanel(self, patch);

    // 这里是「ptt 数字刚被写进标签」的那一刻（setupRating 内部读 user+0x44 → 写标签）
    //   在这一刻挂变化量节点 + 让数字淡出淡入，两者才完全同步。
    if (g_pendingValid && hasRatingContainer(self))
    {
        void *parent = g_topBar ? g_topBar : self;
        showRatingChange(parent, g_pendingDelta);

        if (g_pendingDelta != 0)
            playPttChange(self, g_pendingDelta, g_pendingPtt);

        g_pendingValid = false;
    }
    else if (hasRatingContainer(self))
    {
        // 主界面顶栏刷新：记下这次展示的 ptt，作为下一场结算的变化量基准。
        // 结算期间不记（走上面那条），否则入库后的新值会把基准冲掉 → delta 恒 0。
        g_shownPtt = pttGetX1000();
    }

    return ret;
}

// 结算：st3 重算
// 离线结算分支：成绩在本函数内入库，所以重算必须放在它返回之后。
static void onOfflineResult(void *scene, int flag)
{
    // 基准 = 上一次顶栏展示过的 ptt。不能在这里现算：成绩在进结算前已入库
    const int oldPtt = g_shownPtt;

    reinterpret_cast<PFN_OfflineResult>(g_offlineResult.cave)(scene, flag);

    if (!showPotentialOn())
    {
        g_pendingValid = false;
        return; // 关闭时不读 st3、不计算
    }

    // 成绩入库已完成 → 丢掉缓存并全量重算
    pttInvalidate();
    const int newPtt = pttGetX1000();

    // 只记录，不在此处挂 UI，要等结算顶栏真正渲染出数字的那一刻（见 onPanel）
    g_pendingPtt = newPtt;
    g_pendingDelta = (oldPtt >= 0 && newPtt >= 0) ? (newPtt - oldPtt) : 0;
    g_pendingValid = true;
    (void)scene;
}

// TopBar::init 入口：x0 就是 TopBar 实例
static uintptr_t onTopBarInit(void *self, void *name, int a3, int a4, void *delegate, int a6)
{
    g_topBar = self;
    return reinterpret_cast<PFN_TopBarInit>(g_topBarInit.cave)(self, name, a3, a4, delegate, a6);
}

void installPotentialHook()
{
    g_panel = installTrampoline(g_base + OFF_RatingPanel, (void *)&onPanel);
    g_offlineResult = installTrampoline(g_base + OFF_OfflineResult, (void *)&onOfflineResult);
    g_topBarInit = installTrampoline(g_base + OFF_TopBarInit, (void *)&onTopBarInit);
}
