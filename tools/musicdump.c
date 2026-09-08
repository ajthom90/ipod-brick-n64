#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "music.h"
#include "synth.h"
#include "sfx.h"

static void write_u32le(FILE *f, uint32_t v) {
    fputc((int)(v & 0xFFu), f);
    fputc((int)((v >> 8) & 0xFFu), f);
    fputc((int)((v >> 16) & 0xFFu), f);
    fputc((int)((v >> 24) & 0xFFu), f);
}

static void write_u16le(FILE *f, uint16_t v) {
    fputc((int)(v & 0xFFu), f);
    fputc((int)((v >> 8) & 0xFFu), f);
}

static int write_wav(const char *path, const int16_t *buf, int n, int rate) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror(path);
        return -1;
    }
    uint32_t data_bytes = (uint32_t)n * 2u;
    fwrite("RIFF", 1, 4, f);
    write_u32le(f, 36u + data_bytes);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    write_u32le(f, 16);
    write_u16le(f, 1);
    write_u16le(f, 1);
    write_u32le(f, (uint32_t)rate);
    write_u32le(f, (uint32_t)rate * 2u);
    write_u16le(f, 2);
    write_u16le(f, 16);
    fwrite("data", 1, 4, f);
    write_u32le(f, data_bytes);
    fwrite(buf, 2, (size_t)n, f);
    fclose(f);
    return 0;
}

static const char *sfx_name(int id) {
    static const char *const names[] = {
        "none", "bounce", "hit", "clear", "food", "shot", "explode",
        "point", "merge", "game_over", "menu_move", "menu_select"
    };
    if (id < 0 || id >= SFX_COUNT) return "none";
    return names[id];
}

int main(void) {
    const int rate = 22050;
    char path[256];
    char err[128];

    for (int i = 0; i < MUSIC_TRACK_COUNT; i++) {
        music_track_t tr;
        if (!music_parse(&MUSIC_SRC[i], &tr, err, sizeof err)) {
            fprintf(stderr, "parse %s: %s\n", MUSIC_SRC[i].name, err);
            return 1;
        }
        int step_samples = rate * 15 / tr.bpm;
        if (step_samples < 1) step_samples = 1;
        int n = 2 * tr.steps * step_samples;
        int16_t *buf = (int16_t *)malloc((size_t)n * sizeof(int16_t));
        if (!buf) return 1;
        synth_t s;
        synth_init(&s, rate);
        synth_set_track(&s, &tr);
        synth_render(&s, buf, n);
        snprintf(path, sizeof path, "build/music/%s.wav", MUSIC_SRC[i].name);
        if (write_wav(path, buf, n, rate) != 0) {
            free(buf);
            return 1;
        }
        int ms = (int)((int64_t)n * 1000 / rate);
        printf("%s: %d steps, %d ms, %d samples\n", MUSIC_SRC[i].name, tr.steps, ms, n);
        free(buf);
    }

    int nsfx = rate * 6 / 10;
    int16_t *buf = (int16_t *)malloc((size_t)nsfx * sizeof(int16_t));
    if (!buf) return 1;
    for (int id = 1; id < SFX_COUNT; id++) {
        synth_t s;
        synth_init(&s, rate);
        synth_set_track(&s, NULL);
        synth_play_sfx(&s, (sfx_id_t)id);
        memset(buf, 0, (size_t)nsfx * sizeof(int16_t));
        synth_render(&s, buf, nsfx);
        snprintf(path, sizeof path, "build/music/sfx-%s.wav", sfx_name(id));
        if (write_wav(path, buf, nsfx, rate) != 0) {
            free(buf);
            return 1;
        }
        printf("sfx-%s: 600 ms\n", sfx_name(id));
    }
    free(buf);
    return 0;
}
