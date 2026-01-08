/* Dr. Bafflegab's VBAP Benchmark (C17)
   ------------------------------------------------------------
   Portable single-file benchmark for drb-vbap.h implementations.

   - British English in comments.
   - Measures drb_vbap_process() throughput.
   - Uses built-in layouts via drb_vbap_builtin_layout().
   - Deterministic source generation (xorshift32).
   - Light validation of L2-norm (sum of squares ≈ 1) on a sample.

   Build (Linux/macOS, clang or gcc):
       cc -O3 -march=native -std=c17 -Wall -Wextra -pedantic \
          bench_vbap.c drb-vbap.c -o bench_vbap -lm

   Build (MSVC):
       cl /O2 /std:c17 bench_vbap.c drb-vbap.c

   Usage:
       ./bench_vbap [--layout TAG] [--sources N] [--frames N]
                    [--seed U32] [--warmup N] [--csv]

   Example:
       ./bench_vbap --layout surround-5 --sources 16384 --frames 200
*/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#endif

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

#include "drb-vbap.h" /* Adjust include path as needed */

#ifndef DRB_BENCH_ALIGN
#  define DRB_BENCH_ALIGN 64u
#endif

/* ----------------------- Small utilities ----------------------- */

static void *bench_aligned_alloc(size_t alignment, size_t size) {
#if defined(_MSC_VER)
    return _aligned_malloc(size, alignment);
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    /* aligned_alloc requires size to be a multiple of alignment */
    size_t rounded = (size + (alignment - 1)) & ~(alignment - 1);
    return aligned_alloc(alignment, rounded);
#else
    void *p = NULL;
    if (posix_memalign(&p, alignment, size) != 0) return NULL;
    return p;
#endif
}

static void bench_aligned_free(void *p) {
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    free(p);
#endif
}

static double bench_now_seconds(void) {
#ifdef _WIN32
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER t;
    if (!freq.QuadPart) {
        QueryPerformanceFrequency(&freq);
    }
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
#elif defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#else
    return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

/* Deterministic, tiny RNG */
static uint32_t xorshift32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

static float frand_01(uint32_t *s) {
    return (xorshift32(s) >> 8) * (1.0f / 16777216.0f); /* 24-bit mantissa */
}

/* Sample a unit vector on the circle */
static void unit_vector(float *x, float *y, uint32_t *rng) {
    float a = 2.0f * (float)M_PI * frand_01(rng);
    *x = cosf(a);
    *y = sinf(a);
}

/* Compute |sum_i g_i^2 - 1| for an output row */
static double l2_row_error(float const *row, int32_t n) {
    double s = 0.0;
    for (int32_t i = 0; i < n; ++i) {
        double v = row[i];
        s += v * v;
    }
    return fabs(s - 1.0);
}

/* ------------------- Argument parsing (simple) ------------------ */

typedef struct {
    char layout_tag[64];
    int32_t sources;
    int32_t frames;
    int32_t warmup;
    uint32_t seed;
    int csv;
} Bench_Args;

static void args_defaults(Bench_Args *a) {
    memset(a, 0, sizeof(*a));
    strcpy(a->layout_tag, DRB_VBAP_LAYOUT_TAG_SURROUND_5);
    a->sources = 16384;
    a->frames  = 8 * 1024;
    a->warmup  = 3;
    a->seed    = 0xC0FFEEu;
    a->csv     = 0;
}

static int parse_int32(const char *s, int32_t *out) {
    char *end = NULL;
    long long v = strtoll(s, &end, 10);
    if (!s || *s == '\0' || !end || *end != '\0') return 0;
    if (v < INT32_MIN || v > INT32_MAX) return 0;
    *out = (int32_t)v;
    return 1;
}

static int parse_u32(const char *s, uint32_t *out) {
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (!s || *s == '\0' || !end || *end != '\0') return 0;
    if (v > 0xFFFFFFFFull) return 0;
    *out = (uint32_t)v;
    return 1;
}

static void args_parse(Bench_Args *a, int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--layout") == 0 && i + 1 < argc) {
            strncpy(a->layout_tag, argv[++i], sizeof(a->layout_tag) - 1);
            a->layout_tag[sizeof(a->layout_tag) - 1] = '\0';
        } else if (strcmp(argv[i], "--sources") == 0 && i + 1 < argc) {
            parse_int32(argv[++i], &a->sources);
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            parse_int32(argv[++i], &a->frames);
        } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            parse_int32(argv[++i], &a->warmup);
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            parse_u32(argv[++i], &a->seed);
        } else if (strcmp(argv[i], "--csv") == 0) {
            a->csv = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--layout TAG] [--sources N] [--frames N] [--seed U32] [--warmup N] [--csv]\n", argv[0]);
            exit(0);
        }
    }
}

/* ------------------------- Benchmark --------------------------- */

