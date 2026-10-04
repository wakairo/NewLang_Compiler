#ifndef NEWLANG_TEST_SUPPORT_H
#define NEWLANG_TEST_SUPPORT_H

#include <stdbool.h>
#include <stdio.h>

/* Only for bool-returning test functions. Checks remain active under NDEBUG. */
#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);    \
            return false;                                                      \
        }                                                                      \
    } while (0)

#endif
