#define main p1318_actual_cli
#include "main.c"
#undef main
#include <assert.h>
#include <errno.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool armed, fired, fail_flush;
static size_t ordinal, events;
static bool fail(void)
{
    if (!armed)
        return false;
    ++events;
    if (!ordinal || events != ordinal)
        return false;
    fired = true;
    errno = ENOMEM;
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
int __real_fflush(FILE *);
int __wrap_fflush(FILE *stream)
{
    if (armed && fail_flush && !fired) {
        fired = true;
        errno = ENOSPC;
        return EOF;
    }
    return __real_fflush(stream);
}
int main(int argc, char **argv)
{
    assert(argc == 4);
    fail_flush = !strcmp(argv[3], "flush");
    ordinal = fail_flush ? 0 : (size_t)strtoull(argv[3], NULL, 10);
    char *args[] = {"newlangc", "--emit-c17-to", argv[2], argv[1], NULL};
    armed = true;
    const int status = p1318_actual_cli(4, args);
    armed = false;
    printf("events=%zu fired=%u status=%d\n", events, fired, status);
    return 0;
}
