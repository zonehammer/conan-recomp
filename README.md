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

**First release (0.1.0).** The game boots, plays its videos and is playable (tested through the first levels) with
sound, saves and achievements. Expect bugs; please include `conan.log` with any
report (the launcher's **Report a bug...** button zips the logs for you).

What's in it:

- Resolution 720p / 1440p / 4K, frame rate 30 up to 360 FPS (or your monitor's), V-Sync,
  borderless fullscreen and monitor choice.
- Anti-aliasing (FXAA, SMAA), AMD FSR / CAS and NVIDIA Image Scaling, 16x texture filtering,
  brightness / contrast / gamma / colour.
- Keyboard and mouse (rebindable), any controller through SDL, vibration.
- Button prompts for Xbox, PlayStation or keyboard / mouse (automatic by default).
- Achievements with pop-ups, save backups, shader cache.

Known issues: at 1440p and 4K a thin dark line can show down the middle of the screen.

## Installing (Windows 10/11, 64-bit)

You need your own Conan disc image (tested with *Conan (World) (En,Fr,De,Es,It)*), or the
folder of an already extracted disc, and about 12 GB of free space.

1. Download **ConanSetup.exe** from the
   [latest release](https://github.com/zonehammer/conan-recomp/releases/latest).
2. Run it, choose your disc image (or extracted folder) and a folder to install to, and press
   **Install**. It downloads the build tools it needs (Git, CMake, Ninja, Clang, Python),
   gets this source code, extracts your disc and builds the game on your PC. The first build
   takes a while (10-30 minutes).
3. Start **Conan Recompiled** from the desktop shortcut: the launcher has the settings,
   then press **Play**. In game, F1 opens the settings overlay.

The launcher's **Check for updates** / **Update now** gets and builds new releases; your
saves and settings stay.

## Building by hand

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
