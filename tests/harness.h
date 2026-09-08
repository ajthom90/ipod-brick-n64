#ifndef HARNESS_H
#define HARNESS_H
#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { failures++; fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); return; } \
} while (0)

#define RUN(fn) do { \
    printf("%-52s", #fn); fflush(stdout); \
    int before = failures; fn(); \
    puts(before == failures ? "ok" : "FAILED"); \
} while (0)

#define HARNESS_MAIN_END() do { printf("%d failure(s)\n", failures); return failures ? 1 : 0; } while (0)

#endif
