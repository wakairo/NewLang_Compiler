#define main p1088_original_main
#define __wrap_malloc p1088_original_malloc
#define __wrap_realloc p1088_original_realloc
#include "../unit/loop_test.c"
#undef main
#undef __wrap_malloc
#undef __wrap_realloc

static char failed_kind;
static size_t failed_bytes;
static void *failed_caller;
void *__wrap_malloc(size_t n)
{
    if (injecting && allocation_index == fail_at) {
        failed_kind = 'm';
        failed_bytes = n;
        failed_caller = __builtin_return_address(0);
    }
    return p1088_original_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && allocation_index == fail_at) {
        failed_kind = 'r';
        failed_bytes = n;
        failed_caller = __builtin_return_address(0);
    }
    return p1088_original_realloc(p, n);
}

/* Diagnostic of the unchanged inherited p14_w40 corpus. Unexpected statuses
 * still fail, after deliberate cleanup/retry so LSan can distinguish leaks
 * caused by the original test's early CHECK return from compiler leaks. */
static bool probe(void)
{
    const char *texts[] = {
        "loop(){let a0=x;let a1=y;let a2=x;let a3=y;unit;break y;}"};
    for (size_t t = 0; t < sizeof(texts) / sizeof(*texts); ++t) {
        Fixture f = {0};
        CHECK(setup(&f));
        if (t == 3) {
            NLSemanticBindingView x, y;
            NLSymbolId r;
            CHECK(body_binding(f.c, "x", &x) && body_binding(f.c, "y", &y));
            CHECK(test_reference(f.c, "a", x.place, NL_TYPE_REF, NL_ACCESS_READ,
                                 false, &r, NULL));
            CHECK(test_reference(f.c, "b", y.place, NL_TYPE_REF, NL_ACCESS_READ,
                                 false, &r, NULL));
        }
        if (t == 4 || t == 5) {
            const NLFunctionParameter params[] = {{"flag", f.boolean},
                                                  {"v", f.copy}};
            CHECK(register_body(
                f.c, t == 4 ? "return_path" : "mixed_zero", params, 2, f.copy,
                t == 4
                    ? "{loop(i=v){if(flag){return i;}else{continue(next(i));}}}"
                    : "{if(flag){loop(){continue();}}else{return v;}}",
                NL_CHECK_OK, NULL));
        }
        NLSource *source = NULL;
        NLSyntaxTree *tree = NULL;
        CHECK(body_tree(texts[t], &source, &tree));
        TestState before = {0};
        CHECK(test_state(f.c, &before));
        bool completed = false;
        for (size_t n = 0; n < 10000; ++n) {
            NLSemanticContext *candidate = NULL;
            CHECK(nl_sem_clone(f.c, &candidate) == NL_CHECK_OK);
            NLCheckedFragment *a = NULL;
            NLCheckDiagnostic d = {0};
            fail_at = n;
            allocation_index = 0;
            failed_kind = 0;
            failed_bytes = 0;
            failed_caller = NULL;
            injecting = true;
            NLCheckStatus status =
                nl_semantic_check_source_fragment(candidate, tree, &a, &d);
            injecting = false;
            if (status == NL_CHECK_OK) {
                CHECK(a && !failed_kind);
                const NLCheckedFragment *body =
                    nl_checked_loop_body(a, nl_checked_root(a));
                CHECK(body && body->count == 17);
                const NLControlExitView *edge =
                    nl_control_exit_view(body->exits, 0);
                CHECK(edge && edge->source_node == 17);
                CHECK(nl_checked_loop_break_provenance_valid(
                    a, nl_checked_root(a)));
                nl_checked_destroy(a);
                nl_semantic_destroy(candidate);
                printf("P1088 COMPLETE t=%zu reached=%zu\n", t, n);
                completed = true;
                break;
            }
            bool unchanged = test_unchanged(candidate, &before);
            printf(
                "P1088 OOM t=%zu n=%zu kind=%c bytes=%zu pc=%p calls=%zu "
                "status=%d artifact=%p diagnostic=%s no_clobber=%d source=%s\n",
                t, n, failed_kind ? failed_kind : '-', failed_bytes,
                failed_caller, allocation_index, status, (void *)a,
                d.diagnostic.code ? d.diagnostic.code : "NULL", unchanged,
                texts[t]);
            bool valid = failed_kind && status == NL_CHECK_OUT_OF_MEMORY &&
                         !a && unchanged && d.diagnostic.code;
            nl_checked_destroy(a);
            a = NULL;
            NLCheckDiagnostic retry_d = {0};
            NLCheckStatus retry = nl_semantic_check_source_fragment(
                candidate, tree, &a, &retry_d);
            bool retry_ok =
                retry == NL_CHECK_OK && a &&
                nl_checked_loop_break_provenance_valid(a, nl_checked_root(a));
            printf("P1088 RETRY t=%zu n=%zu status=%d artifact=%d\n", t, n,
                   retry, a != NULL);
            nl_checked_destroy(a);
            nl_semantic_destroy(candidate);
            if (!valid || !retry_ok) {
                nl_syntax_tree_destroy(tree);
                nl_source_destroy(source);
                nl_semantic_destroy(f.c);
                fprintf(stderr,
                        "P1088 FIRST RED t=%zu n=%zu unexpected "
                        "status/rollback/retry; cleanup completed\n",
                        t, n);
                return false;
            }
        }
        CHECK(completed);
        nl_syntax_tree_destroy(tree);
        nl_source_destroy(source);
        nl_semantic_destroy(f.c);
    }
    return true;
}
int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    return probe() ? 0 : 1;
}
