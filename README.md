# Retro Games for Nintendo 64

A Nintendo 64 collection in the spirit of the iPod Games menu: six games, original chiptunes, and a Settings screen in one ROM. Brick is the Breakout-style game Steve Wozniak wrote for the iPod. The game cores are platform-independent C; a thin [libdragon](https://github.com/DragonMinded/libdragon) adapter draws them on the N64 and maps the controller. Play it in the [ares](https://ares-emu.net) emulator.

## Games

- **Brick** — paddle and ball; Steve Wozniak's iPod original.
- **Blocks** — falling-block puzzle; clear lines, raise the level.
- **Snake** — eat food, grow, don't hit a wall or yourself.
- **Pong** — one or two players; first to 11.
- **Parachute** — shoot helicopters and paratroopers before they pile up; the iPod original.
- **2048** — slide tiles, merge equals, reach 2048.

## Controls

| Game       | Move                                                      | Action                                  | Other                                              |
|------------|-----------------------------------------------------------|-----------------------------------------|----------------------------------------------------|
| Brick      | Analog stick or D-pad / C-left / C-right move the paddle  | A launches the ball and confirms        |                                                    |
| Blocks     | D-pad or stick left/right; down soft-drops; up hard-drops | A rotates clockwise; B counter-clockwise |                                                    |
| Snake      | D-pad or stick sets direction                             | A starts                                | Reverse into the body is ignored                   |
| Pong       | Stick or D-pad moves the paddle vertically                | A confirms mode and restart             | Port 2 is player 2; title chooses 1 PLAYER / 2 PLAYERS |
| Parachute  | Stick or D-pad aims the turret                            | A fires; hold A to auto-fire            | Each shot costs 1 point                            |
| 2048       | D-pad or stick slides tiles                               | A starts                                |                                                    |

Start opens the pause menu with RESUME and QUIT TO MENU. A confirms; up/down navigate. C-left / C-right / C-up / C-down alias the D-pad.

## Settings

Music, Sound, and Volume. Changes apply immediately and are saved to cartridge EEPROM together with every game's high score.

## Music

Original chiptunes from an in-repo synth. `make music` renders WAVs to `build/music/`. Play with `afplay build/music/<name>.wav`.

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

## Develop

    make test                       # host unit tests (clang, no N64 toolchain needed)
    make frames GAME=<name>         # dumps autoplay frames to build/frames/<name>/*.png
    make music                      # renders tunes and effects to build/music/*.wav
    make rom-autoplay GAME=<name>   # builds games-autoplay.z64, which plays itself
    make shots                      # captures games-autoplay.z64 in ares at 4, 8, and 15 seconds
    make rom DEBUG=1                # ROM with libdragon RDP validation; assertions appear on screen

## Verification note

mupen64plus cannot boot libdragon ROMs, so ares is the only emulator used. Screenshots are captured by `scripts/ares-shot.sh`.

## Toolchain pin

libdragon commit `c4a7e119eff1cfad07adcfa892a2910c40d8bdb8`. Base image digest `ghcr.io/dragonminded/libdragon@sha256:232540c7145989cc536532ec1e8df8458aa72e98935adf154b8b259de878892c`.

## Trademark note

Blocks is an original implementation of the falling-block genre and is not affiliated with or endorsed by the Tetris trademark holders.
