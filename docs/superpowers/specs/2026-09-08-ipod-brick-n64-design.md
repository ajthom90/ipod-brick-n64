# iPod Brick for Nintendo 64 — Design Spec

Date: 2026-09-08
Status: approved design, awaiting spec review

## 1. Goal

Recreate the classic iPod "Brick" game (Apple's Breakout clone, a 1st-gen iPod easter egg that became a standard Extras game from the 3rd generation on) as a Nintendo 64 ROM that runs in an emulator. The look follows the color iPod (5G / nano) version: colored brick rows on a light field, iPod minimalism, on a 4:3 TV.

### In scope (v1, "faithful core")

- Title screen, serve/play loop, pause, game over.
- Paddle, ball, 6 rows x 10 columns of bricks.
- 3 lives, 1 point per brick, clearing all bricks advances a level and speeds the ball up.
- Session high score (lost at power-off).
- Analog stick, D-pad, and C-left/right move the paddle; A launches/confirms; Start pauses.
- NTSC and PAL both boot; PAL simply ticks at 50 Hz.

### Out of scope (v1)

Audio, EEPROM/persistent saves, sprites or textures, DragonFS assets, custom fonts, multi-ball, paddle shrinking, power-ups, rumble, two-player, real-hardware cartridge testing.

## 2. Platform and toolchain

- SDK: libdragon, `trunk` branch, pinned to commit `c4a7e119eff1cfad07adcfa892a2910c40d8bdb8` (2026-08-29). Bump deliberately, never implicitly.
- Compiler: the official toolchain image `ghcr.io/dragonminded/libdragon:latest-arm64` (native Apple Silicon; the `latest` tag is amd64-only). That image contains only the mips64 GCC toolchain, so the project Dockerfile layers the libdragon library on top:

```dockerfile
FROM ghcr.io/dragonminded/libdragon:latest-arm64
ARG LIBDRAGON_COMMIT=c4a7e119eff1cfad07adcfa892a2910c40d8bdb8
RUN git init /tmp/libdragon && cd /tmp/libdragon \
 && git remote add origin https://github.com/DragonMinded/libdragon.git \
 && git fetch --depth 1 origin ${LIBDRAGON_COMMIT} && git checkout FETCH_HEAD \
 && make -j$(nproc) install-mk \
 && make -j$(nproc) libdragon tools \
 && make -j$(nproc) install tools-install \
 && rm -rf /tmp/libdragon
```

  The image tag is `ipod-brick-n64:dev`. Building it takes about a minute on this machine.
- ROM build: `docker run --rm -v "$PWD:/app" -w /app ipod-brick-n64:dev make rom-in-container` (wrapped by `make rom`). Runs unattended, no TTY.
- Host tools: clang from Xcode CLT, `make`, `sips` (PNG conversion), `screencapture`, `osascript`.
- Emulator for humans and for automated screenshots: ares 148 (`/Applications/ares.app`), launched with `open -a ares --args --system "Nintendo 64" <rom>`. ares is the emulator libdragon recommends and runs libdragon ROMs correctly (verified with the helloworld example on 2026-09-08).
- mupen64plus 2.6.0 from Homebrew is installed but **unusable**: its core predates the April 2025 RDRAM-initialization fix required by libdragon's open-source IPL3, so libdragon ROMs never render. Do not spend time on it.

## 3. Architecture

Two compile graphs share one core:

```
src/brick.h            public core API — no libdragon includes, no floats
src/brick.c            state machine, physics, collision, scoring, autoplay
src/n64/app.c          N64 adapter: display, rdpq drawing, joypad, tick loop
tests/test_brick.c     host unit tests (clang, <assert.h>)
tools/framedump.c      host tool: renders core state to PPM frames
scripts/docker-build.sh
scripts/ares-shot.sh
Dockerfile
Makefile
```

Rules:

- `src/brick.c` and `src/brick.h` must compile with `clang -std=c11 -Wall -Wextra -Werror` on macOS and with the mips64 GCC. They include only `<stdint.h>`, `<stdbool.h>`, `<string.h>`.
- No floating point anywhere in the core. Positions and velocities are Q8.8 fixed point (`int32_t`, 256 = 1 px). Multiply as `(a * b) >> 8`.
- The core is deterministic: identical input sequences produce identical states on host and N64. No randomness in v1.
- The adapter never reaches into core internals beyond the public header. It converts controller state to `brick_input_t`, calls `brick_update` at a fixed tick rate, and draws from the public struct plus rectangle helpers.

### 3.1 Core API (`src/brick.h`)

```c
enum {
    BRICK_SCREEN_W = 320, BRICK_SCREEN_H = 240,
    BRICK_PLAY_X0 = 16, BRICK_PLAY_Y0 = 24, BRICK_PLAY_X1 = 304, BRICK_PLAY_Y1 = 228,
    BRICK_ROWS = 6, BRICK_COLS = 10,
    BRICK_W = 26, BRICK_H = 10, BRICK_GAP_X = 2, BRICK_GAP_Y = 3,
    BRICK_GRID_X0 = 20, BRICK_GRID_Y0 = 32,
    BRICK_PADDLE_W = 48, BRICK_PADDLE_H = 6, BRICK_PADDLE_Y = 218,
    BRICK_BALL_SIZE = 6,
    BRICK_LIVES = 3,
    BRICK_PADDLE_SPEED_DIGITAL = 4,   /* px per tick */
    BRICK_PADDLE_SPEED_ANALOG_MAX = 6, /* px per tick at full deflection */
    BRICK_BALL_SPEED_BASE = 512,      /* Q8.8: 2.00 px per tick */
    BRICK_BALL_SPEED_RAMP = 90,       /* Q8.8: +0.35 px per tick per level */
    BRICK_BALL_SPEED_MAX = 1536,      /* Q8.8: 6.00 px per tick */
};

typedef enum {
    BRICK_ST_TITLE, BRICK_ST_SERVE, BRICK_ST_PLAY, BRICK_ST_PAUSE, BRICK_ST_GAMEOVER,
} brick_state_t;

typedef struct {
    int8_t  paddle_dir;   /* -1, 0, +1 from D-pad / C buttons */
    int16_t paddle_axis;  /* -256..+256 from analog stick, 0 inside dead zone */
    bool    launch;       /* A pressed this frame (edge) */
    bool    confirm;      /* A or B pressed this frame (edge) */
    bool    pause;        /* Start pressed this frame (edge) */
} brick_input_t;

typedef struct {
    brick_state_t state;
    brick_state_t pause_return;   /* SERVE or PLAY */
    int lives, score, high_score, level;
    int paddle_x;                 /* px, left edge */
    int32_t ball_x, ball_y;       /* Q8.8, top-left of the ball AABB */
    int32_t ball_vx, ball_vy;     /* Q8.8 px per tick */
    int32_t ball_speed;           /* Q8.8 magnitude for the current level */
    uint8_t cells[BRICK_ROWS][BRICK_COLS]; /* 1 = brick present */
    int bricks_left;
    uint32_t ticks;               /* ticks since brick_init, for tests/autoplay */
} brick_game_t;

typedef struct { int x0, y0, x1, y1; } brick_rect_t;  /* x1,y1 exclusive */

void brick_init(brick_game_t *g);                 /* zero everything, state TITLE */
void brick_new_game(brick_game_t *g);             /* keeps high_score; lives 3, level 1, SERVE */
void brick_update(brick_game_t *g, const brick_input_t *in); /* advance exactly one tick */
void brick_autoplay_input(const brick_game_t *g, brick_input_t *in); /* AI paddle for tests/screenshots */
void brick_on_ball_lost(brick_game_t *g);         /* transition helper, public for tests and tools */
void brick_on_level_clear(brick_game_t *g);       /* transition helper, public for tests and tools */
bool brick_rects_overlap(brick_rect_t a, brick_rect_t b);

brick_rect_t brick_cell_rect(int row, int col);
brick_rect_t brick_paddle_rect(const brick_game_t *g);
brick_rect_t brick_ball_rect(const brick_game_t *g);
uint32_t     brick_row_color(int row);            /* 0xRRGGBB */
```

Colors (0xRRGGBB): rows top to bottom red `C4472A`, orange `E07A1F`, yellow `D4B01C`, green `3FA34D`, blue `2E6DB4`, purple `7B4EA3`. Background `E8E4DC`. Paddle `2C2C2C`. Ball `1A1A1A`. Text `2C2C2C`. Defined once in `brick.h` so the N64 renderer and the host frame dumper agree.

Derived layout: grid width 10*26 + 9*2 = 278 px, from x 20 to 298; grid height 6*10 + 5*3 = 75 px, from y 32 to 107. All rectangle widths and the grid origin are even, which keeps fill-mode rectangles safely aligned.

### 3.2 Game rules and state machine

- **TITLE**: shows "BRICK", "HIGH SCORE n", "PRESS A". `confirm` → `brick_new_game` → SERVE.
- **SERVE**: ball rests centered on top of the paddle and follows it. Paddle moves normally. `launch` → PLAY with velocity from bounce zone 3 (see below), horizontal sign +1 on odd levels and -1 on even levels. `pause` → PAUSE with `pause_return = SERVE`.
- **PLAY**: paddle moves, ball moves, collisions resolve, score updates. `pause` → PAUSE with `pause_return = PLAY`. Ball top edge >= `BRICK_PLAY_Y1` → lose a life: if lives remain → SERVE, else → GAMEOVER. `bricks_left == 0` → `level++`, recompute `ball_speed`, refill all cells, → SERVE.
- **PAUSE**: nothing moves. `pause` → `pause_return`.
- **GAMEOVER**: shows "GAME OVER", final score, high score. `high_score = max(high_score, score)` is applied on entry. `confirm` → TITLE.

Paddle movement (SERVE and PLAY): if `paddle_axis != 0`, move `(paddle_axis * BRICK_PADDLE_SPEED_ANALOG_MAX) / 256` px; else move `paddle_dir * BRICK_PADDLE_SPEED_DIGITAL` px. Clamp to `[BRICK_PLAY_X0, BRICK_PLAY_X1 - BRICK_PADDLE_W]`.

Ball speed per level: `speed = min(BASE + RAMP * (level - 1), MAX)`. Level 12 and beyond run at the cap.

Ball movement per tick: if `speed > 768` (3.0 px/tick) split the tick into 2 sub-steps, otherwise 1. Each sub-step moves the X axis then resolves X collisions, then moves the Y axis and resolves Y collisions. This axis-separated scheme is deterministic and cannot tunnel: the largest sub-step is 3 px, smaller than the 6 px ball and the 10 px brick.

Collisions, in order, per axis pass:

1. Walls: left/right walls flip `vx`; the top wall flips `vy`; the ball is pushed back inside the playfield.
2. Bricks: the first occupied cell (row-major) whose rectangle overlaps the ball is cleared, `score++`, `bricks_left--`, the moved axis velocity flips, and the ball is pushed out of that cell. Only one brick per axis pass.
3. Paddle (Y pass only, while moving down): if the ball overlaps the paddle rectangle, snap the ball to sit on the paddle and apply the bounce table.

Paddle bounce table (7 zones across the paddle, chosen by the ball's center x; zone 0 is the left edge): angles from vertical -60, -40, -20, ±8, +20, +40, +60 degrees. Unit vectors in Q8.8 (sin, cos): 60° (222, 128), 40° (165, 196), 20° (88, 241), 8° (36, 253). New velocity: `vx = sign * (speed * sin) >> 8`, `vy = -((speed * cos) >> 8)`. Zone 3 keeps the sign of the incoming `vx` (or +1 if it was zero). Speed magnitude therefore stays constant within a level.

Autoplay (`brick_autoplay_input`): TITLE → `confirm`; SERVE → `launch`; GAMEOVER → nothing (stay on the screen so it can be captured). In PLAY during level 1 the paddle steers so the ball lands off-center and gets sent toward the far side of the field, rotating through three bounce zones so a deterministic game cannot settle into a loop that misses bricks: with `k = (ticks / 300) % 3`, the target zone is `4 + k` when the ball center is in the left half of the playfield and `2 - k` in the right half; the target paddle x is `ball_center_x - (zone * BRICK_PADDLE_W) / 7 - BRICK_PADDLE_W / 14`; `paddle_axis` is +256 or -256 toward the target and 0 within 2 px of it. From level 2 on the autoplay holds the paddle still, so the game deliberately loses its lives and reaches GAMEOVER within about 15 seconds of play, giving an unattended emulator run every screen except PAUSE. A unit test drives a new game with autoplay and asserts that level 2 is reached within 30,000 ticks and GAMEOVER within 60,000 ticks, proving a level is clearable, that speed ramps, and that lives run out.

### 3.3 Input mapping (adapter)

| iPod | N64 | `brick_input_t` |
|---|---|---|
| Click wheel | analog stick X, proportional | `paddle_axis` |
| Wheel nudge | D-pad left/right or C-left/C-right | `paddle_dir` |
| Center button | A | `launch`, `confirm` |
| (Menu) | Start | `pause` |
| — | B on title / game over | `confirm` |

Stick: `joypad_get_inputs(JOYPAD_PORT_1).stick_x` is an `int8_t`, about -85..+85 on a healthy stick. Dead zone ±8. `axis = clamp(stick_x, -80, 80) * 256 / 80`. Edge buttons come from `joypad_get_buttons_pressed`; the D-pad and C-button levels from `joypad_get_buttons`. The adapter calls `joypad_poll` once per rendered frame and reuses the same `brick_input_t` for every catch-up tick, so a single press is never applied twice.

### 3.4 Rendering (adapter)

- `display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE)`. Three buffers so `rdpq_detach_show` never stalls the CPU.
- `rdpq_init()`; in debug builds (`make rom DEBUG=1`, which defines `BRICK_DEBUG`) `rdpq_debug_start()` right after it.
- Font: `rdpq_text_register_font(1, rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO))` plus one `rdpq_font_style` with the text color. Font id 0 is reserved by libdragon.
- Per frame: `display_get` → `rdpq_attach(fb, NULL)` → `rdpq_set_mode_fill(bg)` → `rdpq_fill_rectangle(0,0,320,240)` → for each present cell, `rdpq_set_fill_color` when the row changes and `rdpq_fill_rectangle(cell)` → paddle and ball rectangles → `rdpq_set_mode_standard()` → `rdpq_text_printf` for HUD and overlays → `rdpq_detach_show()`.
- `rdpq_fill_rectangle` bounds are exclusive on the right and bottom, matching `brick_rect_t`.
- HUD baseline y = 16: score left-aligned at x 16, "LIVES n" centered, "LV n" right-aligned to x 304. Title, pause, and game-over text are centered with `rdpq_textparms_t{ .width = 320, .align = ALIGN_CENTER }`. Text y coordinates are baselines.

### 3.5 Timing (adapter)

```
hz  = (get_tv_type() == TV_PAL) ? 50 : 60;
dt  = TICKS_PER_SECOND / hz;
acc += get_ticks() - prev; prev = now;
steps = 0;
while (acc >= dt && steps < 4) { brick_update(&game, &input); acc -= dt; steps++; }
```

The 4-step cap prevents a spiral after a stall. PAL runs the same per-tick speeds at 50 Hz and is therefore slightly slower in real time; accepted for v1.

### 3.6 Autoplay build

`make rom-autoplay` builds `brick-autoplay.z64` with `-DBRICK_AUTOPLAY`. In that build the adapter ignores the controller and feeds `brick_autoplay_input` instead, so an unattended emulator run advances from title into real play. Release builds never define the flag.

## 4. Build, test, and verification

Makefile targets (host side unless noted):

| Target | What it does |
|---|---|
| `make test` | `clang -std=c11 -Wall -Wextra -Werror -O1 -Isrc tests/test_brick.c src/brick.c -o build/test_brick && ./build/test_brick`. Must not require `N64_INST`. |
| `make frames` | Builds `tools/framedump.c` with the core, runs an autoplay game, writes `build/frames/frame-NNNN.ppm` every 30 ticks for 1200 ticks, plus `pause.ppm` (Start pressed mid-play) and `gameover.ppm` (autoplay run to its end), and converts them all to PNG with `sips`. |
| `make image` | `docker build -t ipod-brick-n64:dev .` |
| `make rom` | Runs `make rom-in-container` inside the image; produces `brick.z64`. |
| `make rom-autoplay` | Same with `-DBRICK_AUTOPLAY` and `BUILD_DIR=build/autoplay`; produces `brick-autoplay.z64`. |
| `make run` | `open -a ares --args --system "Nintendo 64" brick.z64` |
| `make shots` | `scripts/ares-shot.sh brick-autoplay.z64 build/shots 4 8 15` |
| `make clean` | Removes `build/` and `*.z64`. |

Inside the container, `rom-in-container` includes `$(N64_INST)/include/n64.mk`, compiles `src/brick.c` and `src/n64/app.c`, links `$(BUILD_DIR)/brick.elf`, and produces the ROM with `N64_ROM_TITLE="Brick"`. No DFS, no asset conversion. The top-level Makefile guards the n64.mk include with `ifdef N64_INST` so host targets work without the toolchain.

`scripts/ares-shot.sh <rom> <outdir> <seconds>...`: closes any running ares, launches ares with the ROM, then for each listed number of seconds after launch finds the ares game window through `CGWindowListCopyWindowInfo` (via `osascript -l JavaScript`), captures only that window with `screencapture -x -o -l <id>`, and finally quits ares. Requires a logged-in GUI session; it opens a visible window briefly.

Test coverage required in `tests/test_brick.c` (plain `assert`, one `main`, prints the test name before each case):

- init → TITLE, zero score, 3 lives after new game, 60 bricks.
- confirm on title → SERVE; launch → PLAY with `vy < 0`.
- paddle clamps at both playfield edges; analog beats digital; dead zone yields no motion.
- wall bounces flip the right axis and keep the ball inside.
- a brick hit clears exactly one cell, increments score, flips the moved axis.
- paddle bounce zones produce the table's vectors with constant speed.
- losing the ball decrements lives and returns to SERVE; at zero lives → GAMEOVER and high score updates.
- Start toggles PLAY ↔ PAUSE and SERVE ↔ PAUSE; nothing moves while paused.
- clearing all bricks → level 2, refilled grid, faster speed, SERVE.
- autoplay reaches level 2 within 30,000 ticks and GAMEOVER within 60,000 ticks.
- host and N64 rectangle helpers return even-aligned, in-bounds, exclusive rectangles for every cell.

Review evidence per task: `make test` output, `make frames` PNGs where relevant, and ares screenshots for adapter tasks.

## 5. Repository

- Git repository in `~/projects/ipod-game`, public GitHub remote `ipod-brick-n64` under the user's account.
- `.gitignore`: `build/`, `*.z64`, `.DS_Store`.
- `README.md`: what it is, how to build (Docker), how to run (ares), controls.
- Implementation is done by the Grok Build CLI one task at a time; Claude reviews every task's commit before the next task starts. Task prompts and review notes live in `docs/superpowers/plans/`.

## 6. Task plan

1. **Skeleton and boot**: Dockerfile, Makefile, scripts, `.gitignore`, README stub, `src/brick.h` with the full API and constants, `src/brick.c` stubs, `src/n64/app.c` that clears the screen to the background color and prints "BRICK" centered. Verify: `make image && make rom` produce `brick.z64`; an ares screenshot shows the text on the light background; `make test` compiles (tests may be minimal).
2. **Core state machine**: TITLE/SERVE/PLAY/PAUSE/GAMEOVER transitions, lives, score, level bookkeeping, grid refill, high score. Verify: `make test` passes the state-machine cases.
3. **Core physics**: paddle movement, ball Q8.8 movement with sub-steps, walls, bricks, paddle bounce table, speed ramp, autoplay helper. Verify: `make test` passes all cases including the autoplay level-clear proof.
4. **Frame dump tool**: `tools/framedump.c` and `make frames`. Verify: PNGs show the grid, paddle, and ball progressing through an autoplay game.
5. **N64 adapter rendering and input**: fill-mode drawing of the core state, HUD text, input mapping, but ticking once per frame. Verify: ares screenshot of the release ROM shows the title; of the autoplay ROM shows the playfield.
6. **Fixed tick loop and autoplay build**: `get_ticks` accumulator, catch-up cap, PAL/NTSC rate, `make rom-autoplay`, `make shots`. Verify: screenshots at 4 s, 8 s, and 15 s differ and show bricks disappearing.
7. **Overlays**: title, pause, game-over screens and high score text. Verify: ares screenshots of the autoplay ROM show TITLE (first second), PLAY, and GAMEOVER (after level 2 begins and the lives run out); PAUSE is verified with the `pause.png` host frame dump, since no emulator input is scripted.
8. **Polish**: overscan check in ares, `rdpq_debug_start` in debug builds, README complete, final review. Verify: `make test`, `make rom`, `make rom-autoplay`, `make shots` all green; a human play-test in ares.

## 7. Risks and mitigations

- **Fill-rectangle bounds**: exclusive right/bottom; tests assert the helpers, and the renderer uses them verbatim.
- **Toolchain drift**: libdragon is pinned by commit in the Dockerfile; the base image tag can move, so record the digest in the README when the image is first built.
- **Emulator differences**: ares is the reference. Real-hardware quirks (overscan, stick wear) are accounted for by margins and the dead zone but not tested on hardware.
- **Screenshot automation**: relies on a GUI session and the ares window title; if macOS denies screen capture, the script fails loudly and the reviewer falls back to `make frames` plus a human check.
- **Double-firing edge inputs**: inputs are sampled once per rendered frame and reused across catch-up ticks.
- **Float drift between host and N64**: forbidden by the no-float rule; `-Werror=double-promotion` and a grep for `float`/`double` in the core are part of review.
