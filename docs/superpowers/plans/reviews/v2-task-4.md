# Review: v2 Task 4 (commit bde3d5d)

Verdict: **pass** on everything measurable; audio by ear and EEPROM persistence across a relaunch are delegated to the user.

Checked:
- `save.c` (CRC32, big-endian encode/decode, defaults), `settings.c` (screen), `app_state` extensions (settings screen, dirty flags, `app_high_scores`, `app_apply_save`), autoplay seed now `0x1234567u`. Eight test binaries `0 failure(s)` (rerun by reviewer). Purity grep clean.
- Adapter: `audio_init(22050, 4)`, all seven tracks parsed at boot, EEPROM load through `eepromfs` with signature check and wipe, save written only when settings or a high score change, track switching on screen change, effects drained into the synth, audio buffers filled each frame. `N64_ROM_SAVETYPE = eeprom4k`; ROMs 262144 bytes.
- ares wrote `games.eeprom` (512 bytes) next to the ROM with the eepromfs signature and no record yet, which is correct before any setting changes. `*.eeprom` added to `.gitignore`.
- Reviewer captures: menu at 4 s identical to Task 2, Brick autoplay at 8 s identical to Task 1. No assertion screens, so audio and EEPROM initialisation succeeded.

Process findings:
- Grok's own `scripts/ares-shot.sh` runs failed with "could not create image from window" in its session, and it fell back to full-display screenshots saved under `build/shots/`. Those files were deleted by the reviewer. New standing rule for every Grok prompt: never capture the screen by any means other than `scripts/ares-shot.sh`; if it fails, report and continue.
- `settings.c` keeps key-repeat timers in file-static variables to keep `settings_screen_t` as specified; acceptable, noted.

Update 2026-09-09: the user confirmed by hand in ares that the menu tune, the Brick tune, and the brick-hit effect play, and that a volume change survived quitting and relaunching. Task 4 fully closed.
