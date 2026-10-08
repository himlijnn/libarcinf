
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

static uintptr_t g_base = 0;

static Trampoline g_refresh;
static Trampoline g_render;
static Trampoline g_awaken;
static Trampoline g_modify;
static Trampoline g_pborder;
static Trampoline g_barfill;

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
    gameStrInit(g_base, &name, skillId);
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
    uint64_t ctx = *(uint64_t *)(base + OFF_SealCtx);
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
    setNodeVisible(a1 + 1264, true);
    setNodeVisible(a1 + 1272, true);
    setNodeVisible(a1 + 1280, true);
    setWidgetEnabled(a1 + 1280, true);
}

static void force72SealButton(int64_t a1, int64_t obj, int cid)
{
    if (!obj || cid != 72)
        return;
    setNodeVisible(a1 + 1256, true);
    setWidgetEnabled(a1 + 1256, true);
    setNodeVisible(a1 + 1240, true);
    setNodeVisible(a1 + 1248, true);
}

static void force71SkillMayaDesc(int64_t a1, int64_t obj, int cid)
{
    if (!(obj && cid == 71 && !settingsIsAwakened(71) && !isCharSealed(g_base)))
        return;
    int64_t lbl = a1 ? *(int64_t *)(a1 + 1288) : 0;
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
    getPrefs::AutoPlay();

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
    getPrefs::AutoPlay();

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
    getPrefs::AutoPlay();

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
    getPrefs::Awakened();
    getPrefs::AutoPlay();
    *reinterpret_cast<int *>(obj + 12) = char_id;

    *reinterpret_cast<uint8_t *>(obj + 201) = 0;
    if (hasUncap(char_id))
        *reinterpret_cast<uint8_t *>(obj + 201) = (uint8_t)settingsIsAwakened(char_id);

    g_file_name_ctor(reinterpret_cast<void *>(obj), char_id);

    uint8_t *flag280 = reinterpret_cast<uint8_t *>(obj + 280);
    *flag280 = 0;

    int v5 = 0;

    switch (char_id)
    {

    case -1:
    case 1:
        v5 = 0;
        break;

    case 3:
    case 6:
        *flag280 = 1;
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
        *flag280 = 1;
        break;

    case 91:
        v5 = 0;
        *flag280 = 0;
        break;

    default:
    {
        const CharInfo *ci = findChar(char_id);
        if (!ci)
            return 0;

        const bool uncap = settingsIsAwakened(char_id) != 0;
        const char *sid = skillIdForChar(char_id, uncap);
        if (sid)
            buildOnlineSkill(obj, sid);
        v5 = 0;
        break;
    }
    }

    *reinterpret_cast<int *>(obj + 168) = v5;
    *reinterpret_cast<int *>(obj + 192) = hasUncap(char_id) ? 30 : 20;

    if (char_id >= 0 && char_id < g_char_num && hasUncap(char_id))
    {
        const int on = settingsIsAwakened(char_id);
        *flag280 = (uint8_t)on;
        *reinterpret_cast<int *>(obj + 168) = on;
        *reinterpret_cast<uint8_t *>(obj + 202) = (uint8_t)(on ? 0 : 1);
    }
    return 1;
}

void installSkillHook()
{
    void *handle = dlopen(TARGET, RTLD_NOW | RTLD_GLOBAL);

    uintptr_t base = getModuleBase(TARGET);

    g_file_name_ctor = (PFN_FileNameCtor)(base + OFF_FileNameCtor);
    g_ref_add = (PFN_RefAdd)(base + OFF_RefAdd);
    g_RenderCharUI = (PFN_RenderCharUI)(base + OFF_RenderCharUI);

    g_online_skill = (PFN_OnlineSkill)(base + OFF_OnlineSkill);

    g_base = base;

    (void)replaceFunction(base + OFF_Init, (void *)&setupSkill);

    g_refresh = installTrampoline(base + OFF_RefreshCharState, (void *)&onRefreshCharState);
    g_render = installTrampoline(base + OFF_RenderCharUI, (void *)&onRenderCharUI);
    g_awaken = installTrampoline(base + OFF_AwakenCheck, (void *)&onAwakenCheck);
    g_modify = installTrampoline(base + OFF_ModifyCharState, (void *)&onModifyCharState);
    g_pborder = installTrampoline(base + OFF_SetCharBorder, (void *)&onSetCharBorder);
    g_barfill = installTrampoline(base + OFF_SetBarFill, (void *)&onSetBarFill);
}
