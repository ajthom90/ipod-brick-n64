#include "harness.h"
#include "games/blocks.h"

static void zero_in(input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
}

static void tick(blocks_t *g, const input_t *in, int n) {
    input_t z[GAME_MAX_PLAYERS];
    zero_in(z);
    if (!in) in = z;
    for (int i = 0; i < n; i++) blocks_update(g, in);
}

static void play_start(blocks_t *g, uint32_t seed) {
    blocks_start(g, seed);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    blocks_update(g, in);
}

static void test_shapes_four_cells_and_cycle(void) {
    for (int p = 0; p < BLK_PIECE_COUNT; p++) {
        int mask0 = 0;
        for (int r = 0; r < 4; r++) {
            int mask = 0;
            for (int i = 0; i < 4; i++) {
                int x = BLK_SHAPES[p][r][i][0];
                int y = BLK_SHAPES[p][r][i][1];
                CHECK(x >= 0 && x < 4 && y >= 0 && y < 4);
                int bit = 1 << (y * 4 + x);
                CHECK((mask & bit) == 0);
                mask |= bit;
            }
            CHECK(mask != 0);
            if (r == 0) mask0 = mask;
        }
        CHECK(mask0 != 0);
        /* Four rotations wrap: rot 0's cell set equals rot (0+4)%4. */
        {
            int wrap = 0;
            for (int i = 0; i < 4; i++) {
                int x = BLK_SHAPES[p][0][i][0];
                int y = BLK_SHAPES[p][0][i][1];
                wrap |= 1 << (y * 4 + x);
            }
            CHECK(wrap == mask0);
        }
    }
    CHECK(BLK_GRAVITY[0] == 48 && BLK_GRAVITY[19] == 2);
}

static void test_start_title_then_play(void) {
    blocks_t g;
    blocks_init(&g);
    g.high_score = 77;
    blocks_start(&g, 0x1234567u);
    CHECK(g.state == BLK_ST_TITLE);
    CHECK(g.level == 1);
    CHECK(g.score == 0 && g.lines == 0);
    CHECK(g.high_score == 77);
    for (int r = 0; r < BLK_ROWS; r++)
        for (int c = 0; c < BLK_COLS; c++)
            CHECK(g.cells[r][c] == 0);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    blocks_update(&g, in);
    CHECK(g.state == BLK_ST_PLAY);
    CHECK(g.px == 3 && g.py == 0);
    CHECK(g.rot == 0);
}

static void test_gravity_level1(void) {
    blocks_t g;
    play_start(&g, 1);
    CHECK(g.py == 0);
    tick(&g, NULL, 47);
    CHECK(g.py == 0);
    tick(&g, NULL, 1);
    CHECK(g.py == 1);
}

static void test_das_left(void) {
    blocks_t g;
    play_start(&g, 1);
    CHECK(g.px == 3);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_x = -1;
    blocks_update(&g, in);
    CHECK(g.px == 2);
    for (int i = 0; i < 15; i++) {
        blocks_update(&g, in);
        CHECK(g.px == 2);
    }
    blocks_update(&g, in);
    CHECK(g.px == 1);
    for (int i = 0; i < 5; i++) {
        blocks_update(&g, in);
        CHECK(g.px == 1);
    }
    blocks_update(&g, in);
    CHECK(g.px == 0);
}

static void test_wall_kick_i(void) {
    blocks_t g;
    play_start(&g, 1);
    g.piece = BLK_I;
    g.rot = 1;
    g.px = -2;
    g.py = 5;
    CHECK(blocks_fits(&g, g.piece, g.rot, g.px, g.py));
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    blocks_update(&g, in);
    CHECK(g.rot == 2);
    CHECK(blocks_fits(&g, g.piece, g.rot, g.px, g.py));
}

static void test_soft_drop(void) {
    blocks_t g;
    play_start(&g, 1);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = -1;
    tick(&g, in, 2);
    CHECK(g.py == 1);
    tick(&g, in, 2);
    CHECK(g.py == 2);
    tick(&g, in, 2);
    CHECK(g.py == 3);
}

static void test_hard_drop_lock_o(void) {
    blocks_t g;
    play_start(&g, 1);
    g.piece = BLK_O;
    g.rot = 0;
    g.px = 3;
    g.py = 0;
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = 1;
    blocks_update(&g, in);
    CHECK(g.cells[19][4] == BLK_O + 1 && g.cells[19][5] == BLK_O + 1);
    CHECK(g.cells[18][4] == BLK_O + 1 && g.cells[18][5] == BLK_O + 1);
    CHECK(sfx_pop(&g.sfx) == SFX_HIT);
    CHECK(g.state == BLK_ST_PLAY);
    CHECK(g.py == 0);
}

static void fill_row_cols(blocks_t *g, int row, int c0, int c1, uint8_t v) {
    for (int c = c0; c <= c1; c++) g->cells[row][c] = v;
}

