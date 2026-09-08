# Task 7: Overlay screens verified end to end

You are executing **Task 7** of an approved plan for a Nintendo 64 port of the iPod "Brick" game. Tasks 1 to 6 are committed: the game is complete and runs at a fixed rate in ares. This task is mostly verification with evidence; code changes only if a layout problem is found. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` — read the header, "Global Constraints", and all of **Task 7**.
2. `src/n64/app.c` — the `render` function, to know where each overlay is drawn (title text at baselines 100/130/160; HUD at 16; "PAUSED" at 160; game over at 150/170/190).
3. `scripts/ares-shot.sh`, `tools/framedump.c`.

## The task

1. Make sure both ROMs are current: `make rom && make rom-autoplay`.
2. Run the long capture (ares stays open about six and a half minutes; do not interrupt it):
   `scripts/ares-shot.sh brick-autoplay.z64 build/shots/task7 3 20 60 332 345 390`
   Expected: 3 s = title or first serve; 20 s and 60 s = play with a shrinking grid and rising score; 332 s = level 2 begun (full grid, "LV 2", score at least 60); 345 s = lives dropping on level 2 (paddle held still); 390 s = "GAME OVER" with "HIGH SCORE" at least 60 and "PRESS A". If 390 s still shows level 2 in progress, run one more capture with `450 510` appended to a new outdir and report it.
3. Run `make frames` and confirm `build/frames/pause.png` exists (frozen field, no text by design).
4. Check the overlay placement by reading `render`: "PAUSED" and the game-over lines must fall between the grid bottom (y 107) and the paddle (y 218). Confirm this from the coordinates; no change is expected.
5. If you can view images, confirm each screenshot against the expectations above and list what each shows. If you cannot, verify the files exist, are larger than 20 KB, and say so.
6. Produce evidence: downscale each capture to 768 px wide so the repo stays small, into `docs/superpowers/plans/evidence/`:
   `mkdir -p docs/superpowers/plans/evidence && for f in build/shots/task7/*.png; do sips -Z 768 "$f" --out "docs/superpowers/plans/evidence/task7-$(basename "$f")" >/dev/null; done`
7. Commit.

## Standing rules

- Only edit `src/n64/app.c` if a screenshot shows text overlapping bricks, text off screen, or text within 16 px of an edge. If you change it, rebuild both ROMs and re-capture the affected state, and explain the change.
- Do not edit the plan, the spec, `src/brick.*`, `tests/`, `tools/`, `Makefile`, or `scripts/`.
- Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
ls -la build/shots/task7/            # six PNGs
ls -la docs/superpowers/plans/evidence/   # six downscaled PNGs
make test                            # 24 ok
```

## When done

```
git add docs/superpowers/plans/evidence src/n64/app.c
git commit -m "Task 7: verify title, play, level-up, and game-over screens in ares"
```

Then print a short report: what each of the six screenshots shows (score, lives, level, overlay text), whether any layout change was needed, and the evidence file list.
