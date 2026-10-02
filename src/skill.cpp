
#include <dlfcn.h>
#include <cstdint>
#include <cstdio>

#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

typedef void (*PFN_FileNameCtor)(void *obj, int charId);
typedef void *(*PFN_StringCtor)(void *buf, const char *str);
typedef void (*PFN_RefAdd)(void *obj);
typedef void (*PFN_OperatorDelete)(void *);

typedef void *(*PFN_CharacterAbility)(void *str, int, int, int, int, int, int, double);
typedef void *(*PFN_CharacterAbilityModifyFragmentInterpolated)(void *str, unsigned int, unsigned int, unsigned int, unsigned int, char, char, char, char);
typedef void *(*PFN_CharacterAbilityModifyFragmentOnResult)(void *str, unsigned int, char, char, char, char, unsigned int, int, float);
typedef void *(*PFN_CharacterAbilityModifyFragmentOnFail)(void *str, unsigned int, char, char, char, char);
typedef void *(*PFN_CharacterAbilityGaugeRateModify)(void *str, char, char, char, char, unsigned int, float, float, float, float, float, float);
typedef void *(*PFN_CharacterAbilityModifyFragmentRNG)(void *str, char, char, char, char);
typedef void *(*PFN_CharacterAbilityGaugeChunithm)(void *str, char);
typedef unsigned int (*PFN_GradeCheck)(unsigned int);
typedef void *(*PFN_CharacterAbilityModifyFragmentOnGrade)(void *str, char, unsigned int, unsigned int, char, char, char, char);
typedef void *(*PFN_CharacterAbilityGaugeFixedValueCondition)(void *str, char, char, char, char, unsigned int, char);
typedef void *(*PFN_CharacterAbilitySometimesModifyFragmentMirrored)(void *str, int);
typedef void *(*PFN_CharacterAbilityClearBasedOnScore)(void *str, int, int);
typedef void *(*PFN_CharacterAbilityTempestGauge)(void *str);
typedef void *(*PFN_CharacterAbilityLowComboNoHpGain)(void *str);
typedef void *(*PFN_CharacterAbilityModifyFragmentOnResultOngeki)(void *str, char, char, char);
typedef void *(*PFN_CharacterAbilityHealthLossBasedOnCombo)(void *str);
typedef void *(*PFN_CharacterAbilityDisableGaugeGainAtSongEnd)(void *str);
typedef void *(*PFN_CharacterAbilityMaxHealthReducesBasedOnCurrentHealth)(void *str);
typedef void *(*PFN_CharacterAbilityGaugeRateBasedOnProgress)(void *str);
typedef void *(*PFN_CharacterAbilityReunion)(void *str);
typedef void *(*PFN_CharacterAbilitySafe)(void *str);
typedef void *(*PFN_CharacterAbilityModifyFragmentNami)(void *str);
typedef void *(*PFN_CharacterAbilityModifyFragmentElizabeth)(void *str);
typedef void *(*PFN_CharacterAbilityModifyFragmentLily)(void *str);
typedef void *(*PFN_CharacterAbilityKanaeMidsummer)(void *str);
typedef void *(*PFN_CharacterAbilityDescriptionOnly)(void *str, char, char, char, char);
typedef void *(*PFN_CharacterAbilityFatalisGauge)(void *str);
typedef void *(*PFN_CharacterAbilityAmane)(void *str);
typedef void *(*PFN_CharacterAbilityKouWinter)(void *str);
typedef void *(*PFN_CharacterAbilityShamaMilk)(void *str, char);
typedef void *(*PFN_CharacterAbilityAwardBonusBasedOnCombo)(void *str, unsigned int);
typedef void *(*PFN_CharacterAbilityNamiTwilight)(void *str);
typedef void *(*PFN_CharacterAbilityBonusesBasedOnPeakHealth)(void *str, char, int);
typedef void *(*PFN_CharacterAbilityViewNoteResults)(void *str);
typedef void *(*PFN_CharacterAbilityIlithAwakened)(void *str);
typedef void *(*PFN_CharacterAbilityShirabeUncap)(void *str);
typedef void *(*PFN_CharacterAbilityClearBasedOnBestGrade)(void *str);
typedef void *(*PFN_CharacterAbilityGainStatsFromGauge)(void *str);
typedef void *(*PFN_CharacterAbilityInsightGauge)(void *str);
typedef void *(*PFN_CharacterAbilityModifyFragmentLuin)(void *str, char);
typedef void *(*PFN_CharacterAbilityPreferredSongDaily)(void *str);
typedef void *(*PFN_CharacterAbilityLunaIlot)(void *str);
typedef void *(*PFN_CharacterAbilityEtoHoppe)(void *str);
typedef void *(*PFN_CharacterAbilityHideLifebarNell)(void *str);
typedef void *(*PFN_CharacterAbilityBonusesBasedOnOngeki)(void *str, char, int);
typedef void *(*PFN_CharacterAbilityLoseFragOnLost)(void *str);
typedef void *(*PFN_CharacterAbilityComboIntervalBasedOnNoteCount)(void *str);
typedef void *(*PFN_CharacterAbilityLoseHealthGainFragAtIntervals)(void *str);
typedef void *(*PFN_CharacterAbilityDjmaxFever)(void *str, char, int);
typedef void *(*PFN_CharacterAbilityAlterRankOnClearModifyFrags)(void *str, char);
typedef void *(*PFN_CharacterAbilityLoseGaugeBasedOnNear)(void *str);
typedef void *(*PFN_CharacterAbilityNextstageDaily)(void *str);
typedef void *(*PFN_CharacterAbilitySlowHpDrain)(void *str);
typedef void *(*PFN_CharacterAbilityHpRateBasedOnHp)(void *str);
typedef void *(*PFN_CharacterAbilityHighHpLossAtHighHp)(void *str);
typedef void *(*PFN_CharacterAbilityFragDoubledAfterEarningX)(void *str);
typedef void *(*PFN_CharacterAbilitySayaKonzetsu)(void *str);
typedef void *(*PFN_CharacterAbilityInsightKonzetsu)(void *str);
typedef void *(*PFN_CharacterAbilityPreferredSong)(void *str);

typedef void (*PFN_SetCharBorder)(int64_t a1, char a2);
typedef void (*PFN_SetBarFill)(int64_t bar_node, float fill);
typedef void (*PFN_BuildStringVector)(void *vec_out, const void *str_array, size_t count);
typedef void *(*PFN_BuildIntVector)(void *vec_out, const void *data, size_t count);
typedef void (*PFN_AssignSongList)(void *dest, const void *begin, const void *end);
typedef void (*PFN_AssignIntList)(void *dest, const void *begin, const void *end);
typedef void (*PFN_DestroyStringVector)(void *vec);

typedef int64_t (*PFN_RefreshCharState)(int64_t a1, char a2);
typedef int64_t (*PFN_RenderCharUI)(int64_t a1);
typedef int (*PFN_AwakenCheck)(int64_t a1);
typedef int64_t (*PFN_ModifyCharState)(int64_t a1, int64_t a2, char a3, char a4, char a5);

static PFN_FileNameCtor g_file_name_ctor = nullptr;
static PFN_StringCtor g_string_ctor = nullptr;
static PFN_RefAdd g_ref_add = nullptr;
static PFN_OperatorDelete g_operator_delete = nullptr;

