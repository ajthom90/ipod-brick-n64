# Modern Games Implementation Plan (v3)

> **For agentic workers:** Executed one task at a time by the Grok Build CLI, driven by Claude, who writes each task prompt, reviews the commit, and only then hands out the next task. Steps use checkbox (`- [ ]`) syntax. Keep every single command under four minutes. Capture screens only with `scripts/ares-shot.sh`.

**Goal:** Add Hopper, Flap, and Runner to `games.z64` with a scrolling menu, a twelve-slot save record, and three new tunes.

**Architecture:** Unchanged from v2: pure game modules behind `game_desc_t`, drawing through `draw_t`, tested on the host, self-playing for verification. The framework grows a scroll window in the menu and four more high-score slots.

**Tech Stack:** as v2.

**Spec:** `docs/superpowers/specs/2026-09-14-modern-games-design.md` (v3), over `docs/superpowers/specs/2026-09-08-retro-collection-design.md` (v2).

## Global Constraints

Identical to the v2 plan's Global Constraints (`docs/superpowers/plans/2026-09-08-retro-collection.md`): purity of modules, host flags, exclusive rectangles, palette, determinism with the fixed seed `0x1234567u`, Start owned by the framework, ROM names, ares only, one commit per task, no pushes, no plan or spec edits by workers. Additional palette entry: scrollbar track `0xB8B2A8`.

---

### Task 1: Scrolling menu, save record v2, three tunes

**Files:**
- Modify: `src/menu.h`, `src/menu.c`, `src/app_state.h`, `src/app_state.c`, `src/save.h`, `src/save.c`, `src/game.h` (track ids), `src/music_data.c`, `tools/framedump.c`, `tests/test_app.c`, `tests/test_save.c`, `tests/test_music.c`

**Interfaces:**
- Produces: `int menu_scroll_for(int row, int scroll, int total, int visible);` in `menu.h`; `int menu_scroll;` field in `app_t`; `SAVE_MAX_GAMES = 12`, `SAVE_VERSION = 2`; `MUSIC_HOPPER`, `MUSIC_FLAP`, `MUSIC_RUNNER` (after `MUSIC_2048`), `MUSIC_TRACK_COUNT = 10`; `MUSIC_SRC` entries for the three.

- [ ] **Step 1: Failing tests**

`tests/test_app.c`: `menu_scroll_for(0, 0, 10, 7) == 0`; `menu_scroll_for(6, 0, 10, 7) == 0`; `menu_scroll_for(7, 0, 10, 7) == 1`; `menu_scroll_for(9, 1, 10, 7) == 3`; `menu_scroll_for(2, 3, 10, 7) == 2`; `menu_scroll_for(5, 0, 5, 7) == 0`; after `app_init`, `menu_scroll == 0` and holding down until the row is `GAME_COUNT` leaves `menu_scroll == max(0, GAME_COUNT + 1 - 7)`. A recording `draw_t` test: with the real registry (seven rows: six games plus SETTINGS at the time of this task) no rectangle is drawn at x 296..304; the scrollbar test therefore calls `menu_render_rows(...)` directly with a synthetic `total = 10`, `scroll = 3` and checks a thumb rectangle at x 297..303 whose height is `172 * 7 / 10 = 120` and whose y is `43 + (172 - 120) * 3 / 3 = 95`.
`tests/test_save.c`: `SAVE_MAX_GAMES == 12`; encode/decode round-trip with twelve scores; a hand-built version-1 record (magic, version 1, settings, eight scores `{63, 1360, 40, 11, 60, 3476, 0, 0}`, zero padding, correct CRC over bytes 0..59) decodes with those eight scores in slots 0..7 and zeros in 8..11; version 3 is rejected; the existing bad-CRC and flipped-byte tests still pass.
`tests/test_music.c`: `MUSIC_TRACK_COUNT == 10`; every entry parses with equal channel lengths; `MUSIC_SRC[MUSIC_HOPPER].bpm == 125`, `[MUSIC_FLAP].bpm == 115`, `[MUSIC_RUNNER].bpm == 160`.

- [ ] **Step 2: Implement**

