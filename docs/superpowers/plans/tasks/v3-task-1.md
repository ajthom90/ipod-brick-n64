# v3 Task 1: Scrolling menu, save record v2, three tunes

You are implementing **Task 1 of the v3 plan** for a Nintendo 64 games collection built with libdragon. v2 is complete (six games, menu, audio, settings, EEPROM saves, tagged v2.0). v3 adds three games; this task prepares the framework for them. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-14-modern-games.md` — read the header, "Global Constraints", and all of **Task 1**, including the three tune strings (copy verbatim).
2. `docs/superpowers/specs/2026-09-14-modern-games-design.md` sections 3.1 to 3.3.
3. `src/menu.h`, `src/menu.c`, `src/app_state.h`, `src/app_state.c`, `src/save.h`, `src/save.c`, `src/game.h`, `src/music_data.c`, `tools/framedump.c`, `tests/test_app.c`, `tests/test_save.c`, `tests/test_music.c`, `tests/harness.h`.

## The task

Do Task 1 steps 1 to 4: write the failing tests listed in the plan (scroll helper cases, scrollbar geometry through `menu_render_rows` with a synthetic total, save v2 round-trip and version-1 migration, ten parsed tunes with the three new tempos); implement `menu_scroll_for`, `menu_render_rows`, the scroll window and scrollbar in the games menu, `menu_scroll` in `app_t`; `SAVE_MAX_GAMES = 12` and `SAVE_VERSION = 2` with version-1 decoding; the three track ids and `MUSIC_SRC` entries; the frame dumper's `menu` mode adding a frame after 300 ticks of holding down; verify; commit.

Notes:
- With six games registered the menu has seven rows, exactly the visible count, so no scrollbar appears yet in the real menu; that is expected. The scrollbar is tested through `menu_render_rows` with `total = 10`.
- Keep the version-1 decode path: magic and CRC first, then read eight or twelve scores by version.
- If the parser rejects a bar in a tune string, change only a rest length so that bar has 16 steps and list every such change in your report and commit message.

## Standing rules

- Pure modules stay pure (`src/menu.*`, `src/app_state.*`, `src/save.*`, `src/music_data.c`): only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon.
- Do not edit the plan or specs; do not touch any game module, the synth, or the adapter.
- Capture screens only with `scripts/ares-shot.sh`; never run `screencapture` yourself or capture the full display.
- Every command must finish in under four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make test                                              # all binaries 0 failure(s)
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/*.h src/*.c src/games/*.c src/games/*.h   # prints nothing
make music                                             # 10 tunes + 11 effects
make frames GAME=menu                                  # frame-0300.png exists (last row highlighted)
make rom && scripts/ares-shot.sh games.z64 build/shots/v3-task1 4
```

If you can view images: the 4 s capture must match the v2 menu (six games plus SETTINGS, BRICK highlighted, no scrollbar). Report what you see.

## When done

```
git add -A src tests tools
git commit -m "v3 Task 1: scrolling menu, save record v2, three tunes"
```

Then print a short report: `make test` summary lines, the `make music` listing for the three new tunes, and any deviation or tune fix.
