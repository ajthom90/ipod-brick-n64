# Retro Games Collection for Nintendo 64 — Design Spec (v2)

Date: 2026-09-08
Status: approved design, awaiting spec review
Builds on: `2026-09-08-ipod-brick-n64-design.md` (v1, the Brick game and its toolchain)

## 1. Goal

Turn the single-game ROM into a collection in the spirit of the iPod's "Games" menu: one ROM, a menu that lists six games, a shared framework so each game is a small host-tested C module, and the same Docker, ares, and autoplay verification pipeline as v1.

Games: **Brick** (existing), **Blocks** (falling tetrominoes), **Snake**, **Pong** (one or two players), **Parachute** (the iPod original), **2048**.

### In scope

- Shared game interface, common input struct, abstract draw API, deterministic random generator.
- Brick refactored onto the interface with no rule changes.
- iPod-style games menu; framework-owned pause menu with Resume and Quit to menu.
- Five new games with the rules in section 4, each with unit tests and an autoplay policy.
- Host frame dumps and autoplay ROM builds per game.
- An in-repo chiptune synthesizer with seven original tunes (menu plus one per game) and 8-bit sound effects, rendered to WAV on the host for review.
- A Settings screen (music, sound, volume) reached from the games menu.
- Persistence of settings and every game's high score in cartridge EEPROM.

### Out of scope

Sampled audio or tracker modules, sprites or textures beyond the existing font files, online features, more games than the six above, the storm animation in Parachute, Blocks features beyond the classic rule set (no hold piece, no ghost piece, no T-spin scoring).

## 2. Naming

The falling-block game is called **Blocks** in the menu and code; "Tetris" is a registered trademark and is not used anywhere in the repo. The ROM becomes `games.z64` (autoplay: `games-autoplay.z64`) with the ROM title "Games". The GitHub repository name stays as it is.

## 3. Framework

### 3.1 Files

```
src/game.h         input_t, draw_t, game_desc_t, shared constants
src/prng.h         xorshift32: prng_seed, prng_next, prng_below(n)
src/menu.c/.h      games menu and pause menu (framework screens)
src/games/brick.c/.h      (moved from src/brick.c, refactored onto game_desc_t)
src/games/blocks.c/.h
src/games/snake.c/.h
src/games/pong.c/.h
src/games/parachute.c/.h
src/games/g2048.c/.h
src/games/registry.c/.h   const game_desc_t *const GAMES[]; GAME_COUNT
src/synth.c/.h     4-channel chiptune synthesizer and sound-effect player (pure C, integer math)
src/music.c/.h     pattern notation parser and sequencer driving the synth; defines music_track_id_t (MUSIC_MENU, MUSIC_BRICK, MUSIC_BLOCKS, MUSIC_SNAKE, MUSIC_PONG, MUSIC_PARACHUTE, MUSIC_2048)
src/music_data.c   the seven tunes as notation strings
src/sfx.h          sfx_id_t, sfx_queue_t
src/settings.c/.h  settings screen (framework screen) and settings_t
src/save.c/.h      save_t layout, checksum, encode/decode (pure C); EEPROM I/O lives in the adapter
src/n64/app.c      adapter: draw_t over rdpq, input from ports 1 and 2, audio buffer filling, EEPROM, tick loop, screen switching
tools/musicdump.c  renders every tune and sound effect to WAV files for listening on the host
tools/framedump.c  draw_t over a pixel buffer; takes a game name
tests/harness.h    CHECK / RUN macros shared by all test files
tests/test_<game>.c, tests/test_menu.c, tests/test_prng.c
```

Every file under `src/games/`, `src/menu.c`, and `src/prng.h` follows the v1 purity rule: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, no floats, no libdragon, deterministic.

### 3.2 Input

```c
enum { GAME_MAX_PLAYERS = 2 };
typedef struct {
    int16_t stick_x, stick_y;   /* -256..+256, dead zone applied by the adapter, up is +y */
    int8_t  dpad_x, dpad_y;     /* -1, 0, +1; D-pad, with C-left/right/up/down as aliases */
    bool    a, b, z;            /* pressed this tick (edge) */
    bool    a_held, b_held;     /* level */
} input_t;
```

