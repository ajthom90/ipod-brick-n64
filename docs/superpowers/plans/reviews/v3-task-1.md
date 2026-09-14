# Review: v3 Task 1 (commit 7004e7f)

Verdict: **pass**, no fixes requested.

Checked:
- Menu: `menu_scroll_for` window rule, `MENU_VISIBLE_ROWS = 7`, narrowed highlight and scrollbar (`172`-px track, thumb proportional) drawn only when rows exceed the window; `app_state` applies the helper after every move; frame dumper's `menu` mode adds the 300-tick frame. Tests cover the helper with synthetic totals and the scrollbar geometry through `menu_render_rows`.
- Save: `SAVE_MAX_GAMES = 12`, `SAVE_VERSION = 2`, decode reads eight or twelve scores by version after magic and CRC checks; migration test on a hand-built version-1 record passes.
- Music: `MUSIC_TRACK_COUNT = 10`; the twelve new pattern strings are identical to the plan (Grok restored two truncated bars during its own paste). Rendered levels: Hopper peak 23430 / RMS 7442, Flap 22550 / 8841, Runner 24530 / 8601, all inside the mixer headroom.
- Thirteen test binaries `0 failure(s)` (rerun by reviewer). Purity grep clean. Menu capture unchanged from v2 (no scrollbar yet with seven rows).
