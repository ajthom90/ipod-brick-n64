#include "harness.h"
#include "app_state.h"
#include "games/brick.h"
#include "games/registry.h"

static void zero_in(input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
}

static void drain_sfx(app_t *a) {
    while (app_next_sfx(a) != SFX_NONE) {}
}

static void test_init_is_menu(void) {
    app_t a;
    app_init(&a, false, 0);
    CHECK(a.screen == APP_MENU);
    CHECK(a.menu_row == 0);
    CHECK(app_track(&a) == MUSIC_MENU);
}

static void test_menu_nav_and_repeat(void) {
    app_t a;
    app_init(&a, false, 0);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);

    in[0].dpad_y = -1;
    app_update(&a, in, false, 1);
    CHECK(a.menu_row == 1);
    CHECK(a.repeat_ticks == 18);
    CHECK(app_next_sfx(&a) == SFX_MENU_MOVE);
    CHECK(app_next_sfx(&a) == SFX_NONE);

    /* Holding: no further move for 17 more ticks. */
    for (int i = 0; i < 17; i++) {
        app_update(&a, in, false, 1);
        CHECK(a.menu_row == 1);
        CHECK(a.repeat_ticks == 17 - i);
        CHECK(app_next_sfx(&a) == SFX_NONE);
    }

    /* A move on the 18th (clamped at SETTINGS with one registered game). */
    app_update(&a, in, false, 1);
    CHECK(a.menu_row == GAME_COUNT);
    CHECK(a.repeat_ticks == 8);
    CHECK(app_next_sfx(&a) == SFX_NONE);

    /* Then every 8 ticks. */
    for (int i = 0; i < 7; i++) {
        app_update(&a, in, false, 1);
        CHECK(a.menu_row == GAME_COUNT);
        CHECK(a.repeat_ticks == 7 - i);
    }
    app_update(&a, in, false, 1);
    CHECK(a.menu_row == GAME_COUNT);
    CHECK(a.repeat_ticks == 8);

    /* Clamp at GAME_COUNT while holding down. */
    for (int i = 0; i < 20; i++) app_update(&a, in, false, 1);
    CHECK(a.menu_row == GAME_COUNT);

    /* Clamp at 0: release, move up to Brick, keep holding up. */
    zero_in(in);
    app_update(&a, in, false, 1);
    drain_sfx(&a);
    in[0].dpad_y = 1;
    app_update(&a, in, false, 1);
    CHECK(a.menu_row == 0);
    CHECK(app_next_sfx(&a) == SFX_MENU_MOVE);
    for (int i = 0; i < 30; i++) app_update(&a, in, false, 1);
    CHECK(a.menu_row == 0);

    /* stick_y = -200 behaves like the D-pad (down). */
    zero_in(in);
    app_update(&a, in, false, 1);
    drain_sfx(&a);
    in[0].stick_y = -200;
    app_update(&a, in, false, 1);
    CHECK(a.menu_row == 1);
    CHECK(app_next_sfx(&a) == SFX_MENU_MOVE);
}

static void test_a_launches_brick(void) {
    app_t a;
    app_init(&a, false, 0);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    app_update(&a, in, false, 1);
    CHECK(a.screen == APP_GAME);
    CHECK(a.game_index == 0);
    CHECK(((brick_game_t *)GAME_BRICK.state)->state == BRICK_ST_TITLE);
    CHECK(app_track(&a) == MUSIC_BRICK);
    CHECK(app_next_sfx(&a) == SFX_MENU_SELECT);
    CHECK(app_next_sfx(&a) == SFX_NONE);
}

