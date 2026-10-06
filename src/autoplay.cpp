
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <sys/mman.h>

#include "hook.hpp"
#include "trampoline.hpp"
#include "offset.hpp"

__attribute__((naked, used)) static void asmAddW8W10_0()
{
    __asm__ __volatile__("add w8, w10, #0");
}

#define AP_ENTRY_HOOK 1

#define AP_STUB_SITES ((1 << AP_SITE_MISS_NOTE) | (1 << AP_SITE_MISS_ARC_CHILD))

struct ApSite
{
    uint32_t at;
    uint32_t skip;
    const void *newAsm;
    uint32_t selfMask;
};

static const uint32_t AP_WINDOW_IMM = 0xFFFu << 10;

#define AP_SITE_MISS_NOTE 0
#define AP_SITE_MISS_ARC_CHILD 1
#define AP_SITE_COUNT 5

static const ApSite g_ap_sites[AP_SITE_COUNT] = {
    {OFF_AP_MISS_NOTE, OFF_AP_SKIP_NOTE, nullptr, 0},
    {OFF_AP_MISS_ARC_CHILD, OFF_AP_SKIP_ARC_CHILD, nullptr, 0},
    {OFF_AP_WINDOW_TAP, 0, reinterpret_cast<const void *>(&asmAddW8W10_0), AP_WINDOW_IMM},
    {OFF_AP_WINDOW_NOTE, 0, reinterpret_cast<const void *>(&asmAddW8W10_0), AP_WINDOW_IMM},
    {OFF_AP_WINDOW_ARCTAP, 0, reinterpret_cast<const void *>(&asmAddW8W10_0), AP_WINDOW_IMM},
};

static uintptr_t g_ap_base = 0;
static uint32_t g_ap_orig[AP_SITE_COUNT] = {0};
static int8_t g_ap_patched[AP_SITE_COUNT] = {0};
static int g_started_cnt = 0;
static uint32_t g_ap_stub_save[AP_SITE_COUNT][4] = {{0}};

static Trampoline g_ap_scan;

#if AP_STUB_SITES
static void *apStubFor(int i, uint64_t **backSlot);
#endif

static uint32_t readWord(uintptr_t addr)
{
    uint32_t w = 0;
    memcpy(&w, reinterpret_cast<const void *>(addr), sizeof(w));
    return w;
}
static int rdword(uintptr_t addr)
{
    int v = 0;
    memcpy(&v, reinterpret_cast<const void *>(addr), sizeof(v));
    return v;
}
static uintptr_t rdptr(uintptr_t addr)
{
    uintptr_t v = 0;
    memcpy(&v, reinterpret_cast<const void *>(addr), sizeof(v));
    return v;
}

static void putWord(uintptr_t addr, uint32_t w)
{
    const long ps = trampPageSize();
    const uintptr_t page = addr & ~(static_cast<uintptr_t>(ps) - 1);
    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(reinterpret_cast<void *>(addr), &w, sizeof(w));
    __builtin___clear_cache(reinterpret_cast<char *>(page), reinterpret_cast<char *>(page + ps));
    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_EXEC);
}

static void applySite(int i, int on)
{
    const ApSite &s = g_ap_sites[i];
    const uintptr_t site = g_ap_base + s.at;

#if AP_STUB_SITES
    uint64_t *stubBack = nullptr;
    if (void *stubFn = apStubFor(i, &stubBack))
    {

        if (g_ap_patched[i] == (on ? 1 : 0))
            return;
        if (on)
        {
            memcpy(g_ap_stub_save[i], reinterpret_cast<const void *>(site), 16);
            *stubBack = site + 0x18;
            patchTrampEntry(site, stubFn);
            g_ap_patched[i] = 1;
        }
        else
        {
            for (int k = 0; k < 4; k++)
                putWord(site + 4 * k, g_ap_stub_save[i][k]);
            g_ap_patched[i] = 0;
        }
        return;
    }
#endif
    uint32_t w = g_ap_orig[i];
    if (on)
    {
        if (s.skip)
            w = makeBranchWord(site, g_ap_base + s.skip);
        else
        {
            uint32_t newWord;
            memcpy(&newWord, s.newAsm, sizeof(newWord));
            w = (g_ap_orig[i] & ~s.selfMask) | (newWord & s.selfMask);
        }
    }
    putWord(site, w);
    g_ap_patched[i] = on ? 1 : 0;
}

