#include "harness.h"
#include "sfx.h"
#include "game.h"

static void test_pop_empty_is_none(void) {
    sfx_queue_t q;
    memset(&q, 0, sizeof q);
    CHECK(sfx_pop(&q) == SFX_NONE);
}

static void test_push_seven_pop_in_order(void) {
    sfx_queue_t q;
    memset(&q, 0, sizeof q);
    sfx_id_t ids[7] = { SFX_BOUNCE, SFX_HIT, SFX_CLEAR, SFX_FOOD, SFX_SHOT, SFX_EXPLODE, SFX_POINT };
    for (int i = 0; i < 7; i++) sfx_push(&q, ids[i]);
    for (int i = 0; i < 7; i++) CHECK(sfx_pop(&q) == ids[i]);
    CHECK(sfx_pop(&q) == SFX_NONE);
}

static void test_eighth_push_is_dropped(void) {
    sfx_queue_t q;
    memset(&q, 0, sizeof q);
    for (int i = 1; i <= 7; i++) sfx_push(&q, (sfx_id_t)i);
    sfx_push(&q, SFX_MERGE);
    for (int i = 1; i <= 7; i++) CHECK(sfx_pop(&q) == (sfx_id_t)i);
    CHECK(sfx_pop(&q) == SFX_NONE);
}

static void test_fmt_label_values_and_truncation(void) {
    char buf[32];
    fmt_label(buf, sizeof buf, "SCORE", 0);
    CHECK(strcmp(buf, "SCORE 0") == 0);
    fmt_label(buf, sizeof buf, "LV", 12);
    CHECK(strcmp(buf, "LV 12") == 0);
    fmt_label(buf, sizeof buf, "X", -7);
    CHECK(strcmp(buf, "X -7") == 0);
    char tiny[4];
    memset(tiny, 'Z', sizeof tiny);
    fmt_label(tiny, 4, "SCORE", 0);
    CHECK(tiny[3] == '\0');
    CHECK(strcmp(tiny, "SCO") == 0);
}

int main(void) {
    RUN(test_pop_empty_is_none);
    RUN(test_push_seven_pop_in_order);
    RUN(test_eighth_push_is_dropped);
    RUN(test_fmt_label_values_and_truncation);
    HARNESS_MAIN_END();
}
