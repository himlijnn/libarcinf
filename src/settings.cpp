
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <sys/mman.h>

#include "game.hpp"
#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

static uintptr_t g_base = 0;

static const char *PREF_KEY_AUTOPLAY = "autoPlay";
static const char *PREF_KEY_AWAKENED = "awakened";

static int g_autoPlay = 0;
static uint8_t g_awaken[g_char_num] = {0};
static bool g_autoPlay_loaded = false;
static bool g_awaken_loaded = false;

static void applyAll(int v)
{
    autoplayApply(v);
    securityApply(v);
}

int settingsIsAutoPlay(void)
{
    return g_autoPlay;
}

int settingsIsAwakened(int id)
{
    return (id >= 0 && id < g_char_num) ? g_awaken[id] : 0;
}

void settingsSetAwaken(int id, int on)
{
    if (id < 0 || id >= g_char_num)
        return;
    g_awaken[id] = on ? 1 : 0;
    setPrefs::Awakened();
}

namespace getPrefs
{
    void Awakened()
    {
        if (g_awaken_loaded)
            return;
        g_awaken_loaded = true;

        char buf[512];
        gameGetString(g_base, PREF_KEY_AWAKENED, "", buf, sizeof(buf));
        memset(g_awaken, 0, sizeof(g_awaken));
        int id = -1;
        for (const char *p = buf;; p++)
        {
            const char ch = *p;
            if (ch >= '0' && ch <= '9')
            {
                if (id < g_char_num)
                    id = (id < 0 ? 0 : id) * 10 + (ch - '0');
                continue;
            }
            if (id >= 0 && id < g_char_num)
                g_awaken[id] = 1;
            id = -1;
            if (!ch)
                break;
        }
    }

    void AutoPlay()
    {
        if (g_autoPlay_loaded)
            return;
        g_autoPlay_loaded = true;

        g_autoPlay = gameGetBool(g_base, PREF_KEY_AUTOPLAY, 0) ? 1 : 0;
        applyAll(g_autoPlay);
    }
}

namespace setPrefs
{
    void Awakened()
    {
        char val[512];
        val[0] = 0;
        size_t len = 0;
        for (int i = 0; i < g_char_num; i++)
        {
            if (!g_awaken[i])
                continue;
            const int m = snprintf(val + len, sizeof(val) - len, "%s%d", len ? "," : "", i);
            if (m <= 0 || (size_t)m >= sizeof(val) - len)
                break;
            len += (size_t)m;
        }
        gameSetString(g_base, PREF_KEY_AWAKENED, val);
    }

    void AutoPlay()
    {
        gameSetBool(g_base, PREF_KEY_AUTOPLAY, g_autoPlay);
    }
}

int onStaminaGet(void *callable)
{
    (void)callable;
    getPrefs::AutoPlay();
    return g_autoPlay;
}

int onStaminaTap(void *callable)
{
    (void)callable;
    g_autoPlay = g_autoPlay ? 0 : 1;
    applyAll(g_autoPlay);
    setPrefs::AutoPlay();
    return g_autoPlay;
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
    for (int i = 0; i < g_set_site_count; i++)
        patchWord(g_base + g_set_tbz_sites[i], 0xD503201F);
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
