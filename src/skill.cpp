/*
 * 不要轻易重构 skill.cpp！可能会出现难以调试的错误
 */

//
// 7.3.0c 相对上一个版本 7.1.0c，角色卡 UI 对象内部节点指针整体 +0x20，角色对象没有变化
//
// ---- 角色对象 obj ---- *(int64_t *)(a1 + 0x310)
//   +0x00C  int    char_id
//   +0x0A8  int    类型 v5：0 = 普通，1 = 觉醒显示组，2 = 觉醒判定组?（maya 71 = 2）
//   +0x0C0  int    等级（20 / 30）
//   +0x0C9  u8     当前显示形态（0 = 未觉醒 / 1 = 觉醒）
//   +0x0CA  u8     !toggle，觉醒图标状态（1 = awaken-icon-closed）
//   +0x0D0  ptr    CharacterAbility（技能对象，觉醒切换后要重建）
//   +0x118  u8     觉醒判定标志（AwakenCheck 0xBB1138 = ldr x8,[x0,#0x118] / cset ne）
//                  注意游戏按 64 位读，我们只写最低字节（与旧版一致，实测可用）
//   +0x120  u8     setupSkill 内部用的另一个标志位，与本模块无关，勿动
//   +0x2D0  ptr    SpecialScene（Finale / Testify 异象，null 会崩渲染链）
//
// ---- 角色卡 UI 对象 a1 ----
//                        7.1.0c   7.3.0c  含义
//   +0x310                0x310    0x310   角色对象指针
//   +0x3E0                0x3E0    0x3E0   glow 底图（glow-ez.png / glow-normal.png）
//   lock-icon(sprite)     0x4D8 →  0x4F8   lock-icon
//   锁定按钮附属          0x4E0 →  0x500   setVisible
//   锁定按钮本体          0x4E8 →  0x508   setVisible + setEnabled
//   awaken-icon(sprite) 0x4F0 →  0x510   awaken-icon-closed / awaken-icon
//   觉醒状态文本          0x4F8 →  0x518   "PRE-AWAKENING" / "AWAKENED"
//   觉醒按钮本体          0x500 →  0x520   setVisible + setEnabled（点击入口）
//   技能描述 Label       0x508 →  0x528   SetLabelText 的目标
//   等级突破「唤醒」按钮   0x538 →  0x558   勿强制显示，会在 UI 外显示并拦截触摸
//
//

#include <dlfcn.h>
#include <cstdint>

#include "game.hpp"
#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

typedef void (*PFN_FileNameCtor)(void *obj, int charId);
typedef void (*PFN_RefAdd)(void *obj);

typedef void *(*PFN_OnlineSkill)(const void *gameStr);

typedef int64_t (*PFN_RefreshCharState)(int64_t a1, char a2);
typedef int64_t (*PFN_RenderCharUI)(int64_t a1);
typedef int (*PFN_AwakenCheck)(int64_t a1);
typedef int64_t (*PFN_ModifyCharState)(int64_t a1, int64_t a2, char a3, char a4, char a5);
typedef void (*PFN_SetCharBorder)(int64_t a1, char a2);
typedef void (*PFN_SetBarFill)(int64_t bar_node, float fill);

static PFN_FileNameCtor g_file_name_ctor = nullptr;
static PFN_RefAdd g_ref_add = nullptr;
static PFN_OnlineSkill g_online_skill = nullptr;
static PFN_RenderCharUI g_RenderCharUI = nullptr;

static Trampoline g_refresh;
static Trampoline g_render;
static Trampoline g_awaken;
static Trampoline g_modify;
static Trampoline g_pborder;
static Trampoline g_barfill;
static Trampoline g_userslot1;

static const CharInfo *findChar(int id)
{
    for (int i = 0; i < g_char_num; i++)
        if (g_char_info[i].id == id)
            return &g_char_info[i];
    return nullptr;
}

static bool hasUncap(int id)
{
    const CharInfo *ci = findChar(id);
    return ci && ci->has_uncap;
}

static const char *skillIdForChar(int cid, bool uncap)
{
    const CharInfo *ci = findChar(cid);
    if (!ci)
        return nullptr;
    if (uncap && ci->skill_id_uncap[0])
        return ci->skill_id_uncap;
    return ci->skill_id[0] ? ci->skill_id : nullptr;
}

static bool buildOnlineSkill(int64_t obj, const char *skillId)
{
    if (!obj || !g_online_skill || !skillId || !*skillId)
        return false;
    GameStr name = {};
    gameStrInit(&name, skillId);
    void *ab = g_online_skill(&name);
    gameStrFree(&name);

    if (!ab)
        return false;

    g_ref_add(ab);
    *reinterpret_cast<void **>(obj + 208) = ab;
    return true;
}

static void rebuildSkill(int64_t obj, int cid)
{
    const CharInfo *ci = findChar(cid);
    if (!obj || !ci)
        return;
    if (!ci->skill_id_uncap[0])
        return;
    const char *sid = settingsIsAwakened(cid) ? ci->skill_id_uncap : ci->skill_id;

    if (!sid[0] || !buildOnlineSkill(obj, sid))
        *reinterpret_cast<void **>(obj + 208) = nullptr;
}

