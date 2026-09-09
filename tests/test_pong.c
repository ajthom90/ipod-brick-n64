#include "harness.h"
#include "games/pong.h"

static void zero_in(input_t in[GAME_MAX_PLAYERS]) {
    memset(in, 0, sizeof(input_t) * GAME_MAX_PLAYERS);
}

static void tick(pong_t *g, const input_t *in, int n) {
    input_t z[GAME_MAX_PLAYERS];
    zero_in(z);
    if (!in) in = z;
    for (int i = 0; i < n; i++) pong_update(g, in);
}

static void drain_sfx(pong_t *g) {
    while (sfx_pop(&g->sfx) != SFX_NONE) {}
}

static int paddle_center_y(void) {
    return (PLAY_Y0 + PLAY_Y1 - PNG_PADDLE_H) / 2;
}

/* Title + A on the current row. */
static void confirm_title(pong_t *g) {
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].a = true;
    pong_update(g, in);
}

static void start_1p(pong_t *g, uint32_t seed) {
    pong_init(g);
    pong_start(g, seed);
    confirm_title(g);
}

static void start_2p(pong_t *g, uint32_t seed) {
    pong_init(g);
    pong_start(g, seed);
    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = -1;
    pong_update(g, in);
    zero_in(in);
    in[0].a = true;
    pong_update(g, in);
}

static void test_title_rows_and_mode(void) {
    pong_t g;
    pong_init(&g);
    g.high_score = 7;
    pong_start(&g, 0x1234567u);
    CHECK(g.state == PNG_ST_TITLE);
    CHECK(g.title_row == 0);
    CHECK(g.high_score == 7);
    CHECK(g.score1 == 0 && g.score2 == 0);

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = -1;
    pong_update(&g, in);
    CHECK(g.title_row == 1);
    CHECK(g.state == PNG_ST_TITLE);

    zero_in(in);
    in[0].dpad_y = 1;
    pong_update(&g, in);
    CHECK(g.title_row == 0);

    confirm_title(&g);
    CHECK(g.state == PNG_ST_SERVE);
    CHECK(g.two_players == false);
    CHECK(g.score1 == 0 && g.score2 == 0);
    CHECK(g.serve_to == 2);
    CHECK(g.high_score == 7);

    start_2p(&g, 1);
    CHECK(g.state == PNG_ST_SERVE);
    CHECK(g.two_players == true);
    CHECK(g.serve_to == 2);
}

static void test_serve_delay_and_direction(void) {
    pong_t g;
    start_1p(&g, 0x1234567u);
    CHECK(g.state == PNG_ST_SERVE);
    CHECK((g.ball_x >> 8) == 157 && (g.ball_y >> 8) == 123);
    CHECK(g.ball_vx == 0 && g.ball_vy == 0);
    CHECK(g.serve_to == 2);

    tick(&g, NULL, PNG_SERVE_DELAY - 1);
    CHECK(g.state == PNG_ST_SERVE);
    CHECK(g.ball_vx == 0 && g.ball_vy == 0);
    CHECK((g.ball_x >> 8) == 157 && (g.ball_y >> 8) == 123);

    tick(&g, NULL, 1);
    CHECK(g.state == PNG_ST_PLAY);
    CHECK(g.ball_vx > 0); /* serve_to == 2 → toward player 2 (right) */
    CHECK(g.ball_speed == PNG_SPEED_SERVE);
    CHECK(g.ball_vy != 0);
}

