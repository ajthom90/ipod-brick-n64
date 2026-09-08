#include <stdio.h>
#include <string.h>
#include "brick.h"

static int failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { failures++; fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); return; } \
} while (0)

#define RUN(fn) do { \
    printf("%-44s", #fn); fflush(stdout); \
    int before = failures; fn(); \
    puts(before == failures ? "ok" : "FAILED"); \
} while (0)

/* Advance n ticks with the given input (NULL = no input). */
static void tick(brick_game_t *g, const brick_input_t *in, int n) {
    brick_input_t zero; memset(&zero, 0, sizeof zero);
    if (!in) in = &zero;
    for (int i = 0; i < n; i++) brick_update(g, in);
}

static void test_init_is_title(void) {
    brick_game_t g; brick_init(&g);
    CHECK(g.state == BRICK_ST_TITLE);
    CHECK(g.score == 0);
    CHECK(g.high_score == 0);
    CHECK(g.ticks == 0);
    tick(&g, NULL, 3);
    CHECK(g.ticks == 3);
}

static void test_cell_rects(void) {
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            brick_rect_t rc = brick_cell_rect(r, c);
            CHECK(rc.x1 - rc.x0 == BRICK_W);
            CHECK(rc.y1 - rc.y0 == BRICK_H);
            CHECK(rc.x0 % 2 == 0);
            CHECK(rc.x0 >= BRICK_PLAY_X0 && rc.x1 <= BRICK_PLAY_X1);
            CHECK(rc.y0 >= BRICK_PLAY_Y0 && rc.y1 <= BRICK_PLAY_Y1);
        }
    }
    CHECK(brick_cell_rect(0, 0).x0 == BRICK_GRID_X0);
    CHECK(brick_cell_rect(0, 0).y0 == BRICK_GRID_Y0);
    CHECK(brick_cell_rect(0, 1).x0 == BRICK_GRID_X0 + BRICK_W + BRICK_GAP_X);
    CHECK(brick_cell_rect(1, 0).y0 == BRICK_GRID_Y0 + BRICK_H + BRICK_GAP_Y);
    CHECK(brick_cell_rect(0, BRICK_COLS - 1).x1 == 298);
    CHECK(brick_cell_rect(BRICK_ROWS - 1, 0).y1 == 107);
}

static void test_row_colors(void) {
    CHECK(brick_row_color(0) == 0xC4472Au);
    CHECK(brick_row_color(1) == 0xE07A1Fu);
    CHECK(brick_row_color(2) == 0xD4B01Cu);
    CHECK(brick_row_color(3) == 0x3FA34Du);
    CHECK(brick_row_color(4) == 0x2E6DB4u);
    CHECK(brick_row_color(5) == 0x7B4EA3u);
}

static void test_paddle_and_ball_rects(void) {
    brick_game_t g; brick_init(&g);
    g.paddle_x = 100; g.ball_x = 50 << 8; g.ball_y = 60 << 8;
    brick_rect_t p = brick_paddle_rect(&g), b = brick_ball_rect(&g);
    CHECK(p.x0 == 100 && p.x1 == 148 && p.y0 == BRICK_PADDLE_Y && p.y1 == BRICK_PADDLE_Y + BRICK_PADDLE_H);
    CHECK(b.x0 == 50 && b.x1 == 56 && b.y0 == 60 && b.y1 == 66);
    brick_rect_t a = { 0, 0, 10, 10 }, c = { 10, 0, 20, 10 }, d = { 9, 9, 20, 20 };
    CHECK(!brick_rects_overlap(a, c));
    CHECK(brick_rects_overlap(a, d));
}

static void test_bounce_table_is_unit_length(void) {
    for (int z = 0; z < 7; z++) {
        int32_t s = BRICK_BOUNCE_SIN[z], c = BRICK_BOUNCE_COS[z];
        int32_t len2 = s * s + c * c;          /* should be about 256*256 = 65536 */
        CHECK(len2 > 64000 && len2 < 67000);
    }
    CHECK(BRICK_BOUNCE_SIGN[3] == 0 && BRICK_BOUNCE_SIGN[0] == -1 && BRICK_BOUNCE_SIGN[6] == 1);
}

