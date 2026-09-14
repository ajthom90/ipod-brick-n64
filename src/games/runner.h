#ifndef RUNNER_H
#define RUNNER_H

#include "../game.h"
#include "../prng.h"

enum { RUN_PLAYER_W = 10, RUN_PLAYER_H = 14, RUN_PLAYER_X = 60, RUN_GRAVITY = 77,
       RUN_JUMP_VY = -1536, RUN_JUMP_CUT_VY = -512, RUN_SPEED_START = 768, RUN_SPEED_STEP = 26,
       RUN_SPEED_STEP_TICKS = 300, RUN_SPEED_MAX = 1792, RUN_MAX_BUILDINGS = 8,
       RUN_W_MIN = 60, RUN_W_RAND = 101, RUN_ROOF_MIN = 120, RUN_ROOF_MAX = 200, RUN_ROOF_DELTA = 40,
       RUN_GAP_MIN = 24, RUN_GAP_RAND = 41, RUN_METRE = 10, RUN_AUTOPLAY_STOP = 500, RUN_AUTOPLAY_HOLD = 12 };
typedef enum { RUN_ST_TITLE, RUN_ST_PLAY, RUN_ST_GAMEOVER } run_state_t;
typedef struct { bool alive; int32_t x; int w, roof; } run_building_t;   /* x Q8.8 left edge */
typedef struct {
    run_state_t state;
    int32_t py, vy;                   /* player top Q8.8, velocity */
    bool grounded;
    run_building_t buildings[RUN_MAX_BUILDINGS];
    int32_t speed, distance;          /* Q8.8 px per tick, Q8.8 px */
    int speed_ticks, score, high_score, last_point_score, autoplay_hold;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} runner_t;
void runner_init(runner_t *g); void runner_start(runner_t *g, uint32_t seed);
void runner_update(runner_t *g, const input_t in[GAME_MAX_PLAYERS]);
void runner_render(const runner_t *g, const draw_t *d);
void runner_autoplay(const runner_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_RUNNER;

#endif
