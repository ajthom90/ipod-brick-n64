# Task 4: Host frame-dump tool

You are implementing **Task 4** of an approved plan for a Nintendo 64 port of the iPod "Brick" game. Tasks 1 to 3 are committed: the game core in `src/brick.c` is complete and tested. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` — read the header, "Global Constraints", and all of **Task 4**. Use the `tools/framedump.c` code **verbatim**.
2. `src/brick.h` — the core API the tool consumes. Do not change it.
3. `Makefile` — the `frames` and `build/host/framedump` targets already exist; do not change them.

## The task

1. Create `tools/framedump.c` verbatim from the plan.
2. Run `make frames`. Expected: about 43 stdout lines (`frame-0000.ppm state=TITLE …` through `frame-1200.ppm`, then `pause.ppm state=PAUSE`, `gameover.ppm state=GAMEOVER`), then `frames written to build/frames/*.png`, exit 0. `build/frames/` must contain matching `.png` files.
3. If you can view images, open `build/frames/frame-0300.png` and confirm: light background, six colored brick rows (red, orange, yellow, green, blue, purple from the top) with a few bricks missing, a dark paddle near the bottom, a small dark ball. Otherwise report that you could not view it.
4. Add this line under the README's Develop section, aligned with the existing entries:
   `make frames       # dumps autoplay frames to build/frames/*.png for visual checks`
5. Commit.

## Standing rules

- Do not edit the plan, the spec, `src/`, `tests/`, `scripts/`, or `Makefile`.
- `tools/framedump.c` is host-only code and may use `<stdio.h>`/`<stdlib.h>`, but must still avoid `float`/`double` (the host flags include `-Werror=double-promotion`).
- Do not `git push`.

## Verification (all must pass before committing)

```
make test        # still 24 ok, 0 failure(s)
make frames      # exit 0; ls build/frames/*.png shows frame-0000.png … frame-1200.png, pause.png, gameover.png
```

## When done

```
git add tools/framedump.c README.md
git commit -m "Task 4: host frame-dump tool for visual review"
```

Then print a short report: the `make frames` stdout (all lines), and what `gameover.ppm`'s printed score/lives/level were.
