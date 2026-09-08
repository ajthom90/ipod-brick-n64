#include "harness.h"
#include "save.h"

static void expect_defaults(const save_t *s) {
    CHECK(s->settings.music_on);
    CHECK(s->settings.sound_on);
    CHECK(s->settings.volume == 7);
    for (int i = 0; i < SAVE_MAX_GAMES; i++) CHECK(s->high_scores[i] == 0);
}

static void put_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static int saves_equal(const save_t *a, const save_t *b) {
    if (a->settings.music_on != b->settings.music_on) return 0;
    if (a->settings.sound_on != b->settings.sound_on) return 0;
    if (a->settings.volume != b->settings.volume) return 0;
    for (int i = 0; i < SAVE_MAX_GAMES; i++) {
        if (a->high_scores[i] != b->high_scores[i]) return 0;
    }
    return 1;
}

static void test_crc32_known_vector(void) {
    CHECK(save_crc32((const uint8_t *)"123456789", 9) == 0xCBF43926u);
}

static void test_roundtrip_defaults_and_custom(void) {
    save_t orig, decoded;
    uint8_t raw[SAVE_SIZE];

    save_defaults(&orig);
    save_encode(&orig, raw);
    CHECK(save_decode(raw, &decoded));
    CHECK(saves_equal(&orig, &decoded));
    expect_defaults(&decoded);

    orig.settings.volume = 3;
    orig.settings.music_on = false;
    orig.settings.sound_on = true;
    orig.high_scores[0] = 63;
    orig.high_scores[1] = 12000;
    orig.high_scores[2] = 7;
    orig.high_scores[3] = 11;
    orig.high_scores[4] = 40;
    orig.high_scores[5] = 4096;
    orig.high_scores[6] = 0;
    orig.high_scores[7] = 0;
    save_encode(&orig, raw);
    CHECK(save_decode(raw, &decoded));
    CHECK(saves_equal(&orig, &decoded));
    CHECK(!decoded.settings.music_on);
    CHECK(decoded.settings.sound_on);
    CHECK(decoded.settings.volume == 3);
    CHECK(decoded.high_scores[0] == 63);
    CHECK(decoded.high_scores[1] == 12000);
    CHECK(decoded.high_scores[5] == 4096);
}

static void test_flip_any_byte_rejected(void) {
    save_t orig, decoded;
    uint8_t raw[SAVE_SIZE], flipped[SAVE_SIZE];
    save_defaults(&orig);
    orig.settings.volume = 3;
    orig.high_scores[0] = 99;
    save_encode(&orig, raw);

    for (int i = 0; i < SAVE_SIZE; i++) {
        memcpy(flipped, raw, SAVE_SIZE);
        flipped[i] ^= 0xFFu;
        CHECK(!save_decode(flipped, &decoded));
        expect_defaults(&decoded);
    }
}

static void test_bad_version_rejected(void) {
    save_t orig, decoded;
    uint8_t raw[SAVE_SIZE];
    save_defaults(&orig);
    save_encode(&orig, raw);
    raw[4] = (uint8_t)((SAVE_VERSION + 1) >> 8);
    raw[5] = (uint8_t)(SAVE_VERSION + 1);
    put_be32(raw + 60, save_crc32(raw, 60));
    CHECK(!save_decode(raw, &decoded));
    expect_defaults(&decoded);
}

int main(void) {
    RUN(test_crc32_known_vector);
    RUN(test_roundtrip_defaults_and_custom);
    RUN(test_flip_any_byte_rejected);
    RUN(test_bad_version_rejected);
    HARNESS_MAIN_END();
}
