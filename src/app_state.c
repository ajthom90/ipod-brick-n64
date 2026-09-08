#include "app_state.h"
#include "menu.h"
#include "games/registry.h"
#include <string.h>

static int hs_at_start;
static bool game_was_over;

static const game_desc_t *active_game(const app_t *a) {
    int i = a->game_index;
    if (i < 0 || i >= GAME_COUNT) i = 0;
    return GAMES[i];
}

static void reset_nav(app_t *a) {
    a->last_nav = 0;
    a->repeat_ticks = 0;
}

static void start_game(app_t *a, uint32_t seed) {
    const game_desc_t *g = active_game(a);
    hs_at_start = g->get_high_score(g->state);
    g->start(g->state, seed);
    game_was_over = g->is_over(g->state);
    a->screen = APP_GAME;
    reset_nav(a);
}

static void tick_game(app_t *a, const input_t in[GAME_MAX_PLAYERS]) {
    const game_desc_t *g = active_game(a);
    g->update(g->state, in);
    bool over = g->is_over(g->state);
    if (over && !game_was_over && g->get_high_score(g->state) > hs_at_start) {
        a->high_score_dirty = true;
    }
    game_was_over = over;
}

static void update_menu(app_t *a, const input_t *in, uint32_t seed) {
    int8_t nav = menu_nav(in);
    if (menu_apply_nav(&a->menu_row, 0, GAME_COUNT, &a->repeat_ticks, &a->last_nav, nav)) {
        sfx_push(&a->sfx, SFX_MENU_MOVE);
    }
    if (in->a) {
        sfx_push(&a->sfx, SFX_MENU_SELECT);
        if (a->menu_row >= GAME_COUNT) {
            a->screen = APP_SETTINGS;
            settings_screen_init(&a->settings_screen, &a->settings);
            reset_nav(a);
        } else {
            a->game_index = a->menu_row;
            start_game(a, seed);
        }
    }
}

static void update_settings(app_t *a, const input_t *in) {
    bool close = settings_screen_update(&a->settings_screen, in, &a->sfx);
    if (a->settings_screen.changed) {
        a->settings = a->settings_screen.values;
        a->settings_dirty = true;
    }
    if (close) {
        a->screen = APP_MENU;
        reset_nav(a);
    }
}

static void update_pause(app_t *a, const input_t *in, bool start_pressed) {
    if (start_pressed) {
        a->screen = APP_GAME;
        reset_nav(a);
        return;
    }
    int8_t nav = menu_nav(in);
    if (menu_apply_nav(&a->pause_row, 0, PAUSE_ROWS - 1, &a->repeat_ticks, &a->last_nav, nav)) {
        sfx_push(&a->sfx, SFX_MENU_MOVE);
    }
    if (in->a) {
        sfx_push(&a->sfx, SFX_MENU_SELECT);
        if (a->pause_row == PAUSE_ROW_RESUME) {
            a->screen = APP_GAME;
        } else {
            a->screen = APP_MENU;
        }
        reset_nav(a);
    }
}

void app_init(app_t *a, bool autoplay, int autoplay_game) {
    memset(a, 0, sizeof *a);
    a->autoplay = autoplay;
    settings_defaults(&a->settings);
    for (int i = 0; i < GAME_COUNT; i++) {
        GAMES[i]->init(GAMES[i]->state);
    }
    hs_at_start = 0;
    game_was_over = false;
    if (autoplay) {
        if (autoplay_game < 0 || autoplay_game >= GAME_COUNT) autoplay_game = 0;
        a->game_index = autoplay_game;
        start_game(a, 0x1234567u);
    } else {
        a->screen = APP_MENU;
    }
}

void app_update(app_t *a, const input_t in[GAME_MAX_PLAYERS], bool start_pressed, uint32_t seed) {
    input_t local[GAME_MAX_PLAYERS];
    memcpy(local, in, sizeof local);
    if (a->autoplay) {
        start_pressed = false;
        if (a->screen == APP_GAME) {
            const game_desc_t *g = active_game(a);
            g->autoplay(g->state, local);
        }
    }

    switch (a->screen) {
    case APP_MENU:
        update_menu(a, &local[0], seed);
        break;
    case APP_SETTINGS:
        update_settings(a, &local[0]);
        break;
    case APP_GAME:
        if (start_pressed) {
            a->screen = APP_PAUSE;
            a->pause_row = 0;
            reset_nav(a);
            break;
        }
        tick_game(a, local);
        break;
    case APP_PAUSE:
        update_pause(a, &local[0], start_pressed);
        break;
    }
}

void app_render(const app_t *a, const draw_t *d) {
    const game_desc_t *g;
    switch (a->screen) {
    case APP_MENU:
        menu_draw_games(d, a->menu_row);
        break;
    case APP_SETTINGS:
        settings_screen_render(&a->settings_screen, d);
        break;
    case APP_GAME:
        g = active_game(a);
        g->render(g->state, d);
        break;
    case APP_PAUSE:
        g = active_game(a);
        g->render(g->state, d);
        menu_draw_pause(d, a->pause_row);
        break;
    }
}

music_track_id_t app_track(const app_t *a) {
    if (a->screen == APP_MENU || a->screen == APP_SETTINGS) return MUSIC_MENU;
    return active_game(a)->track;
}

sfx_id_t app_next_sfx(app_t *a) {
    sfx_id_t id = sfx_pop(&a->sfx);
    if (id != SFX_NONE) return id;
    if (a->game_index >= 0 && a->game_index < GAME_COUNT) {
        const game_desc_t *g = GAMES[a->game_index];
        return sfx_pop(g->sfx(g->state));
    }
    return SFX_NONE;
}

void app_high_scores(const app_t *a, int32_t out[SAVE_MAX_GAMES]) {
    (void)a;
    for (int i = 0; i < SAVE_MAX_GAMES; i++) {
        if (i < GAME_COUNT) out[i] = (int32_t)GAMES[i]->get_high_score(GAMES[i]->state);
        else out[i] = 0;
    }
}

void app_apply_save(app_t *a, const save_t *s) {
    a->settings = s->settings;
    for (int i = 0; i < GAME_COUNT && i < SAVE_MAX_GAMES; i++) {
        GAMES[i]->set_high_score(GAMES[i]->state, (int)s->high_scores[i]);
    }
}
