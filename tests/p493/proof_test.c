/* Only real source-issued contexts/artifacts and public views. No seeds. */
#include "../support/node_checked.h"
#include "newlang/ordinary_call.h"
#include "readonly_snapshot.h"
#include <stdlib.h>
#include <time.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && allocation_index++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && allocation_index++ == fail_at ? NULL
                                                      : __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t size)
{
    return injecting && allocation_index++ == fail_at ? NULL
                                                      : __real_calloc(n, size);
}

static bool validator_oom(const NLCheckedFragment *f, NLCheckedNodeId call,
                          const NLOrdinaryCallView *v)
{
    const NLSemanticContext *worlds[] = {
        v->entry, v->before_change, v->after_change,
        v->post,  v->closed_loan,   nl_checked_context(f)};
    TestState *states = calloc(6, sizeof(*states));
    CHECK(states);
    for (size_t i = 0; i < 6; ++i)
        CHECK(readonly_state(worlds[i], &states[i]));
    bool success = false;
    for (fail_at = 0; fail_at < 4096; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status = nl_checked_ordinary_call_validate(f, call, NULL);
        injecting = false;
        CHECK(status == NL_CHECK_OUT_OF_MEMORY || status == NL_CHECK_OK);
        for (size_t i = 0; i < 6; ++i)
            CHECK(readonly_unchanged(worlds[i], &states[i]));
        if (status == NL_CHECK_OK) {
            success = true;
            break;
        }
    }
    CHECK(success);
    printf("validator OOM positions=%zu; all five snapshots + public context "
           "unchanged\n",
           fail_at);
    free(states);
    return true;
}

static bool issuance_oom(const char *path)
{
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL, *entry = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {unit};
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    parser = NULL;
    CHECK(nl_source_create("main()", 6, "real-source-main-entry",
                           &entry_source) == NL_SOURCE_OK &&
          nl_parser_create(entry_source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_expression_fragment(parser, &entry, NULL) ==
              NL_PARSE_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(c, before));
    NLCheckedFragment *owned = NULL;
    bool success = false;
    for (fail_at = 0; fail_at < 8192; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_check_expression(c, entry, &owned, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            success = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && !owned &&
              test_unchanged(c, before));
    }
    CHECK(success && owned);
    const NLCheckedFragment *body =
        nl_checked_call_body(owned, nl_checked_root(owned));
    NLOrdinaryCallView view;
    size_t proofs = 0;
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i)
        if (nl_checked_ordinary_call_view(body, i, &view)) {
            CHECK(nl_checked_ordinary_call_validate(body, i, &view) ==
                  NL_CHECK_OK);
            ++proofs;
        }
#ifdef P493_EXPECT_NO_PROOF
    CHECK(proofs == 0);
#else
    CHECK(proofs == 1);
#endif
    printf("actual-call issuance OOM positions=%zu; failure output NULL/public "
           "state rolled back\n",
           fail_at);
    nl_checked_destroy(owned);
    free(before);
    nl_syntax_tree_destroy(entry);
    nl_parser_destroy(parser);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(c);
    return true;
}

static bool inspect(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f);
    NLOrdinaryCallView v = {0};
    size_t count = 0;
    NLCheckedNodeId call = 0;
    size_t actual_calls = 0;
    NLCheckedNodeId actual_call = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_REGISTERED_CALL) {
            CHECK(nl_checked_call_body(f, i));
            actual_call = i;
            ++actual_calls;
        }
        if (nl_checked_ordinary_call_view(f, i, &v)) {
            ++count;
            call = i;
        }
    }
    CHECK(actual_calls == 1);
    printf("issued certificates=%zu call=%zu\n", count, call);
