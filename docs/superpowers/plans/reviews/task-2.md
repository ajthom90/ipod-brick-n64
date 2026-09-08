# Review: Task 2 (commit 1f7b095)

Verdict: **pass**, no fixes requested.

Checked:
- One commit, only `src/brick.c` and `tests/test_brick.c` changed. Implementation matches the plan's behavior list and skeleton exactly; `step_ball` left as a stub as required.
- `make test`: 14 ok, 0 failures (rerun by reviewer). Grok confirmed the tests failed first.
- Core purity grep: clean.
- Integer division in `move_paddle` truncates toward zero for negative analog values (-255 → -5 px, not -6). Acceptable; the stick is clamped to ±80 so ±256 is the only full-deflection value and it maps exactly.

Plan correction made during this review (before Task 3 starts): `test_brick_side_hit_flips_x` placed the ball in the 2 px gap between columns 2 and 3, where a 6 px ball already overlaps column 2, so the row-major scan would have cleared the wrong cell. The test now clears cell (5,2) in its setup.
