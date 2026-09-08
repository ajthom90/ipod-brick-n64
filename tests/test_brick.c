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

int main(void) {
    RUN(test_init_is_title);
    RUN(test_cell_rects);
    RUN(test_row_colors);
    RUN(test_paddle_and_ball_rects);
    RUN(test_bounce_table_is_unit_length);
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
