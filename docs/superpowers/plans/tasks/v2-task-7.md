# v2 Task 7: Pong

You are implementing **Task 7 of the v2 plan** for a Nintendo 64 games collection built with libdragon. Tasks 1 to 6 are committed (framework, menu, audio, settings, saves, Brick, Blocks, Snake). You are adding the fourth game, the first two-player one. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-retro-collection.md` — read the header, "Global Constraints", "Shared code", and all of **Task 7**: the header, rules (paddles, serve, sub-stepped ball, 7-zone bounce reused from Brick, speed growth, scoring, win, AI), rendering, autoplay, and the test list.
2. `docs/superpowers/specs/2026-09-08-retro-collection-design.md` section 4.4.
3. `src/game.h`, `src/prng.h`, `src/sfx.h`, `src/games/brick.c` and `src/games/brick.h` (the bounce tables `BRICK_BOUNCE_SIN/COS/SIGN` you reuse for the paddle zones, and the sub-step pattern), `src/games/snake.c` (latest descriptor reference), `src/games/registry.c`, `tests/test_snake.c`, `tests/harness.h`.

## The task

Do Task 7 steps 1 to 4 in order: write the failing `tests/test_pong.c` with every case listed in the plan; implement `src/games/pong.h` and `src/games/pong.c` (title mode rows, serve delay and direction, both paddles from `in[0]`/`in[1]` or the AI, Q8.8 ball with sub-steps, wall and paddle bounces, speed growth to the cap, scoring and `serve_to`, first to 11, margin high score, `pong_ai`, rendering with the dashed center line and big-font scores, autoplay with both AIs, descriptor `GAME_PONG` with `players = 2`, `track = MUSIC_PONG`, and `SFX_BOUNCE` / `SFX_POINT` / `SFX_GAME_OVER`); register it fourth; verify; commit.

## Standing rules

- The new game module is pure: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon; randomness only through `prng.h`.
- Do not edit the plan or specs; do not touch other games, the framework, the synth, or the adapter. Update `tests/test_app.c` only if the registered game count requires it.
- Capture screens only with `scripts/ares-shot.sh`; never run `screencapture` yourself or capture the full display. If the script fails, say so and continue.
- Before building the autoplay ROM run `rm -rf build/autoplay` so the previous game's objects are not reused.
- Every command must finish in under four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make test                                              # test_pong joins the green binaries
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/games/pong.c src/games/pong.h   # prints nothing
make frames GAME=pong                                  # build/frames/pong/frame-0300.png shows both paddles, ball, center line
rm -rf build/autoplay && make rom && make rom-autoplay GAME=pong && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task7 5 30 90
```

If you can view images: the captures must show two dark paddles, the ball, the dashed center line, and two big score digits near the top; the 90 s capture should show a higher combined score than the 30 s one. Report what you see plus the autoplay test's tick count and final score.

## When done

```
git add -A src tests
git commit -m "v2 Task 7: Pong"
```

Then print a short report: `make test` summary lines, what the captures show, and any deviation.