Menu: split the existing drawing into `void menu_render_rows(const draw_t *d, const char *const *labels, int total, int row, int scroll)` used by both the games menu and the tests; apply `menu_scroll_for` after every highlight change in `app_state.c`; draw the scrollbar and narrow the highlight when `total > MENU_VISIBLE_ROWS`. Geometry per spec 3.1.

Save: per spec 3.2. `save_decode`: verify magic and CRC first; then `version == 1` reads eight scores, `version == 2` reads twelve, anything else fails.

Music: append the three ids and the three `MUSIC_SRC` entries below. Frame dumper: `menu` mode also writes `frame-0300.ppm` after 300 ticks of holding down.

```c
/* HOPPER: 125 BPM, G major, bouncy. Chords G | C | G | D | G | C | Em | D. */
static const char *const HOPPER_P1 =
    "duty=50 gain=60 decay=3 sustain=50 "
    "G4:2 B4:2 D5:4 B4:2 G4:2 A4:4 | C5:4 E5:2 C5:2 A4:4 G4:4 | G4:2 B4:2 D5:4 G5:2 D5:2 B4:4 | A4:4 F#4:2 A4:2 D5:8 | "
    "D5:2 B4:2 G4:4 B4:2 D5:2 G5:4 | E5:4 C5:2 E5:2 G5:4 E5:4 | E5:2 D5:2 B4:4 G4:2 B4:2 E5:4 | D5:4 C5:2 A4:2 F#4:4 D4:4";
static const char *const HOPPER_P2 =
    "duty=25 gain=38 decay=1 sustain=40 "
    "-:2 D4:2 -:2 D4:2 -:2 D4:2 -:2 D4:2 | -:2 E4:2 -:2 E4:2 -:2 E4:2 -:2 E4:2 | -:2 D4:2 -:2 D4:2 -:2 D4:2 -:2 D4:2 | -:2 F#4:2 -:2 F#4:2 -:2 F#4:2 -:2 F#4:2 | "
    "-:2 D4:2 -:2 D4:2 -:2 D4:2 -:2 D4:2 | -:2 E4:2 -:2 E4:2 -:2 E4:2 -:2 E4:2 | -:2 G4:2 -:2 G4:2 -:2 G4:2 -:2 G4:2 | -:2 F#4:2 -:2 F#4:2 -:2 F#4:2 -:2 F#4:2";
static const char *const HOPPER_TRI =
    "gain=75 "
    "G2:2 G3:2 G2:2 G3:2 G2:2 G3:2 G2:2 G3:2 | C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 | G2:2 G3:2 G2:2 G3:2 G2:2 G3:2 G2:2 G3:2 | D2:2 D3:2 D2:2 D3:2 D2:2 D3:2 D2:2 D3:2 | "
    "G2:2 G3:2 G2:2 G3:2 G2:2 G3:2 G2:2 G3:2 | C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 | E2:2 E3:2 E2:2 E3:2 E2:2 E3:2 E2:2 E3:2 | D2:2 D3:2 D2:2 D3:2 D2:2 D3:2 D2:2 D3:2";
static const char *const HOPPER_NOISE =
    "gain=40 "
    "K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | "
    "K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2";

/* FLAP: 115 BPM, F major, dotted rhythm, cheeky. Chords F | Bb | F | C | F | Bb | C | F. */
static const char *const FLAP_P1 =
    "duty=25 gain=60 decay=2 sustain=50 "
    "F4:3 A4:1 C5:3 A4:1 F4:3 G4:1 A4:4 | A#4:3 D5:1 F5:3 D5:1 A#4:3 C5:1 D5:4 | C5:3 A4:1 F4:3 A4:1 C5:3 D5:1 C5:4 | E4:3 G4:1 A#4:3 G4:1 E4:3 G4:1 C5:4 | "
    "F5:3 C5:1 A4:3 C5:1 F5:3 E5:1 F5:4 | D5:3 A#4:1 F4:3 A#4:1 D5:3 C5:1 D5:4 | G4:3 E4:1 C4:3 E4:1 G4:3 A#4:1 G4:4 | A4:3 F4:1 C4:3 F4:1 A4:3 G4:1 F4:4";
static const char *const FLAP_P2 =
    "duty=12 gain=35 "
    "A3:8 C4:8 | D4:8 F4:8 | A3:8 C4:8 | E4:8 G4:8 | A3:8 C4:8 | D4:8 F4:8 | E4:8 G4:8 | A3:8 C4:8";
static const char *const FLAP_TRI =
    "gain=75 "
    "F2:4 A2:4 C3:4 A2:4 | A#2:4 D3:4 F3:4 D3:4 | F2:4 A2:4 C3:4 A2:4 | C3:4 E3:4 G3:4 E3:4 | F2:4 A2:4 C3:4 A2:4 | A#2:4 D3:4 F3:4 D3:4 | C3:4 E3:4 G3:4 E3:4 | F2:4 A2:4 C3:4 A2:4";
static const char *const FLAP_NOISE =
    "gain=35 "
    "K:3 H:1 S:2 H:2 K:2 H:2 S:3 H:1 | K:3 H:1 S:2 H:2 K:2 H:2 S:3 H:1 | K:3 H:1 S:2 H:2 K:2 H:2 S:3 H:1 | K:3 H:1 S:2 H:2 K:2 H:2 S:3 H:1 | "
    "K:3 H:1 S:2 H:2 K:2 H:2 S:3 H:1 | K:3 H:1 S:2 H:2 K:2 H:2 S:3 H:1 | K:3 H:1 S:2 H:2 K:2 H:2 S:3 H:1 | K:3 H:1 S:2 H:2 K:2 H:2 S:3 H:1";

/* RUNNER: 160 BPM, A minor, driving. Chords Am | Am | F | G | Am | Am | F | E. */
static const char *const RUNNER_P1 =
    "duty=50 gain=62 decay=2 sustain=55 "
    "A4:2 A4:2 C5:2 A4:2 E5:2 C5:2 A4:2 G4:2 | A4:2 C5:2 E5:2 G5:2 E5:2 C5:2 A4:4 | F4:2 A4:2 C5:2 F5:2 C5:2 A4:2 F4:2 A4:2 | G4:2 B4:2 D5:2 G5:2 D5:2 B4:2 G4:4 | "
    "E5:2 E5:2 C5:2 E5:2 A5:2 E5:2 C5:2 A4:2 | A4:2 C5:2 E5:2 A5:2 G5:2 E5:2 C5:4 | F5:2 E5:2 C5:2 A4:2 F5:2 E5:2 C5:2 A4:2 | G#4:2 B4:2 E5:2 G#5:2 E5:2 B4:2 G#4:4";
static const char *const RUNNER_P2 =
    "duty=25 gain=36 decay=1 sustain=50 "
    "A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 | A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 | F3:2 C4:2 F3:2 C4:2 F3:2 C4:2 F3:2 C4:2 | G3:2 D4:2 G3:2 D4:2 G3:2 D4:2 G3:2 D4:2 | "
    "A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 | A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 | F3:2 C4:2 F3:2 C4:2 F3:2 C4:2 F3:2 C4:2 | E3:2 B3:2 E3:2 B3:2 E3:2 B3:2 E3:2 B3:2";
static const char *const RUNNER_TRI =
    "gain=80 "
    "A2:2 A2:2 A3:2 A2:2 A2:2 A2:2 A3:2 A2:2 | A2:2 A2:2 A3:2 A2:2 A2:2 A2:2 A3:2 A2:2 | F2:2 F2:2 F3:2 F2:2 F2:2 F2:2 F3:2 F2:2 | G2:2 G2:2 G3:2 G2:2 G2:2 G2:2 G3:2 G2:2 | "
    "A2:2 A2:2 A3:2 A2:2 A2:2 A2:2 A3:2 A2:2 | A2:2 A2:2 A3:2 A2:2 A2:2 A2:2 A3:2 A2:2 | F2:2 F2:2 F3:2 F2:2 F2:2 F2:2 F3:2 F2:2 | E2:2 E2:2 E3:2 E2:2 E2:2 E2:2 E3:2 E2:2";
static const char *const RUNNER_NOISE =
    "gain=45 "
    "K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | "
    "K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 S:2 S:2 S:2 S:2";
```

