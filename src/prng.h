#ifndef PRNG_H
#define PRNG_H
#include <stdint.h>

typedef struct { uint32_t state; } prng_t;

static inline void prng_seed(prng_t *p, uint32_t seed) { p->state = seed ? seed : 0x9E3779B9u; }

static inline uint32_t prng_next(prng_t *p) {
    uint32_t x = p->state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    p->state = x;
    return x;
}

/* Uniform integer in [0, n). n must be >= 1. */
static inline uint32_t prng_below(prng_t *p, uint32_t n) { return prng_next(p) % n; }

#endif