static PFN_CharacterAbility g_CharacterAbility = nullptr;
static PFN_CharacterAbilityModifyFragmentInterpolated g_CharacterAbilityModifyFragmentInterpolated = nullptr;
static PFN_CharacterAbilityReunion g_CharacterAbilityReunion = nullptr;
static PFN_CharacterAbilityModifyFragmentOnResult g_CharacterAbilityModifyFragmentOnResult = nullptr;
static PFN_CharacterAbilityModifyFragmentOnFail g_CharacterAbilityModifyFragmentOnFail = nullptr;
static PFN_CharacterAbilityGaugeRateModify g_CharacterAbilityGaugeRateModify = nullptr;
static PFN_CharacterAbilityModifyFragmentRNG g_CharacterAbilityModifyFragmentRNG = nullptr;
static PFN_CharacterAbilityGaugeChunithm g_CharacterAbilityGaugeChunithm = nullptr;
static PFN_GradeCheck g_GradeCheck = nullptr;
static PFN_CharacterAbilityModifyFragmentOnGrade g_CharacterAbilityModifyFragmentOnGrade = nullptr;
static PFN_CharacterAbilityGaugeFixedValueCondition g_CharacterAbilityGaugeFixedValueCondition = nullptr;
static PFN_CharacterAbilitySometimesModifyFragmentMirrored g_CharacterAbilitySometimesModifyFragmentMirrored = nullptr;
static PFN_CharacterAbilityClearBasedOnScore g_CharacterAbilityClearBasedOnScore = nullptr;
static PFN_CharacterAbilityTempestGauge g_CharacterAbilityTempestGauge = nullptr;
static PFN_CharacterAbilityLowComboNoHpGain g_CharacterAbilityLowComboNoHpGain = nullptr;
static PFN_CharacterAbilityModifyFragmentOnResultOngeki g_CharacterAbilityModifyFragmentOnResultOngeki = nullptr;
static PFN_CharacterAbilityHealthLossBasedOnCombo g_CharacterAbilityHealthLossBasedOnCombo = nullptr;
static PFN_CharacterAbilityDisableGaugeGainAtSongEnd g_CharacterAbilityDisableGaugeGainAtSongEnd = nullptr;
static PFN_CharacterAbilityMaxHealthReducesBasedOnCurrentHealth g_CharacterAbilityMaxHealthReducesBasedOnCurrentHealth = nullptr;
static PFN_CharacterAbilityGaugeRateBasedOnProgress g_CharacterAbilityGaugeRateBasedOnProgress = nullptr;
static PFN_CharacterAbilitySafe g_CharacterAbilitySafe = nullptr;
static PFN_CharacterAbilityModifyFragmentNami g_CharacterAbilityModifyFragmentNami = nullptr;
static PFN_CharacterAbilityModifyFragmentElizabeth g_CharacterAbilityModifyFragmentElizabeth = nullptr;
static PFN_CharacterAbilityModifyFragmentLily g_CharacterAbilityModifyFragmentLily = nullptr;
static PFN_CharacterAbilityKanaeMidsummer g_CharacterAbilityKanaeMidsummer = nullptr;
static PFN_CharacterAbilityDescriptionOnly g_CharacterAbilityDescriptionOnly = nullptr;
static PFN_CharacterAbilityFatalisGauge g_CharacterAbilityFatalisGauge = nullptr;
static PFN_CharacterAbilityAmane g_CharacterAbilityAmane = nullptr;
static PFN_CharacterAbilityKouWinter g_CharacterAbilityKouWinter = nullptr;
static PFN_CharacterAbilityShamaMilk g_CharacterAbilityShamaMilk = nullptr;
static PFN_CharacterAbilityAwardBonusBasedOnCombo g_CharacterAbilityAwardBonusBasedOnCombo = nullptr;
static PFN_CharacterAbilityNamiTwilight g_CharacterAbilityNamiTwilight = nullptr;
static PFN_CharacterAbilityBonusesBasedOnPeakHealth g_CharacterAbilityBonusesBasedOnPeakHealth = nullptr;
static PFN_CharacterAbilityViewNoteResults g_CharacterAbilityViewNoteResults = nullptr;
static PFN_CharacterAbilityIlithAwakened g_CharacterAbilityIlithAwakened = nullptr;
static PFN_CharacterAbilityShirabeUncap g_CharacterAbilityShirabeUncap = nullptr;
static PFN_CharacterAbilityClearBasedOnBestGrade g_CharacterAbilityClearBasedOnBestGrade = nullptr;
static PFN_CharacterAbilityGainStatsFromGauge g_CharacterAbilityGainStatsFromGauge = nullptr;
static PFN_CharacterAbilityInsightGauge g_CharacterAbilityInsightGauge = nullptr;
static PFN_CharacterAbilityModifyFragmentLuin g_CharacterAbilityModifyFragmentLuin = nullptr;
static PFN_CharacterAbilityPreferredSongDaily g_CharacterAbilityPreferredSongDaily = nullptr;
static PFN_CharacterAbilityLunaIlot g_CharacterAbilityLunaIlot = nullptr;
static PFN_CharacterAbilityEtoHoppe g_CharacterAbilityEtoHoppe = nullptr;
static PFN_CharacterAbilityHideLifebarNell g_CharacterAbilityHideLifebarNell = nullptr;
static PFN_CharacterAbilityBonusesBasedOnOngeki g_CharacterAbilityBonusesBasedOnOngeki = nullptr;
static PFN_CharacterAbilityLoseFragOnLost g_CharacterAbilityLoseFragOnLost = nullptr;
static PFN_CharacterAbilityComboIntervalBasedOnNoteCount g_CharacterAbilityComboIntervalBasedOnNoteCount = nullptr;
static PFN_CharacterAbilityLoseHealthGainFragAtIntervals g_CharacterAbilityLoseHealthGainFragAtIntervals = nullptr;
static PFN_CharacterAbilityDjmaxFever g_CharacterAbilityDjmaxFever = nullptr;
static PFN_CharacterAbilityAlterRankOnClearModifyFrags g_CharacterAbilityAlterRankOnClearModifyFrags = nullptr;
static PFN_CharacterAbilityLoseGaugeBasedOnNear g_CharacterAbilityLoseGaugeBasedOnNear = nullptr;
static PFN_CharacterAbilityNextstageDaily g_CharacterAbilityNextstageDaily = nullptr;
static PFN_CharacterAbilitySlowHpDrain g_CharacterAbilitySlowHpDrain = nullptr;
static PFN_CharacterAbilityHpRateBasedOnHp g_CharacterAbilityHpRateBasedOnHp = nullptr;
static PFN_CharacterAbilityHighHpLossAtHighHp g_CharacterAbilityHighHpLossAtHighHp = nullptr;
static PFN_CharacterAbilityFragDoubledAfterEarningX g_CharacterAbilityFragDoubledAfterEarningX = nullptr;
static PFN_CharacterAbilitySayaKonzetsu g_CharacterAbilitySayaKonzetsu = nullptr;
static PFN_CharacterAbilityInsightKonzetsu g_CharacterAbilityInsightKonzetsu = nullptr;
static PFN_CharacterAbilityPreferredSong g_CharacterAbilityPreferredSong = nullptr;

static PFN_BuildStringVector g_BuildStringVector = nullptr;
static PFN_BuildIntVector g_BuildIntVector = nullptr;
static PFN_AssignSongList g_AssignSongList = nullptr;
static PFN_AssignIntList g_AssignIntList = nullptr;
static PFN_DestroyStringVector g_DestroyStringVector = nullptr;

static const int g_has_uncap[] = {
    0, 1, 2, 4, 5, 10, 11, 12, 13, 19, 21, 23, 26, 27, 28, 29, 30,
    34, 36, 42, 43, 66, 71, 73, 81, 82, 84, 89};
static const int g_has_uncap_n = (int)(sizeof(g_has_uncap) / sizeof(g_has_uncap[0]));
static int g_toggle[128] = {0};
static bool g_toggle_loaded = false;

static uintptr_t g_base = 0;
static PFN_RenderCharUI g_RenderCharUI = nullptr;

