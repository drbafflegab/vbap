#include <stdlib.h>

extern void test_constant_power_sweep (void);
extern void test_exact_speaker_hits (void);
extern void test_fail (void);
extern void test_fuzz (void);
extern void test_midpoint_equal_power (void);
extern void test_sanity (void);
extern void test_wraparound_pair (void);

extern int main (int const argc, char const * const argv [const])
{
    (void)argc, (void)argv;

    test_constant_power_sweep();
    test_exact_speaker_hits();
    test_fail();
    test_fuzz();
    test_midpoint_equal_power();
    test_sanity();
    test_wraparound_pair();

    return EXIT_SUCCESS;
}
