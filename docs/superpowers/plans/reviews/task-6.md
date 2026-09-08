# Review: Task 6 (commit 26a93df)

Verdict: **pass**, no fixes requested.

Checked:
- One commit: `src/n64/app.c` (loop replaced verbatim from the plan; `read_input` wrapped in `#ifndef BRICK_AUTOPLAY`) and README controls/targets.
- Reviewer rebuilt both ROMs: zero warnings.
- Fresh captures at 4, 8, 15 s viewed. At 15 s the HUD reads SCORE 5, which matches the host frame dump's pace (score 5 through tick 780, 6 at 810) once the roughly one-second boot is subtracted, so the accumulator runs the game at the intended 60 Hz.
- Edge-input latching is correct: presses are OR-ed in every frame and cleared only after a tick consumes them, so a press during a frame with no tick is not lost and never double-fires.
