// 自动游玩：按开关值装 / 撤代码补丁。

//   1. 命中接管：MISS_NOTE / MISS_ARC_CHILD 两个站点换成 asm stub，由 stub 调游戏的计分函数；
//   2. 漏键窗口：把 `add w8, w10, #imm` 里的等待毫秒数改小，让判定线附近就开始处理；
//   3. 长条 / 蛇自动按住：onNoteScan 里遍历音符链表，到了时间就调原命中函数。
//
// 只读 settingsIsAutoPlay()，不写存档；关闭时把所有补丁还原成原指令。

// autoPlay 的值在游戏初始化角色时读取，参考 skill.cpp

#include <cstdint>
#include <cstring>

#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

#if enableAutoplayHook

struct ApSite
{
    uint32_t at;      // 站点偏移
    uint32_t skip;    // 非 0 = 把该指令换成跳到 skip（漏键 → 跳过）
    uint32_t newWord; // skip == 0 时用来改位域
    uint32_t selfMask;
};

// 下标具名
static constexpr int AP_SITE_MISS_NOTE = 0;
static constexpr int AP_SITE_MISS_ARC_CHILD = 1;
static constexpr int AP_SITE_WINDOW_TAP = 2;
static constexpr int AP_SITE_WINDOW_NOTE = 3;
static constexpr int AP_SITE_WINDOW_ARCTAP = 4;
static constexpr int AP_SITE_COUNT = 5;

// 窗口判定里要改的位域：`add w8, w10, #imm` 的 imm12 部分（掩码 0x003FFC00）。
static constexpr uint32_t AP_WINDOW_IMM = 0xFFFu << 10;

static const ApSite g_ap_sites[AP_SITE_COUNT] = {
    {OFF_AP_MISS_NOTE, OFF_AP_SKIP_NOTE, 0, 0},
    {OFF_AP_MISS_ARC_CHILD, OFF_AP_SKIP_ARC_CHILD, 0, 0},
    {OFF_AP_WINDOW_TAP, 0, 0x11000148, AP_WINDOW_IMM},
    {OFF_AP_WINDOW_NOTE, 0, 0x11000148, AP_WINDOW_IMM},
    {OFF_AP_WINDOW_ARCTAP, 0, 0x11000148, AP_WINDOW_IMM},
};

// 前两个站点用 asm stub 接管，其余直接改指令字。
static constexpr int AP_STUB_COUNT = 2;

static bool isStubSite(int index)
{
    return index >= 0 && index < AP_STUB_COUNT;
}

// naked asm 用 adrp 引用，名字与外部链接必须固定，不能加 static

// 游戏的计分函数地址（install 时写入 = g_base + OFF_AP_HIT_FN）
uint64_t g_ap_hit_fn = 0;

// stub 跑完后跳回游戏的位置（站点 + 0x18）
uint64_t g_ap_back_note = 0;
uint64_t g_ap_back_arc_child = 0;

static void apHitStubNote();
static void apHitStubArcChild();

static uint32_t g_ap_orig[AP_SITE_COUNT] = {0};
static int8_t g_ap_patched[AP_SITE_COUNT] = {0};
static uint32_t g_ap_stub_save[AP_STUB_COUNT][4] = {{0}};

static Trampoline g_ap_scan;
static uint32_t g_ap_entry_save[4] = {0};
static bool g_ap_entry_on = false;

int64_t onNoteScan(int64_t ctx, int64_t note);
static void setEntryHook(bool on);

// 长条 / 蛇自动按住

// 已"起手"的音符，最多记住 256 个，环状覆盖（同一帧内同一个 note 只调一次起手）。
static constexpr int AP_STARTED_MAX = 256;
static int g_started_cnt = 0;
static uintptr_t g_started[AP_STARTED_MAX] = {0};

static uintptr_t rdptr(uintptr_t addr)
{
    uintptr_t value = 0;
    memcpy(&value, reinterpret_cast<const void *>(addr), sizeof(value));
    return value;
}

static int32_t rdword(uintptr_t addr)
{
    return static_cast<int32_t>(readWord(addr));
}

static bool isStarted(uintptr_t note)
{
    const int limit = g_started_cnt < AP_STARTED_MAX ? g_started_cnt : AP_STARTED_MAX;
    for (int i = 0; i < limit; i++)
        if (g_started[i] == note)
            return true;
    return false;
}

static bool markStartedOnce(uintptr_t note)
{
    if (isStarted(note))
        return false;
    g_started[g_started_cnt % AP_STARTED_MAX] = note;
    g_started_cnt++;
    return true;
}

// 当前谱面时间（毫秒）。cfg+0x2d 是"变速/暂停"标志，两种分支公式不同。
static int32_t nowMs(uintptr_t ctx)
{
    const uintptr_t cfg = rdptr(ctx + 0x30);
    const int32_t cfg28 = rdword(cfg + 0x28);
    if ((rdword(cfg + 0x2d) & 0xff) != 0)
        return rdword(cfg + 0x20) - cfg28;

    const int32_t c34 = rdword(cfg + 0x34);
    return c34 - cfg28 + (c34 > 0 ? 0 : -3000);
}

