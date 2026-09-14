#include "flap.h"
#include <string.h>

int flap_gap_for_score(int score) {
    int gap = FLP_GAP - score / 10;
    if (gap < FLP_GAP_MIN) gap = FLP_GAP_MIN;
    return gap;
}

static int find_slot(const flap_t *g) {
    for (int i = 0; i < FLP_MAX_PIPES; i++)
        if (!g->pipes[i].alive) return i;
    return -1;
}

static const flp_pipe_t *rightmost_pipe(const flap_t *g) {
    const flp_pipe_t *best = 0;
    for (int i = 0; i < FLP_MAX_PIPES; i++) {
        if (!g->pipes[i].alive) continue;
        if (!best || g->pipes[i].x > best->x) best = &g->pipes[i];
    }
    return best;
}

static void spawn_pipe(flap_t *g) {
    int slot = find_slot(g);
    if (slot < 0) return;
    flp_pipe_t *p = &g->pipes[slot];
    p->alive = true;
    p->scored = false;
    p->x = (int32_t)PLAY_X1 << 8;
    p->center = FLP_CENTER_MIN + (int)prng_below(&g->rng, (uint32_t)FLP_CENTER_RAND);
    p->gap = flap_gap_for_score(g->score);
}

static void maybe_spawn(flap_t *g) {
    const flp_pipe_t *last = rightmost_pipe(g);
    if (last && (last->x >> 8) > PLAY_X1 - FLP_PIPE_SPACING) return;
    spawn_pipe(g);
}

static void flp_game_over(flap_t *g) {
    if (g->state != FLP_ST_PLAY) return;
    if (g->score > g->high_score) g->high_score = g->score;
    sfx_push(&g->sfx, SFX_EXPLODE);
    g->state = FLP_ST_GAMEOVER;
}

static int rects_overlap(int ax0, int ay0, int ax1, int ay1,
                         int bx0, int by0, int bx1, int by1) {
    return ax0 < bx1 && bx0 < ax1 && ay0 < by1 && by0 < ay1;
}

static void scroll_pipes(flap_t *g) {
    for (int i = 0; i < FLP_MAX_PIPES; i++) {
        flp_pipe_t *p = &g->pipes[i];
        if (!p->alive) continue;
        p->x -= FLP_SCROLL;
        if ((p->x >> 8) + FLP_PIPE_W <= PLAY_X0) p->alive = false;
    }
}

static void score_pipes(flap_t *g) {
    for (int i = 0; i < FLP_MAX_PIPES; i++) {
        flp_pipe_t *p = &g->pipes[i];
        if (!p->alive || p->scored) continue;
        if ((p->x >> 8) + FLP_PIPE_W <= FLP_BIRD_X) {
            p->scored = true;
            g->score++;
            sfx_push(&g->sfx, SFX_POINT);
        }
    }
}

static void collide(flap_t *g) {
    int bx0 = FLP_BIRD_X;
    int by0 = (int)(g->by >> 8);
    int bx1 = bx0 + FLP_BIRD;
    int by1 = by0 + FLP_BIRD;
    for (int i = 0; i < FLP_MAX_PIPES; i++) {
        const flp_pipe_t *p = &g->pipes[i];
        if (!p->alive) continue;
        int px = (int)(p->x >> 8);
        int top1 = p->center - p->gap / 2;
        int bot0 = p->center + p->gap / 2;
        if (rects_overlap(bx0, by0, bx1, by1, px, PLAY_Y0, px + FLP_PIPE_W, top1) ||
            rects_overlap(bx0, by0, bx1, by1, px, bot0, px + FLP_PIPE_W, FLP_GROUND_Y)) {
            flp_game_over(g);
            return;
        }
    }
    if (by1 >= FLP_GROUND_Y) flp_game_over(g);
}

static void play_tick(flap_t *g, const input_t *in) {
    g->vy += FLP_GRAVITY;
    if (g->vy > FLP_TERMINAL) g->vy = FLP_TERMINAL;
    if (in->a) {
        g->vy = FLP_FLAP_VY;
        sfx_push(&g->sfx, SFX_HIT);
    }
    g->by += g->vy;
    if (g->by < ((int32_t)PLAY_Y0 << 8)) {
        g->by = (int32_t)PLAY_Y0 << 8;
        g->vy = 0;
    }

    scroll_pipes(g);
    maybe_spawn(g);
    score_pipes(g);
    collide(g);
}

void flap_init(flap_t *g) {
    memset(g, 0, sizeof *g);
    g->state = FLP_ST_TITLE;
}

void flap_start(flap_t *g, uint32_t seed) {
    int hs = g->high_score;
    memset(g, 0, sizeof *g);
    g->high_score = hs;
    g->state = FLP_ST_TITLE;
    prng_seed(&g->rng, seed);
    g->by = 90 << 8;
    g->vy = 0;
}

void flap_update(flap_t *g, const input_t in[GAME_MAX_PLAYERS]) {
    g->ticks++;
    switch (g->state) {
    case FLP_ST_TITLE:
        if (in[0].a) {
            g->state = FLP_ST_PLAY;
            play_tick(g, &in[0]);
        }
        break;
    case FLP_ST_PLAY:
        play_tick(g, &in[0]);
        break;
    case FLP_ST_GAMEOVER:
        if (in[0].a) flap_start(g, g->ticks);
        break;
    }
}

