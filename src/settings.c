#include "settings.h"
#include "menu.h"

enum { SET_ROW_MUSIC = 0, SET_ROW_SOUND = 1, SET_ROW_VOLUME = 2, SET_ROW_BACK = 3, SET_ROWS = 4 };

static int s_repeat_ticks;
static int8_t s_last_nav;
static int s_h_repeat_ticks;
static int8_t s_last_h_nav;

void settings_defaults(settings_t *s) {
    s->music_on = true;
    s->sound_on = true;
    s->volume = 7;
}

void settings_screen_init(settings_screen_t *ss, const settings_t *current) {
    ss->values = *current;
    ss->row = SET_ROW_MUSIC;
    ss->changed = false;
    s_repeat_ticks = 0;
    s_last_nav = 0;
    s_h_repeat_ticks = 0;
    s_last_h_nav = 0;
}

static int8_t nav_x(const input_t *in) {
    if (in->dpad_x != 0) return in->dpad_x;
    if (in->stick_x >= 128) return 1;
    if (in->stick_x <= -128) return -1;
    return 0;
}

static bool h_repeat_fire(int8_t nav) {
    if (nav == 0) {
        s_last_h_nav = 0;
        s_h_repeat_ticks = 0;
        return false;
    }
    bool fire = false;
    if (nav != s_last_h_nav) {
        fire = true;
        s_h_repeat_ticks = MENU_REPEAT_FIRST;
        s_last_h_nav = nav;
    } else {
        if (s_h_repeat_ticks > 0) s_h_repeat_ticks--;
        if (s_h_repeat_ticks == 0) {
            fire = true;
            s_h_repeat_ticks = MENU_REPEAT_NEXT;
        }
    }
    return fire;
}

static void set_volume(settings_screen_t *ss, int v, sfx_queue_t *sfx, sfx_id_t id) {
    if (v < 0) v = 0;
    if (v > 10) v = 10;
    if (v == ss->values.volume) return;
    ss->values.volume = v;
    ss->changed = true;
    sfx_push(sfx, id);
}

bool settings_screen_update(settings_screen_t *ss, const input_t *in, sfx_queue_t *sfx) {
    ss->changed = false;

    if (in->b) {
        sfx_push(sfx, SFX_MENU_SELECT);
        return true;
    }

    int8_t vnav = menu_nav(in);
    if (menu_apply_nav(&ss->row, 0, SET_ROWS - 1, &s_repeat_ticks, &s_last_nav, vnav)) {
        sfx_push(sfx, SFX_MENU_MOVE);
    }

    int8_t hnav = nav_x(in);
    if (ss->row == SET_ROW_VOLUME) {
        if (h_repeat_fire(hnav)) {
            set_volume(ss, ss->values.volume + (int)hnav, sfx, SFX_MENU_MOVE);
        }
    } else {
        s_last_h_nav = 0;
        s_h_repeat_ticks = 0;
    }

    if (in->a) {
        if (ss->row == SET_ROW_MUSIC) {
            ss->values.music_on = !ss->values.music_on;
            ss->changed = true;
            sfx_push(sfx, SFX_MENU_SELECT);
        } else if (ss->row == SET_ROW_SOUND) {
            ss->values.sound_on = !ss->values.sound_on;
            ss->changed = true;
            sfx_push(sfx, SFX_MENU_SELECT);
        } else if (ss->row == SET_ROW_VOLUME) {
            int v = ss->values.volume + 1;
            if (v > 10) v = 0;
            set_volume(ss, v, sfx, SFX_MENU_SELECT);
        } else {
            sfx_push(sfx, SFX_MENU_SELECT);
            return true;
        }
    }
    return false;
}

static void draw_row(const draw_t *d, int i, int selected, const char *label) {
    int base = 86 + 24 * i;
    if (i == selected) {
        draw_rect(d, 16, base - 20, 304, base + 6, COLOR_BLUE);
        draw_text(d, DRAW_FONT_BIG, DRAW_LEFT, 24, base, DRAW_TEXT_LIGHT, label);
    } else {
        draw_text(d, DRAW_FONT_BIG, DRAW_LEFT, 24, base, DRAW_TEXT_DARK, label);
    }
}

void settings_screen_render(const settings_screen_t *ss, const draw_t *d) {
    draw_rect(d, 0, 0, SCREEN_W, SCREEN_H, COLOR_BG);
    draw_text(d, DRAW_FONT_BIG, DRAW_LEFT, 16, 30, DRAW_TEXT_DARK, "SETTINGS");
    draw_rect(d, 16, 36, 304, 37, COLOR_DARK);

    draw_row(d, SET_ROW_MUSIC, ss->row, ss->values.music_on ? "MUSIC: ON" : "MUSIC: OFF");
    draw_row(d, SET_ROW_SOUND, ss->row, ss->values.sound_on ? "SOUND: ON" : "SOUND: OFF");
    char vol[16];
    fmt_label(vol, sizeof vol, "VOLUME:", ss->values.volume);
    draw_row(d, SET_ROW_VOLUME, ss->row, vol);
    draw_row(d, SET_ROW_BACK, ss->row, "BACK");
}