void autoplayApply(int on)
{
    if (!on)
        g_started_cnt = 0;
    for (int i = 0; i < AP_SITE_COUNT; i++)
    {
        if (g_ap_patched[i] == (on ? 1 : 0))
            continue;
        applySite(i, on);
    }
}

#define AP_STARTED_MAX 256
static uintptr_t g_started[AP_STARTED_MAX] = {0};

static bool isStarted(uintptr_t n)
{
    const int lim = g_started_cnt < AP_STARTED_MAX ? g_started_cnt : AP_STARTED_MAX;
    for (int i = 0; i < lim; i++)
        if (g_started[i] == n)
            return true;
    return false;
}

static bool markStartedOnce(uintptr_t n)
{
    if (isStarted(n))
        return false;
    g_started[g_started_cnt % AP_STARTED_MAX] = n;
    g_started_cnt++;
    return true;
}

static int32_t nowMs(uintptr_t ctx)
{
    const uintptr_t cfg = rdptr(ctx + 0x30);
    const int32_t cfg28 = rdword(cfg + 0x28);
    if ((rdword(cfg + 0x2d) & 0xff) != 0)
        return rdword(cfg + 0x20) - cfg28;
    const int32_t c34 = rdword(cfg + 0x34);
    return c34 - cfg28 + (c34 > 0 ? 0 : -3000);
}

static void tickLongNotes(uintptr_t ctx)
{
    const uintptr_t begin = rdptr(ctx + 0xa0), end = rdptr(ctx + 0xa8);
    const int32_t now = nowMs(ctx);
    for (uintptr_t slot = begin; slot < end; slot += 8)
    {
        const uintptr_t n = rdptr(slot);
        const uintptr_t vt = rdptr(n);
        const bool isArc = (vt == g_ap_base + OFF_AP_VT_ARC);
        const bool isHold = (vt == g_ap_base + OFF_AP_VT_HOLD);
        if (!isArc && !isHold)
            continue;

        const int32_t t18 = rdword(n + 0x18), t1c = rdword(n + 0x1c);
        const unsigned a54 = *reinterpret_cast<unsigned char *>(n + 0x54);
        const unsigned a64 = *reinterpret_cast<unsigned char *>(n + 0x64);
        const int32_t aA4 = rdword(n + 0xa4);
        const uintptr_t vis = rdptr(n + 0xe0);

        if (a54 == 0)
            continue;

        const uintptr_t sb = rdptr(n + 0x78), se = rdptr(n + 0x80);
        const int64_t segBytes = static_cast<int64_t>(se) - static_cast<int64_t>(sb);
        if (segBytes < 0xc || (segBytes % 0xc) != 0)
            continue;

        if (now < t18 || now > t1c + 100)
            continue;

        const uintptr_t fn = rdptr(vt + 0x60);

        if (isArc)
        {
            if (aA4 != 0)
                continue;
            if (t18 == t1c)
            {

                *reinterpret_cast<uint32_t *>(n + 0xa4) = 1;
                continue;
            }

            if (a64 == 0 && markStartedOnce(n) && fn)
            {
                unsigned char hc[0x40];
                memset(hc, 0, sizeof(hc));
                *reinterpret_cast<int32_t *>(hc + 0x34) = -1;
                reinterpret_cast<void (*)(uintptr_t, void *, int32_t)>(fn)(n, hc, now);
            }
            if (vis)
            {
                *reinterpret_cast<uint16_t *>(vis + 0x10) = 0x0101;
                *reinterpret_cast<unsigned char *>(vis + 0x12) = 1;
                *reinterpret_cast<float *>(vis + 0x14) = static_cast<float>(now + 500);
            }
        }
        else
        {

            if (now < t18 + 16)
                continue;

            if (a64 == 0 && markStartedOnce(n) && fn)
                reinterpret_cast<void (*)(uintptr_t, uintptr_t, int32_t)>(fn)(n, 0, now);
            *reinterpret_cast<uint32_t *>(n + 0x30) = 0;
            *reinterpret_cast<unsigned char *>(n + 0xa8) = 1;
        }

        *reinterpret_cast<uint16_t *>(n + 0x64) = 0x0101;
    }
}

