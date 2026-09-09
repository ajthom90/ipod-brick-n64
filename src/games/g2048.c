#include "g2048.h"
#include <string.h>

static const int ROT_DIR[4] = { 3, 0, 3, 1 }; /* down, left, down, right */

bool g2048_slide_row(uint16_t row[G2048_N], int *gained) {
    uint16_t out[G2048_N] = {0};
    int w = 0;
    int merged_at = -1;
    for (int i = 0; i < G2048_N; i++) {
        if (!row[i]) continue;
        if (w > 0 && out[w - 1] == row[i] && (w - 1) != merged_at) {
            out[w - 1] = (uint16_t)(out[w - 1] * 2);
            *gained += (int)out[w - 1];
            merged_at = w - 1;
        } else {
            out[w++] = row[i];
        }
    }
    bool changed = false;
    for (int i = 0; i < G2048_N; i++) {
        if (row[i] != out[i]) changed = true;
        row[i] = out[i];
    }
    return changed;
}

static void gather(const g2048_t *g, int dir, int i, uint16_t line[G2048_N]) {
    for (int j = 0; j < G2048_N; j++) {
        int r, c;
        switch (dir) {
        case 0: r = i; c = j; break;
        case 1: r = i; c = G2048_N - 1 - j; break;
        case 2: r = j; c = i; break;
        default: r = G2048_N - 1 - j; c = i; break;
        }
        line[j] = g->board[r][c];
    }
}

static void scatter(g2048_t *g, int dir, int i, const uint16_t line[G2048_N]) {
    for (int j = 0; j < G2048_N; j++) {
        int r, c;
        switch (dir) {
        case 0: r = i; c = j; break;
        case 1: r = i; c = G2048_N - 1 - j; break;
        case 2: r = j; c = i; break;
        default: r = G2048_N - 1 - j; c = i; break;
        }
        g->board[r][c] = line[j];
    }
}

static bool dir_changes(const g2048_t *g, int dir) {
    uint16_t line[G2048_N];
    for (int i = 0; i < G2048_N; i++) {
        gather(g, dir, i, line);
        int gained = 0;
        if (g2048_slide_row(line, &gained)) return true;
    }
    return false;
}

void g2048_add_tile(g2048_t *g) {
    int empties = 0;
    for (int r = 0; r < G2048_N; r++)
        for (int c = 0; c < G2048_N; c++)
            if (!g->board[r][c]) empties++;
    if (empties <= 0) return;
    uint32_t k = prng_below(&g->rng, (uint32_t)empties);
    uint16_t val = prng_below(&g->rng, 10) < 9 ? 2 : 4;
    for (int r = 0; r < G2048_N; r++) {
        for (int c = 0; c < G2048_N; c++) {
            if (g->board[r][c]) continue;
            if (k == 0) {
                g->board[r][c] = val;
                return;
            }
            k--;
        }
    }
}

bool g2048_move(g2048_t *g, int dir) {
    if (dir < 0 || dir > 3) return false;
    int gained = 0;
    bool changed = false;
    uint16_t line[G2048_N];
    for (int i = 0; i < G2048_N; i++) {
        gather(g, dir, i, line);
        if (g2048_slide_row(line, &gained)) changed = true;
        scatter(g, dir, i, line);
    }
    g->score += gained;
    if (!changed) return false;
    g2048_add_tile(g);
    if (gained > 0) sfx_push(&g->sfx, SFX_MERGE);
    if (!g->reached_2048) {
        for (int r = 0; r < G2048_N; r++) {
            for (int c = 0; c < G2048_N; c++) {
                if (g->board[r][c] == 2048) {
                    g->reached_2048 = true;
                    sfx_push(&g->sfx, SFX_CLEAR);
                    r = G2048_N;
                    break;
                }
            }
        }
    }
    return true;
}

