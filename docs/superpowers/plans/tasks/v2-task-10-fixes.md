# v2 Task 10 fixes: settings frame dump

Review of your Task 10 commit: README and the seven ares evidence captures are good, and you were right to report rather than patch the settings dump. Now fix it, as a follow-up with source access limited to one function:

1. In `tools/framedump.c`, change `dump_settings` so it no longer walks the menu: after `app_init(&app, false, 0)`, set `app.menu_row = GAME_COUNT` (the SETTINGS row is always the last one), then press A for one tick (`in[0].a = true`, `app_update`, then clear the edge), then render. The result must be the settings screen (header SETTINGS, rows MUSIC / SOUND / VOLUME / BACK, highlight on the first row).
2. Run `make frames GAME=settings` and copy `build/frames/settings/frame-0000.png` to `docs/superpowers/plans/evidence/v2-settings.png` (overwrite).
3. Run `make test` (all binaries green; nothing else changed).
4. Commit:
   ```
   git add tools/framedump.c docs/superpowers/plans/evidence/v2-settings.png
   git commit -m "v2 Task 10: settings frame dump targets the SETTINGS row"
   ```
   Do not stage anything else. Do not push. No emulator runs are needed for this fix.

Then report the stdout `text ...` lines from the settings dump (they should list SETTINGS, MUSIC: ON, SOUND: ON, VOLUME: 7, BACK).
