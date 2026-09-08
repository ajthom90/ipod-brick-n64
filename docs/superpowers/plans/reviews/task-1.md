# Review: Task 1 (commit fbba803)

Verdict: **pass**, no fixes requested.

Checked:
- One commit, tree clean. Diff matches the plan verbatim except the include guard, renamed `BRICK_H` → `BRICK_GAME_H` because `BRICK_H` is also the brick-height enum constant. Correct fix; the plan text has been updated to match.
- `make test`: 5 ok, 0 failures (rerun by reviewer).
- Core purity grep (float/double/math.h/libdragon): clean.
- Makefile recipe lines use tabs; the only space-indented line is the `-include` inside `ifneq`, mirroring libdragon's helloworld example.
- `ipod-brick-n64:dev` image built; `brick.z64` is 196608 bytes.
- Fresh ares capture (`build/shots/review-task1/shot-4s.png`): light `E8E4DC` field, "BRICK" and "PRESS A" centered, 60 VPS.

Notes for later tasks:
- Builtin debug-mono font is small on a 320x240 frame but legible; acceptable for v1 per spec.
