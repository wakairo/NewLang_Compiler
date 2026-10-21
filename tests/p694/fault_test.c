/* Test-private filesystem/allocator faults; no production fault switches. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/test.h"

FILE *__real_fdopen(int, const char *);
int __real_mkstemp(char *);
int __real_fflush(FILE *);
int __real_fclose(FILE *);
int __real_rename(const char *, const char *);
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static enum {
    NONE,
    FLUSH,
    CLOSE,
    COMMIT
} fault;
static FILE *sink;
static char last_temp[4096];
static bool injecting;
static size_t calls, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && calls++ == fail_at ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_realloc(p, n);
}
int __wrap_mkstemp(char *path)
{
    int fd = __real_mkstemp(path);
    if (fd >= 0)
        (void)snprintf(last_temp, sizeof(last_temp), "%s", path);
    return fd;
}
FILE *__wrap_fdopen(int fd, const char *mode)
{
    sink = __real_fdopen(fd, mode);
    return sink;
}
int __wrap_fflush(FILE *f)
{
    int r = __real_fflush(f);
    if (f == sink && fault == FLUSH) {
        errno = ENOSPC;
        return EOF;
    }
    return r;
}
int __wrap_fclose(FILE *f)
{
    const bool staged = f == sink;
    int r = __real_fclose(f);
    if (staged) {
        sink = NULL;
        if (fault == CLOSE) {
            errno = EIO;
            return EOF;
        }
    }
    return r;
}
int __wrap_rename(const char *from, const char *to)
{
    if (fault == COMMIT) {
        errno = EIO;
        return -1;
    }
    return __real_rename(from, to);
}
static const char original[] = "int main(void){return 0;}\n";
static bool preserved(const char *output)
{
    char bytes[sizeof(original)] = {0};
    FILE *f = fopen(output, "rb");
    CHECK(f && fread(bytes, 1, sizeof(bytes), f) == sizeof(original) - 1 &&
          fclose(f) == 0 && strcmp(bytes, original) == 0);
    CHECK(!sink &&
          (!*last_temp || (access(last_temp, F_OK) != 0 && errno == ENOENT)));
    return true;
}
static bool exercise(const char *source, const char *output)
{
    FILE *f = fopen(output, "wb");
    CHECK(f && fputs(original, f) >= 0 && fclose(f) == 0);
    for (fault = FLUSH; fault <= COMMIT; ++fault) {
        *last_temp = '\0';
        CHECK(emit_c17_to(output, source) == 1 && preserved(output));
        printf("fault=%d old artifact preserved; temporary removed\n", fault);
    }
    fault = NONE;
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        *last_temp = '\0';
        calls = 0;
        injecting = true;
        int status = emit_c17_to(output, source);
        injecting = false;
        if (status == 0)
            break;
        CHECK((status == 1 || status == 3) && preserved(output));
    }
    CHECK(fail_at > 0 && fail_at < 8000 && !sink &&
          access(last_temp, F_OK) != 0 && errno == ENOENT);
    printf("wrapped allocation failures=%zu preserved/cleanup; recovery=PASS\n",
           fail_at);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && exercise(argv[1], argv[2]) ? 0 : 1;
}
