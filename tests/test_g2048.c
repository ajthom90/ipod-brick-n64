#include "harness.h"
#include "games/g2048.h"

static void zero_in(input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
}

static void tick(g2048_t *g, const input_t *in, int n) {
    input_t z[GAME_MAX_PLAYERS];
    zero_in(z);
    if (!in) in = z;
    for (int i = 0; i < n; i++) g2048_update(g, in);
}

static void drain_sfx(g2048_t *g) {
    while (sfx_pop(&g->sfx) != SFX_NONE) {}
}

static void play_start(g2048_t *g, uint32_t seed) {
    g2048_init(g);
    g2048_start(g, seed);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    g2048_update(g, in);
}

static int ntiles(const g2048_t *g) {
    int n = 0;
    for (int r = 0; r < G2048_N; r++)
        for (int c = 0; c < G2048_N; c++)
            if (g->board[r][c]) n++;
    return n;
}

static void clear_board(g2048_t *g) {
    memset(g->board, 0, sizeof g->board);
}

static void fill_checker(g2048_t *g) {
    for (int r = 0; r < G2048_N; r++) {
        for (int c = 0; c < G2048_N; c++)
            g->board[r][c] = ((r + c) & 1) ? 2 : 4;
    }
}

static void test_slide_row(void) {
    uint16_t row[G2048_N];
    int gained;

    row[0] = 2; row[1] = 2; row[2] = 2; row[3] = 2;
    gained = 0;
    CHECK(g2048_slide_row(row, &gained) == true);
    CHECK(row[0] == 4 && row[1] == 4 && row[2] == 0 && row[3] == 0);
    CHECK(gained == 8);

    row[0] = 2; row[1] = 0; row[2] = 2; row[3] = 4;
    gained = 0;
    CHECK(g2048_slide_row(row, &gained) == true);
    CHECK(row[0] == 4 && row[1] == 4 && row[2] == 0 && row[3] == 0);
    CHECK(gained == 4);

    row[0] = 4; row[1] = 4; row[2] = 4; row[3] = 0;
    gained = 0;
    CHECK(g2048_slide_row(row, &gained) == true);
    CHECK(row[0] == 8 && row[1] == 4 && row[2] == 0 && row[3] == 0);
    CHECK(gained == 8);

    row[0] = 2; row[1] = 4; row[2] = 8; row[3] = 16;
    gained = 0;
    CHECK(g2048_slide_row(row, &gained) == false);
    CHECK(row[0] == 2 && row[1] == 4 && row[2] == 8 && row[3] == 16);
    CHECK(gained == 0);
}

static void test_start_keeps_high_score(void) {
    g2048_t g;
    g2048_init(&g);
    g.high_score = 42;
    g2048_start(&g, 0x1234567u);
    CHECK(g.state == G2048_ST_TITLE);
    CHECK(g.high_score == 42);
    CHECK(g.score == 0);
    CHECK(g.reached_2048 == false);
    CHECK(ntiles(&g) == 2);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    g2048_update(&g, in);
    CHECK(g.state == G2048_ST_PLAY);
    CHECK(g.high_score == 42);
}

static void test_move_right_merges(void) {
    g2048_t g;
    play_start(&g, 1);
    clear_board(&g);
    g.board[0][0] = 2;
    g.board[0][3] = 2;
    g.score = 0;
    CHECK(g2048_move(&g, 1) == true);
    CHECK(g.board[0][3] == 4);
    CHECK(g.score == 4);
    CHECK(ntiles(&g) == 2);
}

static void test_changing_move_adds_one_tile(void) {
    g2048_t g;
    play_start(&g, 1);
    clear_board(&g);
    g.board[0][3] = 2;
    int before = ntiles(&g);
    CHECK(before == 1);
    CHECK(g2048_move(&g, 0) == true);
    CHECK(g.board[0][0] == 2);
    CHECK(ntiles(&g) == before + 1);
}

static void test_nonchanging_move_adds_none(void) {
    g2048_t g;
    play_start(&g, 1);
    clear_board(&g);
    g.board[0][0] = 2;
    g.board[0][1] = 4;
    g.board[0][2] = 8;
    g.board[0][3] = 16;
    int before = ntiles(&g);
    CHECK(g2048_move(&g, 0) == false);
    CHECK(ntiles(&g) == before);
    CHECK(g.board[0][0] == 2 && g.board[0][1] == 4);
    CHECK(g.board[0][2] == 8 && g.board[0][3] == 16);
}

