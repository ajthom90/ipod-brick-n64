#include "harness.h"
#include "games/snake.h"

static void zero_in(input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
}

static void tick(snake_t *g, const input_t *in, int n) {
    input_t z[GAME_MAX_PLAYERS];
    zero_in(z);
    if (!in) in = z;
    for (int i = 0; i < n; i++) snake_update(g, in);
}

static void play_start(snake_t *g, uint32_t seed) {
    snake_start(g, seed);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    snake_update(g, in);
}

static snk_cell_t seg(const snake_t *g, int i) {
    return g->body[(g->head + i) % SNK_MAX];
}

static void step_hold(snake_t *g, const input_t *in) {
    tick(g, in, g->period);
}

static void test_start_layout_title(void) {
    snake_t g;
    snake_init(&g);
    g.high_score = 42;
    snake_start(&g, 0x1234567u);
    CHECK(g.state == SNK_ST_TITLE);
    CHECK(g.len == 4);
    CHECK(g.period == SNK_PERIOD_START);
    CHECK(g.score == 0 && g.foods == 0);
    CHECK(g.high_score == 42);
    CHECK(g.dir_x == 1 && g.dir_y == 0);
    CHECK(seg(&g, 0).x == 18 && seg(&g, 0).y == 12);
    CHECK(seg(&g, 1).x == 17 && seg(&g, 1).y == 12);
    CHECK(seg(&g, 2).x == 16 && seg(&g, 2).y == 12);
    CHECK(seg(&g, 3).x == 15 && seg(&g, 3).y == 12);
    CHECK(snake_occupied(&g, 18, 12) && snake_occupied(&g, 15, 12));
    CHECK(!snake_occupied(&g, 19, 12) && !snake_occupied(&g, 14, 12));
    CHECK(g.food.x >= 0 && g.food.x < SNK_COLS);
    CHECK(g.food.y >= 0 && g.food.y < SNK_ROWS);
    CHECK(!snake_occupied(&g, g.food.x, g.food.y));
}

static void test_a_starts_play(void) {
    snake_t g;
    play_start(&g, 0x1234567u);
    CHECK(g.state == SNK_ST_PLAY);
    CHECK(seg(&g, 0).x == 18 && seg(&g, 0).y == 12);
}

static void test_first_step_at_tick_8(void) {
    snake_t g;
    play_start(&g, 1);
    tick(&g, NULL, 7);
    CHECK(seg(&g, 0).x == 18 && seg(&g, 0).y == 12);
    tick(&g, NULL, 1);
    CHECK(seg(&g, 0).x == 19 && seg(&g, 0).y == 12);
    CHECK(seg(&g, 1).x == 18 && seg(&g, 1).y == 12);
    CHECK(g.len == 4);
}

static void test_turn_up_and_ignore_reverse(void) {
    snake_t g;
    play_start(&g, 1);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = 1; /* up */
    step_hold(&g, in);
    CHECK(seg(&g, 0).x == 18 && seg(&g, 0).y == 11);
    CHECK(g.dir_x == 0 && g.dir_y == -1);
    CHECK(g.state == SNK_ST_PLAY);

    play_start(&g, 1);
    zero_in(in);
    in[0].dpad_x = -1; /* reverse of heading right */
    step_hold(&g, in);
    CHECK(seg(&g, 0).x == 19 && seg(&g, 0).y == 12);
    CHECK(g.dir_x == 1 && g.dir_y == 0);
}

static void test_eat_food(void) {
    snake_t g;
    play_start(&g, 1);
    g.food.x = 19;
    g.food.y = 12;
    step_hold(&g, NULL);
    CHECK(g.len == 5);
    CHECK(g.score == 1);
    CHECK(g.foods == 1);
    CHECK(sfx_pop(&g.sfx) == SFX_FOOD);
    CHECK(seg(&g, 0).x == 19 && seg(&g, 0).y == 12);
    CHECK(snake_occupied(&g, 15, 12)); /* tail stayed: grew */
    CHECK(g.food.x >= 0 && g.food.x < SNK_COLS);
    CHECK(g.food.y >= 0 && g.food.y < SNK_ROWS);
    CHECK(!snake_occupied(&g, g.food.x, g.food.y));
    CHECK(!(g.food.x == 19 && g.food.y == 12));
}

static void test_speed_after_foods(void) {
    snake_t g;
    play_start(&g, 1);
    for (int n = 0; n < 5; n++) {
        g.food.x = (int8_t)(seg(&g, 0).x + g.dir_x);
        g.food.y = (int8_t)(seg(&g, 0).y + g.dir_y);
        step_hold(&g, NULL);
    }
    CHECK(g.foods == 5);
    CHECK(g.period == 7);

    g.foods = 24;
    g.food.x = (int8_t)(seg(&g, 0).x + g.dir_x);
    g.food.y = (int8_t)(seg(&g, 0).y + g.dir_y);
    step_hold(&g, NULL);
    CHECK(g.foods == 25);
    CHECK(g.period == SNK_PERIOD_MIN);

    g.foods = 29;
    g.food.x = (int8_t)(seg(&g, 0).x + g.dir_x);
    g.food.y = (int8_t)(seg(&g, 0).y + g.dir_y);
    step_hold(&g, NULL);
    CHECK(g.foods == 30);
    CHECK(g.period == SNK_PERIOD_MIN);
}

