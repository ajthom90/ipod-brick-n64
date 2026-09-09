#include "harness.h"
#include "games/parachute.h"

static void zero_in(input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
}

static void tick(parachute_t *g, const input_t *in, int n) {
    input_t z[GAME_MAX_PLAYERS];
    zero_in(z);
    if (!in) in = z;
    for (int i = 0; i < n; i++) parachute_update(g, in);
}

static void drain_sfx(parachute_t *g) {
    while (sfx_pop(&g->sfx) != SFX_NONE) {}
}

static void play_start(parachute_t *g, uint32_t seed) {
    parachute_init(g);
    parachute_start(g, seed);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    parachute_update(g, in);
}

static int nalive_bullets(const parachute_t *g) {
    int n = 0;
    for (int i = 0; i < PAR_MAX_BULLETS; i++) if (g->bullets[i].alive) n++;
    return n;
}

static int nalive_helis(const parachute_t *g) {
    int n = 0;
    for (int i = 0; i < PAR_MAX_HELIS; i++) if (g->helis[i].alive) n++;
    return n;
}

static par_heli_t *first_heli(parachute_t *g) {
    for (int i = 0; i < PAR_MAX_HELIS; i++) if (g->helis[i].alive) return &g->helis[i];
    return 0;
}

static void quiet_spawns(parachute_t *g) {
    g->spawn_ticks = 100000;
}

static void test_table_symmetry_and_unit_length(void) {
    for (int i = 0; i < 31; i++) {
        CHECK(PAR_SIN[i] == -PAR_SIN[30 - i]);
        CHECK(PAR_COS[i] == PAR_COS[30 - i]);
        int32_t s = PAR_SIN[i];
        int32_t c = PAR_COS[i];
        int32_t len2 = s * s + c * c;
        CHECK(len2 >= 64000 && len2 <= 67000);
    }
}

static void test_par_aim_index_rounds_and_clamps(void) {
    CHECK(par_aim_index(-75) == 0);
    CHECK(par_aim_index(0) == 15);
    CHECK(par_aim_index(75) == 30);
    CHECK(par_aim_index(-2) == 15);
    CHECK(par_aim_index(3) == 16);
    CHECK(par_aim_index(-100) == 0);
    CHECK(par_aim_index(100) == 30);
}

static void test_start_keeps_high_score(void) {
    parachute_t g;
    parachute_init(&g);
    g.high_score = 42;
    parachute_start(&g, 0x1234567u);
    CHECK(g.state == PAR_ST_TITLE);
    CHECK(g.high_score == 42);
    CHECK(g.aim == 0);
    CHECK(g.spawn_ticks == 60);
    CHECK(g.score == 0);
    CHECK(g.landed_left == 0 && g.landed_right == 0);
    CHECK(nalive_bullets(&g) == 0);
    CHECK(nalive_helis(&g) == 0);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    parachute_update(&g, in);
    CHECK(g.state == PAR_ST_PLAY);
    CHECK(g.high_score == 42);
}

static void test_aim_clamps(void) {
    parachute_t g;
    play_start(&g, 1);
    quiet_spawns(&g);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].stick_x = 256;
    tick(&g, in, 100);
    CHECK(g.aim == PAR_AIM_MAX);
    zero_in(in);
    in[0].stick_x = -256;
    tick(&g, in, 100);
    CHECK(g.aim == PAR_AIM_MIN);
}

static void test_fire_cost_and_cap(void) {
    parachute_t g;
    play_start(&g, 1);
    quiet_spawns(&g);
    drain_sfx(&g);
    CHECK(g.score == 0);

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    tick(&g, in, 1);
    CHECK(g.score == 0);
    CHECK(nalive_bullets(&g) == 1);
    CHECK(sfx_pop(&g.sfx) == SFX_SHOT);

    g.score = 5;
    drain_sfx(&g);
    tick(&g, in, 1);
    CHECK(g.score == 4);
    CHECK(nalive_bullets(&g) == 2);
    CHECK(sfx_pop(&g.sfx) == SFX_SHOT);

    tick(&g, in, 1);
    tick(&g, in, 1);
    CHECK(nalive_bullets(&g) == 4);
    int score = g.score;
    drain_sfx(&g);
    tick(&g, in, 1);
    CHECK(nalive_bullets(&g) == 4);
    CHECK(g.score == score);
    CHECK(sfx_pop(&g.sfx) == SFX_NONE);
}

