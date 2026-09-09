# v2 Task 8: Parachute

You are implementing **Task 8 of the v2 plan** for a Nintendo 64 games collection built with libdragon. Tasks 1 to 7 are committed (framework, menu, audio, settings, saves, Brick, Blocks, Snake, Pong). You are adding the fifth game. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-retro-collection.md` — read the header, "Global Constraints", "Shared code", and all of **Task 8**: the header, the layout corrections (ground at y 228, helicopters at y 40..90), the `PAR_SIN` / `PAR_COS` tables (copy verbatim), the rules (aim, turret drawing, bullets, helicopters, troopers, landing, hits, scoring, game over), rendering, autoplay with the ceasefire at score 60, and the test list.
2. `docs/superpowers/specs/2026-09-08-retro-collection-design.md` section 4.5.
3. `src/game.h`, `src/prng.h`, `src/sfx.h`, `src/games/pong.c` (latest descriptor reference), `src/games/registry.c`, `tests/test_pong.c`, `tests/harness.h`.

## The task

Do Task 8 steps 1 to 4 in order: write the failing `tests/test_parachute.c` with every case listed in the plan; implement `src/games/parachute.h` and `src/games/parachute.c` (angle table and `par_aim_index`, aiming, turret and barrel squares, bullets, helicopter spawning and drops, trooper descent with and without a chute, landings per side and on the turret, bullet hits, scoring, `SFX_SHOT` / `SFX_EXPLODE` / `SFX_HIT` / `SFX_GAME_OVER`, HUD and overlays, autoplay, descriptor `GAME_PARACHUTE` with `track = MUSIC_PARACHUTE`); register it fifth; verify; commit.

## Standing rules

- The new game module is pure: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon; randomness only through `prng.h`.
- Do not edit the plan or specs; do not touch other games, the framework, the synth, or the adapter. Update `tests/test_app.c` only if the registered game count requires it.
- Capture screens only with `scripts/ares-shot.sh`; never run `screencapture` yourself or capture the full display. If the script fails, say so and continue.
- Before building the autoplay ROM run `rm -rf build/autoplay` so the previous game's objects are not reused.
- Every command must finish in under four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make test                                              # test_parachute joins the green binaries
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/games/parachute.c src/games/parachute.h   # prints nothing
make frames GAME=parachute                             # build/frames/parachute/frame-0600.png shows the turret, a helicopter, troopers
rm -rf build/autoplay && make rom && make rom-autoplay GAME=parachute && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task8 5 30 90
```

If you can view images: the captures must show the turret with its angled barrel at bottom center, dark helicopters near the top, blue troopers under orange chutes, bullets, "SCORE n" and "HIGH n"; 90 s may already be GAME OVER (the autoplay stops firing at score 60). Report what you see plus the autoplay test's tick count and high score.

## When done

```
git add -A src tests
git commit -m "v2 Task 8: Parachute"
```

Then print a short report: `make test` summary lines, what the captures show, and any deviation.
