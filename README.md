# Brick for Nintendo 64

The classic iPod "Brick" game (Apple's Breakout clone) rebuilt for the Nintendo 64 with [libdragon](https://github.com/DragonMinded/libdragon). Runs in the [ares](https://ares-emu.net) emulator.
Text is set in [Inter](https://rsms.me/inter/) (SIL Open Font License 1.1, see assets/LICENSE-Inter.txt).

## Build

    make image        # one-time: build the toolchain container (about a minute)
    make rom          # produces brick.z64
    make run          # opens brick.z64 in ares

## Controls

Analog stick or D-pad / C-left / C-right move the paddle. A launches the ball and confirms. Start pauses.

## Develop

    make test         # host unit tests for the game core (clang, no N64 toolchain needed)
    make frames       # dumps autoplay frames to build/frames/*.png for visual checks
    make rom-autoplay  # builds brick-autoplay.z64, which plays itself for unattended checks
    make shots        # captures brick-autoplay.z64 in ares at 4, 8, and 15 seconds into build/shots/
