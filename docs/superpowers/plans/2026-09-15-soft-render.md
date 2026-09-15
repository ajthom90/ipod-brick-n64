# Software-Rendered Build Implementation Plan (v4)

> **For agentic workers:** Executed one task at a time by the Grok Build CLI, driven by Claude, who writes each task prompt, reviews the commit, and only then hands out the next. Keep every command under four minutes. Capture emulator windows only with the provided scripts.

**Goal:** Add a `games-soft.z64` build that renders on Delta / HLE emulators (mupen64plus + GLideN64), plus a from-source verification rig.

**Architecture:** A `SOFTRENDER` compile flag swaps the adapter's draw backend from rdpq to `graphics.h` (CPU framebuffer) and its boot/frame calls; everything else (games, app state, audio, EEPROM, tick loop) is shared and unchanged.

**Tech Stack:** as the collection, plus mupen64plus-core (master) and GLideN64 built from source for verification.

**Spec:** `docs/superpowers/specs/2026-09-15-soft-render-design.md`.

## Global Constraints

Same as the collection's plans: pure game/framework modules unchanged; only `src/n64/app.c`, `Makefile`, and `scripts/` change here. Exclusive rectangles, the shared palette, ares/HLE only (never mupen64plus with the HLE-RSP for the rdpq build), one commit per task, no pushes by the worker, no plan/spec edits. The rdpq build and all game logic must remain byte-for-byte behaviorally unchanged.

---

## Verified facts (rely on these; do not re-derive)

- A CPU-only libdragon framebuffer renders under GLideN64 with default settings; the HLE RSP rejects rdpq. So the software build must use `graphics.h` only, with no `rdpq_*` and no `dfs_*` calls.
- `graphics.h` API: `graphics_set_default_font()`, `graphics_make_color(r,g,b,a)`, `graphics_draw_box(surf,x,y,w,h,color)`, `graphics_set_color(fore,back)`, `graphics_draw_text(surf,x,y,msg)` (built-in font is fixed 8x8, top-left positioned). `surface_t *display_get()`, `display_show(surface_t*)`.
- The core build needs `pkg-config`/`pkgconf` on PATH (`brew install pkgconf` if missing) and builds with `make all -j8 OSD=0` in `mupen64plus-core/projects/unix`; the dylib lands at `projects/unix/libmupen64plus.dylib`.
- GLideN64 builds with `cmake ../src -DMUPENPLUSAPI=On -DNOHQ=On -DVEC4_OPT=On -DCRC_OPT=On -DCMAKE_BUILD_TYPE=Release` then `make -j8`; the dylib lands at `build/plugin/Release/mupen64plus-video-GLideN64.dylib`.
- Running a ROM: `mupen64plus --corelib <core.dylib> --emumode 0 --windowed --resolution 640x480 --noosd --configdir <cfg> --datadir <data> --gfx <GLideN64.dylib> --rsp dummy --audio dummy <rom>`. GLideN64 needs `GLideN64.ini` and `GLideN64.custom.ini` in the datadir; also copy Homebrew's `$(brew --prefix)/share/mupen64plus/*` there.
- Window capture: find the largest normal-layer window whose owner name contains "mupen" via `CGWindowListCopyWindowInfo` (osascript JavaScript), then `screencapture -x -o -l <id>`.

---

### Task 1: Software draw backend, rom-soft build, HLE rig

**Files:**
- Modify: `src/n64/app.c` (add the `#ifdef SOFTRENDER` backend and boot/frame branches), `Makefile` (`rom-soft`, `rom-soft-autoplay`, `hle-shots`, and the in-container `SOFTRENDER` branch skipping DFS)
- Create: `scripts/build-hle-rig.sh`, `scripts/hle-shot.sh`

**Interfaces:**
- Produces: `make rom-soft` → `games-soft.z64`; `make rom-soft-autoplay GAME=<name>` → `games-soft-autoplay.z64`; `scripts/build-hle-rig.sh`; `scripts/hle-shot.sh <rom> <outdir> <sec>...`; `make hle-shots GAME=<name>`.

- [ ] **Step 1: Adapter software backend**

In `src/n64/app.c`, add a software backend guarded by `#ifdef SOFTRENDER`. The rect and text callbacks:

```c
#ifdef SOFTRENDER
#include <string.h>
static uint32_t soft_col(uint32_t c) { return graphics_make_color((c>>16)&0xFF,(c>>8)&0xFF,c&0xFF,0xFF); }
static void soft_rect(void *ctx, int x0, int y0, int x1, int y1, uint32_t c) {
    surface_t *d = (surface_t*)ctx;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > SCREEN_W) x1 = SCREEN_W; if (y1 > SCREEN_H) y1 = SCREEN_H;
    if (x1 > x0 && y1 > y0) graphics_draw_box(d, x0, y0, x1 - x0, y1 - y0, soft_col(c));
}
static void soft_text(void *ctx, draw_font_t font, draw_align_t align, int x, int y, uint32_t c, const char *s) {
    (void)font;
    surface_t *d = (surface_t*)ctx;
    int w = (int)strlen(s) * 8;
    int tx = (align == DRAW_CENTER) ? x - w/2 : (align == DRAW_RIGHT) ? x - w : x;
    graphics_set_color(soft_col(c), soft_col(COLOR_BG));
    graphics_draw_text(d, tx, y - 7, s);
}
#endif
```

