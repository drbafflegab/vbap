#include "drb-check.h"
#include "drb-vbap.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static double const epsilon = 1.0e-5;
static double const pi = 3.141592653589793;
static double const sqrt_half = 0.7071067811865476;

static DrB_VBAP_Layout const layouts [] =
{
    {
        // LBR: 30°, 180°, 330°.
        .resolution = 12,
        .speaker_steps = (int32_t[]){ 1, 6, 11 },
        .speaker_count = 3
    },
    {
        // 5.1 (no LFS): 0°, 30°, 110°, 250°, 330°.
        .resolution = 36,
        .speaker_steps = (int32_t[]){ 0, 3, 11, 25, 33 },
        .speaker_count = 5
    },
    {   // 7.1 (no LFS): 0°, 30°, 90°, 150°, 210°, 270°, 330°.
        .resolution = 36,
        .speaker_steps = (int32_t[]){ 0, 3, 9, 15, 21, 27, 33 },
        .speaker_count = 7
    },
    {
        // Every 120°.
        .resolution = 12,
        .speaker_steps = (int32_t[]){ 0, 4, 8 },
        .speaker_count = 3
    },
    {
        // Every 90°.
        .resolution = 16,
        .speaker_steps = (int32_t[]){ 0, 4, 8, 12 },
        .speaker_count = 4
    },
    {
        // Every 60°.
        .resolution = 36,
        .speaker_steps = (int32_t[]){ 0, 6, 12, 18, 24, 30 },
        .speaker_count = 6
    },
    {
        // Small cluster + spread.
        .resolution = 24,
        .speaker_steps = (int32_t[]){ 0, 1, 2, 10, 12, 18 },
        .speaker_count = 6
    },
    {
        // Heavy clustering + wrap.
        .resolution = 18,
        .speaker_steps = (int32_t[]){ 0, 1, 7, 8, 9, 10, 16, 17 },
        .speaker_count = 8
    },
    {
        // Adjacent pairs per quadrant.
        .resolution = 32,
        .speaker_steps = (int32_t[]){ 0, 1, 8, 9, 16, 17, 24, 25 },
        .speaker_count = 8
    },
    {
        // Prime with co-prime step sizes
        .resolution = 37,
        .speaker_steps = (int32_t[]){ 0, 5, 10, 15, 20, 25, 30, 35 },
        .speaker_count = 8
    },
    {
        // High-res “real degrees” style.
        .resolution = 360,
        .speaker_steps = (int32_t[]){ 0, 60, 120, 180, 240, 300 },
        .speaker_count = 6
    }
};

enum { layout_count = sizeof(layouts) / sizeof(DrB_VBAP_Layout) };

static void check_unit_power
    (
        DrB_VBAP const * const vbap,
        int32_t const resolution,
        int32_t const * const speakers,
        int32_t const speaker_count
    )
{
    (void)resolution, (void)speakers;

    static float const source_positions [] =
    {
        -8.413f, +6.992f,
        +3.152f, -9.605f,
        +0.487f, +7.341f,
        -9.771f, -1.228f,
        +5.906f, +0.275f,
        -2.638f, -4.517f
    };

    static int_fast32_t const source_count = 6;

    float * const gains = malloc(speaker_count * source_count * sizeof(float));

    CHECK(gains != NULL);

    drb_vbap_gain_matrix(vbap, source_positions, gains, source_count);

    for (int_fast32_t source = 0; source < source_count; source++)
    {
        double sum = 0.0;

        for (int_fast32_t speaker = 0; speaker < speaker_count; speaker++)
        {
            int_fast32_t const gain_index = source * speaker_count + speaker;

            sum += (double)gains[gain_index] * (double)gains[gain_index];
        }

        CHECK(1.0 - epsilon < sum && sum < 1.0 + epsilon);
    }

    free(gains);
}

