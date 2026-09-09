#include "pong.h"
#include "brick.h"
#include <string.h>

enum { PNG_BALL_X0 = 157, PNG_BALL_Y0 = 123 };

static int paddle_home_y(void) {
    return (PLAY_Y0 + PLAY_Y1 - PNG_PADDLE_H) / 2;
}

static int clamp_paddle_y(int y) {
    if (y < PLAY_Y0) y = PLAY_Y0;
    if (y > PLAY_Y1 - PNG_PADDLE_H) y = PLAY_Y1 - PNG_PADDLE_H;
    return y;
}

static int paddle_dy(const input_t *in) {
    if (in->stick_y != 0)
        return -((in->stick_y * PNG_PADDLE_ANALOG_MAX) / 256);
    return -in->dpad_y * PNG_PADDLE_DIGITAL;
}

static void park_ball(pong_t *g) {
    g->ball_x = PNG_BALL_X0 << 8;
    g->ball_y = PNG_BALL_Y0 << 8;
    g->ball_vx = 0;
    g->ball_vy = 0;
}

static void begin_serve(pong_t *g) {
    g->state = PNG_ST_SERVE;
    g->serve_ticks = 0;
    g->ball_speed = PNG_SPEED_SERVE;
    park_ball(g);
}

static void apply_bounce(pong_t *g, int zone, int hdir) {
    if (zone < 0) zone = 0;
    if (zone > 6) zone = 6;
    int vsign = BRICK_BOUNCE_SIGN[zone];
    if (vsign == 0) vsign = (g->ball_vy < 0) ? -1 : 1;
    g->ball_vx = hdir * ((g->ball_speed * BRICK_BOUNCE_COS[zone]) >> 8);
    g->ball_vy = vsign * ((g->ball_speed * BRICK_BOUNCE_SIN[zone]) >> 8);
}

static void launch_ball(pong_t *g) {
    int zone = prng_below(&g->rng, 2) ? 4 : 2;
    int hdir = (g->serve_to == 2) ? 1 : -1;
    apply_bounce(g, zone, hdir);
    g->state = PNG_ST_PLAY;
}

static void start_match(pong_t *g) {
    g->two_players = (g->title_row == 1);
    g->score1 = 0;
    g->score2 = 0;
    g->serve_to = 2;
    g->p1_y = paddle_home_y();
    g->p2_y = paddle_home_y();
    begin_serve(g);
}

static int ball_px(int32_t q) { return q >> 8; }

static int zone_at(int paddle_y, int32_t ball_y) {
    int cy = ball_px(ball_y) + PNG_BALL / 2;
    int zone = ((cy - paddle_y) * 7) / PNG_PADDLE_H;
    if (zone < 0) zone = 0;
    if (zone > 6) zone = 6;
    return zone;
}

static int rects_overlap(int ax0, int ay0, int ax1, int ay1,
                         int bx0, int by0, int bx1, int by1) {
    return ax0 < bx1 && bx0 < ax1 && ay0 < by1 && by0 < ay1;
}

static int ball_hits_paddle(const pong_t *g, int px, int py) {
    int x0 = ball_px(g->ball_x);
    int y0 = ball_px(g->ball_y);
    return rects_overlap(x0, y0, x0 + PNG_BALL, y0 + PNG_BALL,
                         px, py, px + PNG_PADDLE_W, py + PNG_PADDLE_H);
}

static void raise_speed(pong_t *g) {
    g->ball_speed += PNG_SPEED_STEP;
    if (g->ball_speed > PNG_SPEED_MAX) g->ball_speed = PNG_SPEED_MAX;
}

static void hit_paddle(pong_t *g, int py, int hdir, int clear_x) {
    g->ball_x = clear_x << 8;
    raise_speed(g);
    apply_bounce(g, zone_at(py, g->ball_y), hdir);
    sfx_push(&g->sfx, SFX_BOUNCE);
}

static void point_scored(pong_t *g, int scorer) {
    if (scorer == 1) g->score1++;
    else g->score2++;
    sfx_push(&g->sfx, SFX_POINT);
    if (g->score1 >= PNG_WIN_SCORE || g->score2 >= PNG_WIN_SCORE) {
        if (g->score1 >= PNG_WIN_SCORE) {
            int margin = g->score1 - g->score2;
            if (margin > g->high_score) g->high_score = margin;
        }
        sfx_push(&g->sfx, SFX_GAME_OVER);
        g->state = PNG_ST_GAMEOVER;
        return;
    }
    g->serve_to = (scorer == 1) ? 2 : 1;
    begin_serve(g);
}

