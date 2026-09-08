/* Renders the game core to PPM frames on the host, no N64 toolchain needed.
 * Text is not drawn; the state is printed to stdout instead. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "brick.h"

static uint8_t px[BRICK_SCREEN_H][BRICK_SCREEN_W][3];

static void fill(brick_rect_t r, uint32_t color) {
    for (int y = r.y0; y < r.y1; y++) {
        if (y < 0 || y >= BRICK_SCREEN_H) continue;
        for (int x = r.x0; x < r.x1; x++) {
            if (x < 0 || x >= BRICK_SCREEN_W) continue;
            px[y][x][0] = (uint8_t)(color >> 16);
            px[y][x][1] = (uint8_t)(color >> 8);
            px[y][x][2] = (uint8_t)color;
        }
    }
}

static const char *state_name(brick_state_t s) {
    switch (s) {
    case BRICK_ST_TITLE: return "TITLE";
    case BRICK_ST_SERVE: return "SERVE";
    case BRICK_ST_PLAY: return "PLAY";
    case BRICK_ST_PAUSE: return "PAUSE";
    case BRICK_ST_GAMEOVER: return "GAMEOVER";
    }
    return "?";
}

static void save(const char *dir, const char *name, const brick_game_t *g) {
    brick_rect_t screen = { 0, 0, BRICK_SCREEN_W, BRICK_SCREEN_H };
    fill(screen, BRICK_COLOR_BG);
    if (g->state != BRICK_ST_TITLE) {
        for (int r = 0; r < BRICK_ROWS; r++)
            for (int c = 0; c < BRICK_COLS; c++)
                if (g->cells[r][c]) fill(brick_cell_rect(r, c), brick_row_color(r));
        fill(brick_paddle_rect(g), BRICK_COLOR_PADDLE);
        fill(brick_ball_rect(g), BRICK_COLOR_BALL);
    }
    char path[512];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", BRICK_SCREEN_W, BRICK_SCREEN_H);
    fwrite(px, 1, sizeof px, f);
    fclose(f);
    printf("%-16s state=%-8s score=%-4d lives=%d level=%d\n", name, state_name(g->state), g->score, g->lives, g->level);
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : "build/frames";
    brick_game_t g; brick_init(&g);
    brick_input_t in;
    char name[64];
    for (int t = 0; t <= 1200; t++) {
        if (t % 30 == 0) {
            snprintf(name, sizeof name, "frame-%04d.ppm", t);
            save(dir, name, &g);
        }
        brick_autoplay_input(&g, &in);
        brick_update(&g, &in);
    }
    brick_game_t p = g;
    brick_input_t pause; memset(&pause, 0, sizeof pause); pause.pause = true;
    brick_update(&p, &pause);
    save(dir, "pause.ppm", &p);
    int t = 0;
    while (g.state != BRICK_ST_GAMEOVER && t < 60000) {
        brick_autoplay_input(&g, &in);
        brick_update(&g, &in);
        t++;
    }
    save(dir, "gameover.ppm", &g);
    return g.state == BRICK_ST_GAMEOVER ? 0 : 1;
}
