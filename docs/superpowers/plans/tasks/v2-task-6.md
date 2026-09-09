# v2 Task 6: Snake

You are implementing **Task 6 of the v2 plan** for a Nintendo 64 games collection built with libdragon. Tasks 1 to 5 are committed: framework, menu, audio, settings, saves, Brick, and Blocks work. You are adding the third game. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-retro-collection.md` — read the header, "Global Constraints", "Shared code", and all of **Task 6**: the header, the layout correction (36 x 24 cells at y 32..224), the rules, rendering, autoplay, and the test list.
2. `docs/superpowers/specs/2026-09-08-retro-collection-design.md` section 4.3.
3. `src/game.h`, `src/prng.h`, `src/sfx.h`, `src/games/blocks.c` (the most recent reference for a descriptor, title/game-over overlays, `fmt_label`, and `draw_t` use), `src/games/registry.c`, `tests/test_blocks.c`, `tests/harness.h`.

## The task

Do Task 6 steps 1 to 4 in order: write the failing `tests/test_snake.c` with every case listed in the plan; implement `src/games/snake.h` and `src/games/snake.c` (ring-buffer body, pending direction with no reversing, step period that shortens every five foods, food placement through `prng.h`, wall and self collision with the tail-chasing allowance, HUD and overlays, autoplay, descriptor `GAME_SNAKE` with `track = MUSIC_SNAKE`, `SFX_FOOD` and `SFX_GAME_OVER`); register it third in `src/games/registry.c`; update `tests/test_app.c` if the game count change requires it (as Task 5 did); verify; commit.

Stale-object note: before building the autoplay ROM for a new game, run `rm -rf build/autoplay` so the previous game's objects are not reused.

## Standing rules

- `src/games/snake.*` is a pure module: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon; randomness only through `prng.h`.
- Do not edit the plan or specs; do not touch other games, the framework, the synth, or the adapter.
- Capture screens only with `scripts/ares-shot.sh`; never run `screencapture` yourself or capture the full display. If the script fails, say so and continue.
- Every command must finish in under four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make test                                              # test_snake joins the green binaries
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/games/snake.c src/games/snake.h   # prints nothing
make frames GAME=snake                                 # build/frames/snake/frame-0300.png shows the snake and food on the grid
rm -rf build/autoplay && make rom && make rom-autoplay GAME=snake && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task6 5 20 45
```

If you can view images: the captures must show a blue snake with a dark head, a red food square, "SCORE n" at top-left and "HIGH n" at top-right, with the snake longer at 20 s than at 5 s. Report what you see plus the autoplay test's tick count and score.

## When done

```
git add -A src tests
git commit -m "v2 Task 6: Snake"
```

Then print a short report: `make test` summary lines, what the captures show, and any deviation.
