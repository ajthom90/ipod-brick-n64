# v3 Task 5: README, evidence, final verification

You are implementing **Task 5 of the v3 plan**, the last task, for a Nintendo 64 games collection built with libdragon. Tasks 1 to 4 of v3 are committed: nine games, a scrolling menu, save v2, and ten tunes. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-14-modern-games.md` — read the header and all of **Task 5**.
2. `docs/superpowers/specs/2026-09-14-modern-games-design.md` sections 2 and 4.
3. `README.md`, `src/games/registry.c`, `scripts/ares-shot.sh`.

## The task

1. **README**: add HOPPER, FLAP, and RUNNER to the games list (one line each) and the controls table (Hopper: stick or D-pad steers, wraps at the edges; Flap: A flaps; Runner: A jumps, release early for a short hop); mention that the menu scrolls with a scrollbar when more than seven rows exist; extend the trademark note to say that Hopper, Flap, and Runner are original implementations of their genres and are not affiliated with or endorsed by the owners of Doodle Jump, Flappy Bird, or Canabalt. Those three titles may appear only in that note.
2. **Evidence**: `make frames GAME=menu` and copy `build/frames/menu/frame-0300.png` to `docs/superpowers/plans/evidence/v3-menu-bottom.png`; capture `games.z64` at 4 s into `build/shots/v3-final-menu`; for each name in `hopper flap runner`: `rm -rf build/autoplay && make rom-autoplay GAME=<name> && scripts/ares-shot.sh games-autoplay.z64 build/shots/v3-final-<name> 20`; downscale each ares capture to 768 px wide into `docs/superpowers/plans/evidence/v3-<name>.png` (menu as `v3-menu.png`) with `sips -Z 768`.
3. **Final verification**: `make clean && make test && make music && make rom`.
4. Commit.

## Standing rules

- Do not edit the plan, the specs, or any source file; this task changes `README.md` and adds evidence images only. If a build or test fails, stop and report.
- Capture screens only with `scripts/ares-shot.sh`; never run `screencapture` yourself or capture the full display.
- Run builds and captures as separate commands so none exceeds four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make clean && make test
make music && make rom
ls -la docs/superpowers/plans/evidence/v3-*.png        # menu, menu-bottom, hopper, flap, runner = 5 files
grep -ci 'doodle jump' README.md                       # exactly 1
```

## When done

```
git add README.md docs/superpowers/plans/evidence
git commit -m "v3 Task 5: README and evidence"
```

Then print a short report: the README changes, the evidence files with sizes, and the final verification tail.
