#include "harness.h"
#include "games/runner.h"

static void zero_in(input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
}

static void tick(runner_t *g, const input_t *in, int n) {
    input_t z[GAME_MAX_PLAYERS];
    zero_in(z);
    if (!in) in = z;
    for (int i = 0; i < n; i++) runner_update(g, in);
}

static void drain_sfx(runner_t *g) {
    while (sfx_pop(&g->sfx) != SFX_NONE) {}
}

static void play_start(runner_t *g, uint32_t seed) {
    runner_init(g);
    runner_start(g, seed);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    runner_update(g, in);
}

static int nalive(const runner_t *g) {
    int n = 0;
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) if (g->buildings[i].alive) n++;
    return n;
}

static run_building_t *leftmost(runner_t *g) {
    run_building_t *best = 0;
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) {
        if (!g->buildings[i].alive) continue;
        if (!best || g->buildings[i].x < best->x) best = &g->buildings[i];
    }
    return best;
}

static int horiz_overlap_px(int bx, int w) {
    return RUN_PLAYER_X < bx + w && bx < RUN_PLAYER_X + RUN_PLAYER_W;
}

static void long_runway(runner_t *g) {
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) g->buildings[i].alive = false;
    g->buildings[0].alive = true;
    g->buildings[0].x = 0;
    g->buildings[0].w = 4000;
    g->buildings[0].roof = 180;
    g->grounded = true;
    g->vy = 0;
    g->py = (int32_t)(180 - RUN_PLAYER_H) << 8;
}

static void test_start_layout_and_a_plays(void) {
    runner_t g;
    runner_init(&g);
    g.high_score = 42;
    runner_start(&g, 0x1234567u);
    CHECK(g.state == RUN_ST_TITLE);
    CHECK(g.high_score == 42);
    CHECK(g.score == 0);
    CHECK(g.grounded);
    CHECK(g.speed == RUN_SPEED_START);
    CHECK(nalive(&g) >= 1);

    const run_building_t *b = leftmost(&g);
    CHECK(b);
    CHECK(b->roof == 180);
    CHECK((g.py >> 8) + RUN_PLAYER_H == 180);
    CHECK(horiz_overlap_px((int)(b->x >> 8), b->w));

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    runner_update(&g, in);
    CHECK(g.state == RUN_ST_PLAY);
    CHECK(g.high_score == 42);
    CHECK(g.grounded);
}

static void test_jump_and_short_hop_cut(void) {
    runner_t g;
    play_start(&g, 1);
    drain_sfx(&g);

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    in[0].a_held = true;
    tick(&g, in, 1);
    CHECK(g.vy == RUN_JUMP_VY);
    CHECK(!g.grounded);
    CHECK(sfx_pop(&g.sfx) == SFX_MERGE);

    drain_sfx(&g);
    zero_in(in);
    tick(&g, in, 1);
    CHECK(g.vy == RUN_JUMP_CUT_VY);
    CHECK(!g.grounded);
}

static void test_gravity_while_airborne(void) {
    runner_t g;
    play_start(&g, 1);
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) g.buildings[i].alive = false;
    g.grounded = false;
    g.vy = 0;
    g.py = 40 << 8;
    tick(&g, NULL, 10);
    CHECK(g.vy == 10 * RUN_GRAVITY);
    CHECK(g.state == RUN_ST_PLAY);
}

static void test_landing_snaps_to_roof(void) {
    runner_t g;
    play_start(&g, 1);
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) g.buildings[i].alive = false;
    g.buildings[0].alive = true;
    g.buildings[0].x = 50 << 8;
    g.buildings[0].w = 40;
    g.buildings[0].roof = 110;
    g.grounded = false;
    g.vy = 0;
    g.py = (int32_t)(110 - RUN_PLAYER_H) << 8;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.grounded);
    CHECK(g.vy == 0);
    CHECK((g.py >> 8) == 110 - RUN_PLAYER_H);
    CHECK(sfx_pop(&g.sfx) == SFX_HIT);
}

static void test_speed_ramp_and_cap(void) {
    runner_t g;
    play_start(&g, 1);
    long_runway(&g);
    g.speed = RUN_SPEED_START;
    g.speed_ticks = 0;
    CHECK(g.speed == RUN_SPEED_START);
    tick(&g, NULL, RUN_SPEED_STEP_TICKS);
    CHECK(g.speed == RUN_SPEED_START + RUN_SPEED_STEP);

    g.speed = RUN_SPEED_MAX - 10;
    g.speed_ticks = RUN_SPEED_STEP_TICKS - 1;
    tick(&g, NULL, 1);
    CHECK(g.speed == RUN_SPEED_MAX);
    g.speed_ticks = RUN_SPEED_STEP_TICKS - 1;
    tick(&g, NULL, 1);
    CHECK(g.speed == RUN_SPEED_MAX);
}

