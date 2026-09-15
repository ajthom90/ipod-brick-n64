# v4 Task 2: Software-build evidence and Delta/iOS README

You are implementing **Task 2 of the v4 plan**, the last task, for a Nintendo 64 games collection. Task 1 added a CPU-rendered `games-soft.z64` and an HLE verification rig (mupen64plus + GLideN64 built from source, Delta's stack). This task captures evidence and documents Delta/iOS support. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-15-soft-render.md` — header and all of **Task 2**.
2. `docs/superpowers/specs/2026-09-15-soft-render-design.md` sections 1 and 3.4 (the Delta caveat).
3. `README.md`, `src/games/registry.c` (game order/names), `scripts/hle-shot.sh`, `scripts/build-hle-rig.sh`.

## The task

1. **Evidence** — ensure the rig exists (`scripts/build-hle-rig.sh`; it is idempotent and already built, so this is a no-op). Capture `games-soft.z64` (the menu) at 4 s, then each game's software autoplay at 15 s: for each name in `brick blocks snake pong parachute 2048 hopper flap runner`, run `rm -rf build/soft-auto && make rom-soft-autoplay GAME=<name>` then `scripts/hle-shot.sh games-soft-autoplay.z64 build/hle-shots/v4-<name> 15`. Run each game as its own command (each is well under four minutes). Downscale every capture to 768 px wide into `docs/superpowers/plans/evidence/v4-<name>.png` (menu as `v4-menu.png`) with `sips -Z 768`.
2. **README** — add a "Delta and iOS" section: `games-soft.z64` (built with `make rom-soft`) is the build for Delta and other high-level-emulation emulators; it renders on the CPU because HLE emulators cannot run libdragon's RSP microcode; it relies on GLideN64's framebuffer emulation, which is Delta's default, so no Delta settings change is needed; text uses libdragon's built-in font, so it looks more retro than the Inter-based `games.z64`; the rdpq `games.z64` stays the build for ares and real hardware; and note that Delta 1.7.6 crashed on a macOS 27 beta before loading any ROM (a Delta + OS-beta bug, unrelated to the ROM), so prefer a non-beta iPad or iPhone. Also add `make rom-soft`, `make rom-soft-autoplay GAME=<name>`, and `make hle-shots GAME=<name>` to the Develop section with one-line descriptions, and mention the two HLE rig scripts.
3. **Final verification** — `make clean && make test && make rom && make rom-soft`.
4. Commit.

## Standing rules

- Do not edit the plan, the specs, or any source file; this task changes `README.md` and adds evidence images only. If a build or test fails, stop and report.
- Capture emulator windows only with `scripts/hle-shot.sh`; never run `screencapture` directly or capture the full display. If a capture fails, note it and continue with the rest; a black capture must be reported, not shipped as evidence.
- Run builds and captures as separate commands. Never use the HLE stack for the rdpq build. Do not `git push`.

## Verification (all must pass before committing)

```
make clean && make test                       # all 0 failure(s)
make rom && make rom-soft                       # both ROMs build
ls -la docs/superpowers/plans/evidence/v4-*.png # menu + nine games = 10 files
grep -ci delta README.md                        # at least 1
```

## When done

```
git add README.md docs/superpowers/plans/evidence
git commit -m "v4 Task 2: Delta/iOS README and software-build evidence"
```

Then report: the README section added, the evidence files with sizes, what a couple of the game captures show, and the final verification tail.
