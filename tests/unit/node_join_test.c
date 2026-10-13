#include "../../src/semantic_internal.h"
#include "../support/semantic_check.h"
#include "newlang/checked_c_node.h"
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
static bool parse(const char *text, NLSource **s, NLSyntaxTree **t)
{
    NLParser *p = NULL;
    CHECK(nl_source_create(text, strlen(text), "node-join-source", s) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(*s, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p, t, NULL) == NL_PARSE_OK);
    nl_parser_destroy(p);
    return true;
}
static bool registration(NLSemanticContext *c, const char *text,
                         NLCheckStatus expected, const char *code)
{
    NLSource *s = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(parse(text, &s, &t));
    const NLSyntaxTree *inputs[] = {t};
    NLFunctionUnitDiagnostic d = {0};
    TestState before;
    CHECK(test_state(c, &before));
    const NLCheckStatus status =
        nl_semantic_register_function_unit(c, inputs, 1, &d);
    if (status != expected)
        fprintf(stderr, "expected %d got %d: %s\n%s\n", expected, status,
                d.diagnostic.diagnostic.code, text);
    CHECK(status == expected);
    if (status != NL_CHECK_OK) {
        CHECK(test_unchanged(c, &before));
        CHECK(strcmp(d.diagnostic.diagnostic.code, code) == 0);
        printf("rejected %s status=%d rollback=true\n", code, status);
    }
    nl_syntax_tree_destroy(t);
    nl_source_destroy(s);
    return true;
}
/* Evidence below is inspected with public read-only APIs only. Every value
 * originates in a parsed full NewLang unit; there are no seeded rights. */
