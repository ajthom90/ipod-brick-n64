# v2 Task 9: 2048

You are implementing **Task 9 of the v2 plan** for a Nintendo 64 games collection built with libdragon. Tasks 1 to 8 are committed (framework, menu, audio, settings, saves, Brick, Blocks, Snake, Pong, Parachute). You are adding the sixth and last game. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-retro-collection.md` — read the header, "Global Constraints", "Shared code", and all of **Task 9**: the header, the rules (edge-triggered moves, stick arming, `g2048_slide_row`, `g2048_move`, tile spawning, 2048 flag, game over), the color table, rendering with `fmt_int`, autoplay every 10 ticks, and the test list.
2. `docs/superpowers/specs/2026-09-08-retro-collection-design.md` section 4.6.
3. `src/game.h` (`fmt_int`), `src/prng.h`, `src/sfx.h`, `src/games/parachute.c` (latest descriptor reference), `src/games/registry.c`, `tests/test_parachute.c`, `tests/harness.h`.

## The task

Do Task 9 steps 1 to 4 in order: write the failing `tests/test_g2048.c` with every case listed in the plan; implement `src/games/g2048.h` and `src/games/g2048.c` (board, `g2048_slide_row`, `g2048_move` for all four directions with a single merge per tile per move, `g2048_add_tile` with 9/10 twos, `g2048_can_move`, score, `reached_2048` with `SFX_CLEAR` once, `SFX_MERGE`, `SFX_GAME_OVER`, edge-triggered D-pad and armed stick input, rendering with the tile color table and big/small numerals, HUD and overlays, autoplay rotation, descriptor `GAME_2048` with `name = "2048"` and `track = MUSIC_2048`); register it sixth; verify; commit.

## Standing rules

- The new game module is pure: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon; randomness only through `prng.h`.
- Do not edit the plan or specs; do not touch other games, the framework, the synth, or the adapter. Update `tests/test_app.c` only if the registered game count requires it.
- Capture screens only with `scripts/ares-shot.sh`; never run `screencapture` yourself or capture the full display. If the script fails, say so and continue.
- Before building the autoplay ROM run `rm -rf build/autoplay` so the previous game's objects are not reused.
- Every command must finish in under four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make test                                              # test_g2048 joins the green binaries
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/games/g2048.c src/games/g2048.h   # prints nothing
make frames GAME=2048                                  # build/frames/2048/frame-0300.png shows the board with several tiles
rm -rf build/autoplay && make rom && make rom-autoplay GAME=2048 && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task9 5 30 120
```

If you can view images: the captures must show the dark board with 4x4 cells, colored tiles with centered numbers, "SCORE n" and "HIGH n"; tiles should grow in value from 5 s to 120 s. Report what you see plus the autoplay test's tick count and score.

## When done

```
git add -A src tests
git commit -m "v2 Task 9: 2048"
```

Then print a short report: `make test` summary lines, what the captures show, and any deviation.
