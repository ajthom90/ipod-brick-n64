#include "hopper.h"
#include <string.h>

static int imin(int a, int b) { return a < b ? a : b; }

static int find_slot(const hopper_t *g) {
    for (int i = 0; i < HOP_MAX_PLATS; i++)
        if (!g->plats[i].alive) return i;
    return -1;
}

static void sort_plats(hopper_t *g) {
    for (int i = 0; i < HOP_MAX_PLATS; i++) {
        for (int j = i + 1; j < HOP_MAX_PLATS; j++) {
            if (!g->plats[j].alive) continue;
            if (!g->plats[i].alive || g->plats[j].y < g->plats[i].y) {
                hop_plat_t tmp = g->plats[i];
                g->plats[i] = g->plats[j];
                g->plats[j] = tmp;
            }
        }
    }
}

static hop_plat_type_t roll_type(hopper_t *g) {
    uint32_t r = prng_below(&g->rng, 10);
    if (r < 7) return HOP_PLAT_STATIC;
    if (r < 9) return HOP_PLAT_MOVING;
    return HOP_PLAT_SPRING;
}

static void place_plat(hopper_t *g, int slot, int x, int y, hop_plat_type_t type) {
    hop_plat_t *p = &g->plats[slot];
    p->alive = true;
    p->x = x;
    p->y = y;
    p->type = type;
    p->dir = (type == HOP_PLAT_MOVING) ? 1 : 0;
}

void hopper_generate_up_to(hopper_t *g, int world_y) {
    while (g->top_y > world_y) {
        int slot = find_slot(g);
        if (slot < 0) break;
        int extra = imin(HOP_GAP_EXTRA_CAP, g->score / 50);
        int gap = HOP_GAP_MIN + (int)prng_below(&g->rng, (uint32_t)HOP_GAP_RAND) + extra;
        int xspan = PLAY_X1 - PLAY_X0 - HOP_PLAT_W;
        int x = PLAY_X0 + (int)prng_below(&g->rng, (uint32_t)xspan);
        hop_plat_type_t type = roll_type(g);
        int y = g->top_y - gap;
        place_plat(g, slot, x, y, type);
        g->top_y = y;
    }
    sort_plats(g);
}

static void recycle_below(hopper_t *g) {
    int limit = g->camera_y + SCREEN_H + 20;
    for (int i = 0; i < HOP_MAX_PLATS; i++) {
        if (!g->plats[i].alive) continue;
        if (g->plats[i].y >= limit) g->plats[i].alive = false;
    }
}

static void hop_game_over(hopper_t *g) {
    if (g->state != HOP_ST_PLAY) return;
    if (g->score > g->high_score) g->high_score = g->score;
    sfx_push(&g->sfx, SFX_GAME_OVER);
    g->state = HOP_ST_GAMEOVER;
}

static int horiz_overlap(int px, int plat_x) {
    return px < plat_x + HOP_PLAT_W && plat_x < px + HOP_PLAYER;
}

static void wrap_player_x(hopper_t *g) {
    int x = g->px >> 8;
    if (x < PLAY_X0) x = PLAY_X1 - HOP_PLAYER;
    else if (x + HOP_PLAYER > PLAY_X1) x = PLAY_X0;
    g->px = (int32_t)x << 8;
}

static int move_dx(const input_t *in) {
    if (in->stick_x != 0)
        return (in->stick_x * HOP_MOVE_ANALOG_MAX) / 256;
    return in->dpad_x * HOP_MOVE_DIGITAL;
}

static void land_on(hopper_t *g, const hop_plat_t *p) {
    g->py = (int32_t)(p->y - HOP_PLAYER) << 8;
    if (p->type == HOP_PLAT_SPRING) {
        g->vy = HOP_SPRING_VY;
        sfx_push(&g->sfx, SFX_CLEAR);
    } else {
        g->vy = HOP_JUMP_VY;
        sfx_push(&g->sfx, SFX_BOUNCE);
    }
}

static void try_land(hopper_t *g, int old_bottom, int new_bottom) {
    if (g->vy <= 0) return;
    int px = g->px >> 8;
    int best = -1;
    int best_y = 0x7fffffff;
    for (int i = 0; i < HOP_MAX_PLATS; i++) {
        const hop_plat_t *p = &g->plats[i];
        if (!p->alive) continue;
        if (old_bottom <= p->y && new_bottom >= p->y && horiz_overlap(px, p->x)) {
            if (p->y < best_y) {
                best_y = p->y;
                best = i;
            }
        }
    }
    if (best >= 0) land_on(g, &g->plats[best]);
}

