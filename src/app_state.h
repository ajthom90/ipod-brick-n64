#ifndef APP_STATE_H
#define APP_STATE_H
#include "game.h"
#include "settings.h"
#include "save.h"

typedef enum { APP_MENU, APP_SETTINGS, APP_GAME, APP_PAUSE } app_screen_t;
typedef struct {
    app_screen_t screen;
    int game_index;            /* active or last active game */
    int menu_row;              /* 0..GAME_COUNT (last row = SETTINGS) */
    int pause_row;             /* 0 = RESUME, 1 = QUIT TO MENU */
    int repeat_ticks;          /* key-repeat timer for menu navigation */
    int8_t last_nav;           /* last vertical direction held, for repeat */
    sfx_queue_t sfx;           /* framework sounds (menu move/select) */
    bool autoplay;             /* autoplay build: no menu, no pause */
    bool high_score_dirty;     /* set when a game's high score rose after game over; cleared by the adapter after saving */
    settings_t settings;
    settings_screen_t settings_screen;
    bool settings_dirty;
} app_t;

void app_init(app_t *a, bool autoplay, int autoplay_game);           /* inits every game; menu or straight into a game */
void app_update(app_t *a, const input_t in[GAME_MAX_PLAYERS], bool start_pressed, uint32_t seed);
void app_render(const app_t *a, const draw_t *d);
music_track_id_t app_track(const app_t *a);                           /* MUSIC_MENU on menu/settings, the game's track otherwise (also while paused) */
sfx_id_t app_next_sfx(app_t *a);                                      /* drains framework sounds first, then the active game's queue */
void app_high_scores(const app_t *a, int32_t out[SAVE_MAX_GAMES]);
void app_apply_save(app_t *a, const save_t *s);

#endif
