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

static bool hit_brick(brick_game_t *g, bool horizontal) {
    brick_rect_t ball = brick_ball_rect(g);
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (!g->cells[r][c]) continue;
            brick_rect_t cell = brick_cell_rect(r, c);
            if (!brick_rects_overlap(ball, cell)) continue;
            g->cells[r][c] = 0;
            g->score++;
            g->bricks_left--;
            if (horizontal) {
                if (g->ball_vx > 0)
                    g->ball_x = (cell.x0 - BRICK_BALL_SIZE) << 8;
                else
                    g->ball_x = cell.x1 << 8;
                g->ball_vx = -g->ball_vx;
            } else {
                if (g->ball_vy > 0)
                    g->ball_y = (cell.y0 - BRICK_BALL_SIZE) << 8;
                else
                    g->ball_y = cell.y1 << 8;
                g->ball_vy = -g->ball_vy;
            }
            return true;
        }
    }
    return false;
}

static void bounce_off_paddle(brick_game_t *g) {
    g->ball_y = (BRICK_PADDLE_Y - BRICK_BALL_SIZE) << 8;
    int cx = (g->ball_x >> 8) + BRICK_BALL_SIZE / 2;
    int zone = ((cx - g->paddle_x) * 7) / BRICK_PADDLE_W;
    if (zone < 0) zone = 0;
    if (zone > 6) zone = 6;
    int sign = BRICK_BOUNCE_SIGN[zone];
    if (sign == 0) sign = (g->ball_vx < 0) ? -1 : 1;
    g->ball_vx = sign * ((g->ball_speed * BRICK_BOUNCE_SIN[zone]) >> 8);
    g->ball_vy = -((g->ball_speed * BRICK_BOUNCE_COS[zone]) >> 8);
}

static bool pass_x(brick_game_t *g, int32_t dx) {
    g->ball_x += dx;
    brick_rect_t rect = brick_ball_rect(g);
    if (rect.x0 < BRICK_PLAY_X0) {
        g->ball_x = BRICK_PLAY_X0 << 8;
        if (g->ball_vx < 0) g->ball_vx = -g->ball_vx;
    }
    if (rect.x1 > BRICK_PLAY_X1) {
        g->ball_x = (BRICK_PLAY_X1 - BRICK_BALL_SIZE) << 8;
        if (g->ball_vx > 0) g->ball_vx = -g->ball_vx;
    }
    hit_brick(g, true);
    return false;
}

static bool pass_y(brick_game_t *g, int32_t dy) {
    g->ball_y += dy;
    brick_rect_t rect = brick_ball_rect(g);
    if (rect.y0 < BRICK_PLAY_Y0) {
        g->ball_y = BRICK_PLAY_Y0 << 8;
        if (g->ball_vy < 0) g->ball_vy = -g->ball_vy;
    }
    hit_brick(g, false);
    if (g->ball_vy > 0 && brick_rects_overlap(brick_ball_rect(g), brick_paddle_rect(g))) {
        bounce_off_paddle(g);
    }
    if (rect.y0 >= BRICK_PLAY_Y1) {
        brick_on_ball_lost(g);
        return true;
    }
    return false;
}

static void step_ball(brick_game_t *g) {
    int n = (g->ball_speed > BRICK_SUBSTEP_THRESHOLD) ? 2 : 1;
    for (int i = 0; i < n; i++) {
        int32_t dx = g->ball_vx / n;
        int32_t dy = g->ball_vy / n;
        if (pass_x(g, dx)) return;
        if (pass_y(g, dy)) return;
    }
    if (g->bricks_left == 0) brick_on_level_clear(g);
}

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

void brick_autoplay_input(const brick_game_t *g, brick_input_t *in) {
    memset(in, 0, sizeof *in);
    switch (g->state) {
    case BRICK_ST_TITLE:
        in->confirm = true;
        break;
    case BRICK_ST_SERVE:
        in->launch = true;
        break;
    case BRICK_ST_PLAY:
        if (g->level >= 2) break;
        {
            int cx = (g->ball_x >> 8) + BRICK_BALL_SIZE / 2;
            /* Aim at the remaining brick nearest to the ball horizontally,
             * scanning from the bottom row so reachable bricks win ties. */
            int best = -1, best_dx = 0;
            for (int r = BRICK_ROWS - 1; r >= 0; r--) {
                for (int c = 0; c < BRICK_COLS; c++) {
                    if (!g->cells[r][c]) continue;
                    brick_rect_t cell = brick_cell_rect(r, c);
                    int dx = (cell.x0 + cell.x1) / 2 - cx;
                    int adx = dx < 0 ? -dx : dx;
                    if (best < 0 || adx < best) { best = adx; best_dx = dx; }
                }
            }
            int k = (int)((g->ticks / 300) % 3);
            int zt;
            if (best_dx > 8) zt = 4 + k;          /* send it right: zones 4,5,6 */
            else if (best_dx < -8) zt = 2 - k;    /* send it left: zones 2,1,0 */
            else zt = 3;
            int tx = cx - (zt * BRICK_PADDLE_W) / 7 - BRICK_PADDLE_W / 14;
            if (g->paddle_x < tx - 2) in->paddle_axis = 256;
            else if (g->paddle_x > tx + 2) in->paddle_axis = -256;
        }
        break;
    default:
        break;
    }
}
