#ifndef G2048_H
#define G2048_H

#include "../game.h"
#include "../prng.h"

enum { G2048_N = 4, G2048_TILE = 40, G2048_GAP = 4, G2048_X0 = 72, G2048_Y0 = 32, G2048_AUTOPLAY_PERIOD = 10 };
typedef enum { G2048_ST_TITLE, G2048_ST_PLAY, G2048_ST_GAMEOVER } g2048_state_t;
typedef struct {
    g2048_state_t state;
    uint16_t board[G2048_N][G2048_N];
    int score, high_score;
    bool reached_2048;
    int8_t prev_dpad_x, prev_dpad_y; bool stick_armed;
    int autoplay_dir;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} g2048_t;
/* Slides one row toward index 0, merging equal neighbours once; returns true if it changed; adds merged values to *gained. */
bool g2048_slide_row(uint16_t row[G2048_N], int *gained);
/* dir: 0 left, 1 right, 2 up, 3 down. Returns true if the board changed. */
bool g2048_move(g2048_t *g, int dir);
void g2048_add_tile(g2048_t *g);                   /* 2 with probability 9/10 else 4, in a random empty cell */
bool g2048_can_move(const g2048_t *g);
uint32_t g2048_tile_color(uint16_t value);
void g2048_init(g2048_t *g); void g2048_start(g2048_t *g, uint32_t seed);
void g2048_update(g2048_t *g, const input_t in[GAME_MAX_PLAYERS]);
void g2048_render(const g2048_t *g, const draw_t *d);
void g2048_autoplay(const g2048_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_2048;

#endif
