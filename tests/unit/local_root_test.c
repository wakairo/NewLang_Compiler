#include "../support/semantic_check.h"
#include <stdlib.h>

static const char witness[] =
    "fn main()->unit{let x=u8(7);"
    "let p=loan_read(x){|r|ptr_from_ref(r)};"
    "loan_read_ptr(p){|r2|ptr_from_ref(r2);unit};unit}";
static const char mutation_witness[] =
    "fn main()->unit{let x=u8(7);"
    "let p=loan_read(x){|r|ptr_from_ref(r)};"
    "let old=loan_write(x){|w|replace(w,u8(9))};old;"
    "loan_read_ptr(p){|r2|ptr_from_ref(r2);unit};unit}";

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
static bool tree(const char *text, bool unit, NLSource **source,
                 NLSyntaxTree **syntax)
{
    CHECK(nl_source_create(text, strlen(text), "local-root", source) ==
          NL_SOURCE_OK);
    NLParser *p = NULL;
    CHECK(nl_parser_create(*source, &p) == NL_PARSE_OK);
    const NLParseStatus status =
        unit ? nl_parser_parse_function_unit(p, syntax, NULL)
             : nl_parser_parse_source_fragment(p, syntax, NULL);
    nl_parser_destroy(p);
    CHECK(status == NL_PARSE_OK);
    return true;
}
static bool parser(void)
{
    NLSource *source = NULL;
    NLSyntaxTree *syntax = NULL;
    CHECK(tree(witness, true, &source, &syntax));
    const NLSyntaxView *f =
        nl_syntax_node_view(nl_syntax_node_view(nl_syntax_tree_root(syntax))
                                ->data.function_unit.declarations);
    const NLSyntaxView *block = nl_syntax_node_view(f->data.function.body);
    const NLSyntaxView *p =
        nl_syntax_node_view(nl_syntax_next_argument(block->data.block.items));
    const NLSyntaxView *loan = nl_syntax_node_view(p->data.binding.initializer);
    CHECK(loan->kind == NL_SYNTAX_LOCAL_READ_LOAN && !loan->data.loan.from_ptr);
    const NLSyntaxView *body = nl_syntax_node_view(loan->data.loan.body);
    CHECK(body->kind == NL_SYNTAX_BLOCK && body->data.block.tail != NULL);
    const NLSyntaxView *second = nl_syntax_node_view(nl_syntax_next_argument(
        nl_syntax_next_argument(block->data.block.items)));
    loan = nl_syntax_node_view(second->data.statement.expression);
    CHECK(loan->kind == NL_SYNTAX_LOCAL_READ_LOAN && loan->data.loan.from_ptr);
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source);

    source = NULL;
    syntax = NULL;
    CHECK(tree(mutation_witness, true, &source, &syntax));
    f = nl_syntax_node_view(
        nl_syntax_node_view(nl_syntax_tree_root(syntax))
            ->data.function_unit.declarations);
    block = nl_syntax_node_view(f->data.function.body);
    const NLSyntaxNode *third = block->data.block.items;
    third = nl_syntax_next_argument(third);
    third = nl_syntax_next_argument(third);
    const NLSyntaxView *old_binding = nl_syntax_node_view(third);
    loan = nl_syntax_node_view(old_binding->data.binding.initializer);
    CHECK(loan->kind == NL_SYNTAX_LOCAL_WRITE_LOAN &&
          loan->data.loan.access == NL_ACCESS_WRITE &&
          !loan->data.loan.from_ptr);
    body = nl_syntax_node_view(loan->data.loan.body);
    CHECK(body->kind == NL_SYNTAX_BLOCK && body->data.block.tail != NULL);
    CHECK(nl_syntax_node_view(body->data.block.tail)->kind ==
          NL_SYNTAX_EXPR_CALL);
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source);

    const char *outside[] = {"loan_read(x.field){|r|unit}",
                             "loan_read_ptr(make()){|r|unit}", "loan_read()",
                             "loan_read(x)", "loan_write(x.field){|w|unit}",
                             "loan_write(make()){|w|unit}", "loan_write()",
                             "loan_write(x)"};
    for (size_t i = 0; i < sizeof(outside) / sizeof(outside[0]); ++i) {
        source = NULL;
        syntax = NULL;
        CHECK(nl_source_create(outside[i], strlen(outside[i]), "outside",
                               &source) == NL_SOURCE_OK);
        NLParser *p0 = NULL;
        CHECK(nl_parser_create(source, &p0) == NL_PARSE_OK);
        NLParseDiagnostic d = {0};
        CHECK(nl_parser_parse_source_fragment(p0, &syntax, &d) ==
              NL_PARSE_SYNTAX_UNSUPPORTED);
        CHECK(syntax == NULL &&
              strcmp(d.diagnostic.code, "LOCAL-LOAN-PROFILE") == 0);
        nl_parser_destroy(p0);
        nl_source_destroy(source);
    }
    return true;
}
static bool evidence(void)
{
    NLSource *source = NULL;
    NLSyntaxTree *syntax = NULL;
    NLSemanticContext *c = NULL;
    CHECK(tree(witness, true, &source, &syntax));
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {syntax};
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source); /* durable plan, no surviving source seed */
    TestChecked call = {0};
    CHECK(test_run(c, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL, &call));
    const NLCheckedFragment *b =
        nl_checked_call_body(call.artifact, nl_checked_root(call.artifact));
    CHECK(b != NULL);
    const NLCheckedNodeView *block =
        nl_checked_node_view(b, nl_checked_root(b));
    const NLCheckedNodeView *x = nl_checked_node_view(b, block->first_item);
    const NLCheckedNodeView *p = nl_checked_node_view(b, x->next_item);
    const NLCheckedNodeView *first = nl_checked_node_view(b, p->initializer);
    const NLCheckedNodeView *second_statement =
        nl_checked_node_view(b, p->next_item);
    const NLCheckedNodeView *second =
        nl_checked_node_view(b, second_statement->initializer);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    NLSemanticBindingView xb, pb;
    NLSemanticPlaceView root;
    CHECK(nl_semantic_binding_view(c, x->symbol, &xb));
    CHECK(nl_semantic_place_view(c, xb.place, &root));
    const NLCheckedNodeView *literal = nl_checked_node_view(b, x->initializer);
    CHECK(literal->scalar_result.type == u8 &&
          literal->scalar_result.value == 7);
    CHECK(root.type == u8 && root.implicit_local && root.independent_root &&
          root.governing_domain == 0);
    CHECK(first->loan.place == xb.place &&
          first->loan.incarnation == root.incarnation);
    CHECK(first->loan.source == x->symbol && first->loan.prevent_lifetime_end);
    CHECK(first->loan.body_nonescape_proved &&
          first->loan.normal_result_forwarded && !first->loan.from_ptr);
    const NLCheckedNodeView *body = nl_checked_node_view(b, first->initializer);
    const NLCheckedNodeView *ptr = nl_checked_node_view(b, body->tail);
    CHECK(ptr->kind == NL_CHECKED_PTR_FROM_REF && ptr->has_reference_result);
    CHECK(ptr->reference_result.place == first->loan.place &&
          ptr->reference_result.incarnation == first->loan.incarnation &&
          ptr->reference_result.scope == 0);
    CHECK(first->results[0].value == body->results[0].value &&
          first->results[0].value == ptr->results[0].value);
    CHECK(nl_semantic_binding_view(c, p->symbol, &pb));
    CHECK(pb.value ==
          ptr->results[0].value); /* unchanged R received after exit */
    NLSemanticValueView pv;
    CHECK(nl_semantic_value_view(c, pb.value, &pv) &&
          pv.dependencies == NL_DEPENDENCY_FREE);
    CHECK(second->loan.from_ptr && second->loan.source == p->symbol &&
          second->loan.place == first->loan.place &&
          second->loan.incarnation == first->loan.incarnation);
    CHECK(second->loan.scope != first->loan.scope &&
          second->loan.ref_symbol != first->loan.ref_symbol);
    NLSemanticScopeView a, z;
    CHECK(nl_semantic_scope_view(c, first->loan.scope, &a) && !a.active);
    CHECK(nl_semantic_scope_view(c, second->loan.scope, &z) && !z.active);
    body = nl_checked_node_view(b, second->initializer);
    const NLCheckedNodeView *stmt = nl_checked_node_view(b, body->first_item);
    ptr = nl_checked_node_view(b, stmt->initializer);
    CHECK(ptr->kind == NL_CHECKED_PTR_FROM_REF && ptr->has_reference_result &&
          ptr->reference_result.place == first->loan.place);
    const NLCheckedNodeView *ref_use =
        nl_checked_node_view(b, ptr->first_argument);
    CHECK(ref_use->symbol == second->loan.ref_symbol &&
          ref_use->value_use == NL_VALUE_COPIED);
    CHECK(second->type == nl_semantic_unit_type(c) &&
          second->result_count == 0);
    NLSemanticSnapshot counts;
    CHECK(nl_semantic_snapshot(c, &counts) && counts.domains == 0 &&
          counts.backing_regions == 0);
    test_checked_destroy(&call);
    nl_semantic_destroy(c);
    return true;
}
static bool mutation(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);

    TestChecked x_checked = {0};
    CHECK(test_run(c, "let x=u8(7);", TEST_SOURCE, NL_CHECK_OK, NULL,
                   &x_checked));
    const NLSymbolId x = test_root(&x_checked)->symbol;
    NLSemanticBindingView xb;
    NLSemanticPlaceView before;
    NLSemanticValueView initial;
    CHECK(nl_semantic_binding_view(c, x, &xb));
    CHECK(nl_semantic_place_view(c, xb.place, &before) && before.live &&
          before.implicit_local && before.independent_root &&
          before.governing_domain == 0);
    CHECK(nl_semantic_value_view(c, before.current_value, &initial) &&
          initial.scalar_known && initial.scalar_value == 7);

    TestChecked p_checked = {0};
    CHECK(test_run(c, "let p=loan_read(x){|r|ptr_from_ref(r)};", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &p_checked));
    const NLSymbolId p = test_root(&p_checked)->symbol;
    NLSemanticBindingView pb;
    NLSemanticValueView pv;
    CHECK(nl_semantic_binding_view(c, p, &pb));
    CHECK(nl_semantic_value_view(c, pb.value, &pv));
    CHECK(pv.reference.place == xb.place &&
          pv.reference.incarnation == before.incarnation &&
          pv.dependencies == NL_DEPENDENCY_FREE);

    TestChecked old_checked = {0};
    CHECK(test_run(c, "let old=loan_write(x){|w|replace(w,u8(9))};",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &old_checked));
    const NLCheckedNodeView *old_binding = test_root(&old_checked);
    const NLCheckedNodeView *write_loan =
        nl_checked_node_view(old_checked.artifact, old_binding->initializer);
    CHECK(write_loan != NULL && write_loan->kind == NL_CHECKED_LOAN_HEADER &&
          write_loan->loan.access == NL_ACCESS_WRITE &&
          !write_loan->loan.is_exclusive && !write_loan->loan.from_ptr &&
          write_loan->loan.implicit_local &&
          write_loan->loan.body_nonescape_proved &&
          write_loan->loan.normal_result_forwarded &&
          write_loan->loan.place == xb.place &&
          write_loan->loan.incarnation == before.incarnation);
    NLSemanticBindingView wb;
    NLSemanticTypeView wt;
    NLSemanticValueView wv;
    CHECK(nl_semantic_binding_view(c, write_loan->loan.ref_symbol, &wb));
    CHECK(nl_semantic_type_view(c, wb.type, &wt) &&
          wt.kind == NL_TYPE_REF && wt.access == NL_ACCESS_WRITE &&
          !wt.is_exclusive);
    CHECK(nl_semantic_value_view(c, wb.value, &wv) &&
          wv.reference.place == xb.place &&
          wv.reference.incarnation == before.incarnation &&
          wv.reference.scope == write_loan->loan.scope &&
          wv.reference.readable && wv.reference.writable);

    const NLCheckedNodeView *write_body =
        nl_checked_node_view(old_checked.artifact, write_loan->initializer);
    const NLCheckedNodeView *replace =
        nl_checked_node_view(old_checked.artifact, write_body->tail);
    CHECK(replace != NULL && replace->kind == NL_CHECKED_REPLACE &&
          replace->result_count == 1 &&
          replace->results[0].value == write_loan->results[0].value);
    NLSemanticValueView old_value;
    CHECK(nl_semantic_value_view(c, replace->results[0].value, &old_value) &&
          old_value.scalar_known && old_value.scalar_value == 7);

    NLSemanticBindingView after_x;
    NLSemanticPlaceView after;
    NLSemanticValueView current;
    CHECK(nl_semantic_binding_view(c, x, &after_x));
    CHECK(nl_semantic_place_view(c, after_x.place, &after) && after.live);
    CHECK(after_x.place == xb.place && after.incarnation == before.incarnation &&
          after.governing_domain == before.governing_domain &&
          after.current_fact != before.current_fact);
    CHECK(nl_semantic_value_view(c, after.current_value, &current) &&
          current.scalar_known && current.scalar_value == 9);
    CHECK(nl_semantic_binding_view(c, p, &pb));
    CHECK(nl_semantic_value_view(c, pb.value, &pv) &&
          pv.reference.place == after_x.place &&
          pv.reference.incarnation == after.incarnation);

    NLSemanticBindingView oldb;
    NLSemanticValueView old_bound;
    CHECK(nl_semantic_binding_view(c, old_binding->symbol, &oldb));
    CHECK(nl_semantic_value_view(c, oldb.value, &old_bound) &&
          old_bound.scalar_known && old_bound.scalar_value == 7);

    TestChecked reacquire = {0};
    CHECK(test_run(c,
                   "loan_read_ptr(p){|r2|ptr_from_ref(r2);unit}",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &reacquire));
    const NLCheckedNodeView *second = test_root(&reacquire);
    CHECK(second->kind == NL_CHECKED_LOAN_HEADER && second->loan.from_ptr &&
          second->loan.access == NL_ACCESS_READ &&
          second->loan.place == after_x.place &&
          second->loan.incarnation == after.incarnation);

    test_checked_destroy(&reacquire);
    test_checked_destroy(&old_checked);
    test_checked_destroy(&p_checked);
    test_checked_destroy(&x_checked);
    nl_semantic_destroy(c);
    return true;
}

