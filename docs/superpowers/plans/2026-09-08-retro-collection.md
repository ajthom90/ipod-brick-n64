# Retro Games Collection Implementation Plan (v2)

> **For agentic workers:** This plan is executed one task at a time by the Grok Build CLI, driven by Claude, who writes each task prompt, reviews the resulting commit, and only then hands out the next task. Steps use checkbox (`- [ ]`) syntax. A worker sees only its own task prompt plus this file and the spec, so every task repeats what it needs. Keep every single command under four minutes.

**Goal:** Turn `brick.z64` into `games.z64`: an iPod-style games menu with Brick, Blocks, Snake, Pong, Parachute, and 2048, original chiptune music and sound effects, a Settings screen, and EEPROM-saved settings and high scores.

**Architecture:** Every game is a pure C module behind one descriptor (`init`, `start`, `update`, `render`, `autoplay`, high-score accessors, sound-effect queue) that draws through a two-call draw API and never touches libdragon. The framework (menu, pause menu, settings, save record, synth, sequencer) is pure C too. One adapter file binds it all to libdragon: rdpq drawing, joypad input for two ports, audio buffer filling, EEPROM, and the fixed tick loop. Host tools render frames and music so review happens on the Mac.

**Tech Stack:** C11, libdragon trunk pinned at `c4a7e119eff1cfad07adcfa892a2910c40d8bdb8`, Docker image `ipod-brick-n64:dev`, clang, GNU make, ares 148, `screencapture`, `sips`, `afplay`.

**Spec:** `docs/superpowers/specs/2026-09-08-retro-collection-design.md` (v2). The v1 spec `docs/superpowers/specs/2026-09-08-ipod-brick-n64-design.md` still governs Brick's rules.

## Global Constraints

- Pure modules (`src/game.h`, `src/prng.h`, `src/sfx.h`, `src/music*.{c,h}`, `src/synth.{c,h}`, `src/menu.{c,h}`, `src/settings.{c,h}`, `src/save.{c,h}`, everything under `src/games/`) include only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`. Never `<libdragon.h>`, `<math.h>`, `<stdio.h>`, `float`, or `double`. Q8.8 fixed point (`(a * b) >> 8`) for sub-pixel motion; Q16 phase accumulators in the synth.
- Host flags, verbatim: `clang -std=c11 -Wall -Wextra -Werror -Werror=double-promotion -O1 -Isrc`.
- Rectangles use exclusive `x1`/`y1`. Screen 320x240, playfield x 16..304, y 24..228, HUD baseline 26 in `DRAW_FONT_HUD`, overlays in `DRAW_FONT_BIG` with at least 28 px line spacing. Palette: red `C4472A`, orange `E07A1F`, yellow `D4B01C`, green `3FA34D`, blue `2E6DB4`, purple `7B4EA3`, teal `3AAFA9`, background `E8E4DC`, dark `2C2C2C`, ball `1A1A1A`, empty tile `EDE4D6`.
- Determinism: no time source and no randomness outside `prng.h`; the fixed test seed is `0x1234567u`.
- Start never reaches a game (framework pause menu). B is game-specific.
- ROM: `games.z64` / `games-autoplay.z64`, title "Games", save type `eeprom4k`, built in Docker exactly as v1. Emulator: ares only; captures via `scripts/ares-shot.sh`.
- Every task ends with `make test` green, the tree committed with the given message, nothing else uncommitted, no push.
- Workers do not edit the spec or this plan; they report disagreements in their final message.

---

## File Structure

| Path | Responsibility |
|---|---|
| `src/game.h` | `input_t`, `draw_t`, `game_desc_t`, `GAME_MAX_PLAYERS`, screen/palette constants shared by all games |
| `src/prng.h` | xorshift32 (`prng_t`, `prng_seed`, `prng_next`, `prng_below`) |
| `src/sfx.h` | `sfx_id_t`, `sfx_queue_t`, `sfx_push`, `sfx_pop` |
| `src/games/brick.c/.h` | Brick (moved from `src/brick.c`), refactored onto the descriptor |
| `src/games/blocks.c/.h`, `snake.c/.h`, `pong.c/.h`, `parachute.c/.h`, `g2048.c/.h` | the new games |
| `src/games/registry.c/.h` | `GAMES[]`, `GAME_COUNT`, `game_index_by_name` |
| `src/menu.c/.h` | games menu and pause menu (`menu_t`) |
| `src/settings.c/.h` | `settings_t` and the settings screen |
| `src/save.c/.h` | `save_t`, encode/decode with CRC32, defaults |
| `src/synth.c/.h` | 4-channel chiptune synth plus sound-effect voice |
| `src/music.c/.h` | notation parser, sequencer, `music_track_id_t` |
| `src/music_data.c` | the seven tunes |
| `src/app_state.c/.h` | pure "application" state machine: which screen is active, screen transitions, audio track selection, save triggers (so the adapter is a thin shell and the whole flow is host-testable) |
| `src/n64/app.c` | libdragon adapter |
| `tools/framedump.c` | draw API over a pixel buffer; `framedump <game|menu> <outdir>` |
| `tools/musicdump.c` | renders tunes and effects to WAV |
| `tests/harness.h` | `CHECK` / `RUN` macros |
| `tests/test_*.c` | one binary per module |
| `Makefile` | host targets (`test`, `frames`, `music`, `image`, `rom`, `rom-autoplay`, `run`, `shots`, `clean`) plus n64.mk rules |

---

## Shared code (defined once here, used by every task)

### `tests/harness.h`

```c
#ifndef HARNESS_H
#define HARNESS_H
#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { failures++; fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); return; } \
} while (0)

#define RUN(fn) do { \
    printf("%-52s", #fn); fflush(stdout); \
    int before = failures; fn(); \
    puts(before == failures ? "ok" : "FAILED"); \
} while (0)

#define HARNESS_MAIN_END() do { printf("%d failure(s)\n", failures); return failures ? 1 : 0; } while (0)

#endif
```

### `src/prng.h`

```c
#ifndef PRNG_H
#define PRNG_H
#include <stdint.h>

typedef struct { uint32_t state; } prng_t;

static inline void prng_seed(prng_t *p, uint32_t seed) { p->state = seed ? seed : 0x9E3779B9u; }

static inline uint32_t prng_next(prng_t *p) {
    uint32_t x = p->state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    p->state = x;
    return x;
}

/* Uniform integer in [0, n). n must be >= 1. */
static inline uint32_t prng_below(prng_t *p, uint32_t n) { return prng_next(p) % n; }

#endif
```

### `src/sfx.h`

```c
#ifndef SFX_H
#define SFX_H
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SFX_NONE = 0, SFX_BOUNCE, SFX_HIT, SFX_CLEAR, SFX_FOOD, SFX_SHOT, SFX_EXPLODE,
    SFX_POINT, SFX_MERGE, SFX_GAME_OVER, SFX_MENU_MOVE, SFX_MENU_SELECT, SFX_COUNT,
} sfx_id_t;

typedef struct { uint8_t ids[8]; uint8_t head, tail; } sfx_queue_t;   /* ring of up to 7 */

static inline void sfx_push(sfx_queue_t *q, sfx_id_t id) {
    uint8_t next = (uint8_t)((q->tail + 1) % 8);
    if (next == q->head) return;              /* full: drop */
    q->ids[q->tail] = (uint8_t)id; q->tail = next;
}

static inline sfx_id_t sfx_pop(sfx_queue_t *q) {
    if (q->head == q->tail) return SFX_NONE;
    sfx_id_t id = (sfx_id_t)q->ids[q->head]; q->head = (uint8_t)((q->head + 1) % 8);
    return id;
}

#endif
```

### `src/game.h`

```c
#ifndef GAME_H
#define GAME_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "sfx.h"

enum { SCREEN_W = 320, SCREEN_H = 240, PLAY_X0 = 16, PLAY_Y0 = 24, PLAY_X1 = 304, PLAY_Y1 = 228,
       HUD_BASELINE = 26, GAME_MAX_PLAYERS = 2 };

#define COLOR_RED     0xC4472Au
#define COLOR_ORANGE  0xE07A1Fu
#define COLOR_YELLOW  0xD4B01Cu
#define COLOR_GREEN   0x3FA34Du
#define COLOR_BLUE    0x2E6DB4u
#define COLOR_PURPLE  0x7B4EA3u
#define COLOR_TEAL    0x3AAFA9u
#define COLOR_BG      0xE8E4DCu
#define COLOR_DARK    0x2C2C2Cu
#define COLOR_BALL    0x1A1A1Au
#define COLOR_TILE    0xEDE4D6u
#define DRAW_TEXT_DARK  COLOR_DARK
#define DRAW_TEXT_LIGHT COLOR_BG

typedef struct {
    int16_t stick_x, stick_y;   /* -256..+256, up is +y */
    int8_t  dpad_x, dpad_y;     /* -1, 0, +1 (D-pad, C buttons alias), up is +1 */
    bool    a, b, z;            /* pressed this tick */
    bool    a_held, b_held;
} input_t;

typedef enum { DRAW_FONT_HUD = 1, DRAW_FONT_BIG = 2 } draw_font_t;
typedef enum { DRAW_LEFT, DRAW_CENTER, DRAW_RIGHT } draw_align_t;

typedef struct draw_s {
    void *ctx;
    void (*rect)(void *ctx, int x0, int y0, int x1, int y1, uint32_t rgb);
    void (*text)(void *ctx, draw_font_t font, draw_align_t align, int x, int y, uint32_t rgb, const char *utf8);
} draw_t;

static inline void draw_rect(const draw_t *d, int x0, int y0, int x1, int y1, uint32_t rgb) { d->rect(d->ctx, x0, y0, x1, y1, rgb); }
static inline void draw_text(const draw_t *d, draw_font_t f, draw_align_t a, int x, int y, uint32_t rgb, const char *s) { d->text(d->ctx, f, a, x, y, rgb, s); }

/* Tiny integer formatters for HUD strings (no stdio): "PREFIX n" and plain "n". */
void fmt_label(char *buf, size_t cap, const char *prefix, int value);
void fmt_int(char *buf, size_t cap, int value);

typedef enum { MUSIC_MENU = 0, MUSIC_BRICK, MUSIC_BLOCKS, MUSIC_SNAKE, MUSIC_PONG, MUSIC_PARACHUTE, MUSIC_2048, MUSIC_TRACK_COUNT } music_track_id_t;

typedef struct {
    const char *name;
    int players;
    void *state;
    void (*init)(void *st);
    void (*start)(void *st, uint32_t seed);
    void (*update)(void *st, const input_t in[GAME_MAX_PLAYERS]);
    void (*render)(const void *st, const draw_t *d);
    void (*autoplay)(const void *st, input_t in[GAME_MAX_PLAYERS]);
    bool (*is_over)(const void *st);
    int  (*get_high_score)(const void *st);
    void (*set_high_score)(void *st, int value);
    sfx_queue_t *(*sfx)(void *st);
    music_track_id_t track;
} game_desc_t;