static void move_moving_plats(hopper_t *g) {
    for (int i = 0; i < HOP_MAX_PLATS; i++) {
        hop_plat_t *p = &g->plats[i];
        if (!p->alive || p->type != HOP_PLAT_MOVING) continue;
        p->x += p->dir * HOP_MOVING_SPEED;
        if (p->x <= PLAY_X0) {
            p->x = PLAY_X0;
            p->dir = 1;
        } else if (p->x + HOP_PLAT_W >= PLAY_X1) {
            p->x = PLAY_X1 - HOP_PLAT_W;
            p->dir = -1;
        }
    }
}

static void update_camera_score(hopper_t *g) {
    int screen_y = (g->py >> 8) - g->camera_y;
    if (screen_y < HOP_CAMERA_LINE)
        g->camera_y = (g->py >> 8) - HOP_CAMERA_LINE;
    g->score = (g->camera_start - g->camera_y) / 10;
}

static void play_tick(hopper_t *g, const input_t *in) {
    g->px += (int32_t)move_dx(in) << 8;
    wrap_player_x(g);

    g->vy += HOP_GRAVITY;
    int old_bottom = (g->py >> 8) + HOP_PLAYER;
    g->py += g->vy;
    int new_bottom = (g->py >> 8) + HOP_PLAYER;
    try_land(g, old_bottom, new_bottom);

    move_moving_plats(g);
    update_camera_score(g);
    recycle_below(g);
    hopper_generate_up_to(g, g->camera_y - 40);

    if ((g->py >> 8) - g->camera_y > SCREEN_H) hop_game_over(g);
}

void hopper_init(hopper_t *g) {
    memset(g, 0, sizeof *g);
    g->state = HOP_ST_TITLE;
}

void hopper_start(hopper_t *g, uint32_t seed) {
    int hs = g->high_score;
    memset(g, 0, sizeof *g);
    g->high_score = hs;
    g->state = HOP_ST_TITLE;
    prng_seed(&g->rng, seed);

    int plat_x = 160 - HOP_PLAT_W / 2;
    int plat_y = 212;
    place_plat(g, 0, plat_x, plat_y, HOP_PLAT_STATIC);
    g->top_y = plat_y;
    g->px = (int32_t)(160 - HOP_PLAYER / 2) << 8;
    g->py = (int32_t)(plat_y - HOP_PLAYER) << 8;
    g->vy = 0;
    g->camera_y = 0;
    g->camera_start = 0;
    hopper_generate_up_to(g, g->camera_y - 40);
}

void hopper_update(hopper_t *g, const input_t in[GAME_MAX_PLAYERS]) {
    g->ticks++;
    switch (g->state) {
    case HOP_ST_TITLE:
        if (in[0].a) g->state = HOP_ST_PLAY;
        break;
    case HOP_ST_PLAY:
        play_tick(g, &in[0]);
        break;
    case HOP_ST_GAMEOVER:
        if (in[0].a) hopper_start(g, g->ticks);
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

static uint32_t plat_color(hop_plat_type_t t) {
    if (t == HOP_PLAT_MOVING) return COLOR_BLUE;
    if (t == HOP_PLAT_SPRING) return COLOR_ORANGE;
    return COLOR_GREEN;
}

void hopper_render(const hopper_t *g, const draw_t *d) {
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);

    for (int i = 0; i < HOP_MAX_PLATS; i++) {
        const hop_plat_t *p = &g->plats[i];
        if (!p->alive) continue;
        int sy = p->y - g->camera_y;
        clip_rect(d, p->x, sy, p->x + HOP_PLAT_W, sy + HOP_PLAT_H, plat_color(p->type));
    }

    int px = g->px >> 8;
    int py = (g->py >> 8) - g->camera_y;
    clip_rect(d, px, py, px + HOP_PLAYER, py + HOP_PLAYER, COLOR_DARK);
    clip_rect(d, px + 7, py + 3, px + 10, py + 6, COLOR_BG);

    char buf[24];
    fmt_label(buf, sizeof buf, "SCORE", g->score);
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, PLAY_X0, HUD_BASELINE, DRAW_TEXT_DARK, buf);
    fmt_label(buf, sizeof buf, "HIGH", g->high_score);
    draw_text(d, DRAW_FONT_HUD, DRAW_RIGHT, PLAY_X1, HUD_BASELINE, DRAW_TEXT_DARK, buf);

    if (g->state == HOP_ST_TITLE) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 100, DRAW_TEXT_DARK, "HOPPER");
        fmt_label(buf, sizeof buf, "HIGH SCORE", g->high_score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 130, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 160, DRAW_TEXT_DARK, "PRESS A");
    } else if (g->state == HOP_ST_GAMEOVER) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 140, DRAW_TEXT_DARK, "GAME OVER");
        fmt_label(buf, sizeof buf, "SCORE", g->score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 168, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 196, DRAW_TEXT_DARK, "PRESS A");
    }
}