static void setNodeVisible(int64_t node_addr_field, bool vis)
{
    void *node = *(void **)(node_addr_field);
    if (!node || !*(void **)node)
        return;
    typedef void (*SV)(void *, bool);
    SV sv = *(SV *)((char *)*(void **)node + 336);
    if (sv)
        sv(node, vis);
}

static void setWidgetEnabled(int64_t node_addr_field, bool en)
{
    void *node = *(void **)(node_addr_field);
    if (!node || !*(void **)node)
        return;
    typedef void (*SE)(void *, bool);
    SE se = *(SE *)((char *)*(void **)node + 1368);
    if (se)
        se(node, en);
}

static bool isCharSealed(uintptr_t base)
{
    uint64_t ctx = *(uint64_t *)(g_base + OFF_SealCtx);
    if (!ctx)
        return false;
    uint64_t slot = *(uint64_t *)(ctx + 120);
    if (!slot)
        return false;
    return *(uint8_t *)(slot + 12) != 0;
}

static void forceAwakenUi(int64_t a1, int64_t obj, int cid)
{
    if (!obj || cid < 0 || cid >= g_char_num || !hasUncap(cid))
        return;
    setNodeVisible(a1 + 1296, true);
    setNodeVisible(a1 + 1304, true);
    setNodeVisible(a1 + 1312, true);
    setWidgetEnabled(a1 + 1312, true);
}

static void force72SealButton(int64_t a1, int64_t obj, int cid)
{
    if (!obj || cid != 72)
        return;
    setNodeVisible(a1 + 1288, true);
    setWidgetEnabled(a1 + 1288, true);
    setNodeVisible(a1 + 1272, true);
    setNodeVisible(a1 + 1280, true);
}

static void force71SkillMayaDesc(int64_t a1, int64_t obj, int cid)
{
    if (!(obj && cid == 71 && !settingsIsAwakened(71) && !isCharSealed(g_base)))
        return;
    int64_t lbl = a1 ? *(int64_t *)(a1 + 1320) : 0;
    if (!lbl)
        return;
    typedef GameStr (*PFN_Localize)(const char *);
    GameStr s = ((PFN_Localize)(g_base + OFF_Localize))(
        (const char *)(g_base + OFF_SkillMayaText));
    ((void (*)(int64_t, void *))(g_base + OFF_SetLabelText))(lbl, &s);
    gameStrFree(&s);
}

int64_t onRenderCharUI(int64_t a1)
{
    getPrefs::Awakened();

    int64_t ret = ((PFN_RenderCharUI)g_render.cave)(a1);

    int64_t obj = a1 ? *(int64_t *)(a1 + 784) : 0;
    int cid = obj ? *(int *)(obj + 12) : -1;
    forceAwakenUi(a1, obj, cid);
    force72SealButton(a1, obj, cid);
    force71SkillMayaDesc(a1, obj, cid);
    return ret;
}

int onAwakenCheck(int64_t a1)
{
    int ret = ((PFN_AwakenCheck)g_awaken.cave)(a1);

    int cid = a1 ? *(int *)(a1 + 12) : -1;
    if (cid >= 0 && cid < g_char_num && hasUncap(cid))
        return ret | 1;
    return ret;
}

int64_t onModifyCharState(int64_t a1, int64_t a2, char a3, char a4, char a5)
{

    uintptr_t lr;
    asm volatile("mov %0, x30" : "=r"(lr));

    getPrefs::Awakened();

    uintptr_t caller = lr - g_base;
    int cid = a2 ? *(int *)(a2 + 12) : -1;
    bool isAwaken = (caller == OFF_AWAKEN_CALLER);
    if (isAwaken && cid >= 0 && cid < g_char_num && hasUncap(cid))
    {
        settingsSetAwaken(cid, !settingsIsAwakened(cid));
        const int on = settingsIsAwakened(cid);
        *(uint8_t *)(a2 + 201) = (uint8_t)on;
        *(uint8_t *)(a2 + 280) = (uint8_t)on;
        *(int *)(a2 + 168) = on;
        *(uint8_t *)(a2 + 202) = (uint8_t)(on ? 0 : 1);
        *(int *)(a2 + 192) = 30;
        rebuildSkill(a2, cid);
    }

    return ((PFN_ModifyCharState)g_modify.cave)(a1, a2, a3, a4, a5);
}

void onSetCharBorder(int64_t a1, char a2)
{
    int64_t obj = a1 ? *(int64_t *)(a1 + 1216) : 0;
    bool has_obj = (obj != 0);
    int saved_lv = 0;
    if (has_obj)
    {
        saved_lv = *(int *)(obj + 192);
        *(int *)(obj + 192) = 0;
    }

    ((PFN_SetCharBorder)g_pborder.cave)(a1, a2);

    if (has_obj)
        *(int *)(obj + 192) = saved_lv;
}

void onSetBarFill(int64_t bar_node, float fill)
{
    (void)fill;
    ((PFN_SetBarFill)g_barfill.cave)(bar_node, 1.0f);
}

