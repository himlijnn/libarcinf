#pragma once

void installSkillHook();
void installVideoHook();
void installAutoplayHook();
void installSettingsHook();
void installSecurityHook();

namespace getPrefs
{
    void Awakened(void);
    void AutoPlay(void);
}

namespace setPrefs
{
    void Awakened(void);
    void AutoPlay(void);
}

int settingsIsAutoPlay(void);
int settingsIsAwakened(int id);
void settingsSetAwaken(int id, int on);

void autoplayApply(int on);
void securityApply(int on);
