#include <dlfcn.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

static char g_newpath[1024];

void *g_cave = nullptr;
const char *(*g_pathfn)(const char *) = nullptr;

static const char *transformPath(char *buf, int sz, const char *path)
{
    if (!path || !*path)
        return path;

    const char *rel = path;
    while (*rel == '/')
        rel++;

    if (strncmp(rel, "songs/", 6) == 0)
        rel += 6;
    else if (strncmp(rel, "undyingmacula/", 14) != 0)
        return path;

    if (!*rel)
        return path;

    int n = snprintf(buf, sz, "%ssongs/%s", OFF_ASSETS_DIR, rel);
    return (n > 0 && n < sz) ? buf : path;
}

static const char *redirectPath(const char *orig)
{
    return transformPath(g_newpath, sizeof(g_newpath), orig);
}

__attribute__((naked)) static void pathTramp()
{
    asm volatile(
        "stp x29, x30, [sp, #-16]!\n"
        "mov x29, sp\n"
        "stp x19, x20, [sp, #-16]!\n"
        "mov x19, x0\n"
        "mov x20, x1\n"
        "mov x0, x20\n"
        "adrp x16, g_pathfn\n"
        "ldr x16, [x16, #:lo12:g_pathfn]\n"
        "blr x16\n"
        "mov x1, x0\n"
        "mov x0, x19\n"
        "ldp x19, x20, [sp], #16\n"
        "ldp x29, x30, [sp], #16\n"
        "adrp x16, g_cave\n"
        "ldr x16, [x16, #:lo12:g_cave]\n"
        "br x16\n");
}

void installVideoHook()
{
    (void)dlopen(TARGET, RTLD_NOW | RTLD_GLOBAL);
    uintptr_t b = getModuleBase(TARGET);

    g_pathfn = &redirectPath;

    const uintptr_t tgt = b + OFF_PATH_TRAMP;

    uint8_t orig[16];
    memcpy(orig, (const void *)tgt, 16);
    g_cave = (void *)makeTrampCave(orig, tgt + 16);
    if (!g_cave)
        return;

    patchTrampEntry(tgt, (void *)&pathTramp);
}
