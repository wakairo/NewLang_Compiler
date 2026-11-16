#include <stddef.h>
#ifndef P1463_FAIL_AT
#define P1463_FAIL_AT 0
#endif
void *__real_malloc(size_t);
void __real_free(void *);
void p1463_observe_free(void *);
static size_t trials;
void *__wrap_malloc(size_t size)
{
    ++trials;
    if (P1463_FAIL_AT && trials == P1463_FAIL_AT)
        return NULL;
    return __real_malloc(size);
}
void __wrap_free(void *p)
{
    p1463_observe_free(p);
    __real_free(p);
}
