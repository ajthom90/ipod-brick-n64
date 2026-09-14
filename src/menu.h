#ifndef MENU_H
#define MENU_H
#include "game.h"

enum { MENU_REPEAT_FIRST = 18, MENU_REPEAT_NEXT = 8, MENU_VISIBLE_ROWS = 7 };
enum { PAUSE_ROW_RESUME = 0, PAUSE_ROW_QUIT = 1, PAUSE_ROWS = 2 };

int8_t menu_nav(const input_t *in);
bool menu_apply_nav(int *row, int row_min, int row_max,
                    int *repeat_ticks, int8_t *last_nav, int8_t nav);
int menu_scroll_for(int row, int scroll, int total, int visible);

void menu_render_rows(const draw_t *d, const char *const *labels, int total, int row, int scroll);
void menu_draw_games(const draw_t *d, int selected, int scroll);
void menu_draw_pause(const draw_t *d, int selected);

#endif
