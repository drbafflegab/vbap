#include "drb-check.h"
#include "drb-vbap.h"

#include <stdbool.h>
#include <stdlib.h>

#if defined(DRB_USE_TEST_DRIVER)
extern int drb_vbap_test_builtin (void)
#else
extern int main (void)
#endif
{
    char const * tags [] =
    {
        DRB_VBAP_LAYOUT_TAG_SURROUND_5,
        DRB_VBAP_LAYOUT_TAG_SURROUND_7,
        DRB_VBAP_LAYOUT_TAG_QUADROPHONIC,
        DRB_VBAP_LAYOUT_TAG_HEXAGONAL,
        DRB_VBAP_LAYOUT_TAG_OCTOPHONIC,
        DRB_VBAP_LAYOUT_TAG_DODECAPHONIC
    };

    for (int i = 0; i < 6; i++)
    {
        DrB_VBAP_Layout const * const layout = drb_vbap_builtin_layout(tags[i]);

        CHECK(layout != NULL);

        size_t const size = drb_vbap_size(layout);

        CHECK(size > 0);

        void * const memory = malloc(size);

        CHECK(memory != NULL);

        DrB_VBAP_Error error = 0;

        DrB_VBAP const * const vbap = drb_vbap_construct(memory, layout, &error);

        CHECK(vbap != NULL);
        CHECK(error == 0);

        free(memory);
    }

    return EXIT_SUCCESS;
}
