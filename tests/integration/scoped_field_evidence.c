/* P297 read-only public source -> owned evidence, plus sampled rollback/OOM.
 * No private headers or seeded root/Allocation/Domain state. */
#include "../support/test.h"
#include "newlang/captured_closure.h"
#include "newlang/parser.h"
#include "newlang/raw_storage.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocations, failure = SIZE_MAX;
static bool refuse(void)
{
    return injecting && allocations++ == failure;
}
void *__wrap_malloc(size_t n)
{
    return refuse() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return refuse() ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return refuse() ? NULL : __real_realloc(p, n);
}

static NLCheckedNodeId allocation_match(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        const NLCheckedNodeView *a = nl_checked_node_view(f, n->initializer);
        if (n->kind == NL_CHECKED_MATCH && a &&
            a->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
            return i;
    }
    return 0;
}
static bool inventory(const NLSemanticContext *c, NLValueId as[5],
                      NLValueId ds[5])
{
    NLSemanticSnapshot snap;
    CHECK(nl_semantic_snapshot(c, &snap) && snap.backing_regions == 5 &&
          snap.domains == 5);
    memset(as, 0, 5 * sizeof(*as));
    memset(ds, 0, 5 * sizeof(*ds));
    for (size_t i = 1; i <= snap.values; ++i) {
        NLSemanticValueView v;
        NLSemanticTypeView t;
        CHECK(nl_semantic_value_view(c, i, &v) &&
              nl_semantic_type_view(c, v.type, &t));
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        if (t.kind == NL_TYPE_ALLOCATION) {
            CHECK(v.allocation_region >= 1 && v.allocation_region <= 5 &&
                  !as[v.allocation_region - 1]);
            as[v.allocation_region - 1] = i;
        }
        if (v.type == nl_semantic_domain_type(c)) {
            CHECK(v.domain >= 1 && v.domain <= 5 && !ds[v.domain - 1]);
            ds[v.domain - 1] = i;
        }
    }
    for (size_t i = 0; i < 5; ++i) {
        NLSemanticBackingView r;
        NLSemanticDomainView d;
        CHECK(as[i] && ds[i] && nl_semantic_backing_view(c, i + 1, &r) &&
              r.live && nl_semantic_domain_view(c, i + 1, &d) && d.live &&
              d.value == ds[i]);
    }
    return true;
}
static bool inspect(const NLCheckedFragment *entry)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    CHECK(f);
    for (size_t i = 0; i < 5; ++i) {
        NLCheckedNodeId m = allocation_match(f);
        CHECK(m && nl_checked_captured_closure_validate(f, m) == NL_CHECK_OK);
        f = nl_checked_match_arm(f, m, 1);
        CHECK(f);
    }
    CHECK(nl_checked_captured_change_count(f) == 6);
    NLScopedFieldCallView w = {0};
    NLCheckedNodeId call = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_scoped_field_call_view(f, i, &w)) {
            CHECK(!call);
            call = i;
        }
    CHECK(call &&
          nl_checked_scoped_field_call_validate(f, call) == NL_CHECK_OK &&
          nl_checked_captured_change_count(w.body) == 1);
    const NLCheckedNodeView *write =
        nl_checked_node_view(w.body, w.change.operation);
    const NLCheckedNodeView *projection = nl_checked_node_view(f, w.projection);
    const size_t index = projection ? projection->field.index : 0;
    CHECK(write && projection && write->kind == NL_CHECKED_REPLACE &&
          projection->kind == NL_CHECKED_FIELD_REF && index <= 1 &&
          write->field.index == index &&
          write->field.nominal == projection->field.nominal &&
          write->field.index == projection->field.index &&
          write->field.child == projection->field.child);
    NLSemanticPlaceView root[2], field[2], prev[2];
    NLSemanticValueView value[2], payload[2];
    NLSemanticOccurrenceView old_occ, ended_occ, new_occ;
    const NLSemanticContext *worlds[] = {w.change.before, w.change.after};
    NLValueId as[2][5], ds[2][5];
    for (size_t i = 0; i < 2; ++i) {
        CHECK(
            inventory(worlds[i], as[i], ds[i]) &&
            nl_semantic_place_view(worlds[i], write->field.parent, &root[i]) &&
            root[i].live &&
            nl_semantic_place_view(worlds[i], write->field.child, &field[i]) &&
            field[i].live && root[i].fixed_field_count == 4 &&
            root[i].fixed_fields[index] == write->field.child &&
            root[i].fixed_fields[0] != root[i].fixed_fields[1] &&
            nl_semantic_place_view(worlds[i], root[i].fixed_fields[1 - index],
                                   &prev[i]) &&
            prev[i].live &&
            nl_semantic_value_view(worlds[i], field[i].current_value,
                                   &value[i]));
        if (value[i].variant == 2)
            CHECK(nl_semantic_value_view(worlds[i], value[i].sum_payload,
                                         &payload[i]));
    }
    CHECK(!memcmp(as[0], as[1], sizeof(as[0])) &&
          !memcmp(ds[0], ds[1], sizeof(ds[0])) &&
          root[0].placement.region == 2 && root[0].governing_domain == 2 &&
          root[0].incarnation == root[1].incarnation &&
          root[0].governing_domain == root[1].governing_domain &&
          !memcmp(&root[0].placement, &root[1].placement,
                  sizeof(root[0].placement)) &&
          field[0].incarnation == field[1].incarnation &&
          field[0].current_fact != field[1].current_fact &&
          root[0].current_fact != root[1].current_fact &&
          value[0].variant == 2 &&
          field[0].current_value == write->field.old_value &&
          field[1].current_value == write->field.new_value &&
          field[0].current_value != field[1].current_value &&
          field[0].payload_occurrence &&
          nl_semantic_occurrence_view(worlds[0], field[0].payload_occurrence,
                                      &old_occ) &&
          old_occ.live &&
          nl_semantic_occurrence_view(worlds[1], field[0].payload_occurrence,
                                      &ended_occ) &&
          !ended_occ.live && !memcmp(&prev[0], &prev[1], sizeof(prev[0])) &&
          prev[0].payload_occurrence);
    if (value[1].variant == 2)
        CHECK(field[1].payload_occurrence &&
              field[0].payload_occurrence != field[1].payload_occurrence &&
              nl_semantic_occurrence_view(
                  worlds[1], field[1].payload_occurrence, &new_occ) &&
              new_occ.live);
    else
        CHECK(value[1].variant == 1 && !field[1].payload_occurrence);
    NLSemanticPlaceView old_payload_before, old_payload_after;
    CHECK(nl_semantic_place_view(worlds[0], old_occ.payload_place,
                                 &old_payload_before) &&
          old_payload_before.live && old_payload_before.current_fact &&
          nl_semantic_place_view(worlds[1], old_occ.payload_place,
                                 &old_payload_after) &&
          !old_payload_after.live && !old_payload_after.current_fact &&
          !old_payload_after.current_value);
    NLSemanticPlaceView old_target, new_target = {0};
    CHECK(nl_semantic_place_view(worlds[0], payload[0].reference.place,
                                 &old_target) &&
          old_target.placement.region == (index ? 4 : 3));
    if (value[1].variant == 2)
        CHECK(nl_semantic_place_view(worlds[1], payload[1].reference.place,
                                     &new_target) &&
              (new_target.placement.region == 3 ||
               new_target.placement.region == 4));
    NLSemanticScopeView scope;
    CHECK(nl_semantic_scope_view(worlds[0], projection->reference_result.scope,
                                 &scope) &&
          scope.active &&
          nl_semantic_scope_view(nl_checked_context(f),
                                 projection->reference_result.scope, &scope) &&
          !scope.active);
    size_t frees = 0;
    bool released[5] = {false};
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind != NL_CHECKED_DEALLOCATE)
            continue;
        const NLCheckedNodeView *a = nl_checked_node_view(f, n->first_argument);
        const NLCheckedNodeView *r =
            a ? nl_checked_node_view(f, a->next_argument) : NULL;
        NLSemanticValueView av, rv;
        CHECK(a && r && a->result_count == 1 && r->result_count == 1 &&
              nl_semantic_value_view(nl_checked_context(f), a->results[0].value,
                                     &av) &&
              nl_semantic_value_view(nl_checked_context(f), r->results[0].value,
                                     &rv) &&
              av.allocation_region == rv.occupancy.region &&
              rv.occupancy.start == 0 && rv.occupancy.length &&
              av.allocation_region >= 1 && av.allocation_region <= 5 &&
              !released[av.allocation_region - 1]);
        released[av.allocation_region - 1] = true;
        ++frees;
    }
    CHECK(frees == 5);
    printf(
        "{\"owned_source_artifact\":true,\"closure_validators\":5,\"scoped_"
        "call_validator\":true,"
        "\"caller_changes\":6,\"callee_changes\":1,\"ProjectionId_nominal\":"
        "%zu,\"ProjectionId_index\":%zu,"
        "\"root\":%zu,\"root_incarnation\":%zu,\"field\":%zu,\"field_"
        "incarnation\":%zu,"
        "\"fact_before\":%zu,\"fact_after\":%zu,\"old_occurrence\":%zu,"
        "\"new_occurrence\":%zu,"
        "\"old_ptr_region\":%zu,\"new_ptr_region\":%zu,"
        "\"root_fact_before\":%zu,\"root_fact_after\":%zu,"
        "\"Allocation_values\":[%zu,%zu,%zu,%zu,%zu],\"Domain_values\":[%zu,"
        "%zu,%zu,%zu,%zu],"
        "\"old_payload_place\":%zu,\"old_payload_current_cleared\":true,"
        "\"sibling_current_preserved\":true,\"original_A_D_preserved_unique\":"
        "true,"
        "\"scoped_ref_ended\":true,\"matched_frees\":5,\"native_execution\":"
        "false}\n",
        write->field.nominal, write->field.index, write->field.parent,
        root[0].incarnation, write->field.child, field[0].incarnation,
        field[0].current_fact, field[1].current_fact,
        field[0].payload_occurrence, field[1].payload_occurrence,
        old_target.placement.region, new_target.placement.region,
        root[0].current_fact, root[1].current_fact, as[0][0], as[0][1],
        as[0][2], as[0][3], as[0][4], ds[0][0], ds[0][1], ds[0][2], ds[0][3],
        ds[0][4], old_occ.payload_place);
    return true;
}
static bool sample_validator_oom(const NLCheckedFragment *entry)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    for (size_t i = 0; i < 5; ++i)
        f = nl_checked_match_arm(f, allocation_match(f), 1);
    NLCheckedNodeId call = 0;
    NLScopedFieldCallView view;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_scoped_field_call_view(f, i, &view))
            call = i;
    CHECK(call);
    size_t total = 0;
    for (size_t test = 0; test < 8; ++test) {
        const size_t points[] = {0,
                                 1,
                                 2,
                                 total / 4,
                                 total / 2,
                                 total * 3 / 4,
                                 total ? total - 1 : 0};
        NLSemanticSnapshot before, after;
        CHECK(nl_semantic_snapshot(nl_checked_context(f), &before));
        allocations = 0;
        failure = test ? points[test - 1] : SIZE_MAX;
        injecting = true;
        const NLCheckStatus status =
            nl_checked_scoped_field_call_validate(f, call);
        injecting = false;
        if (!test) {
            total = allocations;
            CHECK(status == NL_CHECK_OK && total > 8);
        } else {
            CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
                  nl_semantic_snapshot(nl_checked_context(f), &after) &&
                  !memcmp(&before, &after, sizeof(before)) &&
                  nl_checked_scoped_field_call_validate(f, call) ==
                      NL_CHECK_OK);
        }
    }
    printf("{\"oom_stage\":2,\"baseline_allocations\":%zu,\"injected_"
           "failures\":7,\"rollback_retry\":true}\n",
           total);
    return true;
}

