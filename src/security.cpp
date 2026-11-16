// 本局不保存：Auto 开启时，把结算写入点改成 not-saved。
// 补丁是单条立即数指令，按开关在原指令与替换指令间切换。

#include <cstdint>

#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

#if enableSecurityHook

static uint32_t g_originalInstruction = 0;
static bool g_patchApplied = false;

// MOV W9, #1
static constexpr uint32_t kForceNotSavedInstruction = 0x52800029;

void securityApply(int on)
{
    const bool enable = on != 0;
    if (enable == g_patchApplied)
        return;

    patchWord(g_base + OFF_SEC_NOTSAVED_SET,
              enable ? kForceNotSavedInstruction : g_originalInstruction);
    g_patchApplied = enable;
}

void installSecurityHook()
{
    g_originalInstruction = readWord(g_base + OFF_SEC_NOTSAVED_SET);
    securityApply(settingsIsAutoPlay());
}

#else

void installSecurityHook()
{
}

void securityApply(int on)
{
    (void)on;
}

#endif
