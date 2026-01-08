#ifndef DRB_CHECK_H
#define DRB_CHECK_H

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                       \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            fprintf(stderr, "[FAILED] %s, line %d\n", __FILE__, __LINE__);     \
                                                                               \
abort();/*exit(EXIT_FAILURE);*/                                                \
        }                                                                      \
    }                                                                          \
    while (0)                                                                  \

#endif // DRB_CHECK_H