static void test_bullet_up_and_dies(void) {
    parachute_t g;
    play_start(&g, 1);
    quiet_spawns(&g);
    g.aim = 0;

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    tick(&g, in, 1);
    CHECK(nalive_bullets(&g) == 1);
    CHECK(g.bullets[0].alive);
    int32_t y0 = g.bullets[0].y;
    int32_t x0 = g.bullets[0].x;
    CHECK(g.bullets[0].vx == 0);
    CHECK((g.bullets[0].x >> 8) == 160);

    zero_in(in);
    tick(&g, in, 1);
    CHECK(g.bullets[0].alive);
    CHECK(g.bullets[0].x == x0);
    CHECK(g.bullets[0].y == y0 - (PAR_BULLET_SPEED << 8));

    int t = 0;
    int last_py = g.bullets[0].y >> 8;
    while (g.bullets[0].alive && t < 80) {
        last_py = g.bullets[0].y >> 8;
        tick(&g, in, 1);
        t++;
    }
    CHECK(!g.bullets[0].alive);
    CHECK(last_py <= PLAY_Y0 + PAR_BULLET_SPEED);
}

static void test_heli_spawn_move_die(void) {
    parachute_t g;
    play_start(&g, 0x1234567u);
    CHECK(nalive_helis(&g) == 0);
    g.spawn_ticks = 1;
    tick(&g, NULL, 1);
    CHECK(nalive_helis(&g) == 1);
    par_heli_t *h = first_heli(&g);
    CHECK(h);
    CHECK(h->x == 0 || h->x == 304);
    CHECK(h->y >= 40 && h->y <= 90);
    CHECK(h->drops_left == 2);
    CHECK(h->dir == 1 || h->dir == -1);
    if (h->x == 0) CHECK(h->dir == 1);
    if (h->x == 304) CHECK(h->dir == -1);
    int x = h->x;
    int dir = h->dir;
    g.spawn_ticks = 100000;
    tick(&g, NULL, 1);
    CHECK(h->alive);
    CHECK(h->x == x + dir * PAR_HELI_SPEED);

    h->x = 320;
    h->dir = 1;
    tick(&g, NULL, 1);
    CHECK(!h->alive);

    play_start(&g, 1);
    quiet_spawns(&g);
    g.helis[0].alive = true;
    g.helis[0].x = -16;
    g.helis[0].y = 50;
    g.helis[0].dir = -1;
    g.helis[0].drops_left = 0;
    tick(&g, NULL, 1);
    CHECK(!g.helis[0].alive);
}

static void test_bullet_kills_heli(void) {
    parachute_t g;
    play_start(&g, 1);
    quiet_spawns(&g);
    g.score = 10;
    g.helis[0].alive = true;
    g.helis[0].x = 100;
    g.helis[0].y = 50;
    g.helis[0].dir = 1;
    g.helis[0].drops_left = 2;
    g.bullets[0].alive = true;
    g.bullets[0].x = 108 << 8;
    g.bullets[0].y = 53 << 8;
    g.bullets[0].vx = 0;
    g.bullets[0].vy = 0;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(!g.helis[0].alive);
    CHECK(!g.bullets[0].alive);
    CHECK(g.score == 12);
    CHECK(sfx_pop(&g.sfx) == SFX_EXPLODE);
}

static void test_trooper_fall_speeds(void) {
    parachute_t g;
    play_start(&g, 1);
    quiet_spawns(&g);
    g.troopers[0].alive = true;
    g.troopers[0].chute = true;
    g.troopers[0].x = 40;
    g.troopers[0].y = 100;
    tick(&g, NULL, 1);
    CHECK(g.troopers[0].alive);
    CHECK(g.troopers[0].y == 101);

    g.troopers[1].alive = true;
    g.troopers[1].chute = false;
    g.troopers[1].x = 80;
    g.troopers[1].y = 100;
    tick(&g, NULL, 1);
    CHECK(g.troopers[1].alive);
    CHECK(g.troopers[1].y == 104);
}

static void test_shoot_chute(void) {
    parachute_t g;
    play_start(&g, 1);
    quiet_spawns(&g);
    g.troopers[0].alive = true;
    g.troopers[0].chute = true;
    g.troopers[0].x = 100;
    g.troopers[0].y = 80;
    /* Chute is (97,72)-(109,78); body starts at y 80. */
    g.bullets[0].alive = true;
    g.bullets[0].x = 103 << 8;
    g.bullets[0].y = 75 << 8;
    g.bullets[0].vx = 0;
    g.bullets[0].vy = 0;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.troopers[0].alive);
    CHECK(g.troopers[0].chute == false);
    CHECK(!g.bullets[0].alive);
    CHECK(sfx_pop(&g.sfx) == SFX_HIT);
}