static bool negatives(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(test_rejected(c, "{let x=u8(7);let bad=loan_read(x){|r|r};unit}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P8-EXIT-DEPENDENCY"));
    CHECK(test_rejected(
        c, "{let x=u8(7);let bad=loan_write(x){|w|w};unit}", TEST_SOURCE,
        NL_CHECK_SEMANTIC_ERROR, "P8-EXIT-DEPENDENCY"));
    CHECK(test_rejected(c,
                        "{let p={let "
                        "x=u8(7);loan_read(x){|r|ptr_from_ref(r)}};loan_read_"
                        "ptr(p){|r2|unit};unit}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-STALE-POINTER"));
    /* Isolate the allowed persistent token from forbidden later access. */
    TestChecked token = {0};
    CHECK(test_run(c, "{let x=u8(7);loan_read(x){|r|ptr_from_ref(r)}}",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &token));
    NLSemanticValueView ptr;
    CHECK(nl_semantic_value_view(c, test_root(&token)->results[0].value, &ptr));
    NLSemanticPlaceView old;
    CHECK(nl_semantic_place_view(c, ptr.reference.place, &old) && !old.live);
    NLSymbolId symbol = 0;
    CHECK(nl_semantic_bind_result(c, "p", test_root(&token)->results[0].value,
                                  &symbol) == NL_CHECK_OK);
    CHECK(test_rejected(c, "loan_read_ptr(p){|r2|unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
    test_checked_destroy(&token);
    /* First loan body is real checked syntax, not opaque bytes. */
    CHECK(test_rejected(c, "{let x=u8(7);loan_read(x){|r|missing()};unit}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-UNKNOWN-CALLEE"));
    CHECK(test_rejected(c, "{let x=u8(7);loan_read(x){|r|return unit;};unit}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P9-RETURN-CONTEXT"));
    CHECK(test_rejected(c, "{let x=u8(7);loan_read(x){|r|unit};r;unit}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-UNKNOWN-BINDING"));
    /* Domain zero in a host fixture alone is not implicit source authority. */
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    CHECK(nl_semantic_seed_value(c, "fixture", u8, NL_DEPENDENCY_FREE,
                                 &symbol) == NL_CHECK_OK);
    CHECK(test_rejected(c, "loan_read(fixture){|r|unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "LOCAL-LOAN-PROFILE"));
    /* E6: nested normal ref result preserves the OUTER live scope; only the
     * inner scope ends. Converting it later is legal, escaping outer is not. */
    TestChecked nested = {0};
    CHECK(test_run(c,
                   "{let x=u8(7);loan_read(x){|a|let "
                   "q=loan_read(x){|b|a};ptr_from_ref(q)}}",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &nested));
    test_checked_destroy(&nested);
    CHECK(test_rejected(c, "{let x=u8(7);loan_read(x){|a|loan_read(x){|b|a}}}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P8-EXIT-DEPENDENCY"));
    /* Supporting negative facts on an actual source-created, still-live root.
     * A ptr token with bad facts cannot replace the reacquisition proof. */
    TestChecked live = {0};
    CHECK(
        test_run(c, "let live=u8(7);", TEST_SOURCE, NL_CHECK_OK, NULL, &live));
    NLSemanticBindingView lb;
    NLSemanticPlaceView lp;
    CHECK(nl_semantic_binding_view(c, test_root(&live)->symbol, &lb));
    CHECK(nl_semantic_place_view(c, lb.place, &lp) && lp.live &&
          lp.implicit_local);
    NLTypeId pointer_type;
    CHECK(nl_semantic_compound_type(c, NL_TYPE_PTR, u8, NL_ACCESS_READ, false,
                                    &pointer_type) == NL_CHECK_OK);
    const char *names[] = {"unknown", "invalid", "stale", "no_read"};
    const char *codes[] = {"P3-UNKNOWN-PROVENANCE", "P3-INVALID-PROVENANCE",
                           "P3-STALE-POINTER", "P3-ACCESS"};
    for (size_t i = 0; i < 4; ++i) {
        NLReferenceFacts facts = {.place = lb.place,
                                  .incarnation = lp.incarnation,
                                  .provenance = NL_PROVENANCE_VALID,
                                  .readable = true};
        if (i == 0)
            facts.provenance = NL_PROVENANCE_UNKNOWN;
        else if (i == 1)
            facts.provenance = NL_PROVENANCE_INVALID;
        else if (i == 2)
            facts.incarnation = 0;
        else
            facts.readable = false;
        CHECK(nl_semantic_seed_reference(c, names[i], pointer_type, facts,
                                         &symbol) == NL_CHECK_OK);
        char input[128];
        CHECK(snprintf(input, sizeof(input), "loan_read_ptr(%s){|r|unit}",
                       names[i]) > 0);
        CHECK(test_rejected(c, input, TEST_SOURCE,
                            i == 0 ? NL_CHECK_ANALYSIS_PRECISION_LIMIT
                                   : NL_CHECK_SEMANTIC_ERROR,
                            codes[i]));
    }
    test_checked_destroy(&live);
    NLTypeId hidden_type;
    CHECK(nl_semantic_nominal(c, "Hidden", false, true, &hidden_type) ==
          NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(c, "hidden", hidden_type,
                                 NL_HIDDEN_DEPENDENCIES,
                                 &symbol) == NL_CHECK_OK);
    CHECK(test_rejected(c, "{let x=u8(7);loan_read(x){|r|hidden}}", TEST_SOURCE,
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P3-DEPENDENCIES-UNSUPPORTED"));
    nl_semantic_destroy(c);
    c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestChecked ordinary_names = {0};
    CHECK(test_run(c,
                   "{let loan_read=u8(7);let "
                   "loan_read_ptr=u8(9);loan_read;loan_read_ptr;unit}",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &ordinary_names));
    test_checked_destroy(&ordinary_names);
    nl_semantic_destroy(c);
    return true;
}
static bool conflict(void)
{
    /* N3 SUPPORTING invariant, not an actual-source u8 ending spelling.
     * This is the same production conflicts(...ending=true) path used by
     * source-local scope end and transfer; no extra lifetime API is added. */
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x, r;
    CHECK(nl_semantic_seed_value(f.context, "x", f.discardable,
                                 NL_DEPENDENCY_FREE, &x) == NL_CHECK_OK);
    NLSemanticBindingView xb;
    CHECK(nl_semantic_binding_view(f.context, x, &xb));
    CHECK(test_reference(f.context, "r", xb.place, NL_TYPE_REF, NL_ACCESS_READ,
                         false, &r, NULL));
    CHECK(test_rejected(f.context, "x", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-REF-CONFLICT"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool resource(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestChecked local = {0};
    CHECK(test_run(c, "let x=u8(7);", TEST_SOURCE, NL_CHECK_OK, NULL, &local));
    const NLSymbolId x = test_root(&local)->symbol;
    NLSemanticBindingView xb;
    NLSemanticPlaceView root;
    CHECK(nl_semantic_binding_view(c, x, &xb));
    CHECK(nl_semantic_place_view(c, xb.place, &root) && root.live &&
          root.implicit_local);
    for (size_t i = 0; i < NL_SEMANTIC_MAX_ENTRIES; ++i) {
        NLScopeId scope = 0;
        CHECK(nl_semantic_scope(c, 0, false, &scope) == NL_CHECK_OK);
    }
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(c, &before));
    TestChecked failure = {0};
    CHECK(test_run(c, "loan_read(x){|r|ptr_from_ref(r)}", TEST_SOURCE,
                   NL_CHECK_RESOURCE_LIMIT, "P3-RESOURCE-LIMIT", &failure));
    CHECK(nl_semantic_snapshot(c, &after));
    CHECK(before.types == after.types && before.bindings == after.bindings &&
          before.values == after.values && before.places == after.places &&
          before.scopes == after.scopes &&
          before.last_incarnation == after.last_incarnation &&
          before.last_value_fact == after.last_value_fact);
    NLSemanticPlaceView current;
    CHECK(nl_semantic_place_view(c, xb.place, &current) &&
          test_place_equal(root, current));
    test_checked_destroy(&failure);
    test_checked_destroy(&local);
    nl_semantic_destroy(c);
    return true;
}
static bool failures(void)
{
    /* Exhaust parser, registration (durable plan + definition check), actual
     * call checking and fragment mutation allocations. Every failed attempt
     * keeps context and output owner unchanged; reuse succeeds afterwards. */
    for (size_t phase = 0; phase < 4; ++phase) {
        NLSource *source = NULL;
        NLSyntaxTree *syntax = NULL;
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        const char *text = phase < 2 ? witness
                           : phase == 2
                               ? "main()"
                               : "{let x=u8(7);let "
                                 "p=loan_read(x){|r|ptr_from_ref(r)};loan_read_"
                                 "ptr(p){|r2|ptr_from_ref(r2);unit};unit}";
        if (phase == 2) {
            CHECK(tree(witness, true, &source, &syntax));
            const NLSyntaxTree *inputs[] = {syntax};
            CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
                  NL_CHECK_OK);
            nl_syntax_tree_destroy(syntax);
            nl_source_destroy(source);
            source = NULL;
            syntax = NULL;
        }
        CHECK(nl_source_create(text, strlen(text), "faults", &source) ==
              NL_SOURCE_OK);
        NLParser *parser0 = NULL;
        CHECK(nl_parser_create(source, &parser0) == NL_PARSE_OK);
        if (phase != 0)
            CHECK((phase == 1
                       ? nl_parser_parse_function_unit(parser0, &syntax, NULL)
                       : nl_parser_parse_source_fragment(parser0, &syntax,
                                                         NULL)) == NL_PARSE_OK);
        TestState before;
        CHECK(test_state(c, &before));
        bool completed = false;
        for (fail_at = 0; fail_at < 5000; ++fail_at) {
            NLCheckedFragment *artifact = NULL;
            allocation_index = 0;
            injecting = true;
            const NLSyntaxTree *inputs[] = {syntax};
            const int status =
                phase == 0
                    ? (int)nl_parser_parse_function_unit(parser0, &syntax, NULL)
                : phase == 1
                    ? (int)nl_semantic_register_function_unit(c, inputs, 1,
                                                              NULL)
                    : (int)nl_semantic_check_source_fragment(c, syntax,
                                                             &artifact, NULL);
            injecting = false;
            if (status == 0) {
                nl_checked_destroy(artifact);
                completed = true;
                break;
            }
            CHECK(status == (phase == 0 ? (int)NL_PARSE_OUT_OF_MEMORY
                                        : (int)NL_CHECK_OUT_OF_MEMORY));
            CHECK(artifact == NULL && test_unchanged(c, &before));
            if (phase == 0)
                CHECK(syntax == NULL);
        }
        CHECK(completed && fail_at > 0);
        nl_parser_destroy(parser0);
        nl_syntax_tree_destroy(syntax);
        nl_source_destroy(source);
        nl_semantic_destroy(c);
    }
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "parser") == 0)
        return parser() ? 0 : 1;
    if (strcmp(argv[1], "evidence") == 0)
        return evidence() ? 0 : 1;
    if (strcmp(argv[1], "mutation") == 0)
        return mutation() ? 0 : 1;
    if (strcmp(argv[1], "negatives") == 0)
        return negatives() ? 0 : 1;
    if (strcmp(argv[1], "conflict") == 0)
        return conflict() ? 0 : 1;
    if (strcmp(argv[1], "resource") == 0)
        return resource() ? 0 : 1;
    if (strcmp(argv[1], "failures") == 0)
        return failures() ? 0 : 1;
    return 2;
}