Start never reaches a game: the framework consumes it for the pause menu. Games receive `const input_t in[GAME_MAX_PLAYERS]`; single-player games read `in[0]`. The adapter fills `in[1]` from controller port 2, or zeros if none is connected.

### 3.3 Draw API

```c
typedef enum { DRAW_FONT_HUD = 1, DRAW_FONT_BIG = 2 } draw_font_t;   /* 12 px and 22 px Inter Bold */
typedef enum { DRAW_LEFT, DRAW_CENTER, DRAW_RIGHT } draw_align_t;
#define DRAW_TEXT_DARK  0x2C2C2Cu
#define DRAW_TEXT_LIGHT 0xE8E4DCu

typedef struct draw_s {
    void *ctx;
    void (*rect)(void *ctx, int x0, int y0, int x1, int y1, uint32_t rgb);              /* exclusive x1/y1 */
    void (*text)(void *ctx, draw_font_t font, draw_align_t align, int x, int y,
                 uint32_t rgb, const char *utf8);                                        /* y is the baseline */
} draw_t;
```

`x` is the left edge for `DRAW_LEFT`, the center for `DRAW_CENTER`, the right edge for `DRAW_RIGHT`. Only the two text colors above are valid; the adapter maps them to font styles 0 and 1. The N64 implementation uses `rdpq_fill_rectangle` in fill mode and `rdpq_text_printf`; the host implementation fills pixels and, for text, draws a light-grey placeholder box (6 px per character by 10 px for HUD, 12 px per character by 20 px for BIG, positioned by the alignment) so layout collisions are visible in frame dumps, and prints the string with its position to stdout.

### 3.4 Game descriptor

```c
typedef struct {
    const char *name;                                    /* menu label, upper case */
    int players;                                         /* 1 or 2 */
    void *state;                                         /* file-static struct owned by the game */
    void (*init)(void *st);                              /* once at boot: zero state, high score 0 */
    void (*start)(void *st, uint32_t seed);              /* on launch from the menu: reseed, new round, keep high score */
    void (*update)(void *st, const input_t in[GAME_MAX_PLAYERS]);   /* exactly one tick */
    void (*render)(const void *st, const draw_t *d);
    void (*autoplay)(const void *st, input_t in[GAME_MAX_PLAYERS]); /* AI input for tests and screenshots */
    bool (*is_over)(const void *st);                     /* true on that game's game-over screen */
    int  (*get_high_score)(const void *st);
    void (*set_high_score)(void *st, int value);         /* framework restores the saved value at boot */
    music_track_id_t track;                              /* tune to play while this game is active */
} game_desc_t;
```

Every game state embeds an `sfx_queue_t` (a ring of up to 8 `sfx_id_t` values). `update` pushes sound-effect ids into it; after each tick the framework drains the queue into the synth. Tests read the queue to assert that, for example, a brick hit queues `SFX_HIT`. The queue is the only audio-related thing a game knows about.

Each game launches into its own title state showing its name, "HIGH SCORE n", and "PRESS A" (or a mode choice for Pong), plays, and on game over shows "GAME OVER", the high score, and "PRESS A" to return to its title. Session high scores live inside each game's state and survive returning to the menu. The registry lists games in menu order: Brick, Blocks, Snake, Pong, Parachute, 2048.

### 3.5 Random numbers

`prng.h` implements xorshift32 on a `uint32_t` inside each game's state. `start` receives a seed from the adapter (`(uint32_t)get_ticks()` at launch, made odd) or a fixed seed in tests and frame dumps (`0x1234567u`). Autoplay results are therefore reproducible on the host and only vary on the N64 by launch time.

### 3.6 Framework flow (menu.c plus the adapter)

