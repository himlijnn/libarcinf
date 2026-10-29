#pragma once

#define TARGET "libcocos2dcpp.so"
#define ASSETS_DIR "/sdcard/arcinf/assets/"

/* compile config */

#define enableAutoplayHook true
#define enableSecurityHook (enableAutoplayHook && true)

/* offset with 7.1.0c */

#define OFF_PATH_TRAMP 0x10CD508

#define OFF_Init 0x19FAE38
#define OFF_OnlineSkill 0xC6B6DC
#define OFF_FileNameCtor 0xC7D734
#define OFF_StringCtor 0x104BAAC
#define OFF_RefAdd 0xEA0FE8

#define OFF_SetCharBorder 0xF43A9C
#define OFF_SetBarFill 0xD3F0B8

#define OFF_SetLabelText 0xC0518C
#define OFF_Localize 0xB866D0

#define OFF_SkillMayaText 0x52A25A
#define OFF_SealCtx 0x1BF0B58

#define OFF_RenderCharUI 0x147FCC8
#define OFF_AwakenCheck 0x1412140
#define OFF_RefreshCharState 0xE413B0
#define OFF_ModifyCharState 0x11225F4

#define OFF_AWAKEN_CALL 0x1122658
#define OFF_AWAKEN_CALLER 0xA09C94

#define OFF_UserDefaultSetString 0x999654
#define OFF_UserDefaultGetString 0xEC1D24
#define OFF_UserDefaultSetBool 0xB867F0
#define OFF_UserDefaultGetBool 0xFE2218

#define OFF_SET_AVAIL_BYTE_TBZ 0xCE8BBC
#define OFF_SET_AVAIL_VIRT_TBZ 0xCE8BC8
#define OFF_SET_GET_CB 0x130CF3C
#define OFF_SET_STAMINA_TAP 0x1A44678
#define OFF_SET_INVITE_PUSH 0xCE9808

#define OFF_CREDITS_LINE 0xB1D684

#define OFF_CREATE_UI_NODE 0x12DD104

#define OFF_SEC_NOTSAVED_SET 0x12D1800

#define OFF_AP_SCAN 0xA45D68
#define OFF_AP_SKIP_NOTE 0xA45DF8
#define OFF_AP_MISS_NOTE 0xA4636C
#define OFF_AP_SKIP_ARC_CHILD 0xA46290
#define OFF_AP_MISS_ARC_CHILD 0xA4631C
#define OFF_AP_HIT_FN 0x12EB93C
#define OFF_AP_MISS_HANDLER 0xF4914C
#define OFF_AP_WINDOW_NOTE 0xA46358
#define OFF_AP_WINDOW_ARCTAP 0xA46308
#define OFF_AP_WINDOW_TAP 0xA46250

#define OFF_AP_VT_ARC 0x1A95018
#define OFF_AP_VT_HOLD 0x1A9C498

/* char info */

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
    {100, false, "skill_ilith_halloween", ""},
    {101, false, "skill_stream_palette", ""},
};

static const int g_char_num = (int)(sizeof(g_char_info) / sizeof(g_char_info[0]));