static void test_wall_death(void) {
    snake_t g;
    play_start(&g, 1);
    g.head = 0;
    g.len = 4;
    g.body[0] = (snk_cell_t){35, 12};
    g.body[1] = (snk_cell_t){34, 12};
    g.body[2] = (snk_cell_t){33, 12};
    g.body[3] = (snk_cell_t){32, 12};
    g.dir_x = 1; g.dir_y = 0;
    g.pending_x = 1; g.pending_y = 0;
    g.food.x = 0; g.food.y = 0;
    g.step_ticks = 0;
    g.period = SNK_PERIOD_START;
    g.score = 9;
    g.high_score = 3;
    step_hold(&g, NULL);
    CHECK(g.state == SNK_ST_GAMEOVER);
    CHECK(sfx_pop(&g.sfx) == SFX_GAME_OVER);
    CHECK(g.high_score == 9);
}

static void test_self_collision_uturn(void) {
    snake_t g;
    play_start(&g, 1);
    g.food.x = 19;
    g.food.y = 12;
    step_hold(&g, NULL);
    CHECK(g.len == 5);
    while (sfx_pop(&g.sfx) != SFX_NONE) {}

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = 1; /* up */
    step_hold(&g, in);
    zero_in(in);
    in[0].dpad_x = -1; /* left */
    step_hold(&g, in);
    zero_in(in);
    in[0].dpad_y = -1; /* down into the body */
    step_hold(&g, in);
    CHECK(g.state == SNK_ST_GAMEOVER);
    CHECK(sfx_pop(&g.sfx) == SFX_GAME_OVER);
}

static void test_tail_chase_allowed(void) {
    snake_t g;
    play_start(&g, 1);
    /* 2x2 square; head at (18,12) heading left into the tail at (17,12). */
    g.head = 0;
    g.len = 4;
    g.body[0] = (snk_cell_t){18, 12};
    g.body[1] = (snk_cell_t){18, 11};
    g.body[2] = (snk_cell_t){17, 11};
    g.body[3] = (snk_cell_t){17, 12};
    g.dir_x = -1; g.dir_y = 0;
    g.pending_x = -1; g.pending_y = 0;
    g.food.x = 0; g.food.y = 0;
    g.step_ticks = 0;
    step_hold(&g, NULL);
    CHECK(g.state == SNK_ST_PLAY);
    CHECK(g.len == 4);
    CHECK(seg(&g, 0).x == 17 && seg(&g, 0).y == 12);
}

static void test_autoplay_ends(void) {
    snake_t g;
    snake_init(&g);
    snake_start(&g, 0x1234567u);
    input_t in[GAME_MAX_PLAYERS];
    int t = 0;
    while (g.state != SNK_ST_GAMEOVER && t < 20000) {
        snake_autoplay(&g, in);
        snake_update(&g, in);
        t++;
    }
    printf("\n    game over after %d ticks, score=%d\n%-52s", t, g.score, "");
    CHECK(g.state == SNK_ST_GAMEOVER);
    CHECK(t <= 20000);
    CHECK(g.score >= 5);
}

static void test_descriptor(void) {
    CHECK(strcmp(GAME_SNAKE.name, "SNAKE") == 0);
    CHECK(GAME_SNAKE.track == MUSIC_SNAKE);
    CHECK(GAME_SNAKE.players == 1);
    snake_t *g = GAME_SNAKE.state;
    GAME_SNAKE.init(g);
    GAME_SNAKE.set_high_score(g, 99);
    GAME_SNAKE.start(g, 1);
    CHECK(g->state == SNK_ST_TITLE && GAME_SNAKE.get_high_score(g) == 99);
    CHECK(!GAME_SNAKE.is_over(g));
    GAME_SNAKE.set_high_score(g, 123);
    CHECK(GAME_SNAKE.get_high_score(g) == 123);
}

int main(void) {
    RUN(test_start_layout_title);
    RUN(test_a_starts_play);
    RUN(test_first_step_at_tick_8);
    RUN(test_turn_up_and_ignore_reverse);
    RUN(test_eat_food);
    RUN(test_speed_after_foods);
    RUN(test_wall_death);
    RUN(test_self_collision_uturn);
    RUN(test_tail_chase_allowed);
    RUN(test_autoplay_ends);
    RUN(test_descriptor);
    HARNESS_MAIN_END();
}
