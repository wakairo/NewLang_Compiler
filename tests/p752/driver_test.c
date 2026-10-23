/* Invoke the real driver path with bounded allocation injection. No guest C
 * is authorized for mixed Stage1, including the no-fault backend refusal. */
#define main p752_real_driver_main
#include "../../src/main.c"
#undef main
#include "../support/test.h"

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t calls, at;
void *__wrap_malloc(size_t n)
{
    return injecting && calls++ == at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && calls++ == at ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && calls++ == at ? NULL : __real_realloc(p, n);
}
static bool run(const char *path, bool check)
{
    FILE *diagnostics = tmpfile();
    CHECK(diagnostics && fflush(stderr) == 0);
    int saved = dup(STDERR_FILENO);
    CHECK(saved >= 0 && dup2(fileno(diagnostics), STDERR_FILENO) >= 0);
    size_t counts[5] = {0};
    bool done = false, ok = true;
    for (at = 0; at < 10000; ++at) {
        FILE *output = tmpfile();
        if (!output) {
            ok = false;
            break;
        }
        if (fflush(stderr) || ftruncate(fileno(diagnostics), 0) ||
            fseek(diagnostics, 0, SEEK_SET)) {
            ok = false;
            fclose(output);
            break;
        }
        calls = 0;
        injecting = true;
        int result = compile_v0_to(path, check, NULL, output);
        injecting = false;
        long length = ftell(output);
        if (fclose(output) || length != 0 || result < 0 || result > 4) {
            ok = false;
            break;
        }
        if (calls <= at) {
            done = result == (check ? 0 : 4);
            break;
        }
        if (fflush(stderr) || fseek(diagnostics, 0, SEEK_SET)) {
            ok = false;
            break;
        }
        char message[8192] = {0};
        size_t n = fread(message, 1, sizeof(message) - 1, diagnostics);
        message[n] = 0;
        if (strstr(message, "error(internal)") ||
            (result == 4 && !strstr(message, "V1-BACKEND-UNSUPPORTED")) ||
            (result == 3 && !strstr(message, "OUT-OF-MEMORY") &&
             !strstr(message, "[V0-CHECK]"))) {
            ok = false;
            break;
        }
        ++counts[result];
    }
    CHECK(fflush(stderr) == 0 && dup2(saved, STDERR_FILENO) >= 0 &&
          close(saved) == 0);
    if (!ok || !done)
        fprintf(stderr, "driver fail point=%zu calls=%zu\n", at, calls);
    CHECK(fclose(diagnostics) == 0 && ok && done && at > 0 && at < 10000);
    printf("DRIVER check=%u positions=%zu CLI0=%zu CLI1=%zu CLI3=%zu CLI4=%zu "
           "zero_partial_C=PASS no_internal=PASS recovery=PASS\n",
           check, at, counts[0], counts[1], counts[3], counts[4]);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && run(argv[1], true) && run(argv[1], false) ? 0 : 1;
}
