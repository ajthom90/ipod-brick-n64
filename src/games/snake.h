#ifndef SNAKE_H
#define SNAKE_H

#include "../game.h"
#include "../prng.h"

enum { SNK_COLS = 36, SNK_ROWS = 24, SNK_CELL = 8, SNK_X0 = 16, SNK_Y0 = 32, SNK_MAX = SNK_COLS * SNK_ROWS,
       SNK_PERIOD_START = 8, SNK_PERIOD_MIN = 3, SNK_FOODS_PER_SPEEDUP = 5 };
typedef enum { SNK_ST_TITLE, SNK_ST_PLAY, SNK_ST_GAMEOVER } snk_state_t;
typedef struct { int8_t x, y; } snk_cell_t;
typedef struct {
    snk_state_t state;
    snk_cell_t body[SNK_MAX]; int len; int head;      /* ring buffer: head index, body grows from the tail */
    int8_t dir_x, dir_y, pending_x, pending_y;
    snk_cell_t food;
    int period, step_ticks, score, high_score, foods;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} snake_t;
bool snake_occupied(const snake_t *g, int x, int y);
void snake_place_food(snake_t *g);
void snake_init(snake_t *g); void snake_start(snake_t *g, uint32_t seed);
void snake_update(snake_t *g, const input_t in[GAME_MAX_PLAYERS]);
void snake_render(const snake_t *g, const draw_t *d);
void snake_autoplay(const snake_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_SNAKE;

#endif
