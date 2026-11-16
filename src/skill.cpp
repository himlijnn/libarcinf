// 角色 / 技能 / 觉醒 UI 改写
//
// 三块内容：
//   1. setupSkill（替换 OFF_Init）：按角色表装配 CharacterAbility 与觉醒显示字段；
//   2. 角色卡 UI：渲染后强制显示觉醒入口、补齐 maya 未觉醒描述、强制进度条满格；
//   3. 觉醒切换：在 ModifyCharState 层按调用者识别"觉醒按钮"，翻转存档值并重建技能。
//
//   * onModifyCharState / onRefreshCharState 里 `mov x30` 必须是函数体第一条语句
//     之后任何一次 bl 都会把 x30 改成本函数内的返回地址，靠它识别调用者的逻辑会静默失效。
//   * setupSkill 会被游戏用一个共享占位对象反复调用（cid 100..255 / -1）：无效 cid 必须
//     在写对象之前 return，否则会污染共享对象、后续渲染空指针崩溃。

#include <cstdint>

#include "game.hpp"
#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

typedef void (*PFN_FileNameCtor)(void *obj, int charId);
typedef void (*PFN_RefAdd)(void *obj);
typedef void *(*PFN_OnlineSkill)(const GameStr *skillId);

typedef int64_t (*PFN_RefreshCharState)(int64_t ui, char flag);
typedef int64_t (*PFN_RenderCharUI)(int64_t ui);
typedef int (*PFN_AwakenCheck)(int64_t ability);
typedef int64_t (*PFN_ModifyCharState)(int64_t self, int64_t obj, char b3, char b4, char b5);
typedef void (*PFN_SetCharBorder)(int64_t ui, char on);
typedef void (*PFN_SetBarFill)(int64_t barNode, float fill);
typedef GameStr (*PFN_Localize)(const char *text);
typedef void (*PFN_SetLabelText)(int64_t label, GameStr *text);

// 角色对象（CharacterAbility）字段偏移

static constexpr int kObjCharId = 12;
static constexpr int kObjStyle = 168;      // 展示分组（0 普通 / 1 / 2）
static constexpr int kObjLevel = 192;      // 等级，觉醒角色判 30
static constexpr int kObjFormShown = 201;  // 仅显示用：当前形态
static constexpr int kObjFormClosed = 202; // !觉醒 -> icon 显示未觉醒
static constexpr int kObjAbility = 208;    // CharacterAbility*（技能对象）
static constexpr int kObjAwakened = 280;   // 觉醒判定（异象等逻辑读它）

// 角色卡 UI 字段偏移

static constexpr int kUiAbility = 784;
static constexpr int kUiSealExtra0 = 1240;
static constexpr int kUiSealExtra1 = 1248;
static constexpr int kUiSealButton = 1256;
static constexpr int kUiAwakenIcon = 1264;
static constexpr int kUiAwakenText = 1272;
static constexpr int kUiAwakenButton = 1280;
static constexpr int kUiSkillDesc = 1288;
static constexpr int kCardBorderAbility = 1216;

// 节点 vtable 槽（cocos2d-x Node / Widget）

static constexpr size_t kVtSetVisible = 336;
static constexpr size_t kVtSetEnabled = 1368;

// 角色等级

static constexpr int kAwakenLevel = 30;
static constexpr int kNormalLevel = 20;

// 游戏函数指针（install 之后恒非空）

static PFN_FileNameCtor g_fileNameCtor = nullptr;
static PFN_RefAdd g_refAdd = nullptr;
static PFN_OnlineSkill g_onlineSkill = nullptr;

static Trampoline g_refreshTramp;
static Trampoline g_renderTramp;
static Trampoline g_awakenTramp;
static Trampoline g_modifyTramp;
static Trampoline g_borderTramp;
static Trampoline g_barFillTramp;
static Trampoline g_userSlotTramp;

// 角色表查询

static const CharInfo *findChar(int id)
{
    for (int i = 0; i < g_char_num; i++)
        if (g_char_info[i].id == id)
            return &g_char_info[i];
    return nullptr;
}

