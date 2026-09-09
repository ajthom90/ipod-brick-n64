# v2 Task 5: Blocks

You are implementing **Task 5 of the v2 plan** for a Nintendo 64 games collection built with libdragon. Tasks 1 to 4 are committed: framework, menu, audio, settings, saves, and Brick all work. You are adding the second game. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-retro-collection.md` — read the header, "Global Constraints", "Shared code", and all of **Task 5**: the header, the shape and gravity tables (copy verbatim), the rules list, the rendering geometry, the autoplay heuristic, and the test list.
2. `docs/superpowers/specs/2026-09-08-retro-collection-design.md` section 4.2.
3. `src/game.h`, `src/prng.h`, `src/sfx.h`, `src/games/brick.c` (as the reference for how a game implements the descriptor, uses `fmt_label`, and draws through `draw_t`), `src/games/registry.c`, `tests/test_brick.c`, `tests/harness.h`.

## The task

Do Task 5 steps 1 to 4 in order: write the failing `tests/test_blocks.c` with every case listed in the plan; implement `src/games/blocks.h` and `src/games/blocks.c` per the rules list (7-bag, DAS, rotation kicks, soft and hard drop, gravity table, lock, flash, line clear scoring, level, game over, rendering, autoplay heuristic, descriptor `GAME_BLOCKS` with `track = MUSIC_BLOCKS` and sound effects); register it second in `src/games/registry.c`; verify; commit.

## Standing rules

- `src/games/blocks.*` is a pure module: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon; randomness only through `prng.h`.
- Do not edit the plan or specs; do not touch other games, the framework, the synth, or the adapter.
- The game is called **Blocks** everywhere; never use the word Tetris in code, comments, strings, or commit messages.
- Capture screens only with `scripts/ares-shot.sh`. Never run `screencapture` yourself or capture the full display by any means. If the script fails, say so and continue; the reviewer will capture.
- Every command must finish in under four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make test                                              # test_blocks joins the green binaries
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/games/blocks.c src/games/blocks.h   # prints nothing
grep -rni tetris src tests README.md; test $? -eq 1      # no matches
make frames GAME=blocks                                # build/frames/blocks/frame-0300.png shows a partly filled well
make rom && make rom-autoplay GAME=blocks && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task5 5 20 60
```

If you can view images: the 20 s and 60 s captures must show the bordered well with colored pieces stacking, the NEXT preview, and the SCORE / LEVEL / LINES panel. Report what you see, plus the autoplay test's tick count and lines cleared.

## When done

```
git add -A src tests
git commit -m "v2 Task 5: Blocks"
```

Then print a short report: `make test` summary lines, what the captures show, and any deviation.
