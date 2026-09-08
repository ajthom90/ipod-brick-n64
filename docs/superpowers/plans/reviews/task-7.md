# Review: Task 7 (commit e63980d)

Verdict: **partial pass**. Title, serve, and play states are verified in ares; level 2 and game over were not reached, so their overlays remain unverified. Follow-up is Task 7b.

Checked (reviewer viewed the full-size captures):
- 3 s: play has begun, SCORE 1 / LIVES 3 / LV 1, full grid minus one brick.
- 60 s: SCORE 26, grid cleared from the middle outward.
- 332 s, 345 s, 390 s: still level 1, SCORE 58, the red and orange bricks in the top-right corner remain, ball still in play.

Root cause, two parts:
1. The adapter samples `brick_autoplay_input` once per rendered frame and reuses it for catch-up ticks, so the emulator run is not tick-identical to the host simulation; it took a different path from the same deterministic core.
2. The autoplay policy sends the ball to the far half of the field but never aims at specific bricks, so isolated corner bricks can survive for minutes.

Process finding: Grok's session ended silently (exit 0, truncated output) while the 390 s capture script ran; the script completed and the session was resumed with `grok -c`. Grok tool calls must stay under about four minutes.

Fix (Task 7b): aim the ball at the nearest remaining brick, rotate zones by time to avoid loops, and compute autoplay input per tick in the adapter. Host spike of that policy: level 2 at 10,269 ticks (171 s), game over at 10,579 ticks (176 s).