static bool evidence(const char *text, const char *label)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(registration(c, text, NL_CHECK_OK, NULL));
    TestChecked call = {0};
    CHECK(test_run(c, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL, &call));
    const NLCheckedFragment *body =
        nl_checked_call_body(call.artifact, nl_checked_root(call.artifact));
    CHECK(body != NULL && nl_checked_context(body) == c);
    NLCheckedNodeId match = 0;
    const NLCheckedNodeView *read = NULL;
    NLPlaceId originals[2] = {0};
    NLIncarnationId incarnations[2] = {0};
    size_t roots = 0;
    bool post_write = false;
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, i);
        if (n->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *receive =
                nl_checked_node_view(body, n->first_argument);
            NLSemanticTypeView ty;
            CHECK(receive != NULL &&
                  nl_semantic_type_view(c, receive->type, &ty));
            if (ty.kind == NL_TYPE_NOMINAL && ty.field_count == 2) {
                NLSemanticBindingView binding;
                NLSemanticPlaceView p;
                CHECK(roots < 2 &&
                      nl_semantic_binding_view(c, receive->symbol, &binding));
                CHECK(nl_semantic_place_view(c, binding.place, &p));
                CHECK(p.independent_root && p.placement.region == 0);
                originals[roots] = binding.place;
                incarnations[roots++] = p.incarnation;
            }
        }
        if (n->kind == NL_CHECKED_MATCH && !n->normal_frame_unchanged) {
            CHECK(match == 0 && n->normal_arms == 2 && n->type == 1 &&
                  !n->terminates && !n->normal_frame_unchanged && n->tail == 0);
            match = i;
        }
        if (n->kind == NL_CHECKED_FIELD_READ) {
            NLSemanticValueView v;
            CHECK(nl_semantic_value_view(c, n->results[0].value, &v));
            if (v.variant == 0) {
                CHECK(read == NULL && v.sum_payload == 0 &&
                      v.reference.place == 0 &&
                      v.reference.provenance == NL_PROVENANCE_UNKNOWN &&
                      v.reference_count == 0 &&
                      v.dependencies == NL_DEPENDENCY_FREE &&
                      v.value_dependency_count == 0 &&
                      n->field.payload_occurrence == 0);
                read = n;
            }
        }
        if (n->kind == NL_CHECKED_REPLACE && read != NULL &&
            n->field.child == read->field.child) {
            NLSemanticValueView old, next;
            CHECK(nl_semantic_value_view(c, n->field.old_value, &old) &&
                  nl_semantic_value_view(c, n->field.new_value, &next));
            CHECK(
                old.variant == 0 && old.sum_payload == 0 && next.variant == 1 &&
                n->field.child_fact != n->field.child_post_fact &&
                n->field.parent_fact != n->field.parent_post_fact &&
                n->field.parent_incarnation == read->field.parent_incarnation &&
                n->field.child_incarnation == read->field.child_incarnation);
            post_write = true;
        }
    }
    CHECK(roots == 2 && originals[0] != originals[1] && match != 0 &&
          read != NULL);
    CHECK(read->field.parent == originals[0] &&
          read->field.parent_incarnation == incarnations[0]);
    const NLSemanticContext *worlds[2];
    NLValueFactId sibling_fact = 0;
    NLValueId incoming[2];
    size_t writes = 0;
    for (size_t arm_index = 0; arm_index < 2; ++arm_index) {
        const NLCheckedFragment *arm =
            nl_checked_match_arm(body, match, arm_index);
        CHECK(arm != NULL);
        worlds[arm_index] = nl_checked_context(arm);
        CHECK(worlds[arm_index] != c);
        const NLCheckedNodeView *a =
            nl_checked_node_view(arm, nl_checked_root(arm));
        CHECK(a != NULL && a->kind == NL_CHECKED_MATCH_ARM && !a->terminates);
        for (size_t k = 0; k < 2; ++k) {
            NLSemanticPlaceView p;
            CHECK(nl_semantic_place_view(worlds[arm_index], originals[k], &p));
            CHECK(p.live && p.independent_root &&
                  p.incarnation == incarnations[k] && p.placement.region == 0);
        }
        NLSemanticPlaceView tail, link, sibling;
        NLSemanticValueView current;
        CHECK(nl_semantic_place_view(worlds[arm_index], originals[0], &tail));
        CHECK(nl_semantic_place_view(worlds[arm_index], tail.fixed_fields[0],
                                     &link));
        CHECK(nl_semantic_place_view(worlds[arm_index], tail.fixed_fields[1],
                                     &sibling));
        CHECK(nl_semantic_value_view(worlds[arm_index], link.current_value,
                                     &current));
        incoming[arm_index] = current.variant;
        CHECK(current.variant == 1 || current.variant == 2);
        CHECK(link.incarnation == read->field.child_incarnation);
        if (sibling_fact == 0)
            sibling_fact = sibling.current_fact;
        CHECK(sibling_fact == sibling.current_fact);
        NLSemanticValueView payload;
        CHECK(nl_semantic_value_view(worlds[arm_index], sibling.current_value,
                                     &payload));
        CHECK(payload.scalar_known && payload.scalar_value == 2);
        for (size_t i = 1; i <= nl_checked_node_count(arm); ++i) {
            const NLCheckedNodeView *n = nl_checked_node_view(arm, i);
            if (n->kind == NL_CHECKED_REPLACE) {
                CHECK(n->field.parent == originals[0] &&
                      n->field.child == read->field.child &&
                      n->field.child_incarnation ==
                          read->field.child_incarnation &&
                      n->field.parent_fact != n->field.parent_post_fact &&
                      n->field.child_fact != n->field.child_post_fact &&
                      read->field.parent_fact != n->field.parent_fact &&
                      read->field.child_fact != n->field.child_fact);
                if (strstr(label, "same_locator") != NULL) {
                    NLSemanticValueView old, next, op, np;
                    CHECK(nl_semantic_value_view(worlds[arm_index],
                                                 n->field.old_value, &old));
                    CHECK(nl_semantic_value_view(worlds[arm_index],
                                                 n->field.new_value, &next));
                    CHECK(old.variant == 2 && next.variant == 2 &&
                          nl_semantic_value_view(worlds[arm_index],
                                                 old.sum_payload, &op) &&
                          nl_semantic_value_view(worlds[arm_index],
                                                 next.sum_payload, &np));
                    CHECK(test_reference_equal(op.reference, np.reference) &&
                          n->field.payload_occurrence != 0 &&
                          n->field.post_payload_occurrence != 0 &&
                          n->field.payload_occurrence !=
                              n->field.post_payload_occurrence);
                    printf("O6 same Copy locator: field F%zu->F%zu, occurrence "
                           "P%zu->P%zu, parent RF%zu->RF%zu; common RF%zu\n",
                           n->field.child_fact, n->field.child_post_fact,
                           n->field.payload_occurrence,
                           n->field.post_payload_occurrence,
                           n->field.parent_fact, n->field.parent_post_fact,
                           read->field.parent_fact);
                }
                if (n->field.payload_occurrence != 0) {
                    NLSemanticOccurrenceView ended;
                    CHECK(nl_semantic_occurrence_view(
                        worlds[arm_index], n->field.payload_occurrence,
                        &ended));
                    CHECK(!ended.live);
                }
                ++writes;
            }
            if (n->kind == NL_CHECKED_LOAN_HEADER) {
                NLSemanticScopeView scope;
                CHECK(n->loan.body_nonescape_proved &&
                      nl_semantic_scope_view(worlds[arm_index], n->loan.scope,
                                             &scope));
                CHECK(!scope.active);
            }
        }
    }
    CHECK(worlds[0] != worlds[1] && writes >= 1 && writes <= 2);
    if (strstr(label, "post_write") != NULL)
        CHECK(post_write);
    char *output = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(call.artifact, &output, &length) ==
          NL_NODE_C_UNSUPPORTED);
    CHECK(output == NULL && length == 0);
    printf("%s source-owned=true normal_arms=2 original_roots=%zu,%zu "
           "incarnations=%zu,%zu incoming_variants=%zu,%zu "
           "joined_variant=unknown joined_fact=%zu sibling_fact=%zu "
           "scoped_writes=%zu native=unsupported\n",
           label, originals[0], originals[1], incarnations[0], incarnations[1],
           incoming[0], incoming[1], read->field.child_fact, sibling_fact,
           writes);
    CHECK(nl_sem_validate(c) == NL_CHECK_OK);
    test_checked_destroy(&call);
    nl_semantic_destroy(c);
    return true;
}
static const char decl[] = "struct Node{next:Option<ptr<Node>>,payload:u8}";
static const char prefix[] =
    "let tail=Node{next:Option<ptr<Node>>::None,payload:u8(2)};"
    "let p=loan_read(tail){|r|ptr_from_ref(r)};"
    "let head=Node{next:Option<ptr<Node>>::Some(p),payload:u8(1)};"
    "let observed=head@next;";
