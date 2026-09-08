#include "music.h"

/* MENU: 100 BPM, C major, calm. Chords C | C | Am | Am | F | F | G | G. */
static const char *const MENU_P1 =
    "duty=50 gain=55 decay=6 sustain=50 "
    "E5:8 G5:4 E5:4 | D5:8 C5:8 | C5:8 E5:4 A4:4 | B4:8 A4:8 | "
    "A4:8 C5:4 F5:4 | E5:8 D5:8 | D5:8 G5:4 D5:4 | E5:16";
static const char *const MENU_P2 =
    "duty=25 gain=40 "
    "C4:2 E4:2 G4:2 C5:2 E5:2 C5:2 G4:2 E4:2 | C4:2 E4:2 G4:2 C5:2 E5:2 C5:2 G4:2 E4:2 | "
    "A3:2 C4:2 E4:2 A4:2 C5:2 A4:2 E4:2 C4:2 | A3:2 C4:2 E4:2 A4:2 C5:2 A4:2 E4:2 C4:2 | "
    "F3:2 A3:2 C4:2 F4:2 A4:2 F4:2 C4:2 A3:2 | F3:2 A3:2 C4:2 F4:2 A4:2 F4:2 C4:2 A3:2 | "
    "G3:2 B3:2 D4:2 G4:2 B4:2 G4:2 D4:2 B3:2 | G3:2 B3:2 D4:2 G4:2 B4:2 G4:2 D4:2 B3:2";
static const char *const MENU_TRI =
    "gain=70 "
    "C2:8 C2:8 | C2:8 G2:8 | A2:8 A2:8 | A2:8 E2:8 | F2:8 F2:8 | F2:8 C3:8 | G2:8 G2:8 | G2:8 D2:8";
static const char *const MENU_NOISE =
    "gain=25 "
    "-:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | "
    "-:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2 | -:2 H:2 -:2 H:2 -:2 H:2 -:2 H:2";

/* BRICK: 130 BPM, A minor pentatonic riff, bouncy. Chords Am | Am | G | G | Am | Am | F | G. */
static const char *const BRICK_P1 =
    "duty=50 gain=65 decay=2 sustain=60 "
    "A4:2 C5:2 D5:2 E5:2 D5:2 C5:2 A4:2 G4:2 | A4:2 -:2 A4:2 C5:2 E5:4 D5:2 C5:2 | "
    "A4:2 C5:2 D5:2 E5:2 D5:2 C5:2 A4:2 G4:2 | G4:2 A4:2 C5:2 A4:2 G4:4 E4:4 | "
    "E5:2 G5:2 A5:2 G5:2 E5:2 D5:2 C5:2 D5:2 | E5:4 -:2 E5:2 D5:2 C5:2 A4:4 | "
    "A4:2 C5:2 D5:2 E5:2 D5:2 C5:2 A4:2 G4:2 | A4:4 -:4 A4:2 G4:2 A4:4";
static const char *const BRICK_P2 =
    "duty=25 gain=40 decay=1 sustain=40 "
    "-:2 E4:2 -:2 E4:2 -:2 E4:2 -:2 E4:2 | -:2 E4:2 -:2 E4:2 -:2 E4:2 -:2 E4:2 | "
    "-:2 D4:2 -:2 D4:2 -:2 D4:2 -:2 D4:2 | -:2 D4:2 -:2 D4:2 -:2 D4:2 -:2 D4:2 | "
    "-:2 E4:2 -:2 E4:2 -:2 E4:2 -:2 E4:2 | -:2 E4:2 -:2 E4:2 -:2 E4:2 -:2 E4:2 | "
    "-:2 C4:2 -:2 C4:2 -:2 C4:2 -:2 C4:2 | -:2 D4:2 -:2 D4:2 -:2 D4:2 -:2 D4:2";
