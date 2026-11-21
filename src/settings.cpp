// 设置界面接管 + 全工程唯一存档读写处。
//   1. "Stamina Notification" 行 → Auto Mode 开关（键 autoPlay）。
//   2. "Show Potential" 行 → 我们自己的 ptt 是否展示（键 showPotential）。
//      两行都强行走"在线/可用"分支（NOP 掉各自的两道门），
//      并把取值 / 点击两个 lambda 整段替换成不碰游戏状态的实现。
//   3. "Invite Notification" 行由静态补丁从布局里摘掉（只跳它自己的 push_back）。
//   4. 两处纯展示改写：About 页 Credits 里插入一段曲目信息、登录页隐掉注册/登录按钮。
//
// autoPlay / showPotential 的值在游戏初始化角色时读取，参考 skill.cpp

#include <cstdint>
#include <cstring>

#include "game.hpp"
#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

static const char *PREF_KEY_AUTOPLAY = "autoPlay";
static const char *PREF_KEY_SHOWPOTENTIAL = "showPotential";
static const char *PREF_KEY_AWAKENED = "awakened";

static int g_autoPlay = 0;
// showPotential：1 = 展示我们算出的 ptt，0 = 不展示（我们定义的「禁用」）。
// 存档没读到时用这个初始值 ⇒ 默认「禁用」。
static int g_showPotential = 0;
static uint8_t g_awaken[g_char_num] = {0};
static bool g_autoPlayLoaded = false;
static bool g_showPotentialLoaded = false;
static bool g_awakenLoaded = false;

// 取值 / 归档

// g_showPotential：1 = 展示 ptt，0 = 不展示。
// 选项文案已交换：索引 0 的文案是 "Disabled"，而它在游戏里实际表示「启用」
// ⇒ 展示时返回 0（行上显示 "Disabled"）。

int settingsIsAutoPlay(void)
{
    return g_autoPlay;
}

int settingsIsShowPotential(void)
{
    return g_showPotential;
}

int settingsIsAwakened(int id)
{
    return (id >= 0 && id < g_char_num) ? g_awaken[id] : 0;
}

// 值变化后推给下游模块。两个 apply 都是幂等的，值没变直接返回。
static void applyToggle(int on)
{
#if enableAutoplayHook
    autoplayApply(on);
#if enableSecurityHook
    securityApply(on);
#endif
#else
    (void)on;
#endif
}

void settingsSetAwaken(int id, int on)
{
    if (id < 0 || id >= g_char_num)
        return;
    g_awaken[id] = on ? 1 : 0;
    setPrefs::Awakened();
}

namespace getPrefs
{
    // 非数字字符一律当分隔符；越界 id 直接丢弃（写入时也只输出 [0, g_char_num)）。
    void Awakened()
    {
        if (g_awakenLoaded)
            return;
        g_awakenLoaded = true;

        char buf[512];
        gameGetString(PREF_KEY_AWAKENED, "", buf, sizeof(buf));
        memset(g_awaken, 0, sizeof(g_awaken));

        int id = -1;
        for (const char *ch = buf;; ++ch)
        {
            if (*ch >= '0' && *ch <= '9')
            {
                // 先按 int 累加，超出范围的数字串走到分隔符时自然被丢掉。
                id = (id < 0 ? 0 : id) * 10 + (*ch - '0');
                if (id >= g_char_num)
                    id = g_char_num;
                continue;
            }

            if (id >= 0 && id < g_char_num)
                g_awaken[id] = 1;
            id = -1;

            if (!*ch)
                break;
        }
    }

    void AutoPlay()
    {
        if (g_autoPlayLoaded)
            return;
        g_autoPlayLoaded = true;
        g_autoPlay = gameGetBool(PREF_KEY_AUTOPLAY, 0);
#if enableAutoplayHook
        applyToggle(g_autoPlay);
#endif
    }

    void ShowPotential()
    {
        if (g_showPotentialLoaded)
            return;
        g_showPotentialLoaded = true;
        g_showPotential = gameGetBool(PREF_KEY_SHOWPOTENTIAL, 0);
    }
}

namespace setPrefs
{
    void Awakened()
    {
        char value[512];
        value[0] = 0;
        size_t len = 0;

        for (int id = 0; id < g_char_num; id++)
        {
            if (!g_awaken[id])
                continue;

            const int written = snprintf(value + len, sizeof(value) - len,
                                         "%s%d", len ? "," : "", id);
            if (written <= 0 || static_cast<size_t>(written) >= sizeof(value) - len)
                break;
            len += static_cast<size_t>(written);
        }

        gameSetString(PREF_KEY_AWAKENED, value);
    }

