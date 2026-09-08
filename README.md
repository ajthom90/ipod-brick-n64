# Brick for Nintendo 64

A Nintendo 64 port of Brick, the Breakout-style game Steve Wozniak wrote for the iPod. The game core is platform-independent C; a thin [libdragon](https://github.com/DragonMinded/libdragon) adapter draws it on the N64 and maps the controller. Play it in the [ares](https://ares-emu.net) emulator.

## Fonts

HUD and title text use [Inter](https://rsms.me/inter/) Bold, licensed under the SIL Open Font License 1.1. The full license is in `assets/LICENSE-Inter.txt`.

## Requirements

- [Docker Desktop](https://www.docker.com/products/docker-desktop/)
- ares: `brew install --cask ares-emulator`
- clang from Xcode command line tools (`xcode-select --install`)

## Build

    make image        # one-time: build the toolchain container (about a minute)
    make rom          # produces games.z64

## Run

    make run          # opens games.z64 in ares

Or open `games.z64` in ares yourself.

## Controls

Analog stick or D-pad / C-left / C-right move the paddle. A launches the ball and confirms. Start pauses.

## Develop

    make test          # host unit tests for the game core (clang, no N64 toolchain needed)
    make frames        # dumps autoplay frames to build/frames/*.png for visual checks
    make rom-autoplay  # builds brick-autoplay.z64, which plays itself for unattended checks
    make shots         # captures brick-autoplay.z64 in ares at 4, 8, and 15 seconds into build/shots/
    make rom DEBUG=1   # ROM with libdragon RDP validation (`rdpq_debug_start`); assertions appear on screen

## Layout constants

Screen size, playfield, brick grid, paddle, ball, speeds, and colors live in `src/brick.h`.

## Verification note

mupen64plus cannot boot libdragon ROMs, so ares is the only emulator used. Screenshots are captured by `scripts/ares-shot.sh`.

## Toolchain pin

libdragon commit `c4a7e119eff1cfad07adcfa892a2910c40d8bdb8`. Base image digest `ghcr.io/dragonminded/libdragon@sha256:232540c7145989cc536532ec1e8df8458aa72e98935adf154b8b259de878892c`.
