#include "harness.h"
#include "music.h"
#include "synth.h"
#include <stdint.h>

enum { SR = 22050 };

static int parse_ok(const music_src_t *src, music_track_t *tr) {
    char err[128];
    memset(err, 0, sizeof err);
    if (!music_parse(src, tr, err, sizeof err)) {
        fprintf(stderr, "  parse %s: %s\n", src->name ? src->name : "?", err);
        return 0;
    }
    return 1;
}

static void test_a4_square_zero_crossings_and_peak(void) {
    music_src_t src = {
        "beep", 120, { "A4:16", "-:16", "-:16", "-:16" }
    };
    music_track_t tr;
    CHECK(parse_ok(&src, &tr));

    synth_t s;
    synth_init(&s, SR);
    synth_set_track(&s, &tr);

    int16_t buf[SR];
    synth_render(&s, buf, SR);

    int crossings = 0;
    int peak = 0;
    for (int i = 0; i < SR; i++) {
        int a = buf[i] < 0 ? -buf[i] : buf[i];
        if (a > peak) peak = a;
        if (i > 0) {
            if ((buf[i - 1] > 0 && buf[i] < 0) || (buf[i - 1] < 0 && buf[i] > 0)) crossings++;
        }
    }
    CHECK(crossings >= 860 && crossings <= 900);
    CHECK(peak > 8000);
}

static void test_volume_zero_is_silence(void) {
    music_src_t src = {
        "beep", 120, { "A4:16", "-:16", "-:16", "-:16" }
    };
    music_track_t tr;
    CHECK(parse_ok(&src, &tr));

    synth_t s;
    synth_init(&s, SR);
    synth_set_track(&s, &tr);
    synth_set_volume(&s, 0);

    int16_t buf[SR];
    synth_render(&s, buf, SR);
    for (int i = 0; i < SR; i++) CHECK(buf[i] == 0);
}

static void test_music_mute_leaves_sfx(void) {
    music_src_t src = {
        "beep", 120, { "A4:16", "-:16", "-:16", "-:16" }
    };
    music_track_t tr;
    CHECK(parse_ok(&src, &tr));

    synth_t s;
    synth_init(&s, SR);
    synth_set_track(&s, &tr);
    synth_set_music_enabled(&s, false);
    synth_play_sfx(&s, SFX_HIT);

    int n40 = SR * 40 / 1000;
    int n100 = SR * 100 / 1000;
    int n = n100 + 512;
    int16_t buf[SR];
    synth_render(&s, buf, n);

    int any = 0;
    for (int i = 0; i < n40; i++) if (buf[i] != 0) any = 1;
    CHECK(any);
    for (int i = n100; i < n; i++) CHECK(buf[i] == 0);
}

static void test_every_sfx_is_audible(void) {
    synth_t s;
    synth_init(&s, SR);
    synth_set_track(&s, NULL);
    int16_t buf[SR / 2];
    for (int id = 1; id < SFX_COUNT; id++) {
        synth_init(&s, SR);
        synth_set_track(&s, NULL);
        synth_play_sfx(&s, (sfx_id_t)id);
        synth_render(&s, buf, SR / 2);
        int any = 0;
        for (int i = 0; i < SR / 2; i++) if (buf[i] != 0) any = 1;
        CHECK(any);
    }
}

static void test_every_track_stays_in_range_and_keeps_sounding(void) {
    enum { WIN = SR / 2 };
    int16_t buf[512];
    for (int i = 0; i < MUSIC_TRACK_COUNT; i++) {
        music_track_t tr;
        CHECK(parse_ok(&MUSIC_SRC[i], &tr));
        synth_t s;
        synth_init(&s, SR);
        synth_set_track(&s, &tr);

        int win_nonzero = 0;
        int win_pos = 0;
        int total = SR * 10;
        int n = 0;
        while (n < total) {
            int chunk = 512;
            if (n + chunk > total) chunk = total - n;
            synth_render(&s, buf, chunk);
            for (int k = 0; k < chunk; k++) {
                if (buf[k] != 0) win_nonzero = 1;
                win_pos++;
                if (win_pos == WIN) {
                    CHECK(win_nonzero);
                    win_nonzero = 0;
                    win_pos = 0;
                }
            }
            n += chunk;
        }
    }
}

int main(void) {
    RUN(test_a4_square_zero_crossings_and_peak);
    RUN(test_volume_zero_is_silence);
    RUN(test_music_mute_leaves_sfx);
    RUN(test_every_sfx_is_audible);
    RUN(test_every_track_stays_in_range_and_keeps_sounding);
    HARNESS_MAIN_END();
}
