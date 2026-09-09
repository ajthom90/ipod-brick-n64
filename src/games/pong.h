#ifndef PONG_H
#define PONG_H

#include "../game.h"
#include "../prng.h"

enum { PNG_PADDLE_W = 6, PNG_PADDLE_H = 40, PNG_P1_X = 20, PNG_P2_X = 294, PNG_BALL = 6,
       PNG_SPEED_SERVE = 768, PNG_SPEED_STEP = 64, PNG_SPEED_MAX = 2048, PNG_WIN_SCORE = 11,
       PNG_PADDLE_DIGITAL = 4, PNG_PADDLE_ANALOG_MAX = 6, PNG_AI_SPEED = 3, PNG_SERVE_DELAY = 60 };
typedef enum { PNG_ST_TITLE, PNG_ST_SERVE, PNG_ST_PLAY, PNG_ST_GAMEOVER } png_state_t;
typedef struct {
    png_state_t state;
    bool two_players; int title_row;          /* 0 = 1 PLAYER, 1 = 2 PLAYERS */
    int p1_y, p2_y;                            /* paddle top edges */
    int32_t ball_x, ball_y, ball_vx, ball_vy, ball_speed;  /* Q8.8 */
    int score1, score2, high_score;           /* high score = best margin of victory for player 1 */
    int serve_to;                              /* 1 or 2: who receives the next serve */
    int serve_ticks;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} pong_t;
void pong_ai(const pong_t *g, int paddle_y, int32_t toward_x, int *dy);   /* shared AI: dy in -PNG_AI_SPEED..+PNG_AI_SPEED */
void pong_init(pong_t *g); void pong_start(pong_t *g, uint32_t seed);
void pong_update(pong_t *g, const input_t in[GAME_MAX_PLAYERS]);
void pong_render(const pong_t *g, const draw_t *d);
void pong_autoplay(const pong_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_PONG;

#endif
