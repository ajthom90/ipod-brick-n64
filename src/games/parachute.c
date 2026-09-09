#include "parachute.h"
#include <string.h>

const int16_t PAR_SIN[31] = { -247,-241,-232,-222,-210,-196,-181,-165,-147,-128,-108,-88,-66,-44,-22, 0,
                               22, 44, 66, 88, 108, 128, 147, 165, 181, 196, 210, 222, 232, 241, 247 };
const int16_t PAR_COS[31] = {   66,  88, 108, 128, 147, 165, 181, 196, 210, 222, 232, 241, 247, 252, 255, 256,
                               255, 252, 247, 241, 232, 222, 210, 196, 181, 165, 147, 128, 108, 88, 66 };

int par_aim_index(int deg) {
    if (deg < PAR_AIM_MIN) deg = PAR_AIM_MIN;
    if (deg > PAR_AIM_MAX) deg = PAR_AIM_MAX;
    if (deg >= 0) deg = ((deg + 2) / 5) * 5;
    else deg = ((deg - 2) / 5) * 5;
    return (deg + 75) / 5;
}

static int imax(int a, int b) { return a > b ? a : b; }

static int rects_overlap(int ax0, int ay0, int ax1, int ay1,
                         int bx0, int by0, int bx1, int by1) {
    return ax0 < bx1 && bx0 < ax1 && ay0 < by1 && by0 < ay1;
}

static void bullet_rect(const par_bullet_t *b, int *x0, int *y0, int *x1, int *y1) {
    int px = (int)(b->x >> 8);
    int py = (int)(b->y >> 8);
    *x0 = px - 1;
    *y0 = py - 1;
    *x1 = px + 2;
    *y1 = py + 2;
}

static void aim_dir(int aim, int32_t *dx, int32_t *dy) {
    int k = par_aim_index(aim);
    *dx = PAR_SIN[k];
    *dy = -(int32_t)PAR_COS[k];
}

static int barrel_pos(int origin, int k, int32_t d) {
    return origin + (int)((6 * k * d) >> 8);
}

static int count_alive_bullets(const parachute_t *g) {
    int n = 0;
    for (int i = 0; i < PAR_MAX_BULLETS; i++) if (g->bullets[i].alive) n++;
    return n;
}

static void par_game_over(parachute_t *g) {
    if (g->state != PAR_ST_PLAY) return;
    if (g->score > g->high_score) g->high_score = g->score;
    sfx_push(&g->sfx, SFX_GAME_OVER);
    g->state = PAR_ST_GAMEOVER;
}

static void fire(parachute_t *g) {
    if (count_alive_bullets(g) >= PAR_MAX_BULLETS) return;
    int slot = -1;
    for (int i = 0; i < PAR_MAX_BULLETS; i++) {
        if (!g->bullets[i].alive) { slot = i; break; }
    }
    if (slot < 0) return;
    int32_t dx, dy;
    aim_dir(g->aim, &dx, &dy);
    par_bullet_t *b = &g->bullets[slot];
    b->alive = true;
    b->x = (int32_t)barrel_pos(160, 4, dx) << 8;
    b->y = (int32_t)barrel_pos(220, 4, dy) << 8;
    b->vx = dx * PAR_BULLET_SPEED;
    b->vy = dy * PAR_BULLET_SPEED;
    if (g->score > 0) g->score--;
    sfx_push(&g->sfx, SFX_SHOT);
    g->fire_cooldown = PAR_FIRE_HOLD_TICKS;
}

static void bump_high(parachute_t *g) {
    if (g->score > g->high_score) g->high_score = g->score;
}

static void spawn_heli(parachute_t *g) {
    int slot = -1;
    for (int i = 0; i < PAR_MAX_HELIS; i++) {
        if (!g->helis[i].alive) { slot = i; break; }
    }
    if (slot < 0) return;
    par_heli_t *h = &g->helis[slot];
    h->alive = true;
    if (prng_below(&g->rng, 2) == 0) {
        h->x = 0;
        h->dir = 1;
    } else {
        h->x = 304;
        h->dir = -1;
    }
    h->y = 40 + (int)prng_below(&g->rng, 51);
    h->drops_left = 2;
}

