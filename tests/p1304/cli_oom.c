#define main p1304_actual_cli
#include "main.c"
#undef main
#include <assert.h>
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool armed, fired;
static size_t ordinal, hits;
static bool fail(void)
{
    if (!armed)
        return false;
    ++hits;
    if (!ordinal || hits != ordinal)
        return false;
    fired = true;
    return true;
}
void *__wrap_malloc(size_t n)
{
    return fail() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return fail() ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return fail() ? NULL : __real_realloc(p, n);
}
int main(int argc, char **argv)
{
    assert(argc == 4);
    ordinal = (size_t)strtoul(argv[3], NULL, 10);
    char *args[] = {"newlangc", "--emit-c17-to", argv[2], argv[1], NULL};
    armed = true;
    int status = p1304_actual_cli(4, args);
    armed = false;
    printf("P1304 allocations=%zu fired=%u cli-status=%d\n", hits, fired,
           status);
    return 0;
}
