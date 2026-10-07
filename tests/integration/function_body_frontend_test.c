#include "../support/function_body.h"
#include "../support/ref_join.h"

static bool identity(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLFunctionParameter p[] = {{"x", f.linear}};
    CHECK(register_body(f.context, "identity", p, 1, f.linear, "{x}",
                        NL_CHECK_OK, NULL));
    NLSymbolId input;
    CHECK(nl_semantic_seed_value(f.context, "x", f.linear, NL_DEPENDENCY_FREE,
                                 &input) == NL_CHECK_OK);
    NLSemanticBindingView before, after;
    CHECK(body_binding(f.context, "x", &before));
    NLSemanticPlaceView old;
    CHECK(nl_semantic_place_view(f.context, before.place, &old));
    TestChecked a = {0};
    CHECK(test_run(f.context, "let y=identity(x)", TEST_SOURCE, NL_CHECK_OK,
                   NULL, &a));
    CHECK(body_binding(f.context, "y", &after) && after.value == before.value &&
          after.place != before.place);
    NLSemanticPlaceView destination, ended;
    CHECK(nl_semantic_place_view(f.context, after.place, &destination) &&
          nl_semantic_place_view(f.context, before.place, &ended) &&
          !ended.live && destination.incarnation != old.incarnation);
    NLSemanticBindingView consumed;
    CHECK(nl_semantic_binding_view(f.context, input, &consumed) &&
          consumed.availability == NL_CONSUMED);
    const NLCheckedNodeView *call =
        nl_checked_node_view(a.artifact, test_root(&a)->initializer);
    CHECK(call->body_backed && call->results[0].value == before.value);
    const NLCheckedFragment *body =
        nl_checked_call_body(a.artifact, test_root(&a)->initializer);
    CHECK(body != NULL && nl_checked_source(body) != a.source &&
          nl_checked_context(body) == f.context);
    const NLCheckedNodeView *tail = nl_checked_node_view(
        body, nl_checked_node_view(body, nl_checked_root(body))->tail);
    CHECK(tail->symbol != input && tail->value_use == NL_VALUE_CONSUMED);
    CHECK(nl_semantic_binding_view(f.context, tail->symbol, &consumed) &&
          consumed.availability == NL_CONSUMED);
    CHECK(nl_semantic_find_binding(f.context, "x") == input &&
          nl_semantic_find_binding(f.context, "local") == 0);
    test_checked_destroy(&a);
    CHECK(test_rejected(f.context, "identity(x)", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-USE-AFTER-CONSUME"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool swap_and_order(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x, y, ra, rb;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f.context, "y", f.copy, NL_DEPENDENCY_FREE,
                                 &y) == NL_CHECK_OK);
    NLSemanticBindingView bx, by;
    CHECK(body_binding(f.context, "x", &bx) &&
          body_binding(f.context, "y", &by));
    CHECK(test_reference(f.context, "ra", bx.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &ra, NULL));
    CHECK(test_reference(f.context, "rb", by.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &rb, NULL));
    NLSemanticBindingView ref;
    CHECK(nl_semantic_binding_view(f.context, ra, &ref));
    NLFunctionParameter params[] = {{"a", ref.type}, {"b", ref.type}};
    CHECK(register_body(f.context, "exchange", params, 2, 1, "{swap(a,b);}",
                        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "exchange(ra,rb)"));
    NLSemanticPlaceView ax, ay;
    CHECK(nl_semantic_place_view(f.context, bx.place, &ax) &&
          ax.current_value == by.value &&
          nl_semantic_place_view(f.context, by.place, &ay) &&
          ay.current_value == bx.value);
    CHECK(body_ok(f.context, "exchange(ra,ra)"));
    NLSemanticPlaceView noop;
    CHECK(nl_semantic_place_view(f.context, bx.place, &noop) &&
          test_place_equal(noop, ax));
    CHECK(body_ok(f.context, "swap(ra,rb)"));
    CHECK(nl_semantic_place_view(f.context, bx.place, &ax) &&
          ax.current_value == bx.value &&
          nl_semantic_place_view(f.context, by.place, &ay) &&
          ay.current_value == by.value);
    CHECK(register_body(f.context, "ordered", params, 2, 1,
                        "{swap(a,b);swap(a,a);swap(a,b);}", NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "ordered(ra,rb)"));
    CHECK(nl_semantic_place_view(f.context, bx.place, &noop) &&
          noop.current_value == bx.value && noop.incarnation == ax.incarnation);
    CHECK(nl_semantic_find_binding(f.context, "a") == 0 &&
          nl_semantic_find_binding(f.context, "b") == 0);
    /* Actual argument evaluation order: first read ref ra, then replace ra's
     * value and pass the returned old package. The body performs the next
     * write. */
    NLFunctionParameter replace_params[] = {{"dst", ref.type}, {"v", f.copy}};
    CHECK(register_body(f.context, "install", replace_params, 2, f.copy,
                        "{replace(dst,v)}", NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "let old=install(ra,replace(ra,y))"));
    NLSemanticBindingView returned;
    CHECK(body_binding(f.context, "old", &returned));
    NLSemanticValueView returned_value;
    CHECK(nl_semantic_value_view(f.context, returned.value, &returned_value));
    /* Copy argument y creates a distinct package which the body returns from
     * the second replace; it is not a newly minted result after the body. */
    CHECK(returned.value != bx.value && returned_value.type == f.copy);
    CHECK(nl_semantic_place_view(f.context, bx.place, &noop) &&
          noop.incarnation == ax.incarnation &&
          noop.current_value != returned.value);
    /* Caller-visible mutation failure after the first swap must roll back. */
    CHECK(nl_semantic_register_function(f.context, "observe", &ref.type, 1, 1,
                                        false, false) == NL_CHECK_OK);
    CHECK(register_body(f.context, "partial", params, 2, 1,
                        "{swap(a,b);observe(b);}", NL_CHECK_OK, NULL));
    /* A stale actual cannot pass evaluation; a joined write ref reaches the
     * actual body and rejects rather than choosing one possible destination. */
    NLValueId inputs[] = {ref.value};
    CHECK(nl_semantic_binding_view(f.context, rb, &ref));
    inputs[0] = ref.value;
    NLValueId joined;
    CHECK(nl_semantic_join_references(f.context, inputs, 1, &joined) ==
          NL_CHECK_OK);
    NLSymbolId joined_symbol;
    CHECK(nl_semantic_bind_result(f.context, "joined_write", joined,
                                  &joined_symbol) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "exchange(joined_write,ra)", TEST_SOURCE,
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P7-SINGULAR-REF-PRECISION"));
    CHECK(test_rejected(f.context, "partial(ra,joined_write)", TEST_SOURCE,
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P7-SINGULAR-REF-PRECISION"));
    NLFunctionParameter late_params[] = {
        {"a", ref.type}, {"b", ref.type}, {"v", f.copy}};
    CHECK(register_body(f.context, "late_failure", late_params, 3, 1,
                        "{store(a,v);swap(a,b);}", NL_CHECK_OK, NULL));
    CHECK(test_rejected(f.context, "late_failure(ra,joined_write,y)",
                        TEST_SOURCE, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P7-SINGULAR-REF-PRECISION"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool readonly_join(void)
{
    JoinFixture f = {0};
    CHECK(join_create(&f, true));
    NLFunctionParameter p[] = {{"ref", f.read_type}};
    CHECK(register_body(f.sem.context, "inspect", p, 1, 1,
                        "{let copy=ref;observe(copy);}", NL_CHECK_OK, NULL));
    CHECK(join_ok(&f, "let chosen=match r {Some(v)=>{v},None=>{fallback}}"));
    CHECK(body_ok(f.sem.context, "inspect(chosen)"));
    CHECK(join_reject(&f, "store(rw,Option::None)", NL_CHECK_SEMANTIC_ERROR,
                      "P6-OCCURRENCE-CONFLICT"));
    CHECK(nl_semantic_end_scope(f.sem.context, f.a_scope) == NL_CHECK_OK);
    CHECK(join_reject(&f, "inspect(chosen)", NL_CHECK_SEMANTIC_ERROR,
                      "P3-DEAD-SCOPE"));
    nl_semantic_destroy(f.sem.context);
    return true;
}
static bool capability_result(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId root, ref;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &root) == NL_CHECK_OK);
    NLSemanticBindingView binding;
    CHECK(nl_semantic_binding_view(f.context, root, &binding));
    CHECK(test_reference(f.context, "rw", binding.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &ref, NULL));
    NLTypeId read, pointer;
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_REF, f.copy,
                                    NL_ACCESS_READ, false,
                                    &read) == NL_CHECK_OK);
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_PTR, f.copy,
                                    NL_ACCESS_READ, false,
                                    &pointer) == NL_CHECK_OK);
    NLFunctionParameter p[] = {{"r", read}};
    CHECK(register_body(f.context, "address", p, 1, pointer,
                        "{ptr_from_ref(r)}", NL_CHECK_OK, NULL));
    CHECK(register_body(f.context, "return_ref", p, 1, read, "{r}",
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P8-SIGNATURE-PRECISION"));
    CHECK(test_rejected(f.context, "address(x)", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
    TestChecked call = {0};
    CHECK(test_run(f.context, "address(rw)", TEST_SOURCE, NL_CHECK_OK, NULL,
                   &call));
    const NLCheckedFragment *body =
        nl_checked_call_body(call.artifact, nl_checked_root(call.artifact));
    const NLCheckedNodeView *tail = nl_checked_node_view(
        body, nl_checked_node_view(body, nl_checked_root(body))->tail);
    CHECK(test_root(&call)->results[0].value == tail->results[0].value);
    NLSemanticValueView result, actual;
    CHECK(nl_semantic_value_view(f.context, tail->results[0].value, &result));
    CHECK(nl_semantic_binding_view(f.context, ref, &binding) &&
          nl_semantic_value_view(f.context, binding.value, &actual));
    CHECK(result.type == pointer &&
          result.reference.place == actual.reference.place &&
          result.reference.incarnation == actual.reference.incarnation &&
          result.reference.provenance == actual.reference.provenance &&
          result.reference.writable == actual.reference.writable);
    test_checked_destroy(&call);
    nl_semantic_destroy(f.context);
    return true;
}
int main(void)
{
    return identity() && swap_and_order() && readonly_join() &&
                   capability_result()
               ? 0
               : 1;
}
