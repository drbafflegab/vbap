#include "drb-check.h"
#include "drb-vbap.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline int_fast32_t min (int_fast32_t const a, int_fast32_t const b)
{
    return a < b ? a : b;
}

static uint32_t xorshift32 (void)
{
    static uint32_t state = UINT32_C(0xC0FFEE);

    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;

    return state;
}

static int_fast32_t range (int_fast32_t const low, int_fast32_t const high)
{
    int_fast32_t const span = high - low + 1;

    return low + (int_fast32_t)(xorshift32() % (uint32_t)span);
}

static float uniform (void)
{
    return (float)xorshift32() / (float)UINT32_MAX;
}

static void test_iteration (void)
{
    int_fast32_t const resolution = range(2, 4096);
    int_fast32_t const speaker_count = range(2, min(resolution, 16));
    int_fast32_t const source_count = range(1, 64);
    int_fast32_t const total_gains_count = source_count * speaker_count;

    int32_t * const speaker_angles = malloc(sizeof(int32_t) * speaker_count);

    CHECK(speaker_angles != NULL);

    DrB_VBAP_Layout const layout =
    {
        .resolution = resolution,
        .speaker_steps = speaker_angles,
        .speaker_count = speaker_count
    };

    void * const memory = malloc(drb_vbap_size(&layout));
    float * const source_positions = malloc(sizeof(float) * 2 * source_count);
    float * const speaker_gains_0 = malloc(sizeof(float) * total_gains_count);
    float * const speaker_gains_1 = malloc(sizeof(float) * total_gains_count);

    CHECK(memory != NULL);
    CHECK(source_positions != NULL);
    CHECK(speaker_gains_0 != NULL);
    CHECK(speaker_gains_1 != NULL);

    for (int_fast32_t speaker = 0; speaker < speaker_count; speaker++)
    {
        speaker_angles[speaker] = (speaker * resolution) / speaker_count;
    }

    for (int_fast32_t speaker = 0; speaker < speaker_count; speaker++)
    {
        int_fast32_t const jitter = range(-1, +1);

        speaker_angles[speaker] += jitter + resolution;
        speaker_angles[speaker] %= resolution;
    }

    DrB_VBAP const * const vbap = drb_vbap_construct(memory, &layout, NULL);

    if (vbap != NULL)
    {
        for (int_fast32_t source = 0; source < source_count; source++)
        {
            source_positions[source * 2 + 0] = uniform() * 20.0f - 10.0f;
            source_positions[source * 2 + 1] = uniform() * 20.0f - 10.0f;
        }

        drb_vbap_process(vbap, source_positions, speaker_gains_0, source_count);
        drb_vbap_process(vbap, source_positions, speaker_gains_1, source_count);

        size_t const size = total_gains_count * sizeof(float);

        CHECK(memcmp(speaker_gains_0, speaker_gains_1, size) == 0);

        for (int_fast32_t source = 0; source < source_count; source++)
        {
            for (int_fast32_t speaker = 0; speaker < speaker_count; speaker++)
            {
                int_fast32_t const index = source * speaker_count + speaker;

                CHECK(isfinite(speaker_gains_0[index]));
            }
        }
    }

    free(speaker_gains_1);
    free(speaker_gains_0);
    free(source_positions);
    free(memory);
    free(speaker_angles);
}

#if defined(DRB_USE_TEST_DRIVER)
extern int drb_vbap_test_fuzz (void)
#else
extern int main (void)
#endif
{
    enum { it_count = 10000 };

    for (int_fast32_t it = 0; it < it_count; it++)
    {
        test_iteration();
    }

    return EXIT_SUCCESS;
}