static void test_chuted_land_left(void) {
    parachute_t g;
    play_start(&g, 1);
    quiet_spawns(&g);
    g.troopers[0].alive = true;
    g.troopers[0].chute = true;
    g.troopers[0].x = 40;
    g.troopers[0].y = 220;
    tick(&g, NULL, 1);
    CHECK(!g.troopers[0].alive);
    CHECK(g.landed_left == 1);
    CHECK(g.landed_right == 0);
    CHECK(g.state == PAR_ST_PLAY);
    int found = 0;
    for (int i = 0; i < PAR_MAX_LANDED; i++) {
        if (g.landed[i].alive && g.landed[i].x == 40) found = 1;
    }
    CHECK(found);
}

static void test_four_on_one_side_gameover(void) {
    parachute_t g;
    play_start(&g, 1);
    quiet_spawns(&g);
    g.score = 8;
    g.high_score = 3;
    for (int i = 0; i < 4; i++) {
        g.troopers[i].alive = true;
        g.troopers[i].chute = true;
        g.troopers[i].x = 20 + i * 20;
        g.troopers[i].y = 220;
    }
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.landed_left == PAR_SIDE_LIMIT);
    CHECK(g.state == PAR_ST_GAMEOVER);
    CHECK(g.high_score == 8);
    CHECK(sfx_pop(&g.sfx) == SFX_GAME_OVER);
}

static void test_land_on_turret_gameover(void) {
    parachute_t g;
    play_start(&g, 1);
    quiet_spawns(&g);
    g.score = 11;
    g.high_score = 4;
    g.troopers[0].alive = true;
    g.troopers[0].chute = true;
    g.troopers[0].x = 152;
    g.troopers[0].y = 220;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.state == PAR_ST_GAMEOVER);
    CHECK(g.high_score == 11);
    CHECK(sfx_pop(&g.sfx) == SFX_GAME_OVER);
}

static void test_chuteless_crushes_landed(void) {
    parachute_t g;
    play_start(&g, 1);
    quiet_spawns(&g);
    g.score = 0;
    g.landed[0].alive = true;
    g.landed[0].x = 40;
    g.landed_left = 1;
    g.troopers[0].alive = true;
    g.troopers[0].chute = false;
    g.troopers[0].x = 40;
    g.troopers[0].y = 216;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(!g.troopers[0].alive);
    CHECK(!g.landed[0].alive);
    CHECK(g.landed_left == 0);
    CHECK(g.score == 3); /* +1 self, +2 crushed */
    CHECK(g.state == PAR_ST_PLAY);
    sfx_id_t a = sfx_pop(&g.sfx);
    CHECK(a == SFX_EXPLODE);
}

static void test_autoplay_ends(void) {
    parachute_t g;
    parachute_init(&g);
    parachute_start(&g, 0x1234567u);
    input_t in[GAME_MAX_PLAYERS];
    int t = 0;
    while (g.state != PAR_ST_GAMEOVER && t < 20000) {
        parachute_autoplay(&g, in);
        parachute_update(&g, in);
        t++;
    }
    printf("\n    game over after %d ticks, score=%d high=%d\n%-52s",
           t, g.score, g.high_score, "");
    CHECK(g.state == PAR_ST_GAMEOVER);
    CHECK(t <= 20000);
    CHECK(g.high_score >= 30);
}

static void test_descriptor(void) {
    CHECK(strcmp(GAME_PARACHUTE.name, "PARACHUTE") == 0);
    CHECK(GAME_PARACHUTE.track == MUSIC_PARACHUTE);
    CHECK(GAME_PARACHUTE.players == 1);
    parachute_t *g = GAME_PARACHUTE.state;
    GAME_PARACHUTE.init(g);
    GAME_PARACHUTE.set_high_score(g, 99);
    GAME_PARACHUTE.start(g, 1);
    CHECK(g->state == PAR_ST_TITLE && GAME_PARACHUTE.get_high_score(g) == 99);
    CHECK(!GAME_PARACHUTE.is_over(g));
    GAME_PARACHUTE.set_high_score(g, 123);
    CHECK(GAME_PARACHUTE.get_high_score(g) == 123);
}

int main(void) {
    RUN(test_table_symmetry_and_unit_length);
    RUN(test_par_aim_index_rounds_and_clamps);
    RUN(test_start_keeps_high_score);
    RUN(test_aim_clamps);
    RUN(test_fire_cost_and_cap);
    RUN(test_bullet_up_and_dies);
    RUN(test_heli_spawn_move_die);
    RUN(test_bullet_kills_heli);
    RUN(test_trooper_fall_speeds);
    RUN(test_shoot_chute);
    RUN(test_chuted_land_left);
    RUN(test_four_on_one_side_gameover);
    RUN(test_land_on_turret_gameover);
    RUN(test_chuteless_crushes_landed);
    RUN(test_autoplay_ends);
    RUN(test_descriptor);
    HARNESS_MAIN_END();
}