The `draw_t.ctx` must be set to the current surface each frame (see Step 2). Keep the existing rdpq `n64_rect`/`n64_text` for the non-SOFTRENDER build.

- [ ] **Step 2: Boot and frame branches**

In `main`, branch the boot and the per-frame draw on `SOFTRENDER`:

```c
#ifdef SOFTRENDER
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
    graphics_set_default_font();
#else
    dfs_init(DFS_DEFAULT_LOCATION);
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    ... (existing rdpq font loading) ...
#endif
```

Per frame:

```c
        surface_t *fb = display_get();
#ifdef SOFTRENDER
        draw_t soft_draw = { .ctx = fb, .rect = soft_rect, .text = soft_text };
        app_render(&app, &soft_draw);
        display_show(fb);
#else
        rdpq_attach(fb, NULL);
        rdpq_set_mode_fill(rgb(COLOR_BG));
        app_render(&app, &n64_draw);
        rdpq_detach_show();
#endif
```

Everything else in `main` (audio, input, EEPROM, tick loop) is shared.

- [ ] **Step 3: Makefile**

Add host targets:

```make
rom-soft:
	$(DOCKER_RUN) make rom-in-container ROMNAME=games-soft BUILD_DIR=build/soft SOFTRENDER=1
rom-soft-autoplay:
	$(DOCKER_RUN) make rom-in-container ROMNAME=games-soft-autoplay BUILD_DIR=build/soft-auto SOFTRENDER=1 AUTOPLAY=1 GAME=$(GAME)
hle-shots:
	scripts/build-hle-rig.sh
	$(MAKE) rom-soft-autoplay GAME=$(GAME)
	scripts/hle-shot.sh games-soft-autoplay.z64 build/hle-shots 4 8 15
```

In the `ifdef N64_INST` block: `ifeq ($(SOFTRENDER),1) N64_CFLAGS += -DSOFTRENDER endif`, and make the DFS dependency conditional so the software build has none:

```make
ifneq ($(SOFTRENDER),1)
$(BUILD_DIR)/$(ROMNAME).dfs: filesystem/hud.font64 filesystem/big.font64
$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs
endif
```

