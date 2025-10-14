#include "drb-vbap.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(condition)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            fprintf(stderr, "[FAILED] line %d\n", __LINE__);                   \
                                                                               \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    }                                                                          \
    while (0)                                                                  \

#define PI 3.1415927f

static float const epsilon = 1.0e-4f;

static uint32_t xorshift32 (uint32_t * const state)
{
    uint32_t x = *state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x <<  5;

    return *state = x;
}

static inline int minimum (int const a, int const b)
{
    return a < b ? a : b;
}

static int urand_range (uint32_t * const state, int const low, int const high)
{
    int const span = high - low + 1;

    return low + (int)(xorshift32(state) % (uint32_t)span);
}

static float uniform (uint32_t * const state)
{
    return (float)xorshift32(state) / 4294967295.0f;
}

static float bin_to_angle (int const bin, int const resolution)
{
    return 2.0f * PI * ((float)bin / (float)resolution);
}

static float power2 (float const * const gains, int const count)
{
    double sum = 0.0;

    for (int index = 0; index < count; index++)
    {
        sum += (double)gains[index] * (double)gains[index];
    }

    return (float)sum;
}

static int nonzero_count (float const * const gains, int const gain_count)
{
    int counter = 0;

    for (int index = 0; index < gain_count; index++)
    {
        if (fabsf(gains[index]) > epsilon)
        {
            counter++;
        }
    }

    return counter;
}

static void test_iteration (uint32_t * const state)
{
    int const resolution = urand_range(state, 16, 3600);
    int const speaker_count = urand_range(state, 2, minimum(resolution, 64));
    int const source_count = urand_range(state, 1, 16);
    int const gains_count = source_count * speaker_count;

    // Build a valid ring: unique, sorted, roughly uniform.

    int * const speakers = malloc(sizeof(int) * speaker_count);
    float * const angles = malloc(sizeof(float) * source_count);
    float * const gains1 = malloc(sizeof(float) * gains_count);
    float * const gains2 = malloc(sizeof(float) * gains_count);
    void * const memory = malloc(drb_vbap_2d_size(resolution, speaker_count));

    ASSERT(speakers != NULL);
    ASSERT(angles != NULL);
    ASSERT(gains1 != NULL);
    ASSERT(gains2 != NULL);
    ASSERT(memory != NULL);

    for (int index = 0; index < speaker_count; index++)
    {
        speakers[index] = (index * resolution) / speaker_count;
    }

    for (int index = 0; index < speaker_count; index++)
    {
        int const jitter = urand_range(state, -1, +1);

        speakers[index] = (speakers[index] + jitter + resolution) % resolution;
    }

    DrB_VBAP_2D * const vbap = drb_vbap_2d_construct
    (
        memory,
        resolution,
        speakers,
        speaker_count,
        NULL
    );

    if (!vbap)
    {
        free(memory);
        free(gains2);
        free(gains1);
        free(angles);
        free(speakers);

        return;
    }

    for (int index = 0; index < source_count; index++)
    {
        angles[index] = (uniform(state) * 10.0f - 5.0f) * 2.0f * PI;
    }

    drb_vbap_2d_compute_gains(vbap, angles, source_count, gains1);
    drb_vbap_2d_compute_gains(vbap, angles, source_count, gains2);

    // Deterministic
    ASSERT(memcmp(gains1, gains2, sizeof(float) * gains_count) == 0);

    // Finite & bounded; small sparsity
    for (int index = 0; index < source_count; index++)
    {
        float * const row = &gains1[index * speaker_count];

        for (int i = 0; i < speaker_count; ++i)
        {
            ASSERT(isfinite(row[i]) && row[i] >= -1.0f && row[i] <= +1.0f);
        }

        ASSERT(nonzero_count(row, speaker_count) <= 2);
    }

    // Periodicity (one random source)
    if (source_count >= 2)
    {
        float const shifted_angles [] =
        {
            angles[0] + 2.0f * PI,
            angles[1] - 2.0f * PI
        };

        drb_vbap_2d_compute_gains(vbap, shifted_angles, 2, gains2);

        for (int index = 0; index < speaker_count * 2; index++)
        {
            ASSERT(fabsf(gains2[index] - gains1[index]) < epsilon);
        }
    }

    // Exact speaker hit (random one)

    int const pick = urand_range(state, 0, speaker_count-1);
    float const exact = bin_to_angle(speakers[pick], resolution);

    drb_vbap_2d_compute_gains(vbap, &exact, 1, gains2);

    float const power = power2(gains2, speaker_count);

    ASSERT(1.0f - epsilon < power && power < 1.0f + epsilon);

    free(memory);
    free(gains2);
    free(gains1);
    free(angles);
    free(speakers);
}

extern int main (void)
{
    static int const it_count = 10000;

    uint32_t state = 0xC0FFEEu; // Seed.

    for (int it = 0; it < it_count; it++)
    {
        test_iteration(&state);
    }

    return EXIT_SUCCESS;
}
