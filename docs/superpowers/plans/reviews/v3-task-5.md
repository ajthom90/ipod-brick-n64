# Review: v3 Task 5 (commit 2d04544)

Verdict: **pass** for the README, script hardening, and host evidence. Emulator evidence for Flap, Runner, and the scrolled menu is pending the macOS Screen Recording permission; `v3-hopper.png` comes from Task 2's ares capture.

Checked:
- README: nine games listed, controls rows for Hopper, Flap, Runner, the scrolling-menu note, and the trademark note extended to the three originals (the only place those titles appear).
- `scripts/ares-shot.sh` now reports a failed capture, closes ares, and exits non-zero instead of leaving the emulator open.
- Reviewer's clean verification: `make clean && make test` → 16 binaries `0 failure(s)`; purity grep clean; `make rom` → `games.z64` 294912 bytes with no warnings.
- Renderer safety: Hopper, Flap, and Runner each clip rectangles to the screen before calling the draw API, so scrolled-off geometry never reaches `rdpq_fill_rectangle` with negative coordinates.
- `v3-menu-bottom.png` (host frame) viewed: window scrolled to the last row with the narrowed highlight and the scrollbar thumb at the bottom.

Open: ares captures of Flap, Runner, and the scrolled menu, then the `v3.0` tag.

Update 2026-09-15: Screen Recording permission restored. Reviewer captured `games.z64` (menu with scrollbar, seven of ten rows visible) and the Hopper, Flap, and Runner autoplay ROMs at 20 s: Flap threading pipe pairs at score 21, Runner mid-jump over a gap at 343 m, Hopper climbing. Evidence files `v3-menu.png`, `v3-hopper.png`, `v3-flap.png`, `v3-runner.png` added. v3 complete; tagged `v3.0`.