- Boot: `init` every game; show the games menu with Brick highlighted.
- Games menu: `dpad_y` or `stick_y` beyond ±128 moves the highlight, with key repeat (first repeat after 18 ticks, then every 8); A on a game calls `start(seed)` and switches to it, which also switches the music to that game's track; A on SETTINGS opens the settings screen (section 3.9). The menu and settings screens play the menu tune.
- In a game: each tick the adapter reads both controllers, then the game's `update`. Start opens the pause menu.
- Pause menu: overlay with "PAUSED", rows "RESUME" and "QUIT TO MENU"; up/down selects, A confirms, Start also resumes. The game is not ticked while paused. Quit returns to the games menu; the game's state is left as is until its next `start`.
- Autoplay build: `-DAUTOPLAY_GAME=<index>` boots straight into that game's `start(seed)` with `autoplay` feeding input per tick; Start and the menu are disabled. `make rom-autoplay GAME=snake` sets the index from the registry order (default `brick`).

### 3.7 Menu look

iPod "Games" screen: header "GAMES" in the big font, left-aligned at x 16, baseline 30, a 1 px rule in the text color at y 36 from x 16 to 304. Seven rows (the six games then SETTINGS) at baselines 62, 86, 110, 134, 158, 182, 206 in the big font at x 24. The highlighted row draws a filled bar x 16..304, y baseline-20..baseline+6 in `2E6DB4` with the label in `DRAW_TEXT_LIGHT`. Background `E8E4DC`. The pause menu draws "PAUSED" centered at baseline 120 and its two rows at 150 and 176 with the same highlight bar, over the frozen game.

### 3.8 Audio: chiptune synthesizer, tunes, and sound effects

**Synth** (`src/synth.c`): four music channels modelled on the NES sound chip plus one sound-effect voice, all integer math, rendering signed 16-bit mono samples at the rate the adapter requests (22050 Hz on the N64 and in the host tool):

- PULSE1 and PULSE2: square waves with a duty cycle of 12.5, 25, or 50 percent, Q16 phase accumulators.
- TRIANGLE: a 32-step triangle (4-bit amplitude steps), no envelope, used for bass.
- NOISE: a 15-bit linear-feedback shift register with a short-period mode, for drums.
- SFX voice: a pulse channel with a pitch sweep (start frequency, end frequency, duration) plus a noise burst, so one effect can be "blip" or "explosion".
- Envelope per note on the pulse channels: instant attack, linear decay to a sustain level over a per-track time, release on note end.
- Mixing: channels summed with per-channel gains, then a master volume from settings (0 to 10, linear), then clipped. Music and sound can be muted independently.

**Notation** (`src/music.c`): a tune is a tempo in beats per minute plus one pattern string per channel. A step is a sixteenth note. Tokens are separated by spaces; `|` is ignored and used to mark bars for readability.

- Pulse and triangle tokens: a note name with octave, optionally `:` and a length in steps, for example `C4`, `G#3:2`, `A2:4`. `-` is a rest; `-:4` a four-step rest. `~` after a note ties it to the next token of the same pitch.
- Noise tokens: `K` kick (low short burst), `S` snare (mid burst), `H` closed hat (very short high burst), `O` open hat, `-` rest, each optionally with `:len`.
- Per-channel header options at the start of the string: `duty=25`, `gain=60`, `decay=8`.
- Every channel pattern in a tune must have the same total number of steps; the tune loops.

The parser is pure C, runs once per track at boot, and is fully unit-tested. Note frequencies come from a 12-entry table for octave 4 shifted by octave.

**Tunes** (`src/music_data.c`), all original compositions, 8 or 16 bars each, composed as notation strings and reviewed by listening to the WAV renders:

| Track | Brief |
|---|---|
| Menu | 100 BPM, calm; PULSE2 arpeggiates I–vi–IV–V in C major, PULSE1 carries a slow lyrical melody, triangle roots, closed hats only |
| Brick | 130 BPM, bouncy; A minor pentatonic riff with call-and-response between the pulses, walking triangle bass, kick and snare on 1 and 3 |
| Blocks | 140 BPM, driving; original minor-key melody with Eastern European flavor (not Korobeiniki), off-beat pulse chords, four-on-the-floor kick |
| Snake | 120 BPM, playful; staccato major melody in F, bouncing octave bass, hats on every eighth |
| Pong | 110 BPM, sparse; a two-note "ping-pong" motif alternating between PULSE1 and PULSE2 over a held triangle pedal, snare on 2 and 4 |
| Parachute | 150 BPM, tense; chromatic triangle ostinato, siren-like open fifths in the pulses, driving kick |
| 2048 | 90 BPM, chill; wide arpeggios over a slow ii–V–I–vi cycle, long pulse notes, soft hats, no kick |

