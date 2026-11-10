// Exercises the constructs the real generated code relies on.
#include "ppc_recomp_shared.h"

__attribute__((alias("__imp__sub_82510000"))) PPC_WEAK_FUNC(sub_82510000);
PPC_FUNC_IMPL(__imp__sub_82510000) {
	PPC_FUNC_PROLOGUE();
	uint32_t ea{};
	PPCRegister temp{};
	// mftb
	ctx.r3.u64 = PPC_QUERY_TIMEBASE();
	// mulhd
	ctx.r4.s64 = int64_t((__int128(ctx.r3.s64) * __int128(ctx.r5.s64)) >> 64);
	// stwu / lwz
	ea = -16 + ctx.r1.u32;
	PPC_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	ctx.r6.u64 = PPC_LOAD_U32(ctx.r1.u32 + 0);
	// vector op through simde
	simde_mm_store_ps(ctx.v1.f32, simde_mm_add_ps(simde_mm_load_ps(ctx.v2.f32), simde_mm_load_ps(ctx.v3.f32)));
	temp.f32 = PPCHalfToFloat(0x3C00);
	ctx.f1.f64 = double(temp.f32);
	// import call
	ctx.lr = 0x82510030;
	__imp__KeQueryPerformanceFrequency(ctx, base);
	// switch
	switch (ctx.r3.u64 & 1) {
	case 0:
		goto loc_82510038;
	case 1:
		goto loc_82510038;
	default:
		__builtin_unreachable();
	}
loc_82510038:
	ctx.r1.u32 = ctx.r1.u32 + 16;
	// bctrl (checked indirect call)
	ctx.ctr.u64 = 0x82510040;
	PPC_CALL_INDIRECT_FUNC(ctx.ctr.u32);
}

__attribute__((alias("__imp__sub_82510040"))) PPC_WEAK_FUNC(sub_82510040);
PPC_FUNC_IMPL(__imp__sub_82510040) {
	PPC_FUNC_PROLOGUE();
	ctx.r3.u64 = 0;
}

// Guest functions the runtime overrides (options.cpp) must exist.
PPC_FUNC_IMPL(__imp__sub_82C32BF8) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_83066340) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82CDDE00) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82BF2AA0) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82BF1CC0) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82B0ADD0) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82C7E590) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82B9AC80) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_828D5AE0) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_828D5BD0) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_828F1010) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_828F2248) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_828F35E0) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_824BDAC0) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_824BDEA0) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_824BDF58) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_824BE058) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_824BE120) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82546A30) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_826686F8) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82668C28) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82668D88) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_824F58F0) {
	PPC_FUNC_PROLOGUE();
}
// Conan: the frame timer and physics step (conan_time.cpp).
PPC_FUNC_IMPL(__imp__sub_8225D238) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82260078) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82261B30) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_82324810) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_824AF160) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_8257D8C8) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_821F3768) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_8225EA10) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_8225E3D8) {
	PPC_FUNC_PROLOGUE();
}
PPC_FUNC_IMPL(__imp__sub_821FC9E0) {
	PPC_FUNC_PROLOGUE();
}