static Trampoline g_refresh;
static Trampoline g_render;
static Trampoline g_awaken;
static Trampoline g_modify;
static Trampoline g_pborder;
static Trampoline g_barfill;

static bool hasUncap(int id)
{
    for (int i = 0; i < g_has_uncap_n; i++)
        if (g_has_uncap[i] == id)
            return true;
    return false;
}

struct GameStr
{
    uint64_t a, b, c;
};

static void makeGameStr(GameStr *s, const char *v)
{
    g_string_ctor(s, v);
}

static const char *gameStrText(const GameStr *s)
{
    return (s->a & 1) ? reinterpret_cast<const char *>(s->c)
                      : reinterpret_cast<const char *>(s) + 1;
}

static void freeGameStr(GameStr *s)
{
    if (s->a & 1)
        g_operator_delete(reinterpret_cast<void *>(s->c));
}

static void awakenRead(char *out, size_t sz)
{
    out[0] = 0;

    typedef GameStr (*PFN_UDGetString)(void *, const char *, void *);
    GameStr def = {};
    makeGameStr(&def, "");
    GameStr s = ((PFN_UDGetString)(g_base + OFF_UserDefaultGetString))(nullptr, "awakened", &def);
    snprintf(out, sz, "%s", gameStrText(&s));
    freeGameStr(&s);
    freeGameStr(&def);
}

static bool awakenWrite(const char *val)
{
    typedef bool (*PFN_UDSetString)(void *, const char *, void *);
    GameStr v = {};
    makeGameStr(&v, val);
    bool ok = ((PFN_UDSetString)(g_base + OFF_UserDefaultSetString))(nullptr, "awakened", &v);
    freeGameStr(&v);
    return ok;
}

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
    awakenRead(buf, sizeof(buf));

    for (int i = 0; i < 128; i++)
        g_toggle[i] = 0;

    int id = -1;
    for (const char *p = buf;; p++)
    {
        char c = *p;
        if (c >= '0' && c <= '9')
        {
            if (id < 128)
                id = (id < 0 ? 0 : id) * 10 + (c - '0');
            continue;
        }
        if (id >= 0 && id < 128)
            g_toggle[id] = 1;
        id = -1;
        if (!c)
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
    awakenWrite(val);
}

// cocos Node::setVisible via vtable slot +336 (0x150)
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

// cocos Widget::setEnabled via vtable slot +1368 (0x558)
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

// OFF_SealCtx = 封印上下文；+120 → 当前技能槽对象，+12 = 封印标志
static bool isCharSealed(uintptr_t base)
{
    if (!base)
        return false;
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
    setNodeVisible(a1 + 1264, true);   // awaken-icon / awaken-icon-closed 图标
    setNodeVisible(a1 + 1272, true);   // PRE-AWAKENING / AWAKENED 文本
    setNodeVisible(a1 + 1280, true);   // 觉醒按钮底图
    setWidgetEnabled(a1 + 1280, true); // 未觉醒时原始会 disabled → 强制可用
}

static void force72SealButton(int64_t a1, int64_t obj, int cid)
{
    if (!obj || cid != 72)
        return;
    setNodeVisible(a1 + 1256, true); // 按钮底图
    setWidgetEnabled(a1 + 1256, true);
    setNodeVisible(a1 + 1240, true); // lock / active 图标
    setNodeVisible(a1 + 1248, true); // SEALED / SKILL ACTIVE 文本
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
    freeGameStr(&s);
}

extern "C" int64_t onRenderCharUI(int64_t a1)
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

// obj+272 != 0 = 有觉醒数据；可觉醒角色强制返回 1（否则按钮点不动）
extern "C" int onAwakenCheck(int64_t a1)
{
    int ret = ((PFN_AwakenCheck)g_awaken.cave)(a1);

    int cid = a1 ? *(int *)(a1 + 12) : -1;
    if (cid >= 0 && cid < 128 && hasUncap(cid))
        return ret | 1;
    return ret;
}

