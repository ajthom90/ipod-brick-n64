# v2 Task 10: Collection README and evidence

You are implementing **Task 10 of the v2 plan**, the last task, for a Nintendo 64 games collection built with libdragon. Tasks 1 to 9 are committed: six games, menu, pause menu, settings, saves, music, and effects all work. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-08-retro-collection.md` — read the header, "Global Constraints", and all of **Task 10**.
2. `docs/superpowers/specs/2026-09-08-retro-collection-design.md` sections 1, 2, 3.6, 3.9, 3.10, and the per-game controls in section 4.
3. `README.md` (current, v1 wording), `Makefile` (targets), `src/games/registry.c` (game order and names), `scripts/ares-shot.sh`.

## The task

1. **README** — rewrite `README.md` for the collection as the plan's Task 10 Step 1 specifies: title "Retro Games for Nintendo 64"; what it is (credit the iPod games menu idea, Steve Wozniak's Brick, and libdragon); the six games with one line each; a per-game controls table (columns: Game, Move, Action, Other); menu and pause controls (Start = pause menu with RESUME / QUIT TO MENU; A confirms; up/down navigate); Settings (Music, Sound, Volume; saved to cartridge EEPROM together with every game's high score); Music (original chiptunes from an in-repo synth; `make music` renders WAVs to `build/music/`, play with `afplay build/music/<name>.wav`); Fonts (Inter, SIL OFL 1.1, `assets/LICENSE-Inter.txt`); Requirements; Build (`make image`, `make rom`); Run (`make run`); Develop (`make test`, `make frames GAME=<name>`, `make music`, `make rom-autoplay GAME=<name>`, `make shots`, `make rom DEBUG=1`); Verification note (ares only; mupen64plus cannot boot libdragon ROMs; captures via `scripts/ares-shot.sh`); Toolchain pin (libdragon commit and base image digest from the current README); Trademark note (Blocks is an original implementation of the falling-block genre and is not affiliated with or endorsed by the Tetris trademark holders; that is the only place the word may appear).
2. **Evidence** — capture `games.z64` (the menu) at 4 s, then each game's autoplay ROM at 20 s: for each name in `brick blocks snake pong parachute 2048`, run `rm -rf build/autoplay && make rom-autoplay GAME=<name> && scripts/ares-shot.sh games-autoplay.z64 build/shots/v2-final-<name> 20`. Also `make frames GAME=settings`. Downscale every capture to 768 px wide into `docs/superpowers/plans/evidence/v2-<name>.png` with `sips -Z 768`, and copy `build/frames/settings/frame-0000.png` to `docs/superpowers/plans/evidence/v2-settings.png`.
3. **Final verification** — `make clean && make test && make music && make rom`, then the six autoplay builds above (they are part of the evidence step).
4. Commit.

## Standing rules

- Do not edit the plan, the specs, or any source file; this task changes `README.md` and adds evidence images only. If a build or test fails, stop and report; do not fix source.
- Capture screens only with `scripts/ares-shot.sh`; never run `screencapture` yourself or capture the full display. If a capture fails, note it and continue with the others.
- Each ROM build takes about 40 s and each capture about 25 s; run them as separate commands so no single command exceeds four minutes. Never use mupen64plus. Do not `git push`.

## Verification (all must pass before committing)

```
make clean && make test                                # all binaries 0 failure(s)
make music && make rom                                 # WAVs and games.z64
ls -la docs/superpowers/plans/evidence/v2-*.png        # menu, six games, settings = 8 files
grep -ci tetris README.md                              # exactly 1 (the trademark note)
```

## When done

```
git add README.md docs/superpowers/plans/evidence
git commit -m "v2 Task 10: collection README and evidence"
```

Then print a short report: the README section list, the evidence files with sizes, and the final verification output tail.
