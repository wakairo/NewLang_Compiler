#include <stddef.h>
#include <stdlib.h>
static size_t trials;
void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
    ++trials;
    if (P1441_FAIL_AT && trials == P1441_FAIL_AT)
        return NULL;
    return __real_malloc(size);
}
