#include "drb-vbap.h"

#include "test-utilities.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct
{
    int resolution;
    int speaker_count;
    int const * speaker_positions;
    DrB_VBAP_2D_Error expected_error;
}
Test_Case;

Test_Case const test_cases [] =
{
    {
        .resolution = -1,
        .speaker_count = 2,
        .speaker_positions = NULL,
        .expected_error = drb_vbap_2d_error_invalid_resolution
    },
    {
        .resolution = 10000,
        .speaker_count = 2,
        .speaker_positions = NULL,
        .expected_error = drb_vbap_2d_error_invalid_resolution
    },
    {
        .resolution = 36,
        .speaker_count = -1,
        .speaker_positions = NULL,
        .expected_error = drb_vbap_2d_error_invalid_speaker_count
    },
    {
        .resolution = 36,
        .speaker_count = 500,
        .speaker_positions = NULL,
        .expected_error = drb_vbap_2d_error_invalid_speaker_count
    },
    {
        .resolution = 36,
        .speaker_count = 6,
        .speaker_positions = (int [6]){ 0, 1, 2, 2 },
        .expected_error = drb_vbap_2d_error_invalid_speaker_positions
    },
    {
        .resolution = 36,
        .speaker_count = 6,
        .speaker_positions = (int [6]){ 5, 4 },
        .expected_error = drb_vbap_2d_error_invalid_speaker_positions
    },
    {
        .resolution = 36,
        .speaker_count = 6,
        .speaker_positions = (int [6]){ -1 },
        .expected_error = drb_vbap_2d_error_invalid_speaker_positions
    },
    {
        .resolution = 36,
        .speaker_count = 6,
        .speaker_positions = (int [6]){ 8 },
        .expected_error = drb_vbap_2d_error_invalid_speaker_positions
    }
};

extern int main (void)
{
    enum { test_cast_count = sizeof(Test_Case) / sizeof(test_cases) };

    for (int index = 0; index < test_cast_count; index++)
    {
        Test_Case const * const test_case = &test_cases[index];

        DrB_VBAP_2D_Error error = -1;

        DrB_VBAP_2D const * const vbap = drb_vbap_2d_construct
        (
            NULL,
            test_case->resolution,
            test_case->speaker_positions,
            test_case->speaker_count,
            &error
        );

        ASSERT(vbap == NULL && error == test_case->expected_error);
    }

    return EXIT_SUCCESS;
}
