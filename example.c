#include "drb-vbap.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static const float pi = 3.1415927f;

extern int main (void)
{
    enum { source_count = 37, speaker_count = 5 };

    float source_positions [source_count * 2];
    float speaker_gains [source_count * speaker_count];

    char const * const tag = DRB_VBAP_LAYOUT_TAG_SURROUND_5;

    DrB_VBAP_Layout const * const layout = drb_vbap_builtin_layout(tag);

    void * const memory = malloc(drb_vbap_size(layout));

    DrB_VBAP_Error error;

    DrB_VBAP const * const vbap = drb_vbap_construct(memory, layout, &error);

    if (vbap == NULL)
    {
        fprintf(stderr, "Error: %s.\n", drb_vbap_error_string(error));

        return EXIT_FAILURE;
    }

    for (int_fast32_t source = 0; source < source_count; source++)
    {
        float const theta = (float)source * 2.0f * pi / (float)(source_count-1);

        source_positions[source * 2 + 0] = cosf(theta - pi);
        source_positions[source * 2 + 1] = sinf(theta - pi);
    }

    drb_vbap_process(vbap, source_positions, speaker_gains, source_count);

    for (int_fast32_t source = 0; source < source_count; source++)
    {
        float const theta = (float)source * 360.0f / (float)(source_count-1);

        printf("%+4.0f°:", theta - 180.0f);

        for (int_fast32_t speaker = 0; speaker < speaker_count; speaker++)
        {
            int_fast32_t const index = source * speaker_count + speaker;

            printf(" %7.5f", speaker_gains[index]);
        }

        printf("\n");
    }

    free(memory);

    return EXIT_SUCCESS;
}
