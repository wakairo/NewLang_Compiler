/* Issue 289 read-only actual-source evidence. No private headers, injected
 * heap/domain facts, name-based grants, or new matched bits. */
#include "../support/test.h"
#include "newlang/captured_closure.h"
#include "newlang/checked_c_node.h"
#include "newlang/parser.h"
#include "newlang/raw_storage.h"
#include <stdlib.h>
#include <string.h>

static NLCheckedNodeId allocation_match(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        const NLCheckedNodeView *init = nl_checked_node_view(f, v->initializer);
        if (v->kind == NL_CHECKED_MATCH && init &&
            init->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
            return i;
    }
    return 0;
}

static bool current_with_result(const NLSemanticContext *c, NLValueId id,
                                NLValueId result)
{
    for (size_t depth = 0; depth < 18; ++depth) {
        NLSemanticValueView v;
        CHECK(nl_semantic_value_view(c, id, &v));
        if (v.carrier == NL_CARRIER_LOOSE)
            return result && id == result;
        if (v.carrier == NL_CARRIER_PLACE) {
            NLSemanticPlaceView p;
            NLSemanticSnapshot snap;
            CHECK(nl_semantic_place_view(c, v.owner_place, &p) && p.live &&
                  p.current_value == id && nl_semantic_snapshot(c, &snap));
            size_t count = 0;
            for (size_t i = 1; i <= snap.bindings; ++i) {
                NLSemanticBindingView b;
                CHECK(nl_semantic_binding_view(c, i, &b));
                count += b.availability == NL_AVAILABLE && b.value == id &&
                         b.place == v.owner_place;
            }
            CHECK(count == 1);
            return true;
        }
        CHECK(v.carrier == NL_CARRIER_AGGREGATE && v.aggregate_owner);
        NLSemanticValueView parent;
        CHECK(nl_semantic_value_view(c, v.aggregate_owner, &parent));
        size_t count = 0;
        for (size_t i = 0; i < parent.field_count; ++i)
            count += parent.fields[i] == id;
        CHECK(count == 1);
        id = v.aggregate_owner;
    }
    return false;
}

static bool current(const NLSemanticContext *c, NLValueId id)
{
    return current_with_result(c, id, 0);
}

static bool packet(const NLSemanticContext *c, NLValueId id, size_t *r,
                   size_t *a, size_t *d, NLPlaceId *root, NLIncarnationId *inc)
{
    NLSemanticValueView v, p, av, dv;
    NLSemanticPlaceView place;
    CHECK(nl_semantic_value_view(c, id, &v) && v.field_count == 3 &&
          nl_semantic_value_view(c, v.fields[0], &p) &&
          nl_semantic_value_view(c, v.fields[1], &av) &&
          nl_semantic_value_view(c, v.fields[2], &dv));
    CHECK(p.reference.provenance == NL_PROVENANCE_VALID &&
          p.reference.scope == 0 &&
          nl_semantic_place_view(c, p.reference.place, &place) && place.live &&
          place.independent_root &&
          place.incarnation == p.reference.incarnation);
    CHECK(place.governing_domain == dv.domain && av.allocation_region &&
          current(c, v.fields[1]) && current(c, v.fields[2]));
    *r = place.placement.region;
    *a = av.allocation_region;
    *d = dv.domain;
    *root = p.reference.place;
    *inc = p.reference.incarnation;
    return true;
}

