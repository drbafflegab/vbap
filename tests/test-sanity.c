#include "drb-vbap.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_NAME "Sanity"

static float const epsilon = 1.0e-5f;

static float const pi = 3.1415926535f;

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
      0.0f * (pi / 180.0f),
     45.0f * (pi / 180.0f),
     90.0f * (pi / 180.0f),
    135.0f * (pi / 180.0f),
    180.0f * (pi / 180.0f),
    225.0f * (pi / 180.0f),
    270.0f * (pi / 180.0f),
    315.0f * (pi / 180.0f)
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

extern int main (int const argc, char const * const argv [])
{
    (void)argc, (void)argv;

    size_t const size = drb_vbap_2d_size(resolution, speaker_count);

    void * const memory = malloc(size);

    assert(memory != NULL);

    DrB_VBAP_2D const * const vbap = drb_vbap_2d_construct
    (
        memory,
        resolution,
        speaker_positions,
        speaker_count,
        NULL
    );

    assert(vbap != NULL);

    float gains [speaker_count * source_count];

    drb_vbap_2d_compute_gains(vbap, source_angles, source_count, gains);

    free(memory);

    for (int source = 0; source < source_count; source++)
    {
        for (int speaker = 0; speaker < speaker_count; speaker++)
        {
            float const gain = gains[source * speaker_count + speaker];

            assert(fabsf(reference_gains[source][speaker] - gain) < epsilon);
        }
    }

    return EXIT_SUCCESS;
}
