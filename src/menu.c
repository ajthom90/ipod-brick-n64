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

static void draw_row(const draw_t *d, int i, int selected, const char *label) {
    int base = 62 + 24 * i;
    if (i == selected) {
        draw_rect(d, 16, base - 20, 304, base + 6, COLOR_BLUE);
        draw_text(d, DRAW_FONT_BIG, DRAW_LEFT, 24, base, DRAW_TEXT_LIGHT, label);
    } else {
        draw_text(d, DRAW_FONT_BIG, DRAW_LEFT, 24, base, DRAW_TEXT_DARK, label);
    }
}

void menu_draw_games(const draw_t *d, int selected) {
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);
    draw_text(d, DRAW_FONT_BIG, DRAW_LEFT, 16, 30, DRAW_TEXT_DARK, "GAMES");
    draw_rect(d, 16, 36, 304, 37, COLOR_DARK);
    for (int i = 0; i < GAME_COUNT; i++) {
        draw_row(d, i, selected, GAMES[i]->name);
    }
    draw_row(d, GAME_COUNT, selected, "SETTINGS");
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


