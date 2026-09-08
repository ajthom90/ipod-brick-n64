# Review: Task 8 (commit e8b3d80)

Verdict: **pass**; the "crisp filter" half of the task was dropped for a hardware reason.

Checked:
- Makefile font rules convert `assets/Inter-Bold.ttf` at 12 px and 22 px into `filesystem/`, the DFS is appended to both ROMs (229376 bytes, up from 212992), `filesystem/` ignored, `clean` removes it.
- Adapter: `dfs_init` first, two fonts loaded from `rom:/`, HUD at baseline 26 in the 12 px font, overlays in the 22 px font with 28 px spacing. Reviewer rebuilt both ROMs with zero warnings.
- Grok's first attempt hit libdragon's assertion: `FILTERS_DISABLED` is refused for 16-bit framebuffers at widths of 320 or less because of an NTSC hardware bug (libdragon `display.c`). Grok stopped without committing, as instructed; the fix keeps `FILTERS_RESAMPLE` with a comment explaining why.
- Captures viewed: title "BRICK / HIGH SCORE 0 / PRESS A" in large Inter Bold; play HUD "SCORE 3 / LIVES 3 / LV 1" legible above the grid with a small gap. 59-60 VPS.

Not achievable at this resolution and depth: nearest-neighbor output. Only a 32-bit framebuffer or a 640-wide mode could disable resampling; neither is worth the cost for this game.
