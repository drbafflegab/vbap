#include "drb-check.h"
#include "drb-vbap.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct
{
    DrB_VBAP_Layout layout;
    DrB_VBAP_Error expected_error;
}
Test_Case;

Test_Case const test_cases [] =
{
    {
        .layout =
        {
            .resolution = -1,
            .speaker_steps = NULL,
            .speaker_count = 2
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        .layout =
        {
            .resolution = 10000,
            .speaker_steps = NULL,
            .speaker_count = 2
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        .layout =
        {
            .resolution = 36,
            .speaker_steps = NULL,
            .speaker_count = -1
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        .layout =
        {
            .resolution = 36,
            .speaker_steps = NULL,
            .speaker_count = 500
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        .layout =
        {
            .resolution = 36,
            .speaker_steps = (int [6]){ 0, 1, 2, 2 },
            .speaker_count = 6
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        .layout =
        {
            .resolution = 36,
            .speaker_steps = (int [6]){ 5, 4 },
            .speaker_count = 6
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        .layout =
        {
            .resolution = 36,
            .speaker_steps = (int [6]){ -1 },
            .speaker_count = 6
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        .layout =
        {
            .resolution = 36,
            .speaker_steps = (int [6]){ 8 },
            .speaker_count = 6
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        // Span too small (< MIN_SPAN of 5°)
        .layout =
        {
            .resolution = 360,
            .speaker_steps = (int [3]){ 0, 4, 180 },  // 4° span
            .speaker_count = 3
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        // Span too large (> MAX_SPAN of 175°)
        .layout =
        {
            .resolution = 360,
            .speaker_steps = (int [3]){ 0, 176, 270 },  // 176° span
            .speaker_count = 3
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        // Span wraparound too small
        .layout =
        {
            .resolution = 360,
            .speaker_steps = (int [3]){ 2, 180, 358 },  // wraparound = 4°
            .speaker_count = 3
        },
        .expected_error = drb_vbap_error_invalid_layout
    },
    {
        // Span wraparound too large
        .layout =
        {
            .resolution = 360,
            .speaker_steps = (int [2]){ 10, 200 },  // wraparound = 170°, forward = 190° (too large)
            .speaker_count = 2
        },
        .expected_error = drb_vbap_error_invalid_layout
    }
};

enum { test_case_count = sizeof(test_cases) / sizeof(Test_Case) };

#if defined(DRB_USE_TEST_DRIVER)
extern int drb_vbap_test_fail (void)
#else
extern int main (void)
#endif
{
    for (int index = 0; index < test_case_count; index++)
    {
        Test_Case const * const test_case = &test_cases[index];

        // All test cases should fail validation
        // Use drb_vbap_size which returns 0 for invalid layouts
        size_t size = drb_vbap_size(&test_case->layout);
        CHECK(size == 0);
    }

    // Test misaligned pointer
    {
        DrB_VBAP_Layout const layout =
        {
            .resolution = 8,
            .speaker_steps = (int [4]){ 1, 3, 5, 7 },
            .speaker_count = 4
        };

        size_t size = drb_vbap_size(&layout);
        CHECK(size > 0);

        // Allocate memory with extra space for misalignment
        void * memory = malloc(size + 16);
        CHECK(memory != NULL);

        // Create intentionally misaligned pointer (offset by 1 byte)
        void * misaligned = (unsigned char *)memory + 1;

        DrB_VBAP_Error error = 0;
        DrB_VBAP const * vbap = drb_vbap_construct(misaligned, &layout, &error);

        CHECK(vbap == NULL && error == drb_vbap_error_misaligned_pointer);

        free(memory);
    }

    return EXIT_SUCCESS;
}