static const char *const BRICK_TRI =
    "gain=75 "
    "A2:2 C3:2 E3:2 A3:2 G3:2 E3:2 C3:2 A2:2 | A2:2 C3:2 E3:2 A3:2 G3:2 E3:2 C3:2 A2:2 | "
    "G2:2 B2:2 D3:2 G3:2 F3:2 D3:2 B2:2 G2:2 | G2:2 B2:2 D3:2 G3:2 F3:2 D3:2 B2:2 G2:2 | "
    "A2:2 C3:2 E3:2 A3:2 G3:2 E3:2 C3:2 A2:2 | A2:2 C3:2 E3:2 A3:2 G3:2 E3:2 C3:2 A2:2 | "
    "F2:2 A2:2 C3:2 F3:2 E3:2 C3:2 A2:2 F2:2 | G2:2 B2:2 D3:2 G3:2 F3:2 D3:2 B2:2 G2:2";
static const char *const BRICK_NOISE =
    "gain=40 "
    "K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | "
    "K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 S:2 S:2 S:2 S:2";

/* BLOCKS: 140 BPM, E minor with a raised leading tone, driving. Chords Em | Am | B | Am | Dm | C | G | B. */
static const char *const BLOCKS_P1 =
    "duty=50 gain=65 decay=3 sustain=55 "
    "E5:4 B4:2 C5:2 D5:4 C5:2 B4:2 | A4:4 A4:2 C5:2 E5:4 D5:2 C5:2 | B4:6 C5:2 D5:4 E5:4 | C5:4 A4:4 A4:8 | "
    "-:2 D5:4 F5:2 A5:4 G5:2 F5:2 | E5:6 C5:2 E5:4 D5:2 C5:2 | B4:4 D5:4 C5:4 A4:4 | D#5:4 B4:4 E5:8";
static const char *const BLOCKS_P2 =
    "duty=25 gain=38 decay=1 sustain=50 "
    "E4:2 B4:2 E4:2 B4:2 E4:2 B4:2 E4:2 B4:2 | A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 | "
    "B3:2 F#4:2 B3:2 F#4:2 B3:2 F#4:2 B3:2 F#4:2 | A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 A3:2 E4:2 | "
    "D4:2 A4:2 D4:2 A4:2 D4:2 A4:2 D4:2 A4:2 | C4:2 G4:2 C4:2 G4:2 C4:2 G4:2 C4:2 G4:2 | "
    "G3:2 D4:2 G3:2 D4:2 G3:2 D4:2 G3:2 D4:2 | B3:2 F#4:2 B3:2 F#4:2 B3:2 F#4:2 B3:2 F#4:2";
static const char *const BLOCKS_TRI =
    "gain=80 "
    "E2:2 E2:2 E3:2 E2:2 E2:2 E2:2 E3:2 E2:2 | A2:2 A2:2 A3:2 A2:2 A2:2 A2:2 A3:2 A2:2 | "
    "B2:2 B2:2 B3:2 B2:2 B2:2 B2:2 B3:2 B2:2 | A2:2 A2:2 A3:2 A2:2 A2:2 A2:2 A3:2 A2:2 | "
    "D2:2 D2:2 D3:2 D2:2 D2:2 D2:2 D3:2 D2:2 | C2:2 C2:2 C3:2 C2:2 C2:2 C2:2 C3:2 C2:2 | "
    "G2:2 G2:2 G3:2 G2:2 G2:2 G2:2 G3:2 G2:2 | B2:2 B2:2 B3:2 B2:2 B2:2 B2:2 B3:2 B2:2";
static const char *const BLOCKS_NOISE =
    "gain=45 "
    "K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 S:2 S:2 S:2 S:2 | "
    "K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 K:2 H:2 K:2 H:2 | K:2 H:2 K:2 H:2 S:2 S:2 S:2 S:2";

