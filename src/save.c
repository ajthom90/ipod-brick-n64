#include "save.h"
#include <string.h>

static void put_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void put_be16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static uint32_t get_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint16_t get_be16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

uint32_t save_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1u) crc = (crc >> 1) ^ 0xEDB88320u;
            else crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

void save_defaults(save_t *s) {
    settings_defaults(&s->settings);
    for (int i = 0; i < SAVE_MAX_GAMES; i++) s->high_scores[i] = 0;
}

void save_encode(const save_t *s, uint8_t out[SAVE_SIZE]) {
    memset(out, 0, SAVE_SIZE);
    put_be32(out + 0, SAVE_MAGIC);
    put_be16(out + 4, (uint16_t)SAVE_VERSION);
    out[6] = s->settings.music_on ? 1 : 0;
    out[7] = s->settings.sound_on ? 1 : 0;
    out[8] = (uint8_t)s->settings.volume;
    for (int i = 0; i < SAVE_MAX_GAMES; i++) {
        put_be32(out + 9 + i * 4, (uint32_t)s->high_scores[i]);
    }
    put_be32(out + 60, save_crc32(out, 60));
}

bool save_decode(const uint8_t in[SAVE_SIZE], save_t *out) {
    uint32_t magic = get_be32(in + 0);
    uint16_t version = get_be16(in + 4);
    uint32_t crc = get_be32(in + 60);
    if (magic != SAVE_MAGIC || version != SAVE_VERSION || crc != save_crc32(in, 60)) {
        save_defaults(out);
        return false;
    }
    out->settings.music_on = in[6] != 0;
    out->settings.sound_on = in[7] != 0;
    int vol = (int)in[8];
    if (vol > 10) vol = 10;
    out->settings.volume = vol;
    for (int i = 0; i < SAVE_MAX_GAMES; i++) {
        out->high_scores[i] = (int32_t)get_be32(in + 9 + i * 4);
    }
    return true;
}
