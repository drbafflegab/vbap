#include "drb-vbap.h"

#include <assert.h> // For `assert`.
#include <stdlib.h> // For `NULL`, `malloc`, and `free`.
#include <math.h> // For `fabsf`.

#include <stdio.h>

static float const epsilon = 1.0e-5f;

static float const pi = 3.1415926535f;

enum { resolution = 8 }; // 10º per division.

static int const speaker_positions [] =
{
    1, //  45º
    3, // 135º
    5, // 225º
    7  // 315º
};

enum { speaker_count = sizeof(speaker_positions) / sizeof(*speaker_positions) };

enum { source_count = 8 };

float const reference_gains [source_count][speaker_count] =
{
    { 0.707107, 0.000000, 0.000000, 0.707107 },
    { 1.000000, 0.000000, 0.000000, 0.000000 },
    { 0.707107, 0.707107, 0.000000, 0.000000 },
    { 0.000000, 1.000000, 0.000000, 0.000000 },
    { 0.000000, 0.707107, 0.707107, 0.000000 },
    { 0.000000, 0.000000, 1.000000, 0.000000 },
    { 0.000000, 0.000000, 0.707107, 0.707107 },
    { 0.000000, 0.000000, 0.000000, 1.000000 }
};

extern void test_sanity (void)
{
    size_t const size = drb_vbap_2d_size(resolution, speaker_count);

    void * const memory = malloc(size);

    assert(memory != NULL);

    DrB_VBAP_2D const * const vbap = drb_vbap_2d_construct
    (
        memory,
        resolution,
        speaker_positions,
        speaker_count
    );

    assert(vbap != NULL);

    float source_angles [8] =
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
}