static void test_paddle_move_and_clamp_two_player(void) {
    pong_t g;
    start_2p(&g, 1);
    CHECK(g.p1_y == paddle_center_y());
    CHECK(g.p2_y == paddle_center_y());

    input_t in[GAME_MAX_PLAYERS];
    zero_in(in);
    in[0].dpad_y = 1;   /* up → smaller screen y */
    in[1].dpad_y = -1;  /* down */
    tick(&g, in, 1);
    CHECK(g.p1_y == paddle_center_y() - PNG_PADDLE_DIGITAL);
    CHECK(g.p2_y == paddle_center_y() + PNG_PADDLE_DIGITAL);

    zero_in(in);
    in[0].stick_y = 256;
    in[1].stick_y = -256;
    in[0].dpad_y = -1; /* analog overrides digital */
    int p1 = g.p1_y, p2 = g.p2_y;
    tick(&g, in, 1);
    CHECK(g.p1_y == p1 - PNG_PADDLE_ANALOG_MAX);
    CHECK(g.p2_y == p2 + PNG_PADDLE_ANALOG_MAX);

    zero_in(in);
    in[0].dpad_y = 1;
    in[1].dpad_y = -1;
    tick(&g, in, 80);
    CHECK(g.p1_y == PLAY_Y0);
    CHECK(g.p2_y == PLAY_Y1 - PNG_PADDLE_H);

    zero_in(in);
    in[0].dpad_y = -1;
    in[1].dpad_y = 1;
    tick(&g, in, 80);
    CHECK(g.p1_y == PLAY_Y1 - PNG_PADDLE_H);
    CHECK(g.p2_y == PLAY_Y0);
}

static void test_wall_bounce(void) {
    pong_t g;
    start_1p(&g, 1);
    g.state = PNG_ST_PLAY;
    g.p1_y = paddle_center_y();
    g.p2_y = paddle_center_y();
    g.ball_x = 157 << 8;
    g.ball_y = (PLAY_Y0 + 1) << 8;
    g.ball_vx = 0;
    g.ball_vy = -512;
    g.ball_speed = PNG_SPEED_SERVE;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.ball_vy == 512);
    CHECK((g.ball_y >> 8) >= PLAY_Y0);
    CHECK(sfx_pop(&g.sfx) == SFX_BOUNCE);

    g.ball_y = (PLAY_Y1 - PNG_BALL - 1) << 8;
    g.ball_vy = 512;
    drain_sfx(&g);
    tick(&g, NULL, 1);
    CHECK(g.ball_vy == -512);
    CHECK((g.ball_y >> 8) + PNG_BALL <= PLAY_Y1);
    CHECK(sfx_pop(&g.sfx) == SFX_BOUNCE);
}

static void setup_p1_hit(pong_t *g, int zone) {
    g->state = PNG_ST_PLAY;
    g->p1_y = 80;
    g->p2_y = paddle_center_y();
    int cy = g->p1_y + (zone * PNG_PADDLE_H) / 7 + PNG_PADDLE_H / 14;
    g->ball_x = (PNG_P1_X + 1) << 8;
    g->ball_y = (cy - PNG_BALL / 2) << 8;
    g->ball_vx = -256;
    g->ball_vy = 0;
    g->ball_speed = PNG_SPEED_SERVE;
    drain_sfx(g);
}

static void test_paddle_bounce_speed(void) {
    pong_t g;
    start_1p(&g, 1);
    setup_p1_hit(&g, 3);
    tick(&g, NULL, 1);
    CHECK(g.state == PNG_ST_PLAY);
    CHECK(g.ball_speed == PNG_SPEED_SERVE + PNG_SPEED_STEP);
    CHECK(g.ball_vx > 0);
    CHECK((g.ball_x >> 8) >= PNG_P1_X + PNG_PADDLE_W);
    CHECK(sfx_pop(&g.sfx) == SFX_BOUNCE);
    int32_t vx = g.ball_vx, vy = g.ball_vy;
    int32_t len2 = vx * vx + vy * vy;
    int32_t want = g.ball_speed * g.ball_speed;
    CHECK(len2 > want - want / 8 && len2 < want + want / 8);
}

static void test_seven_zones_distinct_and_symmetric(void) {
    pong_t g;
    int32_t vys[7];
    start_1p(&g, 1);
    for (int z = 0; z < 7; z++) {
        setup_p1_hit(&g, z);
        tick(&g, NULL, 1);
        CHECK(g.state == PNG_ST_PLAY);
        CHECK(g.ball_vx > 0);
        vys[z] = g.ball_vy;
    }
    for (int z = 0; z < 7; z++) {
        for (int w = z + 1; w < 7; w++) CHECK(vys[z] != vys[w]);
    }
    CHECK(vys[0] == -vys[6]);
    CHECK(vys[1] == -vys[5]);
    CHECK(vys[2] == -vys[4]);
}

