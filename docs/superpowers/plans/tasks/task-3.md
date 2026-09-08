# Task 3: Ball physics, collisions, scoring, autoplay

You are implementing **Task 3** of an approved plan for a Nintendo 64 port of the iPod "Brick" game. Tasks 1 and 2 are committed. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` — read the header, "Global Constraints", and all of **Task 3**. Use the test code **verbatim** and follow the behavior list exactly, including the order of checks inside each axis pass.
2. `docs/superpowers/specs/2026-09-08-ipod-brick-n64-design.md` section 3.2 for the intent.
3. `src/brick.h` (fixed API, do not change), `src/brick.c` (Task 2 implementation; you replace the `step_ball` stub and the `brick_autoplay_input` stub), `tests/test_brick.c` (extend).

## The task

Do every step of Task 3 in the plan, in order:

1. Append the ten Task 3 tests to `tests/test_brick.c` verbatim and add a `RUN(...)` line for each in `main`, after the existing fourteen.
2. Run `make test` and confirm the new tests FAIL (exit 1) before implementing.
3. Implement `step_ball` (sub-steps, X pass, Y pass, walls, bricks, paddle bounce table, loss, level clear) and `brick_autoplay_input` exactly as the Task 3 behavior list describes. Suggested structure: `pass_x`, `pass_y`, `hit_brick(g, horizontal)`, `bounce_off_paddle`.
4. Run `make test`: every test `ok`, `0 failure(s)`, exit 0. The autoplay test prints how many ticks level 1 took; include that number in the commit message body.
5. Commit.

## Standing rules

- `src/brick.c` must stay pure: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`; no `float`/`double`, no `<math.h>`, no libdragon. All Q8.8 math is integer: `(a * b) >> 8`.
- Do not edit the plan, the spec, `src/brick.h`, `Makefile`, `src/n64/app.c`, or `scripts/`. Do not change Task 1 or Task 2 tests.
- Determinism matters: no randomness, no time source. Identical inputs must produce identical states.
- The only tuning you may do: if `test_autoplay_clears_level_and_ends` fails because level 1 is not cleared within 30,000 ticks, change the autoplay zone-rotation period (`ticks / 300`) to another value between 120 and 600 and report the value. Do not change the test bounds.
- If any other plan test seems inconsistent with the behavior list, implement the behavior list, leave the test as written, and report the conflict in your final message instead of editing the test.
- Do not `git push`.

## Verification (all must pass before committing)

```
make test        # 24 lines ending in ok, "0 failure(s)", exit 0
grep -nE '\b(float|double)\b|#include <math.h>|libdragon' src/brick.c src/brick.h   # prints nothing
```

## When done

```
git add src/brick.c tests/test_brick.c
git commit -m "Task 3: ball physics, brick and paddle collisions, scoring, autoplay"
```

Then print a short report: the full `make test` output, the tick count for clearing level 1, and anything you deviated from or tuned.
