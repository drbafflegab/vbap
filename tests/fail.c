//#include "drb-check.h"
//#include "drb-vbap.h"
//
//#include <math.h>
//#include <stdbool.h>
//#include <stdlib.h>
//
//typedef struct
//{
//    int resolution;
//    int speaker_count;
//    int const * speaker_angles;
//    DrB_VBAP_Error expected_error;
//}
//Test_Case;
//
//Test_Case const test_cases [] =
//{
//    {
//        .resolution = -1,
//        .speaker_count = 2,
//        .speaker_angles = NULL,
//        .expected_error = drb_vbap_error_invalid_resolution
//    },
//    {
//        .resolution = 10000,
//        .speaker_count = 2,
//        .speaker_angles = NULL,
//        .expected_error = drb_vbap_error_invalid_resolution
//    },
//    {
//        .resolution = 36,
//        .speaker_count = -1,
//        .speaker_angles = NULL,
//        .expected_error = drb_vbap_error_invalid_speaker_count
//    },
//    {
//        .resolution = 36,
//        .speaker_count = 500,
//        .speaker_angles = NULL,
//        .expected_error = drb_vbap_error_invalid_speaker_count
//    },
//    {
//        .resolution = 36,
//        .speaker_count = 6,
//        .speaker_angles = (int [6]){ 0, 1, 2, 2 },
//        .expected_error = drb_vbap_error_invalid_speaker_angles
//    },
//    {
//        .resolution = 36,
//        .speaker_count = 6,
//        .speaker_angles = (int [6]){ 5, 4 },
//        .expected_error = drb_vbap_error_invalid_speaker_angles
//    },
//    {
//        .resolution = 36,
//        .speaker_count = 6,
//        .speaker_angles = (int [6]){ -1 },
//        .expected_error = drb_vbap_error_invalid_speaker_angles
//    },
//    {
//        .resolution = 36,
//        .speaker_count = 6,
//        .speaker_angles = (int [6]){ 8 },
//        .expected_error = drb_vbap_error_invalid_speaker_angles
//    }
//};
//
//enum { test_case_count = sizeof(Test_Case) / sizeof(test_cases) };
//
//#if defined(DRB_USE_TEST_DRIVER)
//extern int drb_vbap_test_fail (void)
//#else
//extern int main (void)
//#endif
//{
//    for (int index = 0; index < test_case_count; index++)
//    {
//        Test_Case const * const test_case = &test_cases[index];
//
//        DrB_VBAP_Error error = 0;
//
//        DrB_VBAP const * const vbap = drb_vbap_construct
//        (
//            NULL,
//            test_case->resolution,
//            test_case->speaker_angles,
//            test_case->speaker_count,
//            &error
//        );
//
//        CHECK(vbap == NULL && error == test_case->expected_error);
//    }
//
//    return EXIT_SUCCESS;
//}
