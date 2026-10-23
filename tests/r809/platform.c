/* R809: passive ORIGINAL physical malloc/free forwarding and NULL-only fault.
 * The observed binary is NOT the untouched/default GCC/Clang O2 binary. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
void *__real_malloc(size_t);
void __real_free(void *);
void r809_observe_free(uintptr_t);
static size_t reqs, freed;
static uintptr_t addresses[2];
unsigned test_fail_number(void)
{
    const char *p = getenv("R809_FAIL");
    if (!p || (p[0] != '0' && p[0] != '1' && p[0] != '2') || p[1])
        abort();
    return (unsigned)(p[0] - '0');
}
void *__wrap_malloc(size_t amount)
{
    size_t request = ++reqs;
    if (request > 2 || amount != 24)
        abort();
    if (test_fail_number() == request)
        return NULL;
    void *p = __real_malloc(amount);
    if (!p || (request == 2 && (uintptr_t)p == addresses[0]))
        abort();
    addresses[request - 1] = (uintptr_t)p;
    return p;
}
void __wrap_free(void *p)
{
    if (!p || freed >= 2)
        abort();
    uintptr_t expect = addresses[1] && freed == 0 ? addresses[1] : addresses[0];
    if ((uintptr_t)p != expect)
        abort();
    r809_observe_free((uintptr_t)p);
    ++freed;
    __real_free(p);
}
size_t test_malloc_calls(void) { return reqs; }
size_t test_free_calls(void) { return freed; }
uintptr_t test_allocated_address(size_t index)
{
    return index < 2 ? addresses[index] : 0;
}