#ifdef P493_EXPECT_NO_PROOF
    CHECK(count == 0);
    CHECK(nl_checked_ordinary_call_validate(f, actual_call, NULL) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    node_checked_destroy(&n);
    puts("DEFAULT OFF: real source CHECK/owned pass, ordinary-call proof "
         "absent");
    CHECK(issuance_oom(path));
    return true;
#else
    CHECK(count == 1 && call == actual_call && v.caller == f &&
          v.body == nl_checked_call_body(f, call));
#endif
    const NLSemanticContext *protected_worlds[] = {
        v.entry, v.before_change, v.after_change,
        v.post,  v.closed_loan,   nl_checked_context(f)};
    TestState *protected_states = calloc(6, sizeof(*protected_states));
    CHECK(protected_states);
    for (size_t i = 0; i < 6; ++i)
        CHECK(readonly_state(protected_worlds[i], &protected_states[i]));
    NLCheckStatus s = nl_checked_ordinary_call_validate(f, call, NULL);
    printf("public validator status=%d\n", (int)s);
    CHECK(s == NL_CHECK_OK &&
          nl_checked_ordinary_call_validate(f, call, &v) == NL_CHECK_OK);
    NLSemanticPlaceView root, field, closed, other;
    CHECK(nl_semantic_place_view(v.entry, v.root, &root) &&
          nl_semantic_place_view(v.entry, v.field, &field) &&
          nl_semantic_place_view(v.closed_loan, v.field, &closed));
    NLSemanticValueView returned, incoming, locator;
    CHECK(
        nl_semantic_value_view(v.post, v.returned, &returned) &&
        returned.variant == 1 && returned.carrier == NL_CARRIER_LOOSE &&
        nl_semantic_value_view(v.closed_loan, closed.current_value,
                               &incoming) &&
        incoming.variant == 2 && incoming.sum_payload &&
        nl_semantic_value_view(v.closed_loan, incoming.sum_payload, &locator) &&
        locator.reference.place == v.root &&
        locator.reference.incarnation == root.incarnation &&
        locator.reference.scope == 0 && locator.domain == 0 &&
        locator.allocation_region == 0);
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticPlaceView before, after;
        CHECK(nl_semantic_place_view(v.entry, v.originals[i], &before) &&
              nl_semantic_place_view(v.closed_loan, v.originals[i], &after) &&
              before.live && after.live && before.implicit_local &&
              before.incarnation == after.incarnation &&
              before.governing_domain == 0 && after.governing_domain == 0 &&
              before.placement.region == 0 && after.placement.region == 0);
        for (size_t j = 0; j < 2; ++j) {
            if (before.fixed_fields[j] == v.field)
                continue;
            NLSemanticPlaceView a, b;
            CHECK(nl_semantic_place_view(v.entry, before.fixed_fields[j], &a) &&
                  nl_semantic_place_view(v.closed_loan, after.fixed_fields[j],
                                         &b) &&
                  a.live && b.live && a.incarnation == b.incarnation &&
                  a.current_fact == b.current_fact &&
                  a.current_value == b.current_value &&
                  a.payload_occurrence == b.payload_occurrence);
        }
    }
    NLSemanticBindingView parameter;
    NLSemanticPlaceView parameter_place;
    NLSemanticOccurrenceView occurrence;
    CHECK(nl_semantic_binding_view(v.before_change, v.parameters[1],
                                   &parameter) &&
          nl_semantic_place_view(v.before_change, parameter.place,
                                 &parameter_place) &&
          parameter_place.payload_occurrence &&
          nl_semantic_occurrence_view(v.before_change,
                                      parameter_place.payload_occurrence,
                                      &occurrence) &&
          occurrence.live &&
          nl_semantic_occurrence_view(
              v.post, parameter_place.payload_occurrence, &occurrence) &&
          !occurrence.live &&
          nl_semantic_occurrence_view(v.closed_loan, closed.payload_occurrence,
                                      &occurrence) &&
          occurrence.live);
    printf("originals/siblings preserved; formal Some occurrence=%zu ends; "
           "original field occurrence=%zu live\n",
           parameter_place.payload_occurrence, closed.payload_occurrence);
    printf(
        "QUALIFIED call=%zu child_op=%zu loan=%zu root=%zu/%zu field=%zu/%zu "
        "scope=%zu F=%zu->%zu RF=%zu->%zu occurrence=%zu->%zu old=%zu\n",
        call, v.operation, v.loan, v.root, root.incarnation, v.field,
        field.incarnation, v.reference.scope, field.current_fact,
        closed.current_fact, root.current_fact,
        nl_checked_node_view(v.body, v.operation)->field.parent_post_fact,
        field.payload_occurrence, closed.payload_occurrence, v.returned);
    NLOrdinaryCallView bad = v;
    bad.root = v.originals[0] == v.root ? v.originals[1] : v.originals[0];
    CHECK(nl_semantic_place_view(v.entry, bad.root, &other));
    bad.field = other.fixed_fields[0];
    CHECK(nl_semantic_place_view(v.entry, bad.field, &other) &&
          other.type == field.type);
    bad.reference.place = bad.field;
    bad.reference.incarnation = other.incarnation;
    CHECK(nl_checked_ordinary_call_validate(f, call, &bad) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    bad = v;
    bad.reference.scope = 0;
    CHECK(nl_checked_ordinary_call_validate(f, call, &bad) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    bad = v;
    ++bad.reference.incarnation;
    CHECK(nl_checked_ordinary_call_validate(f, call, &bad) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    bad = v;
    bad.entry = v.post;
    CHECK(nl_checked_ordinary_call_validate(f, call, &bad) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(nl_checked_ordinary_call_validate(f, call, &v) == NL_CHECK_OK);
    for (size_t i = 0; i < 6; ++i)
        CHECK(readonly_unchanged(protected_worlds[i], &protected_states[i]));
    free(protected_states);
    NLSemanticScopeView active, ended;
    CHECK(nl_semantic_scope_view(v.entry, v.reference.scope, &active) &&
          active.active &&
          nl_semantic_scope_view(v.post, v.reference.scope, &active) &&
          active.active &&
          nl_semantic_scope_view(v.closed_loan, v.reference.scope, &ended) &&
          !ended.active);
    printf("scope=%zu: active entry/call-post, ended at authentic loan-close; "
           "no escaped ref\n",
           v.reference.scope);
    const NLSemanticContext *worlds[] = {v.entry, v.before_change,
                                         v.after_change, v.post, v.closed_loan};
    for (size_t i = 0; i < 5; ++i) {
        NLSemanticSnapshot snap;
        CHECK(nl_semantic_snapshot(worlds[i], &snap));
        printf("CHECKPOINT %zu bindings=%zu values=%zu places=%zu scopes=%zu "
               "occurrences=%zu\n",
               i, snap.bindings, snap.values, snap.places, snap.scopes,
               snap.occurrences);
    }
    struct timespec begin, end;
    CHECK(timespec_get(&begin, TIME_UTC) == TIME_UTC);
    for (size_t i = 0; i < 20; ++i)
        CHECK(nl_checked_ordinary_call_validate(f, call, &v) == NL_CHECK_OK);
    CHECK(timespec_get(&end, TIME_UTC) == TIME_UTC);
    double seconds = (double)(end.tv_sec - begin.tv_sec) +
                     (double)(end.tv_nsec - begin.tv_nsec) / 1e9;
    printf("20 independent validations seconds=%.6f (host diagnostic, not "
           "benchmark)\n",
           seconds);
    CHECK(validator_oom(f, call, &v));
    puts("PUBLIC SOURCE-ISSUED ORDINARY CALL VALID; wrong-root/scope/stale "
         "rejected; no C17/native claim");
    node_checked_destroy(&n);
    CHECK(issuance_oom(path));
    return true;
}
static bool control(const char *path, NLCheckStatus expected, const char *code)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(c, before));
    const NLSyntaxTree *inputs[] = {unit};
    NLFunctionUnitDiagnostic d = {0};
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, &d) == expected &&
          d.diagnostic.diagnostic.code &&
          strcmp(d.diagnostic.diagnostic.code, code) == 0 &&
          test_unchanged(c, before));
    printf("CONTROL %s: %s; semantic registration rollback unchanged\n", path,
           code);
    free(before);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 4 && inspect(argv[1]) &&
                   control(argv[2], NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                           "P9-CONTINUATION-PRECISION") &&
                   control(argv[3], NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH")
               ? 0
               : 1;
}
