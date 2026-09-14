# v3 Task 5: README, evidence, final verification

You are implementing **Task 5 of the v3 plan**, the last task, for a Nintendo 64 games collection built with libdragon. Tasks 1 to 4 of v3 are committed: nine games, a scrolling menu, save v2, and ten tunes. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-14-modern-games.md` — read the header and all of **Task 5**.
2. `docs/superpowers/specs/2026-09-14-modern-games-design.md` sections 2 and 4.
3. `README.md`, `src/games/registry.c`, `scripts/ares-shot.sh`.

## The task

0. **Capture script hardening** (the one source change allowed in this task): in `scripts/ares-shot.sh`, make the `screencapture` call failure-safe so ares is always closed: replace the bare `screencapture -x -o -l "$WID" "$OUT/shot-${SEC}s.png"` line with
   ```
   if ! screencapture -x -o -l "$WID" "$OUT/shot-${SEC}s.png"; then
     echo "capture failed at ${SEC}s (is Screen Recording permission granted?)" >&2
     pkill -x ares 2>/dev/null || true
     exit 1
   fi
   ```
   Commit it together with the rest of this task (add `scripts/ares-shot.sh` to the `git add`).

1. **README**: add HOPPER, FLAP, and RUNNER to the games list (one line each) and the controls table (Hopper: stick or D-pad steers, wraps at the edges; Flap: A flaps; Runner: A jumps, release early for a short hop); mention that the menu scrolls with a scrollbar when more than seven rows exist; extend the trademark note to say that Hopper, Flap, and Runner are original implementations of their genres and are not affiliated with or endorsed by the owners of Doodle Jump, Flappy Bird, or Canabalt. Those three titles may appear only in that note.
2. **Evidence**: `make frames GAME=menu` and copy `build/frames/menu/frame-0300.png` to `docs/superpowers/plans/evidence/v3-menu-bottom.png`; capture `games.z64` at 4 s into `build/shots/v3-final-menu`; for each name in `hopper flap runner`: `rm -rf build/autoplay && make rom-autoplay GAME=<name> && scripts/ares-shot.sh games-autoplay.z64 build/shots/v3-final-<name> 20`; downscale each ares capture to 768 px wide into `docs/superpowers/plans/evidence/v3-<name>.png` (menu as `v3-menu.png`) with `sips -Z 768`.
3. **Final verification**: `make clean && make test && make music && make rom`.
4. Commit.

## Standing rules

- Do not edit the plan, the specs, or any source file other than `scripts/ares-shot.sh` as described in step 0; otherwise this task changes `README.md` and adds evidence images only. If a build or test fails, stop and report.
- Capture screens only with `scripts/ares-shot.sh`; never run `screencapture` yourself or capture the full display. Window capture is currently failing on this Mac (a macOS permission the user must restore); if a capture fails, note it, make sure ares is closed (`pkill -x ares`), and continue with the README, the host frame evidence, and the final verification. Missing ares evidence will be added later by the reviewer.
- Run builds and captures as separate commands so none exceeds four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make clean && make test
make music && make rom
ls -la docs/superpowers/plans/evidence/v3-*.png        # at least v3-menu-bottom.png; the ares captures if the permission works
grep -ci 'doodle jump' README.md                       # exactly 1
```

## When done

```
git add README.md scripts/ares-shot.sh docs/superpowers/plans/evidence
git commit -m "v3 Task 5: README and evidence"
```

Then print a short report: the README changes, the evidence files with sizes, and the final verification tail.
