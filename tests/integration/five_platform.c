/* Only guest malloc/free calls from the generated application are wrapped.
 * NULL injection does not initialize bytes or repair topology. Observer has
 * no allocator use; compiler runs in a separate process. */
#include <stdint.h>
#include <stdlib.h>
void *__real_malloc(size_t);
void __real_free(void *);
#ifndef NEWLANG_UNOBSERVED
void five_observe_free(uintptr_t);
#endif
static size_t requests, releases;
static uintptr_t addresses[5];
void *__wrap_malloc(size_t n)
{
    size_t i = requests++;
    if (i >= 5 || n != 56)
        abort();
    const char *fail = getenv("NEWLANG_TEST_FAIL_CALL");
    if (fail && fail[0] >= '1' && fail[0] <= '5' && !fail[1] &&
        (size_t)(fail[0] - '0') == i + 1)
        return NULL;
    void *p = __real_malloc(n);
    addresses[i] = (uintptr_t)p;
    return p;
}
void __wrap_free(void *p)
{
    ++releases;
#ifndef NEWLANG_UNOBSERVED
    five_observe_free(
        (uintptr_t)p); /* traps invalid request BEFORE real free */
#endif
    __real_free(p);
}
size_t five_requests(void)
{
    return requests;
}
size_t five_releases(void)
{
    return releases;
}
uintptr_t five_address(size_t i)
{
    return i < 5 ? addresses[i] : 0;
}