with `MUSIC_SRC` entries `[MUSIC_HOPPER] = { "hopper", 125, { HOPPER_P1, HOPPER_P2, HOPPER_TRI, HOPPER_NOISE } }`, `[MUSIC_FLAP] = { "flap", 115, { FLAP_P1, FLAP_P2, FLAP_TRI, FLAP_NOISE } }`, `[MUSIC_RUNNER] = { "runner", 160, { RUNNER_P1, RUNNER_P2, RUNNER_TRI, RUNNER_NOISE } }`. If the parser rejects a bar, change only a rest length so the bar has 16 steps and report it.

- [ ] **Step 3: Verify**: `make test`; `make music` (ten tunes); `make frames GAME=menu` (frame 0300 shows the last row highlighted; with only seven rows there is no scrollbar yet); `make rom && scripts/ares-shot.sh games.z64 build/shots/v3-task1 4` (menu unchanged).

- [ ] **Step 4: Commit**: `git add -A src tests tools && git commit -m "v3 Task 1: scrolling menu, save record v2, three tunes"`

---

### Task 2: Hopper

**Files:** create `src/games/hopper.h`, `src/games/hopper.c`, `tests/test_hopper.c`; modify `src/games/registry.c` (append `&GAME_HOPPER` seventh).

```c
/* src/games/hopper.h */
enum { HOP_PLAYER = 12, HOP_PLAT_W = 32, HOP_PLAT_H = 6, HOP_MAX_PLATS = 24,
       HOP_GRAVITY = 64, HOP_JUMP_VY = -1664, HOP_SPRING_VY = -2560,
       HOP_MOVE_ANALOG_MAX = 4, HOP_MOVE_DIGITAL = 3, HOP_CAMERA_LINE = 100,
       HOP_GAP_MIN = 40, HOP_GAP_RAND = 21, HOP_GAP_EXTRA_CAP = 20, HOP_MOVING_SPEED = 1,
       HOP_AUTOPLAY_STOP = 300 };
typedef enum { HOP_ST_TITLE, HOP_ST_PLAY, HOP_ST_GAMEOVER } hop_state_t;
typedef enum { HOP_PLAT_STATIC, HOP_PLAT_MOVING, HOP_PLAT_SPRING } hop_plat_type_t;
typedef struct { bool alive; int x, y; int dir; hop_plat_type_t type; } hop_plat_t;   /* y = world top */
typedef struct {
    hop_state_t state;
    int32_t px, py, vy;               /* Q8.8 world position of the player's top-left, vertical velocity */
    int camera_y, camera_start;       /* world px */
    hop_plat_t plats[HOP_MAX_PLATS];
    int top_y;                        /* world y of the highest generated platform */
    int score, high_score;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} hopper_t;
void hopper_generate_up_to(hopper_t *g, int world_y);   /* generate platforms until top_y <= world_y */
void hopper_init(hopper_t *g); void hopper_start(hopper_t *g, uint32_t seed);
void hopper_update(hopper_t *g, const input_t in[GAME_MAX_PLAYERS]);
void hopper_render(const hopper_t *g, const draw_t *d);
void hopper_autoplay(const hopper_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_HOPPER;
```

