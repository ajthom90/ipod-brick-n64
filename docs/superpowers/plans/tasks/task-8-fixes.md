# Task 8 fixes: keep the resample filter

Review of your uncommitted Task 8 work: the Makefile font rules, DragonFS wiring, `.gitignore`, README line, font loading, and new text baselines are all correct, and stopping on the assertion was the right call.

The assertion is a hard hardware limit in libdragon (`display.c`: resampling cannot be disabled for 16-bit framebuffers at widths of 320 or less because of an NTSC hardware bug), so the crisp-filter part of the task is not achievable at 320x240 16 bpp. Do exactly this:

1. In `src/n64/app.c`, change the `display_init` call back to `FILTERS_RESAMPLE` and add this comment on the line above it:
   `/* FILTERS_DISABLED asserts at 320x240 16 bpp (hardware bug, libdragon display.c); resampling stays on. */`
2. Rebuild: `make rom && make rom-autoplay` (zero warnings).
3. Capture: `scripts/ares-shot.sh brick.z64 build/shots/task8-title 4` and `scripts/ares-shot.sh brick-autoplay.z64 build/shots/task8-play 8`.
4. If you can view images: the title must show "BRICK", "HIGH SCORE 0", "PRESS A" in a large clean sans-serif; the play shot must show the HUD in the small font with glyph tops below about y 16 of the 240-line frame and above the top brick row. Report what you see.
5. Commit:
   ```
   git add Makefile .gitignore src/n64/app.c README.md
   git commit -m "Task 8: Inter font via DragonFS"
   ```
   Do not stage anything else. Do not push. Keep every command under four minutes.

Then print a short report: ROM sizes, what the two screenshots show, and confirmation that `filesystem/hud.font64` and `filesystem/big.font64` exist.
