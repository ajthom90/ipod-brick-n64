#ifndef HOPPER_H
#define HOPPER_H

#include "../game.h"
#include "../prng.h"

enum { HOP_PLAYER = 12, HOP_PLAT_W = 32, HOP_PLAT_H = 6, HOP_MAX_PLATS = 24,
       HOP_GRAVITY = 64, HOP_JUMP_VY = -1664, HOP_SPRING_VY = -2560,
       HOP_MOVE_ANALOG_MAX = 4, HOP_MOVE_DIGITAL = 3, HOP_CAMERA_LINE = 100,
       HOP_GAP_MIN = 40, HOP_GAP_RAND = 21, HOP_GAP_EXTRA_CAP = 20, HOP_MOVING_SPEED = 1,
       HOP_AUTOPLAY_STOP = 300 };
typedef enum { HOP_ST_TITLE, HOP_ST_PLAY, HOP_ST_GAMEOVER } hop_state_t;
typedef enum { HOP_PLAT_STATIC, HOP_PLAT_MOVING, HOP_PLAT_SPRING } hop_plat_type_t;
typedef struct { bool alive; int x, y; int dir; hop_plat_type_t type; } hop_plat_t;   /* y = world top */
typedef struct {
    hop_state_t state;
    int32_t px, py, vy;               /* Q8.8 world position of the player's top-left, vertical velocity */
    int camera_y, camera_start;       /* world px */
    hop_plat_t plats[HOP_MAX_PLATS];
    int top_y;                        /* world y of the highest generated platform */
    int score, high_score;
    prng_t rng; sfx_queue_t sfx; uint32_t ticks;
} hopper_t;
void hopper_generate_up_to(hopper_t *g, int world_y);   /* generate platforms until top_y <= world_y */
void hopper_init(hopper_t *g); void hopper_start(hopper_t *g, uint32_t seed);
void hopper_update(hopper_t *g, const input_t in[GAME_MAX_PLAYERS]);
void hopper_render(const hopper_t *g, const draw_t *d);
void hopper_autoplay(const hopper_t *g, input_t in[GAME_MAX_PLAYERS]);
extern const game_desc_t GAME_HOPPER;

#endif
