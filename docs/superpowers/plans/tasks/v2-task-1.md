# v2 Task 1: Game framework, draw API, Brick on the descriptor

You are implementing **Task 1 of the v2 plan** for a Nintendo 64 games collection built with libdragon. v1 (a single Brick game) is complete and tagged v1.0. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-retro-collection.md` — read the header, "Global Constraints", "File Structure", all of "Shared code", and all of **Task 1**. Use the code blocks **verbatim**.
2. `docs/superpowers/specs/2026-09-08-retro-collection-design.md` sections 3.1 to 3.5 and 4.1, for intent.
3. The v1 files you are refactoring: `src/brick.h`, `src/brick.c`, `tests/test_brick.c`, `tools/framedump.c`, `src/n64/app.c`, `Makefile`. Read `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` Task 5 and Task 7b only if you need to understand the v1 adapter loop.

## The task

Do Task 1 steps 1 to 9 in order: shared headers and harness; tests for prng/sfx/fmt; move Brick to `src/games/` with `git mv` and refactor it onto `game_desc_t` (input from `input_t`, no PAUSE state, sound-effect queue, `brick_render` on `draw_t`, descriptor `GAME_BRICK`); port the Brick tests and add `test_sfx_events` and `test_descriptor`; rewrite `tools/framedump.c` on the draw API with the placeholder text boxes; update the Makefile (per-test binaries, `frames GAME=`, ROM rename to `games.z64`, `AUTOPLAY_GAME` by name); rewrite `src/n64/app.c` as the descriptor-driven adapter with `n64_rect`, `n64_text`, `read_port`; verify; commit.

## Standing rules

- Pure modules (`src/game.h`, `src/prng.h`, `src/sfx.h`, `src/games/*`) include only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon. Brick's rules must not change: every ported v1 test must still pass unchanged in meaning.
- Do not edit the plan or either spec. Do not add any game beyond Brick or any framework screen (menu/pause are Task 2).
- Every command must finish in under four minutes (ROM builds take about 40 s; captures 8 s). Never use mupen64plus. Do not `git push`.
- If a libdragon call from the plan is rejected by the compiler, fix only that line minimally and report the change and the compiler message.

## Verification (all must pass before committing)

```
make test                                            # test_prng, test_sfx, test_brick all "0 failure(s)"
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/game.h src/prng.h src/sfx.h src/games/*.c src/games/*.h   # prints nothing
make frames GAME=brick                               # build/frames/brick/*.png plus "text ..." lines on stdout
make rom && make rom-autoplay GAME=brick             # games.z64 and games-autoplay.z64, zero warnings
scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task1 8
```

If you can view images: the 8 s capture must look like the v1 game (HUD "SCORE n / LIVES 3 / LV 1" in the small font, six colored rows, paddle, ball). Report what you see.

## When done

```
git add -A src tests tools Makefile
git commit -m "v2 Task 1: game framework, draw API, Brick on the descriptor"
```

Then print a short report: the `make test` summary lines, the ROM sizes, what the capture shows, and any deviation.