static bool releases(const NLCheckedFragment *f, size_t order[5], size_t *count,
                     size_t *terminals, size_t depth)
{
    CHECK(depth < 4);
    const NLSemanticContext *c = nl_checked_context(f);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_DEALLOCATE) {
            const NLCheckedNodeView *a =
                nl_checked_node_view(f, n->first_argument);
            const NLCheckedNodeView *raw =
                a ? nl_checked_node_view(f, a->next_argument) : NULL;
            NLSemanticValueView av, rv;
            CHECK(*count < 5 && a && raw && a->result_count == 1 &&
                  raw->result_count == 1 &&
                  nl_semantic_value_view(c, a->results[0].value, &av) &&
                  nl_semantic_value_view(c, raw->results[0].value, &rv));
            CHECK(av.allocation_region == rv.occupancy.region &&
                  rv.occupancy.start == 0 && rv.occupancy.length);
            order[(*count)++] = av.allocation_region;
        }
        if (n->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(f, i);
            CHECK(body);
            if (n->type == nl_semantic_unit_type(c)) {
                NLTwoRootDefinition nested;
                if (!nl_semantic_two_root_definition(c, n->function, &nested)) {
                    NLTypedOwnerDefinition definition;
                    CHECK(nl_semantic_function_applicability(c, n->function,
                                                             &definition) &&
                          definition.definition_checked &&
                          definition.requirements ==
                              NL_OWNER_ALL_REQUIREMENTS &&
                          definition.step_count == 4);
                    for (size_t s = 0; s < 4; ++s)
                        CHECK(definition.steps[s] == (NLTypedOwnerStep)(s + 1));
                    ++*terminals;
                } else
                    CHECK(nested.definition_checked && nested.count == 2);
            }
            CHECK(releases(body, order, count, terminals, depth + 1));
        }
    }
    return true;
}

