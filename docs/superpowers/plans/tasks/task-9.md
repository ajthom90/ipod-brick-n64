# Task 9: Polish and documentation

You are implementing **Task 9**, the last task of an approved plan for a Nintendo 64 port of the iPod "Brick" game. Tasks 1 to 8 are committed. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` — read the header, "Global Constraints", and all of **Task 9** (Steps 1, 1b, 2, 3, 4).
2. `README.md`, `src/n64/app.c`, `Makefile`, `docs/superpowers/plans/reviews/task-7b.md` (the ball-hide note) and `docs/superpowers/plans/reviews/task-8.md` (why the resample filter stays).

## The task

1. **Debug build** (Step 1): `make rom DEBUG=1` must compile with `-DBRICK_DEBUG` and the adapter's `rdpq_debug_start()` must be inside `#ifdef BRICK_DEBUG` (it already is; confirm). Then `scripts/ares-shot.sh brick.z64 build/shots/task9-debug 4` must still show the title. Afterwards rebuild the normal ROM with `make rom`.
2. **Hide the ball on game over** (Step 1b): in `render`, draw the ball only when `g->state != BRICK_ST_GAMEOVER`.
3. **Overscan check** (Step 2): `make rom-autoplay && scripts/ares-shot.sh brick-autoplay.z64 build/shots/task9 6`; confirm nothing except background is drawn within about 14 px of any edge (HUD glyph tops and the paddle bottom are inside the margins).
4. **README** (Step 3): rewrite `README.md` with these sections, in this order: title; one paragraph on what it is, crediting Steve Wozniak's original iPod Brick and libdragon; **Fonts** (Inter, SIL Open Font License 1.1, `assets/LICENSE-Inter.txt`); **Requirements** (Docker Desktop, ares via `brew install --cask ares-emulator`, clang from Xcode command line tools); **Build** (`make image`, `make rom`); **Run** (`make run`, or open `brick.z64` in ares); **Controls** (analog stick or D-pad / C-left / C-right move the paddle; A launches the ball and confirms; Start pauses); **Develop** (`make test`, `make frames`, `make rom-autoplay`, `make shots`, `make rom DEBUG=1`, each with one line); **Layout constants** (point to `src/brick.h`); **Verification note** (mupen64plus cannot boot libdragon ROMs, so ares is the only emulator used; screenshots are captured by `scripts/ares-shot.sh`); **Toolchain pin** with the libdragon commit from the Dockerfile and the base image digest from `docker image inspect ghcr.io/dragonminded/libdragon:latest-arm64 --format '{{index .RepoDigests 0}}'`.
5. **Final verification** (Step 4): `make clean && make test && make rom && make rom-autoplay && make shots`.
6. Commit.

## Standing rules

- Do not edit the plan, the spec, `src/brick.*`, `tests/`, `tools/`, `scripts/`, or `assets/`. `Makefile` only if something in the steps requires it (nothing is expected).
- Every command must finish in under four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make clean && make test && make rom && make rom-autoplay && make shots
ls -la brick.z64 brick-autoplay.z64 build/shots/
```

## When done

```
git add README.md src/n64/app.c Makefile
git commit -m "Task 9: debug build check, hide ball on game over, complete README"
```

Then print a short report: what the debug-build and overscan captures show, and the final verification output tail.
