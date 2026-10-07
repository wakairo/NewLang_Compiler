#include "../../src/semantic_internal.h"
#include "../support/semantic_check.h"
#include <stdint.h>
#include <stdlib.h>

static const char declaration[] =
    "struct Pair{left:u8,right:u8}fn unused()->unit{unit}";
static const char creation[] = "let p=Pair{left:u8(7),right:u8(9)};";
static const char change[] =
    "let old=loan_write(p.left){|w|replace(w,u8(11))};";
static const char witness[] =
    "struct Pair{left:u8,right:u8}fn main()->unit{"
    "let p=Pair{left:u8(7),right:u8(9)};let before=p.left;"
    "let old=loan_write(p.left){|w|replace(w,u8(11))};"
    "let after=p.left;let sibling=p.right;let Pair{left,right}=p;"
    "before;old;after;sibling;left;right;unit}";

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
static bool register_source(NLSemanticContext *c, const char *text)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_create(text, strlen(text), "fixed-source", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    const NLSyntaxTree *inputs[] = {tree};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus status = nl_semantic_register_function_unit(c, inputs, 1, &d);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "registration: %d %s\n", status,
                d.diagnostic.diagnostic.code);
    CHECK(status == NL_CHECK_OK);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool run_ok(NLSemanticContext *c, const char *text)
{
    TestChecked t = {0};
    CHECK(test_run(c, text, TEST_SOURCE, NL_CHECK_OK, NULL, &t));
    test_checked_destroy(&t);
    return true;
}
static bool setup(NLSemanticContext **c)
{
    CHECK(nl_semantic_create(c) == NL_CHECK_OK);
    CHECK(register_source(*c, declaration));
    CHECK(run_ok(*c, creation));
    return true;
}
static bool binding_scalar(NLSemanticContext *c, const char *name, size_t value)
{
    const NLSymbolId id = nl_semantic_find_binding(c, name);
    NLSemanticBindingView b;
    NLSemanticValueView v;
    CHECK(id != 0 && nl_semantic_binding_view(c, id, &b));
    CHECK(nl_semantic_value_view(c, b.value, &v));
    CHECK(v.type == nl_semantic_core_type(c, NL_TYPE_U8) && v.scalar_known &&
          v.scalar_value == value);
    return true;
}
static bool parser_tests(void)
{
    const char *outside[] = {"p.left.right", "make().left",
                             "loan_write(p.left.right){|w|unit}"};
    for (size_t i = 0; i < sizeof(outside) / sizeof(outside[0]); ++i) {
        NLSource *source = NULL;
        NLParser *parser = NULL;
        NLSyntaxTree *tree = NULL;
        CHECK(nl_source_create(outside[i], strlen(outside[i]), "profile",
                               &source) == NL_SOURCE_OK);
        CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
        CHECK(nl_parser_parse_source_fragment(parser, &tree, NULL) ==
              NL_PARSE_SYNTAX_UNSUPPORTED);
        CHECK(tree == NULL);
        nl_parser_destroy(parser);
        nl_source_destroy(source);
    }
    bool success = false;
    for (fail_at = 0; fail_at < 256; ++fail_at) {
        NLSource *source = NULL;
        NLParser *parser = NULL;
        NLSyntaxTree *tree = NULL;
        CHECK(nl_source_create(witness, strlen(witness), "parser-oom",
                               &source) == NL_SOURCE_OK);
        CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
        allocation_index = 0;
        injecting = true;
        const NLParseStatus status =
            nl_parser_parse_function_unit(parser, &tree, NULL);
        injecting = false;
        if (status == NL_PARSE_OK) {
            success = true;
            const NLSyntaxView *unit =
                nl_syntax_node_view(nl_syntax_tree_root(tree));
            const NLSyntaxView *function = nl_syntax_node_view(
                nl_syntax_next_argument(unit->data.function_unit.declarations));
            const NLSyntaxView *block =
                nl_syntax_node_view(function->data.function.body);
            const NLSyntaxNode *before =
                nl_syntax_next_argument(block->data.block.items);
            const NLSyntaxView *binding = nl_syntax_node_view(before);
            CHECK(
                nl_syntax_node_view(binding->data.binding.initializer)->kind ==
                NL_SYNTAX_DOTTED);
            binding = nl_syntax_node_view(nl_syntax_next_argument(before));
            const NLSyntaxView *loan =
                nl_syntax_node_view(binding->data.binding.initializer);
            CHECK(loan->kind == NL_SYNTAX_LOCAL_WRITE_LOAN &&
                  nl_syntax_node_view(loan->data.loan.source)->kind ==
                      NL_SYNTAX_DOTTED);
        } else {
            CHECK(status == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
            CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) ==
                  NL_PARSE_OK);
        }
        nl_syntax_tree_destroy(tree);
        nl_parser_destroy(parser);
        nl_source_destroy(source);
        if (success)
            break;
    }
    CHECK(success);
    return true;
}
static bool state(void)
{
    NLSemanticContext *c = NULL;
    CHECK(setup(&c));
    const NLSymbolId symbol = nl_semantic_find_binding(c, "p");
    const NLSemanticBindingView pb = c->bindings[symbol - 1].view;
    const NLSemanticPlaceView before = c->places[pb.place - 1];
    CHECK(before.live && before.implicit_local &&
          before.fixed_field_count == 2);
    const NLPlaceId left = before.fixed_fields[0],
                    right = before.fixed_fields[1];
    const NLSemanticPlaceView l = c->places[left - 1], r = c->places[right - 1];
    CHECK(!l.independent_root && l.parent_aggregate == pb.place &&
          l.parent_field_index == 0);
    CHECK(l.parent_incarnation == before.incarnation &&
          l.current_value == c->values[pb.value - 1].fields[0]);
    CHECK(c->values[l.current_value - 1].carrier == NL_CARRIER_AGGREGATE);
    CHECK(run_ok(c, "let before=p.left;"));
    CHECK(binding_scalar(c, "before", 7));
    CHECK(run_ok(c, "let q=p;")); /* independent Copy, semantic-only control */
    CHECK(run_ok(c, change));
    const NLSemanticPlaceView after = c->places[pb.place - 1],
                              now = c->places[left - 1];
    CHECK(after.incarnation == before.incarnation &&
          now.incarnation == l.incarnation);
    CHECK(after.current_fact != before.current_fact &&
          now.current_fact != l.current_fact);
    CHECK(after.current_value != before.current_value);
    CHECK(test_place_equal(r, c->places[right - 1]));
    CHECK(c->values[r.current_value - 1].aggregate_owner ==
          after.current_value);
    CHECK(c->values[before.current_value - 1].carrier == NL_CARRIER_ENDED);
    CHECK(nl_fixed_validate(c) == NL_CHECK_OK);
    CHECK(run_ok(c, "let after=p.left;"));
    CHECK(run_ok(c, "let sibling=p.right;"));
    CHECK(binding_scalar(c, "old", 7) && binding_scalar(c, "after", 11) &&
          binding_scalar(c, "sibling", 9));
    CHECK(run_ok(c, "let Pair{left,right}=p;"));
    CHECK(binding_scalar(c, "left", 11) && binding_scalar(c, "right", 9));
    CHECK(run_ok(c, "let qleft=q.left;"));
    CHECK(binding_scalar(c, "qleft", 7));
    CHECK(c->bindings[symbol - 1].view.availability == NL_AVAILABLE);
    CHECK(run_ok(c, "loan_write(p.left){|w|replace(w,u8(13))}"));
    CHECK(c->places[left - 1].incarnation == l.incarnation);
    CHECK(test_place_equal(r, c->places[right - 1]));
    nl_semantic_destroy(c);
    return true;
}
static bool evidence(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(register_source(c, witness));
    TestChecked call = {0};
    CHECK(test_run(c, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL, &call));
    const NLCheckedFragment *body =
        nl_checked_call_body(call.artifact, nl_checked_root(call.artifact));
    CHECK(body != NULL);
    const size_t expected[] = {7, 11, 9};
    size_t reads = 0, loans = 0, replaces = 0;
    NLCheckedField initial = {0};
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(body, i);
        if (v->kind == NL_CHECKED_FIELD_READ) {
            CHECK(reads < 3 && v->field.present &&
                  v->field.dependency_compatible);
            CHECK(v->field.access == NL_ACCESS_READ &&
                  v->scalar_result.value == expected[reads]);
            if (reads == 0)
                initial = v->field;
            CHECK(v->field.parent == initial.parent &&
                  v->field.parent_incarnation == initial.parent_incarnation);
            if (reads < 2)
                CHECK(v->field.child == initial.child &&
                      v->field.child_incarnation == initial.child_incarnation);
            ++reads;
        } else if (v->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(v->field.present && v->loan.access == NL_ACCESS_WRITE &&
                  !v->loan.is_exclusive);
            CHECK(v->loan.body_nonescape_proved &&
                  v->loan.normal_result_forwarded);
            CHECK(v->field.child == initial.child &&
                  v->loan.place == initial.child);
            const NLSemanticTypeView t =
                c->types[c->bindings[v->loan.ref_symbol - 1].view.type - 1]
                    .view;
            CHECK(t.kind == NL_TYPE_REF && t.access == NL_ACCESS_WRITE &&
                  !t.is_exclusive);
            ++loans;
        } else if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(v->field.present && v->field.dependency_compatible);
            CHECK(v->field.parent_post_fact != v->field.parent_fact &&
                  v->field.child_post_fact != v->field.child_fact);
            CHECK(c->values[v->field.old_value - 1].scalar_value == 7 &&
                  c->values[v->field.new_value - 1].scalar_value == 11);
            CHECK(v->results[0].value == v->field.old_value);
            ++replaces;
        }
    }
    CHECK(reads == 3 && loans == 1 && replaces == 1);
    const char *names[] = {"before",  "old",  "after",
                           "sibling", "left", "right"};
    const size_t values[] = {7, 7, 11, 9, 11, 9};
    for (size_t n = 0; n < sizeof(names) / sizeof(names[0]); ++n) {
        bool found = false;
        for (size_t b = 0; b < c->binding_count; ++b) {
            if (strcmp(c->bindings[b].name, names[n]) != 0)
                continue;
            const NLSemanticValueView value =
                c->values[c->bindings[b].view.value - 1];
            CHECK(value.type == nl_semantic_core_type(c, NL_TYPE_U8) &&
                  value.scalar_known && value.scalar_value == values[n]);
            found = true;
        }
        CHECK(found);
    }
    CHECK(!c->places[initial.parent - 1].live &&
          !c->places[initial.child - 1].live);
    CHECK(nl_fixed_validate(c) == NL_CHECK_OK);
    test_checked_destroy(&call);
    nl_semantic_destroy(c);
    return true;
}
static bool dependencies(void)
{
    for (size_t source = 0; source < 3; ++source) {
        NLSemanticContext *c = NULL;
        CHECK(setup(&c));
        const NLPlaceId root =
            c->bindings[nl_semantic_find_binding(c, "p") - 1].view.place;
        const NLPlaceId target =
            source == 0 ? root : c->places[root - 1].fixed_fields[source - 1];
        NLTypeId type;
        CHECK(nl_semantic_nominal(c, "Dependent", false, true, &type) ==
              NL_CHECK_OK);
        NLSymbolId dependent;
        CHECK(nl_semantic_seed_value(c, "dependent", type, NL_DEPENDENCY_FREE,
                                     &dependent) == NL_CHECK_OK);
        /* Trusted negative/control fixture, never an executable source prelude.
         * Production exact evidence/overlap checking receives this valid atom.
         */
        NLSemanticValueView *v =
            &c->values[c->bindings[dependent - 1].view.value - 1];
        v->dependencies = NL_EXACT_VALUE_DEPENDENCIES;
        v->value_dependency_count = 1;
        v->value_dependencies[0] =
            (NLValueDependency){target, c->places[target - 1].current_fact};
        CHECK(nl_fixed_dependencies(c) == NL_CHECK_OK);
        if (source < 2) {
            CHECK(test_rejected(c, change, TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                                "FIELD-VALUE-DEPENDENCY"));
        } else {
            CHECK(run_ok(c, change));
            CHECK(nl_fixed_dependencies(c) == NL_CHECK_OK);
            CHECK(c->values[c->bindings[dependent - 1].view.value - 1]
                      .dependencies == NL_EXACT_VALUE_DEPENDENCIES);
        }
        CHECK(test_rejected(c, "if(unit){unit}else{unit}", TEST_SOURCE,
                            NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                            "FIELD-DEPENDENCY-CONTROL-PRECISION"));
        CHECK(nl_sem_function_exit(c, c->scope_count, root - 1) ==
              NL_CHECK_SEMANTIC_ERROR);
        CHECK(nl_fixed_end_dependencies(c, root, SIZE_MAX) ==
              NL_CHECK_SEMANTIC_ERROR);
        /* Existing reference join/conversion must not erase new exact atoms. */
        NLSymbolId ref;
        CHECK(test_reference(c, "annotated",
                             c->places[root - 1].fixed_fields[0], NL_TYPE_REF,
                             NL_ACCESS_READ, false, &ref, NULL));
        const NLValueId ref_value = c->bindings[ref - 1].view.value;
        c->values[ref_value - 1].dependencies = NL_EXACT_VALUE_DEPENDENCIES;
        c->values[ref_value - 1].value_dependency_count = 1;
        c->values[ref_value - 1].value_dependencies[0] =
            (NLValueDependency){target, c->places[target - 1].current_fact};
        TestState before;
        CHECK(test_state(c, &before));
        NLValueId joined = 0;
        CHECK(nl_semantic_join_references(c, &ref_value, 1, &joined) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);
        CHECK(joined == 0 && test_unchanged(c, &before));
        CHECK(test_rejected(c, "ptr_from_ref(annotated)", TEST_EXPRESSION,
                            NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                            "P3-DEPENDENCIES-UNSUPPORTED"));
        nl_semantic_destroy(c);
    }
    for (size_t k = 0; k < 2; ++k) {
        NLSemanticContext *c = NULL;
        CHECK(setup(&c));
        NLTypeId type;
        NLSymbolId dep;
        CHECK(nl_semantic_nominal(c, "Opaque", false, true, &type) ==
              NL_CHECK_OK);
        CHECK(nl_semantic_seed_value(c, "opaque", type,
                                     k ? NL_DEPENDENCIES_UNKNOWN
                                       : NL_HIDDEN_DEPENDENCIES,
                                     &dep) == NL_CHECK_OK);
        CHECK(test_rejected(c, change, TEST_SOURCE,
                            NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                            "P3-DEPENDENCIES-UNSUPPORTED"));
        nl_semantic_destroy(c);
    }
    return true;
}
static bool negatives(void)
{
    NLSemanticContext *c = NULL;
    CHECK(setup(&c));
    CHECK(test_rejected(c, "p.nope", TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "FIELD-UNKNOWN-FIELD"));
    CHECK(test_rejected(c, "loan_write(p.nope){|w|unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "FIELD-UNKNOWN-FIELD"));
    CHECK(test_rejected(c, "replace(p.left,u8(11))", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-WRITE-REF-REQUIRED"));
    CHECK(test_rejected(c, "loan_write(p.left){|w|replace(w,unit)}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-TYPE-MISMATCH"));
    CHECK(test_rejected(c, "loan_write(p.left){|w|w}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P8-EXIT-DEPENDENCY"));
    CHECK(test_rejected(c, "p.left()", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "FIELD-PROFILE"));
    CHECK(run_ok(c, "let x=u8(7);"));
    CHECK(test_rejected(c, "x.left", TEST_SOURCE, NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "FIELD-PROFILE"));
    CHECK(test_rejected(c, "loan_write(x.left){|w|unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "FIELD-PROFILE"));
    CHECK(run_ok(c, "let token=loan_read(x){|r|ptr_from_ref(r)};"));
    CHECK(test_rejected(c, "token.left", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "FIELD-PROFILE"));
    CHECK(register_source(
        c, "struct Other{left:u8,right:u8}fn other()->unit{unit}"));
    CHECK(run_ok(c, "let other_pair=Other{left:u8(7),right:u8(9)};"));
    CHECK(test_rejected(c, "other_pair.left", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "FIELD-PROFILE"));
    NLTypeId sum;
    const NLSumVariant variants[] = {{"left", 0}};
    CHECK(nl_semantic_register_sum(c, "x", variants, 1, &sum) == NL_CHECK_OK);
    CHECK(test_rejected(c, "x.nope", TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P6-UNKNOWN-VARIANT"));
    CHECK(run_ok(c, "x.left")); /* sum category, not scalar-local fallback */
    CHECK(nl_semantic_register_sum(c, "p", variants, 1, &sum) == NL_CHECK_OK);
    CHECK(test_rejected(c, "p.left", TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "FIELD-DOTTED-AMBIGUOUS"));
    CHECK(test_rejected(c, "p.nope", TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "FIELD-DOTTED-AMBIGUOUS"));
    CHECK(run_ok(c, change)); /* write designator selects field, not sum */
    const NLPlaceId root =
        c->bindings[nl_semantic_find_binding(c, "p") - 1].view.place;
    c->places[root - 1].live =
        false; /* invalid/stale pre-state destruction control */
    CHECK(test_rejected(c, "loan_write(p.left){|w|unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "FIELD-STALE-BASE"));
    nl_semantic_destroy(c);
    c = NULL;
    CHECK(setup(&c));
    const NLPlaceId p =
        c->bindings[nl_semantic_find_binding(c, "p") - 1].view.place;
    NLSymbolId exclusive;
    CHECK(test_reference(c, "exclusive", p, NL_TYPE_REF, NL_ACCESS_READ, true,
                         &exclusive, NULL));
    CHECK(test_rejected(c, "p.left", TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-REF-CONFLICT"));
    CHECK(test_rejected(c, "let q=p;", TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-REF-CONFLICT"));
    CHECK(test_rejected(c, change, TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-REF-CONFLICT"));
    nl_semantic_destroy(c);
    return true;
}
static bool failures(void)
{
    /* The actual source unit/body transaction, not only fragment receiving. */
    bool registered = false;
    NLSource *unit_source = NULL;
    NLParser *unit_parser = NULL;
    NLSyntaxTree *unit_tree = NULL;
    CHECK(nl_source_create(witness, strlen(witness), "unit-oom",
                           &unit_source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(unit_source, &unit_parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(unit_parser, &unit_tree, NULL) ==
          NL_PARSE_OK);
    const NLSyntaxTree *units[] = {unit_tree};
    for (fail_at = 0; fail_at < 512; ++fail_at) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        TestState before;
        CHECK(test_state(c, &before));
        NLFunctionUnitDiagnostic d = {0};
        allocation_index = 0;
        injecting = true;
        const NLCheckStatus status =
            nl_semantic_register_function_unit(c, units, 1, &d);
        injecting = false;
        if (status == NL_CHECK_OK) {
            registered = true;
            CHECK(run_ok(c, "main()"));
        } else {
            CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
                  test_unchanged(c, &before));
        }
        nl_semantic_destroy(c);
        if (registered)
            break;
    }
    CHECK(registered);
    nl_syntax_tree_destroy(unit_tree);
    nl_parser_destroy(unit_parser);
    nl_source_destroy(unit_source);
    const char *operations[] = {
        creation, "let copied=p.left;",
        change,   "{let old=loan_write(p.left){|w|replace(w,u8(11))};old;unit}",
        change,   "main()"};
    for (size_t op = 0; op < sizeof(operations) / sizeof(operations[0]); ++op) {
        bool success = false;
        for (fail_at = 0; fail_at < 256; ++fail_at) {
            NLSemanticContext *c = NULL;
            CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
            CHECK(register_source(c, op == 5 ? witness : declaration));
            if (op != 0 && op != 5)
                CHECK(run_ok(c, creation));
            if (op == 4) {
                NLTypeId type;
                NLSymbolId dep;
                CHECK(nl_semantic_nominal(c, "Dependent", false, true, &type) ==
                      NL_CHECK_OK);
                CHECK(nl_semantic_seed_value(c, "dependent", type,
                                             NL_DEPENDENCY_FREE,
                                             &dep) == NL_CHECK_OK);
                const NLPlaceId root =
                    c->bindings[nl_semantic_find_binding(c, "p") - 1]
                        .view.place;
                const NLPlaceId sibling = c->places[root - 1].fixed_fields[1];
                NLSemanticValueView *v =
                    &c->values[c->bindings[dep - 1].view.value - 1];
                v->dependencies = NL_EXACT_VALUE_DEPENDENCIES;
                v->value_dependency_count = 1;
                v->value_dependencies[0] = (NLValueDependency){
                    sibling, c->places[sibling - 1].current_fact};
            }
            NLSource *source = NULL;
            NLParser *parser = NULL;
            NLSyntaxTree *tree = NULL;
            CHECK(nl_source_create(operations[op], strlen(operations[op]),
                                   "failure", &source) == NL_SOURCE_OK);
            CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
            CHECK(nl_parser_parse_source_fragment(parser, &tree, NULL) ==
                  NL_PARSE_OK);
            TestState before;
            CHECK(test_state(c, &before));
            NLCheckedFragment *artifact = NULL;
            NLCheckDiagnostic d = {0};
            allocation_index = 0;
            injecting = true;
            const NLCheckStatus status =
                nl_semantic_check_source_fragment(c, tree, &artifact, &d);
            injecting = false;
            if (status == NL_CHECK_OK) {
                CHECK(artifact != NULL && nl_fixed_validate(c) == NL_CHECK_OK);
                success = true;
            } else {
                CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL);
                CHECK(test_unchanged(c, &before));
                CHECK(nl_fixed_validate(c) == NL_CHECK_OK);
            }
            nl_checked_destroy(artifact);
            nl_syntax_tree_destroy(tree);
            nl_parser_destroy(parser);
            nl_source_destroy(source);
            nl_semantic_destroy(c);
            if (success)
                break;
        }
        CHECK(success);
    }
    return true;
}
static bool resource(void)
{
    NLSemanticContext *invalid = NULL;
    CHECK(setup(&invalid));
    const NLPlaceId root =
        invalid->bindings[nl_semantic_find_binding(invalid, "p") - 1]
            .view.place;
    const NLValueId parent = invalid->places[root - 1].current_value;
    const NLValueId right = invalid->values[parent - 1].fields[1];
    invalid->values[parent - 1].fields[1] =
        invalid->values[parent - 1].fields[0];
    CHECK(nl_fixed_validate(invalid) == NL_CHECK_INTERNAL_ERROR);
    invalid->values[parent - 1].fields[1] = right;
    const NLPlaceId child = invalid->places[root - 1].fixed_fields[0];
    invalid->places[child - 1].parent_incarnation++;
    CHECK(!nl_fixed_live(invalid, child) &&
          nl_fixed_validate(invalid) == NL_CHECK_INTERNAL_ERROR);
    invalid->places[child - 1].parent_incarnation--;
    CHECK(nl_fixed_validate(invalid) == NL_CHECK_OK);
    nl_semantic_destroy(invalid);
    for (size_t k = 0; k < 3; ++k) {
        NLSemanticContext *c = NULL;
        CHECK(setup(&c));
        c->last_value_fact = SIZE_MAX - k;
        CHECK(test_rejected(c, change, TEST_SOURCE, NL_CHECK_RESOURCE_LIMIT,
                            "P3-RESOURCE-LIMIT"));
        nl_semantic_destroy(c);
    }
    for (size_t k = 0; k < 3; ++k) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        CHECK(register_source(c, declaration));
        c->last_incarnation = SIZE_MAX - k;
        CHECK(test_rejected(c, creation, TEST_SOURCE, NL_CHECK_RESOURCE_LIMIT,
                            "P3-RESOURCE-LIMIT"));
        nl_semantic_destroy(c);
    }
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "parser") == 0)
        return parser_tests() ? 0 : 1;
    if (strcmp(argv[1], "state") == 0)
        return state() ? 0 : 1;
    if (strcmp(argv[1], "evidence") == 0)
        return evidence() ? 0 : 1;
    if (strcmp(argv[1], "dependencies") == 0)
        return dependencies() ? 0 : 1;
    if (strcmp(argv[1], "negatives") == 0)
        return negatives() ? 0 : 1;
    if (strcmp(argv[1], "failures") == 0)
        return failures() ? 0 : 1;
    if (strcmp(argv[1], "resource") == 0)
        return resource() ? 0 : 1;
    return 2;
}