Rules: spec 4.1. Per tick in PLAY: move horizontally (wrap), `vy += HOP_GRAVITY`, `old_bottom = (py >> 8) + 12`, `py += vy`, `new_bottom = (py >> 8) + 12`; if `vy > 0`, find the highest platform with `old_bottom <= plat.y && new_bottom >= plat.y` and horizontal overlap (player x..x+12 against plat x..x+32, wrap-aware is not required): land (`py = (plat.y - 12) << 8`, `vy = type == SPRING ? HOP_SPRING_VY : HOP_JUMP_VY`, sound). Move moving platforms and reverse at the edges. Camera per spec; score; generate platforms up to `camera_y - 40`; recycle those below `camera_y + 260`. Death when `(py >> 8) - camera_y > 240`.

Tests (`tests/test_hopper.c`): start layout (player on the bottom platform, TITLE); A → PLAY; gravity accumulates (after 10 ticks of free fall vy == 640); landing on a static platform sets vy to `HOP_JUMP_VY` and queues `SFX_BOUNCE`; a spring sets `HOP_SPRING_VY` and queues `SFX_CLEAR`; the player passes upward through a platform without landing; wrap on both sides; camera moves up when the player rises above line 100 and never moves down; score increments per 10 px of camera rise; moving platforms reverse at the edges; generation keeps `top_y <= camera_y - 40` and every gap within 40..80; falling below the screen → GAMEOVER with `SFX_GAME_OVER` and high score; autoplay from the fixed seed reaches score 50 within 6,000 ticks and GAMEOVER within 60,000; descriptor "HOPPER", `MUSIC_HOPPER`.

