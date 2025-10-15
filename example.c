#include "drb-vbap.h"

#include <stdio.h>
#include <stdlib.h>

extern int main (void)
{
    static int const resolution = 36; // 10° steps.
    static int const speakers [] = { 0, 3, 11, 25, 33 }; // 0°, 30°, 110°, ….
    static int const speaker_count = sizeof(speakers) / sizeof(speakers[0]);

    void * const memory = malloc(drb_vbap_2d_size(resolution, speaker_count));

    DrB_VBAP_2D_Error error;

    DrB_VBAP_2D * const vbap =
     drb_vbap_2d_construct(memory, resolution, speakers, speaker_count, &error);

    if (vbap == NULL)
    {
        fprintf(stderr, "VBAP construction failed: error code %d\n", error);

        return EXIT_FAILURE;
    }

    float gains [5];

    // Pan a single source around on the unit circle in 5° steps.
    for (int angle = 0; angle <= 72; angle++)
    {
        float const sources [1] = { (float)angle * 2.0f * 3.14159265 / 72.0f };

        drb_vbap_2d_compute_gains(vbap, sources, 1, gains);

        printf("%3d° :", angle);

        for (int s = 0; s < speaker_count; s++) { printf(" %6.3f", gains[s]); }

        printf("\n");
    }

    free(memory);

    return EXIT_SUCCESS;
}
