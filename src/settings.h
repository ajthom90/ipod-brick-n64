#ifndef SETTINGS_H
#define SETTINGS_H
#include "game.h"

typedef struct { bool music_on, sound_on; int volume; } settings_t;     /* volume 0..10; defaults on, on, 7 */
typedef struct { settings_t values; int row; bool changed; } settings_screen_t;  /* row 0 MUSIC, 1 SOUND, 2 VOLUME, 3 BACK */
void settings_defaults(settings_t *s);
void settings_screen_init(settings_screen_t *ss, const settings_t *current);
/* Returns true when the screen wants to close (BACK chosen). Queues SFX_MENU_MOVE / SFX_MENU_SELECT on sfx. */
bool settings_screen_update(settings_screen_t *ss, const input_t *in, sfx_queue_t *sfx);
void settings_screen_render(const settings_screen_t *ss, const draw_t *d);

#endif
