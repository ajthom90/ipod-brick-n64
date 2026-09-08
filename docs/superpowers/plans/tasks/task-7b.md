# Task 7b: Brick-targeting autoplay and per-tick autoplay input

You are implementing **Task 7b** of an approved plan for a Nintendo 64 port of the iPod "Brick" game. Tasks 1 to 7 are committed. Task 7 found that the autoplay ROM stalled on two corner bricks and never showed level 2 or game over. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` — read the header, "Global Constraints", and all of **Task 7b**. Use its code **verbatim**.
2. `docs/superpowers/plans/reviews/task-7.md` — why this task exists.
3. `src/brick.c` (only the PLAY branch of `brick_autoplay_input` changes), `tests/test_brick.c`, `src/n64/app.c` (only the main loop changes).

## The task

Do Task 7b steps 1 to 7 in order: tighten the test bound, see it fail, replace the autoplay PLAY branch, see 24 tests pass (note the printed tick count), change the adapter loop, rebuild both ROMs, capture `3 60 173 175 200`, downscale evidence, commit.

## Standing rules

- Core purity: `src/brick.c` keeps only `<stdint.h>`, `<stdbool.h>`, `<string.h>`; no floats; no time source.
- Do not edit the plan, the spec, `src/brick.h`, `Makefile`, `tools/`, or `scripts/`. Do not touch any test other than the one bound named in the plan.
- **Timing rule:** every single command you run must finish in under four minutes. The capture in step 6 takes about 3 min 25 s; run it exactly once as written, as its own command, and do not add later timestamps.
- Never use mupen64plus. Do not `git push`.
- If the test's printed tick count differs from 10,269, report it and adjust the capture seconds to `(ticks / 60) + 2` and `(ticks / 60) + 4` for the two level-2 shots, keeping 200 for the final shot.

## Verification (all must pass before committing)

```
make test                                   # 24 ok; autoplay line shows level 2 near 10269 ticks
make rom && make rom-autoplay               # zero warnings
scripts/ares-shot.sh brick-autoplay.z64 build/shots/task7b 3 60 173 175 200
ls -la build/shots/task7b/                  # five PNGs
```

If you can view images: 173 s and 175 s must show "LV 2" and 200 s must show "GAME OVER". If either does not, do not retry the capture; commit anyway and report exactly what each shot shows.

## When done

```
for f in build/shots/task7b/*.png; do sips -Z 768 "$f" --out "docs/superpowers/plans/evidence/task7b-$(basename "$f")" >/dev/null; done
git add src/brick.c tests/test_brick.c src/n64/app.c docs/superpowers/plans/evidence
git commit -m "Task 7b: brick-targeting autoplay, per-tick autoplay input, level-2 and game-over evidence"
```

Then print a short report: the autoplay tick count, what each of the five screenshots shows, and any deviation.
