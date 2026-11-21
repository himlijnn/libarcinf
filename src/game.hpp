#pragma once

// 游戏侧 std::string 与 UserDefault 的最小封装。
//
// 为什么不能直接用自己的 std::string：
//   * 游戏用的是 libc++（c++_static），与宿主 App 的 STL 实例不是同一份；
//   * 布局是 libc++ 的 24 字节形态：byte0 bit0 = 1 表示长串（堆上），否则短串
//     （size = byte0 >> 1，数据紧随其后，最多 22 字节）。
// 因此构造一律走游戏自己的 string ctor，析构走游戏的 operator delete。

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <dlfcn.h>

#include "offset.hpp"

extern uintptr_t g_base;

struct GameStr
{
    uint64_t hdr;
    uint64_t pad;
    uint64_t dataPtr;
};

inline bool gameStrIsLong(const GameStr *s)
{
    return (s->hdr & 1) != 0;
}

inline const char *gameStrText(const GameStr *s)
{
    return gameStrIsLong(s) ? reinterpret_cast<const char *>(s->dataPtr)
                            : reinterpret_cast<const char *>(s) + 1;
}

// 游戏侧的 operator delete（_ZdlPv），只在需要释放长串时延迟解析一次。
inline void gameOperatorDelete(void *p)
{
    static void (*fn)(void *) = nullptr;
    static bool resolved = false;
    if (!resolved)
    {
        resolved = true;
        fn = reinterpret_cast<void (*)(void *)>(dlsym(RTLD_DEFAULT, "_ZdlPv"));
    }
    if (fn && p)
        fn(p);
}

inline void gameStrInit(GameStr *s, const char *text)
{
    typedef void (*PFN_StrCtor)(void *, const char *);
    reinterpret_cast<PFN_StrCtor>(g_base + OFF_StringCtor)(s, text ? text : "");
}

inline void gameStrFree(GameStr *s)
{
    if (gameStrIsLong(s))
        gameOperatorDelete(reinterpret_cast<void *>(s->dataPtr));
}

// RAII 版本，避免每个调用点都手写 init / free 配对。
class ScopedGameStr
{
public:
    explicit ScopedGameStr(const char *text)
    {
        gameStrInit(&str_, text);
    }

    ~ScopedGameStr()
    {
        gameStrFree(&str_);
    }

    ScopedGameStr(const ScopedGameStr &) = delete;
    ScopedGameStr &operator=(const ScopedGameStr &) = delete;

    GameStr *get() { return &str_; }
    const GameStr *get() const { return &str_; }
    const char *text() const { return gameStrText(&str_); }

private:
    GameStr str_ = {};
};


// cocos 存档工具

// UserDefault：内部转成 SharedPreferences，this 参数游戏自己不用，传 nullptr 安全。

inline int gameGetBool(const char *key, int def)
{
    typedef int (*PFN)(void *, const char *, int);
    return reinterpret_cast<PFN>(g_base + OFF_UserDefaultGetBool)(nullptr, key, def) ? 1 : 0;
}

inline void gameSetBool(const char *key, int value)
{
    typedef void (*PFN)(void *, const char *, int);
    reinterpret_cast<PFN>(g_base + OFF_UserDefaultSetBool)(nullptr, key, value ? 1 : 0);
}

// 读字符串；返回值复制进 out，调用方不持有游戏侧内存。
inline void gameGetString(const char *key, const char *def, char *out, size_t outSize)
{
    typedef GameStr (*PFN)(void *, const char *, GameStr *);

    if (!out || outSize == 0)
        return;
    out[0] = 0;

    ScopedGameStr defStr(def);
    GameStr value = reinterpret_cast<PFN>(g_base + OFF_UserDefaultGetString)(
        nullptr, key, defStr.get());

    // 拷贝到调用方缓冲；游戏返回的串可能是长串（堆上），必须还给游戏释放。
    const char *text = gameStrText(&value);
    const size_t len = text ? __builtin_strlen(text) : 0;
    if (len >= outSize)
    {
        __builtin_memcpy(out, text, outSize - 1);
        out[outSize - 1] = 0;
    }
    else
    {
        __builtin_memcpy(out, text, len);
        out[len] = 0;
    }
    gameStrFree(&value);
}

inline void gameSetString(const char *key, const char *value)
{
    typedef void (*PFN)(void *, const char *, GameStr *);
    ScopedGameStr str(value);
    reinterpret_cast<PFN>(g_base + OFF_UserDefaultSetString)(nullptr, key, str.get());
}

// cocos 节点通用工具
//
// 节点首字段是 vtable 指针，所有方法都按字节偏移取槽

// 取虚表槽；obj 为空或 vtable 为空时返回 nullptr
inline void *gameVslot(const void *obj, size_t byteOffset)
{
    if (!obj)
        return nullptr;
    void *const *vt = *reinterpret_cast<void *const *const *>(obj);
    if (!vt)
        return nullptr;
    return *reinterpret_cast<void *const *>(reinterpret_cast<const char *>(vt) + byteOffset);
}

// 调用节点虚方法：gameVcall<void (*)(void *, int)>(node, OFF_VT_SET_VISIBLE, 1)
template <typename Fn, typename... Args>
inline auto gameVcall(void *obj, size_t byteOffset, Args... args) -> decltype(auto)
{
    auto fn = reinterpret_cast<Fn>(gameVslot(obj, byteOffset));
    return fn(obj, args...);
}

// 用游戏 ABI 构造 std::string 并调用 fn(&str)，结束后释放。
template <typename Fn> inline bool withGameStr(const char *text, Fn fn)
{
    GameStr str;
    gameStrInit(&str, text);
    const bool ok = fn(&str);
    gameStrFree(&str);
    return ok;
}

// 按名字取子节点（vtable + OFF_VT_GET_CHILD）。找不到返回 nullptr。
inline void *childByName(void *root, const char *name)
{
    if (!root)
        return nullptr;
    void *found = nullptr;
    withGameStr(name, [&](void *str) {
        found = gameVcall<void *(*)(void *, void *)>(root, OFF_VT_GET_CHILD, str);
        return true;
    });
    return found;
}

// getVisibleSize 的 ABI 是 void f(Size *sret /*x8*/)，必须用裸垫片把 x0 搬到 x8。
// 用法：callGetVisibleSize(out, g_base + OFF_GetVisibleSize);
__attribute__((naked)) static void callGetVisibleSize(void *out __attribute__((unused)),
                                                      void *fn __attribute__((unused)))
{
    asm volatile("mov x8, x0\n"
                 "br  x1\n");
}
