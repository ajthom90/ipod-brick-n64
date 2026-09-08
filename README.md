# Brick for Nintendo 64

The classic iPod "Brick" game (Apple's Breakout clone) rebuilt for the Nintendo 64 with [libdragon](https://github.com/DragonMinded/libdragon). Runs in the [ares](https://ares-emu.net) emulator.

## Build

    make image        # one-time: build the toolchain container (about a minute)
    make rom          # produces brick.z64
    make run          # opens brick.z64 in ares

## Develop

    make test         # host unit tests for the game core (clang, no N64 toolchain needed)
    make frames       # dumps autoplay frames to build/frames/*.png for visual checks
