#include "harness.h"
#include "games/hopper.h"

static void zero_in(input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
}

static void tick(hopper_t *g, const input_t *in, int n) {
    input_t z[GAME_MAX_PLAYERS];
    zero_in(z);
    if (!in) in = z;
    for (int i = 0; i < n; i++) hopper_update(g, in);
}

static void drain_sfx(hopper_t *g) {
    while (sfx_pop(&g->sfx) != SFX_NONE) {}
}

static void play_start(hopper_t *g, uint32_t seed) {
    hopper_init(g);
    hopper_start(g, seed);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    hopper_update(g, in);
}

static void clear_plats(hopper_t *g) {
    for (int i = 0; i < HOP_MAX_PLATS; i++) g->plats[i].alive = false;
}

static hop_plat_t *first_alive(hopper_t *g) {
    for (int i = 0; i < HOP_MAX_PLATS; i++) if (g->plats[i].alive) return &g->plats[i];
    return 0;
}

static int nalive(const hopper_t *g) {
    int n = 0;
    for (int i = 0; i < HOP_MAX_PLATS; i++) if (g->plats[i].alive) n++;
    return n;
}

static const hop_plat_t *bottom_plat(const hopper_t *g) {
    const hop_plat_t *best = 0;
    for (int i = 0; i < HOP_MAX_PLATS; i++) {
        if (!g->plats[i].alive) continue;
        if (!best || g->plats[i].y > best->y) best = &g->plats[i];
    }
    return best;
}

static void test_start_layout_and_a_plays(void) {
    hopper_t g;
    hopper_init(&g);
    g.high_score = 42;
    hopper_start(&g, 0x1234567u);
    CHECK(g.state == HOP_ST_TITLE);
    CHECK(g.high_score == 42);
    CHECK(g.score == 0);
    CHECK(g.camera_y == g.camera_start);
    CHECK(nalive(&g) >= 2);

    const hop_plat_t *bot = bottom_plat(&g);
    CHECK(bot);
    CHECK(bot->y == 212);
    CHECK(bot->type == HOP_PLAT_STATIC);
    CHECK((g.py >> 8) + HOP_PLAYER == 212);
    int px = g.px >> 8;
    CHECK(px < bot->x + HOP_PLAT_W && bot->x < px + HOP_PLAYER);
    CHECK(px + HOP_PLAYER / 2 == 160);

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    hopper_update(&g, in);
    CHECK(g.state == HOP_ST_PLAY);
    CHECK(g.high_score == 42);
}

static void test_gravity_accumulates(void) {
    hopper_t g;
    play_start(&g, 1);
    clear_plats(&g);
    g.vy = 0;
    g.py = 80 << 8;
    tick(&g, NULL, 10);
    CHECK(g.vy == 640);
    CHECK(g.state == HOP_ST_PLAY);
}

static void test_land_static_bounces(void) {
    hopper_t g;
    play_start(&g, 1);
    clear_plats(&g);
    g.plats[0].alive = true;
    g.plats[0].x = 144;
    g.plats[0].y = 100;
    g.plats[0].dir = 0;
    g.plats[0].type = HOP_PLAT_STATIC;
    g.px = 154 << 8;
    g.py = 88 << 8;
    g.vy = 0;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.vy == HOP_JUMP_VY);
    CHECK((g.py >> 8) == 100 - HOP_PLAYER);
    CHECK(sfx_pop(&g.sfx) == SFX_BOUNCE);
}

static void test_land_spring(void) {
    hopper_t g;
    play_start(&g, 1);
    clear_plats(&g);
    g.plats[0].alive = true;
    g.plats[0].x = 144;
    g.plats[0].y = 100;
    g.plats[0].dir = 0;
    g.plats[0].type = HOP_PLAT_SPRING;
    g.px = 154 << 8;
    g.py = 88 << 8;
    g.vy = 0;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.vy == HOP_SPRING_VY);
    CHECK((g.py >> 8) == 100 - HOP_PLAYER);
    CHECK(sfx_pop(&g.sfx) == SFX_CLEAR);
}

