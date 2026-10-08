/* Test-only platform call instrumentation, separate from the read-only oracle.
 * Only generated malloc/free requests reach these wrappers. Failure injection
 * can return NULL only; success delegates malloc unchanged. No extra allocator,
 * initializer, repaired topology, implicit cleanup or synthetic free. */
#include <stdlib.h>
#define VERIFY(x)                                                              \
    do {                                                                       \
        if (!(x))                                                              \
            abort();                                                           \
    } while (0)
#include <stdint.h>
void *__real_malloc(size_t);
void __real_free(void *);
void test_observe_free(uintptr_t);
static size_t requests, releases;
static uintptr_t address;
void *__wrap_malloc(size_t size)
{
    VERIFY(requests++ == 0 && size == 24);
    if (getenv("NEWLANG_TEST_ALLOCATION_FAIL") != NULL)
        return NULL;
    void *p = __real_malloc(size);
    VERIFY(p != NULL);
    address = (uintptr_t)p;
    return p;
}
void __wrap_free(void *p)
{
    VERIFY(releases++ == 0 && (uintptr_t)p == address && address != 0);
    test_observe_free((uintptr_t)p);
    __real_free(p);
}
size_t test_malloc_calls(void)
{
    return requests;
}
size_t test_free_calls(void)
{
    return releases;
}
uintptr_t test_allocated_address(void)
{
    return address;
}
