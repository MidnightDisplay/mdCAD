#ifndef MDCAD_MATH_BENCH_H
#define MDCAD_MATH_BENCH_H

#include <stdint.h>
#include <time.h>

typedef void (*mdcad_bench_fn)(void *ctx);

typedef struct {
    const char *name;
    uint64_t iterations;
    double elapsed_ms;
} mdcad_bench_result_t;

static inline double mdcad_bench_now_ms(void) {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return ((double)now.tv_sec * 1000.0) + ((double)now.tv_nsec / 1000000.0);
}

static inline mdcad_bench_result_t mdcad_bench_run(const char *name, uint64_t iterations, mdcad_bench_fn fn, void *ctx) {
    mdcad_bench_result_t result = {
        .name = name,
        .iterations = iterations,
        .elapsed_ms = 0.0,
    };

    if (!fn || iterations == 0) {
        return result;
    }

    double started_ms = mdcad_bench_now_ms();
    for (uint64_t i = 0; i < iterations; ++i) {
        fn(ctx);
    }
    result.elapsed_ms = mdcad_bench_now_ms() - started_ms;
    return result;
}

#endif
