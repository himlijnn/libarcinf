#pragma once

// 7.0.255c

#define OFF_PATH_TRAMP 0x1505498

#define OFF_Init 0x152F6A4
#define OFF_FileNameCtor 0x10D8FE0
#define OFF_StringCtor 0x19AA138
#define OFF_RefAdd 0x1A089B0

#define OFF_CharacterAbility 0xEC32D4                           // gauge_easy
#define OFF_CharacterAbilityModifyFragmentInterpolated 0x9A83D4 // frags_kou
#define OFF_CharacterAbilityReunion 0x19757B8                   // skill_reunion
#define OFF_CharacterAbilityModifyFragmentOnResult 0x1267140    // gauge_easy|frag_plus_15_pst&prs
#define OFF_CharacterAbilityModifyFragmentOnFail 0x17674D8      // gauge_hard|fail_frag_minus_100
#define OFF_CharacterAbilityGaugeRateModify 0xD43D10            // gauge_saya
#define OFF_CharacterAbilityModifyFragmentRNG 0xA637F8          // frag_rng_ayu
#define OFF_CharacterAbilityGaugeChunithm 0xC9C2EC              // gauge_chuni
#define OFF_GradeCheck 0x96EC04
#define OFF_CharacterAbilityModifyFragmentOnGrade 0xA10F00                 // frags_nono
#define OFF_CharacterAbilityGaugeFixedValueCondition 0x18D64A8             // gauge_pandora
#define OFF_CharacterAbilitySometimesModifyFragmentMirrored 0x15C148C      // sometimes(note_mirror|frag_plus_5)
#define OFF_CharacterAbilityClearBasedOnScore 0x110ACD8                    // scoreclear_aa|visual_scoregauge
#define OFF_CharacterAbilityTempestGauge 0x149C324                         // gauge_tempest
#define OFF_CharacterAbilityLowComboNoHpGain 0x982F6C                      // gauge_ilith_summer
#define OFF_CharacterAbilityModifyFragmentOnResultOngeki 0xBC2B28          // frags_ongeki
#define OFF_CharacterAbilityHealthLossBasedOnCombo 0x1014D88               // gauge_areus
#define OFF_CharacterAbilityDisableGaugeGainAtSongEnd 0x1816AC8            // gauge_seele
#define OFF_CharacterAbilityMaxHealthReducesBasedOnCurrentHealth 0x175EE20 // gauge_isabelle
#define OFF_CharacterAbilityGaugeRateBasedOnProgress 0x7A5868              // gauge_exhaustion
#define OFF_CharacterAbilitySafe 0xF86F80                                  // gauge_safe_10
#define OFF_CharacterAbilityModifyFragmentNami 0x16E75FC                   // frags_nami
#define OFF_CharacterAbilityModifyFragmentElizabeth 0x17D7EC0              // skill_elizabeth
#define OFF_CharacterAbilityModifyFragmentLily 0xC53160                    // skill_lily
#define OFF_CharacterAbilityKanaeMidsummer 0x129AA64                       // skill_kanae_midsummer
#define OFF_CharacterAbilityDescriptionOnly 0x16CB20C                      // skill_vita
#define OFF_CharacterAbilityFatalisGauge 0xAC6A6C                          // skill_fatalis
#define OFF_CharacterAbilityAmane 0x1067C30                                // skill_amane
#define OFF_CharacterAbilityKouWinter 0x10A9744                            // skill_kou_winter
#define OFF_CharacterAbilityShamaMilk 0x11B90CC                            // skill_shama
#define OFF_CharacterAbilityAwardBonusBasedOnCombo 0xDE8650                // skill_mithra
#define OFF_CharacterAbilityNamiTwilight 0x192D664                         // skill_nami_twilight
#define OFF_CharacterAbilityBonusesBasedOnPeakHealth 0x919C3C              // skill_ilith_ivy
#define OFF_CharacterAbilityInsightGauge 0x1097AA8                         // skill_intruder
#define OFF_CharacterAbilityModifyFragmentLuin 0xD2180C                    // skill_luin
#define OFF_CharacterAbilityPreferredSongDaily 0xF47058                    // skill_aichan
#define OFF_CharacterAbilityLunaIlot 0xB7DFBC                              // skill_luna_ilot
#define OFF_CharacterAbilityEtoHoppe 0x7EFF00                              // skill_eto_hoppe
#define OFF_CharacterAbilityHideLifebarNell 0xC2348C                       // skill_nell
#define OFF_CharacterAbilityBonusesBasedOnOngeki 0x10CBEC8                 // skill_chinatsu
#define OFF_CharacterAbilityLoseFragOnLost 0x139F18C                       // skill_nai
#define OFF_CharacterAbilityComboIntervalBasedOnNoteCount 0xE15C7C         // skill_selene
#define OFF_CharacterAbilityLoseHealthGainFragAtIntervals 0x17DAB30        // skill_acid
#define OFF_CharacterAbilityDjmaxFever 0x836394                            // skill_hikari_clear
#define OFF_CharacterAbilityAlterRankOnClearModifyFrags 0x17287F4          // skill_nonoka
#define OFF_CharacterAbilityLoseGaugeBasedOnNear 0xFE7E34                  // skill_vita_arc
#define OFF_CharacterAbilityNextstageDaily 0x137229C                       // skill_hikari_tairitsu_debut
#define OFF_CharacterAbilitySlowHpDrain 0x1642AD0                          // skill_hp_slow_drain
#define OFF_CharacterAbilityHpRateBasedOnHp 0x1109AD4                      // skill_hprate_based_on_hp
#define OFF_CharacterAbilityHighHpLossAtHighHp 0x18E60D8                   // skill_lost_to_85
#define OFF_CharacterAbilityFragDoubledAfterEarningX 0x14F0080             // skill_frag_doubled_after_earning_X
#define OFF_CharacterAbilitySayaKonzetsu 0x94CA00                          // skill_saya_konzetsu
#define OFF_CharacterAbilityInsightKonzetsu 0x7AF0D0                       // skill_insight_konzetsu
#define OFF_CharacterAbilityPreferredSong 0x10F9D24                        // frags_preferred_song
#define OFF_CharacterAbilityViewNoteResults 0x119FF48                      // skill_saya_uncap
#define OFF_CharacterAbilityIlithAwakened 0xD88CFC                         // ilith_awakened_skill
#define OFF_CharacterAbilityShirabeUncap 0x149EC0C                         // shirabe_entry_fee
#define OFF_CharacterAbilityClearBasedOnBestGrade 0x145AE50                // skill_doroc_uncap
#define OFF_CharacterAbilityGainStatsFromGauge 0x944BA4                    // skill_maya_uncap

#define OFF_BuildStringVector 0x1151694
#define OFF_BuildIntVector 0x9A593C
#define OFF_AssignSongList 0x13F208C
#define OFF_AssignIntList 0x903BA4
#define OFF_DestroyStringVector 0xAD106C

#define OFF_UserDefaultSetString 0x10FC528
#define OFF_UserDefaultGetString 0x182C54C

#define OFF_SetCharBorder 0xBC0A7C
#define OFF_SetBarFill 0x161F770

#define OFF_SetLabelText 0xACF350
#define OFF_Localize 0x101A7E4
#define OFF_SkillMayaText 0x5272C8
#define OFF_SealCtx 0x1BE7028

#define OFF_RenderCharUI 0x1967CF8
#define OFF_AwakenCheck 0x113F088
#define OFF_RefreshCharState 0x10C8BAC
#define OFF_ModifyCharState 0x127E1CC

#define OFF_AWAKEN_CALL 0x127E230
#define OFF_AWAKEN_CALLER 0xC2406C
