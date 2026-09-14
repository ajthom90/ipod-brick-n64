#include "harness.h"
#include "games/flap.h"

static void zero_in(input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
}

static void tick(flap_t *g, const input_t *in, int n) {
    input_t z[GAME_MAX_PLAYERS];
    zero_in(z);
    if (!in) in = z;
    for (int i = 0; i < n; i++) flap_update(g, in);
}

static void drain_sfx(flap_t *g) {
    while (sfx_pop(&g->sfx) != SFX_NONE) {}
}

static void play_start(flap_t *g, uint32_t seed) {
    flap_init(g);
    flap_start(g, seed);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    flap_update(g, in);
}

static void clear_pipes(flap_t *g) {
    for (int i = 0; i < FLP_MAX_PIPES; i++) g->pipes[i].alive = false;
}

static int nalive(const flap_t *g) {
    int n = 0;
    for (int i = 0; i < FLP_MAX_PIPES; i++) if (g->pipes[i].alive) n++;
    return n;
}

static flp_pipe_t *rightmost(flap_t *g) {
    flp_pipe_t *best = 0;
    for (int i = 0; i < FLP_MAX_PIPES; i++) {
        if (!g->pipes[i].alive) continue;
        if (!best || g->pipes[i].x > best->x) best = &g->pipes[i];
    }
    return best;
}

static void test_gap_for_score(void) {
    CHECK(flap_gap_for_score(0) == 64);
    CHECK(flap_gap_for_score(100) == 54);
    CHECK(flap_gap_for_score(300) == 48);
    CHECK(flap_gap_for_score(160) == 48);
    CHECK(flap_gap_for_score(90) == 55);
}

static void test_gravity_and_terminal(void) {
    flap_t g;
    play_start(&g, 1);
    clear_pipes(&g);
    g.vy = 0;
    g.by = 40 << 8;
    tick(&g, NULL, 10);
    CHECK(g.vy == 10 * FLP_GRAVITY);
    CHECK(g.state == FLP_ST_PLAY);

    g.vy = FLP_TERMINAL - 10;
    g.by = 40 << 8;
    tick(&g, NULL, 1);
    CHECK(g.vy == FLP_TERMINAL);
    tick(&g, NULL, 1);
    CHECK(g.vy == FLP_TERMINAL);
}

static void test_a_sets_flap_vy_and_sfx(void) {
    flap_t g;
    flap_init(&g);
    g.high_score = 42;
    flap_start(&g, 0x1234567u);
    CHECK(g.state == FLP_ST_TITLE);
    CHECK(g.high_score == 42);
    CHECK(g.score == 0);
    CHECK(g.vy == 0);

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    flap_update(&g, in);
    CHECK(g.state == FLP_ST_PLAY);
    CHECK(g.vy == FLP_FLAP_VY);
    CHECK(sfx_pop(&g.sfx) == SFX_HIT);
    CHECK(g.high_score == 42);

    g.vy = 800;
    drain_sfx(&g);
    zero_in(in);
    in[0].a = true;
    tick(&g, in, 1);
    CHECK(g.vy == FLP_FLAP_VY);
    CHECK(sfx_pop(&g.sfx) == SFX_HIT);
}

static void test_ceiling_clamp(void) {
    flap_t g;
    play_start(&g, 1);
    clear_pipes(&g);
    g.by = PLAY_Y0 << 8;
    g.vy = 0;
    drain_sfx(&g);

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    tick(&g, in, 1);
    CHECK((g.by >> 8) == PLAY_Y0);
    CHECK(g.vy == 0);
    CHECK(g.state == FLP_ST_PLAY);
}

static void test_pipes_scroll_spawn_centers(void) {
    flap_t g;
    play_start(&g, 0x1234567u);
    CHECK(nalive(&g) == 1);
    flp_pipe_t *p = rightmost(&g);
    CHECK(p);
    CHECK((p->x >> 8) == PLAY_X1);
    CHECK(p->center >= FLP_CENTER_MIN && p->center <= FLP_CENTER_MIN + FLP_CENTER_RAND - 1);
    CHECK(p->gap == flap_gap_for_score(0));
    int32_t x0 = p->x;

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    tick(&g, in, 1);
    CHECK((p->x >> 8) == PLAY_X1 - 2);
    CHECK(p->x == x0 - FLP_SCROLL);

    tick(&g, in, FLP_PIPE_SPACING / 2 - 1);
    CHECK(nalive(&g) == 2);
    int xs[FLP_MAX_PIPES];
    int n = 0;
    for (int i = 0; i < FLP_MAX_PIPES; i++) {
        if (!g.pipes[i].alive) continue;
        xs[n++] = g.pipes[i].x >> 8;
        CHECK(g.pipes[i].center >= FLP_CENTER_MIN);
        CHECK(g.pipes[i].center <= FLP_CENTER_MIN + FLP_CENTER_RAND - 1);
    }
    CHECK(n == 2);
    int dx = xs[0] - xs[1];
    if (dx < 0) dx = -dx;
    CHECK(dx == FLP_PIPE_SPACING);
}