static int wrap_dx(int from, int to) {
    int dx = to - from;
    int span = PLAY_X1 - PLAY_X0;
    if (dx > span / 2) dx -= span;
    if (dx < -span / 2) dx += span;
    return dx;
}

static int remain_up_px(int32_t vy) {
    if (vy >= 0) return 0;
    int32_t v = -vy;
    return (int)(((v * v) / (HOP_GRAVITY * 2)) >> 8);
}

void hopper_autoplay(const hopper_t *g, input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
    if (g->state == HOP_ST_TITLE || g->state == HOP_ST_GAMEOVER) {
        in[0].a = true;
        return;
    }
    if (g->state != HOP_ST_PLAY) return;
    if (g->score >= HOP_AUTOPLAY_STOP) return;

    int pbottom = (g->py >> 8) + HOP_PLAYER;
    int pc = (g->px >> 8) + HOP_PLAYER / 2;
    int remain = remain_up_px(g->vy);
    int best_i = -1;
    int best_pri = 0x7fffffff;
    for (int i = 0; i < HOP_MAX_PLATS; i++) {
        const hop_plat_t *p = &g->plats[i];
        if (!p->alive) continue;
        int above = pbottom - p->y;
        if (above < -90 || above > 130) continue;
        int tc = p->x + HOP_PLAT_W / 2;
        if (p->type == HOP_PLAT_MOVING) tc += p->dir * 8;
        int dx = wrap_dx(pc, tc);
        int adx = dx < 0 ? -dx : dx;
        int pri;
        if (above > 0 && above <= remain + 8)
            pri = above * 1000 + adx;
        else if (above <= 0)
            pri = 100000 + (-above) * 1000 + adx;
        else
            continue;
        if (pri < best_pri) {
            best_pri = pri;
            best_i = i;
        }
    }
    if (best_i < 0) return;
    const hop_plat_t *p = &g->plats[best_i];
    int tc = p->x + HOP_PLAT_W / 2;
    if (p->type == HOP_PLAT_MOVING) tc += p->dir * 8;
    int dx = wrap_dx(pc, tc);
    if (dx > 2) in[0].stick_x = 256;
    else if (dx < -2) in[0].stick_x = -256;
}

static hopper_t hopper_state;
static void desc_init(void *st) { hopper_init(st); }
static void desc_start(void *st, uint32_t seed) { hopper_start(st, seed); }
static void desc_update(void *st, const input_t in[GAME_MAX_PLAYERS]) { hopper_update(st, in); }
static void desc_render(const void *st, const draw_t *d) { hopper_render(st, d); }
static void desc_autoplay(const void *st, input_t in[GAME_MAX_PLAYERS]) { hopper_autoplay(st, in); }
static bool desc_is_over(const void *st) { return ((const hopper_t *)st)->state == HOP_ST_GAMEOVER; }
static int  desc_get_hs(const void *st) { return ((const hopper_t *)st)->high_score; }
static void desc_set_hs(void *st, int v) { ((hopper_t *)st)->high_score = v; }
static sfx_queue_t *desc_sfx(void *st) { return &((hopper_t *)st)->sfx; }

const game_desc_t GAME_HOPPER = {
    .name = "HOPPER", .players = 1, .state = &hopper_state,
    .init = desc_init, .start = desc_start, .update = desc_update,
    .render = desc_render, .autoplay = desc_autoplay, .is_over = desc_is_over,
    .get_high_score = desc_get_hs, .set_high_score = desc_set_hs, .sfx = desc_sfx,
    .track = MUSIC_HOPPER,
};
