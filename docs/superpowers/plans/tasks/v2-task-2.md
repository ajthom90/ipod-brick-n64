# v2 Task 2: App state machine, games menu, pause menu

You are implementing **Task 2 of the v2 plan** for a Nintendo 64 games collection built with libdragon. Task 1 (framework, Brick on the descriptor) is committed. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-retro-collection.md` — read the header, "Global Constraints", "Shared code", and all of **Task 2**. Skim Task 1 for what exists. Use the interfaces **verbatim**.
2. `docs/superpowers/specs/2026-09-08-retro-collection-design.md` sections 3.6 and 3.7 (framework flow and menu look).
3. `src/game.h`, `src/games/registry.h`, `src/games/brick.h`, `src/n64/app.c`, `tools/framedump.c`, `tests/harness.h`, `Makefile`.

## The task

Do Task 2 steps 1 to 5 in order: write the failing `tests/test_app.c` cases listed in the plan; implement `src/menu.c/.h` (games menu drawing and navigation with key repeat, pause overlay, placeholder settings screen) and `src/app_state.c/.h` (`app_t`, `app_init`, `app_update`, `app_render`, `app_track`, `app_next_sfx`); wire the adapter to own an `app_t` (Start latch → `app_update`, autoplay build boots straight into the named game); extend `tools/framedump.c` to accept `menu` and `pause`; verify; commit.

Notes:
- The ROM is already `games.z64` and `make rom-autoplay GAME=<name>` already exists (Task 1); the plan's mention of renaming is already done. Just make sure `README.md` mentions `games.z64` in the Build and Run sections (one-line edits).
- The Makefile's `C_FILES` and `CORE_SRCS` already glob `src/*.c`, so the new framework sources compile without Makefile changes.
- Menu geometry and repeat timing are in the plan; with only Brick registered the menu shows two rows: BRICK and SETTINGS.

## Standing rules

- `src/menu.*` and `src/app_state.*` are pure modules: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon.
- Do not edit the plan or specs; do not add games or audio (Tasks 3 to 9). Do not change `src/games/brick.*` or its tests.
- Every command must finish in under four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make test                                              # test_app joins the green binaries
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/*.h src/*.c src/games/*.c src/games/*.h   # prints nothing
make frames GAME=menu && make frames GAME=pause        # menu highlight moves; pause shows two placeholder rows over Brick
make rom && scripts/ares-shot.sh games.z64 build/shots/v2-task2-menu 4
make rom-autoplay GAME=brick && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task2-auto 8
```

If you can view images: the 4 s capture must show "GAMES" top-left with a rule under it, "BRICK" on a blue highlight bar in light text, and "SETTINGS" below in dark text; the 8 s autoplay capture must show Brick playing. Report what you see.

## When done

```
git add -A src tests tools README.md
git commit -m "v2 Task 2: app state machine, games menu, pause menu"
```

Then print a short report: `make test` summary lines, what the captures show, and any deviation.
