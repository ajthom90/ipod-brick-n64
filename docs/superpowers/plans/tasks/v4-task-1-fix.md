# v4 Task 1 fix: readable text on the highlight bar

Review of your Task 1 commit: the software build works, the rig is correct, and the Brick capture is perfect. One issue you correctly flagged: the built-in font paints an opaque background cell, so on the menu/settings/pause highlight bar the light text draws a `COLOR_BG` (beige) box over the blue bar and the selected item is unreadable.

Fix it in one place. In `src/n64/app.c` `soft_text`, choose the glyph background color from the text color instead of always using `COLOR_BG`. `DRAW_TEXT_LIGHT` is only ever used on the blue highlight bar (`COLOR_BLUE`), and `DRAW_TEXT_DARK` is always on the `COLOR_BG` field, so:

```c
    uint32_t back = (c == DRAW_TEXT_LIGHT) ? COLOR_BLUE : COLOR_BG;
    graphics_set_color(soft_col(c), soft_col(back));
```

(replace the current `graphics_set_color(soft_col(c), soft_col(COLOR_BG));` line). `DRAW_TEXT_LIGHT`, `DRAW_TEXT_DARK`, `COLOR_BLUE`, and `COLOR_BG` are all already defined in `game.h`.

Then:
1. `make rom-soft` (no warnings).
2. `scripts/hle-shot.sh games-soft.z64 build/hle-shots/fix-menu 4` and, if you can view it, confirm the highlighted row now reads "BRICK" in light text on the blue bar. Report what you see.
3. Commit:
   ```
   git add src/n64/app.c
   git commit -m "v4 Task 1: readable highlight-bar text in the software build"
   ```
   Do not stage anything else. Do not push. Keep every command under four minutes; do not rebuild the HLE rig (it already exists in build/hle).
