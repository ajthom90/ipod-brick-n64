# Review: v2 Task 7 (commit 050d023)

Verdict: **pass**, no fixes requested.

Checked:
- `pong.c`: title mode rows, serve delay and direction, Q8.8 ball with sub-steps above serve speed, wall and paddle bounces using Brick's 7-zone table on the vertical offset, speed growth by 64 up to 2048, scoring with `serve_to` to the conceding side, first to 11, margin high score for player-1 wins only, `pong_ai` (tracks the ball while it approaches, otherwise drifts home at 1 px per tick), effects, dashed center line, big-font scores, overlays. Registered fourth with `players = 2`.
- Eleven test binaries `0 failure(s)` (rerun by reviewer); `test_pong` covers the planned cases; autoplay ends 11-0 in 14,925 ticks. Purity grep clean.
- Capture at 90 s viewed: two dark paddles, dashed center line, scores 3 and 0, ball in play, 60 VPS.

Notes:
- The autoplay match is lopsided because the left paddle's AI output is converted to a full-deflection analog stick (6 px per tick) while the CPU paddle is capped at `PNG_AI_SPEED` = 3. Fine for a demonstration; the CPU difficulty for humans is that same constant, tunable later.
- Two-controller play in ares still needs a human check with port 2 mapped; deferred to the final play-through in Task 10.
