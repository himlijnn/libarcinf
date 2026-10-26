
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
#if enableAutoplayHook
    autoplayApply(v);
#endif
#if enableSecurityHook
    securityApply(v);
#else
    (void)v;
#endif
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

#if enableAutoplayHook
        g_autoPlay = gameGetBool(g_base, PREF_KEY_AUTOPLAY, 0) ? 1 : 0;
        applyAll(g_autoPlay);
#endif
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

#if enableAutoplayHook
static const uint32_t g_set_tbz_sites[] = {OFF_SET_AVAIL_BYTE_TBZ, OFF_SET_AVAIL_VIRT_TBZ};
static const int g_set_site_count =
    static_cast<int>(sizeof(g_set_tbz_sites) / sizeof(g_set_tbz_sites[0]));
#endif

static void patchWord(uintptr_t addr, uint32_t w)
{
    const long ps = trampPageSize();
    const uintptr_t page = addr & ~(static_cast<uintptr_t>(ps) - 1);
    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(reinterpret_cast<void *>(addr), &w, sizeof(w));
    __builtin___clear_cache(reinterpret_cast<char *>(page), reinterpret_cast<char *>(page + ps));
    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_EXEC);
}

#if enableAutoplayHook
static void disableOfflineBranchForOurRow()
{
    for (int i = 0; i < g_set_site_count; i++)
        patchWord(g_base + g_set_tbz_sites[i], 0xD503201F);
}
#endif

static void patchBranch(uintptr_t from, uintptr_t to)
{
    patchWord(from, makeBranchWord(from, to));
}

static void hideInviteRow()
{
    patchBranch(g_base + OFF_SET_INVITE_PUSH, g_base + OFF_SET_INVITE_PUSH + 12);
}

typedef void (*PFN_CreditLine)(void *, void *, int, int, int, float);

static Trampoline g_credits_line;

static bool isCreditStr(const void *str, const char *want)
{
    return strcmp(gameStrText(static_cast<const GameStr *>(str)), want) == 0;
}

struct CreditLine
{
    const char *text;
    int head;
    int font;
    float scale;
};

static const CreditLine kParticleArts[] = {
    {"\"Particle Arts\"", 0, 30, 0.5f},
    {"Performed by Virtual Self", 1, 24, 0.5f},
    {"Written by Porter Robinson", 1, 24, 0.5f},
    {"Courtesy of Virtual Self and mtheory", 1, 24, 0.5f},
    {"By arrangement with Hidden Track Music", 1, 24, 0.5f},
    {" ", 1, 30, 1.0f},
};

static void addCreditLine(void *ctx, const CreditLine &line)
{
    GameStr s = {};
    gameStrInit(g_base, &s, line.text);
    ((PFN_CreditLine)g_credits_line.cave)(ctx, &s, line.head, 0, line.font, line.scale);
    gameStrFree(&s);
}

static void onCreditsLine(void *ctx, void *str, int head, int a4, int font, float scale)
{
    if (head == 0 && a4 == 0 && font == 30 && isCreditStr(str, "\"Scarlet Cage\""))
        for (const CreditLine &line : kParticleArts)
            addCreditLine(ctx, line);

    ((PFN_CreditLine)g_credits_line.cave)(ctx, str, head, a4, font, scale);
}

static void creditsHook(uintptr_t base)
{
    if (!base)
        return;
    g_credits_line = installTrampoline(base + OFF_CREDITS_LINE, (void *)&onCreditsLine);
}


#define VT_GET_CHILD_BY_NAME 528
#define VT_SET_ENABLED 1368
#define VT_HIDE 336

typedef void *(*PFN_CreateUiNode)(void *);
typedef void *(*PFN_GetChildByName)(void *, void *);
typedef void (*PFN_SetBool)(void *, int);

static Trampoline g_login_node;


struct LoginHide
{
    const char *name;
    bool disable;
};

static const LoginHide kLoginHide[] = {
    {"login_button", true},
    {"register_button", true},
    {"login_hint", false},
    {"login_hint_warning", false},
};

static void hideLoginParts(void *root)
{
    const uintptr_t vt = *static_cast<uintptr_t *>(root);

    for (const LoginHide &target : kLoginHide)
    {
        GameStr key = {};
        gameStrInit(g_base, &key, target.name);
        void *child = reinterpret_cast<PFN_GetChildByName>(
            *reinterpret_cast<uintptr_t *>(vt + VT_GET_CHILD_BY_NAME))(root, &key);
        gameStrFree(&key);

        if (!child)
            continue;

    
        const uintptr_t cvt = *static_cast<uintptr_t *>(child);
        if (target.disable)
            reinterpret_cast<PFN_SetBool>(*reinterpret_cast<uintptr_t *>(cvt + VT_SET_ENABLED))(child, 0);
        reinterpret_cast<PFN_SetBool>(*reinterpret_cast<uintptr_t *>(cvt + VT_HIDE))(child, 0);
    }
}

static void *onCreateUiNode(void *str)
{
    void *node = ((PFN_CreateUiNode)g_login_node.cave)(str);

    if (node && str && strstr(gameStrText(static_cast<const GameStr *>(str)), "LoggedOutNetwork"))
        hideLoginParts(node);

    return node;
}

static void loginHook(uintptr_t base)
{
    if (!base)
        return;
    g_login_node = installTrampoline(base + OFF_CREATE_UI_NODE, (void *)&onCreateUiNode);
}

void installSettingsHook()
{
    (void)dlopen(TARGET, RTLD_NOW | RTLD_GLOBAL);
    const uintptr_t base = getModuleBase(TARGET);
    g_base = base;

#if enableAutoplayHook
    disableOfflineBranchForOurRow();
    (void)replaceFunction(base + OFF_SET_GET_CB, (void *)&onStaminaGet);
    (void)replaceFunction(base + OFF_SET_STAMINA_TAP, (void *)&onStaminaTap);
#endif
    hideInviteRow();
    creditsHook(base);
    loginHook(base);
}
