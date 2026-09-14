# Modern Games for the Collection — Design Spec (v3)

Date: 2026-09-14
Status: approved design, awaiting spec review
Builds on: `2026-09-08-retro-collection-design.md` (v2). Everything not restated here is unchanged.

## 1. Goal

Add three mobile-era classics to the collection as games seven to nine: **Hopper** (a vertical platform bouncer in the style of Doodle Jump), **Flap** (a one-button pipe-dodging flyer in the style of Flappy Bird), and **Runner** (a one-button rooftop endless runner in the style of Canabalt). Same architecture: pure C modules behind `game_desc_t`, host tests, self-play, one tune each.

### In scope

- Menu scrolling with an iPod-style scrollbar, since ten rows no longer fit.
- Save record version 2 with twelve high-score slots and migration from version 1.
- Three tunes appended to `MUSIC_SRC`.
- The three games with the rules in section 4.

### Out of scope

New sound effects (the existing set is reused), sprites, new fonts, changes to existing games.

## 2. Names and trademarks

Menu names HOPPER, FLAP, RUNNER. The original titles are trademarks of their owners and appear only in the README's trademark note, which is extended to say these three are original implementations of their genres.

## 3. Framework changes

### 3.1 Scrolling menu

- `MENU_VISIBLE_ROWS = 7`. Rows are the registry's games followed by SETTINGS, `total = GAME_COUNT + 1`.
- `menu_scroll` is the index of the first visible row. After every highlight move: if `row < scroll` then `scroll = row`; if `row >= scroll + 7` then `scroll = row - 6`. On init both are 0.
- Visible rows draw at baselines `62 + 24 * (row - scroll)` exactly as before. The highlight bar becomes `rect(16, base - 20, 292, base + 6)` whenever `total > 7`, to leave room for the scrollbar; otherwise it keeps its full width.
- Scrollbar, drawn only when `total > 7`: track `rect(296, 42, 304, 214, 0xB8B2A8)`; thumb `rect(297, ty, 303, ty + th, COLOR_DARK)` with `th = 172 * 7 / total` and `ty = 43 + (172 - th) * scroll / (total - 7)`.
- The pure helper `int menu_scroll_for(int row, int scroll, int total, int visible)` implements the window rule so tests can cover it with synthetic totals.
- The frame dumper's `menu` mode adds a frame after 300 ticks of holding down, which shows the window scrolled to the bottom with the scrollbar.

### 3.2 Save record version 2

- `SAVE_MAX_GAMES = 12`, `SAVE_VERSION = 2`. Layout: magic (4), version (2), music, sound, volume (1 each), twelve big-endian `int32_t` scores (48), zero padding to byte 59, CRC32 over bytes 0..59 at 60..63. Total still 64 bytes.
- `save_decode` accepts version 1 (eight scores at the same offset) and version 2 (twelve). Version 1 records decode into a version-2 `save_t` with slots 8..11 zero, so existing high scores survive the upgrade. `save_encode` always writes version 2.
- Games are indexed by registry order as before; the three new games take slots 6, 7, 8.

### 3.3 Music

`music_track_id_t` gains `MUSIC_HOPPER`, `MUSIC_FLAP`, `MUSIC_RUNNER` after `MUSIC_2048`; `MUSIC_TRACK_COUNT` becomes 10. Briefs: Hopper 125 BPM, G major, bouncing octave bass and a springy lead; Flap 115 BPM, F major, dotted "swung" rhythm, cheeky; Runner 160 BPM, A minor, driving eighth-note pulses and a four-on-the-floor kick. The notation is in the v3 plan.

## 4. Games

All three use the v2 conventions: Q8.8 for positions and velocities, `input_t` from player 1 only, a title state ("NAME", "HIGH SCORE n", "PRESS A"), a game-over state ("GAME OVER", "SCORE n", "PRESS A"), HUD "SCORE n" left and "HIGH n" right at baseline 26, `prng.h` for randomness, sound effects through the queue, and a self-play policy that deliberately stops playing well after a threshold so demonstrations end.

### 4.1 Hopper