static bool sample_oom(const NLSyntaxTree *syntax,
                       const NLSyntaxTree *entry_syntax)
{
    const NLSyntaxTree *units[] = {syntax};
    for (size_t stage = 0; stage < 2; ++stage) {
        size_t total = 0;
        for (size_t test = 0; test < 8; ++test) {
            NLSemanticContext *c = NULL;
            NLCheckedFragment *entry = NULL;
            NLFunctionUnitDiagnostic d = {0};
            NLCheckDiagnostic e = {0};
            CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
            if (stage)
                CHECK(nl_semantic_register_function_unit(c, units, 1, &d) ==
                      NL_CHECK_OK);
            NLSemanticSnapshot before, after;
            CHECK(nl_semantic_snapshot(c, &before));
            allocations = 0;
            const size_t points[] = {0,
                                     1,
                                     2,
                                     total / 4,
                                     total / 2,
                                     total * 3 / 4,
                                     total ? total - 1 : 0};
            failure = test ? points[test - 1] : SIZE_MAX;
            injecting = true;
            NLCheckStatus status =
                stage
                    ? nl_semantic_check_expression(c, entry_syntax, &entry, &e)
                    : nl_semantic_register_function_unit(c, units, 1, &d);
            injecting = false;
            if (!test) {
                total = allocations;
                CHECK(status == NL_CHECK_OK && total > 8);
            } else {
                CHECK(status == NL_CHECK_OUT_OF_MEMORY && !entry &&
                      nl_semantic_snapshot(c, &after) &&
                      !memcmp(&before, &after, sizeof(before)));
                if (stage)
                    CHECK(nl_semantic_check_expression(c, entry_syntax, &entry,
                                                       &e) == NL_CHECK_OK &&
                          entry);
                else
                    CHECK(nl_semantic_register_function_unit(c, units, 1, &d) ==
                          NL_CHECK_OK);
            }
            nl_checked_destroy(entry);
            nl_semantic_destroy(c);
        }
        printf("{\"oom_stage\":%zu,\"baseline_allocations\":%zu,\"injected_"
               "failures\":7,\"rollback_retry\":true}\n",
               stage, total);
    }
    return true;
}
static bool run(const char *path, bool oom)
{
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *syntax = NULL, *entry_syntax = NULL;
    NLSemanticContext *c = NULL;
    NLCheckedFragment *entry = NULL;
    bool ok = false;
    if (nl_source_load(path, &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(parser, &syntax, NULL) != NL_PARSE_OK ||
        nl_semantic_create(&c) != NL_CHECK_OK)
        goto done;
    const NLSyntaxTree *units[] = {syntax};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckDiagnostic e = {0};
    if (nl_semantic_register_function_unit(c, units, 1, &d) != NL_CHECK_OK) {
        (void)nl_check_diagnostic_render(stderr, source, &d.diagnostic);
        goto done;
    }
    if (nl_source_create("main()", 6, "<P297-entry>", &entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(entry_parser, &entry_syntax,
                                            NULL) != NL_PARSE_OK ||
        nl_semantic_check_expression(c, entry_syntax, &entry, &e) !=
            NL_CHECK_OK ||
        !entry)
        goto done;
    ok = inspect(entry) && (!oom || (sample_oom(syntax, entry_syntax) &&
                                     sample_validator_oom(entry)));
done:
    nl_checked_destroy(entry);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(entry_syntax);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_syntax_tree_destroy(syntax);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok;
}
int main(int argc, char **argv)
{
    return (argc == 2 || (argc == 3 && !strcmp(argv[2], "oom"))) &&
                   run(argv[1], argc == 3)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
