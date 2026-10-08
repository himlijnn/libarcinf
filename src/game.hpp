
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <dlfcn.h>

#include "offset.hpp"

struct GameStr
{
    uint64_t hdr;
    uint64_t pad;
    uint64_t dataPtr;
};

inline const char *gameStrText(const GameStr *s)
{
    return (s->hdr & 1) ? reinterpret_cast<const char *>(s->dataPtr)
                        : reinterpret_cast<const char *>(s) + 1;
}

inline bool gameStrIsLong(const GameStr *s)
{
    return (s->hdr & 1) != 0;
}

inline void gameOperatorDelete(void *p)
{
    static void (*fn)(void *) = nullptr;
    static bool lookedUp = false;
    if (!lookedUp)
    {
        lookedUp = true;
        fn = reinterpret_cast<void (*)(void *)>(dlsym(RTLD_DEFAULT, "_ZdlPv"));
    }
    if (fn && p)
        fn(p);
}

inline void gameStrInit(uintptr_t base, GameStr *s, const char *v)
{
    typedef void (*PFN_StrCtor)(void *, const char *);
    ((PFN_StrCtor)(base + OFF_StringCtor))(s, v ? v : "");
}

inline void gameStrFree(GameStr *s)
{
    if (gameStrIsLong(s))
        gameOperatorDelete(reinterpret_cast<void *>(s->dataPtr));
}

inline int gameGetBool(uintptr_t base, const char *key, int def)
{
    typedef int (*PFN)(void *, const char *, int);
    return ((PFN)(base + OFF_UserDefaultGetBool))(nullptr, key, def) ? 1 : 0;
}

inline void gameSetBool(uintptr_t base, const char *key, int v)
{
    typedef void (*PFN)(void *, const char *, int);
    ((PFN)(base + OFF_UserDefaultSetBool))(nullptr, key, v ? 1 : 0);
}

inline void gameGetString(uintptr_t base, const char *key, const char *def, char *out, size_t sz)
{
    typedef GameStr (*PFN_Get)(void *, const char *, void *);
    GameStr defStr = {};
    gameStrInit(base, &defStr, def ? def : "");
    GameStr s = ((PFN_Get)(base + OFF_UserDefaultGetString))(nullptr, key, &defStr);
    snprintf(out, sz, "%s", gameStrText(&s));
    gameStrFree(&s);
    gameStrFree(&defStr);
}

inline void gameSetString(uintptr_t base, const char *key, const char *val)
{
    typedef void (*PFN_Set)(void *, const char *, void *);
    GameStr v = {};
    gameStrInit(base, &v, val ? val : "");
    ((PFN_Set)(base + OFF_UserDefaultSetString))(nullptr, key, &v);
    gameStrFree(&v);
}