static bool hasUncap(int id)
{
    const CharInfo *info = findChar(id);
    return info && info->has_uncap;
}

static bool isValidCharId(int id)
{
    return id >= 0 && id < g_char_num;
}

// 觉醒态优先用 uncap 技能；没有就退回普通技能；都没有返回 nullptr。
static const char *skillIdForChar(int charId, bool awakened)
{
    const CharInfo *info = findChar(charId);
    if (!info)
        return nullptr;
    if (awakened && info->skill_id_uncap[0])
        return info->skill_id_uncap;
    return info->skill_id[0] ? info->skill_id : nullptr;
}

static int charIdOfAbility(int64_t ability)
{
    return ability ? *reinterpret_cast<int *>(ability + kObjCharId) : -1;
}

// 让游戏自己的在线技能工厂造一个 CharacterAbility 并挂到对象上。
static bool buildOnlineSkill(int64_t obj, const char *skillId)
{
    if (!obj || !skillId || !*skillId)
        return false;

    ScopedGameStr name(skillId);
    void *ability = g_onlineSkill(name.get());
    if (!ability)
        return false;

    g_refAdd(ability);
    *reinterpret_cast<void **>(obj + kObjAbility) = ability;
    return true;
}

// 觉醒切换后重建技能对象：只有带 uncap 技能的角色才换。
static void rebuildSkill(int64_t obj, int charId)
{
    const CharInfo *info = findChar(charId);
    if (!obj || !info || !info->skill_id_uncap[0])
        return;

    const char *skillId = settingsIsAwakened(charId) ? info->skill_id_uncap : info->skill_id;
    if (!skillId[0] || !buildOnlineSkill(obj, skillId))
        *reinterpret_cast<void **>(obj + kObjAbility) = nullptr;
}

// 节点可见性

// 参数是"UI 对象里存节点指针的那个槽的地址"，不是节点本身。

static void setNodeVisible(int64_t nodeSlot, bool visible)
{
    void *node = *reinterpret_cast<void **>(nodeSlot);
    if (!node)
        return;

    void **vtable = *reinterpret_cast<void ***>(node);
    if (!vtable)
        return;

    auto setVisible = reinterpret_cast<void (*)(void *, bool)>(vtable[kVtSetVisible]);
    if (setVisible)
        setVisible(node, visible);
}

static void setWidgetEnabled(int64_t nodeSlot, bool enabled)
{
    void *node = *reinterpret_cast<void **>(nodeSlot);
    if (!node)
        return;

    void **vtable = *reinterpret_cast<void ***>(node);
    if (!vtable)
        return;

    auto setEnabled = reinterpret_cast<void (*)(void *, bool)>(vtable[kVtSetEnabled]);
    if (setEnabled)
        setEnabled(node, enabled);
}

// 角色是否被"封印"：SealCtx -> +120 -> +12 字节非 0。
static bool isCharSealed()
{
    const uint64_t context = *reinterpret_cast<uint64_t *>(g_base + OFF_SealCtx);
    if (!context)
        return false;

    const uint64_t slot = *reinterpret_cast<uint64_t *>(context + 120);
    if (!slot)
        return false;

    return *reinterpret_cast<uint8_t *>(slot + 12) != 0;
}

// 角色卡 UI 改写

// 觉醒按钮的可见性由 obj+201 决定；未觉醒时为 0 → 游戏把入口藏起来并禁用。
static void forceAwakenUi(int64_t ui, int64_t obj, int charId)
{
    if (!obj || !isValidCharId(charId) || !hasUncap(charId))
        return;

    setNodeVisible(ui + kUiAwakenIcon, true);
    setNodeVisible(ui + kUiAwakenText, true);
    setNodeVisible(ui + kUiAwakenButton, true);
    setWidgetEnabled(ui + kUiAwakenButton, true);
}

