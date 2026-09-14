#include "menu.h"
#include "games/registry.h"

int8_t menu_nav(const input_t *in) {
    if (in->dpad_y != 0) return in->dpad_y;
    if (in->stick_y >= 128) return 1;
    if (in->stick_y <= -128) return -1;
    return 0;
}

bool menu_apply_nav(int *row, int row_min, int row_max,
                    int *repeat_ticks, int8_t *last_nav, int8_t nav) {
    if (nav == 0) {
        *last_nav = 0;
        *repeat_ticks = 0;
        return false;
    }
    bool fire = false;
    if (nav != *last_nav) {
        fire = true;
        *repeat_ticks = MENU_REPEAT_FIRST;
        *last_nav = nav;
    } else {
        if (*repeat_ticks > 0) (*repeat_ticks)--;
        if (*repeat_ticks == 0) {
            fire = true;
            *repeat_ticks = MENU_REPEAT_NEXT;
        }
    }
    if (!fire) return false;
    int next = *row - (int)nav; /* up (+1) decreases the row index */
    if (next < row_min) next = row_min;
    if (next > row_max) next = row_max;
    if (next == *row) return false;
    *row = next;
    return true;
}

int menu_scroll_for(int row, int scroll, int total, int visible) {
    if (row < scroll) scroll = row;
    if (row >= scroll + visible) scroll = row - (visible - 1);
    int max_scroll = total - visible;
    if (max_scroll < 0) max_scroll = 0;
    if (scroll < 0) scroll = 0;
    if (scroll > max_scroll) scroll = max_scroll;
    return scroll;
}

void menu_render_rows(const draw_t *d, const char *const *labels, int total, int row, int scroll) {
    int hl_x1 = (total > MENU_VISIBLE_ROWS) ? 292 : 304;
    int last = scroll + MENU_VISIBLE_ROWS;
    if (last > total) last = total;
    for (int r = scroll; r < last; r++) {
        int base = 62 + 24 * (r - scroll);
        if (r == row) {
            draw_rect(d, 16, base - 20, hl_x1, base + 6, COLOR_BLUE);
            draw_text(d, DRAW_FONT_BIG, DRAW_LEFT, 24, base, DRAW_TEXT_LIGHT, labels[r]);
        } else {
            draw_text(d, DRAW_FONT_BIG, DRAW_LEFT, 24, base, DRAW_TEXT_DARK, labels[r]);
        }
    }
    if (total > MENU_VISIBLE_ROWS) {
        int th = 172 * MENU_VISIBLE_ROWS / total;
        int ty = 43 + (172 - th) * scroll / (total - MENU_VISIBLE_ROWS);
        draw_rect(d, 296, 42, 304, 214, 0xB8B2A8u);
        draw_rect(d, 297, ty, 303, ty + th, COLOR_DARK);
    }
}

void menu_draw_games(const draw_t *d, int selected, int scroll) {
    const char *labels[16];
    int total = GAME_COUNT + 1;
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);
    draw_text(d, DRAW_FONT_BIG, DRAW_LEFT, 16, 30, DRAW_TEXT_DARK, "GAMES");
    draw_rect(d, 16, 36, 304, 37, COLOR_DARK);
    for (int i = 0; i < GAME_COUNT && i < 15; i++) {
        labels[i] = GAMES[i]->name;
    }
    labels[GAME_COUNT] = "SETTINGS";
    menu_render_rows(d, labels, total, selected, scroll);
}

void menu_draw_pause(const draw_t *d, int selected) {
    draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, 120, DRAW_TEXT_DARK, "PAUSED");
    static const char *const labels[PAUSE_ROWS] = { "RESUME", "QUIT TO MENU" };
    static const int bases[PAUSE_ROWS] = { 150, 176 };
    for (int i = 0; i < PAUSE_ROWS; i++) {
        int base = bases[i];
        if (i == selected) {
            draw_rect(d, 96, base - 20, 224, base + 6, COLOR_BLUE);
            draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, base, DRAW_TEXT_LIGHT, labels[i]);
        } else {
            draw_text(d, DRAW_FONT_BIG, DRAW_CENTER, SCREEN_W / 2, base, DRAW_TEXT_DARK, labels[i]);
        }
    }
}


