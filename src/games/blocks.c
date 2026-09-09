#include "blocks.h"
#include <string.h>

const int8_t BLK_SHAPES[BLK_PIECE_COUNT][4][4][2] = {
  /* I */ {{{0,1},{1,1},{2,1},{3,1}}, {{2,0},{2,1},{2,2},{2,3}}, {{0,2},{1,2},{2,2},{3,2}}, {{1,0},{1,1},{1,2},{1,3}}},
  /* O */ {{{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}},
  /* T */ {{{1,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{2,1},{1,2}}, {{1,0},{0,1},{1,1},{1,2}}},
  /* S */ {{{1,0},{2,0},{0,1},{1,1}}, {{1,0},{1,1},{2,1},{2,2}}, {{1,1},{2,1},{0,2},{1,2}}, {{0,0},{0,1},{1,1},{1,2}}},
  /* Z */ {{{0,0},{1,0},{1,1},{2,1}}, {{2,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{1,2},{2,2}}, {{1,0},{0,1},{1,1},{0,2}}},
  /* J */ {{{0,0},{0,1},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{1,2}}, {{0,1},{1,1},{2,1},{2,2}}, {{1,0},{1,1},{0,2},{1,2}}},
  /* L */ {{{2,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{1,2},{2,2}}, {{0,1},{1,1},{2,1},{0,2}}, {{0,0},{1,0},{1,1},{1,2}}},
};
const uint8_t BLK_GRAVITY[20] = { 48, 43, 38, 33, 28, 23, 18, 13, 8, 6, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2 };

static const uint32_t PIECE_COLOR[BLK_PIECE_COUNT] = {
    COLOR_TEAL, COLOR_YELLOW, COLOR_PURPLE, COLOR_GREEN, COLOR_RED, COLOR_BLUE, COLOR_ORANGE
};
static const int LINE_SCORE[4] = { 40, 100, 300, 1200 };
static const int KICKS[5] = { 0, -1, 1, -2, 2 };

uint32_t blocks_piece_color(blk_piece_t p) {
    if ((unsigned)p >= BLK_PIECE_COUNT) return COLOR_DARK;
    return PIECE_COLOR[p];
}

static int gravity_for(const blocks_t *g) {
    int i = g->level - 1;
    if (i < 0) i = 0;
    if (i > 19) i = 19;
    return BLK_GRAVITY[i];
}

bool blocks_fits(const blocks_t *g, blk_piece_t p, int rot, int px, int py) {
    if ((unsigned)p >= BLK_PIECE_COUNT) return false;
    rot &= 3;
    for (int i = 0; i < 4; i++) {
        int x = px + BLK_SHAPES[p][rot][i][0];
        int y = py + BLK_SHAPES[p][rot][i][1];
        if (x < 0 || x >= BLK_COLS || y >= BLK_ROWS) return false;
        if (y >= 0 && g->cells[y][x]) return false;
    }
    return true;
}

static void refill_bag(blocks_t *g) {
    for (int i = 0; i < BLK_PIECE_COUNT; i++) g->bag[i] = (uint8_t)i;
    for (int i = BLK_PIECE_COUNT - 1; i > 0; i--) {
        uint32_t j = prng_below(&g->rng, (uint32_t)(i + 1));
        uint8_t t = g->bag[i];
        g->bag[i] = g->bag[j];
        g->bag[j] = t;
    }
    g->bag_left = BLK_PIECE_COUNT;
}

blk_piece_t blocks_bag_next(blocks_t *g) {
    if (g->bag_left <= 0) refill_bag(g);
    return (blk_piece_t)g->bag[--g->bag_left];
}

static bool try_shift(blocks_t *g, int dx, int dy) {
    if (!blocks_fits(g, g->piece, g->rot, g->px + dx, g->py + dy)) return false;
    g->px += dx;
    g->py += dy;
    return true;
}

static void apply_clear(blocks_t *g) {
    uint8_t keep[BLK_ROWS][BLK_COLS];
    memset(keep, 0, sizeof keep);
    int dst = BLK_ROWS - 1;
    for (int r = BLK_ROWS - 1; r >= 0; r--) {
        int flashing = 0;
        for (int i = 0; i < g->flash_count; i++) {
            if (g->flash_rows[i] == (uint8_t)r) flashing = 1;
        }
        if (flashing) continue;
        memcpy(keep[dst], g->cells[r], BLK_COLS);
        dst--;
    }
    memcpy(g->cells, keep, sizeof g->cells);
}

void blocks_spawn(blocks_t *g) {
    g->piece = g->next;
    g->next = blocks_bag_next(g);
    g->rot = 0;
    g->px = 3;
    g->py = 0;
    g->gravity_ticks = gravity_for(g);
    g->soft_ticks = 0;
    g->das_dir = 0;
    g->das_ticks = 0;
    if (!blocks_fits(g, g->piece, g->rot, g->px, g->py)) {
        if (g->score > g->high_score) g->high_score = g->score;
        sfx_push(&g->sfx, SFX_GAME_OVER);
        g->state = BLK_ST_GAMEOVER;
        return;
    }
    g->state = BLK_ST_PLAY;
}

static void lock_piece(blocks_t *g) {
    int rot = g->rot & 3;
    for (int i = 0; i < 4; i++) {
        int x = g->px + BLK_SHAPES[g->piece][rot][i][0];
        int y = g->py + BLK_SHAPES[g->piece][rot][i][1];
        if (y >= 0 && y < BLK_ROWS && x >= 0 && x < BLK_COLS)
            g->cells[y][x] = (uint8_t)(g->piece + 1);
    }
    sfx_push(&g->sfx, SFX_HIT);
    g->flash_count = 0;
    for (int r = 0; r < BLK_ROWS; r++) {
        int full = 1;
        for (int c = 0; c < BLK_COLS; c++) {
            if (!g->cells[r][c]) { full = 0; break; }
        }
        if (full && g->flash_count < 4) g->flash_rows[g->flash_count++] = (uint8_t)r;
    }
    if (g->flash_count > 0) {
        g->state = BLK_ST_FLASH;
        g->flash_ticks = BLK_FLASH_TICKS;
        sfx_push(&g->sfx, SFX_CLEAR);
    } else {
        blocks_spawn(g);
    }
}

static void try_rotate(blocks_t *g, int drot) {
    int nrot = (g->rot + drot) & 3;
    for (int k = 0; k < 5; k++) {
        int npx = g->px + KICKS[k];
        if (blocks_fits(g, g->piece, nrot, npx, g->py)) {
            g->rot = nrot;
            g->px = npx;
            return;
        }
    }
}

static int horiz_dir(const input_t *in) {
    if (in->dpad_x != 0) return in->dpad_x;
    if (in->stick_x >= 128) return 1;
    if (in->stick_x <= -128) return -1;
    return 0;
}

static void play_tick(blocks_t *g, const input_t *in) {
    int dir = horiz_dir(in);
    if (dir != g->das_dir) {
        g->das_dir = dir;
        if (dir != 0) {
            try_shift(g, dir, 0);
            g->das_ticks = BLK_DAS_DELAY;
        }
    } else if (dir != 0) {
        g->das_ticks--;
        if (g->das_ticks <= 0) {
            try_shift(g, dir, 0);
            g->das_ticks = BLK_DAS_REPEAT;
        }
    }

    if (in->a) try_rotate(g, 1);
    if (in->b) try_rotate(g, 3);

    int hard = (in->dpad_y == 1) || (in->stick_y >= 128);
    if ((in->dpad_y != 1) && (in->stick_y < 64)) g->drop_armed = 1;
    if (hard && g->drop_armed) {
        g->drop_armed = 0;
        while (try_shift(g, 0, 1)) {}
        lock_piece(g);
        return;
    }

    int soft = (in->dpad_y == -1) || (in->stick_y <= -128);
    if (soft) {
        g->soft_ticks++;
        if (g->soft_ticks >= BLK_SOFT_DROP_TICKS) {
            g->soft_ticks = 0;
            if (try_shift(g, 0, 1)) g->gravity_ticks = gravity_for(g);
        }
    } else {
        g->soft_ticks = 0;
    }

    g->gravity_ticks--;
    if (g->gravity_ticks <= 0) {
        g->gravity_ticks = gravity_for(g);
        if (!try_shift(g, 0, 1)) lock_piece(g);
    }
}

void blocks_init(blocks_t *g) {
    memset(g, 0, sizeof *g);
    g->state = BLK_ST_TITLE;
    g->level = 1;
    g->drop_armed = 1;
}

void blocks_start(blocks_t *g, uint32_t seed) {
    int hs = g->high_score;
    memset(g, 0, sizeof *g);
    g->high_score = hs;
    g->level = 1;
    g->state = BLK_ST_TITLE;
    g->drop_armed = 1;
    prng_seed(&g->rng, seed);
    refill_bag(g);
    g->next = blocks_bag_next(g);
}

void blocks_update(blocks_t *g, const input_t in[GAME_MAX_PLAYERS]) {
    g->ticks++;
    switch (g->state) {
    case BLK_ST_TITLE:
        if (in[0].a) blocks_spawn(g);
        break;
    case BLK_ST_PLAY:
        play_tick(g, &in[0]);
        break;
    case BLK_ST_FLASH:
        g->flash_ticks--;
        if (g->flash_ticks <= 0) {
            apply_clear(g);
            int n = g->flash_count;
            if (n < 1) n = 1;
            if (n > 4) n = 4;
            g->score += LINE_SCORE[n - 1] * g->level;
            g->lines += n;
            g->level = 1 + g->lines / 10;
            g->flash_count = 0;
            blocks_spawn(g);
        }
        break;
    case BLK_ST_GAMEOVER:
        if (in[0].a) blocks_start(g, g->ticks);
        break;
    }
}

static int row_flashing(const blocks_t *g, int r) {
    for (int i = 0; i < g->flash_count; i++) {
        if (g->flash_rows[i] == (uint8_t)r) return 1;
    }
    return 0;
}

static void draw_cell(const draw_t *d, int c, int r, uint32_t rgb) {
    int x0 = BLK_WELL_X0 + c * BLK_CELL;
    int y0 = BLK_WELL_Y0 + r * BLK_CELL;
    draw_rect(d, x0, y0, x0 + 9, y0 + 9, rgb);
}

static void draw_piece_cells(const draw_t *d, blk_piece_t p, int rot, int px, int py, int cell, int ox, int oy) {
    rot &= 3;
    uint32_t rgb = blocks_piece_color(p);
    for (int i = 0; i < 4; i++) {
        int x = px + BLK_SHAPES[p][rot][i][0];
        int y = py + BLK_SHAPES[p][rot][i][1];
        if (cell == BLK_CELL) {
            if (x < 0 || x >= BLK_COLS || y < 0 || y >= BLK_ROWS) continue;
            draw_cell(d, x, y, rgb);
        } else {
            int x0 = ox + x * cell;
            int y0 = oy + y * cell;
            draw_rect(d, x0, y0, x0 + cell - 1, y0 + cell - 1, rgb);
        }
    }
}

void blocks_render(const blocks_t *g, const draw_t *d) {
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);
    draw_rect(d, 108, 22, 212, 226, COLOR_DARK);
    draw_rect(d, 110, 24, 210, 224, COLOR_TILE);

    for (int r = 0; r < BLK_ROWS; r++) {
        if (g->state == BLK_ST_FLASH && row_flashing(g, r)) {
            draw_rect(d, 110, 24 + r * BLK_CELL, 210, 24 + (r + 1) * BLK_CELL, COLOR_BG);
            continue;
        }
        for (int c = 0; c < BLK_COLS; c++) {
            if (!g->cells[r][c]) continue;
            draw_cell(d, c, r, blocks_piece_color((blk_piece_t)(g->cells[r][c] - 1)));
        }
    }
    if (g->state == BLK_ST_PLAY) {
        draw_piece_cells(d, g->piece, g->rot, g->px, g->py, BLK_CELL, 0, 0);
    }

    char buf[24];
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, 220, 40, DRAW_TEXT_DARK, "NEXT");
    draw_piece_cells(d, g->next, 0, 0, 0, 8, 220, 46);
    fmt_label(buf, sizeof buf, "SCORE", g->score);
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, 220, 100, DRAW_TEXT_DARK, buf);
    fmt_label(buf, sizeof buf, "LEVEL", g->level);
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, 220, 116, DRAW_TEXT_DARK, buf);
    fmt_label(buf, sizeof buf, "LINES", g->lines);
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, 220, 132, DRAW_TEXT_DARK, buf);
    fmt_label(buf, sizeof buf, "HIGH", g->high_score);
    draw_text(d, DRAW_FONT_HUD, DRAW_LEFT, 16, 40, DRAW_TEXT_DARK, buf);

    if (g->state == BLK_ST_TITLE) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 100, DRAW_TEXT_DARK, "BLOCKS");
        fmt_label(buf, sizeof buf, "HIGH SCORE", g->high_score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 130, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 160, DRAW_TEXT_DARK, "PRESS A");
    } else if (g->state == BLK_ST_GAMEOVER) {
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 140, DRAW_TEXT_DARK, "GAME OVER");
        fmt_label(buf, sizeof buf, "SCORE", g->score);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 168, DRAW_TEXT_DARK, buf);
        draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 196, DRAW_TEXT_DARK, "PRESS A");
    }
}

