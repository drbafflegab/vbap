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

static void test_error_string_known_codes (void)
{
    char const * const str1 = drb_vbap_error_string(drb_vbap_error_null_pointer);
    char const * const str2 = drb_vbap_error_string(drb_vbap_error_misaligned_pointer);
    char const * const str3 = drb_vbap_error_string(drb_vbap_error_invalid_layout);

    CHECK(str1 != NULL && strcmp(str1, "null pointer") == 0);
    CHECK(str2 != NULL && strcmp(str2, "misaligned pointer") == 0);
    CHECK(str3 != NULL && strcmp(str3, "invalid layout") == 0);
}

static void test_error_string_unknown_code (void)
{
    char const * str = drb_vbap_error_string(999);

    CHECK(str != NULL && strcmp(str, "unknown") == 0);
}

static void test_alignment_value (void)
{
    size_t const alignment = drb_vbap_alignment();

    // Should be a power of 2.

    CHECK(alignment > 0 && (alignment & (alignment - 1)) == 0);

    // malloc() should return memory aligned to this value.

    void * memory = malloc(1024);

    CHECK(memory != NULL);
    CHECK(((uintptr_t)memory % alignment) == 0);

    free(memory);
}

static void test_builtin_layout_null_tag (void)
{
    DrB_VBAP_Layout const * layout = drb_vbap_builtin_layout(NULL);

    CHECK(layout == NULL);
}

static void test_builtin_layout_invalid_tag (void)
{
    DrB_VBAP_Layout const * layout = drb_vbap_builtin_layout("invalid-tag");

    CHECK(layout == NULL);
}

#if defined(DRB_USE_TEST_DRIVER)
extern int drb_vbap_test_api (void)
#else
extern int main (void)
#endif
{
    test_version_all_parameters();
    test_error_string_known_codes();
    test_error_string_unknown_code();
    test_alignment_value();
    test_builtin_layout_null_tag();
    test_builtin_layout_invalid_tag();

    return EXIT_SUCCESS;
}
