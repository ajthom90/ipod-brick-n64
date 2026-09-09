#include "snake.h"
#include <string.h>

static snk_cell_t *head_cell(snake_t *g) {
    return &g->body[g->head];
}

static const snk_cell_t *head_cell_c(const snake_t *g) {
    return &g->body[g->head];
}

bool snake_occupied(const snake_t *g, int x, int y) {
    for (int i = 0; i < g->len; i++) {
        const snk_cell_t *c = &g->body[(g->head + i) % SNK_MAX];
        if (c->x == x && c->y == y) return true;
    }
    return false;
}

void snake_place_food(snake_t *g) {
    int empty = SNK_COLS * SNK_ROWS - g->len;
    if (empty <= 0) return;
    uint32_t k = prng_below(&g->rng, (uint32_t)empty);
    for (int y = 0; y < SNK_ROWS; y++) {
        for (int x = 0; x < SNK_COLS; x++) {
            if (snake_occupied(g, x, y)) continue;
            if (k == 0) {
                g->food.x = (int8_t)x;
                g->food.y = (int8_t)y;
                return;
            }
            k--;
        }
    }
}

static void read_dir(const input_t *in, int *dx, int *dy) {
    *dx = 0;
    *dy = 0;
    if (in->dpad_x != 0) {
        *dx = in->dpad_x;
        return;
    }
    if (in->dpad_y != 0) {
        *dy = -in->dpad_y;
        return;
    }
    if (in->stick_x >= 128) { *dx = 1; return; }
    if (in->stick_x <= -128) { *dx = -1; return; }
    if (in->stick_y >= 128) { *dy = -1; return; }
    if (in->stick_y <= -128) { *dy = 1; return; }
}

static int in_bounds(int x, int y) {
    return x >= 0 && x < SNK_COLS && y >= 0 && y < SNK_ROWS;
}

static int hits_body(const snake_t *g, int x, int y, int growing) {
    int n = g->len;
    if (!growing && n > 0) n--; /* tail vacates this step */
    for (int i = 0; i < n; i++) {
        const snk_cell_t *c = &g->body[(g->head + i) % SNK_MAX];
        if (c->x == x && c->y == y) return 1;
    }
    return 0;
}

static void die(snake_t *g) {
    if (g->score > g->high_score) g->high_score = g->score;
    sfx_push(&g->sfx, SFX_GAME_OVER);
    g->state = SNK_ST_GAMEOVER;
}

static void apply_period(snake_t *g) {
    int p = SNK_PERIOD_START - g->foods / SNK_FOODS_PER_SPEEDUP;
    if (p < SNK_PERIOD_MIN) p = SNK_PERIOD_MIN;
    g->period = p;
}

static void step_snake(snake_t *g) {
    g->dir_x = g->pending_x;
    g->dir_y = g->pending_y;
    int nx = head_cell(g)->x + g->dir_x;
    int ny = head_cell(g)->y + g->dir_y;
    if (!in_bounds(nx, ny)) {
        die(g);
        return;
    }
    int growing = (nx == g->food.x && ny == g->food.y);
    if (hits_body(g, nx, ny, growing)) {
        die(g);
        return;
    }
    int nh = (g->head - 1 + SNK_MAX) % SNK_MAX;
    g->body[nh].x = (int8_t)nx;
    g->body[nh].y = (int8_t)ny;
    g->head = nh;
    if (growing) {
        g->len++;
        g->score++;
        g->foods++;
        apply_period(g);
        sfx_push(&g->sfx, SFX_FOOD);
        snake_place_food(g);
    }
}

static void play_tick(snake_t *g, const input_t *in) {
    int dx, dy;
    read_dir(in, &dx, &dy);
    if (dx != 0 || dy != 0) {
        if (!(dx == -g->dir_x && dy == -g->dir_y)) {
            g->pending_x = (int8_t)dx;
            g->pending_y = (int8_t)dy;
        }
    }
    g->step_ticks++;
    if (g->step_ticks >= g->period) {
        g->step_ticks = 0;
        step_snake(g);
    }
}

void snake_init(snake_t *g) {
    memset(g, 0, sizeof *g);
    g->state = SNK_ST_TITLE;
    g->period = SNK_PERIOD_START;
}

void snake_start(snake_t *g, uint32_t seed) {
    int hs = g->high_score;
    memset(g, 0, sizeof *g);
    g->high_score = hs;
    g->state = SNK_ST_TITLE;
    g->period = SNK_PERIOD_START;
    g->len = 4;
    g->head = 0;
    g->body[0] = (snk_cell_t){18, 12};
    g->body[1] = (snk_cell_t){17, 12};
    g->body[2] = (snk_cell_t){16, 12};
    g->body[3] = (snk_cell_t){15, 12};
    g->dir_x = 1;
    g->dir_y = 0;
    g->pending_x = 1;
    g->pending_y = 0;
    prng_seed(&g->rng, seed);
    snake_place_food(g);
}

