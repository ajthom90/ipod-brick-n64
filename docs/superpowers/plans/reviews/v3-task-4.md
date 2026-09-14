# Review: v3 Task 4 (commit ea5f8d9)

Verdict: **pass** on host evidence; emulator capture deferred (Screen Recording permission still lapsed).

Checked:
- `runner.c`: buildings 60..160 wide with roofs 120..200 within 40 px of the previous, gaps 24..64, scrolling at a speed that steps by 26 every 300 ticks to a 1792 cap; jump −1536 with the short-hop cut to −512; landing with `SFX_HIT`, `SFX_MERGE` on jump, `SFX_POINT` per 100 m, wall and fall deaths with `SFX_EXPLODE`; overlays. Registered ninth with `MUSIC_RUNNER`; all nine games now in the registry.
- Sixteen test binaries `0 failure(s)` (rerun by reviewer); autoplay reaches 505 m and ends at tick 1,573. Purity grep clean; no trademarked titles in `src` or `tests`. Both ROMs build without warnings.
- Host frame dump viewed: red runner mid-jump between two dark buildings with a gap.
- ares window capture still fails with "could not create image from window" for both Grok and the reviewer. A failed capture also left ares running because `scripts/ares-shot.sh` exits under `set -e` before its cleanup; Task 5 hardens the script.

Accepted deviation: the starter building is a fixed 240 px roof at y 180 so the run always begins on solid ground.