static void drop_trooper(parachute_t *g, const par_heli_t *h) {
    for (int i = 0; i < PAR_MAX_TROOPERS; i++) {
        if (g->troopers[i].alive) continue;
        g->troopers[i].alive = true;
        g->troopers[i].chute = true;
        g->troopers[i].x = h->x + 5;
        g->troopers[i].y = h->y + 6;
        return;
    }
}

static void add_landed(parachute_t *g, int x) {
    for (int i = 0; i < PAR_MAX_LANDED; i++) {
        if (g->landed[i].alive) continue;
        g->landed[i].alive = true;
        g->landed[i].x = x;
        break;
    }
    if (x < PAR_TURRET_X0) {
        g->landed_left++;
        if (g->landed_left >= PAR_SIDE_LIMIT) par_game_over(g);
    } else {
        g->landed_right++;
        if (g->landed_right >= PAR_SIDE_LIMIT) par_game_over(g);
    }
}

static int overlap_6(int ax, int bx) {
    return ax < bx + 6 && bx < ax + 6;
}

static void land_trooper(parachute_t *g, par_trooper_t *t) {
    t->alive = false;
    if (t->x + 6 > PAR_TURRET_X0 && t->x < PAR_TURRET_X1) {
        par_game_over(g);
        return;
    }
    if (t->chute) {
        add_landed(g, t->x);
        return;
    }
    g->score += 1;
    bump_high(g);
    sfx_push(&g->sfx, SFX_EXPLODE);
    for (int i = 0; i < PAR_MAX_LANDED; i++) {
        if (!g->landed[i].alive) continue;
        if (!overlap_6(t->x, g->landed[i].x)) continue;
        int lx = g->landed[i].x;
        g->landed[i].alive = false;
        if (lx < PAR_TURRET_X0) {
            if (g->landed_left > 0) g->landed_left--;
        } else {
            if (g->landed_right > 0) g->landed_right--;
        }
        g->score += 2;
        bump_high(g);
        sfx_push(&g->sfx, SFX_EXPLODE);
    }
}

static void move_bullets(parachute_t *g) {
    for (int i = 0; i < PAR_MAX_BULLETS; i++) {
        par_bullet_t *b = &g->bullets[i];
        if (!b->alive) continue;
        b->x += b->vx;
        b->y += b->vy;
        int x0, y0, x1, y1;
        bullet_rect(b, &x0, &y0, &x1, &y1);
        if (!rects_overlap(x0, y0, x1, y1, PLAY_X0, PLAY_Y0, PLAY_X1, PLAY_Y1))
            b->alive = false;
    }
}

static void move_helis(parachute_t *g) {
    for (int i = 0; i < PAR_MAX_HELIS; i++) {
        par_heli_t *h = &g->helis[i];
        if (!h->alive) continue;
        h->x += h->dir * PAR_HELI_SPEED;
        if (h->x < -16 || h->x > 320) {
            h->alive = false;
            continue;
        }
        if (h->drops_left > 0 && h->x > 40 && h->x < 280 &&
            prng_below(&g->rng, 90) == 0) {
            drop_trooper(g, h);
            h->drops_left--;
        }
    }
}

static void move_troopers(parachute_t *g) {
    for (int i = 0; i < PAR_MAX_TROOPERS; i++) {
        par_trooper_t *t = &g->troopers[i];
        if (!t->alive) continue;
        t->y += t->chute ? 1 : 4;
        if (t->y + 8 >= PAR_GROUND) land_trooper(g, t);
        if (g->state != PAR_ST_PLAY) return;
    }
}