// 72 号角色的封印按钮同理，只在它身上显示。
static void forceSealButtonForIntruder(int64_t ui, int64_t obj, int charId)
{
    if (!obj || charId != 72)
        return;

    setNodeVisible(ui + kUiSealButton, true);
    setWidgetEnabled(ui + kUiSealButton, true);
    setNodeVisible(ui + kUiSealExtra0, true);
    setNodeVisible(ui + kUiSealExtra1, true);
}

// 71 号（maya）未觉醒且未被封印时，用游戏本地化文本覆盖技能描述。
static void forceMayaSkillDesc(int64_t ui, int64_t obj, int charId)
{
    if (!obj || charId != 71 || settingsIsAwakened(71) || isCharSealed())
        return;

    const int64_t label = ui ? *reinterpret_cast<int64_t *>(ui + kUiSkillDesc) : 0;
    if (!label)
        return;

    GameStr text = reinterpret_cast<PFN_Localize>(g_base + OFF_Localize)(
        reinterpret_cast<const char *>(g_base + OFF_SkillMayaText));
    reinterpret_cast<PFN_SetLabelText>(g_base + OFF_SetLabelText)(label, &text);
    gameStrFree(&text);
}

int64_t onRenderCharUI(int64_t ui)
{
    getPrefs::Awakened();
    getPrefs::AutoPlay();

    // 必须走洞穴：g_renderCharUI 是已被 patch 的入口，调它会再次进入本函数 → 无限递归。
    const int64_t result = reinterpret_cast<PFN_RenderCharUI>(g_renderTramp.cave)(ui);

    const int64_t obj = ui ? *reinterpret_cast<int64_t *>(ui + kUiAbility) : 0;
    const int charId = charIdOfAbility(obj);

    forceAwakenUi(ui, obj, charId);
    forceSealButtonForIntruder(ui, obj, charId);
    forceMayaSkillDesc(ui, obj, charId);
    return result;
}

int onAwakenCheck(int64_t ability)
{
    const int result = reinterpret_cast<PFN_AwakenCheck>(g_awakenTramp.cave)(ability);

    const int charId = charIdOfAbility(ability);
    if (isValidCharId(charId) && hasUncap(charId))
        return result | 1;
    return result;
}

// 觉醒切换后统一写这几个字段：+168 决定 icon / 大图，+280 是判定，+202 是"未觉醒"标记。
static void applyAwakenState(int64_t obj, bool awakened)
{
    const uint8_t on = awakened ? 1 : 0;
    *reinterpret_cast<uint8_t *>(obj + kObjFormShown) = on;
    *reinterpret_cast<uint8_t *>(obj + kObjAwakened) = on;
    *reinterpret_cast<int *>(obj + kObjStyle) = on;
    *reinterpret_cast<uint8_t *>(obj + kObjFormClosed) = awakened ? 0 : 1;
    *reinterpret_cast<int *>(obj + kObjLevel) = kAwakenLevel;
}

int64_t onModifyCharState(int64_t self, int64_t obj, char b3, char b4, char b5)
{
    // 必须第一句：之后任何 bl 都会覆盖 x30。
    uintptr_t lr;
    asm volatile("mov %0, x30" : "=r"(lr));

    getPrefs::Awakened();
    getPrefs::AutoPlay();

    const uintptr_t caller = lr - g_base;
    const int charId = charIdOfAbility(obj);

    if (caller == OFF_AWAKEN_CALLER && isValidCharId(charId) && hasUncap(charId))
    {
        settingsSetAwaken(charId, !settingsIsAwakened(charId));
        applyAwakenState(obj, settingsIsAwakened(charId) != 0);
        rebuildSkill(obj, charId);
    }

    return reinterpret_cast<PFN_ModifyCharState>(g_modifyTramp.cave)(
        self, obj, b3, b4, b5);
}

