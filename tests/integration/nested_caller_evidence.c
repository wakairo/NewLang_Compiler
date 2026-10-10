/* Issue 276 read-only actual-source evidence. No private headers, injected
 * heap/domain facts, name-based grants, or new matched bits. */
#include "../support/test.h"
#include "newlang/captured_closure.h"
#include "newlang/checked_c_node.h"
#include "newlang/parser.h"
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

static bool current(const NLSemanticContext *c, NLValueId id)
{
    for (size_t depth = 0; depth < 4; ++depth) {
        NLSemanticValueView v;
        CHECK(nl_semantic_value_view(c, id, &v));
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
                NLTypedOwnerDefinition definition;
                CHECK(nl_semantic_function_applicability(c, n->function,
                                                         &definition) &&
                      definition.definition_checked &&
                      definition.requirements == NL_OWNER_ALL_REQUIREMENTS &&
                      definition.step_count == 4);
                for (size_t s = 0; s < 4; ++s)
                    CHECK(definition.steps[s] == (NLTypedOwnerStep)(s + 1));
                ++*terminals;
            }
            CHECK(releases(body, order, count, terminals, depth + 1));
        }
    }
    return true;
}

static bool inspect(const NLCheckedFragment *entry, const char *expectation)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    CHECK(f);
    for (size_t i = 0; i < 4; ++i) {
        NLCheckedNodeId m = allocation_match(f);
        CHECK(m && nl_checked_captured_closure_validate(f, m) == NL_CHECK_OK);
        f = nl_checked_match_arm(f, m, 1);
        CHECK(f);
    }
    NLCapturedClosureView original;
    NLCheckedNodeId m = allocation_match(f);
    CHECK(nl_checked_captured_closure_validate(f, m) == NL_CHECK_OK &&
          nl_checked_captured_closure_view(f, m, &original) &&
          original.count == 4);
    f = nl_checked_match_arm(f, m, 1);
    CHECK(f);
    NLWholeValueCallView w = {0};
    size_t calls = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        NLWholeValueCallView candidate;
        if (nl_checked_whole_value_call_view(f, i, &candidate)) {
            w = candidate;
            ++calls;
        }
    }
    CHECK(calls == 1 && (w.count == 1 || w.count == 2) && w.entry &&
          w.returned && w.received);
    NLSemanticValueView result, received;
    CHECK(nl_semantic_value_view(w.returned, w.result, &result) &&
          nl_semantic_value_view(w.received, w.result, &received) &&
          result.field_count == 2 && result.carrier == NL_CARRIER_LOOSE &&
          received.carrier == NL_CARRIER_PLACE &&
          !memcmp(result.fields, received.fields, sizeof(result.fields)) &&
          current(w.received, w.result));
    size_t rr[2], aa[2], dd[2];
    NLPlaceId roots[2];
    NLIncarnationId inc[2];
    for (size_t i = 0; i < 2; ++i)
        CHECK(packet(w.received, result.fields[i], &rr[i], &aa[i], &dd[i],
                     &roots[i], &inc[i]));
    CHECK(rr[0] != rr[1] && roots[0] != roots[1] && inc[0] != inc[1] &&
          dd[0] != dd[1]);
    bool h1 = rr[0] == 5 && rr[1] == 3 && aa[0] == 5 && aa[1] == 3;
    bool mixed = rr[0] != aa[0] || rr[1] != aa[1];
    CHECK(h1 == (strcmp(expectation, "matched") == 0 ||
                 strcmp(expectation, "direct") == 0) &&
          mixed == (strcmp(expectation, "mixed") == 0));
    /* Inventory every actual original owner, not just the call's parameters. */
    NLSemanticSnapshot inventory;
    CHECK(nl_semantic_snapshot(w.received, &inventory));
    size_t allocations[5] = {0}, domains[5] = {0};
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
            ++allocations[v.allocation_region - 1];
        }
        if (v.type == nl_semantic_domain_type(w.received)) {
            CHECK(v.domain >= 1 && v.domain <= 5 && current(w.received, i));
            ++domains[v.domain - 1];
        }
    }
    for (size_t i = 0; i < 5; ++i)
        CHECK(allocations[i] == 1 && domains[i] == 1);
    if (strcmp(expectation, "h0") == 0) {
        CHECK(rr[0] == 5 && rr[1] == 4 && aa[0] == 5 && aa[1] == 4);
        NLSemanticValueView bv, dv, copy, pointer;
        NLSemanticPlaceView dst, field;
        CHECK(nl_semantic_value_view(
                  w.received, original.originals[2].allocation_value, &bv) &&
              nl_semantic_value_view(w.received,
                                     original.originals[2].domain_value, &dv));
        CHECK(bv.aggregate_owner == dv.aggregate_owner &&
              bv.aggregate_owner != result.fields[0] &&
              bv.aggregate_owner != result.fields[1]);
        CHECK(nl_semantic_place_view(w.received, roots[0], &dst) &&
              nl_semantic_place_view(w.received, dst.fixed_fields[2], &field) &&
              nl_semantic_value_view(w.received, field.current_value, &copy) &&
              copy.variant == 2 &&
              nl_semantic_value_view(w.received, copy.sum_payload, &pointer) &&
              pointer.reference.place == original.originals[2].root);
    }
    for (size_t i = 0; i < w.count; ++i) {
        NLSemanticBindingView donor, moved, param;
        CHECK(nl_semantic_binding_view(w.entry, w.donors[i], &donor) &&
              donor.availability == NL_AVAILABLE &&
              donor.value == w.inputs[i] && current(w.entry, w.inputs[i]));
        CHECK(nl_semantic_binding_view(w.returned, w.donors[i], &moved) &&
              moved.availability == NL_CONSUMED &&
              nl_semantic_binding_view(w.returned, w.parameters[i], &param) &&
              param.availability == NL_CONSUMED && param.value == w.inputs[i] &&
              donor.place != param.place);
        NLSemanticValueView before, after;
        CHECK(nl_semantic_value_view(w.entry, w.inputs[i], &before) &&
              nl_semantic_value_view(w.returned, w.inputs[i], &after) &&
              !memcmp(before.fields, after.fields, sizeof(before.fields)));
    }
    NLSemanticBindingView receiver, local;
    NLSemanticPlaceView cp, lp;
    CHECK(nl_semantic_binding_view(w.received, w.receiver, &receiver) &&
          receiver.availability == NL_AVAILABLE && receiver.value == w.result &&
          nl_semantic_place_view(w.received, receiver.place, &cp));
    memset(&lp, 0, sizeof(lp));
    if (w.callee_result) {
        CHECK(nl_semantic_binding_view(w.returned, w.callee_result, &local) &&
              local.availability == NL_CONSUMED &&
              nl_semantic_place_view(w.returned, local.place, &lp) &&
              cp.incarnation != lp.incarnation);
    } else
        CHECK(strcmp(expectation, "direct") == 0);
    NLCapturedChangeView change;
    CHECK(nl_checked_captured_change_view(f, 3, &change));
    const NLCheckedNodeView *write = nl_checked_node_view(f, change.operation);
    CHECK(write && write->kind == NL_CHECKED_REPLACE &&
          write->field.parent == original.originals[2].root &&
          write->field.index == 1 && write->field.dependency_compatible);
    size_t order[5] = {0}, count = 0, terminals = 0;
    CHECK(releases(f, order, &count, &terminals, 0) && count == 5 &&
          terminals == (strcmp(expectation, "h0") == 0 ? 3u : 2u));
    for (size_t i = 0; i < 5; ++i)
        for (size_t j = 0; j < i; ++j)
            CHECK(order[i] != order[j]);
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticPlaceView ended;
        CHECK(nl_semantic_place_view(nl_checked_context(f), roots[i], &ended) &&
              !ended.live);
    }
    char *output = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(entry, &output, &length) == NL_NODE_C_UNSUPPORTED &&
          !output && length == 777);
    NLSemanticValueView members[2];
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticValueView before;
        CHECK(
            nl_semantic_value_view(w.entry, result.fields[i], &before) &&
            nl_semantic_value_view(w.received, result.fields[i], &members[i]) &&
            !memcmp(before.fields, members[i].fields, sizeof(before.fields)));
        if (rr[i] == 3)
            CHECK(members[i].fields[1] ==
                      original.originals[mixed ? 3 : 2].allocation_value &&
                  members[i].fields[2] == original.originals[2].domain_value &&
                  roots[i] == original.originals[2].root &&
                  inc[i] == original.originals[2].incarnation);
    }
    printf("{\"whole_result_value\":%zu,\"member_packet_ids\":[%zu,%zu],"
           "\"pointer_values\":[%zu,%zu],\"allocation_values\":[%zu,%zu],"
           "\"domain_values\":[%zu,%zu],\"caller_donors\":[%zu,%zu],\"callee_"
           "formals\":[%zu,%zu],\"callee_result_binding\":%zu,\"caller_result_"
           "binding\":%zu,\"original_A_D_carriers_each\":1,",
           w.result, result.fields[0], result.fields[1], members[0].fields[0],
           members[1].fields[0], members[0].fields[1], members[1].fields[1],
           members[0].fields[2], members[1].fields[2], w.donors[0], w.donors[1],
           w.parameters[0], w.parameters[1], w.callee_result, w.receiver);
    printf("\"owned_validator\":true,\"field_write_B_prev\":true,\"actual_"
           "input_count\":%zu,\"ordinary_whole_return\":true,\"h1_dst_B_"
           "result\":%s,\"mixed_return_accepted\":%s,\"ptr_regions\":[%zu,%zu],"
           "\"allocation_regions\":[%zu,%zu],\"domains\":[%zu,%zu],\"original_"
           "roots\":[%zu,%zu],\"original_incarnations\":[%zu,%zu],\"donors_and_"
           "parameters_consumed\":true,\"fresh_caller_incarnation\":%zu,"
           "\"callee_incarnation\":%zu,\"unchanged_member_ids\":true,\"named_"
           "grant_consumers\":%zu,\"full_original_release_order\":[%zu,%zu,%zu,"
           "%zu,%zu],\"backend\":\"unsupported\"}\n",
           w.count, h1 ? "true" : "false", mixed ? "true" : "false", rr[0],
           rr[1], aa[0], aa[1], dd[0], dd[1], roots[0], roots[1], inc[0],
           inc[1], cp.incarnation, lp.incarnation, terminals, order[0],
           order[1], order[2], order[3], order[4]);
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
