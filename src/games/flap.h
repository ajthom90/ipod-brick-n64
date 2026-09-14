#ifndef FLAP_H
#define FLAP_H

#include "../game.h"
#include "../prng.h"

enum { FLP_BIRD = 10, FLP_BIRD_X = 80, FLP_GRAVITY = 77, FLP_FLAP_VY = -1280, FLP_TERMINAL = 1536,
       FLP_PIPE_W = 24, FLP_GAP = 64, FLP_GAP_MIN = 48, FLP_PIPE_SPACING = 90, FLP_SCROLL = 512,
       FLP_GROUND_Y = 220, FLP_MAX_PIPES = 6, FLP_CENTER_MIN = 70, FLP_CENTER_RAND = 111,
       FLP_AUTOPLAY_STOP = 40 };
typedef enum { FLP_ST_TITLE, FLP_ST_PLAY, FLP_ST_GAMEOVER } flp_state_t;
typedef struct { bool alive, scored; int32_t x; int center, gap; } flp_pipe_t;   /* x Q8.8 left edge */
typedef struct {
    flp_state_t state;
    int32_t by, vy;                  /* bird top Q8.8, velocity */
    flp_pipe_t pipes[FLP_MAX_PIPES];
    int score, high_score;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} flap_t;
int flap_gap_for_score(int score);
void flap_init(flap_t *g); void flap_start(flap_t *g, uint32_t seed);
void flap_update(flap_t *g, const input_t in[GAME_MAX_PLAYERS]);
void flap_render(const flap_t *g, const draw_t *d);
void flap_autoplay(const flap_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_FLAP;

#endif
