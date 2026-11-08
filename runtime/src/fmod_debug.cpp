// Conan: FMOD's memory pool, logged around the --watch range. FMOD (linked into
// the game) hands out blocks from a bitmap; sub_826686F8(pool, first_block, used,
// count) marks blocks used (1) or free (0). Block size at pool+352, base at pool+4.
#include "game.h"
#if !GAME_WET
#include "kernel/kernel.h"
#include "kernel/threads.h"
#include "watch.h"
#include <atomic>
#include <cstdio>
#include <string>

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
    if (watch::Overlaps(ptr - 16, Ld32(ptr - 16) + 16))
    {
        GuestThread* t = CurrentThread();
        Log("FMOD free pool 0x%08X 0x%08X (header size 0x%X), thread %u lr=%08X%s", pool, ptr, Ld32(ptr - 16),
            t ? t->id : 0, uint32_t(ctx.lr), CurrentGuestStack().c_str());
    }
    __imp__sub_82668D88(ctx, base);
}

// The game's heap allocator (heap, size): heap fields: +228 bitmap, +232 base,
// +236 block size, +240 first-free hint, +244 block count.
extern "C" void __imp__sub_824BDF58(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_824BDF58)
{
    uint32_t heap = ctx.r3.u32, size = ctx.r4.u32, lr = uint32_t(ctx.lr);
    __imp__sub_824BDF58(ctx, base);
    uint32_t r = ctx.r3.u32;
    if (!r)
    {
        static int failures = 0;
        if (failures++ < 20)
        {
            GuestThread* t = CurrentThread();
            Log("HEAP 0x%08X alloc 0x%X FAILED (%u bytes in use of %u), thread %u lr=%08X%s", heap, size,
                Ld32(heap + 256), Ld32(heap + 248), t ? t->id : 0, lr, CurrentGuestStack().c_str());
        }
    }
    if (r && watch::Overlaps(r, size))
    {
        uint32_t bmp = Ld32(heap + 228), hb = Ld32(heap + 232), bs = Ld32(heap + 236);
        uint32_t idx = bs ? (r - hb) / bs : 0;
        GuestThread* t = CurrentThread();
        Log("HEAP 0x%08X alloc 0x%X -> 0x%08X (heap base 0x%08X, block 0x%X, %u blocks, bitmap 0x%08X, hint %u; "
            "header count %u index %d class %d; bit %u = %u), thread %u lr=%08X%s",
            heap, size, r, hb, bs, Ld32(heap + 244), bmp, Ld32(heap + 240), Ld32(r + 4), int32_t(Ld32(r + 8)),
            int32_t(Ld32(r + 12)), idx, bmp ? (Ld8(bmp + idx / 8) >> (idx & 7)) & 1 : 0, t ? t->id : 0, lr,
            CurrentGuestStack().c_str());
        // The bitmap bytes covering the block (bit n = byte n/8, bit n%8).
        uint32_t first = idx / 8, last = (idx + Ld32(r + 4) + 7) / 8;
        std::string bytes;
        for (uint32_t b = first; b <= last && b < first + 40; b++)
        {
            char x[4];
            std::snprintf(x, sizeof(x), "%02X", Ld8(bmp + b));
            bytes += x;
        }
        Log("HEAP   bitmap bytes %u..%u (blocks %u..): %s", first, last, first * 8, bytes.c_str());
    }
}

// Heap free (heap, block).
extern "C" void __imp__sub_824BE058(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_824BE058)
{
    uint32_t heap = ctx.r3.u32, blk = ctx.r4.u32;
    int32_t cls = int32_t(Ld32(blk + 12));
    uint32_t extent = cls < 0 ? Ld32(blk + 4) * Ld32(heap + 236) : 16;
    if (watch::Overlaps(blk, extent ? extent : 16))
    {
        GuestThread* t = CurrentThread();
        Log("HEAP 0x%08X free 0x%08X (count %u index %d class %d), thread %u lr=%08X%s", heap, blk, Ld32(blk + 4),
            int32_t(Ld32(blk + 8)), int32_t(Ld32(blk + 12)), t ? t->id : 0, uint32_t(ctx.lr), CurrentGuestStack().c_str());
    }
    __imp__sub_824BE058(ctx, base);
}

// Heap realloc (heap, block, size).
extern "C" void __imp__sub_824BE120(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_824BE120)
{
    uint32_t heap = ctx.r3.u32, blk = ctx.r4.u32, size = ctx.r5.u32, lr = uint32_t(ctx.lr);
    bool hit = blk && watch::Overlaps(blk, int32_t(Ld32(blk + 12)) < 0 ? Ld32(blk + 4) * Ld32(heap + 236) : 16);
    __imp__sub_824BE120(ctx, base);
    if (hit || (ctx.r3.u32 && watch::Overlaps(ctx.r3.u32, size)))
    {
        GuestThread* t = CurrentThread();
        Log("HEAP 0x%08X realloc 0x%08X to 0x%X -> 0x%08X, thread %u lr=%08X%s", heap, blk, size, ctx.r3.u32,
            t ? t->id : 0, lr, CurrentGuestStack().c_str());
    }
}

// Heap reset (heap): frees every block at once.
extern "C" void __imp__sub_824BDEA0(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_824BDEA0)
{
    uint32_t heap = ctx.r3.u32;
    GuestThread* t = CurrentThread();
    Log("HEAP 0x%08X reset (base 0x%08X, %u blocks of 0x%X, %u bytes in use), thread %u lr=%08X%s", heap,
        Ld32(heap + 232), Ld32(heap + 244), Ld32(heap + 236), Ld32(heap + 256), t ? t->id : 0, uint32_t(ctx.lr),
        CurrentGuestStack().c_str());
    __imp__sub_824BDEA0(ctx, base);
}

// Heap bitmap mark (heap, first_block, used, count): every bit change goes through here.
extern "C" void __imp__sub_824BDAC0(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_824BDAC0)
{
    uint32_t heap = ctx.r3.u32, first = ctx.r4.u32, used = ctx.r5.u32, count = ctx.r6.u32;
    uint32_t hb = Ld32(heap + 232), bs = Ld32(heap + 236);
    uint32_t addr = hb + first * bs, len = count * bs;
    uint32_t hintBefore = Ld32(heap + 240);
    __imp__sub_824BDAC0(ctx, base);
    if (watch::Overlaps(addr, len ? len : 1))
    {
        GuestThread* t = CurrentThread();
        Log("HEAP 0x%08X mark blocks %u+%u (0x%08X-0x%08X) %s, hint %u -> %u, thread %u lr=%08X%s", heap, first, count,
            addr, addr + len, used ? "USED" : "FREE", hintBefore, Ld32(heap + 240), t ? t->id : 0, uint32_t(ctx.lr),
            CurrentGuestStack().c_str());
    }
}
#endif