static int pass_x(pong_t *g, int32_t dx) {
    g->ball_x += dx;
    int x0 = ball_px(g->ball_x);

    if (g->ball_vx < 0 && ball_hits_paddle(g, PNG_P1_X, g->p1_y)) {
        hit_paddle(g, g->p1_y, 1, PNG_P1_X + PNG_PADDLE_W);
        return 0;
    }
    if (g->ball_vx > 0 && ball_hits_paddle(g, PNG_P2_X, g->p2_y)) {
        hit_paddle(g, g->p2_y, -1, PNG_P2_X - PNG_BALL);
        return 0;
    }

    if (x0 + PNG_BALL <= PLAY_X0) {
        point_scored(g, 2);
        return 1;
    }
    if (x0 >= PLAY_X1) {
        point_scored(g, 1);
        return 1;
    }
    return 0;
}

static int pass_y(pong_t *g, int32_t dy) {
    g->ball_y += dy;
    int y0 = ball_px(g->ball_y);
    if (y0 < PLAY_Y0) {
        g->ball_y = PLAY_Y0 << 8;
        if (g->ball_vy < 0) g->ball_vy = -g->ball_vy;
        sfx_push(&g->sfx, SFX_BOUNCE);
    }
    if (y0 + PNG_BALL > PLAY_Y1) {
        g->ball_y = (PLAY_Y1 - PNG_BALL) << 8;
        if (g->ball_vy > 0) g->ball_vy = -g->ball_vy;
        sfx_push(&g->sfx, SFX_BOUNCE);
    }
    return 0;
}

static void step_ball(pong_t *g) {
    int n = (g->ball_speed > PNG_SPEED_SERVE) ? 2 : 1;
    for (int i = 0; i < n; i++) {
        int32_t dx = g->ball_vx / n;
        int32_t dy = g->ball_vy / n;
        if (pass_x(g, dx)) return;
        if (pass_y(g, dy)) return;
        if (g->state != PNG_ST_PLAY) return;
    }
}

void pong_ai(const pong_t *g, int paddle_y, int32_t toward_x, int *dy) {
    int paddle_cx = (int)toward_x + PNG_PADDLE_W / 2;
    int ball_cx = ball_px(g->ball_x) + PNG_BALL / 2;
    int ball_cy = ball_px(g->ball_y) + PNG_BALL / 2;
    int toward = (g->ball_vx > 0 && paddle_cx > ball_cx) ||
                 (g->ball_vx < 0 && paddle_cx < ball_cx);
    int target;
    int maxstep;
    if (toward) {
        target = ball_cy;
        maxstep = PNG_AI_SPEED;
    } else {
        target = paddle_home_y();
        maxstep = 1;
    }
    int d = target - paddle_y;
    if (d > maxstep) d = maxstep;
    if (d < -maxstep) d = -maxstep;
    *dy = d;
}

static void move_p2(pong_t *g, const input_t in[GAME_MAX_PLAYERS]) {
    if (g->two_players) {
        g->p2_y = clamp_paddle_y(g->p2_y + paddle_dy(&in[1]));
    } else {
        int dy;
        pong_ai(g, g->p2_y, PNG_P2_X, &dy);
        g->p2_y = clamp_paddle_y(g->p2_y + dy);
    }
}

static void move_paddles(pong_t *g, const input_t in[GAME_MAX_PLAYERS]) {
    g->p1_y = clamp_paddle_y(g->p1_y + paddle_dy(&in[0]));
    move_p2(g, in);
}

static int8_t title_nav(const input_t *in) {
    if (in->dpad_y != 0) return in->dpad_y;
    if (in->stick_y >= 128) return 1;
    if (in->stick_y <= -128) return -1;
    return 0;
}

void pong_init(pong_t *g) {
    memset(g, 0, sizeof *g);
    g->state = PNG_ST_TITLE;
    g->p1_y = paddle_home_y();
    g->p2_y = paddle_home_y();
    park_ball(g);
}

void pong_start(pong_t *g, uint32_t seed) {
    int hs = g->high_score;
    memset(g, 0, sizeof *g);
    g->high_score = hs;
    g->state = PNG_ST_TITLE;
    g->title_row = 0;
    g->p1_y = paddle_home_y();
    g->p2_y = paddle_home_y();
    park_ball(g);
    prng_seed(&g->rng, seed);
}

void pong_update(pong_t *g, const input_t in[GAME_MAX_PLAYERS]) {
    g->ticks++;
    switch (g->state) {
    case PNG_ST_TITLE: {
        int8_t nav = title_nav(&in[0]);
        if (nav < 0) g->title_row = 1;
        if (nav > 0) g->title_row = 0;
        if (in[0].a) start_match(g);
        break;
    }
    case PNG_ST_SERVE:
        move_paddles(g, in);
        park_ball(g);
        g->serve_ticks++;
        if (g->serve_ticks >= PNG_SERVE_DELAY) launch_ball(g);
        break;
    case PNG_ST_PLAY:
        move_paddles(g, in);
        step_ball(g);
        break;
    case PNG_ST_GAMEOVER:
        if (in[0].a) {
            int hs = g->high_score;
            int two = g->two_players;
            int row = g->title_row;
            pong_start(g, g->ticks);
            g->high_score = hs;
            g->two_players = two;
            g->title_row = row;
        }
        break;
    }
}