// 蛇起手：构造一个 0x40 字节的命中上下文，+0x34 置 -1 表示"由我们代按"。
static void startArcHit(uintptr_t note, uintptr_t startFn, int32_t now)
{
    unsigned char hitCtx[0x40];
    memset(hitCtx, 0, sizeof(hitCtx));
    *reinterpret_cast<int32_t *>(hitCtx + 0x34) = -1;
    reinterpret_cast<void (*)(uintptr_t, void *, int32_t)>(startFn)(note, hitCtx, now);
}

static void tickLongNotes(uintptr_t ctx)
{
    const uintptr_t begin = rdptr(ctx + 0xa0), end = rdptr(ctx + 0xa8);
    const int32_t now = nowMs(ctx);

    for (uintptr_t slot = begin; slot < end; slot += 8)
    {
        const uintptr_t note = rdptr(slot);
        const uintptr_t vt = rdptr(note);
        const bool isArc = (vt == g_base + OFF_AP_VT_ARC);
        const bool isHold = (vt == g_base + OFF_AP_VT_HOLD);
        if (!isArc && !isHold)
            continue;

        const int32_t startTime = rdword(note + 0x18), endTime = rdword(note + 0x1c);
        const unsigned flag54 = *reinterpret_cast<unsigned char *>(note + 0x54);
        const unsigned flag64 = *reinterpret_cast<unsigned char *>(note + 0x64);
        const int32_t stateA4 = rdword(note + 0xa4);

        if (flag54 == 0)
            continue;

        // 段表是 0xC 步长；长度不整或太小说明还在构造，跳过这一帧。
        const uintptr_t segBegin = rdptr(note + 0x78), segEnd = rdptr(note + 0x80);
        const int64_t segBytes = static_cast<int64_t>(segEnd) - static_cast<int64_t>(segBegin);
        if (segBytes < 0xc || (segBytes % 0xc) != 0)
            continue;

        if (now < startTime || now > endTime + 100)
            continue;

        if (isArc)
        {
            if (stateA4 != 0)
                continue;

            if (startTime == endTime)
            {
                // 零时长弧 = 单点，直接标记完成。
                *reinterpret_cast<uint32_t *>(note + 0xa4) = 1;
                continue;
            }

            const uintptr_t vis = rdptr(note + 0xe0);

            if (flag64 == 0 && markStartedOnce(note))
            {
                const uintptr_t startFn = rdptr(vt + 0x60);
                if (startFn)
                    startArcHit(note, startFn, now);
            }

            if (vis)
            {
                // 视觉标记：已按住 + 延长显示。
                *reinterpret_cast<uint16_t *>(vis + 0x10) = 0x0101;
                *reinterpret_cast<unsigned char *>(vis + 0x12) = 1;
                *reinterpret_cast<float *>(vis + 0x14) = static_cast<float>(now + 500);
            }
        }
        else
        {
            // 长条在起点后 16ms 再开始，避免视觉抖动。
            if (now < startTime + 16)
                continue;

            if (flag64 == 0 && markStartedOnce(note))
            {
                const uintptr_t startFn = rdptr(vt + 0x60);
                if (startFn)
                    reinterpret_cast<void (*)(uintptr_t, uintptr_t, int32_t)>(startFn)(
                        note, 0, now);
            }

            *reinterpret_cast<uint32_t *>(note + 0x30) = 0;
            *reinterpret_cast<unsigned char *>(note + 0xa8) = 1;
        }

        *reinterpret_cast<uint16_t *>(note + 0x64) = 0x0101;
    }
}

// 站点补丁

// 命中站点：备份原 16 字节 → patchTrampEntry 换成 asm stub。
static void applyStubSite(int index, int on, uintptr_t addr)
{
    uint64_t *const back = (index == AP_SITE_MISS_NOTE) ? &g_ap_back_note
                                                        : &g_ap_back_arc_child;
    void *const stub = (index == AP_SITE_MISS_NOTE)
                           ? reinterpret_cast<void *>(&apHitStubNote)
                           : reinterpret_cast<void *>(&apHitStubArcChild);

    if (on)
    {
        memcpy(g_ap_stub_save[index], reinterpret_cast<const void *>(addr), 16);
        *back = addr + 0x18;
        patchTrampEntry(addr, stub);
    }
    else
    {
        for (int k = 0; k < 4; k++)
            patchWord(addr + 4 * k, g_ap_stub_save[index][k]);
    }
    g_ap_patched[index] = on ? 1 : 0;
}

