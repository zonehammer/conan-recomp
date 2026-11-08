# Conan Recompiled

A work-in-progress native Windows port of **Conan** (Xbox 360, 2007) made by **static
recompilation**: the game's PowerPC code is translated to C++ and compiled for x64, then
run on a small runtime that plays the part of the console. Graphics and audio use
[Xenia](https://github.com/xenia-project/xenia)'s GPU and audio emulation; the
recompiler is [XenonRecomp](https://github.com/hedge-dev/XenonRecomp).

It is built on the runtime, launcher and tools of
[WET Recompiled](https://github.com/zonehammer/wet-recomp).

**This repository contains no game data.** You need your own copy of Conan (title ID
`545107DA`). The build extracts your disc and generates the C++ on your machine;
nothing derived from the game is ever committed.

## Status

**Early work in progress: not playable yet.** The game's code recompiles cleanly; getting
it to boot to the main menu is the current step. The WET-specific features (mouse look,
photo mode, FOV, 60 FPS hooks and so on) are compiled out until Conan's own equivalents
are found.

## Building (Windows 10/11, 64-bit)

You need your Conan disc image (tested with *Conan (World) (En,Fr,De,Es,It)*).

1. Install [Python 3](https://www.python.org/downloads/) (tick "Add python.exe to PATH"),
   [Git for Windows](https://git-scm.com), CMake and Ninja (`pip install cmake ninja`) and
   the latest **llvm-mingw** release (`llvm-mingw-<date>-ucrt-x86_64.zip` from
   [github.com/mstorsjo/llvm-mingw/releases](https://github.com/mstorsjo/llvm-mingw/releases)),
   unzipped with its `bin` folder on your `PATH`.
2. Build:

   ```bat
   git clone --recursive https://github.com/zonehammer/conan-recomp.git
   cd conan-recomp
   python tools\build.py --iso "D:\path\to\Conan.iso"
   ```

   This extracts the disc into `game\extracted`, recompiles `default.xex` into
   `generated\ppc` and builds everything.
3. Run `build\runtime\ConanLauncher.exe` (settings, then Play) or
   `build\runtime\ConanRecomp.exe`. Problems: include `conan.log` (next to the exe).

## How it works

1. `tools/prepare_game.py` extracts the disc into `game/extracted/`.
2. A patched XenonRecomp translates `default.xex` into C++ files in `generated/ppc/`,
   guided by `config/Conan_config.toml` and `config/Conan_switch_tables.toml`
   (`tools/fix_switch_bounds.py` sizes functions whose jump tables run past the
   analyser's estimate).
3. `runtime/` provides what the console did: guest memory, threads and synchronisation,
   files, input, achievements and the kernel/XAM functions.
4. The GPU commands the game submits run through Xenia's command processor and shader
   translator; XMA audio is decoded by Xenia's decoder and played with XAudio2.

## Credits

- [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) (hedge-dev): the recompiler.
- [Xenia](https://github.com/xenia-project/xenia): GPU and audio emulation, and the
  reference for the console's behaviour.
- [SDL2](https://www.libsdl.org/), [Dear ImGui](https://github.com/ocornut/imgui),
  [FFmpeg](https://ffmpeg.org/), [NVIDIA Image Scaling](https://github.com/NVIDIAGameWorks/NVIDIAImageScaling).

## Legal

Conan Recompiled's own code is released under the [MIT licence](LICENSE); the third-party
projects keep their own licences. This project contains no game code or assets and is
intended for people who own a legitimate copy of Conan. It is not affiliated with or
endorsed by THQ Nordic, Nihilistic Software or Conan Properties International. Conan is a
trademark of its respective owner.