/* A game that has been started and launched: state PLAY. */
static brick_game_t playing(void) {
    brick_game_t g; brick_init(&g); brick_new_game(&g);
    brick_input_t in; memset(&in, 0, sizeof in); in.launch = true;
    brick_update(&g, &in);
    return g;
}

static void test_new_game_resets(void) {
    brick_game_t g; brick_init(&g); g.high_score = 42; brick_new_game(&g);
    CHECK(g.state == BRICK_ST_SERVE);
    CHECK(g.lives == BRICK_LIVES && g.level == 1 && g.score == 0);
    CHECK(g.high_score == 42);
    CHECK(g.bricks_left == BRICK_ROWS * BRICK_COLS);
    for (int r = 0; r < BRICK_ROWS; r++) for (int c = 0; c < BRICK_COLS; c++) CHECK(g.cells[r][c] == 1);
    CHECK(g.ball_speed == BRICK_BALL_SPEED_BASE);
    CHECK(g.paddle_x == (BRICK_PLAY_X0 + BRICK_PLAY_X1) / 2 - BRICK_PADDLE_W / 2);
    brick_rect_t b = brick_ball_rect(&g), p = brick_paddle_rect(&g);
    CHECK(b.y1 == p.y0);
    CHECK(b.x0 == p.x0 + (BRICK_PADDLE_W - BRICK_BALL_SIZE) / 2);
    CHECK(g.ball_vx == 0 && g.ball_vy == 0);
}

static void test_title_confirm_starts_game(void) {
    brick_game_t g; brick_init(&g);
    brick_input_t in; memset(&in, 0, sizeof in); in.confirm = true;
    brick_update(&g, &in);
    CHECK(g.state == BRICK_ST_SERVE);
    CHECK(g.lives == BRICK_LIVES);
}

static void test_launch_enters_play(void) {
    brick_game_t g = playing();
    CHECK(g.state == BRICK_ST_PLAY);
    CHECK(g.ball_vy == -((BRICK_BALL_SPEED_BASE * BRICK_BOUNCE_COS[3]) >> 8));
    CHECK(g.ball_vx == ((BRICK_BALL_SPEED_BASE * BRICK_BOUNCE_SIN[3]) >> 8));  /* odd level: right */
}

static void test_serve_ball_follows_paddle(void) {
    brick_game_t g; brick_init(&g); brick_new_game(&g);
    int x0 = g.paddle_x;
    brick_input_t in; memset(&in, 0, sizeof in); in.paddle_dir = 1;
    tick(&g, &in, 5);
    CHECK(g.state == BRICK_ST_SERVE);
    CHECK(g.paddle_x == x0 + 5 * BRICK_PADDLE_SPEED_DIGITAL);
    brick_rect_t b = brick_ball_rect(&g), p = brick_paddle_rect(&g);
    CHECK(b.x0 == p.x0 + (BRICK_PADDLE_W - BRICK_BALL_SIZE) / 2);
    CHECK(b.y1 == p.y0);
}

static void test_paddle_clamps_to_playfield(void) {
    brick_game_t g; brick_init(&g); brick_new_game(&g);
    brick_input_t in; memset(&in, 0, sizeof in);
    in.paddle_dir = -1; tick(&g, &in, 200);
    CHECK(g.paddle_x == BRICK_PLAY_X0);
    in.paddle_dir = 1; tick(&g, &in, 200);
    CHECK(g.paddle_x == BRICK_PLAY_X1 - BRICK_PADDLE_W);
}

static void test_analog_overrides_digital(void) {
    brick_game_t g; brick_init(&g); brick_new_game(&g);
    int x0 = g.paddle_x;
    brick_input_t in; memset(&in, 0, sizeof in);
    in.paddle_axis = 256; in.paddle_dir = -1; brick_update(&g, &in);
    CHECK(g.paddle_x == x0 + BRICK_PADDLE_SPEED_ANALOG_MAX);
    in.paddle_axis = -128; in.paddle_dir = 0; brick_update(&g, &in);
    CHECK(g.paddle_x == x0 + BRICK_PADDLE_SPEED_ANALOG_MAX - BRICK_PADDLE_SPEED_ANALOG_MAX / 2);
    in.paddle_axis = 0; brick_update(&g, &in);
    CHECK(g.paddle_x == x0 + BRICK_PADDLE_SPEED_ANALOG_MAX - BRICK_PADDLE_SPEED_ANALOG_MAX / 2);
}

