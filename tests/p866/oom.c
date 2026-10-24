/* Isolate real private prefix allocations on a genuine source-issued owner. */
#include "../../src/copy_body.c"
#include "support.h"
#include <stdint.h>
void *__real_calloc(size_t, size_t);
static bool injecting, hit;
static size_t calls, fail_at;
void *__wrap_calloc(size_t n, size_t size)
{
    if (injecting && calls++ == fail_at) {
        hit = true;
        return NULL;
    }
    return __real_calloc(n, size);
}
static bool exercise(const char *path)
{
    P866 o = {0};
    CHECK(p866_load(path, &o) && p866_valid(&o));
    NLCopyBodyProof *p = o.body->copy_body;
    TestState *before = calloc(1, sizeof(*before));
    CHECK(before &&
          test_state((NLSemanticContext *)o.returned.field.entry, before));
    calls = 0;
    fail_at = SIZE_MAX;
    injecting = true;
    NLCheckStatus s = unit_prefix_validate(p, &o.returned);
    injecting = false;
    CHECK(s == NL_CHECK_OK && calls == 2 && !hit);
    for (fail_at = 0; fail_at < 2; ++fail_at) {
        calls = 0;
        hit = false;
        injecting = true;
        s = unit_prefix_validate(p, &o.returned);
        injecting = false;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && hit && calls == 2 &&
              test_unchanged((NLSemanticContext *)o.returned.field.entry,
                             before) &&
              p866_valid(&o) &&
              unit_prefix_validate(p, &o.returned) == NL_CHECK_OK);
        printf("P866 prefix allocation index=%zu calls=%zu did_fail=1 "
               "status=OOM unchanged=1 public_retry0=1\n",
               fail_at, calls);
    }
    free(before);
    node_checked_destroy(&o.source);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && exercise(argv[1]) && exercise(argv[2]) ? 0 : 1;
}
