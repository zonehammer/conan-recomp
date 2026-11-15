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
WIDE_HOOK(sub_824DF4D8)
WIDE_HOOK(sub_824DFCB8)
#endif
