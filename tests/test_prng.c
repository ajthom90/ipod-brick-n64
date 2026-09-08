#include "harness.h"
#include "prng.h"

static void test_seed_zero_produces_nonzero(void) {
    prng_t p;
    prng_seed(&p, 0);
    CHECK(prng_next(&p) != 0);
}

static void test_same_seed_same_sequence(void) {
    prng_t a, b;
    prng_seed(&a, 0x1234567u);
    prng_seed(&b, 0x1234567u);
    for (int i = 0; i < 100; i++) CHECK(prng_next(&a) == prng_next(&b));
}

static void test_below_six_is_uniform(void) {
    prng_t p;
    prng_seed(&p, 0x1234567u);
    int counts[6] = {0};
    for (int i = 0; i < 6000; i++) {
        uint32_t v = prng_below(&p, 6);
        CHECK(v < 6);
        counts[v]++;
    }
    for (int i = 0; i < 6; i++) CHECK(counts[i] >= 800 && counts[i] <= 1200);
}

int main(void) {
    RUN(test_seed_zero_produces_nonzero);
    RUN(test_same_seed_same_sequence);
    RUN(test_below_six_is_uniform);
    HARNESS_MAIN_END();
}
