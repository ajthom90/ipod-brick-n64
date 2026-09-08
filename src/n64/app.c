#include <libdragon.h>
#include "../game.h"
#include "../games/registry.h"
#include "../app_state.h"
#include "../synth.h"
#include "../music.h"
#include "../save.h"

static color_t rgb(uint32_t c) { return RGBA32((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 0xFF); }

static void n64_rect(void *ctx, int x0, int y0, int x1, int y1, uint32_t c) {
    (void)ctx;
    rdpq_set_fill_color(rgb(c));
    rdpq_fill_rectangle(x0, y0, x1, y1);
}

static void n64_text(void *ctx, draw_font_t font, draw_align_t align, int x, int y, uint32_t c, const char *s) {
    (void)ctx;
    rdpq_set_mode_standard();
    rdpq_textparms_t p = { .style_id = (c == DRAW_TEXT_LIGHT) ? 1 : 0 };
    if (align == DRAW_LEFT) {
        rdpq_text_print(&p, font, x, y, s);
    } else if (align == DRAW_CENTER) {
        int w = 2 * (x < SCREEN_W - x ? x : SCREEN_W - x);     /* widest box centered on x that stays on screen */
        p.width = (int16_t)w; p.align = ALIGN_CENTER;
        rdpq_text_print(&p, font, x - w / 2, y, s);
    } else {
        p.width = (int16_t)x; p.align = ALIGN_RIGHT;
        rdpq_text_print(&p, font, 0, y, s);
    }
    rdpq_set_mode_fill(rgb(COLOR_BG));
}

static const draw_t n64_draw = { .ctx = NULL, .rect = n64_rect, .text = n64_text };

static app_t app;
static synth_t synth;
static music_track_t tracks[MUSIC_TRACK_COUNT];
static bool have_eeprom;
static int16_t mono[1024];
static music_track_id_t last_track = MUSIC_TRACK_COUNT;

#ifndef AUTOPLAY_GAME
static int16_t axis(int8_t v) {
    int s = v;
    if (s > -8 && s < 8) s = 0;
    if (s > 80) s = 80;
    if (s < -80) s = -80;
    return (int16_t)((s * 256) / 80);
}

/* Levels every frame; edges OR-ed in and cleared by the caller after a tick. */
static void read_port(joypad_port_t port, input_t *in) {
    joypad_inputs_t inputs = joypad_get_inputs(port);
    joypad_buttons_t held = joypad_get_buttons(port);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(port);
    in->stick_x = axis(inputs.stick_x);
    in->stick_y = axis(inputs.stick_y);
    in->dpad_x = (held.d_right || held.c_right) ? 1 : (held.d_left || held.c_left) ? -1 : 0;
    in->dpad_y = (held.d_up || held.c_up) ? 1 : (held.d_down || held.c_down) ? -1 : 0;
    if (pressed.a) in->a = true;
    if (pressed.b) in->b = true;
    if (pressed.z) in->z = true;
    in->a_held = held.a;
    in->b_held = held.b;
}
#endif

static void clear_edges(input_t *in) { in->a = in->b = in->z = false; }

static void apply_settings(void) {
    synth_set_volume(&synth, app.settings.volume);
    synth_set_music_enabled(&synth, app.settings.music_on);
    synth_set_sfx_enabled(&synth, app.settings.sound_on);
}

static void persist_if_dirty(void) {
    if (!app.settings_dirty && !app.high_score_dirty) return;
    save_t s;
    s.settings = app.settings;
    app_high_scores(&app, s.high_scores);
    uint8_t raw[SAVE_SIZE];
    save_encode(&s, raw);
#ifndef AUTOPLAY_GAME
    if (have_eeprom) eepfs_write("/save.dat", raw, SAVE_SIZE);
#endif
    app.settings_dirty = false;
    app.high_score_dirty = false;
    apply_settings();
}

static void fill_audio(void) {
    music_track_id_t id = app_track(&app);
    if (id != last_track) {
        last_track = id;
        synth_set_track(&synth, &tracks[id]);
    }
    sfx_id_t sid;
    while ((sid = app_next_sfx(&app)) != SFX_NONE) {
        synth_play_sfx(&synth, sid);
    }
    while (audio_can_write()) {
        short *buf = audio_write_begin();
        int n = audio_get_buffer_length();
        synth_render(&synth, mono, n);
        for (int i = 0; i < n; i++) { buf[2 * i] = mono[i]; buf[2 * i + 1] = mono[i]; }
        audio_write_end();
    }
}

int main(void) {
    dfs_init(DFS_DEFAULT_LOCATION);
    /* FILTERS_DISABLED asserts at 320x240 16 bpp (hardware bug, libdragon display.c); resampling stays on. */
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
#ifdef BRICK_DEBUG
    rdpq_debug_start();
#endif
    joypad_init();

    audio_init(22050, 4);
    synth_init(&synth, audio_get_frequency());
    for (int i = 0; i < MUSIC_TRACK_COUNT; i++) {
        char err[64];
        if (!music_parse(&MUSIC_SRC[i], &tracks[i], err, sizeof err)) assertf(false, "track %d: %s", i, err);
    }
    save_t save; save_defaults(&save);
    have_eeprom = eeprom_present() != EEPROM_NONE;
    if (have_eeprom) {
        static const eepfs_entry_t entries[] = { { "/save.dat", SAVE_SIZE } };
        if (eepfs_init(entries, 1) == 0) {
            if (!eepfs_verify_signature()) { eepfs_wipe(); }
            uint8_t raw[SAVE_SIZE];
            if (eepfs_read("/save.dat", raw, SAVE_SIZE) == 0) save_decode(raw, &save);
        } else have_eeprom = false;
    }

    rdpq_font_t *hud = rdpq_font_load("rom:/hud.font64");
    rdpq_font_t *big = rdpq_font_load("rom:/big.font64");
    rdpq_font_style(hud, 0, &(rdpq_fontstyle_t){ .color = rgb(DRAW_TEXT_DARK) });
    rdpq_font_style(big, 0, &(rdpq_fontstyle_t){ .color = rgb(DRAW_TEXT_DARK) });
    rdpq_font_style(hud, 1, &(rdpq_fontstyle_t){ .color = rgb(DRAW_TEXT_LIGHT) });
    rdpq_font_style(big, 1, &(rdpq_fontstyle_t){ .color = rgb(DRAW_TEXT_LIGHT) });
    rdpq_text_register_font(DRAW_FONT_HUD, hud);
    rdpq_text_register_font(DRAW_FONT_BIG, big);

#ifdef AUTOPLAY_GAME
    int gi = game_index_by_name(AUTOPLAY_GAME);
    if (gi < 0) gi = 0;
    app_init(&app, true, gi);
#else
    app_init(&app, false, 0);
#endif
    app_apply_save(&app, &save);
    apply_settings();

    input_t in[GAME_MAX_PLAYERS] = {0};
    bool start_pressed = false;

    const int hz = (get_tv_type() == TV_PAL) ? 50 : 60;
    const uint64_t dt = TICKS_PER_SECOND / hz;
    uint64_t prev = get_ticks();
    uint64_t acc = 0;
    enum { CATCHUP_MAX = 4 };

    while (1) {
        uint64_t now = get_ticks();
        acc += now - prev;
        prev = now;
#ifndef AUTOPLAY_GAME
        joypad_poll();
        read_port(JOYPAD_PORT_1, &in[0]);
        read_port(JOYPAD_PORT_2, &in[1]);
        if (joypad_get_buttons_pressed(JOYPAD_PORT_1).start) start_pressed = true;
#endif
        int steps = 0;
        while (acc >= dt && steps < CATCHUP_MAX) {
            app_update(&app, in, start_pressed, (uint32_t)get_ticks() | 1u);
            clear_edges(&in[0]);
            clear_edges(&in[1]);
            start_pressed = false;
            acc -= dt;
            steps++;
        }
        if (steps == CATCHUP_MAX) acc = 0;   /* drop the backlog after a stall */

        persist_if_dirty();
        fill_audio();

        surface_t *fb = display_get();
        rdpq_attach(fb, NULL);
        rdpq_set_mode_fill(rgb(COLOR_BG));
        app_render(&app, &n64_draw);
        rdpq_detach_show();
    }
}
