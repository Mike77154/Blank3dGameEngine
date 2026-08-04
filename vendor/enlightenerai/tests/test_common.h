#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("ASSERT FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS() \
    do { \
        printf("PASS: %s\n", __FILE__); \
        return 0; \
    } while (0)

#endif