static void test_left_miss_scores_for_p2(void) {
    pong_t g;
    start_1p(&g, 1);
    g.state = PNG_ST_PLAY;
    g.p1_y = PLAY_Y0;
    g.p2_y = PLAY_Y0;
    g.ball_x = PLAY_X0 << 8;
    g.ball_y = (PLAY_Y1 - PNG_BALL - 4) << 8;
    g.ball_vx = -1024;
    g.ball_vy = 0;
    g.ball_speed = PNG_SPEED_SERVE;
    g.score1 = 0;
    g.score2 = 0;
    drain_sfx(&g);
    int t;
    for (t = 0; t < 30 && g.state == PNG_ST_PLAY; t++) tick(&g, NULL, 1);
    CHECK(g.score2 == 1 && g.score1 == 0);
    CHECK(g.serve_to == 1);
    CHECK(g.state == PNG_ST_SERVE);
    CHECK(sfx_pop(&g.sfx) == SFX_POINT);

    tick(&g, NULL, PNG_SERVE_DELAY);
    CHECK(g.state == PNG_ST_PLAY);
    CHECK(g.ball_vx < 0); /* next serve toward player 1 */
}

static void test_first_to_11_ends(void) {
    pong_t g;
    start_1p(&g, 1);
    g.state = PNG_ST_PLAY;
    g.p1_y = PLAY_Y0;
    g.p2_y = PLAY_Y0;
    g.ball_x = (PLAY_X1 - PNG_BALL) << 8;
    g.ball_y = (PLAY_Y1 - PNG_BALL - 4) << 8;
    g.ball_vx = 1024;
    g.ball_vy = 0;
    g.ball_speed = PNG_SPEED_SERVE;
    g.score1 = PNG_WIN_SCORE - 1;
    g.score2 = 3;
    drain_sfx(&g);
    int t;
    for (t = 0; t < 30 && g.state == PNG_ST_PLAY; t++) tick(&g, NULL, 1);
    CHECK(g.score1 == PNG_WIN_SCORE);
    CHECK(g.state == PNG_ST_GAMEOVER);
    sfx_id_t a = sfx_pop(&g.sfx), b = sfx_pop(&g.sfx);
    CHECK(a == SFX_POINT || a == SFX_GAME_OVER);
    CHECK(b == SFX_POINT || b == SFX_GAME_OVER);
    CHECK(a != b);
}

static void test_margin_high_score_p1_only(void) {
    pong_t g;
    start_1p(&g, 1);
    g.high_score = 3;
    g.state = PNG_ST_PLAY;
    g.p1_y = PLAY_Y0;
    g.p2_y = PLAY_Y0;
    g.ball_x = (PLAY_X1 - PNG_BALL) << 8;
    g.ball_y = (PLAY_Y1 - PNG_BALL - 4) << 8;
    g.ball_vx = 1024;
    g.ball_vy = 0;
    g.ball_speed = PNG_SPEED_SERVE;
    g.score1 = 10;
    g.score2 = 4;
    int t;
    for (t = 0; t < 30 && g.state == PNG_ST_PLAY; t++) tick(&g, NULL, 1);
    CHECK(g.state == PNG_ST_GAMEOVER);
    CHECK(g.score1 == 11 && g.score2 == 4);
    CHECK(g.high_score == 7); /* margin 11-4 */

    start_1p(&g, 1);
    g.high_score = 3;
    g.state = PNG_ST_PLAY;
    g.p1_y = PLAY_Y0;
    g.p2_y = PLAY_Y0;
    g.ball_x = PLAY_X0 << 8;
    g.ball_y = (PLAY_Y1 - PNG_BALL - 4) << 8;
    g.ball_vx = -1024;
    g.ball_vy = 0;
    g.ball_speed = PNG_SPEED_SERVE;
    g.score1 = 4;
    g.score2 = 10;
    for (t = 0; t < 30 && g.state == PNG_ST_PLAY; t++) tick(&g, NULL, 1);
    CHECK(g.state == PNG_ST_GAMEOVER);
    CHECK(g.score2 == 11);
    CHECK(g.high_score == 3); /* player 2 win does not update high score */

    start_1p(&g, 1);
    g.high_score = 9;
    g.state = PNG_ST_PLAY;
    g.p1_y = PLAY_Y0;
    g.p2_y = PLAY_Y0;
    g.ball_x = (PLAY_X1 - PNG_BALL) << 8;
    g.ball_y = (PLAY_Y1 - PNG_BALL - 4) << 8;
    g.ball_vx = 1024;
    g.ball_vy = 0;
    g.ball_speed = PNG_SPEED_SERVE;
    g.score1 = 10;
    g.score2 = 8;
    for (t = 0; t < 30 && g.state == PNG_ST_PLAY; t++) tick(&g, NULL, 1);
    CHECK(g.state == PNG_ST_GAMEOVER);
    CHECK(g.high_score == 9); /* margin 3 does not replace 9 */
}