/* SNAKE: 120 BPM, F major, staccato and playful. Chords F | C | F | C | Bb | F | C | F. */
static const char *const SNAKE_P1 =
    "duty=50 gain=60 decay=1 sustain=30 "
    "F4:1 -:1 A4:1 -:1 C5:2 -:2 A4:1 -:1 F4:1 -:1 G4:2 -:2 | A4:1 -:1 A4:1 -:1 G4:2 -:2 F4:2 -:2 E4:2 -:2 | "
    "F4:1 -:1 A4:1 -:1 C5:2 -:2 D5:1 -:1 C5:1 -:1 A4:2 -:2 | G4:2 -:2 E4:2 -:2 F4:4 -:4 | "
    "A#4:1 -:1 A#4:1 -:1 A4:2 -:2 G4:1 -:1 A4:1 -:1 A#4:2 -:2 | C5:1 -:1 C5:1 -:1 A4:2 -:2 F4:2 -:2 G4:2 -:2 | "
    "A4:1 -:1 C5:1 -:1 D5:2 -:2 C5:1 -:1 A4:1 -:1 G4:2 -:2 | F4:2 -:2 C4:2 -:2 F4:4 -:4";
static const char *const SNAKE_P2 =
    "duty=12 gain=32 "
    "A3:8 C4:8 | E4:8 G4:8 | A3:8 C4:8 | E4:8 G4:8 | D4:8 F4:8 | A3:8 C4:8 | E4:8 G4:8 | A3:8 C4:8";
static const char *const SNAKE_TRI =
    "gain=75 "
    "F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 | C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 | "
    "F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 | C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 | "
    "A#2:2 A#3:2 A#2:2 A#3:2 A#2:2 A#3:2 A#2:2 A#3:2 | F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 | "
    "C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 C2:2 C3:2 | F2:2 F3:2 F2:2 F3:2 F2:2 F3:2 F2:2 F3:2";
static const char *const SNAKE_NOISE =
    "gain=35 "
    "K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | "
    "K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2 | K:2 H:2 S:2 H:2 K:2 H:2 S:2 H:2";

/* PONG: 110 BPM, sparse call and answer over a pedal tone. */
static const char *const PONG_P1 =
    "duty=50 gain=55 decay=2 sustain=20 "
    "E5:2 -:2 -:4 E5:2 -:2 -:4 | E5:2 -:2 -:4 E5:2 -:2 -:4 | D5:2 -:2 -:4 D5:2 -:2 -:4 | D5:2 -:2 -:4 D5:2 -:2 -:4 | "
    "C5:2 -:2 -:4 C5:2 -:2 -:4 | C5:2 -:2 -:4 C5:2 -:2 -:4 | D5:2 -:2 -:4 D5:2 -:2 -:4 | E5:2 -:2 G5:2 -:2 E5:2 -:2 -:4";
static const char *const PONG_P2 =
    "duty=25 gain=45 decay=2 sustain=20 "
    "-:4 B4:2 -:2 -:4 B4:2 -:2 | -:4 B4:2 -:2 -:4 B4:2 -:2 | -:4 A4:2 -:2 -:4 A4:2 -:2 | -:4 A4:2 -:2 -:4 A4:2 -:2 | "
    "-:4 G4:2 -:2 -:4 G4:2 -:2 | -:4 G4:2 -:2 -:4 G4:2 -:2 | -:4 A4:2 -:2 -:4 A4:2 -:2 | -:4 B4:2 -:2 -:4 B4:2 -:2";
static const char *const PONG_TRI =
    "gain=60 "
    "E2:16 | E2:16 | D2:16 | D2:16 | C2:16 | C2:16 | D2:16 | E2:16";
static const char *const PONG_NOISE =
    "gain=30 "
    "-:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2 | "
    "-:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2 | -:4 S:2 -:2 -:4 S:2 -:2";

/* PARACHUTE: 150 BPM, tense; chromatic bass ostinato, siren fifths. */
static const char *const PARA_P1 =
    "duty=50 gain=55 decay=0 "
    "A4:4 E5:4 A4:4 E5:4 | A4:4 E5:4 A4:4 E5:4 | A#4:4 F5:4 A#4:4 F5:4 | A4:4 E5:4 A4:4 E5:4 | "
    "A4:4 E5:4 A4:4 E5:4 | A#4:4 F5:4 A#4:4 F5:4 | A4:2 E5:2 A4:2 E5:2 A4:2 E5:2 A4:2 E5:2 | A4:2 E5:2 A4:2 E5:2 A#4:2 F5:2 A#4:2 F5:2";
