// Which game this runtime is built for. The WET-specific hooks (guest
// functions overridden by address, WET's globals) are kept for reference while
// Conan's own equivalents are found, but compiled out: an override at a WET
// address would replace an unrelated Conan function.
#pragma once
#define GAME_WET 0