    void AutoPlay()
    {
        gameSetBool(PREF_KEY_AUTOPLAY, g_autoPlay);
    }

    void ShowPotential()
    {
        gameSetBool(PREF_KEY_SHOWPOTENTIAL, g_showPotential);
    }
}

// 设置界面借用的两行
//
// 两行结构完全一样：在 createMainContentBase 里各被建两遍，唯一分叉是两道
// 「在线数据可用吗」的门：
//   tbz w0,#0,<离线块>   -> 不满足则走离线变体（只有 "Unavailable Offline"，点不动）
// 要让开关能开能关，只能走在线变体，所以两行的两条 tbz 一律 NOP。
// 只 NOP 不够：在线变体的取值 lambda 会解引用账号配置，未登录时是 NULL
//   ⇒ 同时把取值 / 点击两个 lambda 整段替换成不碰游戏状态的实现。
//
// 取值 lambda 序言含 adrp（PC 相对），不能蹦床，只能 replaceFunction 整段替换。
// 点击 lambda 原实现走网络提交，替换后只做：翻转值 -> 落盘 -> 返回新值。
//
// 行可见性：
//   Invite Notification（设置界面最后一行）已由静态补丁跳过它自己的 push_back，
//   运行时不需要再做任何事；Stamina 行保持原样显示。

int onAutoPlayGet(void *callable)
{
    (void)callable;
    // 读档放在这里：界面正在显示这一行，说明 Helper 已经就绪。
    getPrefs::AutoPlay();
    return g_autoPlay;
}

int onAutoPlayTap(void *callable)
{
    (void)callable;

    g_autoPlay = g_autoPlay ? 0 : 1;
    applyToggle(g_autoPlay);
    setPrefs::AutoPlay();

    // 不要在这里刷新这一行：游戏刷新 = 重建节点，会导致文字叠层。
    // 行文字的更新只发生在重建整个设置界面时。
    return g_autoPlay;
}

int onShowPotentialGet(void *callable)
{
    (void)callable;
    getPrefs::ShowPotential();
    return g_showPotential ? 0 : 1;
}

int onShowPotentialTap(void *callable)
{
    (void)callable;

    g_showPotential = g_showPotential ? 0 : 1;
    setPrefs::ShowPotential();
    return g_showPotential ? 0 : 1;
}

// 两条门（实际是 4 条：两行各 2 条）的原始字：tbz w0,#0,<离线块>。
// 校验结构而不是整字比对，版本漂移时（比如换成了 tbnz/别的寄存器）能直接暴露。
static bool isAvailabilityGate(uint32_t word, uintptr_t site, uintptr_t offlineBlock)
{
    constexpr uint32_t kTbzW0Bit0 = 0x36000000;
    constexpr uint32_t kMask = 0xFF00001F; // 只关心 opcode + Rt

    if ((word & kMask) != kTbzW0Bit0)
        return false;
    if (((word >> 19) & 0x3F) != 0) // bit number == 0
        return false;

    int64_t imm = static_cast<int64_t>((word >> 5) & 0x3FFF);
    if (imm & 0x2000)
        imm -= 0x4000;

    return site + static_cast<uintptr_t>(imm * 4) == offlineBlock;
}

static void forceOnlineVariantOfOurRows()
{
    constexpr uint32_t kNop = 0xD503201F;

    struct Gate
    {
        uintptr_t offset;
        uintptr_t offlineBlock;
    };
    // 每行的两条门都跳到该行自己的离线块。
    static const Gate gates[] = {
        {OFF_SET_AUTOPLAY_TBZ_BYTE, OFF_SET_AUTOPLAY_OFFLINE},
        {OFF_SET_AUTOPLAY_TBZ_VIRT, OFF_SET_AUTOPLAY_OFFLINE},
        {OFF_SET_PTT_TBZ_BYTE, OFF_SET_PTT_OFFLINE},
        {OFF_SET_PTT_TBZ_VIRT, OFF_SET_PTT_OFFLINE},
    };

    for (const Gate &gate : gates)
    {
        const uintptr_t site = g_base + gate.offset;
        const uintptr_t block = g_base + gate.offlineBlock;
        if (!isAvailabilityGate(readWord(site), site, block))
            continue;
        patchWord(site, kNop);
    }
}

// Credits：插入 Particle Arts

// 整页由一个函数一次建完，每行调用：
//   creditsLine(ctx, &std::string text, head, x3, font, scale)
// 遇到我们的锚点行（"Scarlet Cage"，head=0、x3=0、字号 30）时先插自己的块再转发。