static void bullet_hits(parachute_t *g) {
    for (int i = 0; i < PAR_MAX_BULLETS; i++) {
        par_bullet_t *b = &g->bullets[i];
        if (!b->alive) continue;
        int x0, y0, x1, y1;
        bullet_rect(b, &x0, &y0, &x1, &y1);
        int hit = 0;
        for (int h = 0; h < PAR_MAX_HELIS && !hit; h++) {
            par_heli_t *heli = &g->helis[h];
            if (!heli->alive) continue;
            if (rects_overlap(x0, y0, x1, y1, heli->x, heli->y, heli->x + 16, heli->y + 6)) {
                heli->alive = false;
                g->score += 2;
                bump_high(g);
                sfx_push(&g->sfx, SFX_EXPLODE);
                hit = 1;
            }
        }
        for (int t = 0; t < PAR_MAX_TROOPERS && !hit; t++) {
            par_trooper_t *tr = &g->troopers[t];
            if (!tr->alive) continue;
            if (rects_overlap(x0, y0, x1, y1, tr->x, tr->y, tr->x + 6, tr->y + 8)) {
                tr->alive = false;
                g->score += 2;
                bump_high(g);
                sfx_push(&g->sfx, SFX_EXPLODE);
                hit = 1;
            } else if (tr->chute &&
                       rects_overlap(x0, y0, x1, y1, tr->x - 3, tr->y - 8, tr->x + 9, tr->y - 2)) {
                tr->chute = false;
                sfx_push(&g->sfx, SFX_HIT);
                hit = 1;
            }
        }
        if (hit) b->alive = false;
    }
}

static void play_tick(parachute_t *g, const input_t *in) {
    if (g->fire_cooldown > 0) g->fire_cooldown--;
    if (in->stick_x != 0)
        g->aim += (in->stick_x * PAR_AIM_ANALOG) / 256;
    else
        g->aim += in->dpad_x * PAR_AIM_DIGITAL;
    if (g->aim > PAR_AIM_MAX) g->aim = PAR_AIM_MAX;
    if (g->aim < PAR_AIM_MIN) g->aim = PAR_AIM_MIN;

    if (in->a || (in->a_held && g->fire_cooldown == 0)) fire(g);

    move_bullets(g);
    move_helis(g);
    move_troopers(g);
    if (g->state == PAR_ST_PLAY) bullet_hits(g);

    g->spawn_ticks--;
    if (g->spawn_ticks <= 0) {
        spawn_heli(g);
        g->spawn_ticks = imax(60, 150 - g->score);
    }
}

void parachute_init(parachute_t *g) {
    memset(g, 0, sizeof *g);
    g->state = PAR_ST_TITLE;
}

void parachute_start(parachute_t *g, uint32_t seed) {
    int hs = g->high_score;
    memset(g, 0, sizeof *g);
    g->high_score = hs;
    g->state = PAR_ST_TITLE;
    g->aim = 0;
    g->spawn_ticks = 60;
    prng_seed(&g->rng, seed);
}

void parachute_update(parachute_t *g, const input_t in[GAME_MAX_PLAYERS]) {
    g->ticks++;
    switch (g->state) {
    case PAR_ST_TITLE:
        if (in[0].a) g->state = PAR_ST_PLAY;
        break;
    case PAR_ST_PLAY:
        play_tick(g, &in[0]);
        break;
    case PAR_ST_GAMEOVER:
        if (in[0].a) parachute_start(g, g->ticks);
        break;
    }
}

