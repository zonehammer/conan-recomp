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
#include "kernel/threads.h"
#include "log.h"
#include "options.h"
#include "ppc_context.h"

#include <atomic>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>
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
    double raw = f.f64;
    if (old && now - old->t < 0.25)
    {
        float dx = x - old->x, dy = y - old->y, dz = z - old->z;
        double speed = std::sqrt(dx * dx + dy * dy + dz * dz) / (now - old->t);
        if (speed > f.f64 && speed < 100.0)
            f.f64 = speed;
    }
    // Diagnostics, once a second per character: the game's own measurement,
    // ours, how often each was under the idle / stuck limits, the straight-line
    // distance covered, and the AI's movement state (+1012).
    {
        struct Stats { double since = 0; float sx, sy, sz; uint32_t n = 0, rawIdle = 0, rawStuck = 0, ourIdle = 0, ourStuck = 0;
                       double rawSum = 0, ourSum = 0; uint32_t states[8] = {}; };
        static std::unordered_map<uint32_t, Stats> stats;
        static uint32_t lines = 0;
        Stats& st = stats[ai];
        if (st.since == 0)
        {
            st.since = now; st.sx = x; st.sy = y; st.sz = z;
        }
        st.n++;
        st.rawSum += raw; st.ourSum += f.f64;
        st.rawIdle += raw < 0.01; st.rawStuck += raw <= 0.2;
        st.ourIdle += f.f64 < 0.01; st.ourStuck += f.f64 <= 0.2;
        st.states[std::min<uint32_t>(Ld32(ai + 1012) >> 24, 7)]++;
        if (now - st.since >= 1.0)
        {
            float dx = x - st.sx, dy = y - st.sy, dz = z - st.sz;
            if (lines < 3000 && (st.rawSum > 0 || std::sqrt(dx * dx + dy * dy + dz * dz) > 0.05f))
            {
                lines++;
                Log("AI %08X speed: %u checks, game's %.2f avg (idle %u, stuck %u), ours %.2f (idle %u, stuck %u), "
                    "straight %.2f, %d FPS, state 0:%u 1:%u 2:%u 3:%u 4+:%u",
                    ai, st.n, st.rawSum / st.n, st.rawIdle, st.rawStuck, st.ourSum / st.n, st.ourIdle, st.ourStuck,
                    std::sqrt(dx * dx + dy * dy + dz * dz), wet::options::Fps(), st.states[0], st.states[1],
                    st.states[2], st.states[3], st.states[4] + st.states[5] + st.states[6] + st.states[7]);
            }
            st = Stats{};
        }
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

// Diagnostics: who stops the AI characters. sub_8225EA10 sets a character's
// movement to zero (stop); sub_8225E3D8 sets its movement (v1). Once a second:
// how many stops and moves, and the stops by the code that asked (its return
// address), with a guest call stack the first time each caller is seen.
namespace
{
    std::mutex s_stopLock;
    std::unordered_map<uint32_t, uint32_t> s_stopsBy;
    uint32_t s_stops = 0, s_moves = 0, s_zeroMoves = 0, s_stopLines = 0;
    double s_stopSince = 0;

    void StopReport(double now)
    {
        if (s_stopSince == 0)
            s_stopSince = now;
        if (now - s_stopSince < 1.0)
            return;
        if ((s_stops || s_moves) && s_stopLines < 1500)
        {
            s_stopLines++;
            std::vector<std::pair<uint32_t, uint32_t>> by(s_stopsBy.begin(), s_stopsBy.end());
            std::sort(by.begin(), by.end(), [](auto& a, auto& b) { return a.second > b.second; });
            std::string list;
            for (size_t i = 0; i < by.size() && i < 8; i++)
            {
                char b[32];
                std::snprintf(b, sizeof(b), " %08X:%u", by[i].first, by[i].second);
                list += b;
            }
            Log("AI movement in %.2f s at %d FPS: %u moves (%u of them zero), %u stops; stops by%s", now - s_stopSince,
                wet::options::Fps(), s_moves, s_zeroMoves, s_stops, list.c_str());
        }
        s_stopsBy.clear();
        s_stops = s_moves = s_zeroMoves = 0;
        s_stopSince = now;
    }
}

// ---- The AI at the game's own 30 updates a second ----
// The enemies' AI (sub_821FC9E0, one AI controller - r3 - per character) was
// made for 30 FPS and goes wrong at higher frame rates in more than one place
// (move commands with no speed, stop-start chasing). Above 30 FPS each AI
// controller now runs once per 1/30 s of game time, seeing the time since its
// last update as the frame time; on the frames in between its character keeps
// the last movement it was given. WET_AI_EVERY_FRAME=1: every frame, as before.
namespace
{
    struct LastMove { PPCVRegister v; bool set = false; };
    std::mutex s_moveLock;
    std::unordered_map<uint32_t, LastMove> s_lastMove;   // by character (entity)
    thread_local bool t_replaying = false;

    void RememberMove(uint32_t entity, const PPCVRegister& v)
    {
        if (t_replaying)
            return;
        std::lock_guard l(s_moveLock);
        if (s_lastMove.size() > 1024)
            s_lastMove.clear();
        s_lastMove[entity] = { v, true };
    }
}

extern "C" void __imp__sub_8225E3D8(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__sub_821FC9E0(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_821FC9E0)
{
    static const bool everyFrame = std::getenv("WET_AI_EVERY_FRAME") != nullptr;
    uint32_t ai = ctx.r3.u32;
    float dt = LdF(kGameDt);
    if (everyFrame || !ai || !(dt > 0.0f) || dt >= 1.0f / 31.0f)
    {
        __imp__sub_821FC9E0(ctx, base);
        return;
    }
    static std::mutex lock;
    static std::unordered_map<uint32_t, float> pending;   // game time since the last update
    float acc;
    {
        std::lock_guard l(lock);
        if (pending.size() > 1024)
            pending.clear();
        auto it = pending.find(ai);
        acc = (it == pending.end() ? 1.0f / 30.0f : it->second + dt);
        if (acc < 1.0f / 30.0f - 0.0005f)
        {
            pending[ai] = acc;
            acc = -1.0f;
        }
        else
            pending[ai] = 0.0f;
    }
    if (acc < 0.0f)
    {
        // In between: the character keeps its last movement.
        uint32_t entity = Ld32(ai + 200);
        LastMove last;
        {
            std::lock_guard l(s_moveLock);
            auto it = s_lastMove.find(entity);
            if (it != s_lastMove.end())
                last = it->second;
        }
        if (last.set && entity)
        {
            PPCContext saved = ctx;
            ctx.r3.u64 = entity;
            ctx.v1 = last.v;
            t_replaying = true;
            __imp__sub_8225E3D8(ctx, base);
            t_replaying = false;
            ctx = saved;
        }
        return;
    }
    // The update, with the time since the last one as this frame's time.
    uint32_t ms = std::max<uint32_t>(1, uint32_t(acc * 1000.0f + 0.5f));
    uint32_t saved52 = Ld32(kTimer + 52), saved344 = Ld32(kTimer + 344);
    float saved56 = LdF(kTimer + 56), saved60 = LdF(kTimer + 60), saved348 = LdF(kGameDt), saved352 = LdF(kTimer + 352);
    St32(kTimer + 52, ms);
    St32(kTimer + 344, ms);
    StF(kTimer + 56, acc);
    StF(kTimer + 60, 1.0f / acc);
    StF(kGameDt, acc);
    StF(kTimer + 352, 1.0f / acc);
    __imp__sub_821FC9E0(ctx, base);
    St32(kTimer + 52, saved52);
    St32(kTimer + 344, saved344);
    StF(kTimer + 56, saved56);
    StF(kTimer + 60, saved60);
    StF(kGameDt, saved348);
    StF(kTimer + 352, saved352);
}

extern "C" void __imp__sub_8225EA10(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_8225EA10)
{
    uint32_t lr = uint32_t(ctx.lr);
    {
        double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        std::lock_guard l(s_stopLock);
        StopReport(now);
        s_stops++;
        {
            PPCVRegister zero{};
            RememberMove(ctx.r3.u32, zero);
        }
        if (s_stopsBy[lr]++ == 0 && s_stopsBy.size() < 64)
        {
            static std::unordered_map<uint32_t, bool> stacked;
            if (!stacked[lr] && stacked.size() < 80)
            {
                stacked[lr] = true;
                Log("AI stop from %08X%s", lr, CurrentGuestStack().c_str());
            }
        }
    }
    __imp__sub_8225EA10(ctx, base);
}

PPC_FUNC(sub_8225E3D8)
{
    {
        double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        std::lock_guard l(s_stopLock);
        StopReport(now);
        s_moves++;
        RememberMove(ctx.r3.u32, ctx.v1);
        if (ctx.v1.f32[0] == 0.0f && ctx.v1.f32[1] == 0.0f && ctx.v1.f32[2] == 0.0f && ctx.v1.f32[3] == 0.0f)
            s_zeroMoves++;
    }
    __imp__sub_8225E3D8(ctx, base);
}

// Midasm hook (config/Conan_config.toml), in the move-to action after the
// path's step (f27) and the 0.1 limit it's compared with (f0) are loaded: the
// limit scaled to the frame time. WET_PATH_STEP_GAME=1 keeps the game's.
void ConanPathStepHook(PPCRegister& f27, PPCRegister& f0)
{
    static const bool off = std::getenv("WET_PATH_STEP_GAME") != nullptr;
    float dt = LdF(kGameDt);
    double limit = f0.f64;
    if (!off && dt > 0.0f && dt < 1.0f / 30.0f)
        f0.f64 = limit * double(dt) * 30.0;
    // Diagnostics: the step against the limits, once a second.
    static std::mutex lock;
    static double since = 0, sum = 0;
    static uint32_t n = 0, under = 0, underOurs = 0, lines = 0;
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    std::lock_guard l(lock);
    if (since == 0)
        since = now;
    n++;
    sum += f27.f64;
    under += f27.f64 < limit;
    underOurs += f27.f64 < f0.f64;
    if (now - since >= 1.0)
    {
        if (lines++ < 1500)
            Log("AI path step: %u checks, %.3f avg, under %.2f: %u, under %.3f (ours): %u, %d FPS", n, sum / n, limit, under,
                f0.f64, underOurs, wet::options::Fps());
        since = now;
        sum = 0;
        n = under = underOurs = 0;
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