#if AP_ENTRY_HOOK
int64_t onNoteScan(int64_t a1, int64_t a2)
{
    static bool booted = false;
    if (!booted)
    {
        booted = true;
        settingsLoad();
    }

    if (settingsAutoMode())
        tickLongNotes(static_cast<uintptr_t>(a1));

    typedef int64_t (*PFN)(int64_t, int64_t);
    return ((PFN)g_ap_scan.cave)(a1, a2);
}
#endif

#if AP_STUB_SITES
uint64_t g_ap_hit_fn = 0;
uint64_t g_ap_back_note = 0;
uint64_t g_ap_back_arc_child = 0;

#define AP_HIT_STUB(NAME, BACK)                     \
    __attribute__((naked, used)) static void NAME() \
    {                                               \
        __asm__ __volatile__(                       \
            "str  x1, [sp, #-0x10]!\n"              \
            "ldr  x0, [x19, #0x38]\n"               \
            "mov  w2, #0\n"                         \
            "mov  w3, #0\n"                         \
            "mov  w4, wzr\n"                        \
            "mov  w5, #-1\n"                        \
            "ldr  x1, [sp]\n"                       \
            "adrp x16, g_ap_hit_fn\n"               \
            "ldr  x16, [x16, #:lo12:g_ap_hit_fn]\n" \
            "blr  x16\n"                            \
            "ldr  x0, [x19, #0x40]\n"               \
            "cbz  x0, 9f\n"                         \
            "mov  w2, #0\n"                         \
            "mov  w3, #1\n"                         \
            "ldr  x9, [x0]\n"                       \
            "cbz  x9, 9f\n"                         \
            "ldr  x9, [x9, #0x8]\n"                 \
            "cbz  x9, 9f\n"                         \
            "ldr  x1, [sp]\n"                       \
            "blr  x9\n"                             \
            "9:\n"                                  \
            "ldr  x1, [sp], #0x10\n"                \
            "adrp x16, " #BACK "\n"                 \
            "ldr  x16, [x16, #:lo12:" #BACK "]\n"   \
            "br   x16\n");                          \
    }

AP_HIT_STUB(apHitStubNote, g_ap_back_note)
AP_HIT_STUB(apHitStubArcChild, g_ap_back_arc_child)

static void *apStubFor(int i, uint64_t **backSlot)
{
    *backSlot = nullptr;
    if (((AP_STUB_SITES >> i) & 1) == 0)
        return nullptr;
    switch (i)
    {
    case AP_SITE_MISS_NOTE:
        *backSlot = &g_ap_back_note;
        return reinterpret_cast<void *>(&apHitStubNote);
    case AP_SITE_MISS_ARC_CHILD:
        *backSlot = &g_ap_back_arc_child;
        return reinterpret_cast<void *>(&apHitStubArcChild);
    default:
        return nullptr;
    }
}
#endif

void installAutoplayHook()
{
    (void)dlopen(TARGET, RTLD_NOW | RTLD_GLOBAL);

    const uintptr_t base = getModuleBase(TARGET);
    g_ap_base = base;

    for (int i = 0; i < AP_SITE_COUNT; i++)
        g_ap_orig[i] = readWord(base + g_ap_sites[i].at);

#if AP_STUB_SITES
    g_ap_hit_fn = base + OFF_AP_HIT_FN;
#endif

#if AP_ENTRY_HOOK
    g_ap_scan = installTrampoline(base + OFF_AP_SCAN, (void *)&onNoteScan);
#endif
}
