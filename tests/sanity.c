#include "drb-check.h"
#include "drb-vbap.h"

#include <math.h>
#include <stdlib.h>

static float const epsilon = 1.0e-5f;

enum { speaker_count = 4 };

static DrB_VBAP_Layout const layout =
{
    .resolution = 8,
    .speaker_steps = (int32_t [speaker_count]){ 1, 3, 5, 7 },
    .speaker_count = speaker_count
};

enum { source_count = 8 };

static float const source_positions [source_count * 2] =
{
    +1.0000000f,  0.0000000f, // cos   0°, sin   0° (→)
    +0.7071068f, +0.7071068f, // cos  45°, sin  45° (↗)
     0.0000000f, +1.0000000f, // cos  90°, sin  90° (↑)
    -0.7071068f, +0.7071068f, // cos 135°, sin 135° (↖)
    -1.0000000f,  0.0000000f, // cos 180°, sin 180° (←)
    -0.7071068f, -0.7071068f, // cos 225°, sin 225° (↙)
     0.0000000f, -1.0000000f, // cos 270°, sin 270° (↓)
    +0.7071068f, -0.7071068f  // cos 315°, sin 315° (↘)
};

static float const reference_gains [source_count][speaker_count] =
{
    { 0.7071068f, 0.0000000f, 0.0000000f, 0.7071068f }, // Between spk. 4 and 1.
    { 1.0000000f, 0.0000000f, 0.0000000f, 0.0000000f }, // Centered at spk. 1.
    { 0.7071068f, 0.7071068f, 0.0000000f, 0.0000000f }, // Between spk. 1 and 2.
    { 0.0000000f, 1.0000000f, 0.0000000f, 0.0000000f }, // Centered at spk. 2.
    { 0.0000000f, 0.7071068f, 0.7071068f, 0.0000000f }, // Between spk. 2 and 3.
    { 0.0000000f, 0.0000000f, 1.0000000f, 0.0000000f }, // Centered at spk. 3.
    { 0.0000000f, 0.0000000f, 0.7071068f, 0.7071068f }, // Between spk. 3 and 4.
    { 0.0000000f, 0.0000000f, 0.0000000f, 1.0000000f }  // Centered at spk. 4.
};

#if defined(DRB_USE_TEST_DRIVER)
extern int drb_vbap_test_sanity (void)
#else
extern int main (void)
#endif
{
    size_t const size = drb_vbap_size(&layout);

    void * const memory = malloc(size);

    CHECK(memory != NULL);

    DrB_VBAP const * const vbap = drb_vbap_construct(memory, &layout, NULL);

    CHECK(vbap != NULL);

    float speaker_gains [source_count * speaker_count];

    drb_vbap_gain_matrix(vbap, source_positions, speaker_gains, source_count);

    free(memory);

    for (int source = 0; source < source_count; source++)
    {
        for (int speaker = 0; speaker < speaker_count; speaker++)
        {
            int_fast32_t const index = source * speaker_count + speaker;

            float const estimated_gain = speaker_gains[index];
            float const reference_gain = reference_gains[source][speaker];

            CHECK(fabsf(estimated_gain - reference_gain) < epsilon);
        }
    }

    return EXIT_SUCCESS;
}
