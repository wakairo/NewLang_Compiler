#include <stddef.h>

#ifndef R1478_FAIL_AT
#define R1478_FAIL_AT 0
#endif

void *__real_malloc(size_t);
void __real_free(void *);
void r1478_observe_free(void *);

static size_t r1478_allocation_attempts;

void *__wrap_malloc(size_t size)
{
    ++r1478_allocation_attempts;
    if (R1478_FAIL_AT && r1478_allocation_attempts == R1478_FAIL_AT)
        return NULL;
    return __real_malloc(size);
}

void __wrap_free(void *p)
{
    r1478_observe_free(p);
    __real_free(p);
}
