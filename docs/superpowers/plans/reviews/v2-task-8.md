# Review: v2 Task 8 (commit 6e0ccc6)

Verdict: **pass**, no fixes requested.

Checked:
- `PAR_SIN` / `PAR_COS` values identical to the plan (31 entries each). Aim clamping, turret and barrel squares, bullets with a point cost, helicopter spawning at `max(60, 150 - score)` ticks with random drops, trooper descent with and without chutes, landings per side with the turret-hit and four-per-side game over, chuteless landings killing standing troopers, bullet hits with `SFX_SHOT` / `SFX_EXPLODE` / `SFX_HIT` / `SFX_GAME_OVER`. Registered fifth.
- Twelve test binaries `0 failure(s)` (rerun by reviewer); `test_parachute` covers the planned cases; autoplay ends in 4,698 ticks at the ceasefire score of 60. Purity grep clean.
- Capture at 30 s viewed: turret with an angled three-square barrel, two helicopters with rotors, SCORE 21 / HIGH 21, 59 VPS.

Accepted deviations: the HUD high score updates live rather than only at game over; the autoplay fires only when a 5-degree aim step is predicted to hit a trooper (otherwise long-range shots miss and the demonstration never reaches the ceasefire); after game over the autoplay presses A and starts a new round, matching Blocks and Snake rather than Brick's stay-on-screen behavior. Task 10's evidence captures use mid-play frames, so this does not matter.
