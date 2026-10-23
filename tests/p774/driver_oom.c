/* Real driver, selected fail-at-N; output FILE is private and never published.
 */
#define main p774_driver_main
#include "../../src/main.c"
#undef main
#include "../support/test.h"
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting, hit;
static size_t calls, at;
static bool fail(void)
{
    if (!injecting)
        return false;
    size_t i = calls++;
    if (i != at)
        return false;
    hit = true;
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
static bool driver(const char *path)
{
    FILE *output = tmpfile(), *diagnostics = tmpfile();
    CHECK(output && diagnostics);
    int saved = dup(STDERR_FILENO);
    CHECK(saved >= 0 && fflush(stderr) == 0 &&
          dup2(fileno(diagnostics), STDERR_FILENO) >= 0);
    at = SIZE_MAX;
    calls = 0;
    hit = false;
    injecting = true;
    int result = compile_v0_to(path, false, NULL, output);
    injecting = false;
    size_t total = calls;
    CHECK(result == 0 && !hit && ftell(output) > 0 && total > 16);
    size_t selected[] = {0,
                         1,
                         2,
                         3,
                         4,
                         5,
                         6,
                         7,
                         total / 8,
                         total / 4,
                         total / 3,
                         total / 2,
                         3 * total / 4,
                         7 * total / 8,
                         total - 2,
                         total - 1};
    size_t counts[5] = {0};
    for (size_t i = 0; i < 16; ++i) {
        CHECK(fflush(output) == 0 && ftruncate(fileno(output), 0) == 0 &&
              fseek(output, 0, SEEK_SET) == 0);
        CHECK(fflush(stderr) == 0 && ftruncate(fileno(diagnostics), 0) == 0 &&
              fseek(diagnostics, 0, SEEK_SET) == 0);
        at = selected[i];
        calls = 0;
        hit = false;
        injecting = true;
        result = compile_v0_to(path, false, NULL, output);
        injecting = false;
        CHECK(hit && result > 0 && result < 5 && ftell(output) == 0);
        ++counts[result];
        CHECK(fflush(stderr) == 0 && fseek(diagnostics, 0, SEEK_SET) == 0);
        char text[8192] = {0};
        size_t size = fread(text, 1, sizeof(text) - 1, diagnostics);
        text[size] = 0;
        CHECK(!strstr(text, "error(internal)"));
        CHECK(compile_v0_to(path, false, NULL, output) == 0 &&
              ftell(output) > 0);
    }
    CHECK(fflush(stderr) == 0 && dup2(saved, STDERR_FILENO) >= 0 &&
          close(saved) == 0 && fclose(output) == 0 && fclose(diagnostics) == 0);
    printf("DRIVER OOM_total=%zu selected_positions=16 CLI1=%zu CLI3=%zu "
           "CLI4=%zu zero_partial_C=PASS no_internal=PASS retry=PASS\n",
           total, counts[1], counts[3], counts[4]);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && driver(argv[1]) ? 0 : 1;
}
