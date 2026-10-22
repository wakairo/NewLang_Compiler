/* R736 platform: only NULL injection; success/free are actual libc. */
#include <stdint.h>
#include <stdlib.h>
void *__real_malloc(size_t);
void __real_free(void *);
#ifndef NEWLANG_UNOBSERVED
void test_observe_free(uintptr_t);
#endif
static size_t attempts, releases;
static uintptr_t original[2];
void *__wrap_malloc(size_t n)
{
    const size_t i = attempts++;
    if (i >= 2 || n == 0)
        abort();
    const char *fail = getenv("R736_FAIL_ALLOC");
    if (fail && fail[0] >= '1' && fail[0] <= '2' && fail[1] == 0 &&
        (size_t)(fail[0] - '0') == i + 1)
        return NULL;
    void *p = __real_malloc(n);
    original[i] = (uintptr_t)p;
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
size_t r736_alloc_calls(void)
{
    return attempts;
}
size_t r736_free_calls(void)
{
    return releases;
}
uintptr_t r736_alloc_at(size_t i)
{
    return i < 2 ? original[i] : 0;
}
