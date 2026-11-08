// Conan: FMOD's memory pool, logged around the --watch range. FMOD (linked into
// the game) hands out blocks from a bitmap; sub_826686F8(pool, first_block, used,
// count) marks blocks used (1) or free (0). Block size at pool+352, base at pool+4.
#include "game.h"
#if !GAME_WET
#include "kernel/kernel.h"
#include "kernel/threads.h"
#include "watch.h"

using namespace wet;

extern "C" void __imp__sub_826686F8(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_826686F8)
{
    uint32_t pool = ctx.r3.u32, first = ctx.r4.u32, used = ctx.r5.u32, count = ctx.r6.u32;
    uint32_t blockSize = Ld32(pool + 352), poolBase = Ld32(pool + 4);
    uint32_t addr = poolBase + first * blockSize, len = count * blockSize;
    if (watch::Overlaps(addr, len))
    {
        GuestThread* t = CurrentThread();
        Log("FMOD pool 0x%08X: blocks %u+%u (0x%08X, 0x%X bytes) marked %s by thread %u lr=%08X%s", pool, first,
            count, addr, len, used ? "used" : "free", t ? t->id : 0, uint32_t(ctx.lr), CurrentGuestStack().c_str());
    }
    __imp__sub_826686F8(ctx, base);
}

// FMOD's allocator (pool, size, ...): logs results in the watched range.
extern "C" void __imp__sub_82668C28(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_82668C28)
{
    uint32_t pool = ctx.r3.u32, size = ctx.r4.u32, lr = uint32_t(ctx.lr);
    bool callback = Ld32(pool + 336) != 0;
    __imp__sub_82668C28(ctx, base);
    if (watch::Overlaps(ctx.r3.u32, size ? size : 1))
    {
        GuestThread* t = CurrentThread();
        Log("FMOD alloc pool 0x%08X size 0x%X -> 0x%08X%s (callbacks alloc %08X realloc %08X free %08X), thread %u lr=%08X%s",
            pool, size, ctx.r3.u32, callback ? " (user callback)" : "", Ld32(pool + 336), Ld32(pool + 340),
            Ld32(pool + 344), t ? t->id : 0, lr, CurrentGuestStack().c_str());
    }
}

// FMOD's free (pool, pointer, ...).
extern "C" void __imp__sub_82668D88(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_82668D88)
{
    uint32_t pool = ctx.r3.u32, ptr = ctx.r4.u32;
    if (watch::Overlaps(ptr, 16))
    {
        GuestThread* t = CurrentThread();
        Log("FMOD free pool 0x%08X 0x%08X (header size 0x%X), thread %u lr=%08X%s", pool, ptr, Ld32(ptr - 16),
            t ? t->id : 0, uint32_t(ctx.lr), CurrentGuestStack().c_str());
    }
    __imp__sub_82668D88(ctx, base);
}
#endif
