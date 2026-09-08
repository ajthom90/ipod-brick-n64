# Review: Task 7b (commits 0a8efe4, bf4e9c7)

Verdict: **pass**. Level 2 and game over are now verified in ares; Task 7's open items are closed.

Checked:
- Autoplay PLAY branch matches the plan; `make test` 24 ok with `level 2 after 10269 ticks (171 s)` (rerun by reviewer). Core purity clean.
- Adapter computes autoplay input inside the catch-up loop; the normal build still samples the controller once per frame. Verified determinism: a focused and an unfocused ares run produced pixel-identical frames at 12 s.
- First recapture failed because ares's `Input/Defocus` setting pauses emulation when its window is not frontmost (status bar read "Paused", 15 VPS). Fix in commit bf4e9c7 passes `--setting Input/Defocus=Allow` at launch; reviewer proved it by moving focus to another app mid-run and seeing 60 VPS and a rising score.
- Second recapture viewed: 173 s shows SCORE 60 / LIVES 3 / LV 2 with a full grid; 200 s shows SCORE 63 / LIVES 0 / LV 2 and the overlay "GAME OVER", "HIGH SCORE 63", "PRESS A", all between the grid and the paddle. Timing matched the host prediction to the second.

Cosmetic follow-up for Task 9: on GAMEOVER the ball is still drawn at its last position below the paddle. Skip drawing the ball in that state.