#endif
```

`fmt_label` and `fmt_int` live in `src/games/registry.c` (pure C, no stdio); `fmt_int` is `fmt_label` without the prefix and the space: copies the prefix, a space, then the decimal digits of `value` (handles 0 and negatives), NUL-terminates, never overflows `cap`.

### Host draw implementation (`tools/framedump.c`, shared by every frame dump)

`rect` fills the pixel buffer with clipping. `text` draws a placeholder box in `0xC8C2B8`: width = 6 px per byte of the string for HUD, 12 px for BIG; height 10 or 20 px; the box's bottom edge is the baseline; x is left/center/right per alignment. It also prints `text font=%d align=%d x=%d y=%d "%s"` to stdout. Frame files are PPM converted to PNG with `sips` by the Makefile, exactly as v1.

---
### Task 1: Framework and Brick refactor

**Files:**
- Create: `src/game.h`, `src/prng.h`, `src/sfx.h`, `tests/harness.h`, `src/games/registry.c`, `src/games/registry.h`, `src/games/brick.h`, `tests/test_prng.c`, `tests/test_sfx.c`
- Move: `src/brick.c` → `src/games/brick.c` (`git mv`), `src/brick.h` → `src/games/brick.h`
- Modify: `src/games/brick.c`, `tests/test_brick.c`, `tools/framedump.c`, `src/n64/app.c`, `Makefile`

**Interfaces:**
- Consumes: v1 Brick.
- Produces: everything in "Shared code" above; `const game_desc_t GAME_BRICK`; `const game_desc_t *const GAMES[]` and `GAME_COUNT` (Brick only for now); `int game_index_by_name(const char *)`; adapter functions `n64_rect`, `n64_text`, `read_port`; `make frames GAME=<name>`; `make rom-autoplay GAME=<name>`.

- [ ] **Step 1: Shared headers and harness**

Create `src/game.h`, `src/prng.h`, `src/sfx.h`, `tests/harness.h` exactly as in "Shared code". Create `src/games/registry.h`:

```c
#ifndef REGISTRY_H
#define REGISTRY_H
#include "game.h"
extern const game_desc_t *const GAMES[];
extern const int GAME_COUNT;
int game_index_by_name(const char *name);   /* case-insensitive, -1 if unknown */
#endif
```

and `src/games/registry.c` with `GAMES[] = { &GAME_BRICK }`, `GAME_COUNT = 1`, `game_index_by_name`, and `fmt_label`:

```c
void fmt_label(char *buf, size_t cap, const char *prefix, int value) {
    size_t n = 0;
    while (prefix[n] && n + 1 < cap) { buf[n] = prefix[n]; n++; }
    if (n + 1 < cap) buf[n++] = ' ';
    char digits[12]; int d = 0;
    unsigned v = value < 0 ? (unsigned)(-value) : (unsigned)value;
    do { digits[d++] = (char)('0' + v % 10); v /= 10; } while (v && d < 11);
    if (value < 0 && n + 1 < cap) buf[n++] = '-';
    while (d > 0 && n + 1 < cap) buf[n++] = digits[--d];
    buf[n] = '\0';
}
```

- [ ] **Step 2: Tests for the shared pieces**

`tests/test_prng.c`: seeding with 0 still produces non-zero output; two generators with the same seed produce the same 100 values; `prng_below(p, 6)` over 6000 draws lands every value between 800 and 1200 times.
`tests/test_sfx.c`: pop on empty returns `SFX_NONE`; push seven then pop seven in order; the eighth push is dropped; `fmt_label` produces `"SCORE 0"`, `"LV 12"`, `"X -7"`, and truncates safely into a 4-byte buffer.

- [ ] **Step 3: Port Brick onto the descriptor**

In `src/games/brick.h`: drop `brick_input_t`, `BRICK_ST_PAUSE`, `pause_return`, and `BRICK_CATCHUP_MAX`; include `"../game.h"`; add `sfx_queue_t sfx;` to `brick_game_t`; change signatures to `void brick_update(brick_game_t *g, const input_t in[GAME_MAX_PLAYERS])` and `void brick_autoplay_input(const brick_game_t *g, input_t in[GAME_MAX_PLAYERS])`; add `void brick_render(const brick_game_t *g, const draw_t *d)`; declare `extern const game_desc_t GAME_BRICK;`. Remove the Brick-specific color macros in favor of `game.h` (`BRICK_COLOR_BG` → `COLOR_BG`, etc.), keeping `brick_row_color`.

In `src/games/brick.c`:
- `move_paddle` reads `in[0].stick_x` (analog) and `in[0].dpad_x` (digital).
- SERVE: `in[0].a` launches. TITLE and GAMEOVER: `in[0].a` confirms. No pause handling.
- Push sounds: `SFX_BOUNCE` in `bounce_off_paddle`, `SFX_HIT` in `hit_brick`, `SFX_CLEAR` in `brick_on_level_clear`, `SFX_GAME_OVER` when `brick_on_ball_lost` reaches GAMEOVER.
- `brick_render` is the v1 `render` from `src/n64/app.c` rewritten on `draw_t`: background, bricks by row color, paddle, ball (not on GAMEOVER), HUD with `fmt_label` into a 24-byte buffer, overlays at the v1 baselines (title 100/130/160; game over 140/168/196).
- Descriptor at the bottom of the file:

```c
static brick_game_t brick_state;
static void brick_desc_init(void *st) { brick_init(st); }
static void brick_desc_start(void *st, uint32_t seed) { (void)seed; brick_game_t *g = st; int hs = g->high_score; brick_init(g); g->high_score = hs; }
static void brick_desc_update(void *st, const input_t in[GAME_MAX_PLAYERS]) { brick_update(st, in); }
static void brick_desc_render(const void *st, const draw_t *d) { brick_render(st, d); }
static void brick_desc_autoplay(const void *st, input_t in[GAME_MAX_PLAYERS]) { brick_autoplay_input(st, in); }
static bool brick_desc_is_over(const void *st) { return ((const brick_game_t *)st)->state == BRICK_ST_GAMEOVER; }
static int  brick_desc_get_hs(const void *st) { return ((const brick_game_t *)st)->high_score; }
static void brick_desc_set_hs(void *st, int v) { ((brick_game_t *)st)->high_score = v; }
static sfx_queue_t *brick_desc_sfx(void *st) { return &((brick_game_t *)st)->sfx; }

const game_desc_t GAME_BRICK = {
    .name = "BRICK", .players = 1, .state = &brick_state,
    .init = brick_desc_init, .start = brick_desc_start, .update = brick_desc_update,
    .render = brick_desc_render, .autoplay = brick_desc_autoplay, .is_over = brick_desc_is_over,
    .get_high_score = brick_desc_get_hs, .set_high_score = brick_desc_set_hs, .sfx = brick_desc_sfx,
    .track = MUSIC_BRICK,
};
```

`start` returns to the TITLE state (so the game shows its title, high score, and PRESS A on launch).

- [ ] **Step 4: Port the Brick tests**

In `tests/test_brick.c`: include `"harness.h"` and `"games/brick.h"`; replace `brick_input_t in; memset(...)` with `input_t in[2]; memset(in, 0, sizeof in);`, `in.paddle_dir` → `in[0].dpad_x`, `in.paddle_axis` → `in[0].stick_x`, `in.launch`/`in.confirm` → `in[0].a`. Delete `test_pause_toggles`. Add:

```c
static void test_sfx_events(void) {
    brick_game_t g = playing();
    brick_rect_t cell = brick_cell_rect(BRICK_ROWS - 1, 3);
    g.ball_x = (cell.x0 + 10) << 8; g.ball_y = (cell.y1 + 1) << 8; g.ball_vx = 0; g.ball_vy = -512;
    tick(&g, NULL, 1);
    CHECK(sfx_pop(&g.sfx) == SFX_HIT);
    CHECK(sfx_pop(&g.sfx) == SFX_NONE);
    g.ball_x = (g.paddle_x + 21) << 8; g.ball_y = (BRICK_PADDLE_Y - BRICK_BALL_SIZE - 1) << 8; g.ball_vx = 0; g.ball_vy = 512;
    tick(&g, NULL, 1);
    CHECK(sfx_pop(&g.sfx) == SFX_BOUNCE);
    memset(g.cells, 0, sizeof g.cells); g.cells[BRICK_ROWS - 1][0] = 1; g.bricks_left = 1;
    cell = brick_cell_rect(BRICK_ROWS - 1, 0);
    g.ball_x = (cell.x0 + 10) << 8; g.ball_y = (cell.y1 + 1) << 8; g.ball_vx = 0; g.ball_vy = -512;
    tick(&g, NULL, 1);
    CHECK(sfx_pop(&g.sfx) == SFX_HIT);
    CHECK(sfx_pop(&g.sfx) == SFX_CLEAR);
    g.lives = 1; brick_on_ball_lost(&g);
    CHECK(sfx_pop(&g.sfx) == SFX_GAME_OVER);
}

static void test_descriptor(void) {
    brick_game_t *g = GAME_BRICK.state;
    GAME_BRICK.init(g); GAME_BRICK.set_high_score(g, 99); GAME_BRICK.start(g, 1);
    CHECK(g->state == BRICK_ST_TITLE && GAME_BRICK.get_high_score(g) == 99);
    CHECK(!GAME_BRICK.is_over(g));
    CHECK(GAME_BRICK.players == 1 && GAME_BRICK.track == MUSIC_BRICK);
    input_t in[2]; GAME_BRICK.autoplay(g, in);
    CHECK(in[0].a);
}
```

Use `HARNESS_MAIN_END()` at the end of `main`.

- [ ] **Step 5: Frame dumper on the draw API**

Rewrite `tools/framedump.c`: usage `framedump <game-name> <outdir>`; look the game up with `game_index_by_name`; call `init`, `set_high_score(0)`, `start(0x1234567u)`; every 30 ticks for 1200 ticks call `render` into the pixel buffer through a `draw_t` whose `rect` fills pixels and whose `text` draws the placeholder box and prints the line described under "Host draw implementation"; then run autoplay to `is_over` (cap 200,000 ticks) and save `gameover.ppm`. Print `name state-summary` lines as v1 did (`score=`, from `get_high_score` is not the score; print the tick and whether `is_over`).

- [ ] **Step 6: Makefile**

- `C_FILES := $(wildcard src/*.c) $(wildcard src/games/*.c) src/n64/app.c` (the `src/*.c` glob is empty until Task 2 adds framework sources).
- `TESTS := $(patsubst tests/%.c,build/host/%,$(wildcard tests/test_*.c))`; `build/host/%: tests/%.c $(CORE_SRCS) $(wildcard src/*.h src/games/*.h) tests/harness.h` compiles `tests/$*.c $(CORE_SRCS)` where `CORE_SRCS := $(wildcard src/*.c) $(wildcard src/games/*.c)`; `test: $(TESTS)` runs each in turn with `set -e; for t in $(TESTS); do echo "== $$t"; $$t; done`.
- `frames: build/host/framedump` runs `./build/host/framedump $(GAME) build/frames/$(GAME)` with `GAME ?= brick`, then converts PPMs.
- `ROMNAME ?= games`; `rom-autoplay:` passes `ROMNAME=games-autoplay BUILD_DIR=build/autoplay AUTOPLAY=1 GAME=$(GAME)`; inside the n64 block `ifeq ($(AUTOPLAY),1) N64_CFLAGS += -DAUTOPLAY_GAME=\"$(GAME)\" endif`; `N64_ROM_TITLE = "Games"`; `run` and `shots` use `games.z64` / `games-autoplay.z64`.
- Keep the font and DFS rules from v1 Task 8, renamed to `$(ROMNAME)`.

- [ ] **Step 7: Adapter**

Replace `src/n64/app.c` with the descriptor-driven version. Keep the v1 boot sequence (`dfs_init`, `display_init` with `FILTERS_RESAMPLE`, `rdpq_init`, `joypad_init`, both fonts) and register a second style on each font: `rdpq_font_style(f, 1, &(rdpq_fontstyle_t){ .color = rgb(DRAW_TEXT_LIGHT) })`.

```c
static color_t rgb(uint32_t c) { return RGBA32((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 0xFF); }

static void n64_rect(void *ctx, int x0, int y0, int x1, int y1, uint32_t c) {
    (void)ctx;
    rdpq_set_fill_color(rgb(c));
    rdpq_fill_rectangle(x0, y0, x1, y1);
}

static void n64_text(void *ctx, draw_font_t font, draw_align_t align, int x, int y, uint32_t c, const char *s) {
    (void)ctx;
    rdpq_set_mode_standard();
    rdpq_textparms_t p = { .style_id = (c == DRAW_TEXT_LIGHT) ? 1 : 0 };
    if (align == DRAW_LEFT) {
        rdpq_text_print(&p, font, x, y, s);
    } else if (align == DRAW_CENTER) {
        int w = 2 * (x < SCREEN_W - x ? x : SCREEN_W - x);     /* widest box centered on x that stays on screen */
        p.width = (int16_t)w; p.align = ALIGN_CENTER;
        rdpq_text_print(&p, font, x - w / 2, y, s);
    } else {
        p.width = (int16_t)x; p.align = ALIGN_RIGHT;
        rdpq_text_print(&p, font, 0, y, s);
    }
    rdpq_set_mode_fill(rgb(COLOR_BG));
}

static const draw_t n64_draw = { .ctx = NULL, .rect = n64_rect, .text = n64_text };

static int16_t axis(int8_t v) {
    int s = v;
    if (s > -8 && s < 8) s = 0;
    if (s > 80) s = 80;
    if (s < -80) s = -80;
    return (int16_t)((s * 256) / 80);
}

/* Levels every frame; edges OR-ed in and cleared by the caller after a tick. */
static void read_port(joypad_port_t port, input_t *in) {
    joypad_inputs_t inputs = joypad_get_inputs(port);
    joypad_buttons_t held = joypad_get_buttons(port);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(port);
    in->stick_x = axis(inputs.stick_x);
    in->stick_y = axis(inputs.stick_y);
    in->dpad_x = (held.d_right || held.c_right) ? 1 : (held.d_left || held.c_left) ? -1 : 0;
    in->dpad_y = (held.d_up || held.c_up) ? 1 : (held.d_down || held.c_down) ? -1 : 0;
    if (pressed.a) in->a = true;
    if (pressed.b) in->b = true;
    if (pressed.z) in->z = true;
    in->a_held = held.a;
    in->b_held = held.b;
}