static int placement_cost(const blocks_t *g, int rot, int px, int *ok) {
    int py = 0;
    if (!blocks_fits(g, g->piece, rot, px, py)) {
        *ok = 0;
        return 0;
    }
    while (blocks_fits(g, g->piece, rot, px, py + 1)) py++;
    uint8_t board[BLK_ROWS][BLK_COLS];
    memcpy(board, g->cells, sizeof board);
    for (int i = 0; i < 4; i++) {
        int x = px + BLK_SHAPES[g->piece][rot][i][0];
        int y = py + BLK_SHAPES[g->piece][rot][i][1];
        if (y >= 0 && y < BLK_ROWS && x >= 0 && x < BLK_COLS) board[y][x] = 1;
    }
    int top = BLK_ROWS;
    int holes = 0;
    int lines = 0;
    for (int c = 0; c < BLK_COLS; c++) {
        int seen = 0;
        for (int r = 0; r < BLK_ROWS; r++) {
            if (board[r][c]) {
                seen = 1;
                if (r < top) top = r;
            } else if (seen) {
                holes++;
            }
        }
    }
    for (int r = 0; r < BLK_ROWS; r++) {
        int full = 1;
        for (int c = 0; c < BLK_COLS; c++) {
            if (!board[r][c]) { full = 0; break; }
        }
        if (full) lines++;
    }
    int max_height = BLK_ROWS - top;
    *ok = 1;
    return 4 * max_height + 8 * holes - 10 * lines;
}

