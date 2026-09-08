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

/* Stubs completed in Tasks 2 and 3. */
void brick_new_game(brick_game_t *g) { (void)g; }
void brick_on_ball_lost(brick_game_t *g) { (void)g; }
void brick_on_level_clear(brick_game_t *g) { (void)g; }
void brick_update(brick_game_t *g, const brick_input_t *in) { (void)in; g->ticks++; }
void brick_autoplay_input(const brick_game_t *g, brick_input_t *in) { (void)g; memset(in, 0, sizeof *in); }
