#ifndef BRICK_GAME_H
#define BRICK_GAME_H

#include <stdint.h>
#include <stdbool.h>

/* Layout in pixels. The N64 framebuffer is 320x240, the same size as the
 * 5G iPod screen, so this is a 1:1 layout. 16 px margins stay clear of TV
 * overscan. */
enum {
    BRICK_SCREEN_W = 320, BRICK_SCREEN_H = 240,
    BRICK_PLAY_X0 = 16, BRICK_PLAY_Y0 = 24, BRICK_PLAY_X1 = 304, BRICK_PLAY_Y1 = 228,
    BRICK_ROWS = 6, BRICK_COLS = 10,
    BRICK_W = 26, BRICK_H = 10, BRICK_GAP_X = 2, BRICK_GAP_Y = 3,
    BRICK_GRID_X0 = 20, BRICK_GRID_Y0 = 32,
    BRICK_PADDLE_W = 48, BRICK_PADDLE_H = 6, BRICK_PADDLE_Y = 218,
    BRICK_BALL_SIZE = 6,
    BRICK_LIVES = 3,
    BRICK_PADDLE_SPEED_DIGITAL = 4,        /* px per tick on D-pad / C buttons */
    BRICK_PADDLE_SPEED_ANALOG_MAX = 6,     /* px per tick at full stick deflection */
    BRICK_BALL_SPEED_BASE = 512,           /* Q8.8: 2.00 px per tick at level 1 */
    BRICK_BALL_SPEED_RAMP = 90,            /* Q8.8: +0.35 px per tick per level */
    BRICK_BALL_SPEED_MAX = 1536,           /* Q8.8: 6.00 px per tick */
    BRICK_SUBSTEP_THRESHOLD = 768,         /* Q8.8: above 3.00 px per tick use 2 sub-steps */
    BRICK_CATCHUP_MAX = 4,                 /* adapter: max ticks per rendered frame */
};

/* Colors as 0xRRGGBB. */
#define BRICK_COLOR_BG     0xE8E4DCu
#define BRICK_COLOR_PADDLE 0x2C2C2Cu
#define BRICK_COLOR_BALL   0x1A1A1Au
#define BRICK_COLOR_TEXT   0x2C2C2Cu

typedef enum {
    BRICK_ST_TITLE,
    BRICK_ST_SERVE,
    BRICK_ST_PLAY,
    BRICK_ST_PAUSE,
    BRICK_ST_GAMEOVER,
} brick_state_t;

/* One tick of player input. Edge fields are true only on the tick when the
 * button went down. */
typedef struct {
    int8_t  paddle_dir;   /* -1, 0, +1 from D-pad / C-left / C-right */
    int16_t paddle_axis;  /* -256..+256 from the analog stick, 0 inside the dead zone */
    bool    launch;       /* A pressed (edge) */
    bool    confirm;      /* A or B pressed (edge) */
    bool    pause;        /* Start pressed (edge) */
} brick_input_t;

typedef struct {
    brick_state_t state;
    brick_state_t pause_return;   /* state to return to from PAUSE: SERVE or PLAY */
    int lives, score, high_score, level;
    int paddle_x;                 /* px, left edge */
    int32_t ball_x, ball_y;       /* Q8.8, top-left corner of the ball */
    int32_t ball_vx, ball_vy;     /* Q8.8 px per tick */
    int32_t ball_speed;           /* Q8.8 speed magnitude for the current level */
    uint8_t cells[BRICK_ROWS][BRICK_COLS];  /* 1 = brick present */
    int bricks_left;
    uint32_t ticks;               /* ticks since brick_init */
} brick_game_t;

/* Pixel rectangle with exclusive x1/y1, matching rdpq_fill_rectangle. */
typedef struct { int x0, y0, x1, y1; } brick_rect_t;

/* Paddle bounce table: 7 zones across the paddle. Angle from vertical is
 * -60, -40, -20, +-8, +20, +40, +60 degrees. SIN/COS are Q8.8 unit-vector
 * components; SIGN is the horizontal direction (0 = keep incoming sign, or
 * +1 if the incoming vx is 0). */
extern const int16_t BRICK_BOUNCE_SIN[7];
extern const int16_t BRICK_BOUNCE_COS[7];
extern const int8_t  BRICK_BOUNCE_SIGN[7];

void brick_init(brick_game_t *g);                       /* zero everything, state TITLE */
void brick_new_game(brick_game_t *g);                   /* keep high_score; lives, level 1, refill, SERVE */
void brick_update(brick_game_t *g, const brick_input_t *in);   /* advance exactly one tick */
void brick_autoplay_input(const brick_game_t *g, brick_input_t *in); /* AI input for tests and screenshots */

/* Transition helpers, public so tests and tools can drive them directly. */
void brick_on_ball_lost(brick_game_t *g);               /* lives--, SERVE or GAMEOVER */
void brick_on_level_clear(brick_game_t *g);             /* level++, faster, refill, SERVE */

brick_rect_t brick_cell_rect(int row, int col);
brick_rect_t brick_paddle_rect(const brick_game_t *g);
brick_rect_t brick_ball_rect(const brick_game_t *g);
bool         brick_rects_overlap(brick_rect_t a, brick_rect_t b);
uint32_t     brick_row_color(int row);                  /* 0xRRGGBB */

#endif