static bool rejected(const char *setup, const char *body, NLCheckStatus status,
                     const char *code)
{
    char text[4096];
    snprintf(text, sizeof(text), "%sfn main()->unit{%s%sunit}", decl, setup,
             body);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestChecked marker = {0};
    CHECK(test_run(c, "let marker=u8(71);", TEST_SOURCE, NL_CHECK_OK, NULL,
                   &marker));
    test_checked_destroy(&marker);
    CHECK(registration(c, text, status, code));
    nl_semantic_destroy(c);
    return true;
}
static bool negatives(void)
{
    CHECK(rejected(prefix,
                   "match observed{None=>{unit},Some(q)=>{let "
                   "old=loan_write(tail@next){|w|replace(w,Option<ptr<Node>>::"
                   "Some(q))};old;unit}};"
                   "let post=tail@next;match "
                   "post{None=>{unit},Some(q)=>{loan_read_ptr(q){|r|unit}}};",
                   NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                   "NODE-JOIN-POST-PRECISION"));
    CHECK(rejected(prefix,
                   "match observed{None=>{let "
                   "old=loan_write(head@next){|w|replace(w,Option<ptr<Node>>::"
                   "None)};old;unit},"
                   "Some(q)=>{let "
                   "old=loan_write(tail@next){|w|replace(w,Option<ptr<Node>>::"
                   "Some(q))};old;unit}};",
                   NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                   "P9-CONTINUATION-PRECISION"));
    CHECK(rejected(prefix,
                   "match observed{None=>{unit},Some(q)=>{let "
                   "local=Node{next:Option<ptr<Node>>::None,payload:u8(3)};"
                   "let private=loan_read(local){|r|ptr_from_ref(r)};"
                   "let "
                   "old=loan_write(tail@next){|w|replace(w,Option<ptr<Node>>::"
                   "Some(private))};old;unit}};",
                   NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                   "P9-CONTINUATION-PRECISION"));
    CHECK(rejected(prefix,
                   "match observed{None=>{unit},Some(q)=>{let "
                   "escape=loan_write(tail@next){|w|w};unit}};",
                   NL_CHECK_SEMANTIC_ERROR, "P8-EXIT-DEPENDENCY"));
    CHECK(rejected(
        "let p={let "
        "ended=Node{next:Option<ptr<Node>>::None,payload:u8(3)};loan_read("
        "ended){|r|ptr_from_ref(r)}};"
        "let tail=Node{next:Option<ptr<Node>>::None,payload:u8(2)};"
        "let head=Node{next:Option<ptr<Node>>::Some(p),payload:u8(1)};let "
        "observed=head@next;",
        "match observed{None=>{unit},Some(q)=>{loan_read_ptr(q){|r|unit}}};",
        NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
    CHECK(rejected(
        prefix,
        "match observed{None=>{unit},Some(q)=>{let "
        "old=loan_write(tail@next){|w|replace(w,Option<ptr<Node>>::Some(q))};"
        "let "
        "second=loan_write(head@next){|w|replace(w,Option<ptr<Node>>::None)};"
        "old;second;unit}};",
        NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P9-CONTINUATION-PRECISION"));
    CHECK(rejected(prefix,
                   "match observed{None=>{unit},Some(q)=>{let "
                   "owner=lifetime_domain();unit}};",
                   NL_CHECK_SEMANTIC_UNSUPPORTED, "ALLOCATED-SOURCE-PROFILE"));
    return true;
}

/* Negative trusted boundary only: a pre-existing affine owner must not be
 * consumed on just one normal edge. Positive evidence above is source-only. */
static bool owner_guard(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    NLTypeId linear = 0;
    CHECK(nl_semantic_nominal(c, "Linear", false, false, &linear) ==
          NL_CHECK_OK);
    CHECK(nl_semantic_register_function(c, "consume", &linear, 1,
                                        nl_semantic_unit_type(c), false,
                                        false) == NL_CHECK_OK);
    char text[4096];
    snprintf(text, sizeof(text),
             "%sfn main(owner:Linear)->unit{%s"
             "match observed{None=>{unit},Some(q)=>{consume(owner);"
             "let "
             "old=loan_write(tail@next){|w|replace(w,Option<ptr<Node>>::Some(q)"
             ")};old;unit}};unit}",
             decl, prefix);
    CHECK(registration(c, text, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                       "P9-CONTINUATION-PRECISION"));
    nl_semantic_destroy(c);
    return true;
}
static bool private_ids(void)
{
    const char private_body[] =
        "let local=Node{next:Option<ptr<Node>>::None,payload:u8(3)};"
        "let private_ptr=loan_read(local){|r|ptr_from_ref(r)};";
    char text[4096];
    snprintf(text, sizeof(text),
             "%sfn main()->unit{%s"
             "match observed{None=>{%sunit},Some(_)=>{%sunit}};unit}",
             decl, prefix, private_body, private_body);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(registration(c, text, NL_CHECK_OK, NULL));
    TestChecked call = {0};
    CHECK(test_run(c, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL, &call));
    const NLCheckedFragment *body =
        nl_checked_call_body(call.artifact, nl_checked_root(call.artifact));
    NLCheckedNodeId match = 0;
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i)
        if (nl_checked_node_view(body, i)->kind == NL_CHECKED_MATCH)
            match = i;
    CHECK(match != 0 &&
          nl_checked_node_view(body, match)->normal_frame_unchanged);
    NLReferenceFacts facts[2] = {{0}, {0}};
    const NLSemanticContext *worlds[2];
    for (size_t k = 0; k < 2; ++k) {
        const NLCheckedFragment *arm = nl_checked_match_arm(body, match, k);
        CHECK(arm != NULL);
        worlds[k] = nl_checked_context(arm);
        for (size_t i = 1; i <= nl_checked_node_count(arm); ++i) {
            const NLCheckedNodeView *n = nl_checked_node_view(arm, i);
            if (n->kind == NL_CHECKED_PTR_FROM_REF)
                facts[k] = n->reference_result;
        }
    }
    CHECK(worlds[0] != worlds[1] && facts[0].place != 0 &&
          facts[0].place == facts[1].place &&
          facts[0].incarnation == facts[1].incarnation);
    printf("private-ID calibration: distinct source-owned worlds, same "
           "PlaceId=%zu incarnation=%zu; no import\n",
           facts[0].place, facts[0].incarnation);
    test_checked_destroy(&call);
    nl_semantic_destroy(c);
    /* Same code now stores that private token into an ORIGINAL field. Both
     * arm-local numeric identities still coincide; neither is an original. */
    char body_text[2048];
    snprintf(body_text, sizeof(body_text),
             "match observed{None=>{%slet "
             "old=loan_write(tail@next){|w|replace(w,Option<ptr<Node>>::Some("
             "private_ptr))};old;unit},"
             "Some(_)=>{%slet "
             "old=loan_write(tail@next){|w|replace(w,Option<ptr<Node>>::Some("
             "private_ptr))};old;unit}};",
             private_body, private_body);
    CHECK(rejected(prefix, body_text, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                   "P9-CONTINUATION-PRECISION"));
    return true;
}
/* Trusted negative only: the selected Node source surface does not spell
 * hidden ValueFact dependencies. Both root and field facts here are read
 * from actual source-issued Nodes, but the dependent ref is a test input. */
static bool value_dependencies(void)
{
    for (size_t root_fact = 0; root_fact < 2; ++root_fact) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        CHECK(registration(c, decl, NL_CHECK_OK, NULL));
        TestChecked setup = {0};
        const char *steps[] = {
            "let tail=Node{next:Option<ptr<Node>>::None,payload:u8(2)};",
            "let p=loan_read(tail){|r|ptr_from_ref(r)};",
            "let head=Node{next:Option<ptr<Node>>::Some(p),payload:u8(1)};",
            "let observed=head@next;",
            "let head_ptr=loan_read(head){|r|ptr_from_ref(r)};",
            "let "
            "initial=loan_write(tail@next){|w|replace(w,Option<ptr<Node>>::"
            "Some(head_ptr))};"};
        for (size_t i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i) {
            CHECK(
                test_run(c, steps[i], TEST_SOURCE, NL_CHECK_OK, NULL, &setup));
            test_checked_destroy(&setup);
        }
        NLSemanticBindingView binding;
        CHECK(nl_semantic_binding_view(c, nl_semantic_find_binding(c, "tail"),
                                       &binding));
        NLSemanticPlaceView tail, selected;
        CHECK(nl_semantic_place_view(c, binding.place, &tail));
        const NLPlaceId site = root_fact ? binding.place : tail.fixed_fields[0];
        CHECK(nl_semantic_place_view(c, site, &selected));
        NLTypeId ref = 0;
        NLScopeId scope = 0;
        NLSymbolId dependent = 0;
        CHECK(nl_semantic_compound_type(c, NL_TYPE_REF, tail.type,
                                        NL_ACCESS_READ, false,
                                        &ref) == NL_CHECK_OK);
        CHECK(nl_semantic_scope(c, 0, true, &scope) == NL_CHECK_OK);
        CHECK(nl_semantic_seed_reference(
                  c, "dependent", ref,
                  (NLReferenceFacts){.place = binding.place,
                                     .incarnation = tail.incarnation,
                                     .scope = scope,
                                     .provenance = NL_PROVENANCE_VALID,
                                     .readable = true},
                  &dependent) == NL_CHECK_OK);
        NLSemanticValueView *v =
            &c->values[c->bindings[dependent - 1].view.value - 1];
        v->dependencies = NL_EXACT_VALUE_DEPENDENCIES;
        v->value_dependency_count = 1;
        v->value_dependencies[0] =
            (NLValueDependency){site, selected.current_fact};
        CHECK(nl_sem_validate(c) == NL_CHECK_OK);
        CHECK(test_rejected(c,
                            "loan_write(tail@next){|w|replace(w,Option<ptr<"
                            "Node>>::Some(head_ptr))}",
                            TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                            "FIELD-VALUE-DEPENDENCY"));
        const char block[] = "{match observed{None=>{unit},Some(q)=>{"
                             "let "
                             "old=loan_write(tail@next){|w|replace(w,Option<"
                             "ptr<Node>>::Some(head_ptr))};old;unit}};unit}";
        NLSource *s = NULL;
        NLParser *p = NULL;
        NLSyntaxTree *t = NULL;
        CHECK(nl_source_create(block, strlen(block), "dependent-join", &s) ==
              NL_SOURCE_OK);
        CHECK(nl_parser_create(s, &p) == NL_PARSE_OK);
        CHECK(nl_parser_parse_source_fragment(p, &t, NULL) == NL_PARSE_OK);
        TestState before;
        CHECK(test_state(c, &before));
        NLCheckedFragment *a = NULL;
        NLCheckDiagnostic d = {0};
        CHECK(nl_sem_check_function_block(
                  c, t, (NLFunctionBoundary){.result = 1}, &a, &d) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);
        CHECK(a == NULL &&
              strcmp(d.diagnostic.code, "FIELD-DEPENDENCY-CONTROL-PRECISION") ==
                  0 &&
              test_unchanged(c, &before));
        printf("O6 trusted %s-dependent ref: direct semantic reject, "
               "two-normal control precision, rollback=true\n",
               root_fact ? "parent root" : "field");
        nl_syntax_tree_destroy(t);
        nl_parser_destroy(p);
        nl_source_destroy(s);
        nl_semantic_destroy(c);
    }
    return true;
}
static bool oom(const char *text)
{
    NLSource *s = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(parse(text, &s, &t));
    const NLSyntaxTree *inputs[] = {t};
    bool success = false;
    size_t registration_positions = 0, call_positions = 0;
    for (fail_at = 0; fail_at < 4096; ++fail_at) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        TestState before;
        CHECK(test_state(c, &before));
        NLFunctionUnitDiagnostic d = {0};
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, &d);
        injecting = false;
        if (status == NL_CHECK_OK) {
            success = true;
            registration_positions = fail_at;
        } else
            CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
                  test_unchanged(c, &before));
        nl_semantic_destroy(c);
        if (success)
            break;
    }
    CHECK(success);
    success = false;
    for (fail_at = 0; fail_at < 4096; ++fail_at) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
              NL_CHECK_OK);
        NLSource *es = NULL;
        NLParser *ep = NULL;
        NLSyntaxTree *et = NULL;
        CHECK(nl_source_create("main()", 6, "entry", &es) == NL_SOURCE_OK);
        CHECK(nl_parser_create(es, &ep) == NL_PARSE_OK);
        CHECK(nl_parser_parse_expression_fragment(ep, &et, NULL) ==
              NL_PARSE_OK);
        TestState before;
        CHECK(test_state(c, &before));
        NLCheckedFragment *a = NULL;
        NLCheckDiagnostic d = {0};
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status = nl_semantic_check_expression(c, et, &a, &d);
        injecting = false;
        if (status == NL_CHECK_OK) {
            success = true;
            call_positions = fail_at;
            CHECK(a != NULL);
        } else
            CHECK(status == NL_CHECK_OUT_OF_MEMORY && a == NULL &&
                  test_unchanged(c, &before));
        nl_checked_destroy(a);
        nl_syntax_tree_destroy(et);
        nl_parser_destroy(ep);
        nl_source_destroy(es);
        nl_semantic_destroy(c);
        if (success)
            break;
    }
    CHECK(success);
    printf("OOM registration=%zu call=%zu all_failures_rollback=true\n",
           registration_positions, call_positions);
    nl_syntax_tree_destroy(t);
    nl_source_destroy(s);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    FILE *f = fopen(argv[2], "rb");
    if (f == NULL)
        return 2;
    char text[4096];
    size_t n = fread(text, 1, sizeof(text) - 1, f);
    bool loaded = !ferror(f) && feof(f);
    fclose(f);
    if (!loaded)
        return 2;
    text[n] = 0;
    if (strcmp(argv[1], "evidence") == 0)
        return evidence(text, argv[2]) ? 0 : 1;
    if (strcmp(argv[1], "negatives") == 0)
        return negatives() ? 0 : 1;
    if (strcmp(argv[1], "owner") == 0)
        return owner_guard() ? 0 : 1;
    if (strcmp(argv[1], "private-ids") == 0)
        return private_ids() ? 0 : 1;
    if (strcmp(argv[1], "dependencies") == 0)
        return value_dependencies() ? 0 : 1;
    if (strcmp(argv[1], "oom") == 0)
        return oom(text) ? 0 : 1;
    return 2;
}
