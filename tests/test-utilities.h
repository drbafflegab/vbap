#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include <stdio.h>
#include <stdlib.h>

#define ASSERT(condition)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            fprintf(stderr, "[FAILED] line %d\n", __LINE__);                   \
                                                                               \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    }                                                                          \
    while (0)                                                                  \

#endif // TEST_UTILITIES_H
