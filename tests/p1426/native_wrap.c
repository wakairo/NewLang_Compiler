#include <stddef.h>
#include <stdlib.h>

#ifndef P1426_FAIL_AT
#define P1426_FAIL_AT 0
#endif
static size_t p1426_allocations;
void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
    ++p1426_allocations;
    if (P1426_FAIL_AT && p1426_allocations == P1426_FAIL_AT)
        return NULL;
    return __real_malloc(size);
}
void __real_free(void *);
void __wrap_free(void *pointer)
{
    __real_free(pointer);
}
