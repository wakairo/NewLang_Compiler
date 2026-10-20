#include "../../src/semantic_internal.h"
#include "../support/semantic_check.h"
#include <stdint.h>
#include <stdlib.h>

static const char declaration[] =
    "struct Node{next:Option<ptr<Node>>,payload:u8,}";
static const char creation[] =
    "let head=Node{next:Option<ptr<Node>>::None,payload:u8(1)};";
static const char some[] =
    "loan_write(head@next){|w|replace(w,Option<ptr<Node>>::Some(p))}";
static const char none[] =
    "loan_write(head@next){|w|replace(w,Option<ptr<Node>>::None)}";

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
static bool parse_unit(const char *text, NLSource **s, NLSyntaxTree **t)
{
    CHECK(nl_source_create(text, strlen(text), "node-link", s) == NL_SOURCE_OK);
    NLParser *p = NULL;
    CHECK(nl_parser_create(*s, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p, t, NULL) == NL_PARSE_OK);
    nl_parser_destroy(p);
    return true;
}
static bool register_source(NLSemanticContext *c, const char *text)
{
    NLSource *s = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(parse_unit(text, &s, &t));
    const NLSyntaxTree *units[] = {t};
    NLFunctionUnitDiagnostic d = {0};
    const NLCheckStatus status =
        nl_semantic_register_function_unit(c, units, 1, &d);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "registration %d: %s\n", status,
                d.diagnostic.diagnostic.code);
    CHECK(status == NL_CHECK_OK);
    nl_syntax_tree_destroy(t);
    nl_source_destroy(s);
    return true;
}
static bool run_ok(NLSemanticContext *c, const char *text)
{
    TestChecked t = {0};
    CHECK(test_run(c, text, TEST_SOURCE, NL_CHECK_OK, NULL, &t));
    test_checked_destroy(&t);
    CHECK(nl_sem_validate(c) == NL_CHECK_OK);
    return true;
}
static bool setup(NLSemanticContext **c)
{
    CHECK(nl_semantic_create(c) == NL_CHECK_OK);
    CHECK(register_source(*c, declaration));
    CHECK(run_ok(*c,
                 "let tail=Node{next:Option<ptr<Node>>::None,payload:u8(2)};"));
    CHECK(run_ok(*c, "let p=loan_read(tail){|r|ptr_from_ref(r)};"));
    CHECK(run_ok(*c, creation));
    return true;
}
static NLPlaceId place(NLSemanticContext *c, const char *name)
{
    const NLSymbolId s = nl_semantic_find_binding(c, name);
    return s == 0 ? 0 : c->bindings[s - 1].view.place;
}
static NLValueId value(NLSemanticContext *c, const char *name)
{
    const NLPlaceId p = place(c, name);
    return p == 0 ? 0 : c->places[p - 1].current_value;
}
static bool evidence(const char *witness)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(register_source(c, witness)); /* syntax/source destroyed here */
    TestChecked call = {0};
    CHECK(test_run(c, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL, &call));
    const NLCheckedFragment *body =
        nl_checked_call_body(call.artifact, nl_checked_root(call.artifact));
    CHECK(body != NULL);
    size_t reads = 0, writes = 0, roots = 0, matches = 0;
    NLCheckedField first = {0};
    NLReferenceFacts ptr = {0};
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, i);
        if (n->kind == NL_CHECKED_PTR_FROM_REF) {
            CHECK(n->has_reference_result);
            ptr = n->reference_result;
            CHECK(ptr.provenance == NL_PROVENANCE_VALID && ptr.scope == 0);
        } else if (n->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(!n->loan.is_exclusive && n->loan.implicit_local &&
                  n->loan.prevent_lifetime_end &&
                  n->loan.body_nonescape_proved &&
                  n->loan.normal_result_forwarded);
            if (n->field.present) {
                CHECK(n->loan.access == NL_ACCESS_WRITE &&
                      n->loan.place == n->field.child);
                const NLSemanticTypeView t =
                    c->types[c->bindings[n->loan.ref_symbol - 1].view.type - 1]
                        .view;
                CHECK(t.kind == NL_TYPE_REF && t.target == n->field.type &&
                      t.access == NL_ACCESS_WRITE && !t.is_exclusive);
            } else {
                CHECK(n->loan.access == NL_ACCESS_READ);
                ++roots;
            }
        } else if (n->kind == NL_CHECKED_REPLACE) {
            CHECK(n->field.present && n->field.index == 0 &&
                  n->field.dependency_compatible &&
                  n->field.parent_fact != n->field.parent_post_fact &&
                  n->field.child_fact != n->field.child_post_fact);
            CHECK(n->results[0].value == n->field.old_value);
            const NLSemanticValueView old = c->values[n->field.old_value - 1],
                                      next = c->values[n->field.new_value - 1];
            if (writes == 0) {
                first = n->field;
                CHECK(old.variant == 1 && next.variant == 2 &&
                      n->field.payload_occurrence == 0 &&
                      n->field.post_payload_occurrence != 0);
                CHECK(test_reference_equal(
                    c->values[next.sum_payload - 1].reference, ptr));
            } else {
                CHECK(writes == 1 && old.variant == 2 && next.variant == 1 &&
                      n->field.parent == first.parent &&
                      n->field.child == first.child &&
                      n->field.parent_incarnation == first.parent_incarnation &&
                      n->field.child_incarnation == first.child_incarnation &&
                      n->field.payload_occurrence ==
                          first.post_payload_occurrence &&
                      n->field.post_payload_occurrence == 0);
                CHECK(!c->occurrences[n->field.payload_occurrence - 1].live);
                CHECK(test_reference_equal(
                    c->values[old.sum_payload - 1].reference, ptr));
            }
            ++writes;
        } else if (n->kind == NL_CHECKED_FIELD_READ) {
            CHECK(n->field.child == first.child &&
                  n->value_use == NL_VALUE_COPIED);
            const NLSemanticValueView copied =
                c->values[n->results[0].value - 1];
            CHECK(copied.variant == 2 &&
                  test_reference_equal(
                      c->values[copied.sum_payload - 1].reference, ptr));
            ++reads;
        } else if (n->kind == NL_CHECKED_MATCH) {
            CHECK(n->normal_arms == 2 && n->type == 1 && !n->terminates);
            const NLCheckedFragment *arm = nl_checked_match_arm(body, i, 1);
            CHECK(arm != NULL && nl_checked_context(arm) != c);
            bool reacquired = false;
            for (size_t a = 1; a <= nl_checked_node_count(arm); ++a) {
                const NLCheckedNodeView *v = nl_checked_node_view(arm, a);
                if (v->kind == NL_CHECKED_LOAN_HEADER) {
                    CHECK(v->loan.from_ptr && v->loan.place == ptr.place &&
                          v->loan.incarnation == ptr.incarnation &&
                          v->loan.implicit_local &&
                          v->loan.body_nonescape_proved);
                    reacquired = true;
                }
            }
            CHECK(reacquired);
            ++matches;
        }
    }
    CHECK(reads == 1 && writes == 2 && roots == 1 && matches == 1);
    CHECK(!c->places[first.parent - 1].live &&
          !c->places[first.child - 1].live && !c->places[ptr.place - 1].live);
    CHECK(nl_sem_validate(c) == NL_CHECK_OK);
    test_checked_destroy(&call);
    nl_semantic_destroy(c);
    return true;
}
static bool transitions(void)
{
    NLSemanticContext *c = NULL;
    CHECK(setup(&c));
    const NLPlaceId h = place(c, "head"), l = c->places[h - 1].fixed_fields[0],
                    sibling = c->places[h - 1].fixed_fields[1];
    const NLSemanticPlaceView root = c->places[h - 1], link = c->places[l - 1],
                              payload = c->places[sibling - 1];
    CHECK(link.payload_occurrence == 0 && link.parent_aggregate == h);
    TestChecked replaced = {0};
    CHECK(test_run(c, some, TEST_SOURCE, NL_CHECK_OK, NULL, &replaced));
    const NLValueId old_none = test_root(&replaced)->results[0].value;
    CHECK(c->values[old_none - 1].variant == 1 &&
          c->values[old_none - 1].carrier == NL_CARRIER_LOOSE);
    test_checked_destroy(&replaced);
    const NLOccurrenceId a = c->places[l - 1].payload_occurrence;
    const NLPlaceId old_payload = c->occurrences[a - 1].payload_place;
    const NLValueId old_some = c->places[l - 1].current_value;
    CHECK(run_ok(c, "let observed=head@next;"));
    CHECK(run_ok(c, "let copy=head;"));
    const NLValueId copied = value(c, "observed"), nodecopy = value(c, "copy");
    CHECK(copied != old_some && c->values[copied - 1].sum_payload !=
                                    c->values[old_some - 1].sum_payload);
    CHECK(c->values[nodecopy - 1].fields[0] != old_some);
    const NLReferenceFacts token = c->values[value(c, "p") - 1].reference;
    CHECK(test_reference_equal(
        c->values[c->values[copied - 1].sum_payload - 1].reference, token));
    CHECK(test_run(c, some, TEST_SOURCE, NL_CHECK_OK, NULL, &replaced));
    CHECK(test_root(&replaced)->results[0].value == old_some &&
          c->values[old_some - 1].carrier == NL_CARRIER_LOOSE &&
          c->values[c->values[old_some - 1].sum_payload - 1].carrier ==
              NL_CARRIER_SUM);
    test_checked_destroy(&replaced);
    const NLOccurrenceId b = c->places[l - 1].payload_occurrence;
    CHECK(a != b && b != 0 && !c->occurrences[a - 1].live &&
          !c->places[old_payload - 1].live && c->occurrences[b - 1].live);
    CHECK(run_ok(c, none));
    CHECK(!c->occurrences[b - 1].live &&
          c->places[l - 1].payload_occurrence == 0);
    CHECK(c->places[h - 1].incarnation == root.incarnation &&
          c->places[l - 1].incarnation == link.incarnation &&
          c->places[h - 1].current_fact != root.current_fact &&
          c->places[l - 1].current_fact != link.current_fact &&
          test_place_equal(payload, c->places[sibling - 1]));
    CHECK(c->values[payload.current_value - 1].aggregate_owner ==
          c->places[h - 1].current_value);
    CHECK(run_ok(
        c,
        "match observed{None=>{unit},Some(q)=>{loan_read_ptr(q){|r|unit}},}"));
    CHECK(run_ok(
        c, "loan_write(head@next){|w|store(w,Option<ptr<Node>>::Some(p))}"));
    const NLValueId stored = c->places[l - 1].current_value,
                    stored_ptr = c->values[stored - 1].sum_payload;
    CHECK(run_ok(c,
                 "loan_write(head@next){|w|store(w,Option<ptr<Node>>::None)}"));
    CHECK(c->values[stored - 1].carrier == NL_CARRIER_ENDED &&
          c->values[stored_ptr - 1].carrier == NL_CARRIER_ENDED);
    CHECK(run_ok(c, "loan_read(tail){|r|loan_write(head@next){|w|let alias=w;"
                    "replace(alias,Option<ptr<Node>>::Some(p))}}"));
    CHECK(nl_sem_validate(c) == NL_CHECK_OK);
    /* A live payload ref blocks whole-Option End even for the same variant. */
    const NLOccurrenceId live = c->places[l - 1].payload_occurrence;
    CHECK(live != 0);
    NLSymbolId ref;
    CHECK(test_reference(c, "payload_ref",
                         c->occurrences[live - 1].payload_place, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &ref, NULL));
    c->values[c->bindings[ref - 1].view.value - 1]
        .reference.occurrence_dependency = live;
    CHECK(test_rejected(c, some, TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P6-OCCURRENCE-CONFLICT"));
    nl_semantic_destroy(c);
    /* Field/type names are metadata, not magic Node/next/payload spellings. */
    c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(register_source(c, "struct Cell{link:Option<ptr<Cell>>,data:u8}"));
    CHECK(run_ok(c, "let cell=Cell{link:Option<ptr<Cell>>::None,data:u8(3)};"));
    CHECK(run_ok(c, "let token=loan_read(cell){|r|ptr_from_ref(r)};"));
    CHECK(run_ok(
        c,
        "loan_write(cell@link){|w|replace(w,Option<ptr<Cell>>::Some(token))}"));
    CHECK(run_ok(c, "let observed=cell@link;"));
    CHECK(run_ok(c, "loan_read_ptr(token){|r|unit}"));
    /* Ending an owning aggregate must end its recursively owned Option/ptr,
     * without leaving a detached live payload responsibility. */
    const NLValueId owned = value(c, "cell"),
                    option = c->values[owned - 1].fields[0],
                    token_member = c->values[option - 1].sum_payload;
    nl_fixed_detach(c, place(c, "cell"));
    nl_sem_end_value(c, owned);
    CHECK(c->values[option - 1].carrier == NL_CARRIER_ENDED &&
          c->values[token_member - 1].carrier == NL_CARRIER_ENDED);
    nl_semantic_destroy(c);
    return true;
}
static bool dependencies(void)
{
    for (size_t target = 0; target < 3; ++target) {
        NLSemanticContext *c = NULL;
        CHECK(setup(&c));
        const NLPlaceId h = place(c, "head");
        const NLPlaceId selected =
            target == 0 ? h : c->places[h - 1].fixed_fields[target - 1];
        NLTypeId t;
        NLSymbolId dep;
        CHECK(nl_semantic_nominal(c, "Dependent", false, true, &t) ==
              NL_CHECK_OK);
        CHECK(nl_semantic_seed_value(c, "dep", t, NL_DEPENDENCY_FREE, &dep) ==
              NL_CHECK_OK);
        NLSemanticValueView *v =
            &c->values[c->bindings[dep - 1].view.value - 1];
        v->dependencies = NL_EXACT_VALUE_DEPENDENCIES;
        v->value_dependency_count = 1;
        v->value_dependencies[0] =
            (NLValueDependency){selected, c->places[selected - 1].current_fact};
        if (target < 2)
            CHECK(test_rejected(c, some, TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                                "FIELD-VALUE-DEPENDENCY"));
        else {
            CHECK(run_ok(c, some));
            CHECK(nl_fixed_dependencies(c) == NL_CHECK_OK);
        }
        nl_semantic_destroy(c);
    }
    /* Copying Option preserves exact/opaque dependencies, never launders them.
     */
    for (size_t k = 0; k < 3; ++k) {
        NLSemanticContext *c = NULL;
        CHECK(setup(&c));
        const NLPlaceId h = place(c, "head"),
                        l = c->places[h - 1].fixed_fields[0],
                        s = c->places[h - 1].fixed_fields[1];
        NLSemanticValueView *v = &c->values[c->places[l - 1].current_value - 1];
        v->dependencies = k == 0   ? NL_EXACT_VALUE_DEPENDENCIES
                          : k == 1 ? NL_HIDDEN_DEPENDENCIES
                                   : NL_DEPENDENCIES_UNKNOWN;
        if (k == 0) {
            v->value_dependency_count = 1;
            v->value_dependencies[0] =
                (NLValueDependency){s, c->places[s - 1].current_fact};
            CHECK(run_ok(c, "let dependent_copy=head@next;"));
            const NLSemanticValueView copied =
                c->values[value(c, "dependent_copy") - 1];
            CHECK(copied.dependencies == NL_EXACT_VALUE_DEPENDENCIES &&
                  copied.value_dependency_count == 1 &&
                  copied.value_dependencies[0].place == s &&
                  copied.value_dependencies[0].fact ==
                      c->places[s - 1].current_fact);
        }
        CHECK(test_rejected(c, some, TEST_SOURCE,
                            NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                            "P3-DEPENDENCIES-UNSUPPORTED"));
        nl_semantic_destroy(c);
    }
    /* A returned old Option with a surviving link/ancestor fact blocks Change.
     */
    NLSemanticContext *c = NULL;
    CHECK(setup(&c));
    CHECK(run_ok(c, some));
    CHECK(run_ok(
        c,
        "let "
        "old=loan_write(head@next){|w|replace(w,Option<ptr<Node>>::None)};"));
    const NLPlaceId l = c->places[place(c, "head") - 1].fixed_fields[0];
    NLSemanticValueView *old = &c->values[value(c, "old") - 1];
    old->dependencies = NL_EXACT_VALUE_DEPENDENCIES;
    old->value_dependency_count = 1;
    old->value_dependencies[0] =
        (NLValueDependency){l, c->places[l - 1].current_fact};
    CHECK(test_rejected(c, some, TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "FIELD-VALUE-DEPENDENCY"));
    nl_semantic_destroy(c);
    return true;
}
static bool negatives(void)
{
    NLSemanticContext *c = NULL;
    CHECK(setup(&c));
    const char *inputs[] = {"head@payload",
                            "head@missing",
                            "p@next",
                            "loan_read(head){|r|r@next}",
                            "loan_write(head){|w|unit}",
                            "loan_write(head@next){|w|w}",
                            "loan_write(head@next){|w|replace(w,u8(7))}",
                            "loan_write(head@next){|w|replace(w,w)}",
                            "head::next",
                            "{let Node=u8(7);Node@next;unit}"};
    const char *codes[] = {"NODE-LINK-FIELD-PROFILE", "FIELD-UNKNOWN-FIELD",
                           "FIELD-PROFILE",           "FIELD-PROFILE",
                           "LOCAL-LOAN-PROFILE",      "P8-EXIT-DEPENDENCY",
                           "P3-TYPE-MISMATCH",        "P3-TYPE-MISMATCH",
                           "P6-SUM-QUALIFIER",        "FIELD-PROFILE"};
    const NLCheckStatus statuses[] = {
        NL_CHECK_SEMANTIC_UNSUPPORTED, NL_CHECK_SEMANTIC_ERROR,
        NL_CHECK_SEMANTIC_UNSUPPORTED, NL_CHECK_SEMANTIC_UNSUPPORTED,
        NL_CHECK_SEMANTIC_UNSUPPORTED, NL_CHECK_SEMANTIC_ERROR,
        NL_CHECK_SEMANTIC_ERROR,       NL_CHECK_SEMANTIC_ERROR,
        NL_CHECK_SEMANTIC_ERROR,       NL_CHECK_SEMANTIC_UNSUPPORTED};
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i)
        CHECK(test_rejected(c, inputs[i], TEST_SOURCE, statuses[i], codes[i]));
    CHECK(run_ok(c, "loan_write(head@next){|unused|unit}"));
    CHECK(test_rejected(
        c, "loan_write(head@next){|w|match w{None=>{unit},Some(q)=>{unit},}}",
        TEST_SOURCE, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
        "NODE-LINK-MATCH-PRECISION"));
    CHECK(run_ok(c, "loan_write(head@next){|w|loan_write(head@next){|alias|"
                    "replace(alias,Option<ptr<Node>>::Some(p))}}"));
    CHECK(
        run_ok(c, "let field_ptr=loan_write(head@next){|w|ptr_from_ref(w)};"));
    CHECK(test_rejected(c, "loan_read_ptr(field_ptr){|r|unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "LOCAL-LOAN-PROFILE"));
    CHECK(register_source(
        c, "struct Other{left:u8,right:u8}fn unused()->unit{unit}"));
    CHECK(run_ok(c, "let wrong=Other{left:u8(1),right:u8(2)};"));
    CHECK(test_rejected(c, "wrong@left", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "FIELD-PROFILE"));
    CHECK(register_source(c, "struct Cell{next:Option<ptr<Cell>>,payload:u8}"));
    CHECK(test_rejected(
        c, "loan_write(head@next){|w|replace(w,Option<ptr<Cell>>::None)}",
        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
    nl_semantic_destroy(c);
    /* The new unit join does not choose a representative arm or import Change.
     */
    c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    const char *bad = "struct Node{next:Option<ptr<Node>>,payload:u8}"
                      "fn main()->unit{let "
                      "n=Node{next:Option<ptr<Node>>::None,payload:u8(1)};"
                      "let p=loan_read(n){|r|ptr_from_ref(r)};"
                      "let o=Option<ptr<Node>>::Some(p);match o{None=>{unit},"
                      "Some(q)=>{loan_write(n@next){|w|replace(w,Option<ptr<"
                      "Node>>::Some(q))};unit},};unit}";
    NLSource *s = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(parse_unit(bad, &s, &t));
    const NLSyntaxTree *units[] = {t};
    TestState before;
    CHECK(test_state(c, &before));
    NLFunctionUnitDiagnostic d = {0};
    CHECK(nl_semantic_register_function_unit(c, units, 1, &d) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(strcmp(d.diagnostic.diagnostic.code, "P9-CONTINUATION-PRECISION") ==
              0 &&
          test_unchanged(c, &before));
    nl_syntax_tree_destroy(t);
    nl_source_destroy(s);
    nl_semantic_destroy(c);
    return true;
}
static bool liveness(void)
{
    NLSemanticContext *c = NULL;
    CHECK(setup(&c));
    CHECK(run_ok(c, "loan_read_ptr(p){|r|unit}"));
    const NLPlaceId target = place(c, "tail");
    const NLSemanticPlaceView old = c->places[target - 1];
    NLValueId fresh;
    CHECK(nl_sem_copy_value(c, old.current_value, &fresh) == NL_CHECK_OK);
    nl_fixed_detach(c, target);
    nl_sem_end_value(c, old.current_value);
    c->places[target - 1].live = false;
    c->places[target - 1].current_value = 0;
    CHECK(test_rejected(c, "loan_read_ptr(p){|r|unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
    /* Same PlaceId is not the same incarnation; this is a trusted internal
     * restart control, never an added source allocator or lifecycle API. */
    CHECK(nl_sem_install(c, target, fresh, 0) == NL_CHECK_OK);
    const NLSymbolId symbol = nl_semantic_find_binding(c, "tail");
    c->bindings[symbol - 1].view.value = fresh;
    CHECK(c->places[target - 1].incarnation != old.incarnation);
    CHECK(test_rejected(c, "loan_read_ptr(p){|r|unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
    CHECK(run_ok(c, "let fresh_ptr=loan_read(tail){|r|ptr_from_ref(r)};"));
    CHECK(run_ok(c, "loan_read_ptr(fresh_ptr){|r|unit}"));
    /* A live token without visible governing local stability is insufficient.
     */
    c->bindings[symbol - 1].hidden = true;
    CHECK(test_rejected(c, "loan_read_ptr(fresh_ptr){|r|unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "LOCAL-LOAN-PROFILE"));
    nl_semantic_destroy(c);
    c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(register_source(c, declaration));
    CHECK(run_ok(c, "let stale={let "
                    "n=Node{next:Option<ptr<Node>>::None,payload:u8(1)};loan_"
                    "read(n){|r|ptr_from_ref(r)}};"));
    CHECK(test_rejected(c, "loan_read_ptr(stale){|r|unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
    CHECK(run_ok(
        c, "let n=Node{next:Option<ptr<Node>>::Some(stale),payload:u8(1)};"));
    CHECK(test_rejected(
        c, "match n@next{None=>{unit},Some(q)=>{loan_read_ptr(q){|r|unit}},}",
        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
    nl_semantic_destroy(c);
    return true;
}
static bool resource(void)
{
    for (size_t k = 0; k < 3; ++k) {
        NLSemanticContext *c = NULL;
        CHECK(setup(&c));
        c->last_value_fact = SIZE_MAX - k;
        CHECK(test_rejected(c, some, TEST_SOURCE, NL_CHECK_RESOURCE_LIMIT,
                            "P3-RESOURCE-LIMIT"));
        nl_semantic_destroy(c);
    }
    NLSemanticContext *c = NULL;
    CHECK(setup(&c));
    c->last_incarnation = SIZE_MAX;
    CHECK(test_rejected(c, some, TEST_SOURCE, NL_CHECK_RESOURCE_LIMIT,
                        "P3-RESOURCE-LIMIT"));
    nl_semantic_destroy(c);
    c = NULL;
    CHECK(setup(&c));
    /* The occurrence-history budget fails inside a private candidate. */
    free(c->occurrences);
    c->occurrences = calloc(NL_SEMANTIC_MAX_ENTRIES, sizeof(*c->occurrences));
    CHECK(c->occurrences != NULL);
    c->occurrence_count = NL_SEMANTIC_MAX_ENTRIES;
    const NLPlaceId before_head = place(c, "head");
    const NLSemanticPlaceView before = c->places[before_head - 1];
    NLSemanticSnapshot counts, after;
    CHECK(nl_semantic_snapshot(c, &counts));
    TestChecked checked = {0};
    CHECK(test_run(c, some, TEST_SOURCE, NL_CHECK_RESOURCE_LIMIT,
                   "P3-RESOURCE-LIMIT", &checked));
    CHECK(nl_semantic_snapshot(c, &after));
    CHECK(counts.values == after.values && counts.places == after.places &&
          counts.scopes == after.scopes && counts.bindings == after.bindings &&
          counts.occurrences == after.occurrences &&
          counts.last_incarnation == after.last_incarnation &&
          counts.last_value_fact == after.last_value_fact &&
          test_place_equal(before, c->places[before_head - 1]));
    test_checked_destroy(&checked);
    nl_semantic_destroy(c);
    c = NULL;
    CHECK(setup(&c));
    const NLPlaceId h = place(c, "head"), l = c->places[h - 1].fixed_fields[0];
    const NLValueId link = c->places[l - 1].current_value;
    c->values[link - 1].carrier = NL_CARRIER_PLACE;
    c->values[link - 1].owner_place = l; /* forbid duplicate owner */
    CHECK(nl_sem_validate(c) == NL_CHECK_INTERNAL_ERROR);
    nl_semantic_destroy(c);
    c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    NLTypeId header;
    CHECK(nl_recursive_header(c, "Incomplete", &header) == NL_CHECK_OK);
    CHECK(!nl_recursive_local_type(c, header) && !nl_fixed_type(c, header));
    nl_semantic_destroy(c);
    return true;
}
static bool failures(const char *witness)
{
    NLSource *s = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(parse_unit(witness, &s, &t));
    const NLSyntaxTree *units[] = {t};
    bool success = false;
    for (fail_at = 0; fail_at < 4096; ++fail_at) {
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
        if (status == NL_CHECK_OK)
            success = true;
        else
            CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
                  test_unchanged(c, &before));
        nl_semantic_destroy(c);
        if (success)
            break;
    }
    CHECK(success);
    nl_syntax_tree_destroy(t);
    nl_source_destroy(s);
    const char *ops[] = {creation, some, "let copy=head@next;", "main()"};
    for (size_t op = 0; op < sizeof(ops) / sizeof(ops[0]); ++op) {
        success = false;
        for (fail_at = 0; fail_at < 4096; ++fail_at) {
            NLSemanticContext *c = NULL;
            CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
            CHECK(register_source(c, op == 3 ? witness : declaration));
            if (op != 3) {
                CHECK(run_ok(
                    c,
                    "let "
                    "tail=Node{next:Option<ptr<Node>>::None,payload:u8(2)};"));
                CHECK(run_ok(c, "let p=loan_read(tail){|r|ptr_from_ref(r)};"));
            }
            if (op == 1 || op == 2) {
                CHECK(run_ok(c, creation));
                CHECK(run_ok(
                    c, some)); /* include Some→Some / owned payload Copy */
            }
            NLParser *p = NULL;
            s = NULL;
            t = NULL;
            CHECK(nl_source_create(ops[op], strlen(ops[op]), "oom", &s) ==
                  NL_SOURCE_OK);
            CHECK(nl_parser_create(s, &p) == NL_PARSE_OK);
            CHECK(nl_parser_parse_source_fragment(p, &t, NULL) == NL_PARSE_OK);
            TestState before;
            CHECK(test_state(c, &before));
            NLCheckedFragment *a = NULL;
            NLCheckDiagnostic d = {0};
            allocation_index = 0;
            injecting = true;
            const NLCheckStatus status =
                nl_semantic_check_source_fragment(c, t, &a, &d);
            injecting = false;
            if (status == NL_CHECK_OK) {
                success = true;
                CHECK(a != NULL && nl_sem_validate(c) == NL_CHECK_OK);
            } else {
                CHECK(status == NL_CHECK_OUT_OF_MEMORY && a == NULL &&
                      test_unchanged(c, &before));
            }
            nl_checked_destroy(a);
            nl_syntax_tree_destroy(t);
            nl_parser_destroy(p);
            nl_source_destroy(s);
            nl_semantic_destroy(c);
            if (success)
                break;
        }
        CHECK(success);
    }
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    char witness[2048];
    FILE *f = fopen(argv[2], "rb");
    if (f == NULL)
        return 2;
    const size_t length = fread(witness, 1, sizeof(witness) - 1, f);
    const bool loaded = !ferror(f) && feof(f);
    fclose(f);
    if (!loaded)
        return 2;
    witness[length] = 0;
    if (strcmp(argv[1], "evidence") == 0)
        return evidence(witness) ? 0 : 1;
    if (strcmp(argv[1], "transitions") == 0)
        return transitions() ? 0 : 1;
    if (strcmp(argv[1], "dependencies") == 0)
        return dependencies() ? 0 : 1;
    if (strcmp(argv[1], "negatives") == 0)
        return negatives() ? 0 : 1;
    if (strcmp(argv[1], "liveness") == 0)
        return liveness() ? 0 : 1;
    if (strcmp(argv[1], "resource") == 0)
        return resource() ? 0 : 1;
    if (strcmp(argv[1], "failures") == 0)
        return failures(witness) ? 0 : 1;
    return 2;
}
