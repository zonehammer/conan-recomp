// Conan: ultrawide (widescreen option, a screen or window wider than 16:9).
//
// The runtime shows the 16:9 frame stretched across the screen and renders
// more columns; the game's camera is given the screen's aspect so the view
// widens (Hor+) instead of stretching. The camera (fields: viewport size at
// +688/+690, aspect at +744) uses the float 16/9 at 0x821337C4 as its aspect
// when the console is in widescreen: when it's set up (sub_824DF218,
// sub_824DFCB8) and when it builds its projection (sub_824DF4D8, the width of
// the view = its height x that). Only during those calls the constant holds
// the screen's aspect instead.
#include "game.h"
#include "gpu/renderer.h"
#include "kernel/kernel.h"
#include "log.h"
#include "options.h"
#include "ppc_context.h"

#include <cmath>
#include <cstring>

#if GAME_CONAN
using namespace wet;

namespace
{
    constexpr uint32_t kAspect169 = 0x821337C4;

    struct WideAspect
    {
        uint32_t saved = 0;
        bool active = false;
        explicit WideAspect(uint8_t* base)
        {
            if (!options::Widescreen())
                return;
            renderer::NoteGameplayView();
            float aspect = renderer::DisplayAspect();
            if (aspect <= 16.0f / 9.0f + 0.01f)
                return;
            saved = PPC_LOAD_U32(kAspect169);
            uint32_t bits;
            std::memcpy(&bits, &aspect, sizeof bits);
            PPC_STORE_U32(kAspect169, bits);
            active = true;
            static int logged = 0;
            if (logged++ < 1)
                Log("Ultrawide: camera aspect %.3f", aspect);
        }
        void Restore(uint8_t* base)
        {
            if (active)
                PPC_STORE_U32(kAspect169, saved);
        }
    };

    struct WideFov
    {
        uint32_t camera = 0, saved = 0;
        WideFov(PPCContext& ctx, uint8_t* base)
        {
            int option = options::Fov();
            {
                // Diagnostics: what the projection builder sees (first calls,
                // then a count every few hundred).
                static int calls = 0;
                uint32_t raw = PPC_LOAD_U32(ctx.r3.u32 + 732);
                float f;
                std::memcpy(&f, &raw, sizeof f);
                if (calls < 8 || calls % 600 == 0)
                    Log("Field of view: projection #%d, camera %08X fov field %g (option %d)", calls, ctx.r3.u32, f,
                        option);
                calls++;
            }
            if (option == 90)
                return;
            camera = ctx.r3.u32;
            saved = PPC_LOAD_U32(camera + 732);
            float fov;
            std::memcpy(&fov, &saved, sizeof fov);
            if (!(fov > 0.01f && fov < 3.0f))
            {
                camera = 0;
                return;
            }
            double k = std::tan(option * 3.14159265358979 / 360.0);
            float wider = float(2.0 * std::atan(std::tan(fov * 0.5) * k));
            uint32_t bits;
            std::memcpy(&bits, &wider, sizeof bits);
            PPC_STORE_U32(camera + 732, bits);
            static int logged = 0;
            if (logged++ < 1)
                Log("Field of view: %d (camera %.1f deg -> %.1f deg)", option, fov * 57.29578, wider * 57.29578);
        }
        void Restore(uint8_t* base)
        {
            if (camera)
                PPC_STORE_U32(camera + 732, saved);
        }
    };
}

#define WIDE_HOOK(name)                                         \
    extern "C" void __imp__##name(PPCContext& ctx, uint8_t* base); \
    PPC_FUNC(name)                                              \
    {                                                           \
        WideAspect wide(base);                                  \
        __imp__##name(ctx, base);                               \
        wide.Restore(base);                                     \
    }

WIDE_HOOK(sub_824DF218)
WIDE_HOOK(sub_824DFCB8)

// The projection: wider aspect (above) and the field of view.
extern "C" void __imp__sub_824DF4D8(PPCContext& ctx, uint8_t* base);
PPC_FUNC(sub_824DF4D8)
{
    WideAspect wide(base);
    WideFov fov(ctx, base);
    __imp__sub_824DF4D8(ctx, base);
    fov.Restore(base);
    wide.Restore(base);
}
#endif
