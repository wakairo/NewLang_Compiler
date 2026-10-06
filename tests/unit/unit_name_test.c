#include "../support/function_body.h"
#include <stdint.h>
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}
static bool rejected(NLSemanticContext *c, const char *text, TestEntry entry)
{
    CHECK(test_rejected(c, text, entry, NL_CHECK_SEMANTIC_ERROR,
                        "P10-RESERVED-NAME"));
    CHECK(nl_semantic_find_binding(c, "unit") == 0);
    return true;
}
static bool seed(TestSemantic *f, const char *name, NLTypeId type)
{
    NLSymbolId symbol;
    CHECK(nl_semantic_seed_value(f->context, name, type, NL_DEPENDENCY_FREE,
                                 &symbol) == NL_CHECK_OK);
    return true;
}
static bool admission(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f) && seed(&f, "x", f.copy));
    CHECK(rejected(f.context, "let unit=x", TEST_BINDING));
    CHECK(rejected(f.context, "let unit=x;", TEST_SOURCE));
    CHECK(rejected(f.context, "{{let unit=x;}}", TEST_SOURCE));
    CHECK(rejected(f.context, "let unit=unit;", TEST_SOURCE));
    /* Receiver admission precedes RHS lookup/consumption. */
    CHECK(rejected(f.context, "let unit=unknown;", TEST_SOURCE));
    CHECK(rejected(f.context, "loan read x as unit {}", TEST_LOAN));
    NLFunctionParameter p[] = {{"x", f.copy}};
    CHECK(register_body(f.context, "original_witness", p, 1, 1,
                        "{let unit=x;return unit;}", NL_CHECK_SEMANTIC_ERROR,
                        "P10-RESERVED-NAME"));
    NLFunctionParameter bad[] = {{"unit", f.copy}};
    CHECK(register_body(f.context, "bad_parameter", bad, 1, 1, "{return unit;}",
                        NL_CHECK_SEMANTIC_ERROR, "P10-RESERVED-NAME"));
    CHECK(register_body(f.context, "unit", NULL, 0, 1, "{return unit;}",
                        NL_CHECK_SEMANTIC_ERROR, "P10-RESERVED-NAME"));
    NLPlaceId root;
    NLValueId root_value;
    CHECK(nl_semantic_seed_root(f.context, f.linear, f.domain, true,
                                NL_DEPENDENCY_FREE, &root,
                                &root_value) == NL_CHECK_OK);
    NLSymbolId ptr, ending;
    CHECK(test_reference(f.context, "p", root, NL_TYPE_PTR, NL_ACCESS_READ,
                         false, &ptr, NULL));
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending, NULL));
    CHECK(rejected(f.context, "let(unit,rest)=take(p,ending)", TEST_SOURCE));
    CHECK(rejected(f.context, "let(first,unit)=take(p,ending)", TEST_SOURCE));
    CHECK(nl_semantic_find_binding(f.context, "first") == 0 &&
          nl_semantic_find_binding(f.context, "rest") == 0);
    NLSemanticPlaceView live;
    CHECK(nl_semantic_place_view(f.context, root, &live) && live.live);
    NLSemanticBindingView authority;
    CHECK(nl_semantic_binding_view(f.context, ending, &authority) &&
          authority.availability == NL_AVAILABLE);
    const NLAggregateField fields[] = {{"ok", f.copy}, {"unit", f.linear}};
    NLTypeId record;
    CHECK(nl_semantic_register_aggregate(f.context, "Record", fields, 2,
                                         &record) == NL_CHECK_OK);
    CHECK(seed(&f, "payload", f.linear));
    /* Field label is legal; shorthand receiving that field is an ordinary
     * local introduction, rejected without partially publishing 'ok'. */
    CHECK(body_ok(f.context, "let record=Record{ok:x,unit:payload}"));
    CHECK(rejected(f.context, "let Record{ok,unit}=record", TEST_SOURCE));
    CHECK(nl_semantic_find_binding(f.context, "ok") == 0);
    NLTypeId option;
    const NLSumVariant variants[] = {{"Some", f.linear}, {"None", 0}};
    CHECK(nl_semantic_register_sum(f.context, "Option", variants, 2, &option) ==
          NL_CHECK_OK);
    CHECK(seed(&f, "payload2", f.linear));
    CHECK(body_ok(f.context, "let option=Option.Some(payload2)"));
    CHECK(rejected(f.context, "match option {Some(unit)=>{unit},None=>{unit}}",
                   TEST_SOURCE));
    NLSemanticBindingView option_binding;
    CHECK(body_binding(f.context, "option", &option_binding));
    NLSymbolId read;
    CHECK(test_reference(f.context, "read", option_binding.place, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &read, NULL));
    CHECK(rejected(f.context, "match read {Some(unit)=>{unit},None=>{unit}}",
                   TEST_SOURCE));
    NLFunctionParameter op[] = {{"s", option}};
    CHECK(register_body(
        f.context, "bad_function_arm", op, 1, 1,
        "{match s {Some(unit)=>{return unit;},None=>{return unit;}};}",
        NL_CHECK_SEMANTIC_ERROR, "P10-RESERVED-NAME"));
    /* Programmatic ingress cannot create a lexical binding shadowing unit. */
    TestState before;
    CHECK(test_state(f.context, &before));
    NLSymbolId output = SIZE_MAX;
    NLDomainId domain = SIZE_MAX;
    CHECK(nl_semantic_seed_value(f.context, "unit", f.copy, NL_DEPENDENCY_FREE,
                                 &output) == NL_CHECK_SEMANTIC_ERROR &&
          output == SIZE_MAX);
    CHECK(test_unchanged(f.context, &before));
    CHECK(nl_semantic_seed_domain(f.context, "unit", &output, &domain) ==
              NL_CHECK_SEMANTIC_ERROR &&
          output == SIZE_MAX && domain == SIZE_MAX);
    CHECK(test_unchanged(f.context, &before));
    CHECK(nl_semantic_register_function(f.context, "unit", NULL, 0, 1, false,
                                        false) == NL_CHECK_SEMANTIC_ERROR);
    CHECK(test_unchanged(f.context, &before));
    TestChecked a = {0};
    CHECK(test_run(f.context, "x", TEST_SOURCE, NL_CHECK_OK, NULL, &a));
    CHECK(test_state(f.context, &before));
    CHECK(nl_semantic_bind_result(f.context, "unit",
                                  test_root(&a)->results[0].value,
                                  &output) == NL_CHECK_SEMANTIC_ERROR &&
          output == SIZE_MAX);
    CHECK(test_unchanged(f.context, &before));
    test_checked_destroy(&a);
    CHECK(body_ok(f.context, "{let Unit=x;let unit_=x;let units=x;unit;}"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool builtin(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    const char *forms[] = {"unit", "unit;", "{unit}", "{unit;}"};
    for (size_t i = 0; i < sizeof(forms) / sizeof(forms[0]); ++i) {
        TestChecked a = {0};
        CHECK(
            test_run(f.context, forms[i], TEST_SOURCE, NL_CHECK_OK, NULL, &a));
        CHECK(test_root(&a)->type == nl_semantic_unit_type(f.context) &&
              test_root(&a)->result_count == 0);
        test_checked_destroy(&a);
    }
    TestChecked a = {0};
    CHECK(test_run(f.context, "unit", TEST_EXPRESSION, NL_CHECK_OK, NULL, &a));
    CHECK(test_root(&a)->kind == NL_CHECKED_UNIT && test_root(&a)->symbol == 0);
    test_checked_destroy(&a);
    CHECK(test_run(f.context, "unit", TEST_TYPE, NL_CHECK_OK, NULL, &a));
    CHECK(test_root(&a)->type == nl_semantic_unit_type(f.context));
    test_checked_destroy(&a);
    CHECK(register_body(f.context, "return_unit", NULL, 0, 1, "{return unit;}",
                        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "return_unit()"));
    CHECK(body_ok(f.context, "let alias=unit"));
    NLSemanticBindingView alias;
    CHECK(body_binding(f.context, "alias", &alias) && alias.type == 1);
    NLTypeId unit = nl_semantic_unit_type(f.context);
    CHECK(nl_semantic_register_function(f.context, "observe_unit", &unit, 1,
                                        unit, false, false) == NL_CHECK_OK);
    CHECK(test_run(f.context, "observe_unit(unit)", TEST_SOURCE, NL_CHECK_OK,
                   NULL, &a));
    const NLCheckedNodeView *argument =
        nl_checked_node_view(a.artifact, test_root(&a)->first_argument);
    CHECK(argument->kind == NL_CHECKED_UNIT && argument->symbol == 0 &&
          argument->type == unit && argument->parameter_type == unit);
    test_checked_destroy(&a);
    CHECK(body_ok(f.context, "observe_unit({unit})"));
    CHECK(body_ok(f.context, "observe_unit(alias)"));
    /* Literal unit also bypasses lexical write-parameter inference. It still
     * supplies no reference authority to an invalid destination. */
    CHECK(test_rejected(f.context, "store(unit,unit)", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-SINGLE-RESULT-REQUIRED"));

    NLFunctionParameter up[] = {{"value", unit}};
    CHECK(register_body(f.context, "unit_body", up, 1, unit,
                        "{let local=value;return unit;}", NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "let result=unit_body(unit)"));
    CHECK(nl_semantic_find_binding(f.context, "unit") == 0);
    /* Member labels are still WORDs in their distinct namespaces. */
    NLTypeId flag;
    const NLSumVariant labels[] = {{"unit", 0}, {"Other", 0}};
    CHECK(nl_semantic_register_sum(f.context, "Flag", labels, 2, &flag) ==
          NL_CHECK_OK);
    CHECK(body_ok(f.context, "let flag=Flag.unit"));
    CHECK(body_ok(f.context, "match flag {unit=>{unit},Other=>{unit}}"));
    CHECK(seed(&f, "x", f.copy));
    NLTypeId record;
    const NLAggregateField fields[] = {{"unit", f.copy}};
    CHECK(nl_semantic_register_aggregate(f.context, "Record", fields, 1,
                                         &record) == NL_CHECK_OK);
    CHECK(body_ok(f.context, "let record=Record{unit:x}"));
    CHECK(nl_semantic_find_binding(f.context, "unit") == 0);
    nl_semantic_destroy(f.context);
    return true;
}
static bool check_faults(NLSemanticContext *c, const char *text,
                         NLCheckStatus expected)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(body_tree(text, &source, &tree));
    TestState before;
    CHECK(test_state(c, &before));
    bool complete = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        NLCheckDiagnostic d = {0};
        injecting = true;
        allocation_index = 0;
        NLCheckStatus status =
            nl_semantic_check_source_fragment(c, tree, &artifact, &d);
        injecting = false;
        if (status == expected) {
            if (expected == NL_CHECK_OK)
                nl_checked_destroy(artifact);
            else
                CHECK(artifact == NULL &&
                      strcmp(d.diagnostic.code, "P10-RESERVED-NAME") == 0 &&
                      test_unchanged(c, &before));
            complete = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(c, &before));
    }
    CHECK(complete && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
static bool failure(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f) && seed(&f, "x", f.copy));
    NLSemanticBindingView x;
    CHECK(body_binding(f.context, "x", &x));
    NLSymbolId r;
    CHECK(test_reference(f.context, "rw", x.place, NL_TYPE_REF, NL_ACCESS_WRITE,
                         false, &r, NULL));
    CHECK(rejected(f.context, "{store(rw,x);let unit=unit;}", TEST_SOURCE));
    CHECK(check_faults(f.context, "{store(rw,x);let unit=unit;}",
                       NL_CHECK_SEMANTIC_ERROR));
    CHECK(check_faults(f.context, "let(first,unit)=unknown()",
                       NL_CHECK_SEMANTIC_ERROR));
    NLTypeId record;
    const NLAggregateField fields[] = {{"ok", f.copy}, {"unit", f.copy}};
    CHECK(nl_semantic_register_aggregate(f.context, "Record", fields, 2,
                                         &record) == NL_CHECK_OK);
    CHECK(body_ok(f.context, "let record=Record{ok:x,unit:x}"));
    CHECK(check_faults(f.context, "let Record{ok,unit}=record",
                       NL_CHECK_SEMANTIC_ERROR));
    NLTypeId option;
    const NLSumVariant variants[] = {{"Some", f.copy}, {"None", 0}};
    CHECK(nl_semantic_register_sum(f.context, "Option", variants, 2, &option) ==
          NL_CHECK_OK);
    CHECK(body_ok(f.context, "let option=Option.Some(x)"));
    CHECK(check_faults(f.context,
                       "match option {Some(unit)=>{unit},None=>{unit}}",
                       NL_CHECK_SEMANTIC_ERROR));
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(body_tree("{let alias=value;return unit;}", &source, &tree));
    NLFunctionParameter p[] = {{"value", 1}};
    TestState before;
    CHECK(test_state(f.context, &before));
    bool complete = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status = nl_semantic_register_function_body(
            f.context, "unit_body", p, 1, 1, tree, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            complete = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
              test_unchanged(f.context, &before));
    }
    CHECK(complete && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    CHECK(check_faults(f.context, "let output=unit_body(unit)", NL_CHECK_OK));
    nl_semantic_destroy(f.context);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "admission") == 0)
        return admission() ? 0 : 1;
    if (strcmp(argv[1], "builtin") == 0)
        return builtin() ? 0 : 1;
    if (strcmp(argv[1], "failure") == 0)
        return failure() ? 0 : 1;
    return 2;
}
