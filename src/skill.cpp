#include <dlfcn.h>
#include <cstdint>
#include <cstdio>

#include "game.hpp"
#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

struct CharInfo
{
    int id;
    bool has_uncap;
    const char *skill_id;
    const char *skill_id_uncap;
};

static const CharInfo g_char_info[] = {
    {0, true, "gauge_easy", ""},
    {1, true, "", ""},
    {2, true, "frags_kou", ""},
    {3, false, "", ""},
    {4, true, "note_mirror", "visual_ink"},
    {5, true, "skill_reunion", ""},
    {6, false, "", ""},
    {7, false, "gauge_hard", ""},
    {8, false, "frag_plus_10_pack_stellights", ""},
    {9, false, "gauge_easy|frag_plus_15_pst&prs", ""},
    {10, true, "gauge_hard|fail_frag_minus_100", "ilith_awakened_skill"},
    {11, true, "frag_plus_5_side_light", "eto_uncap"},
    {12, true, "visual_hide_hp", "luna_uncap"},
    {13, true, "frag_plus_5_side_conflict", "shirabe_entry_fee"},
    {14, false, "challenge_fullcombo_0gauge", ""},
    {15, false, "gauge_overflow", ""},
    {16, false, "gauge_easy|note_mirror", ""},
    {17, false, "note_mirror", ""},
    {18, false, "visual_tomato_pack_tonesphere", ""},
    {19, true, "frag_rng_ayu", "ayu_uncap"},
    {20, false, "gaugestart_30|gaugegain_70", ""},
    {21, true, "combo_100-frag_1", "frags_yume"},
    {22, false, "audio_gcemptyhit_pack_groovecoaster", ""},
    {23, true, "gauge_saya", "skill_saya_uncap"},
    {24, false, "gauge_chuni", ""},
    {25, false, "kantandeshou", ""},
    {26, true, "gauge_haruna", ""},
    {27, true, "frags_nono", ""},
    {28, true, "gauge_pandora", ""},
    {29, true, "gauge_regulus", ""},
    {30, true, "omatsuri_daynight", "skill_kanae_uncap"},
    {31, false, "", ""},
    {32, false, "", ""},
    {33, false, "sometimes(note_mirror|frag_plus_5)", ""},
    {34, true, "scoreclear_aa|visual_scoregauge", "skill_doroc_uncap"},
    {35, false, "gauge_tempest", ""},
    {36, true, "gauge_hard", ""},
    {37, false, "gauge_ilith_summer", ""},
    {38, false, "", ""},
    {39, false, "note_mirror|visual_hide_far", ""},
    {40, false, "frags_ongeki", ""},
    {41, false, "gauge_areus", ""},
    {42, true, "gauge_seele", ""},
    {43, true, "gauge_isabelle", ""},
    {44, false, "gauge_exhaustion", ""},
    {45, false, "skill_lagrange", ""},
    {46, false, "gauge_safe_10", ""},
    {47, false, "frags_nami", ""},
    {48, false, "skill_elizabeth", ""},
    {49, false, "skill_lily", ""},
    {50, false, "skill_kanae_midsummer", ""},
    {51, false, "", ""},
    {52, false, "", ""},
    {53, false, "visual_ghost_skynotes", ""},
    {54, false, "skill_vita", ""},
    {55, false, "skill_fatalis", ""},
    {56, false, "frags_ongeki_slash", ""},
    {57, false, "frags_ongeki_hard", ""},
    {58, false, "skill_amane", ""},
    {59, false, "skill_kou_winter", ""},
    {60, false, "", ""},
    {61, false, "gauge_hard|note_mirror", ""},
    {62, false, "skill_shama", ""},
    {63, false, "skill_milk", ""},
    {64, false, "skill_shikoku", ""},
    {65, false, "skill_mika", ""},
    {66, true, "skill_mithra", ""},
    {67, false, "skill_toa", ""},
    {68, false, "skill_nami_twilight", ""},
    {69, false, "skill_ilith_ivy", ""},
    {70, false, "skill_hikari_vanessa", ""},
    {71, true, "", "skill_maya_uncap"},
    {72, false, "skill_intruder", ""},
    {73, true, "skill_luin", "skill_luin_uncap"},
    {74, false, "", ""},
    {75, false, "skill_aichan", ""},
    {76, false, "skill_luna_ilot", ""},
    {77, false, "skill_eto_hoppe", ""},
    {78, false, "skill_nell", ""},
    {79, false, "skill_chinatsu", ""},
    {80, false, "skill_tsumugi", ""},
    {81, true, "skill_nai", ""},
    {82, true, "skill_selene", ""},
    {83, false, "skill_salt", ""},
    {84, true, "skill_acid", ""},
    {85, false, "skill_hikari_selene", ""},
    {86, false, "skill_hikari_clear", ""},
    {87, false, "skill_tairitsu_fail", ""},
    {88, false, "skill_nami_sui", ""},
    {89, true, "skill_nonoka", "skill_nonoka_uncap"},
    {90, false, "skill_vita_arc", ""},
    {91, false, "", ""},
    {92, false, "skill_hikari_tairitsu_debut", ""},
    {93, false, "skill_hp_slow_drain", ""},
    {94, false, "skill_hprate_based_on_hp", ""},
    {95, false, "skill_lost_to_85", ""},
    {96, false, "skill_frag_doubled_after_earning_X", ""},
    {97, false, "skill_saya_konzetsu", ""},
    {98, false, "skill_insight_konzetsu", ""},
    {99, false, "frags_preferred_song", ""},
};
static const int g_char_info_n = (int)(sizeof(g_char_info) / sizeof(g_char_info[0]));

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