(Adjust to match the current Makefile's DFS rule names.) The `$(ROMNAME).elf: $(OBJS)` rule and everything else stay.

- [ ] **Step 4: `scripts/build-hle-rig.sh`**

Idempotent build of the rig into `build/hle/`. Verbatim intent:

```bash
#!/bin/bash
set -euo pipefail
ROOT="build/hle"; mkdir -p "$ROOT/data"
export PATH="$(brew --prefix)/bin:$PATH"
command -v pkg-config >/dev/null || brew install pkgconf
CORE="$ROOT/mupen64plus-core"
[ -d "$CORE" ] || git clone --depth 1 https://github.com/mupen64plus/mupen64plus-core.git "$CORE"
[ -f "$CORE/projects/unix/libmupen64plus.dylib" ] || ( cd "$CORE/projects/unix" && make all -j"$(sysctl -n hw.ncpu)" OSD=0 )
GL="$ROOT/GLideN64"
[ -d "$GL" ] || git clone --depth 1 https://github.com/gonetz/GLideN64.git "$GL"
[ -f "$GL/build/plugin/Release/mupen64plus-video-GLideN64.dylib" ] || ( mkdir -p "$GL/build" && cd "$GL/build" && cmake ../src -DMUPENPLUSAPI=On -DNOHQ=On -DVEC4_OPT=On -DCRC_OPT=On -DCMAKE_BUILD_TYPE=Release && make -j"$(sysctl -n hw.ncpu)" )
cp "$GL/ini/GLideN64.ini" "$GL/ini/GLideN64.custom.ini" "$ROOT/data/" 2>/dev/null || true
cp "$(brew --prefix)/share/mupen64plus/"* "$ROOT/data/" 2>/dev/null || true
echo "HLE rig ready in $ROOT"
```

- [ ] **Step 5: `scripts/hle-shot.sh`**

Same structure and window-finding as `scripts/ares-shot.sh`, but launches the rig and matches a window owner containing "mupen". Verbatim intent:

```bash
#!/bin/bash
set -euo pipefail
if [ $# -lt 3 ]; then echo "usage: $0 <rom.z64> <outdir> <seconds>..." >&2; exit 2; fi
ROM="$1"; OUT="$2"; shift 2
[ -f "$ROM" ] || { echo "no such ROM: $ROM" >&2; exit 1; }
mkdir -p "$OUT"
ROM_ABS="$(cd "$(dirname "$ROM")" && pwd)/$(basename "$ROM")"
ROOT="build/hle"
CORE="$ROOT/mupen64plus-core/projects/unix/libmupen64plus.dylib"
GFX="$ROOT/GLideN64/build/plugin/Release/mupen64plus-video-GLideN64.dylib"
[ -f "$CORE" ] && [ -f "$GFX" ] || { echo "rig missing; run scripts/build-hle-rig.sh" >&2; exit 1; }
read -r -d '' JS <<'JSEOF' || true
ObjC.import("CoreGraphics");
const ref = $.CGWindowListCopyWindowInfo($.kCGWindowListOptionOnScreenOnly | $.kCGWindowListExcludeDesktopElements, 0);
const list = ObjC.deepUnwrap(ObjC.castRefToObject(ref));
const rows = list.filter(d => (d.kCGWindowOwnerName||"").toLowerCase().indexOf("mupen") >= 0 && d.kCGWindowLayer === 0)
                 .sort((a,b) => b.kCGWindowBounds.Width - a.kCGWindowBounds.Width);
rows.length ? String(rows[0].kCGWindowNumber) : "";
JSEOF
pkill -f "mupen64plus --corelib" 2>/dev/null || true
sleep 0.5
mupen64plus --corelib "$CORE" --emumode 0 --windowed --resolution 640x480 --noosd \
  --configdir "$ROOT/cfg" --datadir "$ROOT/data" --gfx "$GFX" --rsp dummy --audio dummy "$ROM_ABS" \
  > "$OUT/run.log" 2>&1 &
START=$(date +%s)
for SEC in "$@"; do
  while [ $(( $(date +%s) - START )) -lt "$SEC" ]; do sleep 0.2; done
  WID=$(osascript -l JavaScript -e "$JS")
  if [ -z "$WID" ]; then echo "mupen window not found at ${SEC}s" >&2; pkill -f "mupen64plus --corelib" 2>/dev/null || true; exit 1; fi
  if ! screencapture -x -o -l "$WID" "$OUT/shot-${SEC}s.png"; then
    echo "capture failed at ${SEC}s (Screen Recording permission?)" >&2; pkill -f "mupen64plus --corelib" 2>/dev/null || true; exit 1
  fi
  echo "captured $OUT/shot-${SEC}s.png"
done
pkill -f "mupen64plus --corelib" 2>/dev/null || true
```

`chmod +x` both scripts.

- [ ] **Step 6: Verify**

```
make test                                   # unchanged, all 0 failure(s)
make rom && make rom-soft                    # games.z64 (rdpq) and games-soft.z64 both build, no warnings
scripts/build-hle-rig.sh                      # builds the rig (few minutes first time)
make rom-soft-autoplay GAME=brick
scripts/hle-shot.sh games-soft-autoplay.z64 build/hle-shots/task1 6
scripts/hle-shot.sh games-soft.z64 build/hle-shots/task1-menu 4
```

If you can view images: the menu shot shows GAMES with the game list in the built-in font on the beige field; the Brick shot shows the colored brick grid, paddle, ball, and HUD. Both must be non-black (that is the whole point: software rendering shows under GLideN64). Report what you see; if a capture is black, stop and report, because that means the software path is not displaying.

- [ ] **Step 7: Commit**

```
git add src/n64/app.c Makefile scripts/build-hle-rig.sh scripts/hle-shot.sh
git commit -m "v4 Task 1: software-render build (games-soft.z64) and HLE verification rig"
```

Then report: build output tails, what the two captures show, and any deviation.

---

### Task 2: Evidence and README

- Capture `games-soft.z64` menu and each game's software autoplay under the rig (`scripts/hle-shot.sh`), downscale to 768 px into `docs/superpowers/plans/evidence/v4-<name>.png`.
- README: add a "Delta and iOS" section — `games-soft.z64` is the build for Delta and other HLE emulators; it uses CPU rendering because HLE emulators cannot run libdragon's RSP microcode; it relies on GLideN64's framebuffer emulation, which is Delta's default; build it with `make rom-soft`; note that Delta 1.7.6 crashed on a macOS 27 beta before loading any ROM (a Delta + OS-beta bug, unrelated to the ROM) and to prefer a non-beta iPad/iPhone; the rdpq `games.z64` remains the build for ares and real hardware.
- Final: `make clean && make test && make rom && make rom-soft`; commit `git add README.md docs/superpowers/plans/evidence && git commit -m "v4 Task 2: Delta/iOS README and software-build evidence"`. Claude tags `v4.0`.

## Review checklist

As before, plus: confirm the rdpq build is unchanged (diff `src/n64/app.c` shows only additive `#ifdef SOFTRENDER` branches), and that every HLE capture is non-black.