**Sound effects** (`src/sfx.h`): `SFX_BOUNCE` (paddle), `SFX_HIT` (brick, block lock), `SFX_CLEAR` (rising three-note arpeggio for line clear or level clear), `SFX_FOOD`, `SFX_SHOT`, `SFX_EXPLODE`, `SFX_POINT`, `SFX_MERGE`, `SFX_GAME_OVER` (four descending notes), `SFX_MENU_MOVE`, `SFX_MENU_SELECT`. Each is a preset for the SFX voice.

**Host tool** (`tools/musicdump.c`, `make music`): renders each tune twice through its loop and each sound effect to `build/music/<name>.wav` (16-bit mono 22050 Hz) and prints durations. Listen with `afplay build/music/menu.wav`. Tune review happens here before anything is heard in the emulator.

**Adapter**: `audio_init(22050, 4)` at boot; each frame, while `audio_can_write()` is true, fill the buffer returned by `audio_write_begin()` with `audio_get_buffer_length()` stereo frames from the synth (the mono sample written to both channels) and call `audio_write_end()`. Switching screens changes the track: the menu plays the menu tune, each game its own, and a track change restarts from the top.

**Tests**: token parsing including errors, note-to-frequency values (A4 = 440 Hz), a pulse wave at 440 Hz rendered at 22050 Hz has about 880 zero crossings per second (within 2 percent), every tune parses with equal channel lengths, every sound effect renders non-silent then silent, master volume 0 renders silence, and each game's rule tests assert the expected `sfx_queue_t` entries.

### 3.9 Settings screen

Reached from the SETTINGS row of the games menu. Rows in the big font at baselines 86, 110, 134, 158 with the same highlight bar: `MUSIC: ON` / `OFF`, `SOUND: ON` / `OFF`, `VOLUME: 7` (0 to 10), `BACK`. Up/down moves the highlight; A toggles MUSIC or SOUND; left/right, or A, changes VOLUME (A cycles upward and wraps); A on BACK returns to the games menu. Every change applies immediately, plays `SFX_MENU_MOVE` or `SFX_MENU_SELECT` as appropriate, and is saved. Defaults: music on, sound on, volume 7.

### 3.10 Persistence (EEPROM)

- ROM save type `eeprom4k` (`N64_ROM_SAVETYPE = eeprom4k` in the Makefile), 512 bytes.
- One record via libdragon's `eepromfs`: `eepfs_init` with a single entry `/save.dat` of 64 bytes; `eepfs_verify_signature` false at boot means a fresh or foreign cartridge, so `eepfs_wipe` and write defaults.
- `save_t` (`src/save.h`, pure C, host-tested): `uint32_t magic` (`0x42524B31`, "BRK1"), `uint16_t version` (1), `uint8_t music_on, sound_on, volume`, `int32_t high_scores[8]` (indexed by registry order, spare slots zero), `uint32_t crc32` over the preceding bytes. `save_encode` / `save_decode` handle byte order explicitly; a bad magic, version, or CRC decodes to defaults.
- The framework loads the record at boot, pushes each high score into its game with `set_high_score`, and writes the record when a setting changes or when a game reports a higher `get_high_score` after game over. Writes are rare and small; EEPROM wear is not a concern.
- The adapter reads and writes through `eepfs_read` / `eepfs_write`; if `eeprom_present()` returns `EEPROM_NONE` (an emulator with saves disabled) the game runs with defaults and skips writes.
- ares persists the EEPROM contents to a `.eep` file per ROM, so verification is: run, change a setting and set a high score, quit ares, relaunch, and capture the Settings screen and the game's title screen.

### 3.11 Timing, layout, and colors