static int16_t dy_to_stick(int dy) {
    if (dy == 0) return 0;
    int mag = dy < 0 ? -dy : dy;
    int stick = (mag * 256 + PNG_PADDLE_ANALOG_MAX - 1) / PNG_PADDLE_ANALOG_MAX;
    if (stick > 256) stick = 256;
    return (int16_t)(dy < 0 ? stick : -stick);
}

void pong_autoplay(const pong_t *g, input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
    if (g->state == PNG_ST_TITLE || g->state == PNG_ST_GAMEOVER) {
        in[0].a = true;
        return;
    }
    int dy;
    pong_ai(g, g->p1_y, PNG_P1_X, &dy);
    in[0].stick_y = dy_to_stick(dy);
    pong_ai(g, g->p2_y, PNG_P2_X, &dy);
    in[1].stick_y = dy_to_stick(dy);
}

void pong_render(const pong_t *g, const draw_t *d) {
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);

    for (int y = PLAY_Y0; y < PLAY_Y1; y += 8) {
        int y1 = y + 4;
        if (y1 > PLAY_Y1) y1 = PLAY_Y1;
        draw_rect(d, 158, y, 162, y1, COLOR_DARK);
    }

    draw_rect(d, PNG_P1_X, g->p1_y, PNG_P1_X + PNG_PADDLE_W, g->p1_y + PNG_PADDLE_H, COLOR_DARK);
    draw_rect(d, PNG_P2_X, g->p2_y, PNG_P2_X + PNG_PADDLE_W, g->p2_y + PNG_PADDLE_H, COLOR_DARK);

    if (g->state != PNG_ST_GAMEOVER) {
        int x0 = ball_px(g->ball_x);
        int y0 = ball_px(g->ball_y);
        draw_rect(d, x0, y0, x0 + PNG_BALL, y0 + PNG_BALL, COLOR_BALL);
    }

    char buf[12];
    fmt_int(buf, sizeof buf, g->score1);
    draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, 120, 50, DRAW_TEXT_DARK, buf);
    fmt_int(buf, sizeof buf, g->score2);
    draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, 200, 50, DRAW_TEXT_DARK, buf);

    if (g->state == PNG_ST_TITLE) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 90, DRAW_TEXT_DARK, "PONG");
        static const char *const labels[2] = { "1 PLAYER", "2 PLAYERS" };
        static const int bases[2] = { 130, 156 };
        for (int i = 0; i < 2; i++) {
            int base = bases[i];
            if (i == g->title_row) {
                draw_rect(d, 96, base - 20, 224, base + 6, COLOR_BLUE);
                draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, base, DRAW_TEXT_LIGHT, labels[i]);
            } else {
                draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, base, DRAW_TEXT_DARK, labels[i]);
            }
        }
    } else if (g->state == PNG_ST_GAMEOVER) {
        const char *msg;
        if (g->two_players) msg = (g->score1 >= PNG_WIN_SCORE) ? "PLAYER 1 WINS" : "PLAYER 2 WINS";
        else msg = (g->score1 >= PNG_WIN_SCORE) ? "YOU WIN" : "CPU WINS";
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 140, DRAW_TEXT_DARK, msg);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 196, DRAW_TEXT_DARK, "PRESS A");
    }
}

static pong_t pong_state;
static void desc_init(void *st) { pong_init(st); }
static void desc_start(void *st, uint32_t seed) { pong_start(st, seed); }
static void desc_update(void *st, const input_t in[GAME_MAX_PLAYERS]) { pong_update(st, in); }
static void desc_render(const void *st, const draw_t *d) { pong_render(st, d); }
static void desc_autoplay(const void *st, input_t in[GAME_MAX_PLAYERS]) { pong_autoplay(st, in); }
static bool desc_is_over(const void *st) { return ((const pong_t *)st)->state == PNG_ST_GAMEOVER; }
static int  desc_get_hs(const void *st) { return ((const pong_t *)st)->high_score; }
static void desc_set_hs(void *st, int v) { ((pong_t *)st)->high_score = v; }
static sfx_queue_t *desc_sfx(void *st) { return &((pong_t *)st)->sfx; }

const game_desc_t GAME_PONG = {
    .name = "PONG", .players = 2, .state = &pong_state,
    .init = desc_init, .start = desc_start, .update = desc_update,
    .render = desc_render, .autoplay = desc_autoplay, .is_over = desc_is_over,
    .get_high_score = desc_get_hs, .set_high_score = desc_set_hs, .sfx = desc_sfx,
    .track = MUSIC_PONG,
};
