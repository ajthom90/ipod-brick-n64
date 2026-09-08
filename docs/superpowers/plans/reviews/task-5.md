# Review: Task 5 (commit b0d7058)

Verdict: **pass**, one cosmetic follow-up folded into Task 6.

Checked:
- One commit, only `src/n64/app.c`; byte-identical to the plan's Task 5 code.
- Reviewer rebuilt both ROMs (`make rom`, `make rom-autoplay`): 212992 bytes each, no errors.
- Fresh ares captures viewed: title shows "BRICK" / "HIGH SCORE 0" / "PRESS A"; autoplay at 6 s shows HUD "SCORE 2 / LIVES 3 / LV 1", six colored rows, paddle, ball; at 12 s "SCORE 5" with more bricks gone. Rendering matches the host frame dumps from Task 4. 59-60 VPS.
- Overscan margins hold: HUD baseline and paddle sit inside the 16 px border.

Follow-up (Task 6): the autoplay build warns `'read_input' defined but not used` because the function is compiled but never called under `BRICK_AUTOPLAY`. Wrap its definition in `#ifndef BRICK_AUTOPLAY`.
