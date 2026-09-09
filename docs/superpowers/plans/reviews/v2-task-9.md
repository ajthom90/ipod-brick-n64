# Review: v2 Task 9 (commit 1d4e8fe)

Verdict: **pass**, no fixes requested. All six games are registered.

Checked:
- `g2048.c`: `g2048_slide_row` with single merges per tile, `g2048_move` for four directions, tile spawn with 9/10 twos, `can_move`, score, `reached_2048` with `SFX_CLEAR` once, `SFX_MERGE`, `SFX_GAME_OVER`, edge-triggered D-pad and armed stick, color table, big/small numerals, HUD and overlays. Registered sixth as "2048".
- Thirteen test binaries `0 failure(s)` (rerun by reviewer); autoplay reaches game over in 2,881 ticks with score 3,476. Purity grep clean.
- Capture at 30 s viewed: dark board, tiles 2 through 256 in the specified colors with centered numbers, SCORE 2060 / HIGH 0, 60 VPS.

Accepted deviation: the autoplay rotation adds "up" as a last resort when down/left/right cannot change the board, so it cannot stall short of game over.
