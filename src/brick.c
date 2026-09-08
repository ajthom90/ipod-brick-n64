#include "brick.h"
#include <string.h>

const int16_t BRICK_BOUNCE_SIN[7]  = { 222, 165,  88,  36,  88, 165, 222 };
const int16_t BRICK_BOUNCE_COS[7]  = { 128, 196, 241, 253, 241, 196, 128 };
const int8_t  BRICK_BOUNCE_SIGN[7] = {  -1,  -1,  -1,   0,   1,   1,   1 };

static const uint32_t ROW_COLORS[BRICK_ROWS] = {
    0xC4472Au, 0xE07A1Fu, 0xD4B01Cu, 0x3FA34Du, 0x2E6DB4u, 0x7B4EA3u,
};

uint32_t brick_row_color(int row) {
    if (row < 0) row = 0;
    if (row >= BRICK_ROWS) row = BRICK_ROWS - 1;
    return ROW_COLORS[row];
}

brick_rect_t brick_cell_rect(int row, int col) {
    brick_rect_t r;
    r.x0 = BRICK_GRID_X0 + col * (BRICK_W + BRICK_GAP_X);
    r.y0 = BRICK_GRID_Y0 + row * (BRICK_H + BRICK_GAP_Y);
    r.x1 = r.x0 + BRICK_W;
    r.y1 = r.y0 + BRICK_H;
    return r;
}

brick_rect_t brick_paddle_rect(const brick_game_t *g) {
    brick_rect_t r = { g->paddle_x, BRICK_PADDLE_Y, g->paddle_x + BRICK_PADDLE_W, BRICK_PADDLE_Y + BRICK_PADDLE_H };
    return r;
}

brick_rect_t brick_ball_rect(const brick_game_t *g) {
    brick_rect_t r;
    r.x0 = g->ball_x >> 8;
    r.y0 = g->ball_y >> 8;
    r.x1 = r.x0 + BRICK_BALL_SIZE;
    r.y1 = r.y0 + BRICK_BALL_SIZE;
    return r;
}

bool brick_rects_overlap(brick_rect_t a, brick_rect_t b) {
    return a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1;
}

void brick_init(brick_game_t *g) {
    memset(g, 0, sizeof *g);
    g->state = BRICK_ST_TITLE;
    g->level = 1;
}

static int32_t speed_for_level(int level) {
    int32_t s = BRICK_BALL_SPEED_BASE + BRICK_BALL_SPEED_RAMP * (level - 1);
    return s > BRICK_BALL_SPEED_MAX ? BRICK_BALL_SPEED_MAX : s;
}

static void refill(brick_game_t *g) {
    memset(g->cells, 1, sizeof g->cells);
    g->bricks_left = BRICK_ROWS * BRICK_COLS;
}

static void park_ball(brick_game_t *g) {
    g->ball_x = (g->paddle_x + (BRICK_PADDLE_W - BRICK_BALL_SIZE) / 2) << 8;
    g->ball_y = (BRICK_PADDLE_Y - BRICK_BALL_SIZE) << 8;
    g->ball_vx = 0;
    g->ball_vy = 0;
}

static void move_paddle(brick_game_t *g, const brick_input_t *in) {
    int dx = in->paddle_axis != 0
        ? (in->paddle_axis * BRICK_PADDLE_SPEED_ANALOG_MAX) / 256
        : in->paddle_dir * BRICK_PADDLE_SPEED_DIGITAL;
    g->paddle_x += dx;
    if (g->paddle_x < BRICK_PLAY_X0) g->paddle_x = BRICK_PLAY_X0;
    if (g->paddle_x > BRICK_PLAY_X1 - BRICK_PADDLE_W) g->paddle_x = BRICK_PLAY_X1 - BRICK_PADDLE_W;
}

static void launch_ball(brick_game_t *g) {
    int sign = (g->level % 2 == 1) ? 1 : -1;
    g->ball_vx = sign * ((g->ball_speed * BRICK_BOUNCE_SIN[3]) >> 8);
    g->ball_vy = -((g->ball_speed * BRICK_BOUNCE_COS[3]) >> 8);
    g->state = BRICK_ST_PLAY;
}

static void step_ball(brick_game_t *g) { (void)g; }

void brick_new_game(brick_game_t *g) {
    int high = g->high_score;
    g->lives = BRICK_LIVES;
    g->score = 0;
    g->level = 1;
    g->ball_speed = BRICK_BALL_SPEED_BASE;
    g->paddle_x = (BRICK_PLAY_X0 + BRICK_PLAY_X1) / 2 - BRICK_PADDLE_W / 2;
    refill(g);
    park_ball(g);
    g->state = BRICK_ST_SERVE;
    g->high_score = high;
}

void brick_on_ball_lost(brick_game_t *g) {
    g->lives--;
    if (g->lives > 0) {
        park_ball(g);
        g->state = BRICK_ST_SERVE;
    } else {
        if (g->score > g->high_score) g->high_score = g->score;
        g->state = BRICK_ST_GAMEOVER;
    }
}

void brick_on_level_clear(brick_game_t *g) {
    g->level++;
    g->ball_speed = speed_for_level(g->level);
    refill(g);
    park_ball(g);
    g->state = BRICK_ST_SERVE;
}

void brick_update(brick_game_t *g, const brick_input_t *in) {
    g->ticks++;
    switch (g->state) {
    case BRICK_ST_TITLE:
        if (in->confirm) brick_new_game(g);
        break;
    case BRICK_ST_SERVE:
        if (in->pause) { g->pause_return = BRICK_ST_SERVE; g->state = BRICK_ST_PAUSE; break; }
        move_paddle(g, in);
        park_ball(g);
        if (in->launch) launch_ball(g);
        break;
    case BRICK_ST_PLAY:
        if (in->pause) { g->pause_return = BRICK_ST_PLAY; g->state = BRICK_ST_PAUSE; break; }
        move_paddle(g, in);
        step_ball(g);
        break;
    case BRICK_ST_PAUSE:
        if (in->pause) g->state = g->pause_return;
        break;
    case BRICK_ST_GAMEOVER:
        if (in->confirm) g->state = BRICK_ST_TITLE;
        break;
    }
}

void brick_autoplay_input(const brick_game_t *g, brick_input_t *in) { (void)g; memset(in, 0, sizeof *in); }
