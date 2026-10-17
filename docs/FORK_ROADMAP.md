# Fork roadmap

This fork builds on TheSuperHackers/GeneralsGameCode. `main` mirrors upstream; all fork work lives on `modern`
and is merged with upstream regularly. Priorities: **performance and scale**, then **graphics and display**.

## Two builds

| Build | Compiler | Preset | Online play | Purpose |
|---|---|---|---|---|
| Online | VC6 SP6 | `vc6` | With retail, upstream and GenTool players | Compatible build. Gets client-side improvements (rendering, display, UI) that do not touch the simulation. |
| Enhanced | VS2022 | `win32-enhanced` | Only with other Enhanced players | All performance work, and all upstream fixes held back by `RETAIL_COMPATIBLE_*` (~780 guarded sites). Path to 64-bit. |

Why two: only VC6 builds are CRC compatible with retail. Measured on 17/10/2026 with the upstream replay set
(`GeneralsReplays/GeneralsZH/1.04`, 10 replays): the official VC6 release matched all 10, a VS2022 build
mismatched all 10 at logic frame 110, and a VS2022 build with `/arch:IA32` (x87) still mismatched at frame 110-111.

Rule for shared code: anything that can run inside the game logic must stay CRC neutral in the Online build,
and must compile with VC6 (C++98).

## Phase 0: foundation (in progress)

- [x] Borderless window when the windowed resolution covers the monitor (works around exclusive fullscreen device loss, upstream #3049)
- [x] `win32-enhanced` and `win32-enhanced-profile` presets (`RTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME=OFF`)
- [x] `-frameStats <seconds>` in release builds: frame time average, p50, p99, max, 1% low FPS, game logic time per logic frame (`FrameStats.txt` in the user data folder)
- [ ] Benchmark scripts: rendered replay (`-replay X.rep -frameStats 60`) and headless simulation time (`-headless -replay`)
- [ ] Baseline numbers for the current code, recorded below
- [ ] Local VC6 toolchain for the Online build and replay compatibility checks
- [ ] Fork CI: build both flavors; replay check against the Online build

## Phase 1: graphics and display

- Port the renderer from Direct3D 8 to Direct3D 9Ex. 9Ex removes most device-lost handling, which is the root cause of the
  fullscreen hang, and it is available in 64-bit. Build on the upstream render backend work (`IRenderBackend`, `DX8Backend.cpp`).
- Proper exclusive fullscreen and borderless fullscreen on the 9Ex device; high refresh rates; correct GPU selection (upstream #880).
- UI scaling for 1440p and 4K.

## Phase 2: performance and scale (Enhanced build)

- Profile with Tracy (`win32-enhanced-profile`) on large replays, fix the top hotspots, measure every change with FrameStats.
- Known candidates from upstream: drawable region iteration (#1502), off-screen river culling (#3496), string handling (#3495, #1777).
- Adopt deterministic math (upstream PR #2670) so Enhanced builds stay in sync with each other across compilers and platforms.
- 64-bit: needs the D3D9 renderer (phase 1), an audio backend instead of Miles (32-bit only), and the FFmpeg video path instead of Bink.
- Multithreading of independent client work (particles, terrain, audio) after profiling shows where it pays off.

## Baselines

| Date | Build | Benchmark | Result |
|---|---|---|---|
| 17/10/2026 | Official release (VC6) | 10 replays headless, `-jobs 4` | 356 s wall, 0 errors |