bool g2048_can_move(const g2048_t *g) {
    for (int r = 0; r < G2048_N; r++) {
        for (int c = 0; c < G2048_N; c++) {
            if (!g->board[r][c]) return true;
            if (c + 1 < G2048_N && g->board[r][c] == g->board[r][c + 1]) return true;
            if (r + 1 < G2048_N && g->board[r][c] == g->board[r + 1][c]) return true;
        }
    }
    return false;
}

uint32_t g2048_tile_color(uint16_t value) {
    switch (value) {
    case 2:    return COLOR_TILE;
    case 4:    return 0xE8D8B8u;
    case 8:    return COLOR_ORANGE;
    case 16:   return 0xD96A2Fu;
    case 32:   return COLOR_RED;
    case 64:   return 0xB03A3Au;
    case 128:  return COLOR_YELLOW;
    case 256:  return 0xC9A518u;
    case 512:  return COLOR_GREEN;
    case 1024: return COLOR_BLUE;
    case 2048: return COLOR_PURPLE;
    default:   return COLOR_DARK;
    }
}

static void game_over(g2048_t *g) {
    if (g->state != G2048_ST_PLAY) return;
    if (g->score > g->high_score) g->high_score = g->score;
    sfx_push(&g->sfx, SFX_GAME_OVER);
    g->state = G2048_ST_GAMEOVER;
}

static int read_move(g2048_t *g, const input_t *in) {
    int dir = -1;
    if (in->dpad_x != 0 && g->prev_dpad_x == 0)
        dir = in->dpad_x < 0 ? 0 : 1;
    else if (in->dpad_y != 0 && g->prev_dpad_y == 0)
        dir = in->dpad_y > 0 ? 2 : 3;
    else if (g->stick_armed) {
        if (in->stick_x <= -128) dir = 0;
        else if (in->stick_x >= 128) dir = 1;
        else if (in->stick_y >= 128) dir = 2;
        else if (in->stick_y <= -128) dir = 3;
        if (dir >= 0) g->stick_armed = false;
    }
    g->prev_dpad_x = in->dpad_x;
    g->prev_dpad_y = in->dpad_y;
    if (in->stick_x >= -64 && in->stick_x <= 64 &&
        in->stick_y >= -64 && in->stick_y <= 64)
        g->stick_armed = true;
    return dir;
}

void g2048_init(g2048_t *g) {
    memset(g, 0, sizeof *g);
    g->state = G2048_ST_TITLE;
}

void g2048_start(g2048_t *g, uint32_t seed) {
    int hs = g->high_score;
    memset(g, 0, sizeof *g);
    g->high_score = hs;
    g->state = G2048_ST_TITLE;
    g->stick_armed = true;
    prng_seed(&g->rng, seed);
    g2048_add_tile(g);
    g2048_add_tile(g);
}

void g2048_update(g2048_t *g, const input_t in[GAME_MAX_PLAYERS]) {
    g->ticks++;
    int dir = read_move(g, &in[0]);
    switch (g->state) {
    case G2048_ST_TITLE:
        if (in[0].a) g->state = G2048_ST_PLAY;
        break;
    case G2048_ST_PLAY:
        if (dir >= 0) {
            if (!g2048_move(g, dir))
                g->autoplay_dir = (g->autoplay_dir + 1) & 3;
        }
        if (!g2048_can_move(g)) game_over(g);
        break;
    case G2048_ST_GAMEOVER:
        if (in[0].a) g2048_start(g, g->ticks);
        break;
    }
}

