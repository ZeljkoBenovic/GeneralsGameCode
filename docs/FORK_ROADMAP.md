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

Machine: i7-8700, 64 GB, RTX 4070 Ti SUPER, Windows 11, 2560x1440 windowed. Replay `12-11-35_2v2_babai_ILnur_HardAI_HardAI.rep` (55 min).

| Date | Build | Benchmark | Result |
|---|---|---|---|
| 17/10/2026 | Official release (VC6) | 10 replays headless, `-jobs 4` | 356 s wall, 0 errors |
| 17/10/2026 | `modern` e64c462f3+ VC6 (Online) | 10 replays headless, `-jobs 4` | 298 s wall, 0 errors |
| 17/10/2026 | `modern` VC6 (Online) | render, minute 20, 60 s (2 runs) | fps avg 665 / 886, 1% low 175 / 220, logic 2.16 / 2.17 ms avg, 9.79 / 9.68 ms p99, 24.7 ms max |
| 17/10/2026 | `modern` VC6 (Online) | full replay headless, `-jobs 1` (2 runs) | 134.6 s / 134.3 s |

Findings:
- Logic timings and headless simulation time repeat within 0.5 %. Render fps varied 33 % between runs and needs a more
  stable setup (fixed camera, fullscreen, nothing else on screen) before it is useful for renderer work.
- On this machine rendering is far from the limit (~900 fps late game). The game logic is: the 33 ms logic tick budget
  already sees 25 ms spikes in a 4 player match. Logic performance is the first target for "performance and scale".
- Logic optimizations that keep results bit identical are allowed in the Online build; verify with the replay check.
- Modern builds desync from retail replays at frame 110, so their replay workload diverges (the match can even end early).
  Use the Online build for workload-exact comparisons.
