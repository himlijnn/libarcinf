// 资源路径重定向：把歌曲资源指向 ASSETS_DIR
// 1. 处理 songs/* 与 undyingmacula/*；其它路径交给原函数。
//
// 2. 处理 <DIR>/dl/hascome_video.mp4
// 这里改成读 ASSETS_DIR + "songs/hascome/video.mp4"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

static char g_redirectedPath[128];

// 被 pathTramp 的 naked asm 用 adrp 直接引用，必须保留外部链接与固定名字。
void *g_cave = nullptr;
const char *(*g_pathfn)(const char *) = nullptr;

// 明日方舟
// 被 buildPathTramp 的 naked asm 用 adrp 引用，同样必须在文件作用域。
void *g_buildPathCave = nullptr;
void (*g_buildPathFn)(const void *nameStr, void *outStr) = nullptr;

namespace
{
    // FileUtils vtable 里 isFileExist 的槽偏移（槽里是 0x12EC728 转发 thunk，
    // 内部再跳 +0x190 到实现 0x15F7664）。
    constexpr size_t kSlotIsFileExist = 0x110;

    constexpr char kVideoMatch[] = "hascome_video.mp4";

    constexpr char kVideoTarget[] = ASSETS_DIR "songs/hascome/video.mp4";

    // libc++ std::string：byte0 bit0 = 1 表示长串（size 在 +8，dataPtr 在 +16），
    // 否则 size = byte0 >> 1，数据紧随其后（SSO，最多 22 字节）。
    void readGameStr(const void *self, char *out, size_t cap)
    {
        if (!out || cap == 0)
            return;
        out[0] = '\0';
        if (!self)
            return;

        const unsigned char *raw = static_cast<const unsigned char *>(self);
        const char *src = nullptr;
        size_t len = 0;

        if (raw[0] & 1)
        {
            memcpy(&src, raw + 16, sizeof(src));
            memcpy(&len, raw + 8, sizeof(len));
        }
        else
        {
            len = raw[0] >> 1;
            src = reinterpret_cast<const char *>(raw + 1);
        }

        if (!src)
            return;
        if (len > 0x1000)
            len = 0x1000;
        if (len >= cap)
            len = cap - 1;

        memcpy(out, src, len);
        out[len] = '\0';
    }

    // 原地改写 libc++ std::string 的内容，不调用游戏的任何字符串函数
    bool writeGameStr(void *self, const char *text)
    {
        if (!self || !text)
            return false;

        auto *raw = static_cast<unsigned char *>(self);
        const size_t len = strlen(text);

        if (len <= 22)
        {
            raw[0] = static_cast<unsigned char>(len << 1);
            memcpy(raw + 1, text, len);
            raw[1 + len] = '\0';
            return true;
        }

        if (!(raw[0] & 1))
            return false; // 老串是 SSO，没有堆缓冲可用

        char *heap = nullptr;
        size_t capacity = 0;
        memcpy(&heap, raw + 16, sizeof(heap));
        memcpy(&capacity, raw + 8, sizeof(capacity));
        if (!heap || capacity < len)
            return false;

        memcpy(heap, text, len);
        heap[len] = '\0';
        memcpy(raw + 8, &len, sizeof(len));
        return true;
    }

    // 挂 FileUtils vtable +0x110：只影响这一个虚接口，其它调用方不受影响
    // 只对 hascome_video.mp4 放行，其它文件保持原逻辑
    typedef bool (*IsFileExistFn)(void *self, const void *nameStr);
    IsFileExistFn g_realIsFileExist = nullptr;

    bool videoExistsHook(void *self, const void *nameStr)
    {
        char name[64];
        readGameStr(nameStr, name, sizeof(name));

        if (!strstr(name, kVideoMatch))
            return g_realIsFileExist ? g_realIsFileExist(self, nameStr) : false;

        // 目标必须真的存在才放行，免得游戏播放滚木
        if (access(kVideoTarget, F_OK) == 0)
            return true;

        return g_realIsFileExist ? g_realIsFileExist(self, nameStr) : false;
    }

    // 放行之后 setURL 还会再拼一次路径，必须导到 assets 里的实际文件
    void rewriteBuiltPath(const void *nameStr, void *outStr)
    {
        char name[64];
        readGameStr(nameStr, name, sizeof(name));

        if (!strstr(name, kVideoMatch))
            return;

        writeGameStr(outStr, kVideoTarget);
    }

    // (x0 = 文件名 std::string*, x8 = sret 返回缓冲区)
    __attribute__((naked)) void buildPathTramp()
    {
        asm volatile(
            "stp x29, x30, [sp, #-0x20]!\n"
            "mov x29, sp\n"
            "stp x19, x20, [sp, #0x10]\n"
            "mov x19, x0\n"
            "mov x20, x8\n"
            "adrp x16, g_buildPathCave\n"
            "ldr x16, [x16, #:lo12:g_buildPathCave]\n"
            "blr x16\n"
            "mov x0, x19\n"
            "mov x1, x20\n"
            "adrp x16, g_buildPathFn\n"
            "ldr x16, [x16, #:lo12:g_buildPathFn]\n"
            "blr x16\n"
            "mov x0, x20\n"
            "ldp x19, x20, [sp, #0x10]\n"
            "ldp x29, x30, [sp], #0x20\n"
            "ret\n");
    }
}

// 其它视频

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

static void installVideoAkHook()
{

    typedef void *(*GetInstanceFn)();
    auto getInstance = reinterpret_cast<GetInstanceFn>(g_base + OFF_FileUtilsInstance);
    void *utils = getInstance ? getInstance() : nullptr;
    if (utils)
    {
        void *vtable = nullptr;
        memcpy(&vtable, utils, sizeof(vtable));

        if (vtable)
        {
            auto *slot = reinterpret_cast<void **>(reinterpret_cast<char *>(vtable) +
                                                   kSlotIsFileExist);
            void *original = nullptr;
            memcpy(&original, slot, sizeof(original));

            if (original)
            {
                // vtable 在 .data.rel.ro（只读）：临时放开写，写完按原样收回。
                const long ps = sysconf(_SC_PAGESIZE) > 0 ? sysconf(_SC_PAGESIZE) : 0x1000;
                const uintptr_t page =
                    reinterpret_cast<uintptr_t>(slot) & ~(static_cast<uintptr_t>(ps) - 1);

                if (mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_WRITE) == 0)
                {
                    g_realIsFileExist = reinterpret_cast<IsFileExistFn>(original);
                    void *replacement = reinterpret_cast<void *>(&videoExistsHook);
                    memcpy(slot, &replacement, sizeof(replacement));
                    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ);
                }
            }
        }
    }

    const uintptr_t buildPath = g_base + OFF_BuildDiskPath;
    uint8_t original[16];
    memcpy(original, reinterpret_cast<const void *>(buildPath), sizeof(original));

    g_buildPathCave = reinterpret_cast<void *>(makeTrampCave(original, buildPath + 16));
    if (!g_buildPathCave)
        return;

    g_buildPathFn = &rewriteBuiltPath;
    patchTrampEntry(buildPath, (void *)&buildPathTramp);
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
    if (g_cave)
        patchTrampEntry(target, (void *)&pathTramp);

    installVideoAkHook();
}