Verify: `make test`; `make frames GAME=hopper`; `rm -rf build/autoplay && make rom-autoplay GAME=hopper && scripts/ares-shot.sh games-autoplay.z64 build/shots/v3-task2 5 20 60`. Commit: `git add -A src tests && git commit -m "v3 Task 2: Hopper"`.

---

### Task 3: Flap

**Files:** create `src/games/flap.h`, `src/games/flap.c`, `tests/test_flap.c`; modify `src/games/registry.c` (append `&GAME_FLAP` eighth).

```c
/* src/games/flap.h */
enum { FLP_BIRD = 10, FLP_BIRD_X = 80, FLP_GRAVITY = 77, FLP_FLAP_VY = -1280, FLP_TERMINAL = 1536,
       FLP_PIPE_W = 24, FLP_GAP = 64, FLP_GAP_MIN = 48, FLP_PIPE_SPACING = 90, FLP_SCROLL = 512,
       FLP_GROUND_Y = 220, FLP_MAX_PIPES = 6, FLP_CENTER_MIN = 70, FLP_CENTER_RAND = 111,
       FLP_AUTOPLAY_STOP = 40 };
typedef enum { FLP_ST_TITLE, FLP_ST_PLAY, FLP_ST_GAMEOVER } flp_state_t;
typedef struct { bool alive, scored; int32_t x; int center, gap; } flp_pipe_t;   /* x Q8.8 left edge */
typedef struct {
    flp_state_t state;
    int32_t by, vy;                  /* bird top Q8.8, velocity */
    flp_pipe_t pipes[FLP_MAX_PIPES];
    int score, high_score;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} flap_t;
int flap_gap_for_score(int score);
void flap_init(flap_t *g); void flap_start(flap_t *g, uint32_t seed);
void flap_update(flap_t *g, const input_t in[GAME_MAX_PLAYERS]);
void flap_render(const flap_t *g, const draw_t *d);
void flap_autoplay(const flap_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_FLAP;
```

Rules: spec 4.2. The gap is fixed per pipe at spawn time from `flap_gap_for_score(score)`.

Tests: gravity and terminal velocity; A sets vy to `FLP_FLAP_VY` and queues `SFX_HIT`; ceiling clamp; pipes scroll 2 px per tick and spawn 90 px apart with centers in 70..180; scoring exactly once per pipe with `SFX_POINT`; `flap_gap_for_score(0) == 64`, `(100) == 54`, `(300) == 48`; collision with a top pipe, a bottom pipe, and the ground each end the game with `SFX_EXPLODE`; autoplay from the fixed seed reaches score 20 and GAMEOVER within 20,000 ticks; descriptor "FLAP", `MUSIC_FLAP`.

Verify: `make test`; `make frames GAME=flap`; `rm -rf build/autoplay && make rom-autoplay GAME=flap && scripts/ares-shot.sh games-autoplay.z64 build/shots/v3-task3 5 20 60`. Commit: `git add -A src tests && git commit -m "v3 Task 3: Flap"`.

---

### Task 4: Runner

**Files:** create `src/games/runner.h`, `src/games/runner.c`, `tests/test_runner.c`; modify `src/games/registry.c` (append `&GAME_RUNNER` ninth).

