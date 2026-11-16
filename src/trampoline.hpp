#pragma once

// 蹦床 / 代码补丁原语。
//
// 蹦床做法：把目标函数前 16 字节改成 `LDR x16, #8 ; BR x16 ; <hook>`，
// 原 16 字节搬到一个 mmap 出来的「洞穴」里，后面接 `LDR x16, #8 ; BR x16 ; 目标+16`。
// 调用洞穴 == 调用原函数（hook 里通过 cave 回调原函数）。
//
// 代价：被搬走的 4 条指令换了地址执行，因此目标序言**不能含 PC 相对指令**
// （adrp/adr/ldr-字面量/b/bl/cbz/tbz）。常规序言（bti/paciasz/stp/mov/sub/str）都安全。

#include <link.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>

// 16 字节绝对跳转：`ldr x16, #8 ; br x16 ; .quad 0`。
// 后 8 字节由使用处填入目标地址。
__attribute__((naked, used)) static void asmAbsJump()
{
    asm volatile("ldr x16, #8\n"
                 "br x16\n"
                 ".quad 0\n");
}

// 单条 `b .`，makeBranchWord() 用它取出 B 的 opcode。
__attribute__((naked, used)) static void asmBranch()
{
    asm volatile("b .\n");
}

inline long trampPageSize()
{
    return sysconf(_SC_PAGESIZE);
}

struct ModuleSearch
{
    const char *soname;
    uintptr_t base;
};

inline bool isModuleName(const char *path, const char *soname)
{
    if (!path)
        return false;
    const char *file = strrchr(path, '/');
    file = file ? file + 1 : path;
    return strcmp(file, soname) == 0;
}

inline int onPhdrInfo(struct dl_phdr_info *info, size_t, void *data)
{
    ModuleSearch *search = static_cast<ModuleSearch *>(data);
    if (isModuleName(info->dlpi_name, search->soname))
    {
        search->base = static_cast<uintptr_t>(info->dlpi_addr);
        return 1;
    }
    return 0;
}

// 找不到返回 0；调用方必须判 0 后直接返回，不能用 dlopen 句柄顶替。
inline uintptr_t getModuleBase(const char *soname)
{
    ModuleSearch search = {soname, 0};
    dl_iterate_phdr(onPhdrInfo, &search);
    return search.base;
}

struct Trampoline
{
    uintptr_t addr = 0;
    uintptr_t cave = 0;
    bool ok = false;
};

// 代码页读写：临时放开写权限，写完立刻收回，不长期留 RWX

inline void patchMemory(uintptr_t addr, const void *bytes, size_t size)
{
    const long pageSize = trampPageSize();
    const uintptr_t page = addr & ~(static_cast<uintptr_t>(pageSize) - 1);
    mprotect(reinterpret_cast<void *>(page), pageSize, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(reinterpret_cast<void *>(addr), bytes, size);
    __builtin___clear_cache(reinterpret_cast<char *>(page),
                            reinterpret_cast<char *>(page + pageSize));
    mprotect(reinterpret_cast<void *>(page), pageSize, PROT_READ | PROT_EXEC);
}

inline void patchWord(uintptr_t addr, uint32_t word)
{
    patchMemory(addr, &word, sizeof(word));
}

inline uint32_t readWord(uintptr_t addr)
{
    uint32_t word = 0;
    memcpy(&word, reinterpret_cast<const void *>(addr), sizeof(word));
    return word;
}

// 生成从 from 跳到 to 的 B 指令字；位移超出 ±128MB 返回 0（调用方不要写）。
inline uint32_t makeBranchWord(uintptr_t from, uintptr_t to)
{
    const int64_t disp = static_cast<int64_t>(to) - static_cast<int64_t>(from);
    if (disp < -0x8000000LL || disp > 0x7FFFFFCLL)
        return 0;

    uint32_t opcode;
    memcpy(&opcode, reinterpret_cast<const void *>(&asmBranch), sizeof(opcode));
    const uint32_t imm26 = (1u << 26) - 1;
    return opcode | (static_cast<uint32_t>(disp >> 2) & imm26);
}

// 越界时不写，返回 false。
inline bool patchBranch(uintptr_t from, uintptr_t to)
{
    const uint32_t word = makeBranchWord(from, to);
    if (!word)
        return false;
    patchWord(from, word);
    return true;
}

inline uintptr_t makeTrampCave(const void *orig16, uintptr_t backAddr)
{
    const long pageSize = trampPageSize();
    void *mem = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED)
        return 0;

    uint8_t *cave = static_cast<uint8_t *>(mem);
    memcpy(cave, orig16, 16);
    memcpy(cave + 16, reinterpret_cast<const void *>(&asmAbsJump), 16);
    memcpy(cave + 24, &backAddr, 8);

    __builtin___clear_cache(reinterpret_cast<char *>(cave), reinterpret_cast<char *>(cave + 32));
    mprotect(mem, pageSize, PROT_READ | PROT_EXEC);
    return reinterpret_cast<uintptr_t>(mem);
}

inline void patchTrampEntry(uintptr_t target, void *hookFn)
{
    uint8_t jump[16];
    memcpy(jump, reinterpret_cast<const void *>(&asmAbsJump), sizeof(jump));

    const uintptr_t fn = reinterpret_cast<uintptr_t>(hookFn);
    memcpy(jump + 8, &fn, 8);

    patchMemory(target, jump, sizeof(jump));
}

// 需要回调原函数时
inline Trampoline installTrampoline(uintptr_t target, void *hookFn)
{
    Trampoline tramp;

    uint8_t orig[16];
    memcpy(orig, reinterpret_cast<const void *>(target), sizeof(orig));

    tramp.addr = target;
    tramp.cave = makeTrampCave(orig, target + 16);
    tramp.ok = tramp.cave != 0;
    if (tramp.ok)
        patchTrampEntry(target, hookFn);
    return tramp;
}

// 整段替换、不回调原函数时。
inline Trampoline replaceFunction(uintptr_t target, void *hookFn)
{
    Trampoline tramp;
    tramp.addr = target;
    patchTrampEntry(target, hookFn);
    tramp.ok = true;
    return tramp;
}

static void *vtableSlot(const void *obj, size_t byteOffset)
{
    void *const *vtable = *reinterpret_cast<void *const *const *>(obj);
    if (!vtable)
        return nullptr;
    return *reinterpret_cast<void *const *>(reinterpret_cast<const char *>(vtable) + byteOffset);
}