typedef void (*PFN_CreditLine)(void *, void *, int, int, int, float);

static Trampoline g_creditsLine;

struct CreditLine
{
    const char *text;
    int head;
    int font;
    float scale;
};

static const CreditLine kParticleArts[] = {
    {"\"Particle Arts\"", 0, 30, 0.5f},
    {"Performed by Virtual Self", 1, 24, 0.5f},
    {"Written by Porter Robinson", 1, 24, 0.5f},
    {"Courtesy of Virtual Self and mtheory", 1, 24, 0.5f},
    {"By arrangement with Hidden Track Music", 1, 24, 0.5f},
    {" ", 1, 30, 1.0f},
};

static void addCreditLine(void *ctx, const CreditLine &line)
{
    ScopedGameStr text(line.text);
    reinterpret_cast<PFN_CreditLine>(g_creditsLine.cave)(
        ctx, text.get(), line.head, 0, line.font, line.scale);
}

static void onCreditsLine(void *ctx, void *str, int head, int x3, int font, float scale)
{

    const bool isAnchor = str &&
                          strcmp(gameStrText(static_cast<const GameStr *>(str)),
                                 "\"Scarlet Cage\"") == 0;
    if (isAnchor)
        for (const CreditLine &line : kParticleArts)
            addCreditLine(ctx, line);

    reinterpret_cast<PFN_CreditLine>(g_creditsLine.cave)(ctx, str, head, x3, font, scale);
}

static void installCreditsHook()
{
    g_creditsLine = installTrampoline(g_base + OFF_CREDITS_LINE, (void *)&onCreditsLine);
}

// 登录页移除 登录 / 注册按钮

// 建页函数接收 CSB 路径并返回根节点。路径里含 "LoggedOutNetwork" 时按名字取子节点并隐藏，
// 全部走节点自身 vtable：+528 = getChildByName，+336 = setVisible，+1368 = setEnabled。
// 只 setEnabled(false) 会变成"禁用但还可见"（贴图变缺失贴图），所以必须 setVisible。
//
//  这些是字节偏移（游戏内即 ldr x8,[x8,#0x150]），
//   不能写成 vtable[336]（那会取到 2688 字节处的别的虚函数）

typedef void *(*PFN_CreateUiNode)(void *);
typedef void *(*PFN_GetChildByName)(void *, void *);
typedef void (*PFN_SetBool)(void *, int);

static constexpr size_t kVtGetChildByName = 528; // 0x210
static constexpr size_t kVtSetVisible = 336;     // 0x150
static constexpr size_t kVtSetEnabled = 1368;    // 0x558

static Trampoline g_createUiNode;

static const char *const kLoginHide[] = {
    "login_button",
    "register_button",
};

static void hideLoginParts(void *root)
{
    auto getChildByName =
        reinterpret_cast<PFN_GetChildByName>(vtableSlot(root, kVtGetChildByName));
    if (!getChildByName)
        return;

    for (const char *name : kLoginHide)
    {
        ScopedGameStr gameName(name);
        void *child = getChildByName(root, gameName.get());
        if (!child)
            continue;

        auto setEnabled = reinterpret_cast<PFN_SetBool>(vtableSlot(child, kVtSetEnabled));
        if (setEnabled)
            setEnabled(child, 0);

        auto setVisible = reinterpret_cast<PFN_SetBool>(vtableSlot(child, kVtSetVisible));
        if (setVisible)
            setVisible(child, 0);
    }
}

static void *onCreateUiNode(void *path)
{
    void *node = reinterpret_cast<PFN_CreateUiNode>(g_createUiNode.cave)(path);

    if (node && path && strstr(gameStrText(static_cast<const GameStr *>(path)), "LoggedOutNetwork"))
        hideLoginParts(node);

    return node;
}

static void installLoginHook()
{
    g_createUiNode = installTrampoline(g_base + OFF_CREATE_UI_NODE, (void *)&onCreateUiNode);
}

void installSettingsHook()
{
    forceOnlineVariantOfOurRows();

    // 取值 / 点击回调整段替换（序言含 adrp，不能蹦床）。
    (void)replaceFunction(g_base + OFF_SET_AUTOPLAY_GET, (void *)&onAutoPlayGet);
    (void)replaceFunction(g_base + OFF_SET_AUTOPLAY_TAP, (void *)&onAutoPlayTap);
    (void)replaceFunction(g_base + OFF_SET_PTT_GET, (void *)&onShowPotentialGet);
    (void)replaceFunction(g_base + OFF_SET_PTT_TAP, (void *)&onShowPotentialTap);

    installCreditsHook();
    installLoginHook();
}
