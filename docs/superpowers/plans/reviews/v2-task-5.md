# Review: v2 Task 5 (commit b46f570)

Verdict: **pass**, no fixes requested.

Checked:
- `BLK_SHAPES` and `BLK_GRAVITY` values identical to the plan (224 shape numbers compared numerically). Kicks `0, -1, +1, -2, +2`; DAS 16/6; 7-bag shuffle; lock → `SFX_HIT`; flash 20 ticks → line clear with `40/100/300/1200 × level` and `SFX_CLEAR`; game over on blocked spawn with `SFX_GAME_OVER`.
- Nine test binaries `0 failure(s)` (rerun by reviewer); `test_blocks` has the 14 planned cases; the autoplay heuristic ends a game in 3,446 ticks with 21 lines cleared. Purity grep clean; no occurrence of the trademarked name anywhere.
- Capture at 20 s viewed: bordered well with a mixed stack, a cleared row mid-flash, NEXT preview showing a T, SCORE 120 / LEVEL 1 / LINES 3, HIGH 0 on the left, 60 VPS.

Accepted deviations: a `drop_armed` field for the hard-drop edge; `test_app` adjusted for two registered games; a stale `build/autoplay` needed a clean rebuild once (the autoplay ROM otherwise kept Brick's objects).
