#pragma once

#include <link.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>

__attribute__((naked, used)) static void asmAbsJump()
{
    asm volatile("ldr x16, #8\n"
                 "br x16\n"
                 ".quad 0\n");
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
    const char *fname = strrchr(path, '/');
    fname = fname ? fname + 1 : path;
    return strcmp(fname, soname) == 0;
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

inline uintptr_t makeTrampCave(const void *orig16, uintptr_t back_addr)
{
    const long ps = trampPageSize();
    void *mem = mmap(nullptr, ps, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    uint8_t *cave = static_cast<uint8_t *>(mem);
    memcpy(cave, orig16, 16);
    memcpy(cave + 16, reinterpret_cast<const void *>(&asmAbsJump), 16);
    memcpy(cave + 24, &back_addr, 8);

    __builtin___clear_cache(reinterpret_cast<char *>(cave), reinterpret_cast<char *>(cave + 32));
    mprotect(mem, ps, PROT_READ | PROT_EXEC);
    return reinterpret_cast<uintptr_t>(mem);
}

inline bool patchTrampEntry(uintptr_t target, void *hook_fn)
{
    const long ps = trampPageSize();
    const uintptr_t page = target & ~(static_cast<uintptr_t>(ps) - 1);

    uint8_t jump[16];
    memcpy(jump, reinterpret_cast<const void *>(&asmAbsJump), sizeof(jump));
    const uintptr_t fn = reinterpret_cast<uintptr_t>(hook_fn);
    memcpy(jump + 8, &fn, 8);

    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(reinterpret_cast<void *>(target), jump, sizeof(jump));
    __builtin___clear_cache(reinterpret_cast<char *>(page), reinterpret_cast<char *>(page + ps));
    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_EXEC);
    return true;
}

__attribute__((naked, used)) static void asmBranch()
{
    asm volatile("b .\n");
}

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

inline Trampoline installTrampoline(uintptr_t target, void *hook_fn)
{
    Trampoline t;

    uint8_t orig[16];
    memcpy(orig, reinterpret_cast<const void *>(target), 16);
    t.addr = target;
    t.cave = makeTrampCave(orig, target + 16);
    patchTrampEntry(target, hook_fn);
    t.ok = true;
    return t;
}

inline Trampoline replaceFunction(uintptr_t target, void *hook_fn)
{
    Trampoline t;
    t.addr = target;
    t.ok = patchTrampEntry(target, hook_fn);
    return t;
}