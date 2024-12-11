#include <stdio.h> // For `printf`.
#include <stdlib.h> // For `EXIT_SUCCESS`.

extern void test_sanity (void);

extern int main (int const argc, char const * const argv [const])
{
    (void)argc, (void)argv;

    test_sanity();

    printf("All tests succeeded!\n");

    return EXIT_SUCCESS;
}