int64_t onRefreshCharState(int64_t a1, char a2)
{
    uintptr_t lr;
    asm volatile("mov %0, x30" : "=r"(lr));

    getPrefs::Awakened();

    uintptr_t off = lr - g_base;

    int64_t obj = a1 ? *(int64_t *)(a1 + 784) : 0;
    int cid = obj ? *(int *)(obj + 12) : -1;
    bool isAwakenBtn = (off == OFF_AWAKEN_CALL);

    int64_t ret = ((PFN_RefreshCharState)g_refresh.cave)(a1, a2);

    if (obj && cid >= 0 && cid < g_char_num && hasUncap(cid))
    {
        const int on = settingsIsAwakened(cid);
        if (isAwakenBtn)
            *(uint8_t *)(obj + 201) = (uint8_t)on;
        *(uint8_t *)(obj + 202) = (uint8_t)(on ? 0 : 1);
        *(int *)(obj + 192) = 30;
    }

    if (isAwakenBtn)
        g_RenderCharUI(a1);

    return ret;
}

int64_t setupSkill(int64_t obj, int char_id)
{

    // 此处在游戏启动时读取 autoPlay
    // 玩家未打开设置界面点选，也能加载 auto 开关状态
    getPrefs::Awakened();
    getPrefs::AutoPlay();

    *reinterpret_cast<int *>(obj + 12) = char_id;
    *reinterpret_cast<uint8_t *>(obj + 201) = 0;
    if (hasUncap(char_id))
        *reinterpret_cast<uint8_t *>(obj + 201) = (uint8_t)settingsIsAwakened(char_id);

    g_file_name_ctor(reinterpret_cast<void *>(obj), char_id);

    const CharInfo *ci = findChar(char_id);

    if (!ci && char_id != -1)
        return 0;

    int v5 = 0;
    switch (char_id)
    {
    case 3:
    case 6:
        v5 = 2;
        break;
    case 31:
    case 32:
    case 38:
    case 51:
    case 52:
    case 60:
    case 74:
        v5 = 1;
        break;
    default:
        break;
    }

    if (ci)
    {
        const bool uncap = settingsIsAwakened(char_id) != 0;
        const char *sid = skillIdForChar(char_id, uncap);
        if (sid)
            buildOnlineSkill(obj, sid);
    }

    *reinterpret_cast<int *>(obj + 168) = v5;
    *reinterpret_cast<int *>(obj + 192) = hasUncap(char_id) ? 30 : 20;

    if (char_id >= 0 && char_id < g_char_num && hasUncap(char_id))
    {
        const int on = settingsIsAwakened(char_id);
        *reinterpret_cast<int *>(obj + 168) = on;
        *reinterpret_cast<uint8_t *>(obj + 202) = (uint8_t)(on ? 0 : 1);
    }
    return 1;
}

struct FakeUserSlot
{
    uint64_t pad[5];
    uint8_t name[24];
    uint64_t tail[5];
};

__attribute__((used)) static FakeUserSlot g_fake_user __asm__("g_fake_user");

static void initFakeUserSlot()
{
    const size_t n = sizeof(CHAR101USERNAME) - 1;
    g_fake_user.name[0] = (uint8_t)(n << 1);
    for (size_t i = 0; i < n; i++)
        g_fake_user.name[1 + i] = (uint8_t)CHAR101USERNAME[i];
}

__attribute__((used)) static uintptr_t g_userslot1_add __asm__("g_userslot1_add") = 0;

__attribute__((naked)) static void onUserSlot1(void)
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
        "adrp x16, g_userslot1_add\n"
        "ldr  x16, [x16, #:lo12:g_userslot1_add]\n"
        "br   x16");
}

void installSkillHook()
{
    (void)dlopen(TARGET, RTLD_NOW | RTLD_GLOBAL);

    g_file_name_ctor = (PFN_FileNameCtor)(g_base + OFF_FileNameCtor);
    g_ref_add = (PFN_RefAdd)(g_base + OFF_RefAdd);
    g_online_skill = (PFN_OnlineSkill)(g_base + OFF_OnlineSkill);
    g_RenderCharUI = (PFN_RenderCharUI)(g_base + OFF_RenderCharUI);

    initFakeUserSlot();

    (void)replaceFunction(g_base + OFF_Init, (void *)&setupSkill);

    g_refresh = installTrampoline(g_base + OFF_RefreshCharState, (void *)&onRefreshCharState);
    g_render = installTrampoline(g_base + OFF_RenderCharUI, (void *)&onRenderCharUI);
    g_awaken = installTrampoline(g_base + OFF_AwakenCheck, (void *)&onAwakenCheck);
    g_modify = installTrampoline(g_base + OFF_ModifyCharState, (void *)&onModifyCharState);
    g_pborder = installTrampoline(g_base + OFF_SetCharBorder, (void *)&onSetCharBorder);
    g_barfill = installTrampoline(g_base + OFF_SetBarFill, (void *)&onSetBarFill);
    g_userslot1_add = g_base + OFF_CHAR101USERNAME_ADD;
    g_userslot1 = installTrampoline(g_base + (OFF_CHAR101USERNAME_ADD - 0x10), (void *)&onUserSlot1);
}
