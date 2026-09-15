# Review: v4 Task 2 (commit 69790bf)

Verdict: **pass** with one documented cosmetic limitation. v4 complete; tagged v4.0.

Checked:
- README "Delta and iOS" section added: `games-soft.z64` via `make rom-soft`, CPU rendering because HLE cannot run rdpq microcode, relies on GLideN64's default framebuffer emulation, built-in font (more retro), rdpq build stays for ares/hardware, and the macOS-27-beta Delta crash caveat. Develop targets and rig scripts documented.
- Ten evidence PNGs (menu + nine games) under the HLE rig. Reviewer's clean verification: 16 test binaries `0 failure(s)`; `games.z64` 294912 and `games-soft.z64` 196608 both build with no warnings.
- Spot-checked captures: menu (BRICK on blue), Brick, and 2048 all render non-black under GLideN64.

Known limitation (built-in font, inherent):
- The libdragon built-in font paints an opaque background cell. `soft_text` maps `DRAW_TEXT_LIGHT` to a blue background so the menu/settings/pause highlight bars read correctly. 2048 is the only game that also uses light text on non-blue tiles, so its digits on colored tiles show a blue cell behind them. The digits stay legible; this is cosmetic and affects only the software build's 2048. A proper fix needs a transparent bitmap-font blitter (draw only foreground pixels), which is disproportionate for a compatibility build; left as optional future polish. The rdpq `games.z64` is unaffected (it uses the Inter font with transparent glyphs).
