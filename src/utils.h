#pragma once
#include <assert.h> // assert
#include <stdio.h>  // print
#include <stdlib.h> // exit

#define LENGTH(a) (sizeof(a) / sizeof(a[0]))

#define ASSERT(cond, ...) do {                      \
        if (!(cond)) {                              \
            printf("ASSERT FAILED: " __VA_ARGS__);  \
            printf("\n");                           \
            assert(cond);                           \
            exit(0);                                \
        } } while (0)

#define NOT_IMPL() ASSERT(0, "not implemented")
#define NO_SUCH_TAG(ty) ASSERT(0, "no such " ty " tag")