// 卡框绘制会读 obj+192 当等级；临时压成 0，避免把未满级角色画成满级边框。
void onSetCharBorder(int64_t ui, char on)
{
    const int64_t obj = ui ? *reinterpret_cast<int64_t *>(ui + kCardBorderAbility) : 0;
    if (!obj)
    {
        reinterpret_cast<PFN_SetCharBorder>(g_borderTramp.cave)(ui, on);
        return;
    }

    const int savedLevel = *reinterpret_cast<int *>(obj + kObjLevel);
    *reinterpret_cast<int *>(obj + kObjLevel) = 0;

    reinterpret_cast<PFN_SetCharBorder>(g_borderTramp.cave)(ui, on);

    *reinterpret_cast<int *>(obj + kObjLevel) = savedLevel;
}

// 等级条画满
void onSetBarFill(int64_t barNode, float fill)
{
    (void)fill;
    reinterpret_cast<PFN_SetBarFill>(g_barFillTramp.cave)(barNode, 1.0f);
}

int64_t onRefreshCharState(int64_t ui, char flag)
{
    // 与 onModifyCharState 一样，x30 必须是第一条语句。
    uintptr_t lr;
    asm volatile("mov %0, x30" : "=r"(lr));

    getPrefs::Awakened();
    getPrefs::AutoPlay();

    const uintptr_t caller = lr - g_base;
    const int64_t obj = ui ? *reinterpret_cast<int64_t *>(ui + kUiAbility) : 0;
    const int charId = charIdOfAbility(obj);
    const bool fromAwakenButton = (caller == OFF_AWAKEN_CALL);

    const int64_t result = reinterpret_cast<PFN_RefreshCharState>(g_refreshTramp.cave)(ui, flag);

    if (obj && isValidCharId(charId) && hasUncap(charId))
    {
        const bool awakened = settingsIsAwakened(charId) != 0;
        if (fromAwakenButton)
            *reinterpret_cast<uint8_t *>(obj + kObjFormShown) = awakened ? 1 : 0;
        *reinterpret_cast<uint8_t *>(obj + kObjFormClosed) = awakened ? 0 : 1;
        *reinterpret_cast<int *>(obj + kObjLevel) = kAwakenLevel;
    }

    // 只有觉醒按钮点击后重绘整卡，其它入口（页面加载 / 锁定）不重绘。
    if (fromAwakenButton)
        reinterpret_cast<PFN_RenderCharUI>(g_renderTramp.cave)(ui);

    return result;
}

// setupSkill：替换 OFF_Init

int64_t setupSkill(int64_t obj, int charId)
{
    getPrefs::Awakened();
    getPrefs::AutoPlay();

    *reinterpret_cast<int *>(obj + kObjCharId) = charId;

    *reinterpret_cast<uint8_t *>(obj + kObjFormShown) = 0;
    if (hasUncap(charId))
        *reinterpret_cast<uint8_t *>(obj + kObjFormShown) = settingsIsAwakened(charId) ? 1 : 0;

    g_fileNameCtor(reinterpret_cast<void *>(obj), charId);

    const CharInfo *info = findChar(charId);

    //   -1 是"默认角色"的有效调用，必须继续往下写字段并返回 1；
    //   拦掉它会留下未初始化的能力对象，游戏随后对 null 做 retain 直接闪退

    if (!info && charId != -1)
        return 0;

    int styleGroup = 0;
    switch (charId)
    {
    case 3:
    case 6:
        styleGroup = 2;
        break;
    case 31:
    case 32:
    case 38:
    case 51:
    case 52:
    case 60:
    case 74:
        styleGroup = 1;
        break;
    default:
        break;
    }

    if (info)
    {
        const bool awakened = settingsIsAwakened(charId) != 0;
        if (const char *skillId = skillIdForChar(charId, awakened))
            buildOnlineSkill(obj, skillId);
    }

    *reinterpret_cast<int *>(obj + kObjStyle) = styleGroup;
    *reinterpret_cast<int *>(obj + kObjLevel) = hasUncap(charId) ? kAwakenLevel : kNormalLevel;

    if (isValidCharId(charId) && hasUncap(charId))
    {
        const int on = settingsIsAwakened(charId);
        *reinterpret_cast<int *>(obj + kObjStyle) = on;
        *reinterpret_cast<uint8_t *>(obj + kObjFormClosed) = on ? 0 : 1;
    }

    return 1;
}

