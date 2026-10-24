/* R845 Linux/glibc allocator interposer, ONLY for R-owned named CLI fault. */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

extern void *__libc_malloc(size_t size);
extern void *__libc_calloc(size_t number, size_t size);
extern void *__libc_realloc(void *pointer, size_t size);

static unsigned long r845_calls;
static unsigned long r845_fail_at;
static int r845_initialized;

static void r845_init(void)
{
    if (r845_initialized)
        return;
    const char *setting = getenv("R845_FAIL_AT");
    if (setting == NULL || *setting == '\0')
        return;
    r845_initialized = 1;
    unsigned long n = 0;
    for (const char *p = setting; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9')
            return;
        if (n > 1000000)
            return;
        n = 10 * n + (unsigned long)(*p - '0');
    }
    r845_fail_at = n;
}

static int r845_reached_failure(void)
{
    r845_init();
    ++r845_calls;
    if (r845_fail_at == 0 || r845_calls != r845_fail_at)
        return 0;
    static const char trace[] = "R845_REACHED_FAIL\n";
    (void)write(STDERR_FILENO, trace, sizeof(trace) - 1);
    errno = ENOMEM;
    return 1;
}

void *malloc(size_t n)
{
    if (r845_reached_failure())
        return NULL;
    return __libc_malloc(n);
}

void *calloc(size_t count, size_t n)
{
    if (r845_reached_failure())
        return NULL;
    return __libc_calloc(count, n);
}

void *realloc(void *p, size_t n)
{
    if (r845_reached_failure())
        return NULL;
    return __libc_realloc(p, n);
}
