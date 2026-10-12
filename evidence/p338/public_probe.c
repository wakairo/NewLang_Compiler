/* P338 public READ-ONLY observation of actual source-generated artifacts.
 * No seed/mutation/replay/clone calls, no private headers, no invented world.
 */
#include "newlang/captured_closure.h"
#include "newlang/parser.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

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
static bool inventory(const NLSemanticContext *c, NLValueId allocations[5],
                      NLValueId domains[5])
{
    NLSemanticSnapshot snap;
    CHECK(nl_semantic_snapshot(c, &snap) && snap.backing_regions == 5 &&
          snap.domains == 5);
    memset(allocations, 0, 5 * sizeof(*allocations));
    memset(domains, 0, 5 * sizeof(*domains));
    for (size_t i = 1; i <= snap.values; ++i) {
        NLSemanticValueView v;
        NLSemanticTypeView t;
        CHECK(nl_semantic_value_view(c, i, &v) &&
              nl_semantic_type_view(c, v.type, &t));
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        if (t.kind == NL_TYPE_ALLOCATION || t.kind == NL_TYPE_NOMINAL) {
            size_t region =
                t.kind == NL_TYPE_ALLOCATION ? v.allocation_region : v.domain;
            if (!region)
                continue;
            CHECK(region <= 5 && v.carrier == NL_CARRIER_PLACE);
            NLSemanticPlaceView holder;
            CHECK(nl_semantic_place_view(c, v.owner_place, &holder) &&
                  holder.live && holder.current_value == i);
            if (t.kind == NL_TYPE_ALLOCATION) {
                CHECK(!allocations[region - 1]);
                allocations[region - 1] = i;
            } else {
                CHECK(!domains[region - 1]);
                domains[region - 1] = i;
            }
        }
    }
    for (size_t i = 0; i < 5; ++i) {
        NLSemanticDomainView d;
        CHECK(allocations[i] && domains[i] &&
              nl_semantic_domain_view(c, i + 1, &d) && d.live &&
              d.value == domains[i]);
    }
    return true;
}
static bool inspect(const NLCheckedFragment *entry, size_t expected_changes,
                    const NLSource *source)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    CHECK(f);
    NLPlaceId roots[5] = {0};
    NLIncarnationId incarnations[5] = {0};
    size_t histogram[6] = {0};
    for (size_t i = 0; i < 5; ++i) {
        NLCheckedNodeId m = allocation_match(f);
        NLCapturedClosureView closure;
        CHECK(m && nl_checked_captured_closure_view(f, m, &closure));
        NLSemanticSnapshot before, after;
        CHECK(nl_semantic_snapshot(nl_checked_context(f), &before));
        NLCheckStatus status = nl_checked_captured_closure_validate(f, m);
        CHECK(nl_semantic_snapshot(nl_checked_context(f), &after) &&
              !memcmp(&before, &after, sizeof(before)));
        printf("{\"captured_depth\":%zu,\"public_validator_status\":%d}\n", i,
               status);
        CHECK(status == NL_CHECK_OK);
        if (i == 0)
            for (size_t j = 0; j < 6; ++j) {
                CHECK(closure.release_worlds[j] == 1);
                histogram[j] = closure.release_worlds[j];
            }
        f = nl_checked_match_arm(f, m, 1);
        CHECK(f);
        size_t initializers = 0;
        for (size_t j = 1; j <= nl_checked_node_count(f); ++j) {
            const NLCheckedNodeView *n = nl_checked_node_view(f, j);
            if (n->kind != NL_CHECKED_INITIALIZE)
                continue;
            CHECK(n->lifetime_range.region == i + 1 &&
                  n->lifetime_domain == i + 1 && n->lifetime_place &&
                  n->lifetime_incarnation);
            roots[i] = n->lifetime_place;
            incarnations[i] = n->lifetime_incarnation;
            ++initializers;
        }
        CHECK(initializers == 1);
    }
    CHECK(nl_checked_captured_change_count(f) == expected_changes);
    NLSourceView whole, body;
    const NLSource *retained = nl_checked_source(f);
    CHECK(nl_source_view(source, (NLSourceSpan){0, nl_source_length(source)},
                         &whole) &&
          nl_source_view(
              retained, (NLSourceSpan){0, nl_source_length(retained)}, &body) &&
          body.length <= whole.length);
    size_t source_offset = 0, matches = 0;
    for (size_t i = 0; i <= whole.length - body.length; ++i) {
        if (!memcmp(whole.bytes + i, body.bytes, body.length)) {
            source_offset = i;
            ++matches;
        }
    }
    CHECK(matches == 1);
    NLValueId original_as[5] = {0}, original_ds[5] = {0};
    for (size_t i = 0; i < expected_changes; ++i) {
        NLCapturedChangeView change;
        CHECK(nl_checked_captured_change_view(f, i, &change));
        const NLCheckedNodeView *n = nl_checked_node_view(f, change.operation);
        CHECK(n && n->kind == NL_CHECKED_REPLACE && n->field.present &&
              n->field.dependency_compatible &&
              n->field.access == NL_ACCESS_WRITE);
        const NLCheckedField e = n->field;
        NLSourceView operation_source;
        CHECK(nl_source_view(retained, n->span, &operation_source) &&
              operation_source.length >= 8 &&
              !memcmp(operation_source.bytes, "replace(", 8));
        NLSemanticPlaceView rb, ra, fb, fa;
        NLSemanticValueView old, new;
        CHECK(nl_semantic_place_view(change.before, e.parent, &rb) &&
              nl_semantic_place_view(change.after, e.parent, &ra) &&
              nl_semantic_place_view(change.before, e.child, &fb) &&
              nl_semantic_place_view(change.after, e.child, &fa) &&
              nl_semantic_value_view(change.before, e.old_value, &old) &&
              nl_semantic_value_view(change.after, e.new_value, &new));
        CHECK(rb.live && ra.live && rb.incarnation == ra.incarnation &&
              rb.placement.region == ra.placement.region &&
              rb.governing_domain == ra.governing_domain &&
              rb.governing_domain == rb.placement.region &&
              rb.fixed_fields[e.index] == e.child && fb.live && fa.live &&
              fb.incarnation == fa.incarnation &&
              fb.current_value == e.old_value &&
              fa.current_value == e.new_value &&
              fb.current_fact == e.child_fact &&
              fa.current_fact == e.child_post_fact &&
              fb.current_fact != fa.current_fact &&
              rb.current_fact == e.parent_fact &&
              ra.current_fact == e.parent_post_fact &&
              rb.current_fact != ra.current_fact &&
              fb.payload_occurrence == e.payload_occurrence &&
              fa.payload_occurrence == e.post_payload_occurrence);
        if (old.variant == 2) {
            NLSemanticOccurrenceView before, after;
            CHECK(e.payload_occurrence &&
                  nl_semantic_occurrence_view(change.before,
                                              e.payload_occurrence, &before) &&
                  before.live &&
                  nl_semantic_occurrence_view(change.after,
                                              e.payload_occurrence, &after) &&
                  !after.live);
        } else
            CHECK(old.variant == 1 && !e.payload_occurrence);
        size_t old_target_region = 0, target_region = 0;
        if (old.variant == 2) {
            NLSemanticValueView p;
            NLSemanticPlaceView t;
            CHECK(
                nl_semantic_value_view(change.before, old.sum_payload, &p) &&
                nl_semantic_place_view(change.before, p.reference.place, &t) &&
                t.live);
            old_target_region = t.placement.region;
        }
        if (new.variant == 2) {
            NLSemanticOccurrenceView occurrence;
            NLSemanticValueView pointer;
            NLSemanticPlaceView target;
            CHECK(e.post_payload_occurrence &&
                  e.post_payload_occurrence != e.payload_occurrence &&
                  nl_semantic_occurrence_view(
                      change.after, e.post_payload_occurrence, &occurrence) &&
                  occurrence.live && occurrence.root == e.child &&
                  nl_semantic_value_view(change.after, new.sum_payload,
                                         &pointer) &&
                  pointer.reference.provenance == NL_PROVENANCE_VALID &&
                  nl_semantic_place_view(change.after, pointer.reference.place,
                                         &target) &&
                  target.live &&
                  target.incarnation == pointer.reference.incarnation &&
                  !pointer.domain && !pointer.allocation_region &&
                  !pointer.reference.scope);
            target_region = target.placement.region;
        } else
            CHECK(new.variant == 1 && !e.post_payload_occurrence);
        NLValueId ab[5], aa[5], db[5], da[5];
        CHECK(inventory(change.before, ab, db) &&
              inventory(change.after, aa, da) && !memcmp(ab, aa, sizeof(ab)) &&
              !memcmp(db, da, sizeof(db)));
        if (!i) {
            memcpy(original_as, ab, sizeof(ab));
            memcpy(original_ds, db, sizeof(db));
        }
        CHECK(!memcmp(original_as, ab, sizeof(ab)) &&
              !memcmp(original_ds, db, sizeof(db)));
        size_t unchanged_fields = 0;
        for (size_t r = 0; r < 5; ++r) {
            NLSemanticPlaceView b, a;
            CHECK(nl_semantic_place_view(change.before, roots[r], &b) &&
                  nl_semantic_place_view(change.after, roots[r], &a) &&
                  b.live && a.live && b.incarnation == incarnations[r] &&
                  a.incarnation == incarnations[r] &&
                  b.placement.region == r + 1 && a.placement.region == r + 1 &&
                  b.governing_domain == r + 1 && a.governing_domain == r + 1 &&
                  b.fixed_field_count == 4 && a.fixed_field_count == 4 &&
                  !memcmp(b.fixed_fields, a.fixed_fields,
                          sizeof(b.fixed_fields)));
            if (roots[r] != e.parent)
                CHECK(!memcmp(&b, &a, sizeof(b)));
            for (size_t k = 0; k < 4; ++k) {
                if (b.fixed_fields[k] == e.child)
                    continue;
                NLSemanticPlaceView sibling_b, sibling_a;
                CHECK(nl_semantic_place_view(change.before, b.fixed_fields[k],
                                             &sibling_b) &&
                      nl_semantic_place_view(change.after, b.fixed_fields[k],
                                             &sibling_a) &&
                      !memcmp(&sibling_b, &sibling_a, sizeof(sibling_b)));
                ++unchanged_fields;
            }
        }
        CHECK(unchanged_fields == 19);
        const NLCheckedNodeView *projection =
            nl_checked_node_view(f, n->first_argument);
        CHECK(projection && projection->kind == NL_CHECKED_FIELD_REF &&
              projection->field.child == e.child &&
              projection->field.index == e.index);
        size_t scope = projection->reference_result.scope;
        CHECK(projection->field.access == NL_ACCESS_WRITE &&
              projection->reference_result.writable &&
              projection->reference_result.readable &&
              projection->reference_result.provenance == NL_PROVENANCE_VALID);
        size_t acquired = 0, loan = 0;
        for (size_t j = 1; j < change.operation; ++j) {
            const NLCheckedNodeView *candidate = nl_checked_node_view(f, j);
            if (candidate->kind == NL_CHECKED_REF_FROM_PTR &&
                candidate->reference_result.scope == scope &&
                candidate->lifetime_place == e.parent) {
                CHECK(candidate->lifetime_domain == rb.governing_domain &&
                      candidate->lifetime_incarnation == rb.incarnation &&
                      candidate->lifetime_range.region == rb.placement.region);
                acquired = j;
            }
            if (candidate->kind == NL_CHECKED_LOAN_HEADER &&
                candidate->loan.scope == scope) {
                CHECK(candidate->loan.domain == rb.governing_domain &&
                      candidate->loan.body_nonescape_proved &&
                      candidate->loan.prevent_lifetime_end &&
                      candidate->loan.access == NL_ACCESS_READ &&
                      !candidate->loan.is_exclusive);
                loan = j;
                NLSemanticBindingView binding;
                CHECK(nl_semantic_binding_view(
                          change.before, candidate->loan.source, &binding) &&
                      binding.availability == NL_AVAILABLE &&
                      binding.value == db[rb.governing_domain - 1]);
            }
        }
        CHECK(acquired && loan);
        NLSemanticScopeView during, final;
        CHECK(scope && nl_semantic_scope_view(change.before, scope, &during) &&
              during.active &&
              nl_semantic_scope_view(nl_checked_context(f), scope, &final) &&
              !final.active);
        printf("{\"event\":%zu,\"operation\":%zu,\"start_byte\":%zu,\"end_"
               "byte\":%zu,\"region\":%zu,\"domain\":%zu,"
               "\"root\":%zu,\"incarnation\":%zu,\"projection\":[%zu,%zu],"
               "\"child\":%zu,\"old_variant\":%zu,\"new_variant\":%zu,"
               "\"old_target_region\":%zu,\"target_region\":%zu,\"old_"
               "occurrence\":%zu,\"new_occurrence\":%zu,\"old_fact\":%zu,\"new_"
               "fact\":%zu,"
               "\"scope\":%zu,\"scope_ended\":true,\"unchanged_siblings\":%zu,"
               "\"original_A_D_unique_and_preserved\":true}\n",
               i, change.operation, source_offset + n->span.start_byte,
               source_offset + n->span.end_byte, rb.placement.region,
               rb.governing_domain, e.parent, rb.incarnation, e.nominal,
               e.index, e.child, old.variant, new.variant, old_target_region,
               target_region, e.payload_occurrence, e.post_payload_occurrence,
               e.child_fact, e.child_post_fact, scope, unchanged_fields);
        if (i) {
            NLCapturedChangeView previous;
            CHECK(nl_checked_captured_change_view(f, i - 1, &previous));
            const NLCheckedNodeView *pn =
                nl_checked_node_view(f, previous.operation);
            CHECK(previous.operation < change.operation);
            for (size_t r = 0; r < 5; ++r) {
                NLSemanticPlaceView a, b;
                CHECK(nl_semantic_place_view(previous.after, roots[r], &a) &&
                      nl_semantic_place_view(change.before, roots[r], &b) &&
                      !memcmp(&a, &b, sizeof(a)));
                for (size_t k = 0; k < 4; ++k) {
                    NLSemanticPlaceView x, y;
                    CHECK(nl_semantic_place_view(previous.after,
                                                 a.fixed_fields[k], &x) &&
                          nl_semantic_place_view(change.before,
                                                 b.fixed_fields[k], &y) &&
                          !memcmp(&x, &y, sizeof(x)));
                }
            }
            const NLCheckedNodeView *pp =
                nl_checked_node_view(f, pn->first_argument);
            NLSemanticScopeView ended;
            CHECK(nl_semantic_scope_view(change.before,
                                         pp->reference_result.scope, &ended) &&
                  !ended.active);
        }
    }
    size_t released = 0;
    const NLSemanticContext *final = nl_checked_context(f);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind != NL_CHECKED_DEALLOCATE)
            continue;
        const NLCheckedNodeView *a = nl_checked_node_view(f, n->first_argument),
                                *raw = a ? nl_checked_node_view(
                                               f, a->next_argument)
                                         : NULL;
        NLSemanticValueView av, rv;
        CHECK(a && raw && a->result_count == 1 && raw->result_count == 1 &&
              nl_semantic_value_view(final, a->results[0].value, &av) &&
              nl_semantic_value_view(final, raw->results[0].value, &rv) &&
              av.allocation_region == 5 - released &&
              rv.occupancy.region == av.allocation_region &&
              rv.occupancy.start == 0);
        ++released;
    }
    CHECK(released == 5);
    for (size_t i = 0; i < 5; ++i) {
        NLSemanticPlaceView root;
        NLSemanticDomainView d;
        CHECK(nl_semantic_place_view(final, roots[i], &root) && !root.live &&
              nl_semantic_domain_view(final, i + 1, &d) && !d.live);
    }
    printf("{\"source_owned_publicly_validated\":true,\"events\":%zu,"
           "\"original_Allocation_values\":[%zu,%zu,%zu,%zu,%zu],"
           "\"original_Domain_values\":[%zu,%zu,%zu,%zu,%zu],\"static_release_"
           "worlds_0_to_5\":[%zu,%zu,%zu,%zu,%zu,%zu],"
           "\"success_release_order\":[5,4,3,2,1],\"native\":false}\n",
           expected_changes, original_as[0], original_as[1], original_as[2],
           original_as[3], original_as[4], original_ds[0], original_ds[1],
           original_ds[2], original_ds[3], original_ds[4], histogram[0],
           histogram[1], histogram[2], histogram[3], histogram[4],
           histogram[5]);
    return true;
}
static bool refusal_rollback(NLSemanticContext *c, const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *syntax = NULL;
    NLSemanticSnapshot before, after;
    NLFunctionUnitDiagnostic d = {0};
    bool ok = false;
    if (nl_source_load(path, &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(parser, &syntax, NULL) != NL_PARSE_OK ||
        !nl_semantic_snapshot(c, &before))
        goto done;
    const NLSyntaxTree *units[] = {syntax};
    NLCheckStatus status = nl_semantic_register_function_unit(c, units, 1, &d);
    ok = status == NL_CHECK_SEMANTIC_ERROR && d.diagnostic.diagnostic.code &&
         !strcmp(d.diagnostic.diagnostic.code, "ALLOCATED-DOMAIN-MISMATCH") &&
         nl_semantic_snapshot(c, &after) &&
         !memcmp(&before, &after, sizeof(before));
    if (ok)
        printf("{\"refused_registration_status\":%d,\"snapshot_unchanged\":"
               "true,\"refused_owned_artifact_published\":false}\n",
               status);
done:
    nl_syntax_tree_destroy(syntax);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok;
}
static bool run(const char *path, size_t count, const char *refused)
{
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *syntax = NULL, *entry_syntax = NULL;
    NLSemanticContext *c = NULL;
    NLCheckedFragment *entry = NULL;
    NLFunctionUnitDiagnostic d = {0};
    NLCheckDiagnostic e = {0};
    bool ok = false;
    if (nl_source_load(path, &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(parser, &syntax, NULL) != NL_PARSE_OK ||
        nl_semantic_create(&c) != NL_CHECK_OK)
        goto done;
    if (refused && !refusal_rollback(c, refused))
        goto done;
    const NLSyntaxTree *units[] = {syntax};
    if (nl_semantic_register_function_unit(c, units, 1, &d) != NL_CHECK_OK)
        goto done;
    if (nl_source_create("main()", 6, "<P338-entry>", &entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(entry_parser, &entry_syntax,
                                            NULL) != NL_PARSE_OK)
        goto done;
    NLCheckStatus status =
        nl_semantic_check_expression(c, entry_syntax, &entry, &e);
    printf("{\"parse_status\":0,\"registration_status\":0,\"main_check_"
           "status\":%d,\"owned_entry_artifact\":%s}\n",
           status, entry ? "true" : "false");
    if (status != NL_CHECK_OK || !entry)
        goto done;
    ok = inspect(entry, count, source);
    if (ok && refused)
        printf("{\"same_context_actual_source_owned_clean_retry\":true}\n");
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
    return (argc == 3 || argc == 4) &&
                   run(argv[1], (size_t)strtoul(argv[2], NULL, 10),
                       argc == 4 ? argv[3] : NULL)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