int main(int argc, char **argv) {
    Bench_Args args;
    args_defaults(&args);
    args_parse(&args, argc, argv);

    int32_t maj=0, min=0, pat=0;
    drb_vbap_version(&maj, &min, &pat);

    DrB_VBAP_Layout const *layout = drb_vbap_builtin_layout(args.layout_tag);
    if (!layout) {
        fprintf(stderr, "Unrecognised layout tag '%s'. Try one of: '%s', '%s'.\n",
                args.layout_tag,
                DRB_VBAP_LAYOUT_TAG_SURROUND_5,
                DRB_VBAP_LAYOUT_TAG_SURROUND_7);
        return 1;
    }

    if (args.sources <= 0) {
        fprintf(stderr, "sources must be positive.\n");
        return 1;
    }
    if (args.frames <= 0) {
        fprintf(stderr, "frames must be positive.\n");
        return 1;
    }

    size_t const vbap_align = drb_vbap_alignment();
    size_t const vbap_size  = drb_vbap_size(layout);
    if (vbap_size == 0) {
        fprintf(stderr, "Layout invalid for construction.\n");
        return 1;
    }

    void *vbap_mem = bench_aligned_alloc(vbap_align, vbap_size);
    if (!vbap_mem) {
        fprintf(stderr, "Failed to allocate %zu bytes for VBAP (errno=%d).\n", vbap_size, errno);
        return 1;
    }

    DrB_VBAP_Error err = 0;
    DrB_VBAP const *vbap = drb_vbap_construct(vbap_mem, layout, &err);
    if (!vbap) {
        fprintf(stderr, "drb_vbap_construct failed: %s (code=%d)\n",
                drb_vbap_error_string(err), (int)err);
        bench_aligned_free(vbap_mem);
        return 1;
    }

    int32_t const speaker_count = layout->speaker_count;
    int32_t const source_count  = args.sources;

    /* Allocate inputs/outputs, aligned for good measure */
    size_t const src_bytes   = (size_t)source_count * 2u * sizeof(float);
    size_t const gain_bytes  = (size_t)source_count * (size_t)speaker_count * sizeof(float);
    float *source_positions  = (float*)bench_aligned_alloc(DRB_BENCH_ALIGN, src_bytes);
    float *speaker_gains     = (float*)bench_aligned_alloc(DRB_BENCH_ALIGN, gain_bytes);
    if (!source_positions || !speaker_gains) {
        fprintf(stderr, "Failed to allocate input/output buffers.\n");
        bench_aligned_free(source_positions);
        bench_aligned_free(speaker_gains);
        bench_aligned_free(vbap_mem);
        return 1;
    }

    /* Fill sources deterministically on the unit circle */
    uint32_t rng = args.seed ? args.seed : 1u;
    for (int32_t i = 0; i < source_count; ++i) {
        float x, y;
        unit_vector(&x, &y, &rng);
        source_positions[i*2+0] = x;
        source_positions[i*2+1] = y;
    }

    /* Warm-up runs (not timed) */
    for (int32_t w = 0; w < args.warmup; ++w) {
        drb_vbap_gain_matrix(vbap, source_positions, speaker_gains, source_count);
    }

    /* Timed runs */
    double const t0 = bench_now_seconds();
    for (int32_t f = 0; f < args.frames; ++f) {
        drb_vbap_gain_matrix(vbap, source_positions, speaker_gains, source_count);
    }
    double const t1 = bench_now_seconds();
    double const elapsed = t1 - t0;

    /* Validate a small sample for L2-norm closeness */
    int32_t const sample = source_count < 1024 ? source_count : 1024;
    double max_err = 0.0;
    for (int32_t i = 0; i < sample; ++i) {
        double e = l2_row_error(&speaker_gains[(size_t)i * (size_t)speaker_count], speaker_count);
        if (e > max_err) max_err = e;
    }

    /* Stats */
    double const calls = (double)args.frames;
    double const total_sources = (double)args.frames * (double)source_count;
    double const total_gains = total_sources * (double)speaker_count;
    double const ns_per_source = (elapsed / total_sources) * 1e9;
    double const srcs_per_s = total_sources / elapsed;
    double const gains_per_s = total_gains / elapsed;
    double const ms_per_call = (elapsed / calls) * 1e3;

    if (args.csv) {
        printf("layout,speakers,resolution,sources,frames,elapsed_s,ns_per_source,sources_per_s,gains_per_s,max_l2_err,version\n");
        printf("%s,%d,%d,%d,%d,%.6f,%.3f,%.3f,%.3f,%.3e,%d.%d.%d\n",
               args.layout_tag,
               speaker_count,
               layout->resolution,
               source_count,
               args.frames,
               elapsed,
               ns_per_source,
               srcs_per_s,
               gains_per_s,
               max_err,
               maj, min, pat);
    } else {
        printf("DrB VBAP version: %d.%d.%d\n", maj, min, pat);
        printf("Layout: %s (speakers=%d, resolution=%d)\n", args.layout_tag, speaker_count, layout->resolution);
        printf("Sources: %d, Frames: %d, Warm-up: %d\n", source_count, args.frames, args.warmup);
        printf("Elapsed: %.6f s | per call: %.3f ms | ns/source: %.3f\n", elapsed, ms_per_call, ns_per_source);
        printf("Throughput: %.3f M sources/s | %.3f M gains/s\n", srcs_per_s * 1e-6, gains_per_s * 1e-6);
        printf("L2-norm error (max over first %d rows): %.3e\n", sample, max_err);
    }

    bench_aligned_free(source_positions);
    bench_aligned_free(speaker_gains);
    bench_aligned_free(vbap_mem);
    return 0;
}

/*

DrB VBAP version: 0.1.0
Layout: surround-5 (speakers=5, resolution=36)
Sources: 16384, Frames: 8192, Warm-up: 3
Elapsed: 1.959728 s | per call: 0.239 ms | ns/source: 14.601
Throughput: 68.488 M sources/s | 342.440 M gains/s
L2-norm error (max over first 1024 rows): 2.648e-07

DrB VBAP version: 0.1.0
Layout: surround-5 (speakers=5, resolution=36)
Sources: 16384, Frames: 8192, Warm-up: 3
Elapsed: 2.110499 s | per call: 0.258 ms | ns/source: 15.724
Throughput: 63.595 M sources/s | 317.976 M gains/s
L2-norm error (max over first 1024 rows): 2.254e-07

*/
