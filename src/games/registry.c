#include "registry.h"
#include "brick.h"
#include "blocks.h"

const game_desc_t *const GAMES[] = { &GAME_BRICK, &GAME_BLOCKS };
const int GAME_COUNT = 2;

static int icmp(const char *a, const char *b) {
    for (;;) {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + ('a' - 'A'));
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + ('a' - 'A'));
        if (ca != cb) return (int)ca - (int)cb;
        if (ca == 0) return 0;
    }
}

int game_index_by_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < GAME_COUNT; i++) {
        if (icmp(GAMES[i]->name, name) == 0) return i;
    }
    return -1;
}

void fmt_label(char *buf, size_t cap, const char *prefix, int value) {
    size_t n = 0;
    while (prefix[n] && n + 1 < cap) { buf[n] = prefix[n]; n++; }
    if (n + 1 < cap) buf[n++] = ' ';
    char digits[12]; int d = 0;
    unsigned v = value < 0 ? (unsigned)(-value) : (unsigned)value;
    do { digits[d++] = (char)('0' + v % 10); v /= 10; } while (v && d < 11);
    if (value < 0 && n + 1 < cap) buf[n++] = '-';
    while (d > 0 && n + 1 < cap) buf[n++] = digits[--d];
    buf[n] = '\0';
}

void fmt_int(char *buf, size_t cap, int value) {
    size_t n = 0;
    char digits[12]; int d = 0;
    unsigned v = value < 0 ? (unsigned)(-value) : (unsigned)value;
    do { digits[d++] = (char)('0' + v % 10); v /= 10; } while (v && d < 11);
    if (value < 0 && n + 1 < cap) buf[n++] = '-';
    while (d > 0 && n + 1 < cap) buf[n++] = digits[--d];
    if (cap) buf[n] = '\0';
}
