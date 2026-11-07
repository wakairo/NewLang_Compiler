/* Retain the frozen C275 harness; record every interposed allocator failure.
 * GCC Release can lower malloc plus zeroing to calloc in unchanged code. */
#define __wrap_calloc c275_original_wrap_calloc
#define __wrap_realloc c275_original_wrap_realloc
#include "../c275/p733_current_v1.c"
#undef __wrap_realloc
#undef __wrap_calloc

void *__wrap_calloc(size_t n, size_t s)
{
    if (inject && calls++ == at) {
        failure_injected = true;
        failure_site = __builtin_return_address(0);
        return NULL;
    }
    return __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (inject && calls++ == at) {
        failure_injected = true;
        failure_site = __builtin_return_address(0);
        return NULL;
    }
    return __real_realloc(p, n);
}
