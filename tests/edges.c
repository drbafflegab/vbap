#include "drb-check.h"
#include "drb-vbap.h"

#include <stdlib.h>

static void test_size_null_layout (void)
{
    size_t size = drb_vbap_size(NULL);

    CHECK(size == 0);
}

static void test_size_invalid_layout (void)
{
    DrB_VBAP_Layout invalid_layout =
    {
        .resolution = -1,
        .speaker_steps = NULL,
        .speaker_count = 0
    };

    size_t size = drb_vbap_size(&invalid_layout);

    CHECK(size == 0);
}

static void test_construct_null_error_parameter (void)
{
    DrB_VBAP_Layout const layout =
    {
        .resolution = 8,
        .speaker_steps = (int32_t []){ 1, 3, 5, 7 },
        .speaker_count = 4
    };

    size_t size = drb_vbap_size(&layout);
    CHECK(size > 0);

    void * memory = malloc(size);
    CHECK(memory != NULL);

    // Pass NULL for error parameter
    DrB_VBAP const * vbap = drb_vbap_construct(memory, &layout, NULL);

    CHECK(vbap != NULL);

    free(memory);
}

static void test_gain_matrix_zero_sources (void)
{
    DrB_VBAP_Layout const layout =
    {
        .resolution = 8,
        .speaker_steps = (int32_t []){ 1, 3, 5, 7 },
        .speaker_count = 4
    };

    size_t size = drb_vbap_size(&layout);
    void * memory = malloc(size);
    CHECK(memory != NULL);

    DrB_VBAP const * vbap = drb_vbap_construct(memory, &layout, NULL);
    CHECK(vbap != NULL);

    float source_positions [2] = { 0.0f, 0.0f };
    float gains [4];

    // Call with 0 sources - should not crash
    drb_vbap_gain_matrix(vbap, source_positions, gains, 0);

    free(memory);
}

#if defined(DRB_USE_TEST_DRIVER)
extern int drb_vbap_test_edges (void)
#else
extern int main (void)
#endif
{
    test_size_null_layout();
    test_size_invalid_layout();
    test_construct_null_error_parameter();
    test_gain_matrix_zero_sources();

    return EXIT_SUCCESS;
}