static void clear_edges(input_t *in) { in->a = in->b = in->z = false; }
```

Frame: `display_get` → `rdpq_attach` → `rdpq_set_mode_fill(rgb(COLOR_BG))` → `game->render(state, &n64_draw)` → `rdpq_detach_show`. Loop: as v1 Task 6/7b (accumulator, catch-up cap 4), reading ports 1 and 2 into `input_t in[2]` once per frame in the normal build, `game->autoplay(state, in)` per tick in the autoplay build, `game->update(state, in)`, then `clear_edges` on both. Start is read (`pressed.start`) into a `bool start_pressed` latch but unused until Task 2. The active game is `GAMES[0]` in the normal build and `GAMES[game_index_by_name(AUTOPLAY_GAME)]` in the autoplay build; call `init`, `set_high_score(0)`, then `start((uint32_t)get_ticks() | 1)`.

- [ ] **Step 8: Verify**

Run: `make test` — expected: `test_prng`, `test_sfx`, `test_brick` binaries all `ok`, `0 failure(s)` each.
Run: `make frames GAME=brick` — expected: frames as v1 plus placeholder text boxes and `text ...` lines on stdout.
Run: `make rom && make rom-autoplay GAME=brick && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task1 8` — expected: identical to the v1 Task 8 play capture (HUD, grid, paddle, ball).

- [ ] **Step 9: Commit**

```bash
git add -A src tests tools Makefile
git commit -m "v2 Task 1: game framework, draw API, Brick on the descriptor"
```

---

### Task 2: App state, games menu, pause menu, registry, ROM rename

**Files:**
- Create: `src/app_state.c`, `src/app_state.h`, `src/menu.c`, `src/menu.h`, `tests/test_app.c`
- Modify: `src/n64/app.c`, `tools/framedump.c` (accept `menu` and `pause` as names), `README.md` (one line: ROM is now `games.z64`)

**Interfaces:**
- Consumes: `GAMES[]`, `GAME_COUNT`, `game_desc_t`, `input_t`, `draw_t`, `sfx_queue_t`.
- Produces:

```c
/* src/app_state.h */
typedef enum { APP_MENU, APP_SETTINGS, APP_GAME, APP_PAUSE } app_screen_t;
typedef struct {
    app_screen_t screen;
    int game_index;            /* active or last active game */
    int menu_row;              /* 0..GAME_COUNT (last row = SETTINGS) */
    int pause_row;             /* 0 = RESUME, 1 = QUIT TO MENU */
    int repeat_ticks;          /* key-repeat timer for menu navigation */
    int8_t last_nav;           /* last vertical direction held, for repeat */
    sfx_queue_t sfx;           /* framework sounds (menu move/select) */
    bool autoplay;             /* autoplay build: no menu, no pause */
    bool high_score_dirty;     /* set when a game's high score rose after game over; cleared by the adapter after saving */
} app_t;

void app_init(app_t *a, bool autoplay, int autoplay_game);           /* inits every game; menu or straight into a game */
void app_update(app_t *a, const input_t in[GAME_MAX_PLAYERS], bool start_pressed, uint32_t seed);
void app_render(const app_t *a, const draw_t *d);
music_track_id_t app_track(const app_t *a);                           /* MUSIC_MENU on menu/settings, the game's track otherwise (also while paused) */
sfx_id_t app_next_sfx(app_t *a);                                      /* drains framework sounds first, then the active game's queue */
```

`seed` is passed by the adapter each tick (`(uint32_t)get_ticks() | 1`) and only consumed when a game is started.

- [ ] **Step 1: Failing tests (`tests/test_app.c`)**

- init → `APP_MENU`, `menu_row == 0`; `app_track == MUSIC_MENU`.
- `dpad_y = -1` for one tick moves to row 1 and queues `SFX_MENU_MOVE`; holding it: no further move for 17 more ticks, a move on the 18th, then every 8 ticks; row clamps at `GAME_COUNT` (SETTINGS) and at 0; `stick_y = -200` behaves like the D-pad.
- A on row 0 → `APP_GAME`, `game_index == 0`, Brick in TITLE, `app_track == MUSIC_BRICK`, `SFX_MENU_SELECT` queued.
- In a game, `start_pressed` → `APP_PAUSE` with `pause_row == 0`; the game does not tick while paused (Brick's `ticks` unchanged over 10 ticks); `start_pressed` again → `APP_GAME`; A on row 0 → `APP_GAME`; `dpad_y = -1` then A → `APP_MENU` with `menu_row` still pointing at Brick.
- A on the SETTINGS row → `APP_SETTINGS` (placeholder screen); A there (row BACK) → `APP_MENU`.
- Autoplay mode: `app_init(a, true, 0)` → `APP_GAME` immediately; `start_pressed` is ignored.
- Game over bookkeeping: drive Brick with autoplay to game over via `app_update` (up to 60,000 ticks); `high_score_dirty` becomes true once.
- `app_next_sfx` returns framework sounds before game sounds and `SFX_NONE` when both are empty.

- [ ] **Step 2: Implement `menu.c` and `app_state.c`**

Menu geometry (spec 3.7): header "GAMES" `DRAW_FONT_BIG` left at (16, 30); rule `rect(16, 36, 304, 37, COLOR_DARK)`; rows at baselines `62 + 24 * i` for `i` in 0..6 (six games in registry order, then "SETTINGS"); the selected row draws `rect(16, base - 20, 304, base + 6, COLOR_BLUE)` and its label in `DRAW_TEXT_LIGHT`, others in `DRAW_TEXT_DARK`, labels at x 24. With fewer than six games registered (Task 2 has one), the SETTINGS row still sits at index `GAME_COUNT`.

Navigation: `nav = dpad_y != 0 ? dpad_y : (stick_y >= 128 ? 1 : stick_y <= -128 ? -1 : 0)`; on a change of `nav` from 0 move immediately and set `repeat_ticks = 18`; while held, decrement and move when it reaches 0, then reset to 8. Up (`+1`) decreases the row index.

Pause overlay (drawn over the frozen game): "PAUSED" centered at baseline 120; rows "RESUME" (150) and "QUIT TO MENU" (176) centered, with the highlight bar `rect(96, base - 20, 224, base + 6, COLOR_BLUE)` on the selected row. Start while paused resumes.

Placeholder settings screen for this task: header "SETTINGS" like the menu header and a single "BACK" row at baseline 86, highlighted; A or B returns to the menu. Task 4 replaces it.

Game over bookkeeping: after each game tick, if `is_over` just became true and `get_high_score` exceeds the value recorded at `start`, set `high_score_dirty`.

- [ ] **Step 3: Adapter and frame dumper**

Adapter: own an `app_t`; per frame read `start_pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1).start` (latched, cleared after a tick like the edges); call `app_update` per tick and `app_render` per frame; ignore `app_track`/`app_next_sfx` until Task 4 (drain and discard sounds so queues never fill). Autoplay build: `app_init(&app, true, game_index_by_name(AUTOPLAY_GAME))`.

Frame dumper: names `menu` (render the menu after 0, 30, 60 ticks of holding down) and `pause` (start Brick, tick 300, press Start, render) in addition to game names.

- [ ] **Step 4: Verify**

Run: `make test` (new `test_app` binary green).
Run: `make frames GAME=menu && make frames GAME=pause` — expected: `build/frames/menu/frame-0030.png` shows the header rule, a blue bar on the second row; `build/frames/pause/frame-0000.png` shows the Brick field with two placeholder boxes centered.
Run: `make rom && scripts/ares-shot.sh games.z64 build/shots/v2-task2 4` — expected: the games menu with "GAMES", the rule, "BRICK" highlighted in a blue bar, "SETTINGS" below.
Run: `make rom-autoplay GAME=brick && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task2-auto 8` — expected: Brick playing.

- [ ] **Step 5: Commit**

```bash
git add -A src tests tools README.md
git commit -m "v2 Task 2: app state machine, games menu, pause menu, games.z64"
```

---
### Task 3: Chiptune synth, notation, sequencer, sound effects, tunes

**Files:**
- Create: `src/synth.h`, `src/synth.c`, `src/music.h`, `src/music.c`, `src/music_data.c`, `tools/musicdump.c`, `tests/test_synth.c`, `tests/test_music.c`
- Modify: `Makefile` (`music` target; `tools/musicdump.c` may use `<stdio.h>`, the synth and music modules may not)

**Interfaces:**
- Consumes: `sfx_id_t`, `music_track_id_t`.
- Produces:

```c
/* src/music.h */
enum { MUSIC_CHANNELS = 4, MUSIC_MAX_EVENTS = 512 };
typedef enum { CH_PULSE1 = 0, CH_PULSE2, CH_TRIANGLE, CH_NOISE } music_channel_t;
typedef struct { uint8_t note; uint8_t len; } music_event_t;   /* note: 0 rest, 1..127 MIDI note, 250 K, 251 S, 252 H, 253 O; len in steps 1..255 */
typedef struct { uint8_t duty; uint8_t gain; uint8_t decay; uint8_t sustain; } music_chparms_t; /* duty 12/25/50, gain 0..100, decay steps, sustain 0..100 */
typedef struct {
    int bpm;
    int steps;                                   /* total steps per loop (equal on all channels) */
    music_chparms_t parms[MUSIC_CHANNELS];
    music_event_t events[MUSIC_CHANNELS][MUSIC_MAX_EVENTS];
    int nevents[MUSIC_CHANNELS];
} music_track_t;
typedef struct { const char *name; int bpm; const char *pattern[MUSIC_CHANNELS]; } music_src_t;

extern const music_src_t MUSIC_SRC[MUSIC_TRACK_COUNT];   /* src/music_data.c, indexed by music_track_id_t */

/* Parses one pattern string into a channel. Returns false and fills err (if errcap > 0) on bad tokens
 * or when a bar delimited by '|' does not contain exactly 16 steps. */
bool music_parse_channel(const char *pattern, music_chparms_t *parms, music_event_t *events, int *nevents, int *steps, char *err, size_t errcap);
/* Parses all four channels; fails if their step counts differ. */
bool music_parse(const music_src_t *src, music_track_t *out, char *err, size_t errcap);
/* MIDI note (0..127) to frequency in Q8.8 Hz. A4 (69) = 440 << 8. */
uint32_t music_note_freq_q8(int midi);
```

```c
/* src/synth.h */
typedef struct { uint32_t phase, inc; uint8_t duty; int vol; int target; int decay_per_sample; } synth_pulse_t; /* internal, shown for size */
typedef struct synth_s {
    int sample_rate;
    const music_track_t *track; int step_samples; int step_pos; int step_index; int ev_index[4]; int ev_left[4];
    /* channel oscillators, envelope state, noise LFSR, sfx voice, volume, enable flags ... (implementation-defined) */
    bool music_on, sfx_on; int volume;   /* volume 0..10 */
} synth_t;

void synth_init(synth_t *s, int sample_rate);
void synth_set_track(synth_t *s, const music_track_t *t);   /* NULL silences music; restarts from step 0 */
void synth_set_volume(synth_t *s, int vol);                 /* 0..10 */
void synth_set_music_enabled(synth_t *s, bool on);
void synth_set_sfx_enabled(synth_t *s, bool on);
void synth_play_sfx(synth_t *s, sfx_id_t id);
void synth_render(synth_t *s, int16_t *out, int nframes);   /* mono, signed 16-bit */
```

**Notation** (as spec 3.8): tokens separated by spaces; `|` marks a bar and the parser checks that every bar has exactly 16 steps; header tokens `duty=12|25|50`, `gain=N`, `decay=N` (steps), `sustain=N` (percent) may appear only before the first note; note tokens are a letter `A`..`G`, an optional `#`, an octave digit `0`..`8`, and an optional `:len` (default 1); `-` is a rest (`-:len`); noise tokens `K`, `S`, `H`, `O`. MIDI note = 12 * (octave + 1) + semitone index (C=0 … B=11), so C4 = 60 and A4 = 69. Frequencies: table of the 12 notes of octave 4 in Q8.8 (`C4 = 66977` … `B4 = 126445`, i.e. `261.63 … 493.88` Hz times 256) shifted by `octave - 4`. Defaults: duty 50, gain 60, decay 0, sustain 100.

