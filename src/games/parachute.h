#ifndef PARACHUTE_H
#define PARACHUTE_H

#include "../game.h"
#include "../prng.h"

enum { PAR_MAX_HELIS = 4, PAR_MAX_TROOPERS = 12, PAR_MAX_BULLETS = 4, PAR_MAX_LANDED = 8,
       PAR_AIM_MIN = -75, PAR_AIM_MAX = 75, PAR_AIM_ANALOG = 3, PAR_AIM_DIGITAL = 2,
       PAR_BULLET_SPEED = 4, PAR_FIRE_HOLD_TICKS = 12, PAR_HELI_SPEED = 1,
       PAR_TURRET_X0 = 150, PAR_TURRET_X1 = 170, PAR_TURRET_Y = 220, PAR_GROUND = 228,
       PAR_SIDE_LIMIT = 4, PAR_AUTOPLAY_CEASEFIRE = 60 };
typedef enum { PAR_ST_TITLE, PAR_ST_PLAY, PAR_ST_GAMEOVER } par_state_t;
typedef struct { bool alive; int x, y, dir, drops_left; } par_heli_t;
typedef struct { bool alive, chute; int x, y; } par_trooper_t;
typedef struct { bool alive; int32_t x, y, vx, vy; } par_bullet_t;   /* Q8.8 */
typedef struct { bool alive; int x; } par_landed_t;                    /* standing at y 220..228 */
typedef struct {
    par_state_t state;
    int aim;                                   /* degrees, -75..+75, 0 straight up */
    int fire_cooldown;
    par_heli_t helis[PAR_MAX_HELIS];
    par_trooper_t troopers[PAR_MAX_TROOPERS];
    par_bullet_t bullets[PAR_MAX_BULLETS];
    par_landed_t landed[PAR_MAX_LANDED];
    int landed_left, landed_right;
    int spawn_ticks, score, high_score;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} parachute_t;
/* Direction table for -75..+75 in 5-degree steps: index = (deg + 75) / 5; Q8.8 (sin, cos). */
extern const int16_t PAR_SIN[31];
extern const int16_t PAR_COS[31];
int par_aim_index(int deg);                    /* rounds to the nearest 5 degrees, clamps */
void parachute_init(parachute_t *g); void parachute_start(parachute_t *g, uint32_t seed);
void parachute_update(parachute_t *g, const input_t in[GAME_MAX_PLAYERS]);
void parachute_render(const parachute_t *g, const draw_t *d);
void parachute_autoplay(const parachute_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_PARACHUTE;

#endif