static void choose_plan(const blocks_t *g, int *best_rot, int *best_px) {
    int best_cost = 0x7fffffff;
    int found = 0;
    for (int rot = 0; rot < 4; rot++) {
        for (int px = -2; px <= 9; px++) {
            int ok = 0;
            int cost = placement_cost(g, rot, px, &ok);
            if (!ok) continue;
            if (!found || cost < best_cost) {
                best_cost = cost;
                *best_rot = rot;
                *best_px = px;
                found = 1;
            }
        }
    }
    if (!found) {
        *best_rot = g->rot;
        *best_px = g->px;
    }
}

void blocks_autoplay(const blocks_t *g, input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
    if (g->state == BLK_ST_TITLE || g->state == BLK_ST_GAMEOVER) {
        in[0].a = true;
        return;
    }
    if (g->state != BLK_ST_PLAY) return;

    int best_rot, best_px;
    choose_plan(g, &best_rot, &best_px);

    if ((g->rot & 3) != best_rot) {
        in[0].a = true;
        return;
    }

    static int gap;
    static uint32_t last_ticks;
    if (g->ticks != last_ticks + 1) gap = 0;
    last_ticks = g->ticks;

    if (g->px != best_px) {
        if (gap) {
            gap = 0;
            return;
        }
        in[0].dpad_x = (int8_t)((g->px < best_px) ? 1 : -1);
        gap = 1;
        return;
    }
    gap = 0;
    in[0].dpad_y = 1;
}

