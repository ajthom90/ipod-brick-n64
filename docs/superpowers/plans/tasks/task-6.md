# Task 6: Fixed-rate tick loop and screenshot target

You are implementing **Task 6** of an approved plan for a Nintendo 64 port of the iPod "Brick" game. Tasks 1 to 5 are committed: the game renders and plays on the N64 side, but the core is ticked once per rendered frame. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` — read the header, "Global Constraints", and all of **Task 6**. Use the main-loop code **verbatim**.
2. `docs/superpowers/specs/2026-09-08-ipod-brick-n64-design.md` section 3.5 (timing) for intent.
3. `src/n64/app.c` (Task 5 version; you change only `main`'s loop and add one `#ifndef` around `read_input`), `Makefile` (`shots` target already exists), `README.md`.

## The task

1. In `src/n64/app.c`, replace the body of the `while (1)` loop in `main` (and add the `hz`, `dt`, `prev`, `acc` declarations before it) with the Task 6 code from the plan, verbatim. Keep everything else in the file unchanged.
2. Wrap the entire `read_input` function definition in `#ifndef BRICK_AUTOPLAY` … `#endif` so the autoplay build no longer warns about an unused function.
3. Run `make rom && make rom-autoplay && make shots`. Expected: no compiler warnings; `build/shots/shot-4s.png`, `shot-8s.png`, `shot-15s.png` exist. Bricks disappear and the score rises across the three shots.
4. If you can view images, confirm; otherwise check the files exist and are larger than 20 KB and say so.
5. Update `README.md`: add a `## Controls` section (analog stick or D-pad / C-left / C-right move the paddle; A launches the ball and confirms; Start pauses) and add to Develop: `make rom-autoplay  # builds brick-autoplay.z64, which plays itself for unattended checks` and `make shots        # captures brick-autoplay.z64 in ares at 4, 8, and 15 seconds into build/shots/`.
6. Commit.

## Standing rules

- `get_ticks()`, `TICKS_PER_SECOND`, `get_tv_type()`, and `TV_PAL` are all in libdragon's `n64sys.h`, already included via `<libdragon.h>`.
- Do not edit the plan, the spec, `src/brick.*`, `tests/`, `tools/`, `Makefile`, or `scripts/`.
- Never use mupen64plus. Do not `git push`.
- If the compiler rejects a line of the plan's code, fix only that line minimally and report the change and the compiler message.

## Verification (all must pass before committing)

```
make test                                   # 24 ok
make rom && make rom-autoplay               # both ROMs, zero warnings
make shots                                  # build/shots/shot-4s.png shot-8s.png shot-15s.png
ls -la build/shots/
```

## When done

```
git add src/n64/app.c README.md
git commit -m "Task 6: fixed 60/50 Hz tick loop and ares screenshot target"
```

Then print a short report: build output tails, the three screenshot sizes, what you saw (or that you could not view them), and any deviation.