void parachute_render(const parachute_t *g, const draw_t *d) {
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);

    for (int i = 0; i < PAR_MAX_HELIS; i++) {
        const par_heli_t *h = &g->helis[i];
        if (!h->alive) continue;
        draw_rect(d, h->x, h->y, h->x + 16, h->y + 6, COLOR_DARK);
        draw_rect(d, h->x + 2, h->y - 3, h->x + 14, h->y - 1, COLOR_DARK);
    }

    for (int i = 0; i < PAR_MAX_TROOPERS; i++) {
        const par_trooper_t *t = &g->troopers[i];
        if (!t->alive) continue;
        draw_rect(d, t->x, t->y, t->x + 6, t->y + 8, COLOR_BLUE);
        if (t->chute)
            draw_rect(d, t->x - 3, t->y - 8, t->x + 9, t->y - 2, COLOR_ORANGE);
    }

    for (int i = 0; i < PAR_MAX_LANDED; i++) {
        const par_landed_t *t = &g->landed[i];
        if (!t->alive) continue;
        draw_rect(d, t->x, PAR_TURRET_Y, t->x + 6, PAR_GROUND, COLOR_BLUE);
    }

    draw_rect(d, PAR_TURRET_X0, PAR_TURRET_Y, PAR_TURRET_X1, PAR_GROUND, COLOR_DARK);
    int32_t dx, dy;
    aim_dir(g->aim, &dx, &dy);
    for (int k = 1; k <= 3; k++) {
        int cx = barrel_pos(160, k, dx);
        int cy = barrel_pos(220, k, dy);
        draw_rect(d, cx - 2, cy - 2, cx + 2, cy + 2, COLOR_DARK);
    }

    for (int i = 0; i < PAR_MAX_BULLETS; i++) {
        const par_bullet_t *b = &g->bullets[i];
        if (!b->alive) continue;
        int x0, y0, x1, y1;
        bullet_rect(b, &x0, &y0, &x1, &y1);
        draw_rect(d, x0, y0, x1, y1, COLOR_DARK);
    }

    char buf[24];
    fmt_label(buf, sizeof buf, "SCORE", g->score);
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, PLAY_X0, HUD_BASELINE, DRAW_TEXT_DARK, buf);
    fmt_label(buf, sizeof buf, "HIGH", g->high_score);
    draw_text(d, DRAW_FONT_HUD, DRAW_RIGHT, PLAY_X1, HUD_BASELINE, DRAW_TEXT_DARK, buf);

    if (g->state == PAR_ST_TITLE) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 100, DRAW_TEXT_DARK, "PARACHUTE");
        fmt_label(buf, sizeof buf, "HIGH SCORE", g->high_score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 130, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 160, DRAW_TEXT_DARK, "PRESS A");
    } else if (g->state == PAR_ST_GAMEOVER) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 140, DRAW_TEXT_DARK, "GAME OVER");
        fmt_label(buf, sizeof buf, "SCORE", g->score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 168, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 196, DRAW_TEXT_DARK, "PRESS A");
    }
}

static int shot_hits_body(int idx, int tx, int ty, int tvx, int tvy, int tw, int th) {
    int32_t dx = PAR_SIN[idx];
    int32_t dy = -(int32_t)PAR_COS[idx];
    int32_t bx = (int32_t)barrel_pos(160, 4, dx) << 8;
    int32_t by = (int32_t)barrel_pos(220, 4, dy) << 8;
    int32_t bvx = dx * PAR_BULLET_SPEED;
    int32_t bvy = dy * PAR_BULLET_SPEED;
    for (int t = 0; t < 90; t++) {
        bx += bvx;
        by += bvy;
        tx += tvx;
        ty += tvy;
        int px = (int)(bx >> 8);
        int py = (int)(by >> 8);
        int x0 = px - 1, y0 = py - 1, x1 = px + 2, y1 = py + 2;
        if (rects_overlap(x0, y0, x1, y1, tx, ty, tx + tw, ty + th)) return 1;
        if (!rects_overlap(x0, y0, x1, y1, PLAY_X0, PLAY_Y0, PLAY_X1, PLAY_Y1)) return 0;
        if (ty + th >= PAR_GROUND) return 0;
    }
    return 0;
}

static int best_shot_index(int tx, int ty, int tvx, int tvy, int tw, int th, int cur_aim) {
    int cur_i = par_aim_index(cur_aim);
    int best = -1, best_ad = 99;
    for (int i = 0; i < 31; i++) {
        if (!shot_hits_body(i, tx, ty, tvx, tvy, tw, th)) continue;
        int ad = i - cur_i;
        if (ad < 0) ad = -ad;
        if (ad < best_ad) {
            best_ad = ad;
            best = i;
        }
    }
    return best;
}

