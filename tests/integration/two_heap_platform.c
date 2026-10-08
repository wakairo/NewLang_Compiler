/* NULL-only platform injection. Never initializes or repairs heap bytes;
 * every successful request and release delegates to the real platform. */
#include <stdint.h>
#include <stdlib.h>
void *__real_malloc(size_t);
void __real_free(void *);
#ifndef NEWLANG_UNOBSERVED
void test_observe_free(uintptr_t);
#endif
static size_t requests, releases;
static uintptr_t addresses[2];
void *__wrap_malloc(size_t size)
{
    size_t index = requests++;
    if (index >= 2 || size != 24)
        abort();
    const char *failure = getenv("NEWLANG_TEST_FAIL_CALL");
    if (failure != NULL && (size_t)(failure[0] - '0') == index + 1)
        return NULL;
    void *p = __real_malloc(size);
    addresses[index] = (uintptr_t)p;
    return p;
}
void __wrap_free(void *p)
{
    ++releases;
#ifndef NEWLANG_UNOBSERVED
    test_observe_free((uintptr_t)p);
#endif
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
uintptr_t test_allocated_address(size_t i)
{
    return i < 2 ? addresses[i] : 0;
}
