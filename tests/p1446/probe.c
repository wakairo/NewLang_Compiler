/* Use the original P-owned loader and rollback apparatus unchanged. */
#define main p1435_original_main
#include "../p1435/probe.c"
#undef main

static bool observe_exit;
static size_t exit_calls, exit_failures, rejected_scope_floor, escaped_scope;
static NLCheckStatus first_exit_status;

NLCheckStatus __real_nl_sem_function_exit(const NLSemanticContext *, size_t,
                                          size_t);
NLCheckStatus __wrap_nl_sem_function_exit(const NLSemanticContext *c,
                                          size_t scopes, size_t places)
{
    const NLCheckStatus status = __real_nl_sem_function_exit(c, scopes, places);
    if (observe_exit) {
        ++exit_calls;
        if (status != NL_CHECK_OK) {
            if (!exit_failures) {
                first_exit_status = status;
                rejected_scope_floor = scopes;
            }
            ++exit_failures;
            printf("real exit status=%d scope_floor=%zu place_floor=%zu "
                   "scope_count=%zu\n",
                   (int)status, scopes, places, c->scope_count);
            /* Observe authentic in-call facts only; the real predicate above
             * owns the result and receives no substituted state/status. */
            for (size_t i = 0; i < c->value_count; ++i) {
                const NLSemanticValueView v = c->values[i];
                if (v.carrier == NL_CARRIER_ENDED ||
                    c->types[v.type - 1].view.kind != NL_TYPE_REF)
                    continue;
                for (size_t k = 0; k < nl_sem_ref_count(v); ++k) {
                    const NLReferenceFacts f = nl_sem_ref_fact(v, k);
                    printf("surviving ref value=%zu place=%zu scope=%zu "
                           "dependencies=%d\n",
                           i + 1, f.place, f.scope, (int)v.dependencies);
                    if (f.scope > scopes && !escaped_scope)
                        escaped_scope = f.scope;
                }
            }
        }
    }
    return status;
}

static bool diagnostic(const char *bad, const char *control,
                       const char *expected_code)
{
    Program p = {0}, valid = {0};
    CHECK(load(bad, &p) && load(control, &valid));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    NLFunctionUnitDiagnostic d = {0};
    observe_exit = true;
    NLCheckStatus status = define(&p, &d);
    observe_exit = false;
    printf("registration status=%d severity=%d category=%s code=%s "
           "span=%zu:%zu input=%zu\n",
           (int)status, (int)d.diagnostic.diagnostic.severity,
           d.diagnostic.diagnostic.category, d.diagnostic.diagnostic.code,
           d.diagnostic.span.start_byte, d.diagnostic.span.end_byte,
           d.input_index);
    CHECK(status == NL_CHECK_SEMANTIC_ERROR && !p.artifact && exit_calls &&
          exit_failures == 1 && first_exit_status == NL_CHECK_SEMANTIC_ERROR &&
          rejected_scope_floor == 4 && escaped_scope == 5 &&
          d.input_index == 0 &&
          d.diagnostic.diagnostic.severity == NL_DIAG_ERROR &&
          !strcmp(d.diagnostic.diagnostic.category, "semantic") &&
          !strcmp(d.diagnostic.diagnostic.code, expected_code) &&
          d.diagnostic.span.start_byte == 3581 &&
          d.diagnostic.span.end_byte == 3790 &&
          nl_copy_state_matches(p.context, &anchor) &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof(before)));
    CHECK(nl_check_diagnostic_render(stdout, p.source, &d.diagnostic));
    NLCheckedFragment *refused = NULL;
    NLCheckDiagnostic missing = {0};
    status =
        nl_semantic_check_expression(p.context, p.call, &refused, &missing);
    printf("after refusal actual main status=%d artifact=%s\n", (int)status,
           refused ? "present" : "NULL");
    CHECK(status == NL_CHECK_SEMANTIC_ERROR && !refused &&
          nl_copy_state_matches(p.context, &anchor));
    const NLSyntaxTree *units[] = {valid.unit};
    CHECK(nl_semantic_register_function_unit(p.context, units, 1, &d) ==
          NL_CHECK_OK);
    CHECK(actual(&p) && output(&p, NL_NODE_C_OK));
    printf("same-context rollback and valid retry MACHINE_PROVED PASS\n");
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    destroy(&valid);
    return true;
}

static bool internal_argument(const char *control)
{
    Program p = {0};
    CHECK(load(control, &p));
    NLCheckedFragment *out = NULL;
    NLCheckDiagnostic d = {
        .diagnostic = {.code = "unchanged-internal-argument-sentinel"},
        .span = {1, 2}};
    NLCheckDiagnostic before;
    memcpy(&before, &d, sizeof(d));
    const NLCheckStatus status =
        nl_semantic_check_expression(NULL, p.call, &out, &d);
    printf("real invalid API argument status=%d artifact=%s\n", (int)status,
           out ? "present" : "NULL");
    CHECK(status == NL_CHECK_INTERNAL_ERROR && !out &&
          !memcmp(&before, &d, sizeof(d)));
    destroy(&p);
    return true;
}

int main(int argc, char **argv)
{
    if (argc == 5 && !strcmp(argv[1], "diagnostic"))
        return diagnostic(argv[2], argv[3], argv[4]) ? 0 : 1;
    if (argc == 3 && !strcmp(argv[1], "internal"))
        return internal_argument(argv[2]) ? 0 : 1;
    return 2;
}
