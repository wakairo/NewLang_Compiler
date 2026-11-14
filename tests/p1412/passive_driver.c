#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
static size_t requests;
static uintptr_t addresses[2];
void *__real_malloc(size_t);
void __real_free(void *);
void p1412_before_free(void *);
size_t p1412_requests(void)
{
    return requests;
}
uintptr_t p1412_address(size_t i)
{
    if (i >= 2)
        exit(78);
    return addresses[i];
}
void *__wrap_malloc(size_t n)
{
    if (n != 24 || requests >= 2)
        exit(78);
    ++requests;
    void *p = (TEST_WORLD < 2 && requests == TEST_WORLD + 1u)
                  ? NULL
                  : __real_malloc(n);
    addresses[requests - 1] = (uintptr_t)p;
    return p;
}
void __wrap_free(void *p)
{
    if (p)
        p1412_before_free(p);
    __real_free(p);
}
int nl_generated_main(void);
int main(void)
{
    return nl_generated_main();
}
