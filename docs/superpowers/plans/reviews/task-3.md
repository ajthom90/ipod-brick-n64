# Review: Task 3 (commit a397cd6)

Verdict: **pass**, no fixes requested.

Checked:
- One commit, only `src/brick.c` and `tests/test_brick.c` changed. `step_ball`, `pass_x`, `pass_y`, `hit_brick`, `bounce_off_paddle`, and `brick_autoplay_input` follow the behavior list, including check order (walls → one brick → paddle while descending → loss) and per-sub-step velocity division.
- `make test`: 24 ok, 0 failures (rerun by reviewer). Grok confirmed the new tests failed first. No tuning of the autoplay rotation was needed.
- Core purity grep: clean. No time source, no randomness: deterministic.
- Speed magnitude is preserved: walls and bricks negate one component; the paddle replaces both from the unit-vector table.
- `pass_y` evaluates the loss condition on the rectangle captured before the brick/paddle corrections. Safe: a paddle overlap needs `y0 < 224` and bricks end at y 107, while a loss needs `y0 >= 228`, so the corrections can never mask or fake a loss. Noted as fragile, not worth a change.

Measured: autoplay reaches level 2 after 19,569 ticks (326 s at 60 Hz). The plan's Task 7 screenshot schedule assumed under three minutes; it has been updated to capture at 330 s and later.
