/* Run the real ordinary CLI, arming failure only in its actual CHECK/emitter
 * call. This is test instrumentation, not a production CLI option. */
#define main p1272_cli_main
#include "main.c"
#undef main
#include <assert.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
NLCheckStatus __real_nl_semantic_check_expression(NLSemanticContext *,
                                                  const NLSyntaxTree *,
                                                  NLCheckedFragment **,
                                                  NLCheckDiagnostic *);
NLNodeCStatus __real_nl_checked_c_node(const NLCheckedFragment *, char **,
                                       size_t *);
static bool armed, fired;
static size_t ordinal, hits, calls;
static bool checking;
static bool fail(void)
{
    if (!armed)
        return false;
    if (++hits != ordinal)
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
NLCheckStatus __wrap_nl_semantic_check_expression(NLSemanticContext *c,
                                                  const NLSyntaxTree *t,
                                                  NLCheckedFragment **f,
                                                  NLCheckDiagnostic *d)
{
    if (checking) {
        ++calls;
        armed = true;
    }
    const NLCheckStatus status =
        __real_nl_semantic_check_expression(c, t, f, d);
    armed = false;
    return status;
}
NLNodeCStatus __wrap_nl_checked_c_node(const NLCheckedFragment *f, char **out,
                                       size_t *length)
{
    if (!checking) {
        ++calls;
        armed = true;
    }
    const NLNodeCStatus status = __real_nl_checked_c_node(f, out, length);
    armed = false;
    return status;
}
int main(int argc, char **argv)
{
    assert(argc == 5);
    checking = strcmp(argv[3], "check") == 0;
    ordinal = (size_t)strtoul(argv[4], NULL, 10);
    char *arguments[] = {"newlangc", "--emit-c17-to", argv[2], argv[1], NULL};
    const int status = p1272_cli_main(4, arguments);
    printf("NAMED phase=%s allocations=%zu fired=%u cli-status=%d\n", argv[3],
           hits, fired, status);
    assert(calls == 1);
    if (ordinal)
        assert(fired && status == 5);
    else
        assert(!fired && status == 0 && hits);
    return 0;
}
