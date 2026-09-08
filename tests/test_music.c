#include "harness.h"
#include "music.h"
#include <stdint.h>

static void test_a4_is_440_q8(void) {
    CHECK(music_note_freq_q8(69) == (440u << 8));
}

static void test_a3_is_220_q8(void) {
    CHECK(music_note_freq_q8(57) == (220u << 8));
}

static void test_parse_headers_notes_and_bars(void) {
    music_chparms_t parms;
    music_event_t events[MUSIC_MAX_EVENTS];
    int nevents = 0, steps = 0;
    char err[128];
    memset(&parms, 0, sizeof parms);
    CHECK(music_parse_channel("duty=25 gain=40 C4:4 -:4 E4:8 | G4:16",
                              &parms, events, &nevents, &steps, err, sizeof err));
    CHECK(parms.duty == 25);
    CHECK(parms.gain == 40);
    CHECK(nevents == 4);
    CHECK(steps == 32);
    CHECK(events[0].note == 60 && events[0].len == 4);
    CHECK(events[1].note == 0 && events[1].len == 4);
    CHECK(events[2].note == 64 && events[2].len == 8);
    CHECK(events[3].note == 67 && events[3].len == 16);
}

static void test_bar_of_15_steps_fails(void) {
    music_chparms_t parms;
    music_event_t events[MUSIC_MAX_EVENTS];
    int nevents = 0, steps = 0;
    char err[128];
    memset(err, 0, sizeof err);
    CHECK(!music_parse_channel("C4:8 E4:7 |", &parms, events, &nevents, &steps, err, sizeof err));
    CHECK(strstr(err, "bar") != NULL);
}

static void test_unknown_token_fails(void) {
    music_chparms_t parms;
    music_event_t events[MUSIC_MAX_EVENTS];
    int nevents = 0, steps = 0;
    char err[128];
    CHECK(!music_parse_channel("C4:4 Q4", &parms, events, &nevents, &steps, err, sizeof err));
}

static void test_header_after_note_fails(void) {
    music_chparms_t parms;
    music_event_t events[MUSIC_MAX_EVENTS];
    int nevents = 0, steps = 0;
    char err[128];
    CHECK(!music_parse_channel("C4:4 duty=25", &parms, events, &nevents, &steps, err, sizeof err));
}

static void test_all_tracks_parse_equal_lengths(void) {
    CHECK(MUSIC_SRC[MUSIC_MENU].bpm == 100);
    for (int i = 0; i < MUSIC_TRACK_COUNT; i++) {
        music_track_t tr;
        char err[128];
        memset(err, 0, sizeof err);
        if (!music_parse(&MUSIC_SRC[i], &tr, err, sizeof err)) {
            fprintf(stderr, "  parse %s: %s\n", MUSIC_SRC[i].name, err);
            CHECK(0 && "music_parse");
        }
        CHECK(tr.steps > 0);
        CHECK(tr.bpm == MUSIC_SRC[i].bpm);
        for (int ch = 0; ch < MUSIC_CHANNELS; ch++) {
            int sum = 0;
            for (int e = 0; e < tr.nevents[ch]; e++) sum += tr.events[ch][e].len;
            CHECK(sum == tr.steps);
        }
    }
}

int main(void) {
    RUN(test_a4_is_440_q8);
    RUN(test_a3_is_220_q8);
    RUN(test_parse_headers_notes_and_bars);
    RUN(test_bar_of_15_steps_fails);
    RUN(test_unknown_token_fails);
    RUN(test_header_after_note_fails);
    RUN(test_all_tracks_parse_equal_lengths);
    HARNESS_MAIN_END();
}