static void setupAwakenSkill(int64_t obj, int cid)
{
    char s[24] = {};
    void *ab = nullptr;

    switch (cid)
    {
    case 4:
        if (g_toggle[4])
        {
            g_string_ctor(s, "visual_ink");
            ab = g_CharacterAbility(s, 0, 0, 0, 0, 7, 0, 0.0);
        }
        else
        {
            g_string_ctor(s, "note_mirror");
            ab = g_CharacterAbility(s, 0, 0, 1, 0, 0, 0, 0.0);
        }
        break;

    case 10:
        if (g_toggle[10])
        {
            g_string_ctor(s, "ilith_awakened_skill");
            ab = g_CharacterAbilityIlithAwakened(s);
        }
        else
        {
            g_string_ctor(s, "gauge_hard|fail_frag_minus_100");
            ab = g_CharacterAbilityModifyFragmentOnFail(s, 4294967196u, 1, 0, 0, 0);
        }
        break;

    case 11:
        if (g_toggle[11])
        {
            g_string_ctor(s, "eto_uncap");
            ab = g_CharacterAbilityDescriptionOnly(s, 0, 0, 0, 0);
            if (ab)
            {
                *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 56) = 1;
                *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 58) = 1;
            }
        }
        else
        {
            g_string_ctor(s, "frag_plus_5_side_light");
            ab = g_CharacterAbilityModifyFragmentOnResult(s, 5u, 0, 0, 0, 0, 0u, 0, 0.0f);
            if (ab)
            {
                int diff_data[4] = {0, 1, 2, 3};
                int side_data[1] = {0};
                char diff_vec[24] = {};
                char side_vec[24] = {};
                g_BuildIntVector(diff_vec, diff_data, 4);
                g_BuildIntVector(side_vec, side_data, 1);
                g_AssignSongList((char *)ab + 88, nullptr, nullptr);
                g_AssignIntList((char *)ab + 112, *(void **)diff_vec, *(void **)(diff_vec + 8));
                g_AssignIntList((char *)ab + 136, *(void **)side_vec, *(void **)(side_vec + 8));
            }
        }
        break;

    case 12:
        if (g_toggle[12])
        {
            g_string_ctor(s, "luna_uncap");
            ab = g_CharacterAbilityDescriptionOnly(s, 0, 0, 0, 0);
            if (ab)
            {
                *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 56) = 1;
                *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 58) = 1;
            }
        }
        else
        {
            g_string_ctor(s, "visual_hide_hp");
            ab = g_CharacterAbility(s, 0, 0, 0, 0, 3, 0, 0.0);
        }
        break;

    case 13:
        if (g_toggle[13])
        {
            g_string_ctor(s, "shirabe_entry_fee");
            ab = g_CharacterAbilityShirabeUncap(s);
        }
        else
        {
            g_string_ctor(s, "frag_plus_5_side_conflict");
            ab = g_CharacterAbilityModifyFragmentOnResult(s, 5u, 0, 0, 0, 0, 0u, 0, 0.0f);
            if (ab)
            {
                int diff_data[4] = {0, 1, 2, 3};
                int side_data[1] = {1};
                char diff_vec[24] = {};
                char side_vec[24] = {};
                g_BuildIntVector(diff_vec, diff_data, 4);
                g_BuildIntVector(side_vec, side_data, 1);
                g_AssignSongList((char *)ab + 88, nullptr, nullptr);
                g_AssignIntList((char *)ab + 112, *(void **)diff_vec, *(void **)(diff_vec + 8));
                g_AssignIntList((char *)ab + 136, *(void **)side_vec, *(void **)(side_vec + 8));
            }
        }
        break;

    case 19:
        if (g_toggle[19])
        {
            g_string_ctor(s, "ayu_uncap");
            ab = g_CharacterAbilityDescriptionOnly(s, 0, 0, 0, 0);
            if (ab)
            {
                *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 56) = 1;
                *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 58) = 1;
            }
        }
        else
        {
            g_string_ctor(s, "frag_rng_ayu");
            ab = g_CharacterAbilityModifyFragmentRNG(s, 0, 0, 0, 0);
        }
        break;

    case 21:
        if (g_toggle[21])
        {
            g_string_ctor(s, "frags_yume");
            ab = g_CharacterAbilityModifyFragmentOnResult(s, 0, 0, 0, 0, 0, 0u, 0, 0.0f);
            if (ab)
            {
                *reinterpret_cast<int *>(reinterpret_cast<char *>(ab) + 164) = 5;
                *reinterpret_cast<int *>(reinterpret_cast<char *>(ab) + 176) = 10;
            }
        }
        else
        {
            g_string_ctor(s, "combo_100-frag_1");
            ab = g_CharacterAbilityModifyFragmentOnResult(s, 0u, 0, 0, 0, 0, 0u, 0, 0.0f);
            if (ab)
                *reinterpret_cast<uint64_t *>(reinterpret_cast<char *>(ab) + 168) = 0x100000064ULL;
        }
        break;

    case 30:
        if (g_toggle[30])
        {
            g_string_ctor(s, "skill_kanae_uncap");
            ab = g_CharacterAbilityDescriptionOnly(s, 0, 0, 0, 0);
            if (ab)
            {
                *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 56) = 1;
                *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 58) = 1;
            }
        }
        else
        {
            g_string_ctor(s, "omatsuri_daynight");
            ab = g_CharacterAbility(s, 0, 0, 0, 0, 1, 0, 0.0);
            if (ab)
            {
                g_string_ctor((char *)ab + 64, "omatsuri");
                char songbuf[48] = {};
                g_string_ctor(songbuf, "eveninginscarlet");
                g_string_ctor(songbuf + 24, "summerfireworks");
                int diff_data[5] = {0, 1, 2, 3, 4};
                int side_data[4] = {1, 0, 2, 3};
                char songs_vec[24] = {};
                char diff_vec[24] = {};
                char side_vec[24] = {};
                g_BuildStringVector(songs_vec, songbuf, 2);
                g_BuildIntVector(diff_vec, diff_data, 5);
                g_BuildIntVector(side_vec, side_data, 4);
                g_AssignSongList((char *)ab + 88, *(void **)songs_vec, *(void **)(songs_vec + 8));
                g_AssignIntList((char *)ab + 112, *(void **)diff_vec, *(void **)(diff_vec + 8));
                g_AssignIntList((char *)ab + 136, *(void **)side_vec, *(void **)(side_vec + 8));
                g_DestroyStringVector(songs_vec);
                if (*(uint64_t *)songbuf & 1)
                    g_operator_delete(*reinterpret_cast<void **>(songbuf + 16));
                if (*(uint64_t *)(songbuf + 24) & 1)
                    g_operator_delete(*reinterpret_cast<void **>(songbuf + 40));
            }
        }
        break;

    case 34:
        if (g_toggle[34])
        {
            g_string_ctor(s, "skill_doroc_uncap");
            ab = g_CharacterAbilityClearBasedOnBestGrade(s);
        }
        else
        {
            g_string_ctor(s, "scoreclear_aa|visual_scoregauge");
            ab = g_CharacterAbilityClearBasedOnScore(s, 9500000, 4);
        }
        break;

    case 71: // 未觉醒 maya 不设技能对象，文案由 onRenderCharUI 覆盖
        ab = nullptr;
        if (g_toggle[71])
        {
            g_string_ctor(s, "skill_maya_uncap");
            ab = g_CharacterAbilityGainStatsFromGauge(s);
        }
        break;

    case 73:
        if (g_toggle[73])
        {
            g_string_ctor(s, "skill_luin_uncap");
            ab = g_CharacterAbilityModifyFragmentLuin(s, 1);
        }
        else
        {
            g_string_ctor(s, "skill_luin");
            ab = g_CharacterAbilityModifyFragmentLuin(s, 0);
        }
        break;

    case 89:
        if (g_toggle[89])
        {
            g_string_ctor(s, "skill_nonoka_uncap");
            ab = g_CharacterAbilityAlterRankOnClearModifyFrags(s, 1);
        }
        else
        {
            g_string_ctor(s, "skill_nonoka");
            ab = g_CharacterAbilityAlterRankOnClearModifyFrags(s, 0);
        }
        break;

    case 23:
        if (g_toggle[23])
        {
            g_string_ctor(s, "skill_saya_uncap");
            ab = g_CharacterAbilityViewNoteResults(s);
        }
        else
        {
            g_string_ctor(s, "gauge_saya");
            ab = g_CharacterAbilityGaugeRateModify(s, 0, 0, 0, 0, 3, 2.0, 0.0, 0.0, 3.0, 1.0, 1.0);
        }
        break;

    default:
        return;
    }

    *reinterpret_cast<void **>(obj + 208) = ab;
    if (*(uint64_t *)s & 1)
        g_operator_delete(*reinterpret_cast<void **>(s + 16));
    if (ab)
        g_ref_add(ab);
}

// 调用者 LR == OFF_AWAKEN_CALLER(0xC2406C) 时视为觉醒按钮点击
extern "C" int64_t onModifyCharState(int64_t a1, int64_t a2, char a3, char a4, char a5)
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
        setupAwakenSkill(a2, cid);
        saveToggle();
    }

    return ((PFN_ModifyCharState)g_modify.cave)(a1, a2, a3, a4, a5);
}

extern "C" void onSetCharBorder(int64_t a1, char a2)
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

extern "C" void onSetBarFill(int64_t bar_node, float fill)
{
    ((PFN_SetBarFill)g_barfill.cave)(bar_node, 1.0f);
}

extern "C" int64_t onRefreshCharState(int64_t a1, char a2)
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