static void test_pong_ai_tracks_and_idles(void) {
    pong_t g;
    start_1p(&g, 1);
    g.state = PNG_ST_PLAY;
    g.p2_y = 100;
    g.ball_x = 200 << 8;
    g.ball_y = 180 << 8;
    g.ball_vx = 512; /* toward player 2 */
    g.ball_vy = 0;
    int dy = 99;
    pong_ai(&g, g.p2_y, PNG_P2_X, &dy);
    CHECK(dy > 0 && dy <= PNG_AI_SPEED);

    g.ball_y = 40 << 8;
    pong_ai(&g, g.p2_y, PNG_P2_X, &dy);
    CHECK(dy < 0 && dy >= -PNG_AI_SPEED);

    g.ball_vx = -512; /* away from player 2 → drift to center */
    g.p2_y = PLAY_Y0;
    pong_ai(&g, g.p2_y, PNG_P2_X, &dy);
    CHECK(dy == 1);

    g.p2_y = PLAY_Y1 - PNG_PADDLE_H;
    pong_ai(&g, g.p2_y, PNG_P2_X, &dy);
    CHECK(dy == -1);

    g.p2_y = paddle_center_y();
    pong_ai(&g, g.p2_y, PNG_P2_X, &dy);
    CHECK(dy == 0);

    /* Player 1 side: ball moving left is "toward". */
    g.ball_vx = -512;
    g.p1_y = 90;
    g.ball_y = 160 << 8;
    pong_ai(&g, g.p1_y, PNG_P1_X, &dy);
    CHECK(dy > 0 && dy <= PNG_AI_SPEED);
}

static void test_autoplay_ends(void) {
    pong_t g;
    pong_init(&g);
    pong_start(&g, 0x1234567u);
    input_t in[GAME_MAX_PLAYERS];
    int t = 0;
    while (g.state != PNG_ST_GAMEOVER && t < 20000) {
        pong_autoplay(&g, in);
        pong_update(&g, in);
        t++;
    }
    printf("\n    game over after %d ticks, score=%d-%d\n%-52s",
           t, g.score1, g.score2, "");
    CHECK(g.state == PNG_ST_GAMEOVER);
    CHECK(t <= 20000);
    CHECK(g.score1 == PNG_WIN_SCORE || g.score2 == PNG_WIN_SCORE);
}

static void test_descriptor(void) {
    CHECK(strcmp(GAME_PONG.name, "PONG") == 0);
    CHECK(GAME_PONG.track == MUSIC_PONG);
    CHECK(GAME_PONG.players == 2);
    pong_t *g = GAME_PONG.state;
    GAME_PONG.init(g);
    GAME_PONG.set_high_score(g, 99);
    GAME_PONG.start(g, 1);
    CHECK(g->state == PNG_ST_TITLE && GAME_PONG.get_high_score(g) == 99);
    CHECK(!GAME_PONG.is_over(g));
    GAME_PONG.set_high_score(g, 123);
    CHECK(GAME_PONG.get_high_score(g) == 123);
}

int main(void) {
    RUN(test_title_rows_and_mode);
    RUN(test_serve_delay_and_direction);
    RUN(test_paddle_move_and_clamp_two_player);
    RUN(test_wall_bounce);
    RUN(test_paddle_bounce_speed);
    RUN(test_seven_zones_distinct_and_symmetric);
    RUN(test_left_miss_scores_for_p2);
    RUN(test_first_to_11_ends);
    RUN(test_margin_high_score_p1_only);
    RUN(test_pong_ai_tracks_and_idles);
    RUN(test_autoplay_ends);
    RUN(test_descriptor);
    HARNESS_MAIN_END();
}
