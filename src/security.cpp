#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <sys/mman.h>

#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

__attribute__((naked, used)) static void asmMovW9_1()
{
    __asm__ __volatile__("mov w9, #1");
}

static uintptr_t g_sec_base = 0;
static uint32_t g_sec_orig = 0;
static uint32_t g_sec_force = 0;
static int g_sec_applied = 0;

static uint32_t secRead(uintptr_t addr)
{
    uint32_t w = 0;
    memcpy(&w, reinterpret_cast<const void *>(addr), sizeof(w));
    return w;
}

static void secWrite(uintptr_t addr, uint32_t w)
{
    const long ps = trampPageSize();
    const uintptr_t page = addr & ~(static_cast<uintptr_t>(ps) - 1);
    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(reinterpret_cast<void *>(addr), &w, sizeof(w));
    __builtin___clear_cache(reinterpret_cast<char *>(page), reinterpret_cast<char *>(page + ps));
    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_EXEC);
}

void securityApply(int on)
{
    const int want = on ? 1 : 0;
    if (want == g_sec_applied)
        return;
    secWrite(g_sec_base + OFF_SEC_NOTSAVED_SET, want ? g_sec_force : g_sec_orig);
    g_sec_applied = want;
}

void installSecurityHook()
{
    (void)dlopen(TARGET, RTLD_NOW | RTLD_GLOBAL);
    const uintptr_t base = getModuleBase(TARGET);
    g_sec_base = base;
    g_sec_orig = secRead(base + OFF_SEC_NOTSAVED_SET);
    memcpy(&g_sec_force, reinterpret_cast<const void *>(&asmMovW9_1), sizeof(g_sec_force));
    securityApply(settingsAutoMode());
}