extern "C" int64_t setupSkill(int64_t obj, int char_id)
{

    ensureToggleLoaded();

    *reinterpret_cast<int *>(obj + 12) = char_id;

    // obj+201 = g_toggle[id] (0=normal, 1=awakened)
    *reinterpret_cast<uint8_t *>(obj + 201) = 0;
    if (char_id >= 0 && char_id < 128 && hasUncap(char_id))
        *reinterpret_cast<uint8_t *>(obj + 201) = (uint8_t)g_toggle[char_id];

    g_file_name_ctor(reinterpret_cast<void *>(obj), char_id);

    *reinterpret_cast<uint8_t *>(obj + 280) = 0;

    int v5 = 0;
    uint8_t *v6 = reinterpret_cast<uint8_t *>(obj + 280);

    switch (char_id)
    {

    case 0:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_easy");
        void *ab = g_CharacterAbility(s, 0, 1, 0, 0, 0, 0, 0.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 2:
    {
        char s[24] = {};
        g_string_ctor(s, "frags_kou");
        void *ab = g_CharacterAbilityModifyFragmentInterpolated(s, 9000000u, 10000000u, 10u, 20u, 0, 0, 0, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 4:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 5:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_reunion");
        void *ab = g_CharacterAbilityReunion(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 7:
    case 36:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_hard");
        void *ab = g_CharacterAbility(s, 1, 0, 0, 0, 0, 0, 0.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 8:
    {
        char s[24] = {};
        g_string_ctor(s, "frag_plus_10_pack_stellights");
        void *ab = g_CharacterAbilityModifyFragmentOnResult(s, 10u, 0, 0, 0, 0, 0u, 0, 0.0f);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
        {
            char songbuf[48] = {};
            g_string_ctor(songbuf, "yozakurafubuki");
            g_string_ctor(songbuf + 24, "surrender");
            int diff_data[5] = {0, 1, 2, 3, 4};
            int side_data[4] = {1, 0, 2, 3};
            char songs_vec[24] = {};
            char diff_vec[24] = {};
            char side_vec[24] = {};
            g_BuildStringVector(songs_vec, songbuf, 2);
            g_BuildIntVector(diff_vec, diff_data, 5);
            g_BuildIntVector(side_vec, side_data, 4);

            g_AssignSongList((char *)ab + 88,
                             *(void **)songs_vec,
                             *(void **)(songs_vec + 8));
            g_AssignIntList((char *)ab + 112,
                            *(void **)diff_vec,
                            *(void **)(diff_vec + 8));
            g_AssignIntList((char *)ab + 136,
                            *(void **)side_vec,
                            *(void **)(side_vec + 8));

            g_DestroyStringVector(songs_vec);
            if (*(uint64_t *)songbuf & 1)
                g_operator_delete(*reinterpret_cast<void **>(songbuf + 16));
            if (*(uint64_t *)(songbuf + 24) & 1)
                g_operator_delete(*reinterpret_cast<void **>(songbuf + 40));

            g_ref_add(ab);
        }
        v5 = 0;
    }
        goto LABEL_5;

    case 9:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_easy|frag_plus_15_pst&prs");
        void *ab = g_CharacterAbilityModifyFragmentOnResult(s, 15u, 0, 1, 0, 0, 0u, 0, 0.0f);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
        {
            int diff_data[2] = {0, 1};
            int side_data[4] = {1, 0, 2, 3};

            char diff_vec[24] = {};
            char side_vec[24] = {};
            g_BuildIntVector(diff_vec, diff_data, 2);
            g_BuildIntVector(side_vec, side_data, 4);

            g_AssignSongList((char *)ab + 88, nullptr, nullptr);
            g_AssignIntList((char *)ab + 112,
                            *(void **)diff_vec,
                            *(void **)(diff_vec + 8));
            g_AssignIntList((char *)ab + 136,
                            *(void **)side_vec,
                            *(void **)(side_vec + 8));

            g_ref_add(ab);
        }
        v5 = 0;
    }
        goto LABEL_5;

    case 10:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 11:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 12:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 13:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 14:
    {
        char s[24] = {};
        g_string_ctor(s, "challenge_fullcombo_0gauge");
        void *ab = g_CharacterAbilityGaugeRateModify(s, 0, 0, 0, 0, 0, 1.0, 0.0, 1.0, 100.0, 1.0, 0.01);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 15:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_overflow");
        void *ab = g_CharacterAbility(s, 0, 0, 0, 1, 0, 0, 0.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 16:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_easy|note_mirror");
        void *ab = g_CharacterAbility(s, 0, 1, 1, 0, 0, 0, 0.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 17:
    {
        char s[24] = {};
        g_string_ctor(s, "note_mirror");
        void *ab = g_CharacterAbility(s, 0, 0, 1, 0, 0, 0, 0.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 18:
    {
        char s[24] = {};
        g_string_ctor(s, "visual_tomato_pack_tonesphere");
        void *ab = g_CharacterAbility(s, 0, 0, 0, 0, 4, 0, 0.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
        {
            g_string_ctor((char *)ab + 64, "tonesphere");

            int diff_data[5] = {0, 1, 2, 3, 4};
            int side_data[4] = {1, 0, 2, 3};

            char diff_vec[24] = {};
            char side_vec[24] = {};
            g_BuildIntVector(diff_vec, diff_data, 5);
            g_BuildIntVector(side_vec, side_data, 4);

            g_AssignSongList((char *)ab + 88, nullptr, nullptr);
            g_AssignIntList((char *)ab + 112,
                            *(void **)diff_vec,
                            *(void **)(diff_vec + 8));
            g_AssignIntList((char *)ab + 136,
                            *(void **)side_vec,
                            *(void **)(side_vec + 8));

            g_ref_add(ab);
        }
        v5 = 0;
    }
        goto LABEL_5;

    case 19:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 20:
    {
        char s[24] = {};
        g_string_ctor(s, "gaugestart_30|gaugegain_70");
        void *ab = g_CharacterAbilityGaugeRateModify(s, 0, 1, 0, 0, 1, 0.7, 30.0, 1.0, 1.0, 1.0, 1.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 21:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 22:
    {
        char s[24] = {};
        g_string_ctor(s, "audio_gcemptyhit_pack_groovecoaster");
        void *ab = g_CharacterAbility(s, 0, 0, 0, 0, 0, 1, 0.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
        {
            g_string_ctor((char *)ab + 64, "groovecoaster");

            char songbuf[120] = {};
            g_string_ctor(songbuf, "buchigireberserker");
            g_string_ctor(songbuf + 24, "aurgelmir");
            g_string_ctor(songbuf + 48, "temptationgc");
            g_string_ctor(songbuf + 72, "signof");
            g_string_ctor(songbuf + 96, "blackmind");
            int diff_data[5] = {0, 1, 2, 3, 4};
            int side_data[3] = {1, 0, 3};

            char songs_vec[24] = {};
            char diff_vec[24] = {};
            char side_vec[24] = {};
            g_BuildStringVector(songs_vec, songbuf, 5);
            g_BuildIntVector(diff_vec, diff_data, 5);
            g_BuildIntVector(side_vec, side_data, 3);

            g_AssignSongList((char *)ab + 88,
                             *(void **)songs_vec,
                             *(void **)(songs_vec + 8));
            g_AssignIntList((char *)ab + 112,
                            *(void **)diff_vec,
                            *(void **)(diff_vec + 8));
            g_AssignIntList((char *)ab + 136,
                            *(void **)side_vec,
                            *(void **)(side_vec + 8));

            g_DestroyStringVector(songs_vec);
            for (int i = 0; i < 5; i++)
                if (*(uint64_t *)(songbuf + i * 24) & 1)
                    g_operator_delete(*reinterpret_cast<void **>(songbuf + i * 24 + 16));

            g_ref_add(ab);
        }
        v5 = 0;
    }
        goto LABEL_5;

    case 23:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 24:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_chuni");
        void *ab = g_CharacterAbilityGaugeChunithm(s, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;

        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 25:
    {
        char s[24] = {};
        g_string_ctor(s, "kantandeshou");
        unsigned int grade = g_GradeCheck(5);
        void *ab = g_CharacterAbilityModifyFragmentOnGrade(s, 1, grade, 14, 0, 0, 0, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 26:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_haruna");
        void *ab = g_CharacterAbilityGaugeRateModify(s, 0, 0, 0, 0, 3, 0.4, 40.0, 1.0, 1.0, 1.0, 1.0);
        *reinterpret_cast<void **>(obj + 208) = ab;

        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 27:
    {
        char s[24] = {};
        g_string_ctor(s, "frags_nono");
        unsigned int grade = g_GradeCheck(5);
        void *ab = g_CharacterAbilityModifyFragmentOnGrade(s, 0, grade, 2525, 0, 0, 0, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 28:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_pandora");
        void *ab = g_CharacterAbilityGaugeFixedValueCondition(s, 1, 0, 0, 0, 20, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;

        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 29:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_regulus");
        void *ab = g_CharacterAbilityGaugeRateModify(s, 1, 0, 0, 0, 0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0);
        *reinterpret_cast<void **>(obj + 208) = ab;

        if (ab)
        {
            *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 184) = 1;
            *reinterpret_cast<int *>(reinterpret_cast<char *>(ab) + 188) = 0xBEEA6666;
            g_ref_add(ab);
        }
        v5 = 0;
    }
        goto LABEL_5;

    case 30:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 33:
    {
        char s[24] = {};
        g_string_ctor(s, "sometimes(note_mirror|frag_plus_5)");
        void *ab = g_CharacterAbilitySometimesModifyFragmentMirrored(s, 5);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 34:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 35:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_tempest");
        void *ab = g_CharacterAbilityTempestGauge(s);
        *reinterpret_cast<void **>(obj + 208) = ab;

        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 37:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_ilith_summer");
        void *ab = g_CharacterAbilityLowComboNoHpGain(s);
        *reinterpret_cast<void **>(obj + 208) = ab;

        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 39:
    {
        char s[24] = {};
        g_string_ctor(s, "note_mirror|visual_hide_far");
        void *ab = g_CharacterAbility(s, 0, 0, 1, 0, 8, 0, 0.0);
        *reinterpret_cast<void **>(obj + 208) = ab;

        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 40:
    {
        char s[24] = {};
        g_string_ctor(s, "frags_ongeki");
        void *ab = g_CharacterAbilityModifyFragmentOnResultOngeki(s, 0, 0, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;

        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 41:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_areus");
        void *ab = g_CharacterAbilityHealthLossBasedOnCombo(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 42:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_seele");
        void *ab = g_CharacterAbilityDisableGaugeGainAtSongEnd(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 43:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_isabelle");
        void *ab = g_CharacterAbilityMaxHealthReducesBasedOnCurrentHealth(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 44:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_exhaustion");
        void *ab = g_CharacterAbilityGaugeRateBasedOnProgress(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 45:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_lagrange");
        void *ab = g_CharacterAbility(s, 0, 0, 0, 0, 10, 0, 0.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 46:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_safe_10");
        void *ab = g_CharacterAbilitySafe(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 47:
    {
        char s[24] = {};
        g_string_ctor(s, "frags_nami");
        void *ab = g_CharacterAbilityModifyFragmentNami(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 48:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_elizabeth");
        void *ab = g_CharacterAbilityModifyFragmentElizabeth(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 49:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_lily");
        void *ab = g_CharacterAbilityModifyFragmentLily(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 50:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_kanae_midsummer");
        void *ab = g_CharacterAbilityKanaeMidsummer(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 53:
    {
        char s[24] = {};
        g_string_ctor(s, "visual_ghost_skynotes");
        void *ab = g_CharacterAbility(s, 0, 0, 0, 0, 11, 0, 0.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
        {
            g_string_ctor((char *)ab + 64, "musedash");

            int diff_data[5] = {0, 1, 2, 3, 4};
            int side_data[4] = {1, 0, 2, 3};

            char diff_vec[24] = {};
            char side_vec[24] = {};
            g_BuildIntVector(diff_vec, diff_data, 5);
            g_BuildIntVector(side_vec, side_data, 4);

            g_AssignSongList((char *)ab + 88, nullptr, nullptr);
            g_AssignIntList((char *)ab + 112,
                            *(void **)diff_vec,
                            *(void **)(diff_vec + 8));
            g_AssignIntList((char *)ab + 136,
                            *(void **)side_vec,
                            *(void **)(side_vec + 8));

            g_ref_add(ab);
        }
        v5 = 0;
    }
        goto LABEL_5;

    case 54:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_vita");
        void *ab = g_CharacterAbilityDescriptionOnly(s, 1, 0, 1, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 55:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_fatalis");
        void *ab = g_CharacterAbilityFatalisGauge(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 56:
    {
        char s[24] = {};
        g_string_ctor(s, "frags_ongeki_slash");
        void *ab = g_CharacterAbilityModifyFragmentOnResultOngeki(s, 0, 1, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 57:
    {
        char s[24] = {};
        g_string_ctor(s, "frags_ongeki_hard");
        void *ab = g_CharacterAbilityModifyFragmentOnResultOngeki(s, 1, 0, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 58:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_amane");
        void *ab = g_CharacterAbilityAmane(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
        {
            *reinterpret_cast<uint8_t *>((char *)ab + 56) = 1;
            g_ref_add(ab);
        }
        v5 = 0;
    }
        goto LABEL_5;

    case 59:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_kou_winter");
        void *ab = g_CharacterAbilityKouWinter(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 61:
    {
        char s[24] = {};
        g_string_ctor(s, "gauge_hard|note_mirror");
        void *ab = g_CharacterAbilityDescriptionOnly(s, 1, 0, 1, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 62:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_shama");
        void *ab = g_CharacterAbilityShamaMilk(s, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 63:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_milk");
        void *ab = g_CharacterAbilityShamaMilk(s, 1);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 64:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_shikoku");
        void *ab = g_CharacterAbilityGaugeRateModify(s, 1, 0, 0, 0, 0, 1.25, 100.0, 1.25, 1.0, 1.25, 1.0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 65:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_mika");
        void *ab = g_CharacterAbilityDescriptionOnly(s, 0, 0, 0, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
        {
            char songbuf[288] = {};
            g_string_ctor(songbuf, "aprilshowers");
            g_string_ctor(songbuf + 24, "seventhsense");
            g_string_ctor(songbuf + 48, "oshamascramble");
            g_string_ctor(songbuf + 72, "amazingmightyyyy");
            g_string_ctor(songbuf + 96, "cycles");
            g_string_ctor(songbuf + 120, "maxrage");
            g_string_ctor(songbuf + 144, "infinity");
            g_string_ctor(songbuf + 168, "temptation");
            g_string_ctor(songbuf + 192, "straightintolights");
            g_string_ctor(songbuf + 216, "virtus");
            g_string_ctor(songbuf + 240, "breakbreak");
            g_string_ctor(songbuf + 264, "yomibitoshirazu");
            int diff_data[5] = {0, 1, 2, 3, 4};
            int side_data[4] = {1, 0, 2, 3};

            char songs_vec[24] = {};
            char diff_vec[24] = {};
            char side_vec[24] = {};
            g_BuildStringVector(songs_vec, songbuf, 12);
            g_BuildIntVector(diff_vec, diff_data, 5);
            g_BuildIntVector(side_vec, side_data, 4);

            g_AssignSongList((char *)ab + 88,
                             *(void **)songs_vec,
                             *(void **)(songs_vec + 8));
            g_AssignIntList((char *)ab + 112,
                            *(void **)diff_vec,
                            *(void **)(diff_vec + 8));
            g_AssignIntList((char *)ab + 136,
                            *(void **)side_vec,
                            *(void **)(side_vec + 8));

            g_DestroyStringVector(songs_vec);
            for (int i = 0; i < 12; i++)
                if (*(uint64_t *)(songbuf + i * 24) & 1)
                    g_operator_delete(*reinterpret_cast<void **>(songbuf + i * 24 + 16));

            g_ref_add(ab);
        }
        v5 = 0;
    }
        goto LABEL_5;

    case 66:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_mithra");
        void *ab = g_CharacterAbilityAwardBonusBasedOnCombo(s, 150);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 67:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_toa");
        void *ab = g_CharacterAbilityGaugeFixedValueCondition(s, 1, 0, 0, 0, 60, 1);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 68:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_nami_twilight");
        void *ab = g_CharacterAbilityNamiTwilight(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 69:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_ilith_ivy");
        void *ab = g_CharacterAbilityBonusesBasedOnPeakHealth(s, 0, 5);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 70:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_hikari_vanessa");
        void *ab = g_CharacterAbilityBonusesBasedOnPeakHealth(s, 1, 5);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 71:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 72:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_intruder");
        void *ab = g_CharacterAbilityInsightGauge(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 73:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 75:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_aichan");
        void *ab = g_CharacterAbilityPreferredSongDaily(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 76:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_luna_ilot");
        void *ab = g_CharacterAbilityLunaIlot(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 77:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_eto_hoppe");
        void *ab = g_CharacterAbilityEtoHoppe(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 78:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_nell");
        void *ab = g_CharacterAbilityHideLifebarNell(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 79:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_chinatsu");
        void *ab = g_CharacterAbilityBonusesBasedOnOngeki(s, 0, 7);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 80:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_tsumugi");
        void *ab = g_CharacterAbilityModifyFragmentOnResultOngeki(s, 0, 0, 1);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 81:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_nai");
        void *ab = g_CharacterAbilityLoseFragOnLost(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 82:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_selene");
        void *ab = g_CharacterAbilityComboIntervalBasedOnNoteCount(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 83:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_salt");
        void *ab = g_CharacterAbilityDescriptionOnly(s, 0, 0, 0, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
        {
            *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 56) = 1;
            *reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(ab) + 58) = 1;
            g_ref_add(ab);
        }
        v5 = 0;
    }
        goto LABEL_5;

    case 84:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_acid");
        void *ab = g_CharacterAbilityLoseHealthGainFragAtIntervals(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 85:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_hikari_selene");
        void *ab = g_CharacterAbilityGaugeChunithm(s, 1);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 86:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_hikari_clear");
        void *ab = g_CharacterAbilityDjmaxFever(s, 0, 0);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 87:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_tairitsu_fail");
        void *ab = g_CharacterAbilityDjmaxFever(s, 1, 1);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 88:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_nami_sui");
        void *ab = g_CharacterAbilityDjmaxFever(s, 0, 2);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 89:
        setupAwakenSkill(obj, char_id);
        v5 = 0;
        goto LABEL_5;

    case 90:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_vita_arc");
        void *ab = g_CharacterAbilityLoseGaugeBasedOnNear(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 92:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_hikari_tairitsu_debut");
        void *ab = g_CharacterAbilityNextstageDaily(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 93:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_hp_slow_drain");
        void *ab = g_CharacterAbilitySlowHpDrain(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 94:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_hprate_based_on_hp");
        void *ab = g_CharacterAbilityHpRateBasedOnHp(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 95:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_lost_to_85");
        void *ab = g_CharacterAbilityHighHpLossAtHighHp(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 96:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_frag_doubled_after_earning_X");
        void *ab = g_CharacterAbilityFragDoubledAfterEarningX(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 97:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_saya_konzetsu");
        void *ab = g_CharacterAbilitySayaKonzetsu(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 98:
    {
        char s[24] = {};
        g_string_ctor(s, "skill_insight_konzetsu");
        void *ab = g_CharacterAbilityInsightKonzetsu(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (*(uint64_t *)s & 1)
            g_operator_delete(*reinterpret_cast<void **>(s + 16));
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case 99:
    {
        char s[24] = {};
        g_string_ctor(s, "frags_preferred_song");
        void *ab = g_CharacterAbilityPreferredSong(s);
        *reinterpret_cast<void **>(obj + 208) = ab;
        if (ab)
            g_ref_add(ab);
        v5 = 0;
    }
        goto LABEL_5;

    case -1:
    case 1:
        goto LABEL_5;

    case 3:
    case 6:
        *v6 = 1;
        v5 = 2;
        goto LABEL_5;

    case 31:
    case 32:
    case 38:
    case 51:
    case 52:
    case 60:
    case 74:
        v5 = 1;
        *v6 = 1;
        goto LABEL_5;

    case 91:
        v5 = 0;
        *v6 = 0;
        goto LABEL_5;

    default:
        return 0;
    }

LABEL_5:
    *reinterpret_cast<int *>(obj + 168) = v5;
    *reinterpret_cast<int *>(obj + 192) = hasUncap(char_id) ? 30 : 20;

    if (char_id >= 0 && char_id < 128 && hasUncap(char_id))
    {
        *reinterpret_cast<uint8_t *>(obj + 280) = (uint8_t)g_toggle[char_id];
        *reinterpret_cast<int *>(obj + 168) = (g_toggle[char_id] ? 1 : 0);
        *reinterpret_cast<uint8_t *>(obj + 202) = (uint8_t)(g_toggle[char_id] ? 0 : 1);
    }
    return 1;
}

void installSkillHook()
{
    void *handle = dlopen("libcocos2dcpp.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle)
        return;

    uintptr_t base = getModuleBase("libcocos2dcpp.so");
    if (!base)
        base = reinterpret_cast<uintptr_t>(handle);

    g_file_name_ctor = (PFN_FileNameCtor)(base + OFF_FileNameCtor);
    g_string_ctor = (PFN_StringCtor)(base + OFF_StringCtor);
    g_ref_add = (PFN_RefAdd)(base + OFF_RefAdd);
    g_operator_delete = (PFN_OperatorDelete)dlsym(handle, "_ZdlPv");

    g_CharacterAbility = (PFN_CharacterAbility)(base + OFF_CharacterAbility);
    g_CharacterAbilityModifyFragmentInterpolated = (PFN_CharacterAbilityModifyFragmentInterpolated)(base + OFF_CharacterAbilityModifyFragmentInterpolated);
    g_CharacterAbilityReunion = (PFN_CharacterAbilityReunion)(base + OFF_CharacterAbilityReunion);
    g_CharacterAbilityModifyFragmentOnResult = (PFN_CharacterAbilityModifyFragmentOnResult)(base + OFF_CharacterAbilityModifyFragmentOnResult);
    g_CharacterAbilityModifyFragmentOnFail = (PFN_CharacterAbilityModifyFragmentOnFail)(base + OFF_CharacterAbilityModifyFragmentOnFail);
    g_CharacterAbilityGaugeRateModify = (PFN_CharacterAbilityGaugeRateModify)(base + OFF_CharacterAbilityGaugeRateModify);
    g_CharacterAbilityModifyFragmentRNG = (PFN_CharacterAbilityModifyFragmentRNG)(base + OFF_CharacterAbilityModifyFragmentRNG);
    g_CharacterAbilityGaugeChunithm = (PFN_CharacterAbilityGaugeChunithm)(base + OFF_CharacterAbilityGaugeChunithm);
    g_GradeCheck = (PFN_GradeCheck)(base + OFF_GradeCheck);
    g_CharacterAbilityModifyFragmentOnGrade = (PFN_CharacterAbilityModifyFragmentOnGrade)(base + OFF_CharacterAbilityModifyFragmentOnGrade);
    g_CharacterAbilityGaugeFixedValueCondition = (PFN_CharacterAbilityGaugeFixedValueCondition)(base + OFF_CharacterAbilityGaugeFixedValueCondition);
    g_CharacterAbilitySometimesModifyFragmentMirrored = (PFN_CharacterAbilitySometimesModifyFragmentMirrored)(base + OFF_CharacterAbilitySometimesModifyFragmentMirrored);
    g_CharacterAbilityClearBasedOnScore = (PFN_CharacterAbilityClearBasedOnScore)(base + OFF_CharacterAbilityClearBasedOnScore);
    g_CharacterAbilityTempestGauge = (PFN_CharacterAbilityTempestGauge)(base + OFF_CharacterAbilityTempestGauge);
    g_CharacterAbilityLowComboNoHpGain = (PFN_CharacterAbilityLowComboNoHpGain)(base + OFF_CharacterAbilityLowComboNoHpGain);
    g_CharacterAbilityModifyFragmentOnResultOngeki = (PFN_CharacterAbilityModifyFragmentOnResultOngeki)(base + OFF_CharacterAbilityModifyFragmentOnResultOngeki);
    g_CharacterAbilityHealthLossBasedOnCombo = (PFN_CharacterAbilityHealthLossBasedOnCombo)(base + OFF_CharacterAbilityHealthLossBasedOnCombo);
    g_CharacterAbilityDisableGaugeGainAtSongEnd = (PFN_CharacterAbilityDisableGaugeGainAtSongEnd)(base + OFF_CharacterAbilityDisableGaugeGainAtSongEnd);
    g_CharacterAbilityMaxHealthReducesBasedOnCurrentHealth = (PFN_CharacterAbilityMaxHealthReducesBasedOnCurrentHealth)(base + OFF_CharacterAbilityMaxHealthReducesBasedOnCurrentHealth);
    g_CharacterAbilityGaugeRateBasedOnProgress = (PFN_CharacterAbilityGaugeRateBasedOnProgress)(base + OFF_CharacterAbilityGaugeRateBasedOnProgress);
    g_CharacterAbilitySafe = (PFN_CharacterAbilitySafe)(base + OFF_CharacterAbilitySafe);
    g_CharacterAbilityModifyFragmentNami = (PFN_CharacterAbilityModifyFragmentNami)(base + OFF_CharacterAbilityModifyFragmentNami);
    g_CharacterAbilityModifyFragmentElizabeth = (PFN_CharacterAbilityModifyFragmentElizabeth)(base + OFF_CharacterAbilityModifyFragmentElizabeth);
    g_CharacterAbilityModifyFragmentLily = (PFN_CharacterAbilityModifyFragmentLily)(base + OFF_CharacterAbilityModifyFragmentLily);
    g_CharacterAbilityKanaeMidsummer = (PFN_CharacterAbilityKanaeMidsummer)(base + OFF_CharacterAbilityKanaeMidsummer);
    g_CharacterAbilityDescriptionOnly = (PFN_CharacterAbilityDescriptionOnly)(base + OFF_CharacterAbilityDescriptionOnly);
    g_CharacterAbilityLoseGaugeBasedOnNear = (PFN_CharacterAbilityLoseGaugeBasedOnNear)(base + OFF_CharacterAbilityLoseGaugeBasedOnNear);
    g_CharacterAbilityFatalisGauge = (PFN_CharacterAbilityFatalisGauge)(base + OFF_CharacterAbilityFatalisGauge);
    g_CharacterAbilityAmane = (PFN_CharacterAbilityAmane)(base + OFF_CharacterAbilityAmane);
    g_CharacterAbilityKouWinter = (PFN_CharacterAbilityKouWinter)(base + OFF_CharacterAbilityKouWinter);
    g_CharacterAbilityShamaMilk = (PFN_CharacterAbilityShamaMilk)(base + OFF_CharacterAbilityShamaMilk);
    g_CharacterAbilityAwardBonusBasedOnCombo = (PFN_CharacterAbilityAwardBonusBasedOnCombo)(base + OFF_CharacterAbilityAwardBonusBasedOnCombo);
    g_CharacterAbilityNamiTwilight = (PFN_CharacterAbilityNamiTwilight)(base + OFF_CharacterAbilityNamiTwilight);
    g_CharacterAbilityBonusesBasedOnPeakHealth = (PFN_CharacterAbilityBonusesBasedOnPeakHealth)(base + OFF_CharacterAbilityBonusesBasedOnPeakHealth);
    g_CharacterAbilityViewNoteResults = (PFN_CharacterAbilityViewNoteResults)(base + OFF_CharacterAbilityViewNoteResults);
    g_CharacterAbilityIlithAwakened = (PFN_CharacterAbilityIlithAwakened)(base + OFF_CharacterAbilityIlithAwakened);
    g_CharacterAbilityShirabeUncap = (PFN_CharacterAbilityShirabeUncap)(base + OFF_CharacterAbilityShirabeUncap);
    g_CharacterAbilityClearBasedOnBestGrade = (PFN_CharacterAbilityClearBasedOnBestGrade)(base + OFF_CharacterAbilityClearBasedOnBestGrade);
    g_CharacterAbilityGainStatsFromGauge = (PFN_CharacterAbilityGainStatsFromGauge)(base + OFF_CharacterAbilityGainStatsFromGauge);
    g_CharacterAbilityInsightGauge = (PFN_CharacterAbilityInsightGauge)(base + OFF_CharacterAbilityInsightGauge);
    g_CharacterAbilityModifyFragmentLuin = (PFN_CharacterAbilityModifyFragmentLuin)(base + OFF_CharacterAbilityModifyFragmentLuin);
    g_CharacterAbilityPreferredSongDaily = (PFN_CharacterAbilityPreferredSongDaily)(base + OFF_CharacterAbilityPreferredSongDaily);
    g_CharacterAbilityLunaIlot = (PFN_CharacterAbilityLunaIlot)(base + OFF_CharacterAbilityLunaIlot);
    g_CharacterAbilityEtoHoppe = (PFN_CharacterAbilityEtoHoppe)(base + OFF_CharacterAbilityEtoHoppe);
    g_CharacterAbilityHideLifebarNell = (PFN_CharacterAbilityHideLifebarNell)(base + OFF_CharacterAbilityHideLifebarNell);
    g_CharacterAbilityBonusesBasedOnOngeki = (PFN_CharacterAbilityBonusesBasedOnOngeki)(base + OFF_CharacterAbilityBonusesBasedOnOngeki);
    g_CharacterAbilityLoseFragOnLost = (PFN_CharacterAbilityLoseFragOnLost)(base + OFF_CharacterAbilityLoseFragOnLost);
    g_CharacterAbilityComboIntervalBasedOnNoteCount = (PFN_CharacterAbilityComboIntervalBasedOnNoteCount)(base + OFF_CharacterAbilityComboIntervalBasedOnNoteCount);
    g_CharacterAbilityLoseHealthGainFragAtIntervals = (PFN_CharacterAbilityLoseHealthGainFragAtIntervals)(base + OFF_CharacterAbilityLoseHealthGainFragAtIntervals);
    g_CharacterAbilityDjmaxFever = (PFN_CharacterAbilityDjmaxFever)(base + OFF_CharacterAbilityDjmaxFever);
    g_CharacterAbilityAlterRankOnClearModifyFrags = (PFN_CharacterAbilityAlterRankOnClearModifyFrags)(base + OFF_CharacterAbilityAlterRankOnClearModifyFrags);
    g_CharacterAbilityNextstageDaily = (PFN_CharacterAbilityNextstageDaily)(base + OFF_CharacterAbilityNextstageDaily);
    g_CharacterAbilitySlowHpDrain = (PFN_CharacterAbilitySlowHpDrain)(base + OFF_CharacterAbilitySlowHpDrain);
    g_CharacterAbilityHpRateBasedOnHp = (PFN_CharacterAbilityHpRateBasedOnHp)(base + OFF_CharacterAbilityHpRateBasedOnHp);
    g_CharacterAbilityHighHpLossAtHighHp = (PFN_CharacterAbilityHighHpLossAtHighHp)(base + OFF_CharacterAbilityHighHpLossAtHighHp);
    g_CharacterAbilityFragDoubledAfterEarningX = (PFN_CharacterAbilityFragDoubledAfterEarningX)(base + OFF_CharacterAbilityFragDoubledAfterEarningX);
    g_CharacterAbilitySayaKonzetsu = (PFN_CharacterAbilitySayaKonzetsu)(base + OFF_CharacterAbilitySayaKonzetsu);
    g_CharacterAbilityInsightKonzetsu = (PFN_CharacterAbilityInsightKonzetsu)(base + OFF_CharacterAbilityInsightKonzetsu);
    g_CharacterAbilityPreferredSong = (PFN_CharacterAbilityPreferredSong)(base + OFF_CharacterAbilityPreferredSong);

    g_BuildStringVector = (PFN_BuildStringVector)(base + OFF_BuildStringVector);
    g_BuildIntVector = (PFN_BuildIntVector)(base + OFF_BuildIntVector);
    g_AssignSongList = (PFN_AssignSongList)(base + OFF_AssignSongList);
    g_AssignIntList = (PFN_AssignIntList)(base + OFF_AssignIntList);
    g_DestroyStringVector = (PFN_DestroyStringVector)(base + OFF_DestroyStringVector);
    g_RenderCharUI = (PFN_RenderCharUI)(base + OFF_RenderCharUI);
    if (!g_operator_delete)
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