- World y grows downward; `camera_y` is the world y of the screen's top edge. Screen y = world y − `camera_y`.
- Player: 12x12 square, `COLOR_DARK` with a 3x3 `COLOR_BG` eye at (+7, +3). Horizontal control: `stick_x` up to 4 px per tick, else `dpad_x` times 3. The player wraps: leaving the playfield on one side re-enters on the other.
- Gravity 0.25 px per tick² (Q8.8 64). Bounce velocity −6.5 px per tick (−1664); spring −10 (−2560). Peak jump height is about 84 px, so platform gaps never exceed 80.
- Platforms: 32x6, up to 24 alive, kept sorted by y. Types: static (`COLOR_GREEN`, 70%), moving (`COLOR_BLUE`, 20%, slides 1 px per tick and reverses at the playfield edges), spring (`COLOR_ORANGE`, 10%, bounce uses the spring velocity). Generated upward: `gap = 40 + prng_below(21) + min(20, score / 50)`, `x = PLAY_X0 + prng_below(PLAY_X1 - PLAY_X0 - 32)`. Platforms more than 20 px below the screen bottom are recycled.
- Collision only while falling: if the player's bottom edge crosses a platform's top edge during the tick with horizontal overlap, the player lands on it and bounces (`SFX_BOUNCE`; `SFX_CLEAR` on a spring). Platforms never block upward motion.
- Camera: when the player's screen y is above 100, `camera_y` moves up so it is exactly 100; it never moves down. `score = (camera_start - camera_y) / 10`.
- Start: player centered on a static platform at world y 212 with the screen filled with generated platforms above. Death: player's screen top beyond 240 → game over.
- Self-play: among platforms whose top is between 8 and 130 px above the player's bottom, steer toward the horizontally nearest (wrap-aware); above score 300 stop steering.

### 4.2 Flap

- Bird: 10x10 `COLOR_YELLOW` square with a 2x2 `COLOR_DARK` eye, fixed at x 80. Gravity 0.3 px per tick² (77); A sets vy to −5 (−1280); vy is capped at +6 (1536). Ceiling: y clamps to `PLAY_Y0` with vy 0. Ground strip `rect(16, 220, 304, 228, COLOR_DARK)`; the bird's bottom reaching 220 ends the run.
- Pipes: up to 6 pairs, 24 px wide, `COLOR_GREEN`, scrolling left at 2 px per tick (512). A pair spawns when the last pair's x is below `PLAY_X1 - 90`, at `x = PLAY_X1`, with the gap center at `70 + prng_below(111)`. The gap is `64 - min(16, score / 10)`, never below 48. The top pipe spans `PLAY_Y0` to `center - gap / 2`, the bottom pipe `center + gap / 2` to 220. Pairs whose right edge passes `PLAY_X0` are removed.
- Scoring: one point (`SFX_POINT`) the first time the bird's left edge passes a pair's right edge. Death (`SFX_EXPLODE`): overlapping either pipe rectangle, or the ground. `SFX_HIT` on every flap.
- Start: A on the title starts the run with an immediate flap.
- Self-play: with the next pair ahead, flap when the bird's center is more than 4 px below the gap center and vy is not negative; above score 40 stop flapping.

### 4.3 Runner

- Player: 10x14 `COLOR_RED` rectangle at x 60. Gravity 0.3 px per tick² (77). A while grounded sets vy to −6 (−1536), `SFX_MERGE`; releasing A while vy is below −2 raises vy to −2 (−512), the short hop. Landing on a roof: `SFX_HIT`.
- Buildings: up to 8, `COLOR_DARK` from the roof to the screen bottom, scrolling left at the current speed. Widths `60 + prng_below(101)`, roof y `120..200` with each roof within 40 px of the previous one, gaps `24 + prng_below(41)`. A new building spawns when the last one's right edge is below `PLAY_X1 + 64`; buildings whose right edge passes `PLAY_X0 - 16` are removed.
- Speed: starts at 3 px per tick (768), rises by 26 (about 0.1) every 300 ticks, caps at 7 (1792). `score = distance / 10` with `SFX_POINT` at every 100.
- Landing: while falling, if the player's bottom crosses a roof with horizontal overlap, snap to the roof. Wall hit: a building's left edge enters the player's x-span while the player's bottom is more than 2 px below that roof → game over (`SFX_EXPLODE`). Falling below the screen → game over.
- Start: the player standing on a wide first building whose roof is at y 180. Self-play: when grounded and the current roof ends within `speed * 8` px, press and hold A for 12 ticks; above 500 m stop jumping.

## 5. Build, test, verification

As v2: `make test` with one binary per game, `make frames GAME=hopper|flap|runner`, `make rom-autoplay GAME=<name>`, ares captures through `scripts/ares-shot.sh`. Each game's autoplay test: Hopper reaches score 50 within 6,000 ticks and game over within 60,000; Flap reaches 20 and ends within 20,000; Runner reaches 200 m and ends within 60,000.

## 6. Task plan

1. Framework: scrolling menu with scrollbar and tests; save record v2 with migration tests; three tunes; frame dumper `menu` bottom frame.
2. Hopper.
3. Flap.
4. Runner.
5. README (games, controls, trademark note), evidence (menu scrolled, three games), final verification, tag v3.0.

## 7. Risks

- **Reachability in Hopper**: gaps are capped at 80 px against an 84 px jump; moving platforms can still strand a player, which is the intended difficulty.
- **Save migration**: covered by a decode test on a hand-built version-1 record; a bad CRC still falls back to defaults.
- **Menu regressions**: the scroll helper is unit-tested with synthetic totals since the real menu only just exceeds the window.
