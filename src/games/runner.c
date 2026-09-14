#include "runner.h"
#include <string.h>

enum { RUN_FIRST_W = 240, RUN_SPAWN_AHEAD = 64, RUN_DESPAWN_PAD = 16 };

static int find_slot(const runner_t *g) {
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++)
        if (!g->buildings[i].alive) return i;
    return -1;
}

static const run_building_t *rightmost(const runner_t *g) {
    const run_building_t *best = 0;
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) {
        if (!g->buildings[i].alive) continue;
        if (!best || g->buildings[i].x > best->x) best = &g->buildings[i];
    }
    return best;
}

static int b_left(const run_building_t *b) { return (int)(b->x >> 8); }
static int b_right(const run_building_t *b) { return (int)(b->x >> 8) + b->w; }

static int horiz_overlap(const run_building_t *b) {
    int bx = b_left(b);
    return RUN_PLAYER_X < bx + b->w && bx < RUN_PLAYER_X + RUN_PLAYER_W;
}

static void place(runner_t *g, int slot, int32_t x, int w, int roof) {
    run_building_t *b = &g->buildings[slot];
    b->alive = true;
    b->x = x;
    b->w = w;
    b->roof = roof;
}

static int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void spawn_next(runner_t *g) {
    int slot = find_slot(g);
    if (slot < 0) return;
    const run_building_t *last = rightmost(g);
    if (!last) return;
    int w = RUN_W_MIN + (int)prng_below(&g->rng, (uint32_t)RUN_W_RAND);
    int gap = RUN_GAP_MIN + (int)prng_below(&g->rng, (uint32_t)RUN_GAP_RAND);
    int lo = clamp_int(last->roof - RUN_ROOF_DELTA, RUN_ROOF_MIN, RUN_ROOF_MAX);
    int hi = clamp_int(last->roof + RUN_ROOF_DELTA, RUN_ROOF_MIN, RUN_ROOF_MAX);
    int roof = lo + (int)prng_below(&g->rng, (uint32_t)(hi - lo + 1));
    int32_t x = last->x + ((int32_t)(last->w + gap) << 8);
    place(g, slot, x, w, roof);
}

static void fill_buildings(runner_t *g) {
    for (;;) {
        const run_building_t *last = rightmost(g);
        if (!last) return;
        if (b_right(last) >= PLAY_X1 + RUN_SPAWN_AHEAD) return;
        if (find_slot(g) < 0) return;
        spawn_next(g);
    }
}

static void despawn(runner_t *g) {
    int limit = PLAY_X0 - RUN_DESPAWN_PAD;
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) {
        run_building_t *b = &g->buildings[i];
        if (!b->alive) continue;
        if (b_right(b) <= limit) b->alive = false;
    }
}

static void run_game_over(runner_t *g) {
    if (g->state != RUN_ST_PLAY) return;
    if (g->score > g->high_score) g->high_score = g->score;
    sfx_push(&g->sfx, SFX_EXPLODE);
    g->state = RUN_ST_GAMEOVER;
}

static int player_bottom(const runner_t *g) {
    return (int)(g->py >> 8) + RUN_PLAYER_H;
}

static void check_walls(runner_t *g, int32_t old_speed) {
    int pb = player_bottom(g);
    int pright = RUN_PLAYER_X + RUN_PLAYER_W;
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) {
        const run_building_t *b = &g->buildings[i];
        if (!b->alive) continue;
        int new_bx = b_left(b);
        int old_bx = (int)((b->x + old_speed) >> 8);
        if (old_bx >= pright && new_bx < pright && pb > b->roof + 2) {
            run_game_over(g);
            return;
        }
    }
}

static int building_under(const runner_t *g, int *roof_out) {
    int found = 0;
    int roof = 0;
    int pb = player_bottom(g);
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) {
        const run_building_t *b = &g->buildings[i];
        if (!b->alive || !horiz_overlap(b)) continue;
        if (!found || b->roof == pb) {
            found = 1;
            roof = b->roof;
        }
    }
    if (roof_out) *roof_out = roof;
    return found;
}

static void try_land(runner_t *g, int old_bottom, int new_bottom) {
    if (g->vy <= 0) return;
    int best = -1;
    int best_roof = 0x7fffffff;
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) {
        const run_building_t *b = &g->buildings[i];
        if (!b->alive || !horiz_overlap(b)) continue;
        if (old_bottom <= b->roof && new_bottom >= b->roof) {
            if (b->roof < best_roof) {
                best_roof = b->roof;
                best = i;
            }
        }
    }
    if (best < 0) return;
    g->py = (int32_t)(g->buildings[best].roof - RUN_PLAYER_H) << 8;
    g->vy = 0;
    g->grounded = true;
    sfx_push(&g->sfx, SFX_HIT);
}