Unchanged from v1: 320x240, 16 px margins (playfield x 16..304, y 24..228), 60 Hz NTSC / 50 Hz PAL fixed tick with a 4-tick catch-up cap, the v1 palette (`C4472A` red, `E07A1F` orange, `D4B01C` yellow, `3FA34D` green, `2E6DB4` blue, `7B4EA3` purple, `E8E4DC` background, `2C2C2C` dark, `1A1A1A` ball) plus `3AAFA9` teal for the Blocks I piece. HUD text baseline 26 in the HUD font. Q8.8 fixed point wherever sub-pixel motion is needed.

## 4. Games

### 4.1 Brick (refactor only)

Rules, layout, physics, and tests as in v1. Changes: input comes from `input_t` (`stick_x` → analog paddle, `dpad_x` → digital, `a` → launch/confirm), the PAUSE state and `pause` input are removed because the framework owns pausing, rendering moves into `brick.c` through `draw_t`, the ball is not drawn on the game-over screen, and the module exposes a `game_desc_t`. Existing tests are ported; behavior must not change otherwise. Sound effects: `SFX_BOUNCE` on a paddle hit, `SFX_HIT` on a brick, `SFX_CLEAR` on a level clear, `SFX_GAME_OVER`.

### 4.2 Blocks

- Well: 10 columns x 20 rows of 10 px cells at x 110..210, y 24..224. Well border: 2 px dark frame just outside. Right panel at x 220..304: "NEXT" label and a 4x4-cell preview at 8 px per cell, then "SCORE n", "LEVEL n", "LINES n" in the HUD font. Left panel at x 16..100: "HIGH n".
- Pieces and colors: I teal `3AAFA9`, O yellow `D4B01C`, T purple `7B4EA3`, S green `3FA34D`, Z red `C4472A`, J blue `2E6DB4`, L orange `E07A1F`. Each piece is defined as four rotation states in a 4x4 grid (classic orientations; spawn orientation flat, spawn position columns 3..6, rows 0..1).
- Randomizer: 7-bag (shuffle each bag with the PRNG); "next" preview shows the following piece.
- Controls: `dpad_x`/`stick_x` beyond ±128 move with auto-repeat (initial delay 16 ticks, then every 6); `a` rotates clockwise, `b` counter-clockwise; rotation tries horizontal kicks 0, -1, +1, -2, +2 and fails if none fits; `dpad_y`/`stick_y` down soft-drops (one row every 2 ticks while held); up hard-drops and locks immediately.
- Gravity: ticks per row by level from the table `{48, 43, 38, 33, 28, 23, 18, 13, 8, 6, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2}` (level 1 first; level 20 and beyond use 2). A piece locks when a gravity step is blocked. After locking, full rows clear (with a 20-tick flash of the cleared rows in the light color), rows above shift down, then the next piece spawns.
- Scoring: 40, 100, 300, 1200 points for 1 to 4 lines, multiplied by the level. Level = 1 + lines / 10.
- Game over: the new piece overlaps existing cells at spawn.
- Sound effects: `SFX_HIT` on lock, `SFX_CLEAR` on any line clear, `SFX_GAME_OVER`.
- Autoplay: for the current piece, evaluate every rotation and column, simulate the hard drop, and pick the placement minimizing `4 * max_height + 8 * holes - 10 * lines_cleared` (ties: leftmost, first rotation); then issue the moves and a hard drop.

### 4.3 Snake

- Grid: 36 x 25 cells of 8 px at x 16..304, y 24..224. Snake body `2E6DB4`, head `1A1A1A`, food `C4472A`, 1 px inner gap so cells read as segments.
- Start: length 4 at the center heading right; the snake advances one cell every `period` ticks where `period` starts at 8 and drops by 1 for every 5 foods eaten, minimum 3. The first move waits for A ("PRESS A" on the title).
- Input: `dpad_x/y` or `stick_x/y` beyond ±128 set the pending direction; reversing into the body is ignored; the pending direction applies at the next step (one turn per step).
- Food: placed with the PRNG in a random empty cell. Eating grows the snake by one and scores 1.
- Game over: the head enters a wall or a body cell. HUD: "SCORE n" left, "HIGH n" right.
- Sound effects: `SFX_FOOD` on eating, `SFX_GAME_OVER`.
- Autoplay: each step, choose the direction that reduces Manhattan distance to the food without stepping into a wall or the body; if none, any safe direction; if none, keep going.

