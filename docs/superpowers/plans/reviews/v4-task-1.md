# Review: v4 Task 1 (commits 6c5e107, 9e7cd8a)

Verdict: **pass**. The software build renders on Delta's video stack.

Checked:
- `src/n64/app.c` change is additive: the rdpq path is wrapped in `#ifndef SOFTRENDER` (bodies unchanged), the software `soft_rect`/`soft_text` backend and the `SOFTRENDER` boot/frame branches are new. 16 test binaries still `0 failure(s)`.
- `make rom-soft` produces `games-soft.z64` (196608 bytes, no DFS) with no warnings; `scripts/build-hle-rig.sh` builds mupen64plus-core + GLideN64 from source; `scripts/hle-shot.sh` captures the mupen window.
- Reviewer viewed the captures under the rig (mupen64plus master + GLideN64, Delta's stack): Brick shows the full colored grid, paddle, ball, and HUD; the menu shows GAMES, the rule, the game list, the scrollbar, and after the fix the highlighted row reads "BRICK" in light text on the blue bar. All clearly non-black, which is the whole point: HLE emulators display the CPU framebuffer.
- Fix commit 9e7cd8a: `soft_text` now uses `COLOR_BLUE` as the glyph background for `DRAW_TEXT_LIGHT` (only ever on the highlight bar) and `COLOR_BG` otherwise, so the built-in font's opaque cell matches what's beneath it.

Accepted deviations: clamp lines split for `-Werror=misleading-indentation`; the rig scripts also create `build/hle/cfg` and `brew install cmake/pkgconf` if missing. Both fine.
