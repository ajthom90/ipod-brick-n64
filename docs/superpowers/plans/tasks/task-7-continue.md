# Task 7, continued

Your previous session ended while `scripts/ares-shot.sh` was still running, but the script completed on its own. All six captures now exist:

```
build/shots/task7/shot-3s.png shot-20s.png shot-60s.png shot-332s.png shot-345s.png shot-390s.png
```

Do NOT run the long capture again. Finish Task 7 steps 5 to 7 only:

1. If you can view images, open each of the six PNGs and note what it shows (score, lives, level, overlay text). If you cannot, check each is larger than 20 KB and say so.
2. Create the evidence files, downscaled to 768 px wide:
   `mkdir -p docs/superpowers/plans/evidence && for f in build/shots/task7/*.png; do sips -Z 768 "$f" --out "docs/superpowers/plans/evidence/task7-$(basename "$f")" >/dev/null; done`
3. Run `make test` (expect 24 ok).
4. Commit exactly:
   ```
   git add docs/superpowers/plans/evidence
   git commit -m "Task 7: verify title, play, level-up, and game-over screens in ares"
   ```
   Do not stage anything else. Do not push.

Then print a short report listing what each screenshot shows and the six evidence file names. Keep every tool call short; nothing here should take more than a minute.