static void clip_rect(const draw_t *d, int x0, int y0, int x1, int y1, uint32_t rgb) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > SCREEN_W) x1 = SCREEN_W;
    if (y1 > SCREEN_H) y1 = SCREEN_H;
    if (x0 < x1 && y0 < y1) draw_rect(d, x0, y0, x1, y1, rgb);
}

void flap_render(const flap_t *g, const draw_t *d) {
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);

    for (int i = 0; i < FLP_MAX_PIPES; i++) {
        const flp_pipe_t *p = &g->pipes[i];
        if (!p->alive) continue;
        int px = (int)(p->x >> 8);
        int top1 = p->center - p->gap / 2;
        int bot0 = p->center + p->gap / 2;
        clip_rect(d, px, PLAY_Y0, px + FLP_PIPE_W, top1, COLOR_GREEN);
        clip_rect(d, px, bot0, px + FLP_PIPE_W, FLP_GROUND_Y, COLOR_GREEN);
    }

    int y = (int)(g->by >> 8);
    clip_rect(d, FLP_BIRD_X, y, FLP_BIRD_X + FLP_BIRD, y + FLP_BIRD, COLOR_YELLOW);
    clip_rect(d, FLP_BIRD_X + 7, y + 3, FLP_BIRD_X + 9, y + 5, COLOR_DARK);

    draw_rect(d, PLAY_X0, FLP_GROUND_Y, PLAY_X1, PLAY_Y1, COLOR_DARK);

    char buf[24];
    fmt_label(buf, sizeof buf, "SCORE", g->score);
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, PLAY_X0, HUD_BASELINE, DRAW_TEXT_DARK, buf);
    fmt_label(buf, sizeof buf, "HIGH", g->high_score);
    draw_text(d, DRAW_FONT_HUD, DRAW_RIGHT, PLAY_X1, HUD_BASELINE, DRAW_TEXT_DARK, buf);

    if (g->state == FLP_ST_TITLE) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 100, DRAW_TEXT_DARK, "FLAP");
        fmt_label(buf, sizeof buf, "HIGH SCORE", g->high_score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 130, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 160, DRAW_TEXT_DARK, "PRESS A");
    } else if (g->state == FLP_ST_GAMEOVER) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 140, DRAW_TEXT_DARK, "GAME OVER");
        fmt_label(buf, sizeof buf, "SCORE", g->score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 168, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 196, DRAW_TEXT_DARK, "PRESS A");
    }
}

void flap_autoplay(const flap_t *g, input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
    if (g->state == FLP_ST_TITLE || g->state == FLP_ST_GAMEOVER) {
        in[0].a = true;
        return;
    }
    if (g->state != FLP_ST_PLAY) return;
    if (g->score >= FLP_AUTOPLAY_STOP) return;
    if (g->vy < 0) return;

    const flp_pipe_t *next = 0;
    int32_t best_x = 0x7fffffff;
    for (int i = 0; i < FLP_MAX_PIPES; i++) {
        const flp_pipe_t *p = &g->pipes[i];
        if (!p->alive) continue;
        if ((p->x >> 8) + FLP_PIPE_W <= FLP_BIRD_X) continue;
        if (p->x < best_x) {
            best_x = p->x;
            next = p;
        }
    }

    int bird_c = (int)(g->by >> 8) + FLP_BIRD / 2;
    int bird_b = (int)(g->by >> 8) + FLP_BIRD;
    int target = next ? next->center : (PLAY_Y0 + FLP_GROUND_Y) / 2;
    int gap_bot = next ? next->center + next->gap / 2 : FLP_GROUND_Y;
    int px = next ? (int)(next->x >> 8) : PLAY_X1;
    int in_column = next && px < FLP_BIRD_X + FLP_BIRD && px + FLP_PIPE_W > FLP_BIRD_X;

    if (bird_b >= gap_bot - 6 || bird_b >= FLP_GROUND_Y - 12) {
        in[0].a = true;
        return;
    }
    if (in_column) return;
    if (bird_c > target + 22) in[0].a = true;
}

static flap_t flap_state;
static void desc_init(void *st) { flap_init(st); }
static void desc_start(void *st, uint32_t seed) { flap_start(st, seed); }
static void desc_update(void *st, const input_t in[GAME_MAX_PLAYERS]) { flap_update(st, in); }
static void desc_render(const void *st, const draw_t *d) { flap_render(st, d); }
static void desc_autoplay(const void *st, input_t in[GAME_MAX_PLAYERS]) { flap_autoplay(st, in); }
static bool desc_is_over(const void *st) { return ((const flap_t *)st)->state == FLP_ST_GAMEOVER; }
static int  desc_get_hs(const void *st) { return ((const flap_t *)st)->high_score; }
static void desc_set_hs(void *st, int v) { ((flap_t *)st)->high_score = v; }
static sfx_queue_t *desc_sfx(void *st) { return &((flap_t *)st)->sfx; }

const game_desc_t GAME_FLAP = {
    .name = "FLAP", .players = 1, .state = &flap_state,
    .init = desc_init, .start = desc_start, .update = desc_update,
    .render = desc_render, .autoplay = desc_autoplay, .is_over = desc_is_over,
    .get_high_score = desc_get_hs, .set_high_score = desc_set_hs, .sfx = desc_sfx,
    .track = MUSIC_FLAP,
};