static void test_line_clear_four(void) {
    blocks_t g;
    play_start(&g, 1);
    for (int r = 16; r <= 19; r++) fill_row_cols(&g, r, 1, 9, 1);
    g.piece = BLK_I;
    g.rot = 1;
    g.px = -2;
    g.py = 0;
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = 1;
    blocks_update(&g, in);
    CHECK(g.state == BLK_ST_FLASH);
    CHECK(sfx_pop(&g.sfx) == SFX_HIT);
    CHECK(sfx_pop(&g.sfx) == SFX_CLEAR);
    tick(&g, NULL, BLK_FLASH_TICKS);
    CHECK(g.lines == 4);
    CHECK(g.score == 1200);
    CHECK(g.level == 1);
    for (int r = 16; r <= 19; r++)
        for (int c = 0; c < BLK_COLS; c++)
            CHECK(g.cells[r][c] == 0);
}

static void setup_one_line_left_col9(blocks_t *g) {
    fill_row_cols(g, 19, 0, 8, 1);
    g->piece = BLK_I;
    g->rot = 1;
    g->px = 7;
    g->py = 0;
}

static void test_scoring(void) {
    blocks_t g;
    play_start(&g, 1);
    setup_one_line_left_col9(&g);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = 1;
    blocks_update(&g, in);
    CHECK(g.state == BLK_ST_FLASH);
    tick(&g, NULL, BLK_FLASH_TICKS);
    CHECK(g.lines == 1);
    CHECK(g.score == 40);
    CHECK(g.level == 1);

    play_start(&g, 1);
    g.lines = 20;
    g.level = 3;
    g.score = 0;
    setup_one_line_left_col9(&g);
    zero_in(in);
    in[0].dpad_y = 1;
    blocks_update(&g, in);
    tick(&g, NULL, BLK_FLASH_TICKS);
    CHECK(g.score == 120);
}

static void test_level_up(void) {
    blocks_t g;
    play_start(&g, 1);
    g.lines = 9;
    g.level = 1;
    setup_one_line_left_col9(&g);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = 1;
    blocks_update(&g, in);
    tick(&g, NULL, BLK_FLASH_TICKS);
    CHECK(g.lines == 10);
    CHECK(g.level == 2);
    CHECK(g.gravity_ticks == 43);
}

static void test_game_over(void) {
    blocks_t g;
    blocks_start(&g, 1);
    g.score = 50;
    g.high_score = 0;
    for (int r = 1; r < BLK_ROWS; r++)
        for (int c = 0; c < BLK_COLS; c++)
            g.cells[r][c] = 1;
    blocks_spawn(&g);
    CHECK(g.state == BLK_ST_GAMEOVER);
    CHECK(sfx_pop(&g.sfx) == SFX_GAME_OVER);
    CHECK(g.high_score == 50);
}

static void test_seven_bag(void) {
    blocks_t g;
    blocks_init(&g);
    prng_seed(&g.rng, 0x1234567u);
    int counts[BLK_PIECE_COUNT];
    memset(counts, 0, sizeof counts);
    for (int i = 0; i < 14; i++) {
        blk_piece_t p = blocks_bag_next(&g);
        CHECK(p >= 0 && p < BLK_PIECE_COUNT);
        counts[p]++;
    }
    for (int i = 0; i < BLK_PIECE_COUNT; i++) CHECK(counts[i] == 2);
}

static void test_autoplay_ends(void) {
    blocks_t g;
    blocks_init(&g);
    blocks_start(&g, 0x1234567u);
    input_t in[GAME_MAX_PLAYERS];
    int t = 0;
    while (g.state != BLK_ST_GAMEOVER && t < 20000) {
        blocks_autoplay(&g, in);
        blocks_update(&g, in);
        t++;
    }
    printf("\n    game over after %d ticks, lines=%d\n%-52s", t, g.lines, "");
    CHECK(g.state == BLK_ST_GAMEOVER);
    CHECK(t <= 20000);
    CHECK(g.lines >= 4);
}

static void test_descriptor(void) {
    CHECK(strcmp(GAME_BLOCKS.name, "BLOCKS") == 0);
    CHECK(GAME_BLOCKS.track == MUSIC_BLOCKS);
    CHECK(GAME_BLOCKS.players == 1);
    blocks_t *g = GAME_BLOCKS.state;
    GAME_BLOCKS.init(g);
    GAME_BLOCKS.set_high_score(g, 99);
    GAME_BLOCKS.start(g, 1);
    CHECK(g->state == BLK_ST_TITLE && GAME_BLOCKS.get_high_score(g) == 99);
    CHECK(!GAME_BLOCKS.is_over(g));
    GAME_BLOCKS.set_high_score(g, 123);
    CHECK(GAME_BLOCKS.get_high_score(g) == 123);
}

int main(void) {
    RUN(test_shapes_four_cells_and_cycle);
    RUN(test_start_title_then_play);
    RUN(test_gravity_level1);
    RUN(test_das_left);
    RUN(test_wall_kick_i);
    RUN(test_soft_drop);
    RUN(test_hard_drop_lock_o);
    RUN(test_line_clear_four);
    RUN(test_scoring);
    RUN(test_level_up);
    RUN(test_game_over);
    RUN(test_seven_bag);
    RUN(test_autoplay_ends);
    RUN(test_descriptor);
    HARNESS_MAIN_END();
}