// 101 号角色，离线时指定用户名

// 结构：5 个未用 qword + 24 字节 std::string（昵称）+ 5 个未用 qword。
// 名字固定，因为下面的 naked asm 用 adrp 直接引用。
struct FakeUserSlot
{
    uint64_t head[5];
    uint8_t name[24];
    uint64_t tail[5];
};

__attribute__((used)) static FakeUserSlot g_fake_user __asm__("g_fake_user");

// 原函数续跑地址，同样被 asm 引用。
__attribute__((used)) static uintptr_t g_userslot1_cont __asm__("g_userslot1_cont") = 0;

static void initFakeUserSlot()
{
    constexpr size_t kNameLen = sizeof(CHAR101USERNAME) - 1;

    // 手写 libc++ 短串布局：byte0 = size << 1，数据紧随其后。
    g_fake_user.name[0] = static_cast<uint8_t>(kNameLen << 1);
    for (size_t i = 0; i < kNameLen; i++)
        g_fake_user.name[1 + i] = static_cast<uint8_t>(CHAR101USERNAME[i]);
}

// 游戏读昵称的入口：名字长度合法就放行原逻辑（8f），否则换成本地占位串（9f）。
__attribute__((naked)) static void onUserSlot1()
{
    asm volatile(
        "cbz  x9, 9f\n"
        "ldr  x9, [x9, #0xd0]\n"
        "cbz  x9, 9f\n"
        "ldrb w17, [x9, #0x28]\n"
        "tbz  w17, #0, 1f\n"
        "ldr  x16, [x9, #0x38]\n"
        "cbz  x16, 9f\n"
        "ldr  x16, [x9, #0x30]\n"
        "cmp  x16, #256\n"
        "b.lo 8f\n"
        "b    9f\n"
        "1:\n"
        "cmp  w17, #44\n"
        "b.ls 8f\n"
        "9:\n"
        "adrp x9, g_fake_user\n"
        "add  x9, x9, :lo12:g_fake_user\n"
        "8:\n"
        "sturb w8, [x29, #-0x1c]\n"
        "ldr  w8, [x0, #0xac]\n"
        "stur w10, [x29, #-0x18]\n"
        "adrp x16, g_userslot1_cont\n"
        "ldr  x16, [x16, #:lo12:g_userslot1_cont]\n"
        "br   x16");
}

void installSkillHook()
{
    g_fileNameCtor = reinterpret_cast<PFN_FileNameCtor>(g_base + OFF_FileNameCtor);
    g_refAdd = reinterpret_cast<PFN_RefAdd>(g_base + OFF_RefAdd);
    g_onlineSkill = reinterpret_cast<PFN_OnlineSkill>(g_base + OFF_OnlineSkill);

    initFakeUserSlot();

    (void)replaceFunction(g_base + OFF_Init, (void *)&setupSkill);

    g_refreshTramp = installTrampoline(g_base + OFF_RefreshCharState, (void *)&onRefreshCharState);
    g_renderTramp = installTrampoline(g_base + OFF_RenderCharUI, (void *)&onRenderCharUI);
    g_awakenTramp = installTrampoline(g_base + OFF_AwakenCheck, (void *)&onAwakenCheck);
    g_modifyTramp = installTrampoline(g_base + OFF_ModifyCharState, (void *)&onModifyCharState);
    g_borderTramp = installTrampoline(g_base + OFF_SetCharBorder, (void *)&onSetCharBorder);
    g_barFillTramp = installTrampoline(g_base + OFF_SetBarFill, (void *)&onSetBarFill);

    g_userslot1_cont = g_base + OFF_CHAR101USERNAME_CONT;
    g_userSlotTramp = installTrampoline(g_base + OFF_CHAR101USERNAME_READ, (void *)&onUserSlot1);
}