// 非 stub 站点：skip 非 0 改成跳转，否则按 selfMask 改位域（窗口毫秒数）。
static void applyWordSite(int index, int on, uintptr_t addr)
{
    const ApSite &site = g_ap_sites[index];
    uint32_t word = g_ap_orig[index];

    if (on)
    {

        word = (g_ap_orig[index] & ~site.selfMask) | (site.newWord & site.selfMask);
    }

    patchWord(addr, word);
    g_ap_patched[index] = on ? 1 : 0;
}

static void applySite(int index, int on)
{
    const uintptr_t addr = g_base + g_ap_sites[index].at;
    if (isStubSite(index))
        applyStubSite(index, on, addr);
    else
        applyWordSite(index, on, addr);
}

// 入口 hook（只在开启时挂）

static void setEntryHook(bool on)
{
    if (on == g_ap_entry_on)
        return;

    const uintptr_t entry = g_base + OFF_AP_SCAN;
    if (on)
        patchTrampEntry(entry, (void *)&onNoteScan);
    else
        for (int k = 0; k < 4; k++)
            patchWord(entry + 4 * k, g_ap_entry_save[k]);

    g_ap_entry_on = on;
}

// 对外入口

void autoplayApply(int on)
{
    if (!g_base)
        return;

    if (!on)
        g_started_cnt = 0;

    const bool enable = on != 0;
    for (int i = 0; i < AP_SITE_COUNT; i++)
    {
        if (g_ap_patched[i] == (enable ? 1 : 0))
            continue;
        applySite(i, enable ? 1 : 0);
    }

    setEntryHook(enable);
}

int64_t onNoteScan(int64_t ctx, int64_t note)
{
    if (settingsIsAutoPlay())
        tickLongNotes(static_cast<uintptr_t>(ctx));

    typedef int64_t (*PFN)(int64_t, int64_t);
    return reinterpret_cast<PFN>(g_ap_scan.cave)(ctx, note);
}

// 命中 stub（naked，只能用汇编写）

//
// 两个漏键站点都进这里：
//   * x19 = judgeCtx，[x19+0x38] = 计分对象，[x19+0x40] = 回调对象；
//   * 先把 x1（当前 note）压栈，之后所有 bl 都可能破坏 x1；
//   * 调完 g_ap_hit_fn 后恢复 x1、x0，再跳回 g_ap_back_*。
//
// 规则：跨 bl 的值必须存栈或 callee-saved；这里把 note 存在 [sp]。

#define AP_HIT_STUB_BODY(BACK_FIELD)                \
    asm volatile(                                   \
        "str  x1, [sp, #-0x10]!\n"                  \
        "ldr  x0, [x19, #0x38]\n"                   \
        "mov  w2, #0\n"                             \
        "mov  w3, #0\n"                             \
        "mov  w4, wzr\n"                            \
        "mov  w5, #-1\n"                            \
        "ldr  x1, [sp]\n"                           \
        "adrp x16, g_ap_hit_fn\n"                   \
        "ldr  x16, [x16, #:lo12:g_ap_hit_fn]\n"     \
        "blr  x16\n"                                \
        "ldr  x0, [x19, #0x40]\n"                   \
        "cbz  x0, 9f\n"                             \
        "mov  w2, #0\n"                             \
        "mov  w3, #1\n"                             \
        "ldr  x9, [x0]\n"                           \
        "cbz  x9, 9f\n"                             \
        "ldr  x9, [x9, #0x8]\n"                     \
        "cbz  x9, 9f\n"                             \
        "ldr  x1, [sp]\n"                           \
        "blr  x9\n"                                 \
        "9:\n"                                      \
        "ldr  x1, [sp], #0x10\n"                    \
        "adrp x16, " #BACK_FIELD "\n"               \
        "ldr  x16, [x16, #:lo12:" #BACK_FIELD "]\n" \
        "br   x16\n")

__attribute__((naked, used)) static void apHitStubNote()
{
    AP_HIT_STUB_BODY(g_ap_back_note);
}

__attribute__((naked, used)) static void apHitStubArcChild()
{
    AP_HIT_STUB_BODY(g_ap_back_arc_child);
}

void installAutoplayHook()
{
    // 备份站点原指令与入口序言。此时不碰游戏代码，也不读存档。
    for (int i = 0; i < AP_SITE_COUNT; i++)
        if (!isStubSite(i))
            g_ap_orig[i] = readWord(g_base + g_ap_sites[i].at);

    g_ap_hit_fn = g_base + OFF_AP_HIT_FN;

    for (int k = 0; k < 4; k++)
        g_ap_entry_save[k] = readWord(g_base + OFF_AP_SCAN + 4 * k);

    // 洞穴必须无条件建：关闭状态下之后也可能被打开。
    if (!g_ap_scan.cave)
    {
        g_ap_scan.addr = g_base + OFF_AP_SCAN;
        g_ap_scan.cave = makeTrampCave(g_ap_entry_save, g_ap_scan.addr + 16);
    }
}

#else

void installAutoplayHook()
{
}

void autoplayApply(int on)
{
    (void)on;
}

#endif