**Synth behavior**:
- Pulse: phase Q16; output +1/-1 by duty; per-note envelope: full level at note-on, linear decay over `decay` steps to `sustain` percent, then hold; note-off (a rest, or the next event) drops to 0 immediately.
- Triangle: 32-step staircase from the phase's top 5 bits, no envelope.
- Noise: 15-bit LFSR `bit = (r ^ (r >> tap)) & 1; r = (r >> 1) | (bit << 14)` with tap 1 (long) or 6 (short); percussion presets: K = tap 1, 16 samples per LFSR shift, 90 ms linear decay; S = tap 1, 4 samples, 120 ms; H = tap 6, 1 sample, 30 ms; O = tap 6, 1 sample, 150 ms.
- Sequencer: `step_samples = sample_rate * 15 / bpm` (a sixteenth at `bpm`); at each step boundary each channel consumes its event list; at the end of the loop it restarts.
- Sound-effect voice: a pulse with a linear pitch sweep from `f0` to `f1` over `ms`, plus an optional noise burst; presets:

| id | f0 Hz | f1 Hz | ms | duty | noise | notes |
|---|---|---|---|---|---|---|
| SFX_BOUNCE | 880 | 660 | 50 | 50 | none | |
| SFX_HIT | 1320 | 1320 | 40 | 25 | none | |
| SFX_CLEAR | 523, 659, 784 | | 3 x 60 | 50 | none | three notes in sequence (C5 E5 G5) |
| SFX_FOOD | 660, 990 | | 2 x 50 | 50 | none | |
| SFX_SHOT | 200 | 100 | 60 | 12 | H 30 ms | |
| SFX_EXPLODE | 120 | 40 | 200 | 12 | K 200 ms | |
| SFX_POINT | 1047 | 1047 | 120 | 50 | none | |
| SFX_MERGE | 440 | 880 | 80 | 25 | none | |
| SFX_GAME_OVER | 523, 440, 349, 262 | | 4 x 150 | 50 | none | descending |
| SFX_MENU_MOVE | 1500 | 1500 | 20 | 12 | none | |
| SFX_MENU_SELECT | 880 | 1320 | 60 | 50 | none | |

- Mixing per sample: `pulse1 * gain1 + pulse2 * gain2 + triangle * gain3 + noise * gain4` scaled so that all channels at full gain sum to about 24000, plus the sfx voice at 8000, times `volume / 10`, clipped to int16. `music_on == false` silences the four channels; `sfx_on == false` silences the voice.

- [ ] **Step 1: Failing tests**

`tests/test_music.c`: `music_note_freq_q8(69) == 440 << 8`; `music_note_freq_q8(57) == 220 << 8`; parsing `"duty=25 gain=40 C4:4 -:4 E4:8 | G4:16"` yields parms duty 25 gain 40, 4 events (`60,4`, `0,4`, `64,8`, `67,16`), 32 steps; a bar with 15 steps fails with an error mentioning `bar`; an unknown token fails; a header after a note fails; `music_parse` of every `MUSIC_SRC` entry succeeds (loop over `MUSIC_TRACK_COUNT`) and all four channels report the same step count; `MUSIC_SRC[MUSIC_MENU].bpm == 100`.

`tests/test_synth.c`: after `synth_init(&s, 22050)` and a one-channel track containing `A4:16` on PULSE1 (other channels `-:16`), rendering one second yields a signal whose sign changes about 880 times (accept 860..900) and whose peak exceeds 8000; with `synth_set_volume(&s, 0)` the same render is all zeros; `synth_set_music_enabled(false)` silences music but `synth_play_sfx(SFX_HIT)` still produces non-zero samples in the first 40 ms and zeros after 100 ms; each `sfx_id_t` from 1 to `SFX_COUNT - 1` renders at least one non-zero sample; rendering 10 seconds of every `MUSIC_SRC` track never produces a sample outside int16 range (implicitly true) and produces non-silence in every 500 ms window.

- [ ] **Step 2: Implement `music.c`, `synth.c`**

Per the behavior above. All arithmetic integer; use `uint64_t` intermediates for `inc = (freq_q8 << 8) / sample_rate` style computations. No `<stdio.h>` in these two files.

- [ ] **Step 3: The tunes (`src/music_data.c`)**

Copy these strings exactly. Every bar between `|` has 16 steps; the parser enforces it.

```c
#include "music.h"

/* MENU: 100 BPM, C major, calm. Chords C | C | Am | Am | F | F | G | G. */
static const char *const MENU_P1 =
    "duty=50 gain=55 decay=6 sustain=50 "
    "E5:8 G5:4 E5:4 | D5:8 C5:8 | C5:8 E5:4 A4:4 | B4:8 A4:8 | "
    "A4:8 C5:4 F5:4 | E5:8 D5:8 | D5:8 G5:4 D5:4 | E5:16";
static const char *const MENU_P2 =
    "duty=25 gain=40 "
    "C4:2 E4:2 G4:2 C5:2 E5:2 C5:2 G4:2 E4:2 | C4:2 E4:2 G4:2 C5:2 E5:2 C5:2 G4:2 E4:2 | "
    "A3:2 C4:2 E4:2 A4:2 C5:2 A4:2 E4:2 C4:2 | A3:2 C4:2 E4:2 A4:2 C5:2 A4:2 E4:2 C4:2 | "
    "F3:2 A3:2 C4:2 F4:2 A4:2 F4:2 C4:2 A3:2 | F3:2 A3:2 C4:2 F4:2 A4:2 F4:2 C4:2 A3:2 | "
    "G3:2 B3:2 D4:2 G4:2 B4:2 G4:2 D4:2 B3:2 | G3:2 B3:2 D4:2 G4:2 B4:2 G4:2 D4:2 B3:2";
static const char *const MENU_TRI =
    "gain=70 "
    "C2:8 C2:8 | C2:8 G2:8 | A2:8 A2:8 | A2:8 E2:8 | F2:8 F2:8 | F2:8 C3:8 | G2:8 G2:8 | G2:8 D2:8";
static const char *const MENU_NOISE =
    "gain=25 "
    "-:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | "
    "-:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2";

/* BRICK: 130 BPM, A minor pentatonic riff, bouncy. Chords Am | Am | G | G | Am | Am | F | G. */
static const char *const BRICK_P1 =
    "duty=50 gain=65 decay=2 sustain=60 "
    "A4:2 C5:2 D5:2 E5:2 D5:2 C5:2 A4:2 G4:2 | A4:2 -:2 A4:2 C5:2 E5:4 D5:2 C5:2 | "
    "A4:2 C5:2 D5:2 E5:2 D5:2 C5:2 A4:2 G4:2 | G4:2 A4:2 C5:2 A4:2 G4:4 E4:4 | "
    "E5:2 G5:2 A5:2 G5:2 E5:2 D5:2 C5:2 D5:2 | E5:4 -:2 E5:2 D5:2 C5:2 A4:4 | "
    "A4:2 C5:2 D5:2 E5:2 D5:2 C5:2 A4:2 G4:2 | A4:4 -:4 A4:2 G4:2 A4:4";
static const char *const BRICK_P2 =
    "duty=25 gain=40 decay=1 sustain=40 "
    "-:2 E4:2 -:2 E4:2 -:2 E4:2 -:2 E4:2 | -:2 E4:2 -:2 E4:2 -:2 E4:2 -:2 E4:2 | "
    "-:2 D4:2 -:2 D4:2 -:2 D4:2 -:2 D4:2 | -:2 D4:2 -:2 D4:2 -:2 D4:2 -:2 D4:2 | "
    "-:2 E4:2 -:2 E4:2 -:2 E4:2 -:2 E4:2 | -:2 E4:2 -:2 E4:2 -:2 E4:2 -:2 E4:2 | "
    "-:2 C4:2 -:2 C4:2 -:2 C4:2 -:2 C4:2 | -:2 D4:2 -:2 D4:2 -:2 D4:2 -:2 D4:2";
static const char *const BRICK_TRI =
    "gain=75 "
    "A2:2 C3:2 E3:2 A3:2 G3:2 E3:2 C3:2 A2:2 | A2:2 C3:2 E3:2 A3:2 G3:2 E3:2 C3:2 A2:2 | "
    "G2:2 B2:2 D3:2 G3:2 F3:2 D3:2 B2:2 G2:2 | G2:2 B2:2 D3:2 G3:2 F3:2 D3:2 B2:2 G2:2 | "
    "A2:2 C3:2 E3:2 A3:2 G3:2 E3:2 C3:2 A2:2 | A2:2 C3:2 E3:2 A3:2 G3:2 E3:2 C3:2 A2:2 | "
    "F2:2 A2:2 C3:2 F3:2 E3:2 C3:2 A2:2 F2:2 | G2:2 B2:2 D3:2 G3:2 F3:2 D3:2 B2:2 G2:2";
static const char *const BRICK_NOISE =
    "gain=40 "
    "K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | "
    "K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 S:2 S:2 S:2 S:2";

/* BLOCKS: 140 BPM, E minor with a raised leading tone, driving. Chords Em | Am | B | Am | Dm | C | G | B. */
static const char *const BLOCKS_P1 =
    "duty=50 gain=65 decay=3 sustain=55 "
    "E5:4 B4:2 C5:2 D5:4 C5:2 B4:2 | A4:4 A4:2 C5:2 E5:4 D5:2 C5:2 | B4:6 C5:2 D5:4 E5:4 | C5:4 A4:4 A4:8 | "
    "-:2 D5:4 F5:2 A5:4 G5:2 F5:2 | E5:6 C5:2 E5:4 D5:2 C5:2 | B4:4 D5:4 C5:4 A4:4 | D#5:4 B4:4 E5:8";
static const char *const BLOCKS_P2 =
    "duty=25 gain=38 decay=1 sustain=50 "
    "E4:2 B4:2 E4:2 B4:2 E4:2 B4:2 E4:2 B4:2 | A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 | "
    "B3:2 F#4:2 B3:2 F#4:2 B3:2 F#4:2 B3:2 F#4:2 | A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 | "
    "D4:2 A4:2 D4:2 A4:2 D4:2 A4:2 D4:2 A4:2 | C4:2 G4:2 C4:2 G4:2 C4:2 G4:2 C4:2 G4:2 | "
    "G3:2 D4:2 G3:2 D4:2 G3:2 D4:2 G3:2 D4:2 | B3:2 F#4:2 B3:2 F#4:2 B3:2 F#4:2 B3:2 F#4:2";
static const char *const BLOCKS_TRI =
    "gain=80 "
    "E2:2 E2:2 E3:2 E2:2 E2:2 E2:2 E3:2 E2:2 | A2:2 A2:2 A3:2 A2:2 A2:2 A2:2 A3:2 A2:2 | "
    "B2:2 B2:2 B3:2 B2:2 B2:2 B2:2 B3:2 B2:2 | A2:2 A2:2 A3:2 A2:2 A2:2 A2:2 A3:2 A2:2 | "
    "D2:2 D2:2 D3:2 D2:2 D2:2 D2:2 D3:2 D2:2 | C2:2 C2:2 C3:2 C2:2 C2:2 C2:2 C3:2 C2:2 | "
    "G2:2 G2:2 G3:2 G2:2 G2:2 G2:2 G3:2 G2:2 | B2:2 B2:2 B3:2 B2:2 B2:2 B2:2 B3:2 B2:2";
static const char *const BLOCKS_NOISE =
    "gain=45 "
    "K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 S:2 S:2 S:2 S:2 | "
    "K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 S:2 S:2 S:2 S:2";

/* SNAKE: 120 BPM, F major, staccato and playful. Chords F | C | F | C | Bb | F | C | F. */
static const char *const SNAKE_P1 =
    "duty=50 gain=60 decay=1 sustain=30 "
    "F4:1 -:1 A4:1 -:1 C5:2 -:2 A4:1 -:1 F4:1 -:1 G4:2 -:2 | A4:1 -:1 A4:1 -:1 G4:2 -:2 F4:2 -:2 E4:2 -:2 | "
    "F4:1 -:1 A4:1 -:1 C5:2 -:2 D5:1 -:1 C5:1 -:1 A4:2 -:2 | G4:2 -:2 E4:2 -:2 F4:4 -:4 | "
    "A#4:1 -:1 A#4:1 -:1 A4:2 -:2 G4:1 -:1 A4:1 -:1 A#4:2 -:2 | C5:1 -:1 C5:1 -:1 A4:2 -:2 F4:2 -:2 G4:2 -:2 | "
    "A4:1 -:1 C5:1 -:1 D5:2 -:2 C5:1 -:1 A4:1 -:1 G4:2 -:2 | F4:2 -:2 C4:2 -:2 F4:4 -:4";
static const char *const SNAKE_P2 =
    "duty=12 gain=32 "
    "A3:8 C4:8 | E4:8 G4:8 | A3:8 C4:8 | E4:8 G4:8 | D4:8 F4:8 | A3:8 C4:8 | E4:8 G4:8 | A3:8 C4:8";
static const char *const SNAKE_TRI =
    "gain=75 "
    "F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 | C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 | "
    "F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 | C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 | "
    "A#2:2 A#3:2 A#2:2 A#3:2 A#2:2 A#3:2 A#2:2 A#3:2 | F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 | "
    "C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 | F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 F2:2 F3:2";
static const char *const SNAKE_NOISE =
    "gain=35 "
    "K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | "
    "K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2";

/* PONG: 110 BPM, sparse call and answer over a pedal tone. */
static const char *const PONG_P1 =
    "duty=50 gain=55 decay=2 sustain=20 "
    "E5:2 -:2 -:4 E5:2 -:2 -:4 | E5:2 -:2 -:4 E5:2 -:2 -:4 | D5:2 -:2 -:4 D5:2 -:2 -:4 | D5:2 -:2 -:4 D5:2 -:2 -:4 | "
    "C5:2 -:2 -:4 C5:2 -:2 -:4 | C5:2 -:2 -:4 C5:2 -:2 -:4 | D5:2 -:2 -:4 D5:2 -:2 -:4 | E5:2 -:2 G5:2 -:2 E5:2 -:2 -:4";
static const char *const PONG_P2 =
    "duty=25 gain=45 decay=2 sustain=20 "
    "-:4 B4:2 -:2 -:4 B4:2 -:2 | -:4 B4:2 -:2 -:4 B4:2 -:2 | -:4 A4:2 -:2 -:4 A4:2 -:2 | -:4 A4:2 -:2 -:4 A4:2 -:2 | "
    "-:4 G4:2 -:2 -:4 G4:2 -:2 | -:4 G4:2 -:2 -:4 G4:2 -:2 | -:4 A4:2 -:2 -:4 A4:2 -:2 | -:4 B4:2 -:2 -:4 B4:2 -:2";
static const char *const PONG_TRI =
    "gain=60 "
    "E2:16 | E2:16 | D2:16 | D2:16 | C2:16 | C2:16 | D2:16 | E2:16";
static const char *const PONG_NOISE =
    "gain=30 "
    "-:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2 | "
    "-:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2";

/* PARACHUTE: 150 BPM, tense; chromatic bass ostinato, siren fifths. */
static const char *const PARA_P1 =
    "duty=50 gain=55 decay=0 "
    "A4:4 E5:4 A4:4 E5:4 | A4:4 E5:4 A4:4 E5:4 | A#4:4 F5:4 A#4:4 F5:4 | A4:4 E5:4 A4:4 E5:4 | "
    "A4:4 E5:4 A4:4 E5:4 | A#4:4 F5:4 A#4:4 F5:4 | A4:2 E5:2 A4:2 E5:2 A4:2 E5:2 A4:2 E5:2 | A4:2 E5:2 A4:2 E5:2 A#4:2 F5:2 A#4:2 F5:2";
static const char *const PARA_P2 =
    "duty=12 gain=40 decay=1 sustain=40 "
    "-:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 E4:2 | -:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 E4:2 | -:2 F4:2 -:2 F4:2 -:2 F#4:2 -:2 F4:2 | -:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 E4:2 | "
    "-:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 E4:2 | -:2 F4:2 -:2 F4:2 -:2 F#4:2 -:2 F4:2 | -:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 E4:2 | -:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 F#4:2";
static const char *const PARA_TRI =
    "gain=80 "
    "A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | "
    "A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | "
    "A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | "
    "A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2";
static const char *const PARA_NOISE =
    "gain=45 "
    "K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | "
    "K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 S:2 S:2 S:2 S:2";

/* 2048: 90 BPM, chill; Dm7 | Dm7 | G7 | G7 | Cmaj7 | Cmaj7 | Am7 | Am7. */
static const char *const G2048_P1 =
    "duty=50 gain=50 decay=8 sustain=45 "
    "A5:8 F5:8 | E5:16 | D5:8 B4:8 | G4:16 | E5:8 G5:8 | B5:16 | C6:8 A5:8 | E5:16";
static const char *const G2048_P2 =
    "duty=25 gain=42 "
    "D4:2 F4:2 A4:2 C5:2 F5:2 C5:2 A4:2 F4:2 | D4:2 F4:2 A4:2 C5:2 F5:2 C5:2 A4:2 F4:2 | "
    "G3:2 B3:2 D4:2 F4:2 B4:2 F4:2 D4:2 B3:2 | G3:2 B3:2 D4:2 F4:2 B4:2 F4:2 D4:2 B3:2 | "
    "C4:2 E4:2 G4:2 B4:2 E5:2 B4:2 G4:2 E4:2 | C4:2 E4:2 G4:2 B4:2 E5:2 B4:2 G4:2 E4:2 | "
    "A3:2 C4:2 E4:2 G4:2 C5:2 G4:2 E4:2 C4:2 | A3:2 C4:2 E4:2 G4:2 C5:2 G4:2 E4:2 C4:2";
static const char *const G2048_TRI =
    "gain=65 "
    "D2:16 | D2:16 | G2:16 | G2:16 | C2:16 | C2:16 | A2:16 | A2:16";
static const char *const G2048_NOISE =
    "gain=22 "
    "-:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 H:2 -:2 | "
    "-:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 O:2 -:2";

const music_src_t MUSIC_SRC[MUSIC_TRACK_COUNT] = {
    [MUSIC_MENU]      = { "menu",      100, { MENU_P1,   MENU_P2,   MENU_TRI,   MENU_NOISE } },
    [MUSIC_BRICK]     = { "brick",     130, { BRICK_P1,  BRICK_P2,  BRICK_TRI,  BRICK_NOISE } },
    [MUSIC_BLOCKS]    = { "blocks",    140, { BLOCKS_P1, BLOCKS_P2, BLOCKS_TRI, BLOCKS_NOISE } },
    [MUSIC_SNAKE]     = { "snake",     120, { SNAKE_P1,  SNAKE_P2,  SNAKE_TRI,  SNAKE_NOISE } },
    [MUSIC_PONG]      = { "pong",      110, { PONG_P1,   PONG_P2,   PONG_TRI,   PONG_NOISE } },
    [MUSIC_PARACHUTE] = { "parachute", 150, { PARA_P1,   PARA_P2,   PARA_TRI,   PARA_NOISE } },
    [MUSIC_2048]      = { "2048",      90,  { G2048_P1,  G2048_P2,  G2048_TRI,  G2048_NOISE } },
};
```

