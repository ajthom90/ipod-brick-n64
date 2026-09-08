# Review: v2 Task 1 (commit e2f429c)

Verdict: **pass**, no fixes requested.

Checked:
- `src/game.h`, `src/prng.h`, `src/sfx.h`, `tests/harness.h` byte-identical to the plan. `registry.c` has `GAMES[]`, case-insensitive `game_index_by_name`, `fmt_label`, `fmt_int`.
- Brick moved to `src/games/` with `git mv`; API now takes `input_t`, PAUSE state removed, sound-effect queue added, `brick_render` on `draw_t`, `GAME_BRICK` descriptor. Rules unchanged: all ported v1 tests pass plus `test_sfx_events` and `test_descriptor`.
- `make test` runs three binaries, all `0 failure(s)` (rerun by reviewer). Purity grep over the pure modules: clean.
- Adapter read in full: `n64_rect`/`n64_text` as planned (text switches to standard mode and back to fill mode), both controller ports read per frame, Start latched for Task 2, autoplay build selects the game by name.
- Capture at 8 s of `games-autoplay.z64` (Brick): identical to v1's play screen, 59 VPS.

Accepted deviations: `-Isrc` for the N64 build and `-Itests` for host tests; `read_port`/`axis` compiled out of autoplay builds to avoid an unused-function warning; `fmt_int` digits-only; a stale v1 dependency file required clearing `build/n64` once.