### 4.4 Pong

- Field x 16..304, y 24..228 with a dashed center line (4 px dashes every 8 px). Paddles 6 x 40 px at x 20 (player 1) and x 294 (player 2), ball 6 x 6.
- Paddles move by `stick_y` (up to 6 px per tick) or `dpad_y` (4 px per tick), clamped to the field.
- Ball: Q8.8; serve speed 3.0 px per tick toward the player who was just scored on (first serve toward player 2), angle from the same 7-zone bounce table as Brick applied to the vertical hit offset; each paddle hit adds 0.25 px per tick up to 8.0; top and bottom walls reflect.
- Scoring: a ball leaving left or right scores for the other side; first to 11 wins and shows "PLAYER n WINS" (or "YOU WIN" / "CPU WINS" in single player) with "PRESS A".
- Modes: the title screen offers "1 PLAYER" and "2 PLAYERS" (up/down, A). In single player, player 2 is an AI that moves toward the ball's y at up to 3 px per tick while the ball travels toward it and drifts to center otherwise. The high score for Pong is the best margin of victory for player 1.
- Sound effects: `SFX_BOUNCE` on paddle and wall hits, `SFX_POINT` when a side scores, `SFX_GAME_OVER` at match end.
- Autoplay: both sides use the AI; `autoplay` picks "1 PLAYER" on the title.

### 4.5 Parachute

- Turret: base rectangle 20 x 8 at x 150..170, y 220..228; barrel drawn as three 4 x 4 squares stepping outward along the aim direction from the base center (160, 220). Aim angle in whole degrees from -75 (left) to +75 (right), changed by `stick_x` (up to 3 degrees per tick) or `dpad_x` (2 degrees per tick). A 31-entry Q8.8 table of (sin, cos) for -75..+75 in 5-degree steps provides direction vectors; the angle is rounded to the nearest 5 degrees for drawing and shooting.
- Bullets: 3 x 3 dark squares, 4 px per tick along the aim direction, at most 4 in flight; `a` fires (edge) and holding A fires every 12 ticks; each shot costs 1 point (score never below 0).
- Helicopters: 16 x 6 body plus a 12 x 2 rotor above, color `2C2C2C`, entering from a random side at a random y in 30..90, moving 1 px per tick; one spawns every `max(60, 150 - score)` ticks. While over the playfield each helicopter drops up to 2 paratroopers at random ticks.
- Paratroopers: body 6 x 8 in `2E6DB4` with a chute 12 x 6 in `E07A1F` above; descend 1 px per tick with a chute and 4 px per tick without. A bullet hitting the body kills it (+2). A bullet hitting the chute removes the chute; a chuteless trooper that lands dies (+1) and kills any landed trooper it hits (+2 each).
- Landing: a trooper whose feet reach y 212 lands; x < 150 counts for the left side, x > 170 for the right, otherwise it lands on the turret and the game ends. Four landed on one side ends the game. Landed troopers stay drawn.
- Scoring: +2 per helicopter or trooper shot, -1 per shot. HUD: "SCORE n" left, "HIGH n" right.
- Sound effects: `SFX_SHOT` per shot, `SFX_EXPLODE` per kill, `SFX_GAME_OVER`.
- Autoplay: aim at the lowest paratrooper with a chute, else the nearest helicopter, and fire when within 5 degrees of the target angle.

### 4.6 2048