If the parser rejects a bar in any of these strings, fix the string minimally (a rest length) so the bar has 16 steps, and list every such fix in the commit message.

- [ ] **Step 4: `tools/musicdump.c` and `make music`**

The tool parses every track, renders two full loops of each at 22050 Hz into `build/music/<name>.wav`, and each sound effect into `build/music/sfx-<lowercase id>.wav` (0.6 s each), writing a standard 44-byte PCM WAV header. `make music` builds it with the host flags plus `src/synth.c src/music.c src/music_data.c` and runs it, then prints `afplay build/music/menu.wav` as a hint.

- [ ] **Step 5: Verify**

Run: `make test` (new `test_music`, `test_synth` green) and `make music` (14 + 11 WAV files; each tune file between 200 KB and 2 MB). Open `build/music/blocks.wav` with `afplay` for a human listen; the reviewer will do this.

- [ ] **Step 6: Commit**

```bash
git add -A src tests tools Makefile
git commit -m "v2 Task 3: chiptune synth, notation sequencer, sound effects, seven tunes"
```

---
### Task 4: Audio in the adapter, Settings screen, EEPROM save

**Files:**
- Create: `src/settings.h`, `src/settings.c`, `src/save.h`, `src/save.c`, `tests/test_settings.c`, `tests/test_save.c`
- Modify: `src/app_state.h`, `src/app_state.c`, `src/menu.c` (settings screen replaces the placeholder), `src/n64/app.c`, `Makefile` (`N64_ROM_SAVETYPE = eeprom4k`), `tools/framedump.c` (name `settings`)

**Interfaces:**
- Consumes: `synth_t`, `music_track_t`, `MUSIC_SRC`, `app_t`.
- Produces:

```c
/* src/settings.h */
typedef struct { bool music_on, sound_on; int volume; } settings_t;     /* volume 0..10; defaults on, on, 7 */
typedef struct { settings_t values; int row; bool changed; } settings_screen_t;  /* row 0 MUSIC, 1 SOUND, 2 VOLUME, 3 BACK */
void settings_defaults(settings_t *s);
void settings_screen_init(settings_screen_t *ss, const settings_t *current);
/* Returns true when the screen wants to close (BACK chosen). Queues SFX_MENU_MOVE / SFX_MENU_SELECT on sfx. */
bool settings_screen_update(settings_screen_t *ss, const input_t *in, sfx_queue_t *sfx);
void settings_screen_render(const settings_screen_t *ss, const draw_t *d);

/* src/save.h */
enum { SAVE_SIZE = 64, SAVE_MAGIC = 0x42524B31u, SAVE_VERSION = 1, SAVE_MAX_GAMES = 8 };
typedef struct { settings_t settings; int32_t high_scores[SAVE_MAX_GAMES]; } save_t;
void save_defaults(save_t *s);
void save_encode(const save_t *s, uint8_t out[SAVE_SIZE]);          /* big-endian fields, CRC32 (IEEE, poly 0xEDB88320) over bytes 0..59 stored at 60..63 */
bool save_decode(const uint8_t in[SAVE_SIZE], save_t *out);         /* false (and defaults) on bad magic, version, or CRC */
uint32_t save_crc32(const uint8_t *data, size_t len);
```

`app_t` gains `settings_t settings; settings_screen_t settings_screen; bool settings_dirty;` and `app_update` handles `APP_SETTINGS` through the settings screen, setting `settings_dirty` whenever a value changes. `app_high_scores(const app_t *, int32_t out[SAVE_MAX_GAMES])` copies every game's `get_high_score` in registry order (zeros beyond `GAME_COUNT`); `app_apply_save(app_t *, const save_t *)` pushes settings and high scores in at boot.

- [ ] **Step 1: Failing tests**

