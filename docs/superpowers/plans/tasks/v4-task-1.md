# v4 Task 1: Software-render build and HLE verification rig

You are implementing **Task 1 of the v4 plan** for a Nintendo 64 games collection built with libdragon. The collection (nine games, tagged v3.0) renders with rdpq, which HLE emulators like Delta cannot run. This task adds a CPU-rendered `games-soft.z64` that works on Delta, plus a from-source mupen64plus + GLideN64 rig to verify it. You have no other context: read the files below first.

## Read first (in this order)

1. `docs/superpowers/plans/2026-09-15-soft-render.md` — header, "Global Constraints", "Verified facts", and all of **Task 1**. Use its code and script bodies verbatim, adjusting only to match the current Makefile's actual DFS rule names.
2. `docs/superpowers/specs/2026-09-15-soft-render-design.md` for intent.
3. `src/n64/app.c` (the adapter you extend with `#ifdef SOFTRENDER` branches), `Makefile` (targets and the `ifdef N64_INST` block with the current DFS/font rules), `scripts/ares-shot.sh` (the pattern the new capture script follows).

## The task

Do Task 1 steps 1 to 7: add the software `soft_rect`/`soft_text` backend and the `SOFTRENDER` boot/frame branches to `src/n64/app.c` (keep the rdpq path exactly as-is, additive only); add the `rom-soft`, `rom-soft-autoplay`, and `hle-shots` Makefile targets and the in-container `SOFTRENDER` branch that skips the DFS; create `scripts/build-hle-rig.sh` and `scripts/hle-shot.sh` and `chmod +x` them; verify; commit.

## Standing rules

- Do not edit the plan, the specs, any game module, the synth, the app state, or the rdpq draw path. Changes are limited to `src/n64/app.c` (additive `#ifdef SOFTRENDER` only), `Makefile`, and the two new scripts.
- The rdpq `games.z64` build and all game behavior must be unchanged; `make test` must still pass untouched.
- Capture emulator windows only with the provided scripts; never run `screencapture` directly or capture the full display. If a capture fails, note it, ensure the emulator is closed, and continue.
- The rig build (`scripts/build-hle-rig.sh`) clones and compiles mupen64plus-core and GLideN64; it may take a few minutes the first time. Run it as its own command. If `cmake` or `pkg-config` is missing, `brew install cmake pkgconf`.
- Every other command under four minutes. Never use the HLE stack for the rdpq build. Do not `git push`.
- If a software capture comes out black, stop and report: that means the CPU framebuffer is not displaying and the approach needs rethinking (do not ship a black ROM).

## Verification (all must pass before committing)

```
make test                                    # unchanged, all 0 failure(s)
make rom && make rom-soft                     # games.z64 and games-soft.z64 both build, no warnings
scripts/build-hle-rig.sh                       # rig built (core + GLideN64 dylibs present)
make rom-soft-autoplay GAME=brick
scripts/hle-shot.sh games-soft.z64 build/hle-shots/task1-menu 4
scripts/hle-shot.sh games-soft-autoplay.z64 build/hle-shots/task1-brick 6
ls -la games-soft.z64 build/hle-shots/task1-menu build/hle-shots/task1-brick
```

If you can view images: the menu capture must show "GAMES" and the game list in the built-in font on the beige field; the Brick capture must show the colored brick grid, paddle, ball, and HUD. Both must be clearly non-black. Report what you see.

## When done

```
git add src/n64/app.c Makefile scripts/build-hle-rig.sh scripts/hle-shot.sh
git commit -m "v4 Task 1: software-render build (games-soft.z64) and HLE verification rig"
```

Then report: build output tails, what the two captures show, whether the rdpq build diff is additive-only, and any deviation.
