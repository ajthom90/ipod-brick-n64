# Task 7b fixes: ares paused when unfocused

Review of your Task 7b commit: the code changes are correct and all 24 tests pass. The captures were invalid for a reason outside your code: ares's setting `Input/Defocus` defaults to `Pause`, so emulation pauses whenever the ares window is not frontmost (your 3 s capture shows "Paused" in the status bar). Passing the setting on the command line fixes it; this was verified by launching with the flag, moving focus to another app, and seeing the game keep running at 60 VPS.

Do exactly this:

1. In `scripts/ares-shot.sh`, change the launch line to:
   ```
   open -a ares --args --system "Nintendo 64" --setting Input/Defocus=Allow "$ROM_ABS"
   ```
   Also add a comment line above it: `# Input/Defocus=Allow keeps emulation running when the window is not frontmost.`
2. Rerun the capture exactly once, as its own command (about 3 min 25 s):
   `scripts/ares-shot.sh brick-autoplay.z64 build/shots/task7b 3 60 173 175 200`
3. If you can view images, report what each of the five shots shows. Expected: 3 s and 60 s play on level 1 (60 s score around 20 to 30); 173 s and 175 s show "LV 2" with a full or nearly full grid and lives dropping; 200 s shows "GAME OVER", "HIGH SCORE 63", "PRESS A". If the 200 s shot is not game over, do not retry; report what it shows.
4. Regenerate the evidence (overwrite the previous task7b files):
   `for f in build/shots/task7b/*.png; do sips -Z 768 "$f" --out "docs/superpowers/plans/evidence/task7b-$(basename "$f")" >/dev/null; done`
5. Commit:
   ```
   git add scripts/ares-shot.sh docs/superpowers/plans/evidence
   git commit -m "Task 7b: keep ares running when unfocused, recapture level-2 and game-over evidence"
   ```
   Do not stage anything else. Do not push. Keep every command under four minutes.

Then print a short report with what each shot shows.
