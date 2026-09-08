#include "harness.h"
#include "settings.h"

enum { SET_ROW_MUSIC = 0, SET_ROW_SOUND = 1, SET_ROW_VOLUME = 2, SET_ROW_BACK = 3 };

static void zero_in(input_t *in) { memset(in, 0, sizeof *in); }

static void drain(sfx_queue_t *sfx) {
    while (sfx_pop(sfx) != SFX_NONE) {}
}

static void tick(settings_screen_t *ss, const input_t *in, sfx_queue_t *sfx) {
    settings_screen_update(ss, in, sfx);
}

static void tap_nav(settings_screen_t *ss, sfx_queue_t *sfx, int8_t dpad_y, int8_t dpad_x) {
    input_t in;
    zero_in(&in);
    in.dpad_y = dpad_y;
    in.dpad_x = dpad_x;
    tick(ss, &in, sfx);
    zero_in(&in);
    tick(ss, &in, sfx);
    drain(sfx);
}

typedef struct {
    char texts[16][32];
    uint32_t colors[16];
    int n;
} rec_t;

static void rec_rect(void *ctx, int x0, int y0, int x1, int y1, uint32_t rgb) {
    (void)ctx; (void)x0; (void)y0; (void)x1; (void)y1; (void)rgb;
}

static void rec_text(void *ctx, draw_font_t font, draw_align_t align, int x, int y,
                     uint32_t rgb, const char *utf8) {
    rec_t *r = ctx;
    (void)font; (void)align; (void)x; (void)y;
    if (r->n >= 16) return;
    size_t n = 0;
    while (utf8[n] && n < 31) { r->texts[r->n][n] = utf8[n]; n++; }
    r->texts[r->n][n] = '\0';
    r->colors[r->n] = rgb;
    r->n++;
}

static int find_text(const rec_t *r, const char *s) {
    for (int i = 0; i < r->n; i++) {
        if (strcmp(r->texts[i], s) == 0) return i;
    }
    return -1;
}

static void test_defaults(void) {
    settings_t s;
    settings_defaults(&s);
    CHECK(s.music_on);
    CHECK(s.sound_on);
    CHECK(s.volume == 7);
}

static void test_down_moves_and_queues_move(void) {
    settings_t cur;
    settings_defaults(&cur);
    settings_screen_t ss;
    settings_screen_init(&ss, &cur);
    CHECK(ss.row == SET_ROW_MUSIC);
    CHECK(!ss.changed);

    sfx_queue_t sfx;
    memset(&sfx, 0, sizeof sfx);
    input_t in;
    zero_in(&in);
    in.dpad_y = -1;
    CHECK(!settings_screen_update(&ss, &in, &sfx));
    CHECK(ss.row == SET_ROW_SOUND);
    CHECK(sfx_pop(&sfx) == SFX_MENU_MOVE);
    CHECK(sfx_pop(&sfx) == SFX_NONE);
}

static void test_a_toggles_music_and_sets_changed(void) {
    settings_t cur;
    settings_defaults(&cur);
    settings_screen_t ss;
    settings_screen_init(&ss, &cur);
    sfx_queue_t sfx;
    memset(&sfx, 0, sizeof sfx);
    input_t in;
    zero_in(&in);
    in.a = true;
    CHECK(!settings_screen_update(&ss, &in, &sfx));
    CHECK(!ss.values.music_on);
    CHECK(ss.changed);
    CHECK(sfx_pop(&sfx) == SFX_MENU_SELECT);
}