static int g_toggle[128] = {0};
static bool g_toggle_loaded = false;

static uintptr_t g_base = 0;

static Trampoline g_refresh;
static Trampoline g_render;
static Trampoline g_awaken;
static Trampoline g_modify;
static Trampoline g_pborder;
static Trampoline g_barfill;

static const char *AWAKEN_KEY = "awakened";

static void setToggle(int id, int on)
{
    if (id < 0 || id >= 128)
        return;
    g_toggle[id] = on ? 1 : 0;
}

static void toggleToCsv(char *out, size_t sz)
{
    out[0] = 0;
    size_t len = 0;
    for (int i = 0; i < 128; i++)
    {
        if (!g_toggle[i])
            continue;
        int m = snprintf(out + len, sz - len, "%s%d", len ? "," : "", i);
        if (m <= 0 || (size_t)m >= sz - len)
            break;
        len += (size_t)m;
    }
}

static void loadToggle()
{
    char buf[512];
    gameGetString(g_base, AWAKEN_KEY, "", buf, sizeof(buf));

    for (int i = 0; i < 128; i++)
        g_toggle[i] = 0;

    int id = -1;
    for (const char *p = buf;; p++)
    {
        char ch = *p;
        if (ch >= '0' && ch <= '9')
        {
            if (id < 128)
                id = (id < 0 ? 0 : id) * 10 + (ch - '0');
            continue;
        }
        if (id >= 0 && id < 128)
            g_toggle[id] = 1;
        id = -1;
        if (!ch)
            break;
    }
}

static void ensureToggleLoaded()
{
    if (g_toggle_loaded)
        return;
    loadToggle();
    g_toggle_loaded = true;
}

static void saveToggle()
{
    ensureToggleLoaded();

    char val[512];
    toggleToCsv(val, sizeof(val));
    gameSetString(g_base, AWAKEN_KEY, val);
}

static const CharInfo *findChar(int id)
{
    for (int i = 0; i < g_char_info_n; i++)
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
    const char *sid = g_toggle[cid] ? ci->skill_id_uncap : ci->skill_id;

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
    if (!obj || cid < 0 || cid >= 128 || !hasUncap(cid))
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
    if (!(obj && cid == 71 && !g_toggle[71] && !isCharSealed(g_base)))
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
    ensureToggleLoaded();

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
    if (cid >= 0 && cid < 128 && hasUncap(cid))
        return ret | 1;
    return ret;
}

