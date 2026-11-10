#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void *__real_malloc(size_t);
void __real_free(void *);
static unsigned world, attempts, successes, releases;
static uintptr_t addresses[2];
static unsigned live[2];
static int configured;

static void finish_allocator(void)
{
    assert(attempts == (world ? 2u : 1u));
    assert(successes == world && releases == successes && !live[0] && !live[1]);
    printf("ALLOCATOR world=%u attempts=%u successes=%u frees=%u PASS\n", world,
           attempts, successes, releases);
}
static void configure(void)
{
    if (configured)
        return;
    const char *s = getenv("P1312_WORLD");
    assert(s && s[0] >= '0' && s[0] <= '2' && !s[1]);
    world = (unsigned)(s[0] - '0');
    configured = 1;
    assert(atexit(finish_allocator) == 0);
}
void *__wrap_malloc(size_t size)
{
    configure();
    assert(size == 24 && attempts < 2);
    ++attempts;
    if (world < 2 && attempts == world + 1) {
        errno = ENOMEM;
        return NULL;
    }
    void *p = __real_malloc(size);
    assert(p && successes < 2);
    addresses[successes] = (uintptr_t)p;
    live[successes++] = 1;
    return p;
}
void __wrap_free(void *p)
{
    assert(configured && p);
    size_t i = 0;
    while (i < successes && addresses[i] != (uintptr_t)p)
        ++i;
    assert(i < successes && live[i]);
    live[i] = 0;
    ++releases;
    __real_free(p);
}
unsigned p1312_world(void)
{
    configure();
    return world;
}
unsigned p1312_attempts(void)
{
    return attempts;
}
unsigned p1312_successes(void)
{
    return successes;
}
unsigned p1312_frees(void)
{
    return releases;
}