static void play_tick(runner_t *g, const input_t *in) {
    int32_t spd = g->speed;
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) {
        if (g->buildings[i].alive) g->buildings[i].x -= spd;
    }

    g->distance += spd;
    g->score = (int)(g->distance >> 8) / RUN_METRE;
    if (g->score / 100 > g->last_point_score / 100)
        sfx_push(&g->sfx, SFX_POINT);
    g->last_point_score = g->score;

    g->speed_ticks++;
    if (g->speed_ticks >= RUN_SPEED_STEP_TICKS) {
        g->speed_ticks = 0;
        g->speed += RUN_SPEED_STEP;
        if (g->speed > RUN_SPEED_MAX) g->speed = RUN_SPEED_MAX;
    }

    check_walls(g, spd);
    if (g->state != RUN_ST_PLAY) return;

    if (g->grounded) {
        int roof;
        if (!building_under(g, &roof)) {
            g->grounded = false;
        } else {
            g->py = (int32_t)(roof - RUN_PLAYER_H) << 8;
            g->vy = 0;
        }
    }

    int jumped = 0;
    if (g->grounded && in->a) {
        g->vy = RUN_JUMP_VY;
        g->grounded = false;
        sfx_push(&g->sfx, SFX_MERGE);
        g->autoplay_hold = RUN_AUTOPLAY_HOLD;
        jumped = 1;
    }

    if (!g->grounded) {
        if (!jumped) {
            g->vy += RUN_GRAVITY;
            if (!in->a_held && g->vy < RUN_JUMP_CUT_VY)
                g->vy = RUN_JUMP_CUT_VY;
        }
        int old_bottom = player_bottom(g);
        g->py += g->vy;
        int new_bottom = player_bottom(g);
        try_land(g, old_bottom, new_bottom);
        if (g->state == RUN_ST_PLAY && player_bottom(g) >= SCREEN_H)
            run_game_over(g);
    }

    if (g->autoplay_hold > 0) g->autoplay_hold--;

    despawn(g);
    fill_buildings(g);
}

void runner_init(runner_t *g) {
    memset(g, 0, sizeof *g);
    g->state = RUN_ST_TITLE;
}

void runner_start(runner_t *g, uint32_t seed) {
    int hs = g->high_score;
    memset(g, 0, sizeof *g);
    g->high_score = hs;
    g->state = RUN_ST_TITLE;
    prng_seed(&g->rng, seed);
    g->speed = RUN_SPEED_START;
    place(g, 0, (int32_t)PLAY_X0 << 8, RUN_FIRST_W, 180);
    g->py = (int32_t)(180 - RUN_PLAYER_H) << 8;
    g->vy = 0;
    g->grounded = true;
    fill_buildings(g);
}

void runner_update(runner_t *g, const input_t in[GAME_MAX_PLAYERS]) {
    g->ticks++;
    switch (g->state) {
    case RUN_ST_TITLE:
        if (in[0].a) g->state = RUN_ST_PLAY;
        break;
    case RUN_ST_PLAY:
        play_tick(g, &in[0]);
        break;
    case RUN_ST_GAMEOVER:
        if (in[0].a) runner_start(g, g->ticks);
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

void runner_render(const runner_t *g, const draw_t *d) {
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);

    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) {
        const run_building_t *b = &g->buildings[i];
        if (!b->alive) continue;
        int x0 = b_left(b);
        clip_rect(d, x0, b->roof, x0 + b->w, SCREEN_H, COLOR_DARK);
    }

    int y = (int)(g->py >> 8);
    clip_rect(d, RUN_PLAYER_X, y, RUN_PLAYER_X + RUN_PLAYER_W, y + RUN_PLAYER_H, COLOR_RED);

    char buf[24];
    fmt_label(buf, sizeof buf, "SCORE", g->score);
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, PLAY_X0, HUD_BASELINE, DRAW_TEXT_DARK, buf);
    fmt_label(buf, sizeof buf, "HIGH", g->high_score);
    draw_text(d, DRAW_FONT_HUD, DRAW_RIGHT, PLAY_X1, HUD_BASELINE, DRAW_TEXT_DARK, buf);

    if (g->state == RUN_ST_TITLE) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 100, DRAW_TEXT_DARK, "RUNNER");
        fmt_label(buf, sizeof buf, "HIGH SCORE", g->high_score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 130, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 160, DRAW_TEXT_DARK, "PRESS A");
    } else if (g->state == RUN_ST_GAMEOVER) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 140, DRAW_TEXT_DARK, "GAME OVER");
        fmt_label(buf, sizeof buf, "SCORE", g->score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 168, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 196, DRAW_TEXT_DARK, "PRESS A");
    }
}

void runner_autoplay(const runner_t *g, input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
    if (g->state == RUN_ST_TITLE || g->state == RUN_ST_GAMEOVER) {
        in[0].a = true;
        return;
    }
    if (g->state != RUN_ST_PLAY) return;
    if (g->autoplay_hold > 0) in[0].a_held = true;
    if (g->score >= RUN_AUTOPLAY_STOP) return;
    if (!g->grounded) return;

    int end = -1;
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) {
        const run_building_t *b = &g->buildings[i];
        if (!b->alive || !horiz_overlap(b)) continue;
        end = b_right(b);
    }
    if (end < 0) return;
    int look = (int)((g->speed * 8) >> 8);
    if (end - RUN_PLAYER_X <= look) in[0].a = true;
}

static runner_t runner_state;
static void desc_init(void *st) { runner_init(st); }
static void desc_start(void *st, uint32_t seed) { runner_start(st, seed); }
static void desc_update(void *st, const input_t in[GAME_MAX_PLAYERS]) { runner_update(st, in); }
static void desc_render(const void *st, const draw_t *d) { runner_render(st, d); }
static void desc_autoplay(const void *st, input_t in[GAME_MAX_PLAYERS]) { runner_autoplay(st, in); }
static bool desc_is_over(const void *st) { return ((const runner_t *)st)->state == RUN_ST_GAMEOVER; }
static int  desc_get_hs(const void *st) { return ((const runner_t *)st)->high_score; }
static void desc_set_hs(void *st, int v) { ((runner_t *)st)->high_score = v; }
static sfx_queue_t *desc_sfx(void *st) { return &((runner_t *)st)->sfx; }

const game_desc_t GAME_RUNNER = {
    .name = "RUNNER", .players = 1, .state = &runner_state,
    .init = desc_init, .start = desc_start, .update = desc_update,
    .render = desc_render, .autoplay = desc_autoplay, .is_over = desc_is_over,
    .get_high_score = desc_get_hs, .set_high_score = desc_set_hs, .sfx = desc_sfx,
    .track = MUSIC_RUNNER,
};