static void test_score_once_per_pipe(void) {
    flap_t g;
    play_start(&g, 1);
    clear_pipes(&g);
    g.by = 100 << 8;
    g.vy = 0;
    g.pipes[0].alive = true;
    g.pipes[0].scored = false;
    g.pipes[0].center = 120;
    g.pipes[0].gap = 64;
    /* Right edge 82; after one 2 px scroll it becomes 80 and the bird (x 80) passes it. */
    g.pipes[0].x = 58 << 8;
    g.score = 0;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.score == 1);
    CHECK(g.pipes[0].scored);
    CHECK(sfx_pop(&g.sfx) == SFX_POINT);
    drain_sfx(&g);
    tick(&g, NULL, 5);
    CHECK(g.score == 1);
    CHECK(sfx_pop(&g.sfx) == SFX_NONE);
}

static void test_collide_top_pipe(void) {
    flap_t g;
    play_start(&g, 1);
    clear_pipes(&g);
    g.high_score = 3;
    g.score = 7;
    g.by = 40 << 8;
    g.vy = 0;
    g.pipes[0].alive = true;
    g.pipes[0].scored = false;
    g.pipes[0].x = 78 << 8;
    g.pipes[0].center = 120;
    g.pipes[0].gap = 64; /* top pipe PLAY_Y0 .. 88 */
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.state == FLP_ST_GAMEOVER);
    CHECK(g.high_score == 7);
    CHECK(sfx_pop(&g.sfx) == SFX_EXPLODE);
}

static void test_collide_bottom_pipe(void) {
    flap_t g;
    play_start(&g, 1);
    clear_pipes(&g);
    g.by = 140 << 8;
    g.vy = 0;
    g.pipes[0].alive = true;
    g.pipes[0].scored = false;
    g.pipes[0].x = 78 << 8;
    g.pipes[0].center = 100;
    g.pipes[0].gap = 64; /* bottom pipe 132 .. 220 */
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.state == FLP_ST_GAMEOVER);
    CHECK(sfx_pop(&g.sfx) == SFX_EXPLODE);
}

static void test_collide_ground(void) {
    flap_t g;
    play_start(&g, 1);
    clear_pipes(&g);
    g.by = (FLP_GROUND_Y - FLP_BIRD) << 8;
    g.vy = 0;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.state == FLP_ST_GAMEOVER);
    CHECK((g.by >> 8) + FLP_BIRD >= FLP_GROUND_Y);
    CHECK(sfx_pop(&g.sfx) == SFX_EXPLODE);
}

static void test_autoplay_score_and_end(void) {
    flap_t g;
    flap_init(&g);
    flap_start(&g, 0x1234567u);
    input_t in[GAME_MAX_PLAYERS];
    int t = 0;
    int reached20 = 0;
    while (g.state != FLP_ST_GAMEOVER && t < 20000) {
        flap_autoplay(&g, in);
        flap_update(&g, in);
        t++;
        if (!reached20 && g.score >= 20) {
            reached20 = 1;
        }
    }
    printf("\n    game over after %d ticks, score=%d high=%d\n%-52s",
           t, g.score, g.high_score, "");
    CHECK(reached20);
    CHECK(g.state == FLP_ST_GAMEOVER);
    CHECK(t <= 20000);
}

static void test_descriptor(void) {
    CHECK(strcmp(GAME_FLAP.name, "FLAP") == 0);
    CHECK(GAME_FLAP.track == MUSIC_FLAP);
    CHECK(GAME_FLAP.players == 1);
    flap_t *g = GAME_FLAP.state;
    GAME_FLAP.init(g);
    GAME_FLAP.set_high_score(g, 99);
    GAME_FLAP.start(g, 1);
    CHECK(g->state == FLP_ST_TITLE && GAME_FLAP.get_high_score(g) == 99);
    CHECK(!GAME_FLAP.is_over(g));
    GAME_FLAP.set_high_score(g, 123);
    CHECK(GAME_FLAP.get_high_score(g) == 123);
}

int main(void) {
    RUN(test_gap_for_score);
    RUN(test_gravity_and_terminal);
    RUN(test_a_sets_flap_vy_and_sfx);
    RUN(test_ceiling_clamp);
    RUN(test_pipes_scroll_spawn_centers);
    RUN(test_score_once_per_pipe);
    RUN(test_collide_top_pipe);
    RUN(test_collide_bottom_pipe);
    RUN(test_collide_ground);
    RUN(test_autoplay_score_and_end);
    RUN(test_descriptor);
    HARNESS_MAIN_END();
}
