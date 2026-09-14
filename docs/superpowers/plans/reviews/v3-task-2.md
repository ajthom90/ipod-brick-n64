# Review: v3 Task 2 (commit 1eee9e5)

Verdict: **pass**, no fixes requested.

Checked:
- `hopper.c`: Q8.8 gravity 64 and bounce −1664 / spring −2560, wrap-around steering, landing only while falling, moving platforms reversing at the edges, camera line 100 with a never-descending camera, score per 10 px, upward generation with gaps 40..80 (cap grows with score), recycling, death below the screen, effects, overlays. Registered seventh with `MUSIC_HOPPER`.
- Fourteen test binaries `0 failure(s)` (rerun by reviewer); autoplay reaches score 50 by tick 385 and dies at tick 1,079 with 126. Purity grep clean; no trademarked titles anywhere in `src` or `tests`.
- Grok's 20 s capture viewed offline (no new emulator run, per the user's meeting request): hopper square with eye, green platforms, SCORE 126 / HIGH 0, 59 VPS.

Accepted deviations: autoplay keeps steering toward its chosen pad through the whole jump instead of re-selecting inside the 8..130 px band (the strict band stalled at score 3); recycling runs before generation so freed slots fill in the same tick; `test_app` now expects a scrollbar because eight rows exceed the seven-row window.
