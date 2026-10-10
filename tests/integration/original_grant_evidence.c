/* Issue 274 read-only evidence observer. No seed/Matched/probe or private
 * headers. The public parser, unit transaction, main() check and owned
 * closure validator supply every fact. This executable never grants facts. */
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
        if (v->kind == NL_CHECKED_MATCH && init != NULL &&
            init->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
            return i;
    }
    return 0;
}

static bool inspect(const NLCheckedFragment *entry, bool matched)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    CHECK(f != NULL);
    for (size_t i = 0; i < 4; ++i) {
        NLCheckedNodeId m = allocation_match(f);
        CHECK(m != 0 &&
              nl_checked_captured_closure_validate(f, m) == NL_CHECK_OK);
        f = nl_checked_match_arm(f, m, 1);
        CHECK(f != NULL);
    }
    const NLCheckedNodeId m = allocation_match(f);
    NLCapturedClosureView originals;
    CHECK(nl_checked_captured_closure_validate(f, m) == NL_CHECK_OK &&
          nl_checked_captured_closure_view(f, m, &originals) &&
          originals.count == 4);
    const NLCapturedOriginal b = originals.originals[2];
    const NLCapturedOriginal a = originals.originals[matched ? 2 : 3];
    CHECK(b.origin == originals.ancestor && a.origin == b.origin);
    f = nl_checked_match_arm(f, m, 1);
    CHECK(f != NULL && nl_checked_captured_change_count(f) == 6);
    NLCapturedChangeView change;
    CHECK(nl_checked_captured_change_view(f, 3, &change));
    const NLCheckedField field =
        nl_checked_node_view(f, change.operation)->field;
    CHECK(field.parent == b.root && field.index == 1 &&
          field.access == NL_ACCESS_WRITE && field.dependency_compatible);
    NLSemanticPlaceView before, after;
    CHECK(nl_semantic_place_view(change.before, b.root, &before) &&
          nl_semantic_place_view(change.after, b.root, &after) && after.live &&
          after.incarnation == b.incarnation &&
          before.incarnation == after.incarnation &&
          after.placement.region == b.extent.region &&
          after.governing_domain == b.domain);
    /* Exactly one current source local A and D at the write checkpoint;
     * historic donor bindings are consumed. Check all current bindings. */
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(change.after, &snapshot));
    size_t ac = 0, dc = 0;
    for (size_t i = 1; i <= snapshot.bindings; ++i) {
        NLSemanticBindingView binding;
        CHECK(nl_semantic_binding_view(change.after, i, &binding));
        if (binding.availability != NL_AVAILABLE)
            continue;
        ac += binding.value == a.allocation_value;
        dc += binding.value == b.domain_value;
    }
    CHECK(ac == 1 && dc == 1);
    NLSemanticBindingView old_a, old_d;
    CHECK(
        nl_semantic_binding_view(change.after, a.allocation_binding, &old_a) &&
        nl_semantic_binding_view(change.after, b.domain_binding, &old_d) &&
        old_a.availability == NL_CONSUMED && old_d.availability == NL_CONSUMED);
    const NLSemanticContext *c = nl_checked_context(f);
    NLSemanticValueView records[2] = {{0}};
    NLValueId ids[2] = {0};
    size_t count = 0, whole = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_AGGREGATE) {
            NLSemanticTypeView t;
            CHECK(nl_semantic_type_view(c, n->type, &t));
            if (t.field_count != 3 || t.is_copy || t.is_discardable)
                continue;
            CHECK(count < 2 && n->result_count == 1);
            ids[count] = n->results[0].value;
            CHECK(nl_semantic_value_view(c, ids[count], &records[count]));
            ++count;
        }
        if (n->kind == NL_CHECKED_AGGREGATE_BINDING && n->argument_count == 3)
            ++whole;
    }
    CHECK(count == 2 && whole == 2 && ids[0] != ids[1]);
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticValueView p, allocation, domain;
        CHECK(records[i].carrier == NL_CARRIER_ENDED &&
              nl_semantic_value_view(c, records[i].fields[0], &p) &&
              nl_semantic_value_view(c, records[i].fields[1], &allocation) &&
              nl_semantic_value_view(c, records[i].fields[2], &domain));
        CHECK(p.reference.provenance == NL_PROVENANCE_VALID &&
              p.reference.place == b.root &&
              p.reference.incarnation == b.incarnation &&
              allocation.allocation_region == a.extent.region &&
              domain.domain == b.domain &&
              records[i].fields[1] == a.allocation_value &&
              records[i].fields[2] == b.domain_value);
    }
    NLSemanticSnapshot final;
    CHECK(nl_semantic_snapshot(c, &final));
    NLIncarnationId local_incarnations[2] = {0};
    for (size_t i = 1; i <= final.bindings; ++i) {
        NLSemanticBindingView binding;
        CHECK(nl_semantic_binding_view(c, i, &binding));
        for (size_t j = 0; j < 2; ++j) {
            if (binding.value != ids[j])
                continue;
            NLSemanticPlaceView place;
            CHECK(binding.availability == NL_CONSUMED &&
                  nl_semantic_place_view(c, binding.place, &place) &&
                  !place.live);
            local_incarnations[j] = place.incarnation;
        }
    }
    CHECK(local_incarnations[0] != 0 && local_incarnations[1] != 0 &&
          local_incarnations[0] != local_incarnations[1]);
    NLBackingRegionId release_order[5] = {0};
    size_t releases = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind != NL_CHECKED_DEALLOCATE)
            continue;
        const NLCheckedNodeView *aa =
            nl_checked_node_view(f, n->first_argument);
        const NLCheckedNodeView *rr =
            aa == NULL ? NULL : nl_checked_node_view(f, aa->next_argument);
        NLSemanticValueView allocation, raw;
        CHECK(releases < 5 && aa != NULL && rr != NULL &&
              nl_semantic_value_view(c, aa->results[0].value, &allocation) &&
              nl_semantic_value_view(c, rr->results[0].value, &raw));
        CHECK(allocation.allocation_region == raw.occupancy.region &&
              raw.occupancy.start == 0 &&
              raw.occupancy.length == b.extent.length);
        release_order[releases++] = allocation.allocation_region;
    }
    CHECK(releases == 5);
    for (size_t i = 0; i < 5; ++i)
        for (size_t j = 0; j < i; ++j)
            CHECK(release_order[i] != release_order[j]);
    CHECK(nl_semantic_place_view(c, b.root, &after) && !after.live);
    char *output = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(entry, &output, &length) == NL_NODE_C_UNSUPPORTED &&
          output == NULL && length == 777);
    printf("{\"source_path\":\"main/Some^5/B.prev\",\"owned_validator\":true,"
           "\"ptr_root\":%zu,\"ptr_incarnation\":%zu,\"ptr_region\":%zu,"
           "\"allocation_region\":%zu,\"domain\":%zu,\"field_write\":true,"
           "\"matched_at_write\":%s,\"current_allocation_carriers\":%zu,"
           "\"current_domain_carriers\":%zu,\"record_value_ids\":[%zu,%zu],"
           "\"same_original_A_D_through_repack\":true,\"whole_destructures\":2,"
           "\"local_record_incarnations\":[%zu,%zu],"
           "\"full_original_release_order\":[%zu,%zu,%zu,%zu,%zu],"
           "\"terminal_B_ended\":true,\"backend\":\"unsupported\"}\n",
           b.root, b.incarnation, b.extent.region, a.extent.region, b.domain,
           matched ? "true" : "false", ac, dc, ids[0], ids[1],
           local_incarnations[0], local_incarnations[1], release_order[0],
           release_order[1], release_order[2], release_order[3],
           release_order[4]);
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
    ok = inspect(entry, strcmp(expectation, "matched") == 0);
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
