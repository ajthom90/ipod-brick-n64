# Task 2: Core state machine

You are implementing **Task 2** of an approved plan for a Nintendo 64 port of the iPod "Brick" game. Task 1 is already committed. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` — read the header, "Global Constraints", and all of **Task 2**. Skim Task 1 only to see what already exists. Use the test code in Task 2 **verbatim** and follow the behavior list exactly.
2. `docs/superpowers/specs/2026-09-08-ipod-brick-n64-design.md` section 3.2 (game rules and state machine) for the intent behind each rule.
3. `src/brick.h` (the fixed public API; do not change any signature; the include guard is `BRICK_GAME_H`), `src/brick.c` (Task 1 stubs you will replace), `tests/test_brick.c` (harness you will extend).

## The task

Do every step of Task 2 in the plan, in order:

1. Append the Task 2 tests to `tests/test_brick.c` verbatim (the `playing()` helper and nine tests) and add a `RUN(...)` line for each in `main`, after the existing five.
2. Run `make test` and confirm the new tests FAIL (exit 1) before implementing.
3. Implement in `src/brick.c`: `speed_for_level`, `refill`, `park_ball`, `move_paddle`, `launch_ball`, `brick_new_game`, `brick_on_ball_lost`, `brick_on_level_clear`, and the full `brick_update` switch from the plan. Keep `static void step_ball(brick_game_t *g) { (void)g; }` as a stub — ball flight is Task 3.
4. Run `make test`: every test `ok`, `0 failure(s)`, exit 0.
5. Commit.

## Standing rules

- `src/brick.c` and `src/brick.h` must include only `<stdint.h>`, `<stdbool.h>`, `<string.h>`; no `<libdragon.h>`, no `<math.h>`, no `float`/`double`. Q8.8 fixed point everywhere (`(a * b) >> 8`).
- Do not edit the plan, the spec, `src/brick.h`, `Makefile`, `src/n64/app.c`, or `scripts/`. Do not add behavior beyond Task 2 (no ball movement, no collisions).
- Do not `git push`.
- If a plan test seems inconsistent with the behavior list, implement the behavior list, leave the test as written, and report the conflict in your final message instead of editing the test.

## Verification (all must pass before committing)

```
make test        # 14 lines ending in ok, "0 failure(s)", exit 0
grep -nE '\b(float|double)\b|#include <math.h>|libdragon' src/brick.c src/brick.h   # prints nothing
```

## When done

```
git add src/brick.c tests/test_brick.c
git commit -m "Task 2: core state machine, paddle movement, serve and launch"
```

Then print a short report: the full `make test` output and anything you deviated from.
