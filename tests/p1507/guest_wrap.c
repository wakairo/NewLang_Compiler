#include <stddef.h>
#include <stdlib.h>
#ifndef P1507_FAIL_AT
#define P1507_FAIL_AT                                                          \
    (getenv("P1507_FAIL_AT")                                                   \
         ? (unsigned)strtoul(getenv("P1507_FAIL_AT"), NULL, 10)                \
         : 0u)
#endif
void *__real_malloc(size_t);
void __real_free(void *);
void p1507_observe_free(void *);
static size_t attempts;
void *__wrap_malloc(size_t size)
{
    ++attempts;
    return P1507_FAIL_AT && attempts == P1507_FAIL_AT ? NULL
                                                      : __real_malloc(size);
}
void __wrap_free(void *p)
{
    p1507_observe_free(p);
    __real_free(p);
}
