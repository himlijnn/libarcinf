// 资源路径重定向：把歌曲资源指向 ASSETS_DIR。
// 只处理 songs/* 与 undyingmacula/*；其它路径交给原函数。

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

static char g_redirectedPath[128];

// 被 pathTramp 的 naked asm 用 adrp 直接引用，必须保留外部链接与固定名字。
void *g_cave = nullptr;
const char *(*g_pathfn)(const char *) = nullptr;

// undyingmacula/* 是旧版布局，统一挪到 songs/ 下；其它路径原样返回。
static const char *transformPath(char *buffer, int size, const char *path)
{
    if (!path || !*path)
        return path;

    const char *relativePath = path;
    while (*relativePath == '/')
        relativePath++;

    const char *prefix = "";
    if (strncmp(relativePath, "undyingmacula/", 14) == 0)
        prefix = "songs/";
    else if (strncmp(relativePath, "songs/", 6) != 0)
        return path;

    const int written = snprintf(buffer, size, "%s%s%s", ASSETS_DIR, prefix, relativePath);
    return (written < size) ? buffer : path;
}

static const char *redirectPath(const char *originalPath)
{
    return transformPath(g_redirectedPath, sizeof(g_redirectedPath), originalPath);
}

// (x0=receiver, x1=path)：保留 receiver，用重定向后的 path 调原函数。
// 原函数后半段直接使用游戏的 x19 等寄存器，因此这里显式保存/恢复。
__attribute__((naked)) static void pathTramp()
{
    asm volatile(
        "stp x29, x30, [sp, #-16]!\n"
        "mov x29, sp\n"
        "stp x19, x20, [sp, #-16]!\n"
        "mov x19, x0\n"
        "mov x0, x1\n"
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
    g_pathfn = &redirectPath;

    const uintptr_t target = g_base + OFF_PATH_TRAMP;

    // 原序言搬到洞穴，入口只覆写一次；之后始终按路径分流。
    uint8_t originalInstructions[16];
    memcpy(originalInstructions, reinterpret_cast<const void *>(target),
           sizeof(originalInstructions));
    g_cave = reinterpret_cast<void *>(makeTrampCave(originalInstructions, target + 16));
    if (!g_cave)
        return;

    patchTrampEntry(target, (void *)&pathTramp);
}
