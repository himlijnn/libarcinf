#pragma once

#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

inline long trampPageSize()
{
    const long ps = sysconf(_SC_PAGESIZE);
    return ps == -1 ? 0x1000 : ps;
}

inline uintptr_t getModuleBase(const char *soname)
{
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp)
        return 0;

    char line[512];
    while (fgets(line, sizeof(line), fp))
    {
        char perms[8], path[256];
        unsigned long start, end;
        if (sscanf(line, "%lx-%lx %4s %*s %*s %*s %255s",
                   &start, &end, perms, path) >= 3)
        {
            const char *fname = strrchr(path, '/');
            fname = fname ? fname + 1 : path;
            if (strstr(fname, soname) && perms[2] == 'x')
            {
                fclose(fp);
                return start;
            }
        }
    }
    fclose(fp);
    return 0;
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
    void *p = mmap(nullptr, ps, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED)
        return 0;

    uint8_t *c = static_cast<uint8_t *>(p);
    memcpy(c, orig16, 16);
    uint32_t *w = reinterpret_cast<uint32_t *>(c + 16);
    w[0] = 0x58000050; // LDR X16, #8
    w[1] = 0xD61F0200; // BR  X16
    memcpy(c + 24, &back_addr, 8);

    __builtin___clear_cache(reinterpret_cast<char *>(c), reinterpret_cast<char *>(c + 32));
    if (mprotect(p, ps, PROT_READ | PROT_EXEC) != 0)
    {
        munmap(p, ps);
        return 0;
    }
    return reinterpret_cast<uintptr_t>(p);
}

inline bool patchTrampEntry(uintptr_t target, void *hook_fn)
{
    const long ps = trampPageSize();
    const uintptr_t page = target & ~(static_cast<uintptr_t>(ps) - 1);

    uint32_t jump[4] = {0x58000050, 0xD61F0200, 0, 0};
    const uintptr_t fn = reinterpret_cast<uintptr_t>(hook_fn);
    memcpy(&jump[2], &fn, 8);

    if (mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
        return false;

    memcpy(reinterpret_cast<void *>(target), jump, 16);
    __builtin___clear_cache(reinterpret_cast<char *>(page), reinterpret_cast<char *>(page + ps));
    mprotect(reinterpret_cast<void *>(page), ps, PROT_READ | PROT_EXEC);
    return true;
}

inline Trampoline installTrampoline(uintptr_t target, void *hook_fn)
{
    Trampoline t;
    if (!target || !hook_fn)
        return t;

    uint8_t orig[16];
    memcpy(orig, reinterpret_cast<const void *>(target), 16);
    t.addr = target;
    t.cave = makeTrampCave(orig, target + 16);
    if (!t.cave)
        return t;
    if (!patchTrampEntry(target, hook_fn))
    {
        munmap(reinterpret_cast<void *>(t.cave), trampPageSize());
        t.cave = 0;
        return t;
    }
    t.ok = true;
    return t;
}

inline Trampoline replaceFunction(uintptr_t target, void *hook_fn)
{
    Trampoline t;
    if (!target || !hook_fn)
        return t;
    t.addr = target;
    t.ok = patchTrampEntry(target, hook_fn);
    return t;
}
