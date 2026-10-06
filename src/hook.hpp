#pragma once

void installSkillHook();
void installVideoHook();
void installAutoplayHook();
void installSettingsHook();
void installSecurityHook();

int settingsAutoMode(void);

void settingsLoad(void);
void autoplayApply(int on);
void securityApply(int on);
