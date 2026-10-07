#include "../support/semantic_check.h"

/* Non-terminating W1 equivalent: both normal paths transfer affine payloads
 * into registered handlers. W2 then pressures a known None and known Some. */
static bool workload(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLTypeId result, option;
    const NLSumVariant results[] = {{"Success", f.linear},
                                    {"Failure", f.discardable}};
    const NLSumVariant options[] = {{"Some", f.copy}, {"None", 0}};
    CHECK(nl_semantic_register_sum(f.context, "Result", results, 2, &result) ==
          NL_CHECK_OK);
    CHECK(nl_semantic_register_sum(f.context, "Option", options, 2, &option) ==
          NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "success", &f.linear, 1,
                                        f.copy, false, false) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "failure", &f.discardable, 1,
                                        f.copy, false, false) == NL_CHECK_OK);
    NLSymbolId error, next;
    CHECK(nl_semantic_seed_value(f.context, "error", f.discardable,
                                 NL_DEPENDENCY_FREE, &error) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f.context, "next", f.copy, NL_DEPENDENCY_FREE,
                                 &next) == NL_CHECK_OK);
    TestChecked a = {0};
    CHECK(test_run(f.context,
                   "{let result=Result::Failure(error);match result "
                   "{Success(h)=>{success(h)},Failure(e)=>{failure(e)},}}",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &a));
    CHECK(test_root(&a)->result_count == 1 && test_root(&a)->type == f.copy);
    test_checked_destroy(&a);
    CHECK(nl_semantic_find_binding(f.context, "result") == 0 &&
          nl_semantic_find_binding(f.context, "h") == 0);
    CHECK(test_run(f.context, "let state=Option::None", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &a));
    NLSemanticBindingView binding;
    CHECK(nl_semantic_binding_view(f.context, test_root(&a)->symbol, &binding));
    test_checked_destroy(&a);
    NLSymbolId rw;
    CHECK(test_reference(f.context, "rw", binding.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &rw, NULL));
    CHECK(
        test_run(f.context,
                 "match rw {Some(payload)=>{replace(payload,next);},None=>{}}",
                 TEST_SOURCE, NL_CHECK_OK, NULL, &a));
    NLSemanticPlaceView p;
    CHECK(nl_semantic_place_view(f.context, binding.place, &p) &&
          p.payload_occurrence == 0);
    /* Hypothetical Some arm evidence must not mint a public occurrence. */
    const NLCheckedFragment *arm =
        nl_checked_match_arm(a.artifact, nl_checked_root(a.artifact), 0);
    const NLCheckedNodeView *pattern =
        nl_checked_node_view(arm, nl_checked_root(arm));
    NLSemanticBindingView payload;
    NLSemanticValueView ref;
    CHECK(nl_semantic_binding_view(nl_checked_context(arm), pattern->symbol,
                                   &payload));
    CHECK(
        nl_semantic_value_view(nl_checked_context(arm), payload.value, &ref) &&
        ref.reference.occurrence_dependency != 0);
    test_checked_destroy(&a);
    CHECK(test_run(f.context, "store(rw,Option::Some(next))", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &a));
    test_checked_destroy(&a);
    CHECK(nl_semantic_place_view(f.context, binding.place, &p) &&
          p.payload_occurrence != 0);
    NLOccurrenceId before = p.payload_occurrence;
    CHECK(test_run(f.context,
                   "match rw {Some(payload)=>{store(payload,next);},None=>{}}",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &a));
    CHECK(nl_semantic_place_view(f.context, binding.place, &p) &&
          p.payload_occurrence == before);
    test_checked_destroy(&a);
    nl_semantic_destroy(f.context);
    return true;
}
int main(void)
{
    return workload() ? 0 : 1;
}