static void test_volume_left_right_clamp_and_a_wrap(void) {
    settings_t cur;
    settings_defaults(&cur);
    settings_screen_t ss;
    settings_screen_init(&ss, &cur);
    sfx_queue_t sfx;
    memset(&sfx, 0, sizeof sfx);

    tap_nav(&ss, &sfx, -1, 0);
    tap_nav(&ss, &sfx, -1, 0);
    CHECK(ss.row == SET_ROW_VOLUME);
    CHECK(ss.values.volume == 7);

    input_t in;
    zero_in(&in);
    in.a = true;
    CHECK(!settings_screen_update(&ss, &in, &sfx));
    CHECK(ss.values.volume == 8);
    CHECK(ss.changed);
    drain(&sfx);

    zero_in(&in);
    tick(&ss, &in, &sfx);
    in.a = true;
    settings_screen_update(&ss, &in, &sfx);
    CHECK(ss.values.volume == 9);
    zero_in(&in);
    tick(&ss, &in, &sfx);
    in.a = true;
    settings_screen_update(&ss, &in, &sfx);
    CHECK(ss.values.volume == 10);
    zero_in(&in);
    tick(&ss, &in, &sfx);
    in.a = true;
    settings_screen_update(&ss, &in, &sfx);
    CHECK(ss.values.volume == 0);
    drain(&sfx);

    /* Right clamps at 10; left clamps at 0. */
    for (int i = 0; i < 12; i++) tap_nav(&ss, &sfx, 0, 1);
    CHECK(ss.values.volume == 10);
    zero_in(&in);
    in.dpad_x = 1;
    settings_screen_update(&ss, &in, &sfx);
    CHECK(ss.values.volume == 10);

    for (int i = 0; i < 12; i++) tap_nav(&ss, &sfx, 0, -1);
    CHECK(ss.values.volume == 0);
    zero_in(&in);
    in.dpad_x = -1;
    settings_screen_update(&ss, &in, &sfx);
    CHECK(ss.values.volume == 0);
}

static void test_a_on_back_and_b_close(void) {
    settings_t cur;
    settings_defaults(&cur);
    settings_screen_t ss;
    settings_screen_init(&ss, &cur);
    sfx_queue_t sfx;
    memset(&sfx, 0, sizeof sfx);

    tap_nav(&ss, &sfx, -1, 0);
    tap_nav(&ss, &sfx, -1, 0);
    tap_nav(&ss, &sfx, -1, 0);
    CHECK(ss.row == SET_ROW_BACK);

    input_t in;
    zero_in(&in);
    in.a = true;
    CHECK(settings_screen_update(&ss, &in, &sfx));
    CHECK(sfx_pop(&sfx) == SFX_MENU_SELECT);

    settings_screen_init(&ss, &cur);
    drain(&sfx);
    zero_in(&in);
    in.b = true;
    CHECK(settings_screen_update(&ss, &in, &sfx));
}

static void test_render_labels_and_highlight(void) {
    settings_t cur;
    settings_defaults(&cur);
    settings_screen_t ss;
    settings_screen_init(&ss, &cur);

    rec_t rec;
    memset(&rec, 0, sizeof rec);
    draw_t d = { .ctx = &rec, .rect = rec_rect, .text = rec_text };
    settings_screen_render(&ss, &d);

    int music = find_text(&rec, "MUSIC: ON");
    int sound = find_text(&rec, "SOUND: ON");
    int volume = find_text(&rec, "VOLUME: 7");
    int back = find_text(&rec, "BACK");
    CHECK(music >= 0);
    CHECK(sound >= 0);
    CHECK(volume >= 0);
    CHECK(back >= 0);
    CHECK(rec.colors[music] == DRAW_TEXT_LIGHT);
    CHECK(rec.colors[sound] == DRAW_TEXT_DARK);
    CHECK(rec.colors[volume] == DRAW_TEXT_DARK);
    CHECK(rec.colors[back] == DRAW_TEXT_DARK);
}

int main(void) {
    RUN(test_defaults);
    RUN(test_down_moves_and_queues_move);
    RUN(test_a_toggles_music_and_sets_changed);
    RUN(test_volume_left_right_clamp_and_a_wrap);
    RUN(test_a_on_back_and_b_close);
    RUN(test_render_labels_and_highlight);
    HARNESS_MAIN_END();
}