static void test_spawned_building_constraints(void) {
    runner_t g;
    runner_init(&g);
    runner_start(&g, 0x1234567u);
    CHECK(nalive(&g) >= 2);

    int xs[RUN_MAX_BUILDINGS], ws[RUN_MAX_BUILDINGS], rs[RUN_MAX_BUILDINGS];
    int n = 0;
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) {
        if (!g.buildings[i].alive) continue;
        xs[n] = (int)(g.buildings[i].x >> 8);
        ws[n] = g.buildings[i].w;
        rs[n] = g.buildings[i].roof;
        n++;
    }
    CHECK(n >= 2);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (xs[j] < xs[i]) {
                int t;
                t = xs[i]; xs[i] = xs[j]; xs[j] = t;
                t = ws[i]; ws[i] = ws[j]; ws[j] = t;
                t = rs[i]; rs[i] = rs[j]; rs[j] = t;
            }
        }
    }

    int spawned = 0;
    for (int i = 0; i < n; i++) {
        CHECK(rs[i] >= RUN_ROOF_MIN && rs[i] <= RUN_ROOF_MAX);
        /* The starter building is wider than the spawn table; spawned ones are not. */
        if (ws[i] > RUN_W_MIN + RUN_W_RAND - 1) continue;
        CHECK(ws[i] >= RUN_W_MIN && ws[i] <= RUN_W_MIN + RUN_W_RAND - 1);
        spawned++;
    }
    CHECK(spawned >= 1);

    for (int i = 1; i < n; i++) {
        int gap = xs[i] - (xs[i - 1] + ws[i - 1]);
        CHECK(gap >= RUN_GAP_MIN && gap <= RUN_GAP_MIN + RUN_GAP_RAND - 1);
        int d = rs[i] - rs[i - 1];
        if (d < 0) d = -d;
        CHECK(d <= RUN_ROOF_DELTA);
    }
}

static void test_fall_gap_gameover(void) {
    runner_t g;
    play_start(&g, 1);
    g.high_score = 3;
    g.score = 7;
    g.distance = 70 << 8;
    g.last_point_score = 7;
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) g.buildings[i].alive = false;
    g.grounded = false;
    g.vy = 0;
    g.py = (SCREEN_H - 5) << 8;
    drain_sfx(&g);
    tick(&g, NULL, 20);
    CHECK(g.state == RUN_ST_GAMEOVER);
    CHECK(g.high_score == 7);
    CHECK(sfx_pop(&g.sfx) == SFX_EXPLODE);
}

static void test_wall_hit_gameover(void) {
    runner_t g;
    play_start(&g, 1);
    for (int i = 0; i < RUN_MAX_BUILDINGS; i++) g.buildings[i].alive = false;
    g.buildings[0].alive = true;
    g.buildings[0].x = (RUN_PLAYER_X + RUN_PLAYER_W) << 8;
    g.buildings[0].w = 80;
    g.buildings[0].roof = 120;
    g.grounded = true;
    g.vy = 0;
    g.py = (int32_t)(180 - RUN_PLAYER_H) << 8;
    g.speed = RUN_SPEED_START;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.state == RUN_ST_GAMEOVER);
    CHECK(sfx_pop(&g.sfx) == SFX_EXPLODE);
}

static void test_score_metres_and_point_at_100(void) {
    runner_t g;
    play_start(&g, 1);
    long_runway(&g);
    g.speed = RUN_SPEED_START;
    g.distance = (1000 << 8) - g.speed;
    g.score = (g.distance >> 8) / RUN_METRE;
    g.last_point_score = g.score;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.score == 100);
    CHECK(sfx_pop(&g.sfx) == SFX_POINT);

    drain_sfx(&g);
    tick(&g, NULL, 5);
    CHECK(g.score >= 100);
    CHECK(sfx_pop(&g.sfx) == SFX_NONE);
}

static void test_autoplay_score_and_end(void) {
    runner_t g;
    runner_init(&g);
    runner_start(&g, 0x1234567u);
    input_t in[GAME_MAX_PLAYERS];
    int t = 0;
    int reached200 = 0;
    while (g.state != RUN_ST_GAMEOVER && t < 60000) {
        runner_autoplay(&g, in);
        runner_update(&g, in);
        t++;
        if (!reached200 && g.score >= 200) {
            reached200 = 1;
        }
    }
    printf("\n    game over after %d ticks, score=%d high=%d\n%-52s",
           t, g.score, g.high_score, "");
    CHECK(reached200);
    CHECK(g.state == RUN_ST_GAMEOVER);
    CHECK(t <= 60000);
}

static void test_descriptor(void) {
    CHECK(strcmp(GAME_RUNNER.name, "RUNNER") == 0);
    CHECK(GAME_RUNNER.track == MUSIC_RUNNER);
    CHECK(GAME_RUNNER.players == 1);
    runner_t *g = GAME_RUNNER.state;
    GAME_RUNNER.init(g);
    GAME_RUNNER.set_high_score(g, 99);
    GAME_RUNNER.start(g, 1);
    CHECK(g->state == RUN_ST_TITLE && GAME_RUNNER.get_high_score(g) == 99);
    CHECK(!GAME_RUNNER.is_over(g));
    GAME_RUNNER.set_high_score(g, 123);
    CHECK(GAME_RUNNER.get_high_score(g) == 123);
}

int main(void) {
    RUN(test_start_layout_and_a_plays);
    RUN(test_jump_and_short_hop_cut);
    RUN(test_gravity_while_airborne);
    RUN(test_landing_snaps_to_roof);
    RUN(test_speed_ramp_and_cap);
    RUN(test_spawned_building_constraints);
    RUN(test_fall_gap_gameover);
    RUN(test_wall_hit_gameover);
    RUN(test_score_metres_and_point_at_100);
    RUN(test_autoplay_score_and_end);
    RUN(test_descriptor);
    HARNESS_MAIN_END();
}