void g2048_render(const g2048_t *g, const draw_t *d) {
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);
    draw_rect(d, G2048_X0, G2048_Y0, 248, 208, COLOR_DARK);

    for (int r = 0; r < G2048_N; r++) {
        for (int c = 0; c < G2048_N; c++) {
            int x = 76 + c * (G2048_TILE + G2048_GAP);
            int y = 36 + r * (G2048_TILE + G2048_GAP);
            uint16_t v = g->board[r][c];
            uint32_t rgb = v ? g2048_tile_color(v) : COLOR_TILE;
            draw_rect(d, x, y, x + G2048_TILE, y + G2048_TILE, rgb);
            if (!v) continue;
            char num[12];
            fmt_int(num, sizeof num, (int)v);
            uint32_t tc = (v <= 4) ? DRAW_TEXT_DARK : DRAW_TEXT_LIGHT;
            if (v < 1000)
                draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, x + 20, y + 28, tc, num);
            else
                draw_text(d, DRAW_FONT_HUD, DRAW_CENTER, x + 20, y + 25, tc, num);
        }
    }

    char buf[24];
    fmt_label(buf, sizeof buf, "SCORE", g->score);
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, PLAY_X0, HUD_BASELINE, DRAW_TEXT_DARK, buf);
    fmt_label(buf, sizeof buf, "HIGH", g->high_score);
    draw_text(d, DRAW_FONT_HUD, DRAW_RIGHT, PLAY_X1, HUD_BASELINE, DRAW_TEXT_DARK, buf);

    if (g->reached_2048)
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 224, DRAW_TEXT_DARK, "2048!");

    if (g->state == G2048_ST_TITLE) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 100, DRAW_TEXT_DARK, "2048");
        fmt_label(buf, sizeof buf, "HIGH SCORE", g->high_score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 130, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 160, DRAW_TEXT_DARK, "PRESS A");
    } else if (g->state == G2048_ST_GAMEOVER) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 140, DRAW_TEXT_DARK, "GAME OVER");
        fmt_label(buf, sizeof buf, "SCORE", g->score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 168, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 196, DRAW_TEXT_DARK, "PRESS A");
    }
}

void g2048_autoplay(const g2048_t *g, input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
    if (g->state == G2048_ST_TITLE || g->state == G2048_ST_GAMEOVER) {
        in[0].a = true;
        return;
    }
    if (g->state != G2048_ST_PLAY) return;
    if (g->ticks % G2048_AUTOPLAY_PERIOD != 0) return;

    int slot = g->autoplay_dir & 3;
    int dir = -1;
    for (int t = 0; t < 4; t++) {
        int d = ROT_DIR[(slot + t) & 3];
        if (dir_changes(g, d)) {
            dir = d;
            break;
        }
    }
    /* Rotation is down/left/right; up is last resort so autoplay cannot stall. */
    if (dir < 0 && dir_changes(g, 2)) dir = 2;
    if (dir < 0) return;
    if (dir == 0) in[0].dpad_x = -1;
    else if (dir == 1) in[0].dpad_x = 1;
    else if (dir == 2) in[0].dpad_y = 1;
    else in[0].dpad_y = -1;
}

static g2048_t g2048_state;
static void desc_init(void *st) { g2048_init(st); }
static void desc_start(void *st, uint32_t seed) { g2048_start(st, seed); }
static void desc_update(void *st, const input_t in[GAME_MAX_PLAYERS]) { g2048_update(st, in); }
static void desc_render(const void *st, const draw_t *d) { g2048_render(st, d); }
static void desc_autoplay(const void *st, input_t in[GAME_MAX_PLAYERS]) { g2048_autoplay(st, in); }
static bool desc_is_over(const void *st) { return ((const g2048_t *)st)->state == G2048_ST_GAMEOVER; }
static int  desc_get_hs(const void *st) { return ((const g2048_t *)st)->high_score; }
static void desc_set_hs(void *st, int v) { ((g2048_t *)st)->high_score = v; }
static sfx_queue_t *desc_sfx(void *st) { return &((g2048_t *)st)->sfx; }

const game_desc_t GAME_2048 = {
    .name = "2048", .players = 1, .state = &g2048_state,
    .init = desc_init, .start = desc_start, .update = desc_update,
    .render = desc_render, .autoplay = desc_autoplay, .is_over = desc_is_over,
    .get_high_score = desc_get_hs, .set_high_score = desc_set_hs, .sfx = desc_sfx,
    .track = MUSIC_2048,
};
