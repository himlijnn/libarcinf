#pragma once
#include <cstdint>

extern uintptr_t g_base;

void installSkillHook();
void installVideoHook();
void installSettingsHook();
void installAutoplayHook();
void installSecurityHook();
void installPotentialHook();

// 存档读写由 settings.cpp 实现，其它模块只读内存

namespace getPrefs
{
    void Awakened(void);
    void AutoPlay(void);
    void ShowPotential(void);
}

namespace setPrefs
{
    void Awakened(void);
    void AutoPlay(void);
    void ShowPotential(void);
}

int settingsIsAutoPlay(void);
int settingsIsShowPotential(void);
int settingsIsAwakened(int id);
void settingsSetAwaken(int id, int on);

void autoplayApply(int on);
void securityApply(int on);
