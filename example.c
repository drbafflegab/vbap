#include "drb-vbap.h"

#include <assert.h> // For `assert`.
#include <stdio.h> // For `printf`.
#include <stdlib.h> // For `EXIT_SUCCESS`, `NULL`, `malloc`, and `free`.

static float const pi = 3.1415926535f;

enum { resolution = 36 }; // 10º per division.

static int const speaker_positions [] =
{
     0, //   0º - front center (FC)
     3, //  30º - front left   (FL)
    11, // 110º - back  left   (BL)
    25, // 250º - back  right  (BR)
    33  // 330º - front right  (FR)
};

enum { speaker_count = sizeof(speaker_positions) / sizeof(*speaker_positions) };

enum { source_count = 360 };

extern int main (int const argc, char const * const argv [const])
{
    (void)argc, (void)argv; // Suppress unused parameter warnings.

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

    float source_angles [source_count];

    for (int source = 0; source < source_count; source++)
    {
        source_angles[source] = (float)source * (pi / 180.0f);
    }

    float gains [source_count * speaker_count];

    drb_vbap_2d_compute_gains(vbap, source_angles, source_count, gains);

    printf("angle: fc gain:  fl gain:  bl gain:  br gain:  fr gain:  power:\n");

    for (int source = 0; source < source_count; source++)
    {
        float power = 0.0;

        for (int speaker = 0; speaker < speaker_count; speaker++)
        {
            float const gain = gains[source * speaker_count + speaker];

            power += gain * gain;
        }

        printf("%5d, ", source);

        for (int speaker = 0; speaker < speaker_count; speaker++)
        {
            float const gain = gains[source * speaker_count + speaker];

            printf("%f, ", gain);
        }

        printf("%f\n", power);
    }

    free(memory);

    return EXIT_SUCCESS;
}