static void test_pass_upward_through_platform(void) {
    hopper_t g;
    play_start(&g, 1);
    clear_plats(&g);
    g.plats[0].alive = true;
    g.plats[0].x = 144;
    g.plats[0].y = 100;
    g.plats[0].type = HOP_PLAT_STATIC;
    g.px = 154 << 8;
    g.py = 88 << 8;
    g.vy = HOP_JUMP_VY;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.vy == HOP_JUMP_VY + HOP_GRAVITY);
    CHECK((g.py >> 8) < 88);
    CHECK(sfx_pop(&g.sfx) == SFX_NONE);
}

static void test_wrap_both_sides(void) {
    hopper_t g;
    play_start(&g, 1);
    clear_plats(&g);
    g.vy = HOP_JUMP_VY;
    g.py = 80 << 8;
    g.camera_y = (80) - HOP_CAMERA_LINE;

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_x = -1;
    g.px = PLAY_X0 << 8;
    tick(&g, in, 1);
    CHECK((g.px >> 8) >= PLAY_X1 - HOP_PLAYER - HOP_MOVE_DIGITAL);
    CHECK((g.px >> 8) + HOP_PLAYER <= PLAY_X1);

    g.vy = HOP_JUMP_VY;
    g.py = 80 << 8;
    zero_in(in);
    in[0].dpad_x = 1;
    g.px = (PLAY_X1 - HOP_PLAYER) << 8;
    tick(&g, in, 1);
    CHECK((g.px >> 8) >= PLAY_X0);
    CHECK((g.px >> 8) <= PLAY_X0 + HOP_MOVE_DIGITAL);
}

static void test_camera_up_never_down_and_score(void) {
    hopper_t g;
    play_start(&g, 1);
    clear_plats(&g);
    g.camera_start = 0;
    g.camera_y = 0;
    g.px = 154 << 8;
    g.py = 0;
    g.vy = 0;
    tick(&g, NULL, 1);
    CHECK(g.camera_y == (0) - HOP_CAMERA_LINE);
    CHECK(g.score == (g.camera_start - g.camera_y) / 10);
    CHECK(g.score == 10);

    int cam = g.camera_y;
    int sc = g.score;
    g.vy = 256;
    tick(&g, NULL, 20);
    CHECK(g.camera_y == cam);
    CHECK(g.camera_y <= cam);
    CHECK(g.score == sc);
}

static hop_plat_t *moving_at_y(hopper_t *g, int y) {
    for (int i = 0; i < HOP_MAX_PLATS; i++) {
        if (g->plats[i].alive && g->plats[i].type == HOP_PLAT_MOVING && g->plats[i].y == y)
            return &g->plats[i];
    }
    return 0;
}

static void test_moving_platforms_reverse(void) {
    hopper_t g;
    play_start(&g, 1);
    clear_plats(&g);
    g.top_y = -10000;
    g.py = 80 << 8;
    g.vy = HOP_JUMP_VY;
    g.camera_y = 80 - HOP_CAMERA_LINE;

    g.plats[0].alive = true;
    g.plats[0].x = PLAY_X0;
    g.plats[0].y = 40;
    g.plats[0].dir = -1;
    g.plats[0].type = HOP_PLAT_MOVING;
    tick(&g, NULL, 1);
    hop_plat_t *p = moving_at_y(&g, 40);
    CHECK(p);
    CHECK(p->x >= PLAY_X0);
    CHECK(p->dir == 1);

    p->x = PLAY_X1 - HOP_PLAT_W;
    p->dir = 1;
    tick(&g, NULL, 1);
    p = moving_at_y(&g, 40);
    CHECK(p);
    CHECK(p->x + HOP_PLAT_W <= PLAY_X1);
    CHECK(p->dir == -1);
}

