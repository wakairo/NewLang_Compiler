/* Diagnostic-only public export. Existing observer supplies live validation;
 * included without modification. No private state, seed, clone or authority. */
#define main p338_observer_entry
#include "../p338/public_probe.c"
#undef main
#include "newlang/raw_storage.h"

static bool value_json(const NLSemanticContext *c, NLValueId id)
{
    NLSemanticValueView v;
    NLSemanticTypeView t;
    CHECK(id && nl_semantic_value_view(c, id, &v) &&
          nl_semantic_type_view(c, v.type, &t));
    printf("{\"value_id\":%zu,\"type_id\":%zu,\"type_kind\":%d,\"is_copy\":%s,"
           "\"carrier\":%d,\"owner_place_id\":%zu,"
           "\"allocation_region_id\":%zu,\"domain_id\":%zu,\"variant\":%zu,"
           "\"sum_payload_id\":%zu,\"scalar_known\":%s,\"scalar_value\":%zu,"
           "\"reference\":{\"place_id\":%zu,\"incarnation_id\":%zu,\"scope_"
           "id\":%zu,\"provenance\":%d,\"readable\":%s,\"writable\":%s,"
           "\"occurrence_dependency_id\":%zu}}",
           id, v.type, t.kind, t.is_copy ? "true" : "false", v.carrier,
           v.owner_place, v.allocation_region, v.domain, v.variant,
           v.sum_payload, v.scalar_known ? "true" : "false", v.scalar_value,
           v.reference.place, v.reference.incarnation, v.reference.scope,
           v.reference.provenance, v.reference.readable ? "true" : "false",
           v.reference.writable ? "true" : "false",
           v.reference.occurrence_dependency);
    return true;
}
static bool offset_for(const NLSource *source, const NLSource *retained,
                       size_t *offset)
{
    NLSourceView whole, body;
    CHECK(nl_source_view(source, (NLSourceSpan){0, nl_source_length(source)},
                         &whole) &&
          nl_source_view(
              retained, (NLSourceSpan){0, nl_source_length(retained)}, &body) &&
          body.length <= whole.length);
    size_t matches = 0;
    for (size_t i = 0; i <= whole.length - body.length; ++i)
        if (!memcmp(whole.bytes + i, body.bytes, body.length)) {
            *offset = i;
            ++matches;
        }
    CHECK(matches == 1);
    return true;
}
static bool nodes_json(const NLCheckedFragment *f, const NLSource *source)
{
    size_t offset = 0;
    CHECK(offset_for(source, nl_checked_source(f), &offset));
    printf("[");
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        CHECK(n);
        printf("%s{\"node_id\":%zu,\"kind\":%d,\"source_span_bytes\":[%zu,%zu],"
               "\"symbol_id\":%zu,\"initializer_id\":%zu,"
               "\"first_argument_id\":%zu,\"next_argument_id\":%zu,\"argument_"
               "count\":%zu,\"result_value_ids\":[",
               i > 1 ? "," : "", i, n->kind, offset + n->span.start_byte,
               offset + n->span.end_byte, n->symbol, n->initializer,
               n->first_argument, n->next_argument, n->argument_count);
        for (size_t j = 0; j < n->result_count; ++j)
            printf("%s%zu", j ? "," : "", n->results[j].value);
        printf("],\"allocation_trial\":%s,\"allocation_success\":%s,"
               "\"allocation_target_type_id\":%zu,\"allocation_size\":%zu,"
               "\"allocation_alignment\":%zu,"
               "\"backing_id\":%zu,\"allocation_value_id\":%zu,\"storage_value_"
               "id\":%zu,\"root_place_id\":%zu,\"incarnation_id\":%zu,\"domain_"
               "id\":%zu,"
               "\"extent\":[%zu,%zu,%zu],\"loan\":{\"source_binding_id\":%zu,"
               "\"ref_symbol_id\":%zu,\"stability_symbol_id\":%zu,\"domain_"
               "id\":%zu,\"place_id\":%zu,\"incarnation_id\":%zu,\"scope_id\":%"
               "zu,\"access\":%d,\"exclusive\":%s,\"nonescape_proved\":%s,"
               "\"prevent_lifetime_end\":%s},"
               "\"field\":{\"present\":%s,\"nominal_id\":%zu,\"index\":%zu,"
               "\"parent_id\":%zu,\"child_id\":%zu},"
               "\"reference\":{\"present\":%s,\"place_id\":%zu,\"incarnation_"
               "id\":%zu,\"scope_id\":%zu,\"provenance\":%d,\"readable\":%s,"
               "\"writable\":%s}",
               n->allocation_trial ? "true" : "false",
               n->allocation_success ? "true" : "false", n->allocation_target,
               n->allocation_size, n->allocation_alignment, n->backing,
               n->allocation_authority, n->storage_authority, n->lifetime_place,
               n->lifetime_incarnation, n->lifetime_domain,
               n->lifetime_range.region, n->lifetime_range.start,
               n->lifetime_range.length, n->loan.source, n->loan.ref_symbol,
               n->loan.stability, n->loan.domain, n->loan.place,
               n->loan.incarnation, n->loan.scope, n->loan.access,
               n->loan.is_exclusive ? "true" : "false",
               n->loan.body_nonescape_proved ? "true" : "false",
               n->loan.prevent_lifetime_end ? "true" : "false",
               n->field.present ? "true" : "false", n->field.nominal,
               n->field.index, n->field.parent, n->field.child,
               n->has_reference_result ? "true" : "false",
               n->reference_result.place, n->reference_result.incarnation,
               n->reference_result.scope, n->reference_result.provenance,
               n->reference_result.readable ? "true" : "false",
               n->reference_result.writable ? "true" : "false");
        printf(",\"graph\":{\"first_item_id\":%zu,\"next_item_id\":%zu,\"tail_"
               "id\":%zu,\"item_count\":%zu,\"value_use\":%d,\"type_id\":%zu},"
               "\"field_evidence\":{\"access\":%d,\"dependency_compatible\":%s,"
               "\"parent_incarnation_id\":%zu,\"child_incarnation_id\":%zu,"
               "\"parent_fact_id\":%zu,\"child_fact_id\":%zu,\"old_value_id\":%"
               "zu,\"new_value_id\":%zu},"
               "\"loan_body_close_span_bytes\":[%zu,%zu]}",
               n->first_item, n->next_item, n->tail, n->item_count,
               n->value_use, n->type, n->field.access,
               n->field.dependency_compatible ? "true" : "false",
               n->field.parent_incarnation, n->field.child_incarnation,
               n->field.parent_fact, n->field.child_fact, n->field.old_value,
               n->field.new_value, offset + n->loan.body_close.start_byte,
               offset + n->loan.body_close.end_byte);
    }
    printf("]");
    return true;
}
static bool state_json(const NLSemanticContext *c, const NLPlaceId roots[5])
{
    NLValueId as[5], ds[5];
    CHECK(inventory(c, as, ds));
    printf("[");
    for (size_t r = 0; r < 5; ++r) {
        NLSemanticPlaceView p;
        NLSemanticBackingView backing;
        CHECK(nl_semantic_place_view(c, roots[r], &p) && p.live &&
              p.fixed_field_count == 4 &&
              nl_semantic_backing_view(c, p.placement.region, &backing) &&
              backing.live);
        printf("%s{\"root_place_id\":%zu,\"incarnation_id\":%zu,\"nominal_type_"
               "id\":%zu,\"backing_id\":%zu,\"domain_id\":%zu,\"current_fact_"
               "id\":%zu,\"current_value_id\":%zu,"
               "\"extent\":[%zu,%zu,%zu],\"backing_size\":%zu,\"backing_"
               "alignment\":%zu,\"original_allocation\":",
               r ? "," : "", roots[r], p.incarnation, p.type,
               p.placement.region, p.governing_domain, p.current_fact,
               p.current_value, p.placement.region, p.placement.start,
               p.placement.length, backing.size, backing.alignment);
        CHECK(value_json(c, as[p.placement.region - 1]));
        printf(",\"original_domain\":");
        CHECK(value_json(c, ds[p.governing_domain - 1]));
        printf(",\"fixed_fields\":[");
        for (size_t k = 0; k < 4; ++k) {
            NLSemanticPlaceView field;
            NLAggregateField declared;
            CHECK(nl_semantic_place_view(c, p.fixed_fields[k], &field) &&
                  nl_semantic_aggregate_field_view(c, p.type, k, &declared));
            const char *names[] = {"next", "prev", "child", "payload"};
            CHECK(!strcmp(declared.name, names[k]));
            printf(
                "%s{\"index\":%zu,\"name\":\"%s\",\"place_id\":%zu,"
                "\"incarnation_id\":%zu,\"governing_domain_id\":%zu,\"current_"
                "fact_id\":%zu,\"payload_occurrence_id\":%zu,\"value\":",
                k ? "," : "", k, declared.name, p.fixed_fields[k],
                field.incarnation, field.governing_domain, field.current_fact,
                field.payload_occurrence);
            CHECK(value_json(c, field.current_value));
            if (field.payload_occurrence) {
                NLSemanticOccurrenceView occurrence;
                CHECK(nl_semantic_occurrence_view(c, field.payload_occurrence,
                                                  &occurrence));
                printf(",\"occurrence\":{\"id\":%zu,\"live\":%s,\"root_place_"
                       "id\":%zu,\"payload_place_id\":%zu,\"variant\":%zu}",
                       field.payload_occurrence,
                       occurrence.live ? "true" : "false", occurrence.root,
                       occurrence.payload_place, occurrence.variant);
            }
            NLSemanticValueView v;
            CHECK(nl_semantic_value_view(c, field.current_value, &v));
            if (v.sum_payload) {
                printf(",\"pointer_payload\":");
                CHECK(value_json(c, v.sum_payload));
            }
            printf("}");
        }
        printf("]}");
    }
    printf("]");
    return true;
}
static bool export_live(const NLCheckedFragment *entry, const NLSource *source,
                        size_t count)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    CHECK(f);
    char path[1024];
    snprintf(path, sizeof(path), "entry/call:%zu/body", nl_checked_root(entry));
    NLPlaceId roots[5] = {0};
    for (size_t depth = 0; depth < 5; ++depth) {
        NLCheckedNodeId m = allocation_match(f);
        NLCapturedClosureView closure;
        CHECK(m && nl_checked_captured_closure_view(f, m, &closure));
        printf("{\"record\":\"issuer_layer\",\"fragment_path\":\"%s\","
               "\"depth\":%zu,\"match_node_id\":%zu,\"trial_node_id\":%zu,"
               "\"ancestor_world\":\"%s/closure:%zu/"
               "ancestor\",\"captured_originals\":[",
               path, depth, m, nl_checked_node_view(f, m)->initializer, path,
               m);
        for (size_t j = 0; j < closure.count; ++j) {
            const NLCapturedOriginal o = closure.originals[j];
            CHECK(o.origin == closure.ancestor);
            printf("%s{\"root_place_id\":%zu,\"incarnation_id\":%zu,\"domain_"
                   "id\":%zu,\"extent\":[%zu,%zu,%zu],\"allocation_binding_"
                   "id\":%zu,\"domain_binding_id\":%zu,\"allocation_value_id\":"
                   "%zu,\"domain_value_id\":%zu,\"allocation_availability\":%d,"
                   "\"domain_availability\":%d}",
                   j ? "," : "", o.root, o.incarnation, o.domain,
                   o.extent.region, o.extent.start, o.extent.length,
                   o.allocation_binding, o.domain_binding, o.allocation_value,
                   o.domain_value, o.allocation_availability,
                   o.domain_availability);
        }
        printf("],\"parent_nodes\":");
        CHECK(nodes_json(f, source));
        const NLCheckedFragment *failed = nl_checked_match_arm(f, m, 0),
                                *success = nl_checked_match_arm(f, m, 1);
        CHECK(failed && success);
        printf(
            ",\"failure_arm_path\":\"%s/match:%zu/arm:0\",\"failure_nodes\":",
            path, m);
        CHECK(nodes_json(failed, source));
        size_t used = strlen(path);
        CHECK(used + 40 < sizeof(path));
        snprintf(path + used, sizeof(path) - used, "/match:%zu/arm:1", m);
        printf(",\"success_arm_path\":\"%s\",\"success_nodes\":", path);
        CHECK(nodes_json(success, source));
        printf("}\n");
        f = success;
        size_t initials = 0;
        for (size_t j = 1; j <= nl_checked_node_count(f); ++j) {
            const NLCheckedNodeView *n = nl_checked_node_view(f, j);
            if (n->kind == NL_CHECKED_INITIALIZE) {
                roots[depth] = n->lifetime_place;
                ++initials;
            }
        }
        CHECK(initials == 1);
    }
    size_t replaced = 0;
    NLCheckedNodeId operations[4] = {0};
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_REPLACE) {
            CHECK(replaced < count && count <= 4);
            operations[replaced++] = i;
        }
    CHECK(replaced == count && nl_checked_captured_change_count(f) == count);
    size_t offset = 0;
    CHECK(offset_for(source, nl_checked_source(f), &offset));
    for (size_t i = 0; i < count; ++i) {
        NLCapturedChangeView c;
        CHECK(nl_checked_captured_change_view(f, i, &c) &&
              c.operation == operations[i] && c.world == nl_checked_context(f));
        const NLCheckedNodeView *n = nl_checked_node_view(f, c.operation),
                                *projected =
                                    nl_checked_node_view(f, n->first_argument);
        CHECK(projected && projected->kind == NL_CHECKED_FIELD_REF);
        NLCheckedNodeId loan = 0, acquired = 0;
        NLScopeId scope = projected->reference_result.scope;
        for (size_t j = 1; j < c.operation; ++j) {
            const NLCheckedNodeView *q = nl_checked_node_view(f, j);
            if (q->kind == NL_CHECKED_LOAN_HEADER && q->loan.scope == scope) {
                CHECK(!loan);
                loan = j;
            }
            if (q->kind == NL_CHECKED_REF_FROM_PTR &&
                q->reference_result.scope == scope &&
                q->lifetime_place == n->field.parent) {
                CHECK(!acquired);
                acquired = j;
            }
        }
        CHECK(loan && acquired);
        const NLCheckedNodeView *ln = nl_checked_node_view(f, loan);
        NLSemanticBindingView domain_binding;
        NLSemanticScopeView during, closed;
        CHECK(nl_semantic_binding_view(c.before, ln->loan.source,
                                       &domain_binding) &&
              nl_semantic_scope_view(c.before, scope, &during) &&
              nl_semantic_scope_view(nl_checked_context(f), scope, &closed) &&
              during.active && !closed.active);
        printf("{\"record\":\"event_detail\",\"fragment_path\":\"%s\","
               "\"ordinal\":%zu,\"checked_operation_node_id\":%zu,\"source_"
               "span_bytes\":[%zu,%zu],"
               "\"before_world\":\"%s/event:%zu/before\",\"after_world\":\"%s/"
               "event:%zu/after\","
               "\"loan_chain\":{\"read_loan_node_id\":%zu,\"write_ref_node_"
               "id\":%zu,\"projection_node_id\":%zu,\"scope_id\":%zu,"
               "\"original_domain_value_id\":%zu,\"active_before\":true,"
               "\"closed_at_terminal\":true,\"close_node\":null,\"close_"
               "evidence\":\"public scope views; no distinct close node\"},"
               "\"facts\":{\"parent_before\":%zu,\"parent_after\":%zu,\"child_"
               "before\":%zu,\"child_after\":%zu},\"old_option\":",
               path, i, c.operation, offset + n->span.start_byte,
               offset + n->span.end_byte, path, i, path, i, loan, acquired,
               n->first_argument, scope, domain_binding.value,
               n->field.parent_fact, n->field.parent_post_fact,
               n->field.child_fact, n->field.child_post_fact);
        CHECK(value_json(c.before, n->field.old_value));
        const NLCheckedNodeView *acq = nl_checked_node_view(f, acquired);
        const NLCheckedNodeView *ptr =
            nl_checked_node_view(f, acq->first_argument);
        const NLCheckedNodeView *stable =
            ptr ? nl_checked_node_view(f, ptr->next_argument) : NULL;
        CHECK(ptr && stable && ptr->result_count == 1 &&
              stable->result_count == 1 && projected->result_count == 1);
        printf(",\"pointer_operand\":");
        CHECK(value_json(c.before, ptr->results[0].value));
        printf(",\"read_stability_operand\":");
        CHECK(value_json(c.before, stable->results[0].value));
        printf(",\"projected_write_operand\":");
        CHECK(value_json(c.before, projected->results[0].value));
        NLSemanticScopeView after_scope;
        CHECK(nl_semantic_scope_view(c.after, scope, &after_scope));
        printf(",\"scope_observations\":{\"during_before\":%s,\"during_after_"
               "change\":%s,\"terminal_active\":%s,\"parent_scope_id\":%zu,"
               "\"parent_authority_value_id\":%zu,\"next_checkpoint_active\":",
               during.active ? "true" : "false",
               after_scope.active ? "true" : "false",
               closed.active ? "true" : "false", during.parent,
               during.parent_authority);
        if (i + 1 < count) {
            NLCapturedChangeView next;
            NLSemanticScopeView next_scope;
            CHECK(nl_checked_captured_change_view(f, i + 1, &next) &&
                  nl_semantic_scope_view(next.before, scope, &next_scope) &&
                  !next_scope.active);
            printf("false");
        } else
            printf("null");
        printf("}");
        printf(",\"new_option\":");
        CHECK(value_json(c.after, n->field.new_value));
        printf(",\"old_occurrence\":");
        if (n->field.payload_occurrence) {
            NLSemanticOccurrenceView before, after;
            CHECK(nl_semantic_occurrence_view(
                      c.before, n->field.payload_occurrence, &before) &&
                  nl_semantic_occurrence_view(
                      c.after, n->field.payload_occurrence, &after));
            printf("{\"id\":%zu,\"live_before\":%s,\"live_after\":%s,\"root_"
                   "place_id\":%zu,\"payload_place_id\":%zu}",
                   n->field.payload_occurrence, before.live ? "true" : "false",
                   after.live ? "true" : "false", before.root,
                   before.payload_place);
        } else
            printf("null");
        printf(",\"new_occurrence\":");
        if (n->field.post_payload_occurrence) {
            NLSemanticOccurrenceView occurrence;
            CHECK(nl_semantic_occurrence_view(
                c.after, n->field.post_payload_occurrence, &occurrence));
            printf("{\"id\":%zu,\"live\":%s,\"root_place_id\":%zu,\"payload_"
                   "place_id\":%zu}",
                   n->field.post_payload_occurrence,
                   occurrence.live ? "true" : "false", occurrence.root,
                   occurrence.payload_place);
        } else
            printf("null");
        printf(",\"before_all_roots\":");
        CHECK(state_json(c.before, roots));
        printf(",\"after_all_roots\":");
        CHECK(state_json(c.after, roots));
        printf("}\n");
    }
    printf(
        "{\"record\":\"inventory_check\",\"fragment_path\":\"%s\",\"checked_"
        "replace_count\":%zu,\"captured_change_count\":%zu,\"selected_fragment_"
        "bijection_observer_checked\":true,\"issuer_tree_completeness\":\"NOT_"
        "PUBLICLY_ATTESTED\",\"portable_authority\":false}\n",
        path, replaced, count);
    return true;
}
static bool export_refusal(NLSemanticContext *c, const char *path)
{
    NLSource *s = NULL;
    NLParser *p = NULL;
    NLSyntaxTree *tree = NULL;
    NLFunctionUnitDiagnostic d = {0};
    NLSemanticSnapshot before, after;
    bool ok = false;
    if (nl_source_load(path, &s) != NL_SOURCE_OK ||
        nl_parser_create(s, &p) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(p, &tree, NULL) != NL_PARSE_OK ||
        !nl_semantic_snapshot(c, &before))
        goto done;
    const NLSyntaxTree *units[] = {tree};
    NLCheckStatus status = nl_semantic_register_function_unit(c, units, 1, &d);
    ok = status == NL_CHECK_SEMANTIC_ERROR && d.diagnostic.diagnostic.code &&
         !strcmp(d.diagnostic.diagnostic.code, "ALLOCATED-DOMAIN-MISMATCH") &&
         nl_semantic_snapshot(c, &after) &&
         !memcmp(&before, &after, sizeof(before));
    if (ok)
        printf("{\"record\":\"source_refusal\",\"parser_status\":0,\"stage\":"
               "\"function_registration\",\"status\":%d,\"diagnostic_code\":"
               "\"ALLOCATED-DOMAIN-MISMATCH\",\"source_span_bytes\":[%zu,%zu],"
               "\"owned_artifact_present\":false,\"main_attempted\":false,"
               "\"backend_attempted\":false,\"snapshot_unchanged\":true}\n",
               status, d.diagnostic.span.start_byte,
               d.diagnostic.span.end_byte);
done:
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    return ok;
}
static bool export_run(const char *path, size_t count, const char *refused)
{
    NLSource *s = NULL, *es = NULL;
    NLParser *p = NULL, *ep = NULL;
    NLSyntaxTree *tree = NULL, *et = NULL;
    NLSemanticContext *c = NULL;
    NLCheckedFragment *entry = NULL;
    NLFunctionUnitDiagnostic d = {0};
    NLCheckDiagnostic e = {0};
    bool ok = false;
    if (nl_source_load(path, &s) != NL_SOURCE_OK ||
        nl_parser_create(s, &p) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(p, &tree, NULL) != NL_PARSE_OK ||
        nl_semantic_create(&c) != NL_CHECK_OK)
        goto done;
    if (!export_refusal(c, refused))
        goto done;
    const NLSyntaxTree *units[] = {tree};
    if (nl_semantic_register_function_unit(c, units, 1, &d) != NL_CHECK_OK ||
        nl_source_create("main()", 6, "<P346-driver>", &es) != NL_SOURCE_OK ||
        nl_parser_create(es, &ep) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(ep, &et, NULL) != NL_PARSE_OK ||
        nl_semantic_check_expression(c, et, &entry, &e) != NL_CHECK_OK ||
        !entry)
        goto done;
    printf(
        "{\"record\":\"admission\",\"parser_status\":0,\"registration_status\":"
        "0,\"main_status\":0,\"owned_artifact_present\":true,\"same_context_"
        "clean_retry\":true,\"driver_expression\":\"main()\"}\n");
    ok = inspect(entry, count, s) && export_live(entry, s, count);
done:
    nl_checked_destroy(entry);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(et);
    nl_parser_destroy(ep);
    nl_source_destroy(es);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    return ok;
}
int main(int argc, char **argv)
{
    return argc == 4 && export_run(argv[1], (size_t)strtoul(argv[2], NULL, 10),
                                   argv[3])
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
