# v2 Task 4: Audio in the adapter, Settings screen, EEPROM save

You are implementing **Task 4 of the v2 plan** for a Nintendo 64 games collection built with libdragon. Tasks 1 to 3 are committed: the framework, menu, and a host-tested chiptune synth exist, but the N64 build is still silent and has no settings or saving. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-retro-collection.md` — read the header, "Global Constraints", "Shared code", and all of **Task 4**. Use the interfaces and the adapter snippets **verbatim**.
2. `docs/superpowers/specs/2026-09-08-retro-collection-design.md` sections 3.8 (adapter paragraph), 3.9, and 3.10.
3. `src/app_state.h`, `src/app_state.c`, `src/menu.h`, `src/menu.c`, `src/synth.h`, `src/music.h`, `src/n64/app.c`, `tools/framedump.c`, `Makefile`, `tests/test_app.c`.

## The task

Do Task 4 steps 1 to 5 in order: failing tests (`tests/test_save.c`, `tests/test_settings.c`, additions to `tests/test_app.c`); implement `src/save.c/.h` (CRC32, encode/decode, defaults) and `src/settings.c/.h` (settings screen); extend `app_t` and `app_update` with the settings screen, `settings_dirty`, `app_high_scores`, `app_apply_save`; adapter: `audio_init`, parse all tracks at boot, EEPROM load through `eepromfs`, `apply_settings`, per-frame save when dirty, track switching, sound-effect playback, audio buffer filling; Makefile `N64_ROM_SAVETYPE = eeprom4k`; `tools/framedump.c` accepts `settings`; verify; commit.

Also, as part of this task (a Task 2 review follow-up): in `app_init`, the autoplay build must start its game with the fixed seed `0x1234567u` (not `1`) so emulator autoplay matches the host frame dumps.

Facts you can rely on (verified against libdragon trunk headers):
- `audio_init(int frequency, int numbuffers)`, `audio_get_frequency()`, `audio_can_write()`, `short *audio_write_begin(void)`, `audio_get_buffer_length()` (stereo frames), `audio_write_end(void)`.
- `eeprom_present()` returns `EEPROM_NONE`, `EEPROM_4K`, or `EEPROM_16K`. `eepfs_init(const eepfs_entry_t *entries, size_t count)` returns 0 on success; `eepfs_entry_t` has `path` and `size` fields; `eepfs_verify_signature()`, `eepfs_wipe()`, `eepfs_read(path, dest, size)`, `eepfs_write(path, src, size)` return 0 on success.
- n64.mk honors `N64_ROM_SAVETYPE = eeprom4k`.
- `assertf(cond, fmt, ...)` exists in libdragon for boot-time checks.

## Standing rules

- `src/save.*` and `src/settings.*` are pure modules: only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<stddef.h>`; no floats, no stdio, no libdragon. EEPROM and audio calls live only in `src/n64/app.c`.
- Do not edit the plan or specs; do not touch `src/games/*`, `src/synth.*`, `src/music*`.
- Every command must finish in under four minutes. Never use mupen64plus. Do not `git push`.
- If a libdragon call is rejected by the compiler, fix only that line minimally and report the change with the compiler message. If ares shows an assertion screen, report its text and stop.

## Verification (all must pass before committing)

```
make test                                              # test_save, test_settings join the green binaries; test_app extended
grep -nE '\b(float|double)\b|#include <(math|stdio)\.h>|libdragon' src/*.h src/*.c src/games/*.c src/games/*.h   # prints nothing
make frames GAME=settings                              # settings screen with four rows and the highlight bar
make rom && scripts/ares-shot.sh games.z64 build/shots/v2-task4-menu 4
make rom-autoplay GAME=brick && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-task4-auto 8
```

The menu capture must look as in Task 2 (no assertion). Report what you see. The reviewer will verify audio and EEPROM persistence by hand in ares.

## When done

```
git add -A src tests tools Makefile
git commit -m "v2 Task 4: audio playback, settings screen, EEPROM save of settings and high scores"
```

Then print a short report: `make test` summary lines, ROM sizes, what the captures show, and any deviation.