static const char *const PARA_P2 =
    "duty=12 gain=40 decay=1 sustain=40 "
    "-:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 E4:2 | -:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 E4:2 | -:2 F4:2 -:2 F4:2 -:2 F#4:2 -:2 F4:2 | -:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 E4:2 | "
    "-:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 E4:2 | -:2 F4:2 -:2 F4:2 -:2 F#4:2 -:2 F4:2 | -:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 E4:2 | -:2 E4:2 -:2 E4:2 -:2 F4:2 -:2 F#4:2";
static const char *const PARA_TRI =
    "gain=80 "
    "A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | "
    "A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | "
    "A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | "
    "A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2 | A2:2 A#2:2 B2:2 C3:2 B2:2 A#2:2 A2:2 G#2:2";
static const char *const PARA_NOISE =
    "gain=45 "
    "K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | "
    "K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 K:2 H:2 K:2 S:2 | K:2 H:2 K:2 S:2 S:2 S:2 S:2 S:2";

/* 2048: 90 BPM, chill; Dm7 | Dm7 | G7 | G7 | Cmaj7 | Cmaj7 | Am7 | Am7. */
static const char *const G2048_P1 =
    "duty=50 gain=50 decay=8 sustain=45 "
    "A5:8 F5:8 | E5:16 | D5:8 B4:8 | G4:16 | E5:8 G5:8 | B5:16 | C6:8 A5:8 | E5:16";
static const char *const G2048_P2 =
    "duty=25 gain=42 "
    "D4:2 F4:2 A4:2 C5:2 F5:2 C5:2 A4:2 F4:2 | D4:2 F4:2 A4:2 C5:2 F5:2 C5:2 A4:2 F4:2 | "
    "G3:2 B3:2 D4:2 F4:2 B4:2 F4:2 D4:2 B3:2 | G3:2 B3:2 D4:2 F4:2 B4:2 F4:2 D4:2 B3:2 | "
    "C4:2 E4:2 G4:2 B4:2 E5:2 B4:2 G4:2 E4:2 | C4:2 E4:2 G4:2 B4:2 E5:2 B4:2 G4:2 E4:2 | "
    "A3:2 C4:2 E4:2 G4:2 C5:2 G4:2 E4:2 C4:2 | A3:2 C4:2 E4:2 G4:2 C5:2 G4:2 E4:2 C4:2";
static const char *const G2048_TRI =
    "gain=65 "
    "D2:16 | D2:16 | G2:16 | G2:16 | C2:16 | C2:16 | A2:16 | A2:16";
static const char *const G2048_NOISE =
    "gain=22 "
    "-:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 H:2 -:2 | "
    "-:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 H:2 -:2 | -:4 H:2 -:2 -:4 O:2 -:2";

const music_src_t MUSIC_SRC[MUSIC_TRACK_COUNT] = {
    [MUSIC_MENU]      = { "menu",      100, { MENU_P1,   MENU_P2,   MENU_TRI,   MENU_NOISE } },
    [MUSIC_BRICK]     = { "brick",     130, { BRICK_P1,  BRICK_P2,  BRICK_TRI,  BRICK_NOISE } },
    [MUSIC_BLOCKS]    = { "blocks",    140, { BLOCKS_P1, BLOCKS_P2, BLOCKS_TRI, BLOCKS_NOISE } },
    [MUSIC_SNAKE]     = { "snake",     120, { SNAKE_P1,  SNAKE_P2,  SNAKE_TRI,  SNAKE_NOISE } },
    [MUSIC_PONG]      = { "pong",      110, { PONG_P1,   PONG_P2,   PONG_TRI,   PONG_NOISE } },
    [MUSIC_PARACHUTE] = { "parachute", 150, { PARA_P1,   PARA_P2,   PARA_TRI,   PARA_NOISE } },
    [MUSIC_2048]      = { "2048",      90,  { G2048_P1,  G2048_P2,  G2048_TRI,  G2048_NOISE } },
};
