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
#include "log.h"
#include "options.h"
#include "ppc_context.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
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

    std::atomic<bool> s_ownStepping{ std::getenv("WET_PHYSICS_60") == nullptr };
    bool OwnStepping() { return s_ownStepping.load(std::memory_order_relaxed); }

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

// Diagnostics: how fast the AI characters really move. sub_821F3768 is one of
// the per-character AI updates (r3: the AI object; +200: its entity, whose
// position is at +352). Once a second, each character's speed over that second.
extern "C" void __imp__sub_821F3768(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_821F3768)
{
    uint32_t ai = ctx.r3.u32;
    __imp__sub_821F3768(ctx, base);
    static const bool on = std::getenv("WET_NO_AI_SPEED_LOG") == nullptr;
    if (!on || !ai)
        return;
    uint32_t entity = Ld32(ai + 200);
    if (entity < 0x40000000u || entity >= 0xC0000000u)
        return;
    struct Track { float x, y, z; double since; float path; uint32_t updates; };
    static std::mutex lock;
    static std::unordered_map<uint32_t, Track> tracks;
    static uint32_t lines = 0;
    float x = LdF(entity + 352), y = LdF(entity + 356), z = LdF(entity + 360);
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    std::lock_guard l(lock);
    auto it = tracks.find(ai);
    if (it == tracks.end())
    {
        if (tracks.size() > 256)
            tracks.clear();
        tracks[ai] = { x, y, z, now, 0.0f, 0 };
        return;
    }
    Track& t = it->second;
    float step = std::sqrt((x - t.x) * (x - t.x) + (y - t.y) * (y - t.y) + (z - t.z) * (z - t.z));
    if (step < 50.0f)   // (not a teleport)
        t.path += step;
    t.x = x; t.y = y; t.z = z;
    t.updates++;
    if (now - t.since >= 1.0)
    {
        if (t.path > 0.05f && lines < 4000)
        {
            lines++;
            Log("AI %08X: moved %.2f in %.2f s (%u updates), %d FPS, frame %.2f ms, physics %s", ai, t.path,
                now - t.since, t.updates, wet::options::Fps(), LdF(kGameDt) * 1000.0f,
                OwnStepping() ? "ours" : "game's");
        }
        t.since = now;
        t.path = 0.0f;
        t.updates = 0;
    }
}

// Midasm hook (config/Conan_config.toml): after the AI has measured a
// character's speed from one frame's movement (r31: the AI object, f: the
// speed), the speed over the last 1/30 s or more - what the game saw at its own
// 30 FPS - if that's higher. WET_AI_SPEED_PER_FRAME=1 leaves the game's.
void ConanAiSpeedHook(PPCRegister& r31, PPCRegister& f)
{
    static const bool off = std::getenv("WET_AI_SPEED_PER_FRAME") != nullptr;
    uint32_t ai = r31.u32;
    if (off || !ai)
        return;
    uint32_t entity = Ld32(ai + 200);
    if (entity < 0x40000000u || entity >= 0xC0000000u)
        return;
    struct Sample { double t; float x, y, z; };
    struct History { Sample s[16]; uint32_t count = 0, next = 0, frame = ~0u; };
    static std::mutex lock;
    static std::unordered_map<uint32_t, History> histories;
    float x = LdF(entity + 352), y = LdF(entity + 356), z = LdF(entity + 360);
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    uint32_t frame = Ld32(kTimer + 40);
    std::lock_guard l(lock);
    if (histories.size() > 512)
        histories.clear();
    History& h = histories[ai];
    // The newest sample at least 1/30 s old.
    const Sample* old = nullptr;
    for (uint32_t i = 0; i < h.count; i++)
    {
        const Sample& s = h.s[(h.next + 16 - 1 - i) % 16];
        if (now - s.t >= 1.0 / 30.0)
        {
            old = &s;
            break;
        }
    }
    if (old && now - old->t < 0.25)
    {
        float dx = x - old->x, dy = y - old->y, dz = z - old->z;
        double speed = std::sqrt(dx * dx + dy * dy + dz * dz) / (now - old->t);
        if (speed > f.f64 && speed < 100.0)
            f.f64 = speed;
    }
    if (frame != h.frame)
    {
        h.frame = frame;
        h.s[h.next] = { now, x, y, z };
        h.next = (h.next + 1) % 16;
        if (h.count < 16)
            h.count++;
    }
}

void ConanTogglePhysicsStepping()
{
    bool own = !s_ownStepping.load();
    s_ownStepping = own;
    Log("Physics stepping: %s (F3)", own ? "ours (a frame's own length above 60 FPS)" : "the game's (whole 1/60 s steps)");
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
#else
void ConanTogglePhysicsStepping() {}
#endif
