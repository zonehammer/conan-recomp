// Conan above 60 FPS: the game's clock and its physics step.
//
// 1. Physics. The game steps Havok in whole 1/60 s steps: sub_8225D238 rounds
//    the frame's time UP to a multiple of 1/60 s (the "physics step", with its
//    inverse, at 0x82A57168 / 0x82A5716C) and sub_82261B30 steps the world
//    1/60 s at a time until that's used up. Above 60 FPS every frame is shorter
//    than 1/60 s but still gets a whole step, so physics ran faster than the
//    rest of the game (twice as fast at 120 FPS) and what's moved through it
//    with a velocity worked out from the frame's own time - enemies walking -
//    no longer matched: they stood still. Frames shorter than 1/60 s now get a
//    physics step of their own length. At 60 FPS and below nothing changes.
//    WET_PHYSICS_60=1 keeps the game's own stepping.
//
// 2. Clock. The frame timer (sub_824AF160, timer object at 0x82B81298) keeps
//    time in whole milliseconds and drops the rest of every frame: 6.94 ms at
//    144 FPS counted as 6, so the game ran 14% slow (4% at 120). The counter it
//    reads (QueryPerformanceCounter, sub_8257D8C8) is given to it in whole
//    milliseconds instead, so what one frame drops the next one gets.
#include "game.h"
#include "kernel/kernel.h"
#include "ppc_context.h"

#include <cstdlib>
#include <cstring>

#if !GAME_WET
using namespace wet;

namespace
{
    constexpr uint32_t kTimer = 0x82B81298;          // the game's frame timer
    constexpr uint32_t kGameDt = kTimer + 348;       // this frame, seconds (game clock)
    constexpr uint32_t kPhysStep = 0x82A57168;       // physics step, seconds
    constexpr uint32_t kPhysStepInv = 0x82A5716C;    // 1 / physics step
    constexpr uint32_t kPhysRemaining = 0x82A57164;  // left to step this frame
    constexpr uint32_t kPhysWorld = 0x82A57158;      // the Havok world
    constexpr uint32_t kPhysStepping = 0x82A57174;   // set while stepping
    constexpr float kSixtieth = 0.01666f;            // the game's own constant

    float LdF(uint32_t a) { uint32_t u = Ld32(a); float f; std::memcpy(&f, &u, 4); return f; }
    void StF(uint32_t a, float f) { uint32_t u; std::memcpy(&u, &f, 4); St32(a, u); }

    bool OwnStepping()
    {
        static const bool off = std::getenv("WET_PHYSICS_60") != nullptr;
        return !off;
    }

    thread_local bool t_inFrameTimer = false;
}

extern "C" void __imp__sub_8225D238(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__sub_82260078(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__sub_82261B30(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__sub_82324810(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__sub_824AF160(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__sub_8257D8C8(PPCContext& ctx, uint8_t* base);

// The physics step for this frame (and its inverse).
PPC_FUNC(sub_8225D238)
{
    __imp__sub_8225D238(ctx, base);
    if (!OwnStepping())
        return;
    float dt = LdF(kGameDt);
    if (dt > 1.0e-6f && dt < kSixtieth)
    {
        StF(kPhysStep, dt);
        StF(kPhysStepInv, 1.0f / dt);
    }
}

// Steps the world by the physics step (r3: a float to use instead, or null).
PPC_FUNC(sub_82261B30)
{
    uint32_t given = ctx.r3.u32;
    if (!OwnStepping())
    {
        __imp__sub_82261B30(ctx, base);
        return;
    }
    float step;
    if (given)
        step = LdF(given);
    else
    {
        __imp__sub_82260078(ctx, base);
        step = float(ctx.f1.f64);
    }
    if (!(step > 1.0e-6f) || step >= kSixtieth)
    {
        ctx.r3.u64 = given;
        __imp__sub_82261B30(ctx, base);   // the game's own 1/60 s steps
        return;
    }
    // A frame shorter than 1/60 s: one step of its own length.
    St32(kPhysStepping, 1);
    StF(kPhysRemaining, step);
    ctx.r3.u64 = Ld32(kPhysWorld);
    ctx.f1.f64 = double(step);
    __imp__sub_82324810(ctx, base);
    StF(kPhysRemaining, 0.0f);
    St32(kPhysStepping, 0);
}

// The frame timer's update.
PPC_FUNC(sub_824AF160)
{
    t_inFrameTimer = true;
    __imp__sub_824AF160(ctx, base);
    t_inFrameTimer = false;
}

// QueryPerformanceCounter(r3: where the 64-bit count goes); 50 MHz.
PPC_FUNC(sub_8257D8C8)
{
    uint32_t out = ctx.r3.u32;
    __imp__sub_8257D8C8(ctx, base);
    if (!t_inFrameTimer || !out)
        return;
    // Whole milliseconds, each counted as 1.001 ms so the timer's own
    // conversion (float, rounded down) can't land just under a whole number.
    uint64_t count = (uint64_t(Ld32(out)) << 32) | Ld32(out + 4);
    count = (count / 50000) * 50050;
    St32(out, uint32_t(count >> 32));
    St32(out + 4, uint32_t(count));
}
#endif
