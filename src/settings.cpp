
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <sys/mman.h>

#include "game.hpp"
#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

__attribute__((naked, used)) static void asmNop()
{
    __asm__ __volatile__("nop");
}

static const char *SET_KEY_AUTOMODE = "autoMode";

static uintptr_t g_base = 0;
static int g_auto = 0;
static int g_auto_loaded = 0;

static void applyAll(int v)
{
    autoplayApply(v);
    securityApply(v);
}

static void setAutoMode(int v, int persist)
{
    v = v ? 1 : 0;
    g_auto = v;
    if (persist && g_auto_loaded)
        gameSetBool(g_base, SET_KEY_AUTOMODE, v);
    applyAll(v);
}

int settingsAutoMode(void)
{
    return g_auto;
}

void settingsLoad(void)
{
    if (g_auto_loaded)
        return;
    g_auto_loaded = 1;
    g_auto = gameGetBool(g_base, SET_KEY_AUTOMODE, 0) ? 1 : 0;
    applyAll(g_auto);
}

int onStaminaGet(void *callable)
{
    (void)callable;
    settingsLoad();
    return g_auto ? 1 : 0;
}

int onStaminaTap(void *callable)
{
    (void)callable;
    setAutoMode(g_auto ? 0 : 1, 1);
    return g_auto;
}

static const uint32_t g_set_tbz_sites[] = {OFF_SET_AVAIL_BYTE_TBZ, OFF_SET_AVAIL_VIRT_TBZ};
static const int g_set_site_count =
    static_cast<int>(sizeof(g_set_tbz_sites) / sizeof(g_set_tbz_sites[0]));

static void patchWord(uintptr_t addr, uint32_t w)
{
    const long ps = trampPageSize();
    const uintptr_t page = addr & ~(static_cast<uintptr_t>(ps) - 1);
    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(reinterpret_cast<void *>(addr), &w, sizeof(w));
    __builtin___clear_cache(reinterpret_cast<char *>(page), reinterpret_cast<char *>(page + ps));
    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_EXEC);
}

static void disableOfflineBranchForOurRow()
{
    uint32_t nop;
    memcpy(&nop, reinterpret_cast<const void *>(&asmNop), sizeof(nop));
    for (int i = 0; i < g_set_site_count; i++)
        patchWord(g_base + g_set_tbz_sites[i], nop);
}

static void patchBranch(uintptr_t from, uintptr_t to)
{
    patchWord(from, makeBranchWord(from, to));
}

static void hideInviteRow()
{
    patchBranch(g_base + OFF_SET_INVITE_PUSH, g_base + OFF_SET_INVITE_PUSH + 12);
}

void installSettingsHook()
{
    (void)dlopen(TARGET, RTLD_NOW | RTLD_GLOBAL);
    const uintptr_t base = getModuleBase(TARGET);
    g_base = base;

    disableOfflineBranchForOurRow();
    (void)replaceFunction(base + OFF_SET_GET_CB, (void *)&onStaminaGet);
    (void)replaceFunction(base + OFF_SET_STAMINA_TAP, (void *)&onStaminaTap);
    hideInviteRow();
}
