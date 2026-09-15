# Software-Rendered Build for Delta / HLE Emulators (v4)

Date: 2026-09-15
Status: approved design, awaiting spec review
Builds on: the v1-v3 collection. Nothing about the rdpq build changes.

## 1. Goal and finding

Produce a second ROM, `games-soft.z64`, that renders on high-level-emulation N64 emulators, chiefly Delta (iOS/iPad/macOS), which uses mupen64plus with an HLE RSP plugin and GLideN64.

Verified 2026-09-15 on a from-source mupen64plus-core (master) + GLideN64 rig:
- The HLE RSP plugin rejects libdragon's rdpq microcode ("unknown RSP code"), and the core segfaults, so the rdpq `games.z64` cannot render on Delta.
- A CPU-only libdragon framebuffer (via `graphics.h`, no rdpq, no RSP) renders correctly under GLideN64 with its default framebuffer emulation. This is the path.

The collection already routes every draw through the `draw_t` {rect, text} callbacks, so the software build is a new backend plus a changed boot sequence, behind a `SOFTRENDER` compile flag. Audio already bypasses the RSP; EEPROM is unaffected.

## 2. Scope

- In: a `SOFTRENDER` build of the existing adapter that draws with `graphics.h` into the framebuffer, a `make rom-soft` target producing `games-soft.z64`, a from-source HLE verification rig (mupen64plus-core master + GLideN64) with build and capture scripts, and a README section on Delta / iOS.
- Out: any change to the rdpq build, the games, the synth, the save format, or the tunes. No new fonts (the software build uses libdragon's built-in font). No runtime auto-detection; the two builds are separate ROMs.

## 3. Design

### 3.1 Software draw backend (`src/n64/app.c`, under `#ifdef SOFTRENDER`)

- Boot: `display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE)`; `graphics_set_default_font()`. No `rdpq_init`, no `dfs_init`, no font64 loading, no `rdpq_debug_start`.
- Frame: `surface_t *d = display_get();` then draw directly through the software `draw_t`; end with `display_show(d)`. No `rdpq_attach` / `rdpq_detach_show`.
- `soft_rect(ctx, x0, y0, x1, y1, rgb)`: clamp to the screen, then `graphics_draw_box(d, x0, y0, x1-x0, y1-y0, graphics_make_color(r,g,b,0xFF))`. The active surface is passed through `draw_t.ctx` each frame.
- `soft_text(ctx, font, align, x, y, rgb, s)`: the built-in font is fixed 8x8. Width = `strlen(s) * 8`. For `DRAW_LEFT` start x; `DRAW_CENTER` x - width/2; `DRAW_RIGHT` x - width. The `draw_t` contract puts y at the baseline, so draw with top-left y - 7. `graphics_set_color(graphics_make_color(r,g,b,0xFF), graphics_make_color(0xE8,0xE4,0xDC,0xFF))` (text color on the background color, since all text sits on the `COLOR_BG` field), then `graphics_draw_text(d, tx, y-7, s)`. Both `DRAW_FONT_HUD` and `DRAW_FONT_BIG` use the same 8x8 font; big overlays are therefore smaller than on the rdpq build, an accepted downgrade for the compatibility ROM.
- The tick loop, input mapping, audio filling, EEPROM, and app-state calls are shared with the rdpq build unchanged; only the draw backend and the boot/frame calls differ by `#ifdef`.

### 3.2 Build

- `make rom-soft`: `docker run ... make rom-in-container ROMNAME=games-soft BUILD_DIR=build/soft SOFTRENDER=1`. Inside the container, `ifeq ($(SOFTRENDER),1) N64_CFLAGS += -DSOFTRENDER` and the DFS/font rules are skipped (no `$(ROMNAME).dfs` dependency, no mkfont), because the software build embeds no assets.
- `make rom-soft-autoplay GAME=<name>`: adds `AUTOPLAY=1` and `-DAUTOPLAY_GAME`, as the rdpq autoplay build does, for unattended capture.

### 3.3 HLE verification rig

Delta's stack is not ares, so verification uses mupen64plus-core (master, which has the 2025 RDRAM-init fix) plus GLideN64, both built from source.

- `scripts/build-hle-rig.sh`: clones `mupen64plus/mupen64plus-core` and `gonetz/GLideN64` (shallow) into `build/hle/`, builds the core (`make all -j OSD=0` in `projects/unix`, needs `pkg-config`/`pkgconf` on PATH) and GLideN64 (`cmake ../src -DMUPENPLUSAPI=On -DNOHQ=On -DVEC4_OPT=On -DCRC_OPT=On -DCMAKE_BUILD_TYPE=Release` then `make -j`), and stages the data files (`GLideN64.ini`, `GLideN64.custom.ini`, and Homebrew's mupen64plus share files) into `build/hle/data/`. Idempotent: skips a build whose dylib already exists.
- `scripts/hle-shot.sh <rom> <outdir> <sec>...`: runs the ROM under `build/hle/` (core, GLideN64, dummy RSP, dummy audio) at normal speed, capturing the mupen64plus window at each given second with `screencapture -l`, then quits. Mirrors `scripts/ares-shot.sh`, including the defocus-safe capture and the fix that closes the emulator on a failed capture.
- `make hle-shots GAME=<name>`: builds `games-soft-autoplay.z64` and captures it under the rig at 4, 8, 15 s.

### 3.4 Delta caveat

Delta 1.7.6 crashed on the user's macOS 27 beta with an AppKit `VideoManager` selector exception before loading any ROM. That is a Delta + OS-beta bug, independent of the ROM, and cannot be fixed from our side. The software ROM should be tested on a non-beta iPad or iPhone. The README states this.

## 4. Tasks

1. Software draw backend and `SOFTRENDER` build, `make rom-soft` / `rom-soft-autoplay` (no DFS), the two rig scripts and `make hle-shots`. Verify: `games-soft.z64` builds; the menu and Brick render under GLideN64 via the rig.
2. Capture all nine games' software autoplay under the rig as evidence; README "Delta and iOS" section (how to build `games-soft.z64`, that it needs GLideN64's framebuffer emulation which is Delta's default, and the macOS-beta caveat); tag v4.0.

## 5. Risks

- **Built-in font legibility**: single 8x8 size for both HUD and overlays; accepted. A blitted Inter sheet is a later option.
- **Text over non-background pixels**: the built-in font draws an opaque background cell; all text in the collection sits on `COLOR_BG`, so a `COLOR_BG` back color is invisible. If any game later draws text over gameplay, this would show a box.
- **Rig reproducibility**: the scripts pin nothing; if an upstream build breaks, the rig is developer tooling only and does not affect the shipped ROMs.
