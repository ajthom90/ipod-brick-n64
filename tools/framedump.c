/* Renders a game through the draw API to PPM frames on the host. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "games/registry.h"
#include "app_state.h"

static uint8_t px[SCREEN_H][SCREEN_W][3];

static void host_rect(void *ctx, int x0, int y0, int x1, int y1, uint32_t color) {
    (void)ctx;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > SCREEN_W) x1 = SCREEN_W;
    if (y1 > SCREEN_H) y1 = SCREEN_H;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            px[y][x][0] = (uint8_t)(color >> 16);
            px[y][x][1] = (uint8_t)(color >> 8);
            px[y][x][2] = (uint8_t)color;
        }
    }
}

static void host_text(void *ctx, draw_font_t font, draw_align_t align, int x, int y, uint32_t rgb, const char *s) {
    (void)ctx;
    (void)rgb;
    int nbytes = (int)strlen(s);
    int cw = (font == DRAW_FONT_BIG) ? 12 : 6;
    int h = (font == DRAW_FONT_BIG) ? 20 : 10;
    int w = cw * nbytes;
    int x0;
    if (align == DRAW_LEFT) x0 = x;
    else if (align == DRAW_CENTER) x0 = x - w / 2;
    else x0 = x - w;
    int y0 = y - h;
    host_rect(NULL, x0, y0, x0 + w, y, 0xC8C2B8u);
    printf("text font=%d align=%d x=%d y=%d \"%s\"\n", (int)font, (int)align, x, y, s);
}

static const draw_t host_draw = { .ctx = NULL, .rect = host_rect, .text = host_text };

static void save(const char *dir, const char *name, int tick, int is_over) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    fwrite(px, 1, sizeof px, f);
    fclose(f);
    printf("%-16s tick=%d is_over=%d\n", name, tick, is_over);
}

static int dump_game(const char *dir, const game_desc_t *game) {
    void *st = game->state;
    game->init(st);
    game->set_high_score(st, 0);
    game->start(st, 0x1234567u);

    input_t in[GAME_MAX_PLAYERS];
    char name[64];
    for (int t = 0; t <= 1200; t++) {
        if (t % 30 == 0) {
            game->render(st, &host_draw);
            snprintf(name, sizeof name, "frame-%04d.ppm", t);
            save(dir, name, t, game->is_over(st));
        }
        game->autoplay(st, in);
        game->update(st, in);
    }
    int t = 0;
    while (!game->is_over(st) && t < 200000) {
        game->autoplay(st, in);
        game->update(st, in);
        t++;
    }
    game->render(st, &host_draw);
    save(dir, "gameover.ppm", t, game->is_over(st));
    return game->is_over(st) ? 0 : 1;
}

static int dump_menu(const char *dir) {
    app_t app;
    app_init(&app, false, 0);
    input_t in[GAME_MAX_PLAYERS];
    char name[64];
    for (int t = 0; t <= 60; t++) {
        if (t % 30 == 0) {
            app_render(&app, &host_draw);
            snprintf(name, sizeof name, "frame-%04d.ppm", t);
            save(dir, name, t, 0);
        }
        memset(in, 0, sizeof in);
        in[0].dpad_y = -1;
        app_update(&app, in, false, 0x1234567u);
    }
    return 0;
}

static int dump_settings(const char *dir) {
    app_t app;
    app_init(&app, false, 0);
    input_t in[GAME_MAX_PLAYERS];
    memset(in, 0, sizeof in);
    in[0].dpad_y = -1;
    app_update(&app, in, false, 0x1234567u);
    memset(in, 0, sizeof in);
    in[0].a = true;
    app_update(&app, in, false, 0x1234567u);
    app_render(&app, &host_draw);
    save(dir, "frame-0000.ppm", 0, 0);
    return 0;
}

static int dump_pause(const char *dir) {
    app_t app;
    app_init(&app, false, 0);
    input_t in[GAME_MAX_PLAYERS];
    memset(in, 0, sizeof in);
    in[0].a = true;
    app_update(&app, in, false, 0x1234567u);
    const game_desc_t *game = GAMES[app.game_index];
    for (int t = 0; t < 300; t++) {
        memset(in, 0, sizeof in);
        game->autoplay(game->state, in);
        app_update(&app, in, false, 0x1234567u);
    }
    memset(in, 0, sizeof in);
    app_update(&app, in, true, 0x1234567u);
    app_render(&app, &host_draw);
    save(dir, "frame-0000.ppm", 0, 0);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: framedump <game-name|menu|pause|settings> <outdir>\n");
        return 2;
    }
    const char *dir = argv[2];
    if (strcmp(argv[1], "menu") == 0) return dump_menu(dir);
    if (strcmp(argv[1], "pause") == 0) return dump_pause(dir);
    if (strcmp(argv[1], "settings") == 0) return dump_settings(dir);
    int gi = game_index_by_name(argv[1]);
    if (gi < 0) {
        fprintf(stderr, "unknown game: %s\n", argv[1]);
        return 2;
    }
    return dump_game(dir, GAMES[gi]);
}
