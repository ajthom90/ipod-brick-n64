#ifndef GAME_H
#define GAME_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "sfx.h"

enum { SCREEN_W = 320, SCREEN_H = 240, PLAY_X0 = 16, PLAY_Y0 = 24, PLAY_X1 = 304, PLAY_Y1 = 228,
       HUD_BASELINE = 26, GAME_MAX_PLAYERS = 2 };

#define COLOR_RED     0xC4472Au
#define COLOR_ORANGE  0xE07A1Fu
#define COLOR_YELLOW  0xD4B01Cu
#define COLOR_GREEN   0x3FA34Du
#define COLOR_BLUE    0x2E6DB4u
#define COLOR_PURPLE  0x7B4EA3u
#define COLOR_TEAL    0x3AAFA9u
#define COLOR_BG      0xE8E4DCu
#define COLOR_DARK    0x2C2C2Cu
#define COLOR_BALL    0x1A1A1Au
#define COLOR_TILE    0xEDE4D6u
#define DRAW_TEXT_DARK  COLOR_DARK
#define DRAW_TEXT_LIGHT COLOR_BG

typedef struct {
    int16_t stick_x, stick_y;   /* -256..+256, up is +y */
    int8_t  dpad_x, dpad_y;     /* -1, 0, +1 (D-pad, C buttons alias), up is +1 */
    bool    a, b, z;            /* pressed this tick */
    bool    a_held, b_held;
} input_t;

typedef enum { DRAW_FONT_HUD = 1, DRAW_FONT_BIG = 2 } draw_font_t;
typedef enum { DRAW_LEFT, DRAW_CENTER, DRAW_RIGHT } draw_align_t;

typedef struct draw_s {
    void *ctx;
    void (*rect)(void *ctx, int x0, int y0, int x1, int y1, uint32_t rgb);
    void (*text)(void *ctx, draw_font_t font, draw_align_t align, int x, int y, uint32_t rgb, const char *utf8);
} draw_t;

static inline void draw_rect(const draw_t *d, int x0, int y0, int x1, int y1, uint32_t rgb) { d->rect(d->ctx, x0, y0, x1, y1, rgb); }
static inline void draw_text(const draw_t *d, draw_font_t f, draw_align_t a, int x, int y, uint32_t rgb, const char *s) { d->text(d->ctx, f, a, x, y, rgb, s); }

/* Tiny integer formatters for HUD strings (no stdio): "PREFIX n" and plain "n". */
void fmt_label(char *buf, size_t cap, const char *prefix, int value);
void fmt_int(char *buf, size_t cap, int value);

typedef enum { MUSIC_MENU = 0, MUSIC_BRICK, MUSIC_BLOCKS, MUSIC_SNAKE, MUSIC_PONG, MUSIC_PARACHUTE, MUSIC_2048, MUSIC_TRACK_COUNT } music_track_id_t;

typedef struct {
    const char *name;
    int players;
    void *state;
    void (*init)(void *st);
    void (*start)(void *st, uint32_t seed);
    void (*update)(void *st, const input_t in[GAME_MAX_PLAYERS]);
    void (*render)(const void *st, const draw_t *d);
    void (*autoplay)(const void *st, input_t in[GAME_MAX_PLAYERS]);
    bool (*is_over)(const void *st);
    int  (*get_high_score)(const void *st);
    void (*set_high_score)(void *st, int value);
    sfx_queue_t *(*sfx)(void *st);
    music_track_id_t track;
} game_desc_t;

#endif