- Board: 4 x 4 tiles of 40 px with 4 px gaps, board rectangle x 72..248, y 32..208 drawn in `2C2C2C` with empty cells in `EDE4D6`. HUD: "SCORE n" left, "HIGH n" right at baseline 26; "2048!" centered at baseline 224 once the tile has been reached.
- Tile colors by value: 2 `EDE4D6` with dark text, 4 `E8D8B8` dark text, 8 `E07A1F`, 16 `D96A2F`, 32 `C4472A`, 64 `B03A3A`, 128 `D4B01C`, 256 `C9A518`, 512 `3FA34D`, 1024 `2E6DB4`, 2048 `7B4EA3`, above `2C2C2C`; light text on everything from 8 up. Numbers use the BIG font up to three digits and the HUD font from four digits.
- Moves: `dpad_x/y` edge, or `stick_x/y` crossing ±128 after having returned inside ±64. A move slides all tiles, merging equal neighbours once per tile per move in the direction of travel; the score increases by each merged value. If the board changed, a new tile appears in a random empty cell: 2 with probability 9/10, 4 otherwise.
- Start: two random tiles. Game over: no empty cell and no equal adjacent pair. The game continues past 2048.
- Sound effects: `SFX_MERGE` when any merge happens in a move, `SFX_CLEAR` the first time 2048 is reached, `SFX_GAME_OVER`.
- Autoplay: try down, left, down, right in rotation; if a move does not change the board, try the next; stop when none does.

## 5. Build, test, and verification

- `make test` builds and runs one test binary per game plus the menu and PRNG tests, all with the v1 host flags, and fails if any fails.
- Each game's tests cover: init/start/high-score retention, the core rule set listed above (at least one test per bullet that describes behavior), and an autoplay run with the fixed seed that reaches game over (Brick within 60,000 ticks, Blocks within 20,000, Snake within 20,000, Parachute within 20,000, 2048 within 200,000 ticks of moves, Pong within 20,000 with a winner) without ever drawing outside the playfield.
- `make frames GAME=<name>` writes `build/frames/<name>/frame-NNNN.png` every 30 ticks for 1200 ticks plus `gameover.png`, using the fixed seed.
- `make music` renders all tunes and effects to `build/music/*.wav` for listening.
- `make rom` builds `games.z64`; `make rom-autoplay GAME=<name>` builds `games-autoplay.z64` for that game; `make shots GAME=<name>` captures it in ares at 4, 8, and 15 s; game-over captures use each game's measured autoplay time from the host test output.
- Review after every task, as in v1.

## 6. Task plan

1. Framework: `game.h`, `prng.h`, `sfx.h`, `tests/harness.h`, draw API, `tools/framedump.c` on the draw API, Brick moved and refactored onto the interface (queueing its sound effects) with its tests ported, adapter running Brick through the descriptor. Verify: all Brick tests pass; ares shows Brick unchanged.
2. Menu and pause menu with tests; registry; autoplay build flag by game; ROM renamed. Verify: menu screenshot, pause overlay in a frame dump, Brick launches and quits.
3. Synth, notation parser, sequencer, sound-effect presets, the seven tunes, `make music`, tests. Verify: tests green; the user listens to the WAV renders and requests changes before the task closes.
4. Audio in the adapter, settings screen, `save.c` with EEPROM load/save, high scores wired through the descriptor, Brick's effects audible. Verify: settings screenshot; a setting and a high score survive an ares relaunch.
5. Blocks with tests, sound effects, autoplay, frames, ares screenshots.
6. Snake, same.
7. Pong, same, including a two-controller check in ares (port 2 mapped to keyboard).
8. Parachute, same.
9. 2048, same.
10. README for the collection, controls table per game, evidence, final review.

## 7. Risks

- **Scope creep per game**: the rule lists above are the whole rule set; anything not listed is out.
- **Rotation and kick edge cases in Blocks**: tests cover each piece's four rotations against walls and the floor.
- **Parachute geometry without trig**: the 31-entry table is the only source of direction vectors; tests check it is unit length and symmetric.
- **Two-player input in emulators**: ares maps controller 2 in its input settings; the AI mode keeps every check unattended.
- **ROM size and RAM**: all states together are under 64 KB; the ROM stays well under 1 MB.
- **Composing blind**: tunes are written as notation and judged by ear from the WAV renders; expect one or two revision rounds per tune. Keep each tune's notation in one place so a change is a string edit.
- **Audio timing**: the synth renders whatever the audio buffer asks for, independent of the game tick, so a dropped frame never garbles the music; sound effects are queued per tick and started on the next buffer fill.
- **EEPROM on real hardware**: the save format has a magic, version, and CRC so a corrupted or foreign save falls back to defaults instead of crashing.
