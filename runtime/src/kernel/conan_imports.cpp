// Imports first needed by Conan. Behaviour follows Xenia (kernel/xam, xboxkrnl).
#include "kernel.h"
#include "os.h"

using namespace wet;

// XGetVideoMode(X_VIDEO_MODE*): 1280x720 progressive widescreen HD, 60 Hz.
GUEST_IMPORT(XGetVideoMode)
{
    uint32_t m = Arg(ctx, 0);
    Zero(m, 0x30);
    St32(m + 0x00, 1280);
    St32(m + 0x04, 720);
    St32(m + 0x08, 0);          // interlaced
    St32(m + 0x0C, 1);          // widescreen
    St32(m + 0x10, 1);          // high definition
    float hz = 60.0f;
    uint32_t bits;
    std::memcpy(&bits, &hz, 4);
    St32(m + 0x14, bits);
    St32(m + 0x18, 1);          // video standard: NTSC
    St32(m + 0x1C, 0x4A);
    St32(m + 0x20, 0x01);
}

// 6 is VGA; titles treat anything other than 3/4/6/8 as PAL.
GUEST_IMPORT(XGetAVPack) { Ret(ctx, 6); }

GUEST_IMPORT(XAudioGetVoiceCategoryVolumeChangeMask)
{
    if (Arg(ctx, 1))
        St32(Arg(ctx, 1), 0);
    Ret(ctx, ERROR_SUCCESS);
}

// XamContentGetDeviceData(device_id, X_CONTENT_DEVICE_DATA*)
GUEST_IMPORT(XamContentGetDeviceData)
{
    uint32_t d = Arg(ctx, 1);
    if (Arg(ctx, 0) != 1)
        return Ret(ctx, ERROR_DEVICE_NOT_CONNECTED);
    Zero(d, 0x50);
    St32(d + 0x00, 1);                       // device id
    St32(d + 0x04, 1);                       // hard drive
    St64(d + 0x08, 20ull << 30);             // total bytes
    St64(d + 0x10, 16ull << 30);             // free bytes
    const char* name = "Hard Drive";
    for (int i = 0; name[i]; i++)
        St16(d + 0x18 + i * 2, uint16_t(name[i]));
    Ret(ctx, ERROR_SUCCESS);
}

// Sessions (online play): a dummy handle and object, as Xenia does.
GUEST_IMPORT(XamSessionCreateHandle)
{
    St32(Arg(ctx, 0), 0xCAFEDEAD);
    Ret(ctx, ERROR_SUCCESS);
}
GUEST_IMPORT(XamSessionRefObjByHandle)
{
    St32(Arg(ctx, 1), 0xDEADF00D);
    Ret(ctx, ERROR_SUCCESS);
}

// Leaderboards and gamer cards need Xbox Live.
GUEST_IMPORT(XamUserCreateStatsEnumerator) { Ret(ctx, ERROR_FUNCTION_FAILED); }
GUEST_IMPORT(XamShowGamerCardUIForXUID) { Ret(ctx, ERROR_FUNCTION_FAILED); }

GUEST_IMPORT(XamShowDirtyDiscErrorUI)
{
    Log("XamShowDirtyDiscErrorUI: the game could not read a file from the disc; check game\\extracted");
    LogFlush();
    os::ShowFatalMessage("Conan Recomp stopped: the game could not read one of its files.\n\n"
                         "Check that the game was extracted completely (see conan.log).");
    os::ExitProcess(1);
}

// Structured exception handler for C code: not used by recompiled code, which
// has no SEH unwinding. ExceptionContinueSearch.
GUEST_IMPORT(__C_specific_handler) { Ret(ctx, 1); }

// ---------------------------------------------------------------------------
// Network: no network (see misc.cpp).
// ---------------------------------------------------------------------------
constexpr uint32_t kSocketError = 0xFFFFFFFF;

GUEST_IMPORT(NetDll_closesocket) { Ret(ctx, 0); }
GUEST_IMPORT(NetDll_accept) { Ret(ctx, kSocketError); }
GUEST_IMPORT(NetDll_bind) { Ret(ctx, kSocketError); }
GUEST_IMPORT(NetDll_connect) { Ret(ctx, kSocketError); }
GUEST_IMPORT(NetDll_listen) { Ret(ctx, kSocketError); }
GUEST_IMPORT(NetDll_recv) { Ret(ctx, kSocketError); }
GUEST_IMPORT(NetDll_send) { Ret(ctx, kSocketError); }
GUEST_IMPORT(NetDll_select) { Ret(ctx, kSocketError); }
GUEST_IMPORT(NetDll_setsockopt) { Ret(ctx, kSocketError); }
GUEST_IMPORT(NetDll___WSAFDIsSet) { Ret(ctx, 0); }

// NetDll_XNetGetTitleXnAddr(caller, XNADDR*): loopback, as Xenia (a zero MAC
// breaks some network libraries).
GUEST_IMPORT(NetDll_XNetGetTitleXnAddr)
{
    uint32_t a = Arg(ctx, 1);
    Zero(a, 0x24);
    St32(a + 0, 0x7F000001);
    for (int i = 0; i < 6; i++)
        St8(a + 10 + i, 0xCC);
    Ret(ctx, 4); // XNET_GET_XNADDR_STATIC
}