static blocks_t blocks_state;
static void desc_init(void *st) { blocks_init(st); }
static void desc_start(void *st, uint32_t seed) { blocks_start(st, seed); }
static void desc_update(void *st, const input_t in[GAME_MAX_PLAYERS]) { blocks_update(st, in); }
static void desc_render(const void *st, const draw_t *d) { blocks_render(st, d); }
static void desc_autoplay(const void *st, input_t in[GAME_MAX_PLAYERS]) { blocks_autoplay(st, in); }
static bool desc_is_over(const void *st) { return ((const blocks_t *)st)->state == BLK_ST_GAMEOVER; }
static int  desc_get_hs(const void *st) { return ((const blocks_t *)st)->high_score; }
static void desc_set_hs(void *st, int v) { ((blocks_t *)st)->high_score = v; }
static sfx_queue_t *desc_sfx(void *st) { return &((blocks_t *)st)->sfx; }

const game_desc_t GAME_BLOCKS = {
    .name = "BLOCKS", .players = 1, .state = &blocks_state,
    .init = desc_init, .start = desc_start, .update = desc_update,
    .render = desc_render, .autoplay = desc_autoplay, .is_over = desc_is_over,
    .get_high_score = desc_get_hs, .set_high_score = desc_set_hs, .sfx = desc_sfx,
    .track = MUSIC_BLOCKS,
};