```c
/* src/games/runner.h */
enum { RUN_PLAYER_W = 10, RUN_PLAYER_H = 14, RUN_PLAYER_X = 60, RUN_GRAVITY = 77,
       RUN_JUMP_VY = -1536, RUN_JUMP_CUT_VY = -512, RUN_SPEED_START = 768, RUN_SPEED_STEP = 26,
       RUN_SPEED_STEP_TICKS = 300, RUN_SPEED_MAX = 1792, RUN_MAX_BUILDINGS = 8,
       RUN_W_MIN = 60, RUN_W_RAND = 101, RUN_ROOF_MIN = 120, RUN_ROOF_MAX = 200, RUN_ROOF_DELTA = 40,
       RUN_GAP_MIN = 24, RUN_GAP_RAND = 41, RUN_METRE = 10, RUN_AUTOPLAY_STOP = 500, RUN_AUTOPLAY_HOLD = 12 };
typedef enum { RUN_ST_TITLE, RUN_ST_PLAY, RUN_ST_GAMEOVER } run_state_t;
typedef struct { bool alive; int32_t x; int w, roof; } run_building_t;   /* x Q8.8 left edge */
typedef struct {
    run_state_t state;
    int32_t py, vy;                   /* player top Q8.8, velocity */
    bool grounded;
    run_building_t buildings[RUN_MAX_BUILDINGS];
    int32_t speed, distance;          /* Q8.8 px per tick, Q8.8 px */
    int speed_ticks, score, high_score, last_point_score, autoplay_hold;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} runner_t;
void runner_init(runner_t *g); void runner_start(runner_t *g, uint32_t seed);
void runner_update(runner_t *g, const input_t in[GAME_MAX_PLAYERS]);
void runner_render(const runner_t *g, const draw_t *d);
void runner_autoplay(const runner_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_RUNNER;
```

Rules: spec 4.3. Per tick in PLAY: scroll buildings by `speed`; `distance += speed`; `score = (distance >> 8) / 10`; speed step every 300 ticks up to the cap; jump on `a` while grounded; short hop when `!a_held && vy < RUN_JUMP_CUT_VY`; gravity when airborne; landing and wall checks per spec; spawn and despawn buildings; `SFX_POINT` when `score / 100` increases.

Tests: start on the first building (roof 180, grounded, TITLE); A → PLAY; jump sets `RUN_JUMP_VY` with `SFX_MERGE`, and releasing A early cuts vy to `RUN_JUMP_CUT_VY`; gravity while airborne; landing snaps to the roof with `SFX_HIT`; speed rises by 26 after 300 ticks and caps at 1792; spawned buildings keep widths 60..160, gaps 24..64, roof deltas at most 40, roofs within 120..200; falling into a gap → GAMEOVER with `SFX_EXPLODE`; a wall hit → GAMEOVER; score is distance in metres and `SFX_POINT` fires at 100; autoplay from the fixed seed reaches 200 m and GAMEOVER within 60,000 ticks; descriptor "RUNNER", `MUSIC_RUNNER`.

Verify: `make test`; `make frames GAME=runner`; `rm -rf build/autoplay && make rom-autoplay GAME=runner && scripts/ares-shot.sh games-autoplay.z64 build/shots/v3-task4 5 20 60`. Commit: `git add -A src tests && git commit -m "v3 Task 4: Runner"`.

---

### Task 5: README, evidence, final verification

- README: add the three games to the list and the controls table (Hopper: stick or D-pad steers, wraps at the edges; Flap: A flaps; Runner: A jumps, release early for a short hop); extend the trademark note to say Hopper, Flap, and Runner are original implementations of their genres and are not affiliated with Doodle Jump, Flappy Bird, or Canabalt or their owners; note the scrolling menu.
- Evidence: `make frames GAME=menu` and copy `frame-0300.png` (scrolled to the bottom with the scrollbar) to `docs/superpowers/plans/evidence/v3-menu-bottom.png`; `games.z64` at 4 s; each new game's autoplay ROM at 20 s; downscale to 768 px into `docs/superpowers/plans/evidence/v3-<name>.png`.
- Final: `make clean && make test && make music && make rom`; commit `git add README.md docs/superpowers/plans/evidence && git commit -m "v3 Task 5: README and evidence"`. Claude tags `v3.0`.

---

## Review checklist

As v2, plus: the trademark grep now allows the four names only inside the README's trademark note.