static void check_midpoint_equal_power
    (
        DrB_VBAP const * const vbap,
        int32_t const resolution,
        int32_t const * const speakers,
        int32_t const speaker_count
    )
{
    int_fast32_t const source_count = speaker_count - 1;

    float * const source_positions = malloc(source_count * 2 * sizeof(float));
    float * const gains = malloc(speaker_count * source_count * sizeof(float));

    CHECK(source_positions != NULL);
    CHECK(gains != NULL);

    for (int_fast32_t source = 0; source < source_count; source++)
    {
        double const angle =
        (
            pi * ((double)speakers[source + 0] / (double)resolution)
          +
            pi * ((double)speakers[source + 1] / (double)resolution)
        );

        source_positions[source * 2 + 0] = (float)cos(angle);
        source_positions[source * 2 + 1] = (float)sin(angle);
    }

    drb_vbap_gain_matrix(vbap, source_positions, gains, source_count);

    for (int_fast32_t source = 0; source < source_count; source++)
    {
        int_fast32_t const gain_index_0 = source * speaker_count + source + 0;
        int_fast32_t const gain_index_1 = source * speaker_count + source + 1;

        CHECK(fabs((double)fabsf(gains[gain_index_0]) - sqrt_half) < epsilon);
        CHECK(fabs((double)fabsf(gains[gain_index_1]) - sqrt_half) < epsilon);

        for (int_fast32_t other = 0; other < speaker_count; other++)
        {
            if (other != (source + 0) && other != (source + 1))
            {
                int_fast32_t const other_index = source * speaker_count + other;

                CHECK((double)fabsf(gains[other_index]) < epsilon);
            }
        }
    }

    free(gains);
    free(source_positions);
}

static void check_midpoint_equal_power_wrap
    (
        DrB_VBAP const * const vbap,
        int32_t const resolution,
        int32_t const * const speakers,
        int32_t const speaker_count
    )
{
    float * const gains = malloc(speaker_count * sizeof(float));

    double const angle =
    (
        pi * ((double)speakers[speaker_count - 1] / (double)resolution)
      +
        pi * ((double)speakers[0] / (double)resolution) + pi
    );

    float const source_positions [2] = { (float)cos(angle), (float)sin(angle) };

    drb_vbap_gain_matrix(vbap, source_positions, gains, 1);

    CHECK(fabs((double)fabsf(gains[speaker_count - 1]) - sqrt_half) < epsilon);
    CHECK(fabs((double)fabsf(gains[0]) - sqrt_half) < epsilon);

    for (int_fast32_t other = 0; other < speaker_count; other++)
    {
        if (other != (speaker_count - 1) && other != 0)
        {
            CHECK((double)fabsf(gains[other]) < epsilon);
        }
    }
}

static void check_excact_speaker_hit
    (
        DrB_VBAP const * const vbap,
        int32_t const resolution,
        int32_t const * const speakers,
        int32_t const speaker_count
    )
{
    int_fast32_t const source_count = speaker_count - 1;

    float * const source_positions = malloc(source_count * 2 * sizeof(float));
    float * const gains = malloc(speaker_count * source_count * sizeof(float));

    CHECK(source_positions != NULL);
    CHECK(gains != NULL);

    for (int_fast32_t source = 0; source < source_count; source++)
    {
        double const angle =
        (
            2.0 * pi * ((double)speakers[source] / (double)resolution)
        );

        source_positions[source * 2 + 0] = (float)cos(angle);
        source_positions[source * 2 + 1] = (float)sin(angle);
    }

    drb_vbap_gain_matrix(vbap, source_positions, gains, source_count);

    for (int_fast32_t source = 0; source < source_count; source++)
    {
        int_fast32_t const gain_index = source * speaker_count + source;

        CHECK(fabs((double)fabsf(gains[gain_index]) - 1.0) < epsilon);

        for (int_fast32_t other = 0; other < source; other++)
        {
            if (other != source)
            {
                int_fast32_t const other_index = source * speaker_count + other;

                CHECK((double)fabsf(gains[other_index]) < epsilon);
            }
        }
    }

    free(gains);
}

#if defined(DRB_USE_TEST_DRIVER)
extern int drb_vbap_test_properties (void)
#else
extern int main (void)
#endif
{
    for (int_fast32_t index = 0; index < layout_count; index++)
    {
        DrB_VBAP_Layout const * const layout = &layouts[index];

        size_t const size = drb_vbap_size(layout);

        void * const memory = malloc(size);

        CHECK(memory != NULL);

        DrB_VBAP const * const vbap = drb_vbap_construct(memory, layout, NULL);

        CHECK(vbap != NULL);

        check_unit_power
        (
            vbap,
            layouts[index].resolution,
            layouts[index].speaker_steps,
            layouts[index].speaker_count
        );

        check_midpoint_equal_power
        (
            vbap,
            layouts[index].resolution,
            layouts[index].speaker_steps,
            layouts[index].speaker_count
        );

        check_midpoint_equal_power_wrap
        (
            vbap,
            layouts[index].resolution,
            layouts[index].speaker_steps,
            layouts[index].speaker_count
        );

        check_excact_speaker_hit
        (
            vbap,
            layouts[index].resolution,
            layouts[index].speaker_steps,
            layouts[index].speaker_count
        );

        free(memory);
    }

    return EXIT_SUCCESS;
}