static void test_pause_menu(void) {
    app_t a;
    app_init(&a, false, 0);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    app_update(&a, in, false, 1);
    CHECK(a.screen == APP_GAME);
    drain_sfx(&a);

    brick_game_t *g = GAME_BRICK.state;
    zero_in(in);
    app_update(&a, in, true, 1);
    CHECK(a.screen == APP_PAUSE);
    CHECK(a.pause_row == 0);

    uint32_t frozen = g->ticks;
    for (int i = 0; i < 10; i++) app_update(&a, in, false, 1);
    CHECK(g->ticks == frozen);
    CHECK(a.screen == APP_PAUSE);

    /* Start while paused resumes. */
    app_update(&a, in, true, 1);
    CHECK(a.screen == APP_GAME);

    /* A on RESUME returns to the game. */
    app_update(&a, in, true, 1);
    CHECK(a.screen == APP_PAUSE);
    in[0].a = true;
    app_update(&a, in, false, 1);
    CHECK(a.screen == APP_GAME);
    drain_sfx(&a);

    /* dpad down then A quits to the menu, still highlighting Brick. */
    zero_in(in);
    app_update(&a, in, true, 1);
    CHECK(a.screen == APP_PAUSE);
    CHECK(a.pause_row == 0);
    in[0].dpad_y = -1;
    app_update(&a, in, false, 1);
    CHECK(a.pause_row == 1);
    zero_in(in);
    in[0].a = true;
    app_update(&a, in, false, 1);
    CHECK(a.screen == APP_MENU);
    CHECK(a.menu_row == 0);
}

static void test_settings_placeholder(void) {
    app_t a;
    app_init(&a, false, 0);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = -1;
    app_update(&a, in, false, 1);
    CHECK(a.menu_row == GAME_COUNT);
    drain_sfx(&a);

    zero_in(in);
    in[0].a = true;
    app_update(&a, in, false, 1);
    CHECK(a.screen == APP_SETTINGS);
    CHECK(app_track(&a) == MUSIC_MENU);

    /* A on BACK returns to the menu. */
    app_update(&a, in, false, 1);
    CHECK(a.screen == APP_MENU);
    CHECK(a.menu_row == GAME_COUNT);
}

static void test_autoplay_boots_into_game(void) {
    app_t a;
    app_init(&a, true, 0);
    CHECK(a.screen == APP_GAME);
    CHECK(a.game_index == 0);
    CHECK(a.autoplay);
    CHECK(((brick_game_t *)GAME_BRICK.state)->state == BRICK_ST_TITLE);

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    app_update(&a, in, true, 1);
    CHECK(a.screen == APP_GAME);
}

static void test_game_over_sets_high_score_dirty_once(void) {
    app_t a;
    app_init(&a, true, 0);
    CHECK(!a.high_score_dirty);

    input_t in[GAME_MAX_PLAYERS];
    int became = 0;
    bool prev = false;
    int t;
    for (t = 0; t < 60000; t++) {
        zero_in(in);
        app_update(&a, in, false, 1);
        if (a.high_score_dirty && !prev) became++;
        prev = a.high_score_dirty;
        if (GAME_BRICK.is_over(GAME_BRICK.state) && a.high_score_dirty) break;
    }
    CHECK(GAME_BRICK.is_over(GAME_BRICK.state));
    CHECK(a.high_score_dirty);
    CHECK(became == 1);

    /* Further ticks leave the flag set and do not pulse it. */
    for (int i = 0; i < 10; i++) {
        zero_in(in);
        app_update(&a, in, false, 1);
        CHECK(a.high_score_dirty);
    }
    CHECK(became == 1);
}

static void test_next_sfx_framework_then_game(void) {
    app_t a;
    app_init(&a, false, 0);
    CHECK(app_next_sfx(&a) == SFX_NONE);

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    app_update(&a, in, false, 1);
    CHECK(a.screen == APP_GAME);

    sfx_push(&a.sfx, SFX_MENU_MOVE);
    sfx_push(GAME_BRICK.sfx(GAME_BRICK.state), SFX_HIT);

    CHECK(app_next_sfx(&a) == SFX_MENU_SELECT);
    CHECK(app_next_sfx(&a) == SFX_MENU_MOVE);
    CHECK(app_next_sfx(&a) == SFX_HIT);
    CHECK(app_next_sfx(&a) == SFX_NONE);
}

int main(void) {
    RUN(test_init_is_menu);
    RUN(test_menu_nav_and_repeat);
    RUN(test_a_launches_brick);
    RUN(test_pause_menu);
    RUN(test_settings_placeholder);
    RUN(test_autoplay_boots_into_game);
    RUN(test_game_over_sets_high_score_dirty_once);
    RUN(test_next_sfx_framework_then_game);
    HARNESS_MAIN_END();
}