static void test_pause_toggles(void) {
    brick_game_t g = playing();
    brick_input_t pause; memset(&pause, 0, sizeof pause); pause.pause = true;
    brick_update(&g, &pause);
    CHECK(g.state == BRICK_ST_PAUSE && g.pause_return == BRICK_ST_PLAY);
    int32_t x = g.ball_x, y = g.ball_y; int px = g.paddle_x;
    brick_input_t move; memset(&move, 0, sizeof move); move.paddle_dir = 1;
    tick(&g, &move, 10);
    CHECK(g.ball_x == x && g.ball_y == y && g.paddle_x == px);
    brick_update(&g, &pause);
    CHECK(g.state == BRICK_ST_PLAY);

    brick_game_t s; brick_init(&s); brick_new_game(&s);
    brick_update(&s, &pause);
    CHECK(s.state == BRICK_ST_PAUSE && s.pause_return == BRICK_ST_SERVE);
    brick_update(&s, &pause);
    CHECK(s.state == BRICK_ST_SERVE);
}

static void test_ball_lost_costs_life_then_game_over(void) {
    brick_game_t g = playing(); g.score = 7;
    brick_on_ball_lost(&g);
    CHECK(g.lives == BRICK_LIVES - 1 && g.state == BRICK_ST_SERVE);
    brick_rect_t b = brick_ball_rect(&g), p = brick_paddle_rect(&g);
    CHECK(b.y1 == p.y0 && g.ball_vx == 0 && g.ball_vy == 0);
    brick_on_ball_lost(&g);
    brick_on_ball_lost(&g);
    CHECK(g.lives == 0 && g.state == BRICK_ST_GAMEOVER);
    CHECK(g.high_score == 7);
    brick_input_t in; memset(&in, 0, sizeof in); in.confirm = true;
    brick_update(&g, &in);
    CHECK(g.state == BRICK_ST_TITLE && g.high_score == 7);
    brick_new_game(&g);
    CHECK(g.high_score == 7 && g.score == 0);
}

static void test_level_clear_refills_and_speeds_up(void) {
    brick_game_t g = playing(); g.score = 60;
    memset(g.cells, 0, sizeof g.cells); g.bricks_left = 0;
    brick_on_level_clear(&g);
    CHECK(g.level == 2 && g.state == BRICK_ST_SERVE);
    CHECK(g.bricks_left == BRICK_ROWS * BRICK_COLS);
    for (int r = 0; r < BRICK_ROWS; r++) for (int c = 0; c < BRICK_COLS; c++) CHECK(g.cells[r][c] == 1);
    CHECK(g.ball_speed == BRICK_BALL_SPEED_BASE + BRICK_BALL_SPEED_RAMP);
    CHECK(g.score == 60);
    brick_input_t in; memset(&in, 0, sizeof in); in.launch = true;
    brick_update(&g, &in);
    CHECK(g.state == BRICK_ST_PLAY && g.ball_vx < 0);   /* even level: serve to the left */
    g.level = 30; brick_on_level_clear(&g);
    CHECK(g.ball_speed == BRICK_BALL_SPEED_MAX);
}

int main(void) {
    RUN(test_init_is_title);
    RUN(test_cell_rects);
    RUN(test_row_colors);
    RUN(test_paddle_and_ball_rects);
    RUN(test_bounce_table_is_unit_length);
    RUN(test_new_game_resets);
    RUN(test_title_confirm_starts_game);
    RUN(test_launch_enters_play);
    RUN(test_serve_ball_follows_paddle);
    RUN(test_paddle_clamps_to_playfield);
    RUN(test_analog_overrides_digital);
    RUN(test_pause_toggles);
    RUN(test_ball_lost_costs_life_then_game_over);
    RUN(test_level_clear_refills_and_speeds_up);
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