int64_t onModifyCharState(int64_t a1, int64_t a2, char a3, char a4, char a5)
{

    uintptr_t lr;
    __asm__ __volatile__("mov %0, x30" : "=r"(lr));

    ensureToggleLoaded();

    uintptr_t caller = lr - g_base;
    int cid = a2 ? *(int *)(a2 + 12) : -1;
    bool isAwaken = (caller == OFF_AWAKEN_CALLER);

    if (isAwaken && a2 && cid >= 0 && cid < 128 && hasUncap(cid))
    {
        setToggle(cid, g_toggle[cid] ? 0 : 1);
        *(uint8_t *)(a2 + 201) = (uint8_t)g_toggle[cid];
        *(uint8_t *)(a2 + 280) = (uint8_t)g_toggle[cid];
        *(int *)(a2 + 168) = (g_toggle[cid] ? 1 : 0);
        *(uint8_t *)(a2 + 202) = (uint8_t)(g_toggle[cid] ? 0 : 1);
        *(int *)(a2 + 192) = 30;
        rebuildSkill(a2, cid);
        saveToggle();
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
    __asm__ __volatile__("mov %0, x30" : "=r"(lr));

    ensureToggleLoaded();

    uintptr_t off = lr - g_base;

    int64_t obj = a1 ? *(int64_t *)(a1 + 784) : 0;
    int cid = obj ? *(int *)(obj + 12) : -1;
    bool isAwakenBtn = (off == OFF_AWAKEN_CALL);

    int64_t ret = ((PFN_RefreshCharState)g_refresh.cave)(a1, a2);

    if (isAwakenBtn && obj && cid >= 0 && cid < 128 && hasUncap(cid))
        *(uint8_t *)(obj + 201) = (uint8_t)g_toggle[cid];

    if (obj && cid >= 0 && cid < 128 && hasUncap(cid))
    {
        *(uint8_t *)(obj + 202) = (uint8_t)(g_toggle[cid] ? 0 : 1);
        *(int *)(obj + 192) = 30;
    }

    if (isAwakenBtn)
        g_RenderCharUI(a1);

    return ret;
}

int64_t setupSkill(int64_t obj, int char_id)
{

    ensureToggleLoaded();

    *reinterpret_cast<int *>(obj + 12) = char_id;

    *reinterpret_cast<uint8_t *>(obj + 201) = 0;
    if (char_id >= 0 && char_id < 128 && hasUncap(char_id))
        *reinterpret_cast<uint8_t *>(obj + 201) = (uint8_t)g_toggle[char_id];

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

        bool uncap = (char_id >= 0 && char_id < 128) ? (g_toggle[char_id] != 0) : false;
        const char *sid = skillIdForChar(char_id, uncap);
        if (sid)
            buildOnlineSkill(obj, sid);
        v5 = 0;
        break;
    }
    }

    *reinterpret_cast<int *>(obj + 168) = v5;
    *reinterpret_cast<int *>(obj + 192) = hasUncap(char_id) ? 30 : 20;

    if (char_id >= 0 && char_id < 128 && hasUncap(char_id))
    {
        *flag280 = (uint8_t)g_toggle[char_id];
        *reinterpret_cast<int *>(obj + 168) = (g_toggle[char_id] ? 1 : 0);
        *reinterpret_cast<uint8_t *>(obj + 202) = (uint8_t)(g_toggle[char_id] ? 0 : 1);
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

    if (!dlsym(handle, "_ZdlPv"))
        return;

    g_base = base;

    (void)replaceFunction(base + OFF_Init, (void *)&setupSkill);

    g_refresh = installTrampoline(base + OFF_RefreshCharState, (void *)&onRefreshCharState);
    g_render = installTrampoline(base + OFF_RenderCharUI, (void *)&onRenderCharUI);
    g_awaken = installTrampoline(base + OFF_AwakenCheck, (void *)&onAwakenCheck);
    g_modify = installTrampoline(base + OFF_ModifyCharState, (void *)&onModifyCharState);
    g_pborder = installTrampoline(base + OFF_SetCharBorder, (void *)&onSetCharBorder);
    g_barfill = installTrampoline(base + OFF_SetBarFill, (void *)&onSetBarFill);
}