static bool collect(const NLSemanticContext *c, NLValueId id,
                    NLValueId leaves[4], size_t *count, size_t depth)
{
    NLSemanticValueView v;
    NLSemanticTypeView t;
    CHECK(depth < 18 && nl_semantic_value_view(c, id, &v) &&
          nl_semantic_type_view(c, v.type, &t) && !t.is_copy &&
          !t.is_discardable);
    NLAggregateField a, d;
    NLSemanticTypeView p;
    if (v.field_count == 3 &&
        nl_semantic_aggregate_field_view(c, v.type, 0, &a) &&
        nl_semantic_type_view(c, a.type, &p) && p.kind == NL_TYPE_PTR &&
        nl_semantic_aggregate_field_view(c, v.type, 1, &a) &&
        nl_semantic_aggregate_field_view(c, v.type, 2, &d) &&
        a.type == nl_semantic_core_type(c, NL_TYPE_ALLOCATION) &&
        d.type == nl_semantic_domain_type(c)) {
        CHECK(*count < 4);
        leaves[(*count)++] = id;
        return true;
    }
    CHECK(v.field_count && v.field_count <= 4);
    for (size_t i = 0; i < v.field_count; ++i)
        CHECK(collect(c, v.fields[i], leaves, count, depth + 1));
    return true;
}
static bool inspect(const NLCheckedFragment *entry, const char *expectation)
{
    const NLCheckedFragment *top =
        nl_checked_call_body(entry, nl_checked_root(entry));
    const NLCheckedFragment *f = top;
    NLCapturedClosureView original = {0};
    for (size_t i = 0; i < 5; ++i) {
        NLCheckedNodeId m = allocation_match(f);
        CHECK(m && nl_checked_captured_closure_validate(f, m) == NL_CHECK_OK);
        if (i == 4)
            CHECK(nl_checked_captured_closure_view(f, m, &original) &&
                  original.count == 4);
        f = nl_checked_match_arm(f, m, 1);
        CHECK(f);
    }
    NLWholeValueCallView w = {0};
    size_t calls = 0, four_construct = 0, four_consume = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        NLWholeValueCallView candidate;
        if (nl_checked_whole_value_call_view(f, i, &candidate)) {
            w = candidate;
            ++calls;
        }
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        NLSemanticTypeView t = {0};
        const bool owner =
            n->type &&
            nl_semantic_type_view(nl_checked_context(f), n->type, &t) &&
            !t.is_copy && !t.is_discardable;
        four_construct +=
            owner && n->kind == NL_CHECKED_AGGREGATE && n->argument_count == 4;
        four_consume +=
            n->kind == NL_CHECKED_AGGREGATE_BINDING && n->argument_count == 4;
    }
    CHECK(calls == 1 && w.count == 3 && four_construct == 1 &&
          four_consume == 1);
    NLValueId leaves[4] = {0};
    size_t count = 0;
    CHECK(collect(w.received, w.result, leaves, &count, 0) && count == 2);
    NLSemanticValueView returned, received;
    CHECK(nl_semantic_value_view(w.returned, w.result, &returned) &&
          nl_semantic_value_view(w.received, w.result, &received) &&
          returned.carrier == NL_CARRIER_LOOSE &&
          received.carrier == NL_CARRIER_PLACE &&
          !memcmp(returned.fields, received.fields, sizeof(returned.fields)));
    NLSemanticBindingView receiver;
    CHECK(nl_semantic_binding_view(w.received, w.receiver, &receiver) &&
          receiver.availability == NL_AVAILABLE);
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticBindingView donor, moved, param;
        CHECK(nl_semantic_binding_view(w.entry, w.donors[i], &donor) &&
              donor.availability == NL_AVAILABLE &&
              donor.value == w.inputs[i] &&
              nl_semantic_binding_view(w.returned, w.donors[i], &moved) &&
              moved.availability == NL_CONSUMED &&
              nl_semantic_binding_view(w.returned, w.parameters[i], &param) &&
              param.availability == NL_CONSUMED && param.value == w.inputs[i] &&
              donor.place != param.place && param.place != receiver.place &&
              current(w.entry, w.inputs[i]));
    }
    NLSemanticBindingView token, token_after, formal;
    NLSemanticValueView copy[3];
    NLSemanticTypeView copy_type;
    CHECK(nl_semantic_binding_view(w.entry, w.donors[2], &token) &&
          token.availability == NL_AVAILABLE &&
          nl_semantic_binding_view(w.received, w.donors[2], &token_after) &&
          token_after.availability == NL_AVAILABLE &&
          token_after.value == token.value &&
          nl_semantic_binding_view(w.returned, w.parameters[2], &formal) &&
          formal.value == w.inputs[2] && formal.availability == NL_CONSUMED &&
          nl_semantic_value_view(w.entry, token.value, &copy[0]) &&
          nl_semantic_value_view(w.returned, w.inputs[2], &copy[1]) &&
          nl_semantic_value_view(w.received, w.inputs[2], &copy[2]) &&
          nl_semantic_type_view(w.entry, token.type, &copy_type) &&
          copy_type.kind == NL_TYPE_PTR && copy_type.is_copy &&
          copy_type.is_discardable &&
          !memcmp(&copy[0].reference, &copy[1].reference,
                  sizeof(copy[0].reference)) &&
          !memcmp(&copy[1].reference, &copy[2].reference,
                  sizeof(copy[0].reference)) &&
          !copy[1].allocation_region && !copy[1].domain &&
          !copy[1].reference.scope);
    size_t regions[4], allocations[4], domains[4];
    NLPlaceId roots[4];
    NLIncarnationId inc[4];
    bool mixed = false;
    for (size_t i = 0; i < count; ++i) {
        CHECK(packet(w.received, leaves[i], &regions[i], &allocations[i],
                     &domains[i], &roots[i], &inc[i]));
        mixed |= regions[i] != allocations[i];
        NLSemanticValueView leaf[3];
        const NLSemanticContext *worlds[] = {w.entry, w.returned, w.received};
        for (size_t j = 0; j < 3; ++j)
            CHECK(nl_semantic_value_view(worlds[j], leaves[i], &leaf[j]));
        CHECK(!memcmp(leaf[0].fields, leaf[1].fields, sizeof(leaf[0].fields)) &&
              !memcmp(leaf[0].fields, leaf[2].fields, sizeof(leaf[0].fields)));
        for (size_t k = 0; k < 3; ++k) {
            NLSemanticValueView v[3];
            for (size_t j = 0; j < 3; ++j)
                CHECK(nl_semantic_value_view(worlds[j], leaf[j].fields[k],
                                             &v[j]));
            CHECK(v[0].allocation_region == v[1].allocation_region &&
                  v[1].allocation_region == v[2].allocation_region &&
                  v[0].domain == v[1].domain && v[1].domain == v[2].domain &&
                  !memcmp(&v[0].reference, &v[1].reference,
                          sizeof(v[0].reference)) &&
                  !memcmp(&v[0].reference, &v[2].reference,
                          sizeof(v[0].reference)));
        }
        CHECK(regions[i] >= 1 && regions[i] <= 5);
        if (regions[i] < 5)
            CHECK(roots[i] == original.originals[regions[i] - 1].root &&
                  inc[i] == original.originals[regions[i] - 1].incarnation);
        if (allocations[i] < 5)
            CHECK(leaf[2].fields[1] ==
                  original.originals[allocations[i] - 1].allocation_value);
        if (domains[i] < 5)
            CHECK(leaf[2].fields[2] ==
                  original.originals[domains[i] - 1].domain_value);
        if (regions[i] == 5) {
            size_t roots_from_source = 0;
            for (size_t n = 1; n <= nl_checked_node_count(f); ++n) {
                const NLCheckedNodeView *node = nl_checked_node_view(f, n);
                if (node->kind == NL_CHECKED_INITIALIZE &&
                    node->lifetime_range.region == 5) {
                    CHECK(node->lifetime_place == roots[i] &&
                          node->lifetime_incarnation == inc[i] &&
                          node->lifetime_domain == domains[i]);
                    ++roots_from_source;
                }
            }
            CHECK(roots_from_source == 1);
        }
        for (size_t j = 0; j < i; ++j)
            CHECK(roots[i] != roots[j] && inc[i] != inc[j] &&
                  domains[i] != domains[j] && allocations[i] != allocations[j]);
    }
    CHECK(mixed == (strcmp(expectation, "mixed") == 0));
    NLSemanticSnapshot inventory;
    size_t as[5] = {0}, ds[5] = {0};
    CHECK(nl_semantic_snapshot(w.received, &inventory));
    for (size_t i = 1; i <= inventory.values; ++i) {
        NLSemanticValueView v;
        NLSemanticTypeView t;
        CHECK(nl_semantic_value_view(w.received, i, &v) &&
              nl_semantic_type_view(w.received, v.type, &t));
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        if (t.kind == NL_TYPE_ALLOCATION) {
            CHECK(v.allocation_region >= 1 && v.allocation_region <= 5 &&
                  current(w.received, i));
            ++as[v.allocation_region - 1];
        }
        if (v.type == nl_semantic_domain_type(w.received)) {
            CHECK(v.domain >= 1 && v.domain <= 5 && current(w.received, i));
            ++ds[v.domain - 1];
        }
    }
    for (size_t i = 0; i < 5; ++i) {
        NLSemanticBackingView region;
        CHECK(nl_semantic_backing_view(w.received, i + 1, &region));
        CHECK(as[i] == (size_t)region.live && ds[i] == (size_t)region.live);
    }
    const NLSemanticContext *call_worlds[] = {w.entry, w.returned, w.received};
    for (size_t j = 0; j < 3; ++j) {
        size_t owners_A[5] = {0}, owners_D[5] = {0};
        NLSemanticSnapshot all;
        CHECK(nl_semantic_snapshot(call_worlds[j], &all));
        for (size_t i = 1; i <= all.values; ++i) {
            NLSemanticValueView v;
            NLSemanticTypeView t;
            CHECK(nl_semantic_value_view(call_worlds[j], i, &v) &&
                  nl_semantic_type_view(call_worlds[j], v.type, &t));
            if (v.carrier == NL_CARRIER_ENDED)
                continue;
            if (t.kind == NL_TYPE_ALLOCATION) {
                CHECK(v.allocation_region >= 1 && v.allocation_region <= 5 &&
                      current_with_result(call_worlds[j], i,
                                          j == 1 ? w.result : 0));
                ++owners_A[v.allocation_region - 1];
            }
            if (v.type == nl_semantic_domain_type(call_worlds[j])) {
                CHECK(v.domain >= 1 && v.domain <= 5 &&
                      current_with_result(call_worlds[j], i,
                                          j == 1 ? w.result : 0));
                ++owners_D[v.domain - 1];
            }
        }
        for (size_t i = 0; i < 5; ++i) {
            NLSemanticBackingView r;
            CHECK(nl_semantic_backing_view(call_worlds[j], i + 1, &r));
            CHECK(owners_A[i] == (size_t)r.live &&
                  owners_D[i] == (size_t)r.live);
        }
        NLSemanticSnapshot snap;
        CHECK(nl_semantic_snapshot(call_worlds[j], &snap));
        for (size_t i = 1; i <= snap.scopes; ++i) {
            NLSemanticScopeView scope;
            CHECK(nl_semantic_scope_view(call_worlds[j], i, &scope) &&
                  !scope.active);
        }
    }
    size_t order[5] = {0}, released = 0, terminals = 0;
    CHECK(releases(f, order, &released, &terminals, 0) && released == 5 &&
          terminals == 5);
    char *output = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(entry, &output, &length) == NL_NODE_C_UNSUPPORTED &&
          !output && length == 777);
    printf("{\"owned_source_artifact\":true,\"public_validator\":true,\"four_"
           "construct_consume\":true,\"whole_value\":%zu,\"caller\":%zu,"
           "\"callee\":%zu,\"receiver\":%zu,\"mixed_return_preserved\":%s,"
           "\"three_arguments\":true,\"Copy_token_caller_retained\":true,"
           "\"original_leaves\":[",
           w.result, w.donors[0], w.parameters[0], w.receiver,
           mixed ? "true" : "false");
    for (size_t i = 0; i < count; ++i) {
        NLSemanticValueView v;
        CHECK(nl_semantic_value_view(w.received, leaves[i], &v));
        printf("%s{\"value\":%zu,\"ptr_value\":%zu,\"Allocation_value\":%zu,"
               "\"Domain_value\":%zu,\"root\":%zu,\"incarnation\":%zu,"
               "\"region\":%zu,\"Allocation_region\":%zu,\"Domain\":%zu}",
               i ? "," : "", leaves[i], v.fields[0], v.fields[1], v.fields[2],
               roots[i], inc[i], regions[i], allocations[i], domains[i]);
    }
    printf("],\"donors\":[%zu,%zu,%zu],\"parameters\":[%zu,%zu,%zu],\"Copy_"
           "input_value\":%zu,\"all_three_worlds_unique_A_D\":true,",
           w.donors[0], w.donors[1], w.donors[2], w.parameters[0],
           w.parameters[1], w.parameters[2], w.inputs[2]);
    printf("\"active_loans\":0,\"one_current_A_D_per_original\":true,"
           "\"release_order\":[%zu,%zu,"
           "%zu,%zu,%zu],\"C_output\":false}\n",
           order[0], order[1], order[2], order[3], order[4]);
    return true;
}
static bool run(const char *path, const char *expectation)
{
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *syntax = NULL, *entry_syntax = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *entry = NULL;
    bool ok = false;
    NLCheckDiagnostic diagnostic = {0};
    if (nl_source_load(path, &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(parser, &syntax, NULL) != NL_PARSE_OK ||
        nl_semantic_create(&context) != NL_CHECK_OK)
        goto done;
    const NLSyntaxTree *units[] = {syntax};
    NLFunctionUnitDiagnostic d = {0};
    NLSemanticSnapshot before = {0}, after = {0};
    if (!nl_semantic_snapshot(context, &before))
        goto done;
    const NLCheckStatus status =
        nl_semantic_register_function_unit(context, units, 1, &d);
    if (strncmp(expectation, "reject:", 7) == 0) {
        ok = status != NL_CHECK_OK && d.diagnostic.diagnostic.code != NULL &&
             strcmp(d.diagnostic.diagnostic.code, expectation + 7) == 0 &&
             nl_semantic_snapshot(context, &after) &&
             memcmp(&before, &after, sizeof(before)) == 0 && entry == NULL;
        if (ok)
            printf("{\"registration_rejected\":true,\"status\":%d,\"snapshot_"
                   "unchanged\":true,\"owned_artifact\":false,\"code\":\"%s\","
                   "\"start_byte\":%zu,\"end_byte\":%zu}\n",
                   status, d.diagnostic.diagnostic.code,
                   d.diagnostic.span.start_byte, d.diagnostic.span.end_byte);
        goto done;
    }
    if (status != NL_CHECK_OK) {
        (void)nl_check_diagnostic_render(stderr, source, &d.diagnostic);
        goto done;
    }
    if (nl_source_create("main()", 6, "<evidence-entry>", &entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(entry_parser, &entry_syntax,
                                            NULL) != NL_PARSE_OK)
        goto done;
    if (nl_semantic_check_expression(context, entry_syntax, &entry,
                                     &diagnostic) != NL_CHECK_OK) {
        (void)nl_check_diagnostic_render(stderr, entry_source, &diagnostic);
        goto done;
    }
    ok = inspect(entry, expectation);
done:
    nl_checked_destroy(entry);
    nl_semantic_destroy(context);
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
    return argc == 3 && run(argv[1], argv[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