`tests/test_save.c`: `save_crc32("123456789", 9) == 0xCBF43926u`; encode then decode round-trips defaults and a record with volume 3, music off, high scores `{63, 12000, 7, 11, 40, 4096, 0, 0}`; flipping any byte of the encoded record makes `save_decode` return false and yield defaults; a record with `SAVE_VERSION + 1` is rejected.
`tests/test_settings.c`: defaults on/on/7; down moves the row and queues `SFX_MENU_MOVE`; A on row 0 toggles music and sets `changed`; left/right on VOLUME clamp at 0 and 10, A on VOLUME cycles 7 → 8 … 10 → 0; A on BACK returns true; B anywhere returns true; render calls produce the expected four labels (capture through a recording `draw_t` in the test: check the strings "MUSIC: ON", "SOUND: ON", "VOLUME: 7", "BACK" are drawn and that the selected row's text uses `DRAW_TEXT_LIGHT`).
`tests/test_app.c` additions: entering SETTINGS from the menu, changing volume, BACK → `APP_MENU` and `settings_dirty`; `app_high_scores` after a Brick game over reports its score in slot 0; `app_apply_save` with high score 500 in slot 0 makes Brick's title show 500 (check `get_high_score`).

- [ ] **Step 2: Implement `settings.c`, `save.c`, app changes**

Settings screen geometry (spec 3.9): header "SETTINGS" like the menu header; rows at baselines 86, 110, 134, 158; labels built with `fmt_label` for VOLUME; the highlight bar as in the menu. Left/right come from `dpad_x` or `stick_x` beyond ±128 with the menu's repeat timing.

CRC32: standard table-free bitwise loop (8 iterations per byte with the reflected polynomial `0xEDB88320`), initial `0xFFFFFFFF`, final xor `0xFFFFFFFF`.

- [ ] **Step 3: Adapter**

Boot additions, in this order after `joypad_init`:

```c
    audio_init(22050, 4);
    synth_init(&synth, audio_get_frequency());
    for (int i = 0; i < MUSIC_TRACK_COUNT; i++) {
        char err[64];
        if (!music_parse(&MUSIC_SRC[i], &tracks[i], err, sizeof err)) assertf(false, "track %d: %s", i, err);
    }
    save_t save; save_defaults(&save);
    bool have_eeprom = eeprom_present() != EEPROM_NONE;
    if (have_eeprom) {
        static const eepfs_entry_t entries[] = { { "/save.dat", SAVE_SIZE } };
        if (eepfs_init(entries, 1) == 0) {
            if (!eepfs_verify_signature()) { eepfs_wipe(); }
            uint8_t raw[SAVE_SIZE];
            if (eepfs_read("/save.dat", raw, SAVE_SIZE) == 0) save_decode(raw, &save);
        } else have_eeprom = false;
    }
    app_apply_save(&app, &save);
    apply_settings();   /* synth volume / enables from app.settings */
```

Per frame: after ticking, if `app.settings_dirty || app.high_score_dirty` build a `save_t` from `app.settings` and `app_high_scores`, `save_encode`, `eepfs_write("/save.dat", raw, SAVE_SIZE)` when `have_eeprom`, clear both flags, and `apply_settings()`. Then set the synth track when `app_track(&app)` changed since the last frame (`synth_set_track(&synth, &tracks[id])`), drain `app_next_sfx` into `synth_play_sfx`, and fill audio:

```c
    while (audio_can_write()) {
        short *buf = audio_write_begin();
        int n = audio_get_buffer_length();
        synth_render(&synth, mono, n);            /* static int16_t mono[1024]; n is well below that */
        for (int i = 0; i < n; i++) { buf[2 * i] = mono[i]; buf[2 * i + 1] = mono[i]; }
        audio_write_end();
    }
```

Makefile: `N64_ROM_SAVETYPE = eeprom4k` inside the n64 block. Autoplay builds skip EEPROM writes (so verification runs never dirty the save) but still play music, which is harmless for screenshots.

- [ ] **Step 4: Verify**

Run: `make test` (all binaries green).
Run: `make frames GAME=settings` — the settings screen with four placeholder rows and a blue bar.
Run: `make rom && scripts/ares-shot.sh games.z64 build/shots/v2-task4-menu 4` — the menu.
Persistence check (reviewer, by hand in ares): start `games.z64`, open SETTINGS, set VOLUME to 3, BACK, quit ares, relaunch, open SETTINGS: VOLUME reads 3.
Audio check (reviewer, by ear in ares): the menu tune plays; selecting Brick switches to the Brick tune; hitting a brick clicks.

- [ ] **Step 5: Commit**

```bash
git add -A src tests tools Makefile
git commit -m "v2 Task 4: audio playback, settings screen, EEPROM save of settings and high scores"
```

---
### Task 5: Blocks

**Files:**
- Create: `src/games/blocks.h`, `src/games/blocks.c`, `tests/test_blocks.c`
- Modify: `src/games/registry.c` (add `&GAME_BLOCKS` second)

**Interfaces:**
- Consumes: framework headers, `prng.h`, `sfx.h`.
- Produces: `const game_desc_t GAME_BLOCKS` and, for tests, the public struct below.

```c
/* src/games/blocks.h */
enum { BLK_COLS = 10, BLK_ROWS = 20, BLK_CELL = 10, BLK_WELL_X0 = 110, BLK_WELL_Y0 = 24,
       BLK_DAS_DELAY = 16, BLK_DAS_REPEAT = 6, BLK_SOFT_DROP_TICKS = 2, BLK_FLASH_TICKS = 20 };
typedef enum { BLK_I = 0, BLK_O, BLK_T, BLK_S, BLK_Z, BLK_J, BLK_L, BLK_PIECE_COUNT } blk_piece_t;
typedef enum { BLK_ST_TITLE, BLK_ST_PLAY, BLK_ST_FLASH, BLK_ST_GAMEOVER } blk_state_t;
typedef struct {
    blk_state_t state;
    uint8_t cells[BLK_ROWS][BLK_COLS];      /* 0 empty, else piece + 1 (for color) */
    blk_piece_t piece, next;
    int rot, px, py;                        /* current piece rotation and 4x4 box origin (px column, py row) */
    uint8_t bag[BLK_PIECE_COUNT]; int bag_left;
    int score, lines, level, high_score;
    int gravity_ticks;                      /* ticks until the next gravity step */
    int das_dir, das_ticks;                 /* horizontal auto-repeat state */
    int soft_ticks;
    int flash_ticks; uint8_t flash_rows[4]; int flash_count;
    prng_t rng;
    sfx_queue_t sfx;
    uint32_t ticks;
} blocks_t;

extern const int8_t BLK_SHAPES[BLK_PIECE_COUNT][4][4][2];   /* [piece][rotation][cell][x,y] within a 4x4 box, y down */
extern const uint8_t BLK_GRAVITY[20];                         /* ticks per row by level-1; level 20+ uses index 19 */
uint32_t blocks_piece_color(blk_piece_t p);
bool blocks_fits(const blocks_t *g, blk_piece_t p, int rot, int px, int py);
blk_piece_t blocks_bag_next(blocks_t *g);                     /* 7-bag draw (tests) */
void blocks_spawn(blocks_t *g);                               /* tests: force a spawn of g->next */
void blocks_init(blocks_t *g); void blocks_start(blocks_t *g, uint32_t seed);
void blocks_update(blocks_t *g, const input_t in[GAME_MAX_PLAYERS]);
void blocks_render(const blocks_t *g, const draw_t *d);
void blocks_autoplay(const blocks_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_BLOCKS;
```

Shapes (x, y within the 4x4 box, y down), spawn at `px = 3, py = 0`, rotation 0:

```c
const int8_t BLK_SHAPES[BLK_PIECE_COUNT][4][4][2] = {
  /* I */ {{{0,1},{1,1},{2,1},{3,1}}, {{2,0},{2,1},{2,2},{2,3}}, {{0,2},{1,2},{2,2},{3,2}}, {{1,0},{1,1},{1,2},{1,3}}},
  /* O */ {{{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}},
  /* T */ {{{1,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{2,1},{1,2}}, {{1,0},{0,1},{1,1},{1,2}}},
  /* S */ {{{1,0},{2,0},{0,1},{1,1}}, {{1,0},{1,1},{2,1},{2,2}}, {{1,1},{2,1},{0,2},{1,2}}, {{0,0},{0,1},{1,1},{1,2}}},
  /* Z */ {{{0,0},{1,0},{1,1},{2,1}}, {{2,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{1,2},{2,2}}, {{1,0},{0,1},{1,1},{0,2}}},
  /* J */ {{{0,0},{0,1},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{1,2}}, {{0,1},{1,1},{2,1},{2,2}}, {{1,0},{1,1},{0,2},{1,2}}},
  /* L */ {{{2,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{1,2},{2,2}}, {{0,1},{1,1},{2,1},{0,2}}, {{0,0},{1,0},{1,1},{1,2}}},
};
const uint8_t BLK_GRAVITY[20] = { 48, 43, 38, 33, 28, 23, 18, 13, 8, 6, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2 };
```

Colors: I `COLOR_TEAL`, O `COLOR_YELLOW`, T `COLOR_PURPLE`, S `COLOR_GREEN`, Z `COLOR_RED`, J `COLOR_BLUE`, L `COLOR_ORANGE`.

Rules (spec 4.2), stated as the implementation must do them:
- `start`: keep `high_score`; clear the well; `prng_seed`; refill the bag; draw `next`; `state = TITLE`; score/lines 0; level 1.
- TITLE: `in[0].a` → spawn (`piece = next`, `next = bag`), `gravity_ticks = BLK_GRAVITY[level-1]`, `state = PLAY`.
- PLAY, per tick, in this order: (1) horizontal: `dir = dpad_x` or stick beyond ±128; on a new press move once and set `das_ticks = BLK_DAS_DELAY`; while held, count down and move when it reaches 0, then `das_ticks = BLK_DAS_REPEAT`; a move that does not fit is skipped. (2) rotation: `a` → rot+1, `b` → rot+3 (mod 4), trying `px` offsets 0, -1, +1, -2, +2; the first that fits wins, otherwise no rotation. (3) hard drop: `dpad_y == +1` or `stick_y >= 128` edge (must return below +64 between drops): move down while it fits, then lock. (4) soft drop: `dpad_y == -1` or `stick_y <= -128` held: every `BLK_SOFT_DROP_TICKS` ticks move down one if it fits (and reset `gravity_ticks`). (5) gravity: decrement `gravity_ticks`; at 0 reset it and move down one row if it fits, else lock.
- Lock: write the piece into `cells` (value `piece + 1`), push `SFX_HIT`; collect full rows; if any: `state = FLASH`, `flash_ticks = BLK_FLASH_TICKS`, push `SFX_CLEAR`; else spawn.
- FLASH: count down; at 0 remove the rows (shift everything above down), add `score += (40, 100, 300, 1200)[n-1] * level`, `lines += n`, `level = 1 + lines / 10`, spawn.
- Spawn: if the new piece does not fit at `(3, 0)` → `high_score = max`, push `SFX_GAME_OVER`, `state = GAMEOVER`.
- GAMEOVER: `in[0].a` → `start` with the same seed handling (re-seed from `ticks`).
- Rendering: well border `rect(108, 22, 212, 226, COLOR_DARK)`, well interior `rect(110, 24, 210, 224, COLOR_TILE)`; each occupied cell as a 9x9 rectangle at `(110 + c*10, 24 + r*10)` in the piece color; the falling piece the same way; FLASH rows drawn in `COLOR_BG`. Right panel: "NEXT" HUD at (220, 40); preview cells 8 px at `(220 + x*8, 46 + y*8)`; "SCORE n" (220, 100), "LEVEL n" (220, 116), "LINES n" (220, 132) in the HUD font. Left: "HIGH n" HUD at (16, 40). TITLE overlay centered: "BLOCKS" 100, "HIGH SCORE n" 130, "PRESS A" 160. GAMEOVER overlay: "GAME OVER" 140, "SCORE n" 168, "PRESS A" 196.
- Autoplay: on TITLE/GAMEOVER press A; in PLAY, if no plan exists for the current piece, evaluate every rotation (0..3) and every `px` from -2 to 9 where the piece fits somewhere in the well after dropping straight down; simulate the drop and count `max_height` (rows from the floor to the highest occupied cell), `holes` (empty cells with an occupied cell above them in the same column), and `lines` (full rows); choose the minimum of `4 * max_height + 8 * holes - 10 * lines` (ties: lower rotation, then lower px); then each tick emit at most one action toward that plan: rotate (A) until the rotation matches, then move left/right (edge presses, one per tick with a gap tick between so DAS does not engage), then hard drop.

- [ ] **Step 1: Failing tests (`tests/test_blocks.c`)**

Concrete cases (each is a separate test function):
- every shape has exactly 4 cells inside the 4x4 box, and rotating 4 times returns the same cell set;
- `blocks_start` → TITLE, empty well, level 1; `in[0].a` → PLAY with `px == 3 && py == 0`;
- gravity: after 47 ticks `py == 0`, after 48 `py == 1` (level 1);
- DAS: `dpad_x = -1` moves to `px == 2` on the first tick, stays for 15 more ticks, is at `px == 1` on tick 17, `px == 0` on tick 23;
- wall kick: force `piece = BLK_I, rot = 1, px = -2, py = 5` (vertical I touching the left wall: its cells sit at column 0); press A; the piece rotates to `rot == 2` with `px` adjusted so it fits;
- soft drop: holding `dpad_y = -1` moves the piece down one row every 2 ticks;
- hard drop and lock: on an empty well, hard-drop an O piece from spawn; `cells[19][4] == BLK_O + 1 && cells[19][5] == BLK_O + 1`, `SFX_HIT` queued, a new piece has spawned;
- line clear: fill rows 16..19 in columns 1..9; force `piece = BLK_I, rot = 1, px = -2, py = 0` (vertical I in column 0); hard drop → `state == FLASH`, `SFX_CLEAR` queued; after `BLK_FLASH_TICKS` ticks `lines == 4`, `score == 1200`, `level == 1`, rows 16..19 empty;
- scoring: one line at level 1 → 40; at `level = 3` (set `lines = 20`) → 120;
- level up: `lines = 9` then clear one → `level == 2` and `gravity_ticks` reset to 43;
- game over: fill rows 1..19 completely, spawn → `state == GAMEOVER`, `SFX_GAME_OVER`, `high_score == score`;
- 7-bag: 14 draws contain each piece exactly twice;
- autoplay: from `start(0x1234567u)`, reaches GAMEOVER within 20,000 ticks with `lines >= 4`;
- descriptor: name "BLOCKS", `track == MUSIC_BLOCKS`, high score round-trip.

- [ ] **Step 2: Implement `blocks.c`**  (per the rules above; no other behavior)

- [ ] **Step 3: Verify**

`make test`; `make frames GAME=blocks` (frame 0300 shows a partially filled well, preview box, panel text placeholders); `make rom-autoplay GAME=blocks && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task5 5 20 60` (pieces stacking, score rising, eventually GAME OVER within a few minutes — capture 60 s only).

- [ ] **Step 4: Commit**

```bash
git add -A src tests
git commit -m "v2 Task 5: Blocks"
```

---

### Task 6: Snake

**Files:**
- Create: `src/games/snake.h`, `src/games/snake.c`, `tests/test_snake.c`
- Modify: `src/games/registry.c` (add `&GAME_SNAKE` third)

Layout correction to the spec: the HUD sits at baseline 26, so the grid is **36 x 24 cells** of 8 px at x 16..304, **y 32..224** (not 25 rows from y 24).

```c
/* src/games/snake.h */
enum { SNK_COLS = 36, SNK_ROWS = 24, SNK_CELL = 8, SNK_X0 = 16, SNK_Y0 = 32, SNK_MAX = SNK_COLS * SNK_ROWS,
       SNK_PERIOD_START = 8, SNK_PERIOD_MIN = 3, SNK_FOODS_PER_SPEEDUP = 5 };
typedef enum { SNK_ST_TITLE, SNK_ST_PLAY, SNK_ST_GAMEOVER } snk_state_t;
typedef struct { int8_t x, y; } snk_cell_t;
typedef struct {
    snk_state_t state;
    snk_cell_t body[SNK_MAX]; int len; int head;      /* ring buffer: head index, body grows from the tail */
    int8_t dir_x, dir_y, pending_x, pending_y;
    snk_cell_t food;
    int period, step_ticks, score, high_score, foods;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} snake_t;
bool snake_occupied(const snake_t *g, int x, int y);
void snake_place_food(snake_t *g);
void snake_init(snake_t *g); void snake_start(snake_t *g, uint32_t seed);
void snake_update(snake_t *g, const input_t in[GAME_MAX_PLAYERS]);
void snake_render(const snake_t *g, const draw_t *d);
void snake_autoplay(const snake_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_SNAKE;
```

Rules (spec 4.3): start with length 4 at cells `(18,12) (17,12) (16,12) (15,12)` (head first) heading right (`dir_x = 1`), `period = 8`, food placed by `snake_place_food` (random empty cell), TITLE until A. Input each tick sets `pending` from `dpad_x/y` or the stick beyond ±128, ignoring the exact reverse of `dir`. Every `period` ticks: `dir = pending`; new head = head + dir; wall (outside the grid) or body cell (excluding the tail cell that is about to move, unless the snake is growing) → `SFX_GAME_OVER`, GAMEOVER, high score; food → grow by one, `score++`, `foods++`, `SFX_FOOD`, place new food, `period = max(SNK_PERIOD_MIN, SNK_PERIOD_START - foods / SNK_FOODS_PER_SPEEDUP)`. Render: HUD "SCORE n" left at 16, "HIGH n" right at 304 (baseline 26); each body cell a 7x7 rectangle at `(16 + x*8, 32 + y*8)`, head `COLOR_BALL`, body `COLOR_BLUE`, food `COLOR_RED`; TITLE/GAMEOVER overlays like Blocks with the name "SNAKE". Autoplay: on each tick where a step is due next tick, pick among the three non-reverse directions the one that minimizes Manhattan distance to the food among moves that do not hit a wall or body; if none is safe, keep `dir`.

- [ ] **Step 1: Failing tests (`tests/test_snake.c`)**: start layout and TITLE; A → PLAY; no movement for 7 ticks then the head advances at tick 8; `dpad_y = +1` (up) turns the snake up at the next step and `dir_x = -1` while heading right is ignored; eating: place food directly ahead, step → `len == 5`, `score == 1`, `SFX_FOOD`, food moved to an empty cell; speed: after 5 foods `period == 7`, floor at 3 after 25; wall death: head at `(35, 12)` heading right → GAMEOVER with `SFX_GAME_OVER`; self collision: a U-turn into the body ends the game; tail chasing: moving into the cell the tail vacates this step is allowed; autoplay reaches GAMEOVER within 20,000 ticks with `score >= 5`; descriptor name "SNAKE", `MUSIC_SNAKE`.

- [ ] **Step 2: Implement `snake.c`**

- [ ] **Step 3: Verify**: `make test`; `make frames GAME=snake`; `make rom-autoplay GAME=snake && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task6 5 20 45`.

- [ ] **Step 4: Commit**: `git add -A src tests && git commit -m "v2 Task 6: Snake"`

---

### Task 7: Pong

**Files:**
- Create: `src/games/pong.h`, `src/games/pong.c`, `tests/test_pong.c`
- Modify: `src/games/registry.c` (add `&GAME_PONG` fourth)

```c
/* src/games/pong.h */
enum { PNG_PADDLE_W = 6, PNG_PADDLE_H = 40, PNG_P1_X = 20, PNG_P2_X = 294, PNG_BALL = 6,
       PNG_SPEED_SERVE = 768, PNG_SPEED_STEP = 64, PNG_SPEED_MAX = 2048, PNG_WIN_SCORE = 11,
       PNG_PADDLE_DIGITAL = 4, PNG_PADDLE_ANALOG_MAX = 6, PNG_AI_SPEED = 3, PNG_SERVE_DELAY = 60 };
typedef enum { PNG_ST_TITLE, PNG_ST_SERVE, PNG_ST_PLAY, PNG_ST_GAMEOVER } png_state_t;
typedef struct {
    png_state_t state;
    bool two_players; int title_row;          /* 0 = 1 PLAYER, 1 = 2 PLAYERS */
    int p1_y, p2_y;                            /* paddle top edges */
    int32_t ball_x, ball_y, ball_vx, ball_vy, ball_speed;  /* Q8.8 */
    int score1, score2, high_score;           /* high score = best margin of victory for player 1 */
    int serve_to;                              /* 1 or 2: who receives the next serve */
    int serve_ticks;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} pong_t;
void pong_ai(const pong_t *g, int paddle_y, int32_t toward_x, int *dy);   /* shared AI: dy in -PNG_AI_SPEED..+PNG_AI_SPEED */
void pong_init(pong_t *g); void pong_start(pong_t *g, uint32_t seed);
void pong_update(pong_t *g, const input_t in[GAME_MAX_PLAYERS]);
void pong_render(const pong_t *g, const draw_t *d);
void pong_autoplay(const pong_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_PONG;
```

Rules (spec 4.4): field x 16..304, y 24..228; paddles clamp to `[PLAY_Y0, PLAY_Y1 - PNG_PADDLE_H]`; paddle 1 from `in[0]` (`stick_y` → up to 6 px per tick, else `dpad_y` → 4 px; up is +y in input and means smaller screen y), paddle 2 from `in[1]` in two-player mode or from `pong_ai` otherwise. TITLE: up/down choose the row, A starts (`two_players = title_row == 1`), scores 0, `serve_to = 2`. SERVE: ball centered at (157, 123) for `PNG_SERVE_DELAY` ticks, then launched toward `serve_to` at `PNG_SPEED_SERVE` with a vertical component from bounce zone 2 or 4 (alternate per serve using the PRNG), sign toward that player. PLAY: Q8.8 motion with sub-steps as in Brick (2 when speed > 768); top/bottom walls reflect `vy` and push inside (`SFX_BOUNCE`); a paddle hit (ball moving toward that paddle and overlapping its rectangle) reflects `vx`, pushes the ball clear of the paddle, applies the Brick 7-zone table to the vertical hit offset (zones along the paddle's height, angle measured from horizontal), raises `ball_speed` by `PNG_SPEED_STEP` up to `PNG_SPEED_MAX`, and rescales the velocity to the new speed, `SFX_BOUNCE`; a ball whose rectangle leaves the field on the left scores for player 2 (right → player 1), `SFX_POINT`, `serve_to` = the player who conceded, back to SERVE; reaching `PNG_WIN_SCORE` → GAMEOVER, `SFX_GAME_OVER`, and if player 1 won, `high_score = max(high_score, score1 - score2)`. AI (`pong_ai`): if the ball moves toward the paddle, move toward the ball's center y at up to `PNG_AI_SPEED` px per tick, else drift toward the field center at 1 px per tick. Render: center line of 4 px dashes every 8 px at x 158..162 in `COLOR_DARK`; paddles `COLOR_DARK`; ball `COLOR_BALL`; scores in `DRAW_FONT_BIG` at (120, 50) and (200, 50) centered; TITLE overlay: "PONG" 90, rows "1 PLAYER" 130 and "2 PLAYERS" 156 with the menu-style highlight bar (x 96..224); GAMEOVER overlay: "PLAYER 1 WINS" / "PLAYER 2 WINS" (or "YOU WIN" / "CPU WINS" in one-player) at 140, "PRESS A" at 196. Autoplay: TITLE → A on row 0; both paddles by `pong_ai`.

- [ ] **Step 1: Failing tests (`tests/test_pong.c`)**: title rows and mode selection; serve delay and direction (`ball_vx > 0` when `serve_to == 2`); paddle movement and clamping for both players in two-player mode; wall bounce; paddle bounce raises speed by 64 and keeps the magnitude within 1/8; the seven zones give distinct `vy` values symmetric about the paddle center; a miss on the left scores for player 2 and switches `serve_to` to 1; first to 11 ends the game; margin high score only for player-1 wins; `pong_ai` tracks a ball moving toward it and idles otherwise; autoplay produces a GAMEOVER within 20,000 ticks; descriptor name "PONG", `players == 2`, `MUSIC_PONG`.

- [ ] **Step 2: Implement `pong.c`**

- [ ] **Step 3: Verify**: `make test`; `make frames GAME=pong`; `make rom-autoplay GAME=pong && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task7 5 30 90`; two-controller check by the reviewer in ares (map port 2 to keys, start `games.z64`, choose 2 PLAYERS, move both paddles).

- [ ] **Step 4: Commit**: `git add -A src tests && git commit -m "v2 Task 7: Pong"`

---
### Task 8: Parachute

**Files:**
- Create: `src/games/parachute.h`, `src/games/parachute.c`, `tests/test_parachute.c`
- Modify: `src/games/registry.c` (add `&GAME_PARACHUTE` fifth)

Layout corrections to the spec: the ground is the playfield bottom, y 228 (a trooper lands when its body bottom reaches 228, and landed troopers stand at y 220..228); helicopters fly at y 40..90 so they stay clear of the HUD.

```c
/* src/games/parachute.h */
enum { PAR_MAX_HELIS = 4, PAR_MAX_TROOPERS = 12, PAR_MAX_BULLETS = 4, PAR_MAX_LANDED = 8,
       PAR_AIM_MIN = -75, PAR_AIM_MAX = 75, PAR_AIM_ANALOG = 3, PAR_AIM_DIGITAL = 2,
       PAR_BULLET_SPEED = 4, PAR_FIRE_HOLD_TICKS = 12, PAR_HELI_SPEED = 1,
       PAR_TURRET_X0 = 150, PAR_TURRET_X1 = 170, PAR_TURRET_Y = 220, PAR_GROUND = 228,
       PAR_SIDE_LIMIT = 4, PAR_AUTOPLAY_CEASEFIRE = 60 };
typedef enum { PAR_ST_TITLE, PAR_ST_PLAY, PAR_ST_GAMEOVER } par_state_t;
typedef struct { bool alive; int x, y, dir, drops_left; } par_heli_t;
typedef struct { bool alive, chute; int x, y; } par_trooper_t;
typedef struct { bool alive; int32_t x, y, vx, vy; } par_bullet_t;   /* Q8.8 */
typedef struct { bool alive; int x; } par_landed_t;                    /* standing at y 220..228 */
typedef struct {
    par_state_t state;
    int aim;                                   /* degrees, -75..+75, 0 straight up */
    int fire_cooldown;
    par_heli_t helis[PAR_MAX_HELIS];
    par_trooper_t troopers[PAR_MAX_TROOPERS];
    par_bullet_t bullets[PAR_MAX_BULLETS];
    par_landed_t landed[PAR_MAX_LANDED];
    int landed_left, landed_right;
    int spawn_ticks, score, high_score;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} parachute_t;
/* Direction table for -75..+75 in 5-degree steps: index = (deg + 75) / 5; Q8.8 (sin, cos). */
extern const int16_t PAR_SIN[31];
extern const int16_t PAR_COS[31];
int par_aim_index(int deg);                    /* rounds to the nearest 5 degrees, clamps */
void parachute_init(parachute_t *g); void parachute_start(parachute_t *g, uint32_t seed);
void parachute_update(parachute_t *g, const input_t in[GAME_MAX_PLAYERS]);
void parachute_render(const parachute_t *g, const draw_t *d);
void parachute_autoplay(const parachute_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_PARACHUTE;
```

```c
const int16_t PAR_SIN[31] = { -247,-241,-232,-222,-210,-196,-181,-165,-147,-128,-108,-88,-66,-44,-22, 0,
                               22, 44, 66, 88, 108, 128, 147, 165, 181, 196, 210, 222, 232, 241, 247 };
const int16_t PAR_COS[31] = {   66,  88, 108, 128, 147, 165, 181, 196, 210, 222, 232, 241, 247, 252, 255, 256,
                               255, 252, 247, 241, 232, 222, 210, 196, 181, 165, 147, 128, 108, 88, 66 };
```

Rules (spec 4.5 with the corrections above):
- `start`: keep `high_score`; clear all entities; `aim = 0`; `spawn_ticks = 60`; `score = 0`; TITLE. A → PLAY.
- Aim: `stick_x` changes `aim` by `(stick_x * PAR_AIM_ANALOG) / 256` per tick, else `dpad_x * PAR_AIM_DIGITAL`; clamp to ±75. Barrel direction uses `k = par_aim_index(aim)`: `dx = PAR_SIN[k]`, `dy = -PAR_COS[k]` (Q8.8).
- Turret: base `rect(150, 220, 170, 228, COLOR_DARK)`; barrel squares for `k = 1..3` centered at `(160 + (6*k*dx >> 8), 220 + (6*k*dy >> 8))`, each `rect(cx-2, cy-2, cx+2, cy+2, COLOR_DARK)`.
- Fire: `in[0].a` edge, or `a_held` with `fire_cooldown == 0` (then `fire_cooldown = PAR_FIRE_HOLD_TICKS`), when fewer than 4 bullets are alive: bullet at the k=4 barrel position (Q8.8), velocity `(dx * PAR_BULLET_SPEED, dy * PAR_BULLET_SPEED)`, `score = max(0, score - 1)`, `SFX_SHOT`. Bullets move each tick and die outside the playfield. Bullet rectangle 3x3.
- Helicopters: `spawn_ticks--`; at 0, spawn in a free slot from a random side (`x = 0` moving right or `x = 304` moving left) at `y = 40 + prng_below(51)`, `drops_left = 2`, and reset `spawn_ticks = max(60, 150 - score)`. Move `dir * PAR_HELI_SPEED` per tick; die when `x < -16` or `x > 320`. While `40 < x < 280` and `drops_left > 0`, each tick with probability 1/90 (`prng_below(90) == 0`) drop a trooper at `(x + 5, y + 6)` with a chute. Body `rect(x, y, x+16, y+6, COLOR_DARK)`, rotor `rect(x+2, y-3, x+14, y-1, COLOR_DARK)`.
- Troopers: with chute descend 1 px per tick, without 4 px. Body `rect(x, y, x+6, y+8, COLOR_BLUE)`, chute `rect(x-3, y-8, x+9, y-2, COLOR_ORANGE)`. Landing when `y + 8 >= PAR_GROUND`: if `x + 6 > 150 && x < 170` → turret hit → game over; else `side = x < 150 ? left : right`; with chute → add to `landed` (standing at `x`), increment the side count, `landed_* == PAR_SIDE_LIMIT` → game over; without chute → `score += 1`, and every landed trooper whose 6 px wide rectangle overlaps the falling one dies with `score += 2` and its side count decremented; `SFX_EXPLODE` on any death.
- Bullet hits, checked after movement: helicopter body → heli dies, `score += 2`, `SFX_EXPLODE`; trooper body → dies, `+2`, `SFX_EXPLODE`; chute rectangle → `chute = false`, `SFX_HIT`. The bullet dies on any hit.
- Game over: `high_score = max`, `SFX_GAME_OVER`, GAMEOVER; A → `start`.
- HUD: "SCORE n" left, "HIGH n" right; overlays "PARACHUTE" / "GAME OVER" as in Blocks. Landed troopers drawn standing.
- Autoplay: TITLE/GAMEOVER → A. PLAY: target = the trooper with a chute that has the largest `y` (lowest on screen); if none, the helicopter nearest to x 160; if none, aim 0. Target angle in degrees = the table index whose direction best matches the vector from (160, 220) to the target center (pick the index minimizing `|dx_target * PAR_COS[i] + dy_target * PAR_SIN[i]|` with the sign check `dx_target * PAR_SIN[i] - dy_target * PAR_COS[i] > 0`... simpler and sufficient: choose `i` maximizing the dot product `dx_target * PAR_SIN[i] - dy_target * PAR_COS[i]` where `dy_target` is negative upward). Steer `stick_x = +256` if the target angle is above `aim + 2`, `-256` if below `aim - 2`, else 0 and press A once per 12 ticks. From `score >= PAR_AUTOPLAY_CEASEFIRE` on, stop firing (so the demonstration ends).

- [ ] **Step 1: Failing tests (`tests/test_parachute.c`)**: table symmetry (`PAR_SIN[i] == -PAR_SIN[30-i]`, `PAR_COS[i] == PAR_COS[30-i]`) and unit length (`sin² + cos²` within 64000..67000); `par_aim_index(-75) == 0`, `(0) == 15`, `(75) == 30`, `(-2) == 15`, `(3) == 16`; aim clamps at ±75 after 100 ticks of stick; firing costs a point but not below 0, queues `SFX_SHOT`, and a fifth bullet is refused; a bullet fired at aim 0 moves straight up 4 px per tick and dies above the playfield; a helicopter spawns when `spawn_ticks` runs out, moves 1 px per tick, and dies off screen; a bullet placed on a helicopter kills it for +2 and `SFX_EXPLODE`; a trooper with a chute falls 1 px per tick, without 4; shooting the chute clears it; a chuted trooper landing on the left increments `landed_left` and stands at y 220; four on one side → GAMEOVER; landing on the turret → GAMEOVER; a chuteless trooper landing on a standing trooper kills it (+2, count decremented) and itself (+1); autoplay reaches GAMEOVER within 20,000 ticks with `high_score >= 30`; descriptor name "PARACHUTE", `MUSIC_PARACHUTE`.

- [ ] **Step 2: Implement `parachute.c`**

- [ ] **Step 3: Verify**: `make test`; `make frames GAME=parachute`; `make rom-autoplay GAME=parachute && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task8 5 30 90`.

- [ ] **Step 4: Commit**: `git add -A src tests && git commit -m "v2 Task 8: Parachute"`

---

### Task 9: 2048

**Files:**
- Create: `src/games/g2048.h`, `src/games/g2048.c`, `tests/test_g2048.c`
- Modify: `src/games/registry.c` (add `&GAME_2048` sixth)

```c
/* src/games/g2048.h */
enum { G2048_N = 4, G2048_TILE = 40, G2048_GAP = 4, G2048_X0 = 72, G2048_Y0 = 32, G2048_AUTOPLAY_PERIOD = 10 };
typedef enum { G2048_ST_TITLE, G2048_ST_PLAY, G2048_ST_GAMEOVER } g2048_state_t;
typedef struct {
    g2048_state_t state;
    uint16_t board[G2048_N][G2048_N];
    int score, high_score;
    bool reached_2048;
    int8_t prev_dpad_x, prev_dpad_y; bool stick_armed;
    int autoplay_dir;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} g2048_t;
/* Slides one row toward index 0, merging equal neighbours once; returns true if it changed; adds merged values to *gained. */
bool g2048_slide_row(uint16_t row[G2048_N], int *gained);
/* dir: 0 left, 1 right, 2 up, 3 down. Returns true if the board changed. */
bool g2048_move(g2048_t *g, int dir);
void g2048_add_tile(g2048_t *g);                   /* 2 with probability 9/10 else 4, in a random empty cell */
bool g2048_can_move(const g2048_t *g);
uint32_t g2048_tile_color(uint16_t value);
void g2048_init(g2048_t *g); void g2048_start(g2048_t *g, uint32_t seed);
void g2048_update(g2048_t *g, const input_t in[GAME_MAX_PLAYERS]);
void g2048_render(const g2048_t *g, const draw_t *d);
void g2048_autoplay(const g2048_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_2048;
```

Rules (spec 4.6): `start` keeps `high_score`, clears the board, adds two tiles, TITLE; A → PLAY. Input: a move triggers on a D-pad edge (`dpad_x/y` non-zero while the previous tick's was zero) or when the stick crosses ±128 while `stick_armed` (re-armed when both stick axes are inside ±64). `g2048_move` maps every line of the board onto a temporary row in the direction of travel, calls `g2048_slide_row`, writes back, sums `gained` into `score`, and if anything changed calls `g2048_add_tile`, pushes `SFX_MERGE` when `gained > 0`, sets `reached_2048` (and pushes `SFX_CLEAR` the first time) when a 2048 tile exists; afterwards if `!g2048_can_move` → `high_score = max`, `SFX_GAME_OVER`, GAMEOVER; A on GAMEOVER → `start`. Colors: 2 `COLOR_TILE` and 4 `0xE8D8B8` with dark text; 8 `COLOR_ORANGE`, 16 `0xD96A2F`, 32 `COLOR_RED`, 64 `0xB03A3A`, 128 `COLOR_YELLOW`, 256 `0xC9A518`, 512 `COLOR_GREEN`, 1024 `COLOR_BLUE`, 2048 `COLOR_PURPLE`, larger `COLOR_DARK`, all with light text. Render: `rect(72, 32, 248, 208, COLOR_DARK)`; cell `(76 + c*44, 36 + r*44)` 40x40 in the tile color (empty: `COLOR_TILE`); the value centered at `x + 20` in `DRAW_FONT_BIG` at baseline `y + 28` for values below 1000, else `DRAW_FONT_HUD` at baseline `y + 25`; HUD "SCORE n" left, "HIGH n" right; "2048!" centered at baseline 224 once reached; overlays "2048" / "GAME OVER" as in Blocks. Number formatting uses `fmt_int` (see Task 1). Autoplay: every `G2048_AUTOPLAY_PERIOD` ticks emit a D-pad edge in the rotation down, left, down, right; if the previous move did not change the board, advance the rotation; TITLE/GAMEOVER → A.

- [ ] **Step 1: Failing tests (`tests/test_g2048.c`)**: `slide_row`: `[2,2,2,2] → [4,4,0,0]` gained 8; `[2,0,2,4] → [4,4,0,0]` gained 4; `[4,4,4,0] → [8,4,0,0]` gained 8; `[2,4,8,16]` unchanged returns false; `g2048_move` right on a board with `[2,0,0,2]` in row 0 gives `[0,0,0,4]`; a changing move adds exactly one tile (count non-zero cells before and after), a non-changing move adds none; `add_tile` over 1000 seeds yields 4 between 60 and 140 times; `can_move` false on a checkerboard of 2/4 without empties, true when any pair matches; game over transition with `SFX_GAME_OVER`; `reached_2048` and `SFX_CLEAR` when two 1024 tiles merge; stick moves require re-arming; autoplay from the fixed seed reaches GAMEOVER within 200,000 ticks with `score > 100`; descriptor name "2048", `MUSIC_2048`.

- [ ] **Step 2: Implement `g2048.c`**

- [ ] **Step 3: Verify**: `make test`; `make frames GAME=2048`; `make rom-autoplay GAME=2048 && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task9 5 30 120`.

- [ ] **Step 4: Commit**: `git add -A src tests && git commit -m "v2 Task 9: 2048"`

---

### Task 10: Collection README, evidence, final review

**Files:**
- Modify: `README.md`
- Create: `docs/superpowers/plans/evidence/v2-*.png`

- [ ] **Step 1: README** — retitle to "Retro Games for Nintendo 64"; keep the v1 sections and add: the games list with one line each; a per-game controls table (columns: game, move, action, other); the menu and pause controls (Start = pause menu with Resume / Quit to menu); Settings (Music, Sound, Volume, saved to EEPROM with high scores); Music (original chiptunes rendered by an in-repo synth; `make music` to render WAVs; `afplay build/music/<name>.wav`); the ROM names `games.z64` and `games-autoplay.z64` with `make rom-autoplay GAME=<name>`; trademark note that Blocks is an original implementation of the falling-block genre and not affiliated with Tetris.

- [ ] **Step 2: Evidence** — capture `games.z64` menu at 4 s, then each game's autoplay ROM at 20 s (six captures), plus the settings screen from `make frames GAME=settings`; downscale to 768 px into `docs/superpowers/plans/evidence/v2-<name>.png`.

- [ ] **Step 3: Final verification** — `make clean && make test && make music && make rom && for g in brick blocks snake pong parachute 2048; do make rom-autoplay GAME=$g && cp games-autoplay.z64 build/games-autoplay-$g.z64; done`, all green; a human play-through of every game in ares by the reviewer.

- [ ] **Step 4: Commit**

```bash
git add README.md docs/superpowers/plans/evidence
git commit -m "v2 Task 10: collection README and evidence"
```

---

## Review checklist (Claude, after every task)

1. `git status` clean, one new commit with the task's message; diff read in full.
2. `make test` rerun locally; purity grep over every pure module: `grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/*.c src/*.h src/games/*.c src/games/*.h` returns nothing.
3. Frame dumps and ares captures viewed for every visual change; music listened to (`afplay`) for Task 3 and Task 4.
4. Findings to `docs/superpowers/plans/reviews/v2-task-N.md`; fixes via `grok -c`; push only when clean.
