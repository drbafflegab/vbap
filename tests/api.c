#include "drb-check.h"
#include "drb-vbap.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void test_version_all_parameters (void)
{
    int32_t major = -1, minor = -1, patch = -1;

    drb_vbap_version(&major, &minor, &patch);

    CHECK(major == DRB_VBAP_VERSION_MAJOR);
    CHECK(minor == DRB_VBAP_VERSION_MINOR);
    CHECK(patch == DRB_VBAP_VERSION_PATCH);
}

static void test_alignment_value (void)
{
    size_t const alignment = drb_vbap_alignment();

    // Should be a power of 2.

    CHECK(alignment > 0 && (alignment & (alignment - 1)) == 0);

    // malloc() should return memory aligned to this value.

    void * const memory = malloc(1024);

    CHECK(memory != NULL);
    CHECK(((uintptr_t)memory % alignment) == 0);

    free(memory);
}

static void test_builtin_layout_null_tag (void)
{
    DrB_VBAP_Layout const * const layout = drb_vbap_builtin_layout(NULL);

    CHECK(layout == NULL);
}

static void test_builtin_layout_invalid_tag (void)
{
    DrB_VBAP_Layout const * const layout = drb_vbap_builtin_layout("volapuk");

    CHECK(layout == NULL);
}

#if defined(DRB_USE_TEST_DRIVER)
extern int drb_vbap_test_api (void)
#else
extern int main (void)
#endif
{
    test_version_all_parameters();
    test_alignment_value();
    test_builtin_layout_null_tag();
    test_builtin_layout_invalid_tag();

    return EXIT_SUCCESS;
}
