# Task 8: Real font and crisp video output

You are implementing **Task 8** of an approved plan for a Nintendo 64 port of the iPod "Brick" game. Tasks 1 to 7b are committed; the game is complete and verified but uses libdragon's tiny builtin debug font. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-ipod-brick-n64.md` — read the header, "Global Constraints", and all of **Task 8**. Use its Makefile and C snippets **verbatim**.
2. `Makefile`, `.gitignore`, `src/n64/app.c`, `README.md` — the files you change. `assets/Inter-Bold.ttf` and `assets/LICENSE-Inter.txt` are already committed.

## The task

Do Task 8 steps 1 to 5 in order: Makefile font conversion and DragonFS rules, `.gitignore`, adapter changes (`dfs_init` first, `FILTERS_DISABLED`, two fonts, new baselines), README line, build both ROMs, capture, commit.

Facts you can rely on (verified on this machine):
- `mkfont` and `mkdfs` exist in the container at `$N64_INST/bin` and n64.mk exposes them as `$(N64_MKFONT)` and `$(N64_MKDFS)`; `%.dfs` rules already exist in n64.mk and pack the `filesystem/` directory.
- `mkfont --size 12 --display 320x240 -o <dir> assets/Inter-Bold.ttf` writes `<dir>/Inter-Bold.font64` (about 3.6 KB); at `--size 22` about 7.8 KB. It does not create `<dir>`.
- Adding `$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs` makes n64.mk append the filesystem to the ROM, exactly as libdragon's fontdemo example does.
- In the adapter, `dfs_init(DFS_DEFAULT_LOCATION)` must run before `rdpq_font_load("rom:/hud.font64")`.

## Standing rules

- Do not edit the plan, the spec, `src/brick.*`, `tests/`, `tools/`, `scripts/`, or `assets/`.
- Every command must finish in under four minutes (the ROM builds take about 40 s each; the captures 4 s and 8 s).
- Never use mupen64plus. Do not `git push`.
- If the ROM build fails inside the Makefile rules, fix only the rule (tabs, paths) and report what changed. If ares shows a libdragon assertion screen instead of the game, report the text you can read and stop; do not guess at fixes in libdragon calls.

## Verification (all must pass before committing)

```
make test                                       # 24 ok
make rom && make rom-autoplay                   # zero warnings; both ROMs about 12 KB larger than 212992 bytes
scripts/ares-shot.sh brick.z64 build/shots/task8-title 4
scripts/ares-shot.sh brick-autoplay.z64 build/shots/task8-play 8
ls -la brick.z64 brick-autoplay.z64 filesystem/ build/shots/task8-title build/shots/task8-play
```

If you can view images: the title must show "BRICK", "HIGH SCORE 0", "PRESS A" in a clean large sans-serif with crisp (not blurred) pixels; the play shot must show a small, clearly legible HUD ("SCORE n", "LIVES 3", "LV 1") whose glyphs stay below about y 14 of the 240-line frame, above the top brick row, with the bricks now hard-edged. Report what you see or that you could not view them.

## When done

```
git add Makefile .gitignore src/n64/app.c README.md
git commit -m "Task 8: Inter font via DragonFS and crisp video output"
```

Then print a short report: ROM sizes, the mkfont/DFS lines from the build output, what the two screenshots show, and any deviation.
