#include "drb-vbap.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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

static float const epsilon = 1.0e-5f;

enum { resolution = 8 }; // 45º per division.

enum { source_count = 8 };

static int const speaker_positions [] =
{
    1, //  45º
    3, // 135º
    5, // 225º
    7  // 315º
};

enum { speaker_count = sizeof(speaker_positions) / sizeof(*speaker_positions) };

static float const source_angles [source_count] =
{
      0.0f * (PI / 180.0f),
     45.0f * (PI / 180.0f),
     90.0f * (PI / 180.0f),
    135.0f * (PI / 180.0f),
    180.0f * (PI / 180.0f),
    225.0f * (PI / 180.0f),
    270.0f * (PI / 180.0f),
    315.0f * (PI / 180.0f)
};

static float const reference_gains [source_count][speaker_count] =
{
    { 0.707107f, 0.000000f, 0.000000f, 0.707107f },
    { 1.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.707107f, 0.707107f, 0.000000f, 0.000000f },
    { 0.000000f, 1.000000f, 0.000000f, 0.000000f },
    { 0.000000f, 0.707107f, 0.707107f, 0.000000f },
    { 0.000000f, 0.000000f, 1.000000f, 0.000000f },
    { 0.000000f, 0.000000f, 0.707107f, 0.707107f },
    { 0.000000f, 0.000000f, 0.000000f, 1.000000f }
};

extern int main (void)
{
    size_t const size = drb_vbap_2d_size(resolution, speaker_count);

    void * const memory = malloc(size);

    ASSERT(memory != NULL);

    DrB_VBAP_2D const * const vbap = drb_vbap_2d_construct
    (
        memory,
        resolution,
        speaker_positions,
        speaker_count,
        NULL
    );

    ASSERT(vbap != NULL);

    float gains [speaker_count * source_count];

    drb_vbap_2d_compute_gains(vbap, source_angles, source_count, gains);

    free(memory);

    for (int source = 0; source < source_count; source++)
    {
        for (int speaker = 0; speaker < speaker_count; speaker++)
        {
            float const gain = gains[source * speaker_count + speaker];
            float const reference_gain = reference_gains[source][speaker];

            ASSERT(fabsf(reference_gain - gain) < epsilon);
        }
    }

    return EXIT_SUCCESS;
}