static void test_add_tile_four_rate(void) {
    int fours = 0;
    for (int i = 0; i < 1000; i++) {
        g2048_t g;
        g2048_init(&g);
        prng_seed(&g.rng, (uint32_t)(i + 1));
        g2048_add_tile(&g);
        int found = 0;
        for (int r = 0; r < G2048_N; r++) {
            for (int c = 0; c < G2048_N; c++) {
                if (g.board[r][c] == 4) fours++;
                if (g.board[r][c]) found++;
            }
        }
        CHECK(found == 1);
    }
    CHECK(fours >= 60 && fours <= 140);
}

static void test_can_move_checkerboard_and_pair(void) {
    g2048_t g;
    play_start(&g, 1);
    fill_checker(&g);
    CHECK(g2048_can_move(&g) == false);
    g.board[0][0] = g.board[0][1];
    CHECK(g2048_can_move(&g) == true);
}

static void test_game_over_sfx(void) {
    g2048_t g;
    play_start(&g, 1);
    fill_checker(&g);
    g.score = 50;
    g.high_score = 10;
    drain_sfx(&g);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_x = -1;
    tick(&g, in, 1);
    CHECK(g.state == G2048_ST_GAMEOVER);
    CHECK(g.high_score == 50);
    CHECK(sfx_pop(&g.sfx) == SFX_GAME_OVER);
}

static void test_reached_2048_clear_sfx(void) {
    g2048_t g;
    play_start(&g, 1);
    clear_board(&g);
    g.board[0][0] = 1024;
    g.board[0][1] = 1024;
    g.score = 0;
    g.reached_2048 = false;
    drain_sfx(&g);
    CHECK(g2048_move(&g, 0) == true);
    CHECK(g.board[0][0] == 2048);
    CHECK(g.reached_2048 == true);
    CHECK(g.score == 2048);
    CHECK(sfx_pop(&g.sfx) == SFX_MERGE);
    CHECK(sfx_pop(&g.sfx) == SFX_CLEAR);
    drain_sfx(&g);
    clear_board(&g);
    g.board[1][0] = 1024;
    g.board[1][1] = 1024;
    CHECK(g2048_move(&g, 0) == true);
    CHECK(g.reached_2048 == true);
    CHECK(sfx_pop(&g.sfx) == SFX_MERGE);
    CHECK(sfx_pop(&g.sfx) == SFX_NONE);
}

static void test_stick_requires_rearming(void) {
    g2048_t g;
    play_start(&g, 1);
    clear_board(&g);
    g.board[0][0] = 2;
    g.board[0][3] = 2;
    g.stick_armed = true;
    drain_sfx(&g);

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].stick_x = 200;
    tick(&g, in, 1);
    CHECK(g.board[0][3] == 4);
    CHECK(g.stick_armed == false);

    uint16_t snap[G2048_N][G2048_N];
    memcpy(snap, g.board, sizeof snap);
    tick(&g, in, 1);
    CHECK(memcmp(g.board, snap, sizeof snap) == 0);
    CHECK(g.stick_armed == false);

    zero_in(in);
    tick(&g, in, 1);
    CHECK(g.stick_armed == true);
}

static void test_autoplay_ends(void) {
    g2048_t g;
    g2048_init(&g);
    g2048_start(&g, 0x1234567u);
    input_t in[GAME_MAX_PLAYERS];
    int t = 0;
    while (g.state != G2048_ST_GAMEOVER && t < 200000) {
        g2048_autoplay(&g, in);
        g2048_update(&g, in);
        t++;
    }
    printf("\n    game over after %d ticks, score=%d high=%d\n%-52s",
           t, g.score, g.high_score, "");
    CHECK(g.state == G2048_ST_GAMEOVER);
    CHECK(t <= 200000);
    CHECK(g.score > 100);
}

static void test_descriptor(void) {
    CHECK(strcmp(GAME_2048.name, "2048") == 0);
    CHECK(GAME_2048.track == MUSIC_2048);
    CHECK(GAME_2048.players == 1);
    g2048_t *g = GAME_2048.state;
    GAME_2048.init(g);
    GAME_2048.set_high_score(g, 99);
    GAME_2048.start(g, 1);
    CHECK(g->state == G2048_ST_TITLE && GAME_2048.get_high_score(g) == 99);
    CHECK(!GAME_2048.is_over(g));
    GAME_2048.set_high_score(g, 123);
    CHECK(GAME_2048.get_high_score(g) == 123);
}

int main(void) {
    RUN(test_slide_row);
    RUN(test_start_keeps_high_score);
    RUN(test_move_right_merges);
    RUN(test_changing_move_adds_one_tile);
    RUN(test_nonchanging_move_adds_none);
    RUN(test_add_tile_four_rate);
    RUN(test_can_move_checkerboard_and_pair);
    RUN(test_game_over_sfx);
    RUN(test_reached_2048_clear_sfx);
    RUN(test_stick_requires_rearming);
    RUN(test_autoplay_ends);
    RUN(test_descriptor);
    HARNESS_MAIN_END();
}
