# v3 Task 4: Runner

You are implementing **Task 4 of the v3 plan** for a Nintendo 64 games collection built with libdragon. Previous v3 tasks are committed. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-14-modern-games.md` — read the header, "Global Constraints", and all of **Task 4**: the header block (copy verbatim), the rules, the test list, and the verification commands.
2. `docs/superpowers/specs/2026-09-14-modern-games-design.md` section 4 (all three games, for the shared conventions) and section 4.3.
3. `src/game.h`, `src/prng.h`, `src/sfx.h`, `src/games/g2048.c` and `src/games/parachute.c` (recent descriptor references: title/game-over overlays, `fmt_label`, `draw_t`, autoplay stop thresholds), `src/games/registry.c`, `tests/test_parachute.c`, `tests/harness.h`.

## The task

Write the failing `tests/test_runner.c` with every case in the plan; implement `src/games/runner.h` and `src/games/runner.c` (buildings with widths, roofs, gaps, spawn and despawn, scrolling at a ramping speed, jump with the short-hop cut, gravity, landing, wall and fall deaths, metres score with `SFX_POINT` every 100, HUD and overlays, autoplay with the hold and stop threshold, descriptor `GAME_RUNNER` with `track = MUSIC_RUNNER`); append it ninth in `src/games/registry.c`; verify; commit.

## Standing rules

- The new game module is pure: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon; randomness only through `prng.h`.
- Do not edit the plan or specs; do not touch other games, the framework, the synth, or the adapter. Update `tests/test_app.c` only if the registered game count requires it.
- The original titles (Doodle Jump, Flappy Bird, Canabalt) must not appear anywhere in code, comments, strings, tests, or commit messages.
- Capture screens only with `scripts/ares-shot.sh`; never run `screencapture` yourself or capture the full display. If the script fails, say so and continue.
- Before building the autoplay ROM run `rm -rf build/autoplay`.
- Every command must finish in under four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make test
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/games/runner.c src/games/runner.h
make frames GAME=runner
rm -rf build/autoplay && make rom && make rom-autoplay GAME=runner && scripts/ares-shot.sh games-autoplay.z64 build/shots/v3-task4 5 20 60
```

Report what the captures show (or that you could not view them) plus the autoplay test's tick count and score.

## When done

```
git add -A src tests
git commit -m "v3 Task 4: Runner"
```

Then print a short report: `make test` summary lines, what the captures show, and any deviation.
