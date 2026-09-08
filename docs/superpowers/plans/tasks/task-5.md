# Task 5: N64 adapter — input mapping and full rendering

You are implementing **Task 5** of an approved plan for a Nintendo 64 port of the iPod "Brick" game. Tasks 1 to 4 are committed: the core is complete and tested, and `src/n64/app.c` currently only draws a static title. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` — read the header, "Global Constraints", and all of **Task 5**. Use the `src/n64/app.c` code **verbatim**; every libdragon call in it was checked against the trunk headers.
2. `docs/superpowers/specs/2026-09-08-ipod-brick-n64-design.md` sections 3.3 (input) and 3.4 (rendering) for intent.
3. `src/brick.h` (core API, do not change), `src/n64/app.c` (Task 1 version you replace), `Makefile` (targets `rom`, `rom-autoplay` already exist), `scripts/ares-shot.sh`.

## The task

1. Replace `src/n64/app.c` with the Task 5 version from the plan, verbatim.
2. Run `make rom` and `make rom-autoplay`. Both build inside Docker (`ipod-brick-n64:dev` already exists; if it is missing run `make image` first). Expected outputs: `brick.z64` and `brick-autoplay.z64` in the repo root.
3. Run `scripts/ares-shot.sh brick.z64 build/shots/task5-title 4`. Expected: the title screen with "BRICK", "HIGH SCORE 0", "PRESS A" on the light field.
4. Run `scripts/ares-shot.sh brick-autoplay.z64 build/shots/task5-play 5 10`. Expected: both shots show the HUD ("SCORE n" left, "LIVES 3" center, "LV 1" right), six colored brick rows, the dark paddle, and the ball; the 10 s shot has fewer bricks or a higher score than the 5 s shot.
5. If you can view images, confirm the expectations above; otherwise verify the PNG files exist and are larger than 20 KB and say you could not view them.
6. Commit.

## Standing rules

- The ROM build must compile with no warnings treated as errors failing; if the compiler rejects a line of the plan's code, fix only that line minimally, keep the same libdragon API, and report exactly what you changed and the compiler message.
- Do not edit the plan, the spec, `src/brick.h`, `src/brick.c`, `tests/`, `Makefile`, or `scripts/`.
- Never use mupen64plus.
- Do not `git push`.

## Verification (all must pass before committing)

```
make test                       # 24 ok
make rom && make rom-autoplay   # brick.z64 and brick-autoplay.z64 present
scripts/ares-shot.sh brick.z64 build/shots/task5-title 4
scripts/ares-shot.sh brick-autoplay.z64 build/shots/task5-play 5 10
ls -la brick.z64 brick-autoplay.z64 build/shots/task5-title build/shots/task5-play
```

## When done

```
git add src/n64/app.c
git commit -m "Task 5: N64 adapter with controller mapping and fill-mode rendering"
```

Then print a short report: build output tail for both ROMs, the screenshot paths and sizes, what you saw in them (or that you could not view them), and any line you had to change.