void snake_update(snake_t *g, const input_t in[GAME_MAX_PLAYERS]) {
    g->ticks++;
    switch (g->state) {
    case SNK_ST_TITLE:
        if (in[0].a) g->state = SNK_ST_PLAY;
        break;
    case SNK_ST_PLAY:
        play_tick(g, &in[0]);
        break;
    case SNK_ST_GAMEOVER:
        if (in[0].a) snake_start(g, g->ticks);
        break;
    }
}

void snake_render(const snake_t *g, const draw_t *d) {
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);

    for (int i = g->len - 1; i >= 0; i--) {
        const snk_cell_t *c = &g->body[(g->head + i) % SNK_MAX];
        int x0 = SNK_X0 + c->x * SNK_CELL;
        int y0 = SNK_Y0 + c->y * SNK_CELL;
        uint32_t rgb = (i == 0) ? COLOR_BALL : COLOR_BLUE;
        draw_rect(d, x0, y0, x0 + 7, y0 + 7, rgb);
    }

    int fx0 = SNK_X0 + g->food.x * SNK_CELL;
    int fy0 = SNK_Y0 + g->food.y * SNK_CELL;
    draw_rect(d, fx0, fy0, fx0 + 7, fy0 + 7, COLOR_RED);

    char buf[24];
    fmt_label(buf, sizeof buf, "SCORE", g->score);
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, SNK_X0, HUD_BASELINE, DRAW_TEXT_DARK, buf);
    fmt_label(buf, sizeof buf, "HIGH", g->high_score);
    draw_text(d, DRAW_FONT_HUD, DRAW_RIGHT, PLAY_X1, HUD_BASELINE, DRAW_TEXT_DARK, buf);

    if (g->state == SNK_ST_TITLE) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 100, DRAW_TEXT_DARK, "SNAKE");
        fmt_label(buf, sizeof buf, "HIGH SCORE", g->high_score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 130, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 160, DRAW_TEXT_DARK, "PRESS A");
    } else if (g->state == SNK_ST_GAMEOVER) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 140, DRAW_TEXT_DARK, "GAME OVER");
        fmt_label(buf, sizeof buf, "SCORE", g->score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 168, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 196, DRAW_TEXT_DARK, "PRESS A");
    }
}

static int manh(int x0, int y0, int x1, int y1) {
    int dx = x0 - x1;
    int dy = y0 - y1;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx + dy;
}

void snake_autoplay(const snake_t *g, input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
    if (g->state == SNK_ST_TITLE || g->state == SNK_ST_GAMEOVER) {
        in[0].a = true;
        return;
    }
    if (g->state != SNK_ST_PLAY) return;
    if (g->step_ticks + 1 < g->period) return;

    static const int8_t DIRS[4][2] = {{1, 0}, {0, -1}, {-1, 0}, {0, 1}};
    int hx = head_cell_c(g)->x;
    int hy = head_cell_c(g)->y;
    int best_i = -1;
    int best_m = 0;
    for (int i = 0; i < 4; i++) {
        int dx = DIRS[i][0];
        int dy = DIRS[i][1];
        if (dx == -g->dir_x && dy == -g->dir_y) continue;
        int nx = hx + dx;
        int ny = hy + dy;
        if (!in_bounds(nx, ny)) continue;
        int growing = (nx == g->food.x && ny == g->food.y);
        if (hits_body(g, nx, ny, growing)) continue;
        int m = manh(nx, ny, g->food.x, g->food.y);
        if (best_i < 0 || m < best_m) {
            best_i = i;
            best_m = m;
        }
    }
    if (best_i < 0) return;
    int dx = DIRS[best_i][0];
    int dy = DIRS[best_i][1];
    if (dx != 0) in[0].dpad_x = (int8_t)dx;
    else in[0].dpad_y = (int8_t)(-dy);
}

static snake_t snake_state;
static void desc_init(void *st) { snake_init(st); }
static void desc_start(void *st, uint32_t seed) { snake_start(st, seed); }
static void desc_update(void *st, const input_t in[GAME_MAX_PLAYERS]) { snake_update(st, in); }
static void desc_render(const void *st, const draw_t *d) { snake_render(st, d); }
static void desc_autoplay(const void *st, input_t in[GAME_MAX_PLAYERS]) { snake_autoplay(st, in); }
static bool desc_is_over(const void *st) { return ((const snake_t *)st)->state == SNK_ST_GAMEOVER; }
static int  desc_get_hs(const void *st) { return ((const snake_t *)st)->high_score; }
static void desc_set_hs(void *st, int v) { ((snake_t *)st)->high_score = v; }
static sfx_queue_t *desc_sfx(void *st) { return &((snake_t *)st)->sfx; }

const game_desc_t GAME_SNAKE = {
    .name = "SNAKE", .players = 1, .state = &snake_state,
    .init = desc_init, .start = desc_start, .update = desc_update,
    .render = desc_render, .autoplay = desc_autoplay, .is_over = desc_is_over,
    .get_high_score = desc_get_hs, .set_high_score = desc_set_hs, .sfx = desc_sfx,
    .track = MUSIC_SNAKE,
};