static int cur_aim_hits(const parachute_t *g, int idx) {
    for (int i = 0; i < PAR_MAX_TROOPERS; i++) {
        const par_trooper_t *t = &g->troopers[i];
        if (!t->alive) continue;
        if (shot_hits_body(idx, t->x, t->y, 0, t->chute ? 1 : 4, 6, 8)) return 1;
    }
    for (int i = 0; i < PAR_MAX_HELIS; i++) {
        const par_heli_t *h = &g->helis[i];
        if (!h->alive) continue;
        if (shot_hits_body(idx, h->x, h->y, h->dir, 0, 16, 6)) return 1;
    }
    return 0;
}

void parachute_autoplay(const parachute_t *g, input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
    if (g->state == PAR_ST_TITLE || g->state == PAR_ST_GAMEOVER) {
        in[0].a = true;
        return;
    }

    int tx = 160, ty = 0, tw = 6, th = 8, tvx = 0, tvy = 0, have = 0, best_y = -1;
    for (int i = 0; i < PAR_MAX_TROOPERS; i++) {
        const par_trooper_t *t = &g->troopers[i];
        if (!t->alive || !t->chute) continue;
        if (t->y >= best_y) {
            best_y = t->y;
            tx = t->x; ty = t->y; tw = 6; th = 8; tvx = 0; tvy = 1; have = 1;
        }
    }
    if (!have) {
        int best_d = 99999;
        for (int i = 0; i < PAR_MAX_HELIS; i++) {
            const par_heli_t *h = &g->helis[i];
            if (!h->alive) continue;
            int d = h->x + 8 - 160;
            if (d < 0) d = -d;
            if (d < best_d) {
                best_d = d;
                tx = h->x; ty = h->y; tw = 16; th = 6; tvx = h->dir; tvy = 0; have = 1;
            }
        }
    }

    int target_deg = 0;
    if (have) {
        int want_i = best_shot_index(tx, ty, tvx, tvy, tw, th, g->aim);
        if (want_i < 0) {
            int dx_t = tx + tw / 2 - 160;
            int dy_t = ty + th / 2 - 220;
            int32_t best_dot = (int32_t)0x80000000;
            want_i = 15;
            for (int i = 0; i < 31; i++) {
                int32_t dot = (int32_t)dx_t * PAR_SIN[i] - (int32_t)dy_t * PAR_COS[i];
                if (dot > best_dot) { best_dot = dot; want_i = i; }
            }
        }
        target_deg = want_i * 5 - 75;
    }

    if (target_deg > g->aim + 2) in[0].stick_x = 256;
    else if (target_deg < g->aim - 2) in[0].stick_x = -256;
    else if (have && best_y >= 0 && g->score < PAR_AUTOPLAY_CEASEFIRE &&
             count_alive_bullets(g) == 0 &&
             cur_aim_hits(g, par_aim_index(g->aim))) {
        in[0].a = true;
    }
}

static parachute_t parachute_state;
static void desc_init(void *st) { parachute_init(st); }
static void desc_start(void *st, uint32_t seed) { parachute_start(st, seed); }
static void desc_update(void *st, const input_t in[GAME_MAX_PLAYERS]) { parachute_update(st, in); }
static void desc_render(const void *st, const draw_t *d) { parachute_render(st, d); }
static void desc_autoplay(const void *st, input_t in[GAME_MAX_PLAYERS]) { parachute_autoplay(st, in); }
static bool desc_is_over(const void *st) { return ((const parachute_t *)st)->state == PAR_ST_GAMEOVER; }
static int  desc_get_hs(const void *st) { return ((const parachute_t *)st)->high_score; }
static void desc_set_hs(void *st, int v) { ((parachute_t *)st)->high_score = v; }
static sfx_queue_t *desc_sfx(void *st) { return &((parachute_t *)st)->sfx; }

const game_desc_t GAME_PARACHUTE = {
    .name = "PARACHUTE", .players = 1, .state = &parachute_state,
    .init = desc_init, .start = desc_start, .update = desc_update,
    .render = desc_render, .autoplay = desc_autoplay, .is_over = desc_is_over,
    .get_high_score = desc_get_hs, .set_high_score = desc_set_hs, .sfx = desc_sfx,
    .track = MUSIC_PARACHUTE,
};