static int plat_gaps_ok(const hopper_t *g) {
    int ys[HOP_MAX_PLATS];
    int n = 0;
    for (int i = 0; i < HOP_MAX_PLATS; i++) {
        if (!g->plats[i].alive) continue;
        ys[n++] = g->plats[i].y;
    }
    if (n < 2) return 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (ys[j] < ys[i]) {
                int t = ys[i]; ys[i] = ys[j]; ys[j] = t;
            }
        }
    }
    for (int i = 1; i < n; i++) {
        int gap = ys[i] - ys[i - 1];
        if (gap < HOP_GAP_MIN || gap > HOP_GAP_MIN + (HOP_GAP_RAND - 1) + HOP_GAP_EXTRA_CAP)
            return 0;
    }
    return 1;
}

static void test_generation_caps_and_gaps(void) {
    hopper_t g;
    hopper_init(&g);
    hopper_start(&g, 0x1234567u);
    CHECK(g.top_y <= g.camera_y - 40);
    CHECK(plat_gaps_ok(&g));
    CHECK(first_alive(&g));

    hopper_generate_up_to(&g, g.camera_y - 400);
    CHECK(g.top_y <= g.camera_y - 400);
    CHECK(plat_gaps_ok(&g));
}

static void test_fall_gameover_high_score(void) {
    hopper_t g;
    play_start(&g, 1);
    clear_plats(&g);
    g.top_y = -10000;
    g.high_score = 5;
    g.camera_start = 0;
    g.camera_y = -150;
    g.py = 91 << 8;
    g.vy = 0;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.score == 15);
    CHECK(g.state == HOP_ST_GAMEOVER);
    CHECK(g.high_score == 15);
    CHECK(sfx_pop(&g.sfx) == SFX_GAME_OVER);
}

static void test_autoplay_score_and_end(void) {
    hopper_t g;
    hopper_init(&g);
    hopper_start(&g, 0x1234567u);
    input_t in[GAME_MAX_PLAYERS];
    int t = 0;
    int reached50 = 0;
    while (g.state != HOP_ST_GAMEOVER && t < 60000) {
        hopper_autoplay(&g, in);
        hopper_update(&g, in);
        t++;
        if (!reached50 && g.score >= 50) {
            CHECK(t <= 6000);
            reached50 = 1;
        }
    }
    printf("\n    game over after %d ticks, score=%d high=%d\n%-52s",
           t, g.score, g.high_score, "");
    CHECK(reached50);
    CHECK(g.state == HOP_ST_GAMEOVER);
    CHECK(t <= 60000);
}

static void test_descriptor(void) {
    CHECK(strcmp(GAME_HOPPER.name, "HOPPER") == 0);
    CHECK(GAME_HOPPER.track == MUSIC_HOPPER);
    CHECK(GAME_HOPPER.players == 1);
    hopper_t *g = GAME_HOPPER.state;
    GAME_HOPPER.init(g);
    GAME_HOPPER.set_high_score(g, 99);
    GAME_HOPPER.start(g, 1);
    CHECK(g->state == HOP_ST_TITLE && GAME_HOPPER.get_high_score(g) == 99);
    CHECK(!GAME_HOPPER.is_over(g));
    GAME_HOPPER.set_high_score(g, 123);
    CHECK(GAME_HOPPER.get_high_score(g) == 123);
}

int main(void) {
    RUN(test_start_layout_and_a_plays);
    RUN(test_gravity_accumulates);
    RUN(test_land_static_bounces);
    RUN(test_land_spring);
    RUN(test_pass_upward_through_platform);
    RUN(test_wrap_both_sides);
    RUN(test_camera_up_never_down_and_score);
    RUN(test_moving_platforms_reverse);
    RUN(test_generation_caps_and_gaps);
    RUN(test_fall_gameover_high_score);
    RUN(test_autoplay_score_and_end);
    RUN(test_descriptor);
    HARNESS_MAIN_END();
}
