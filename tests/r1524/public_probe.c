#include "newlang/checked_c_node.h"
#include "newlang/copy_body.h"
#include "newlang/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t copies;
    size_t old29;
    size_t old149;
    size_t owners;
    size_t trials;
    size_t scalar_projections;
    size_t expected_payload;
} Audit;

static bool demand(bool condition, const char *description)
{
    if (!condition)
        fprintf(stderr, "R1524 OWNED_ASSERT %s\n", description);
    return condition;
}

static bool named(const NLCheckedFragment *fragment, NLSourceSpan span,
                  const char *name)
{
    NLSourceView view;
    return nl_source_view(nl_checked_source(fragment), span, &view) &&
           view.length == strlen(name) &&
           memcmp(view.bytes, name, view.length) == 0;
}

static bool scalar(const NLSemanticContext *world, NLValueId id,
                   size_t expected)
{
    NLSemanticValueView value;
    return nl_semantic_value_view(world, id, &value) && value.scalar_known &&
           value.scalar_value == expected;
}

static bool inspect_copy(const NLCheckedFragment *fragment, NLCheckedNodeId id,
                         Audit *audit)
{
    NLCopyBodyView body;
    NLCopyBodyParameters parameters;
    if (!demand(nl_checked_copy_body_view(fragment, id, &body),
                "named Copy view") ||
        !demand(nl_checked_copy_body_validate(fragment, id, &body) ==
                    NL_CHECK_OK,
                "public named Copy validator") ||
        !demand(nl_checked_copy_body_parameters(fragment, id, &parameters),
                "named Copy actual/formal view") ||
        !demand(nl_checked_copy_body_parameters_validate(
                    fragment, id, &parameters) == NL_CHECK_OK,
                "public Copy actual/formal validator"))
        return false;
    size_t changes = 0;
    for (size_t i = 0; i < body.event_count; ++i) {
        NLCopyBodyEventView event;
        if (!demand(nl_checked_copy_body_event(fragment, id, i, &event),
                    "Copy event view") ||
            !demand(nl_checked_copy_body_event_validate(fragment, id, i,
                                                        &event) == NL_CHECK_OK,
                    "public Copy event validator"))
            return false;
        if (event.kind != NL_COPY_BODY_CHANGE)
            continue;
        ++changes;
        NLSemanticPlaceView before, after, sibling_before, sibling_after;
        NLSemanticValueView old, incoming;
        if (!demand(event.field.present && event.field.index == 0,
                    "original T field0") ||
            !demand(nl_semantic_place_view(event.before, event.field.parent,
                                           &before) &&
                        nl_semantic_place_view(event.after, event.field.parent,
                                               &after),
                    "T original before/after") ||
            !demand(before.live && after.live &&
                        before.incarnation == after.incarnation &&
                        before.fixed_field_count == 2 &&
                        after.fixed_field_count == 2,
                    "T original incarnation preserved") ||
            !demand(nl_semantic_place_view(event.before, before.fixed_fields[1],
                                           &sibling_before) &&
                        nl_semantic_place_view(
                            event.after, after.fixed_fields[1], &sibling_after),
                    "T disjoint sibling views") ||
            !demand(
                sibling_before.incarnation == sibling_after.incarnation &&
                    sibling_before.current_fact == sibling_after.current_fact &&
                    scalar(event.before, sibling_before.current_value, 101) &&
                    scalar(event.after, sibling_after.current_value, 101),
                "disjoint sibling identity/current101 preserved") ||
            !demand(nl_semantic_value_view(event.before, event.field.old_value,
                                           &old) &&
                        nl_semantic_value_view(
                            event.after, event.field.new_value, &incoming) &&
                        old.scalar_known && incoming.scalar_known,
                    "actual old/new scalar packages"))
            return false;
        if (old.scalar_value == 29 && incoming.scalar_value == 149)
            ++audit->old29;
        else if (old.scalar_value == 149 &&
                 incoming.scalar_value == audit->expected_payload)
            ++audit->old149;
        else
            return demand(false, "frozen old/current Copy values");
        if (!demand(event.field.parent_fact != event.field.parent_post_fact &&
                        event.field.child_fact != event.field.child_post_fact,
                    "fresh changed child/ancestor facts"))
            return false;
        printf("R1524 COPY call=%zu old=%zu new=%zu sibling=101 "
               "parent=%zu child=%zu validated=1\n",
               id, old.scalar_value, incoming.scalar_value, event.field.parent,
               event.field.child);
    }
    if (!demand(changes == 1, "one genuine Change per named Copy"))
        return false;
    ++audit->copies;
    return true;
}

static bool inspect_owner(const NLCheckedFragment *fragment, NLCheckedNodeId id,
                          const NLCheckedNodeView *node, Audit *audit)
{
    const NLSemanticContext *entry = nl_checked_owner_entry(fragment, id);
    const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
    NLTypedOwnerDefinition definition;
    if (!demand(entry && body, "source-owned terminal entry/body") ||
        !demand(node->owner_call.entry_proved && node->owner_call.post_proved,
                "terminal actual entry/post predicates") ||
        !demand(nl_semantic_function_applicability(nl_checked_context(fragment),
                                                   node->function, &definition),
                "independent receiver definition") ||
        !demand(definition.definition_checked && !definition.live_return &&
                    definition.requirements == NL_OWNER_ALL_REQUIREMENTS &&
                    definition.step_count == 4 &&
                    definition.steps[0] == NL_OWNER_END_ROOT &&
                    definition.steps[1] == NL_OWNER_ERASE_SLOT &&
                    definition.steps[2] == NL_OWNER_FINALIZE_DOMAIN &&
                    definition.steps[3] == NL_OWNER_END_REGION,
                "four-step conditional receiver definition"))
        return false;
    NLSemanticValueView actual[3], formal[3];
    for (size_t i = 0; i < 3; ++i) {
        NLSemanticBindingView parameter;
        if (!demand(
                nl_semantic_value_view(entry, node->owner_call.inputs[i],
                                       &actual[i]) &&
                    nl_semantic_binding_view(
                        entry, node->owner_call.parameters[i], &parameter) &&
                    nl_semantic_value_view(entry, parameter.value, &formal[i]),
                "actual and formal source packages") ||
            !demand(node->owner_call.donor[i] != node->owner_call.parameters[i],
                    "fresh formal binding does not reuse donor binding"))
            return false;
    }
    NLSemanticPlaceView root;
    if (!demand(nl_semantic_place_view(entry, node->owner_call.root, &root) &&
                    root.live && root.independent_root,
                "actual original second root live at call") ||
        !demand(actual[0].reference.provenance == NL_PROVENANCE_VALID &&
                    actual[0].reference.place == node->owner_call.root &&
                    actual[0].reference.incarnation ==
                        node->owner_call.incarnation &&
                    formal[0].reference.place == actual[0].reference.place &&
                    formal[0].reference.incarnation ==
                        actual[0].reference.incarnation,
                "original pointer/incarnation passed to formal") ||
        !demand(actual[1].allocation_region == root.placement.region &&
                    formal[1].allocation_region ==
                        actual[1].allocation_region &&
                    actual[2].domain == root.governing_domain &&
                    formal[2].domain == actual[2].domain &&
                    root.governing_domain == node->owner_call.domain &&
                    root.placement.region == node->owner_call.range.region &&
                    root.placement.start == node->owner_call.range.start &&
                    root.placement.length == node->owner_call.range.length,
                "original Allocation/Domain/full range passed") ||
        !demand(root.fixed_field_count == 2, "H fixed children"))
        return false;
    NLSemanticPlaceView payload;
    if (!demand(
            nl_semantic_place_view(entry, root.fixed_fields[1], &payload) &&
                scalar(entry, payload.current_value, audit->expected_payload),
            "actual second original current payload"))
        return false;
    NLSemanticSnapshot snapshot;
    if (!demand(nl_semantic_snapshot(entry, &snapshot), "entry snapshot"))
        return false;
    size_t live_originals = 0;
    for (NLPlaceId place = 1; place <= snapshot.places; ++place) {
        NLSemanticPlaceView other;
        if (!nl_semantic_place_view(entry, place, &other) || !other.live ||
            !other.independent_root || other.type != definition.target ||
            other.placement.region == 0)
            continue;
        ++live_originals;
        if (place == node->owner_call.root)
            continue;
        NLSemanticPlaceView first_payload;
        if (!demand(other.placement.region != root.placement.region &&
                        other.governing_domain != root.governing_domain &&
                        other.incarnation != root.incarnation &&
                        other.fixed_field_count == 2 &&
                        nl_semantic_place_view(entry, other.fixed_fields[1],
                                               &first_payload) &&
                        scalar(entry, first_payload.current_value, 29),
                    "first original independently live with actual old29"))
            return false;
    }
    if (!demand(live_originals == 2, "two distinct actual originals at call"))
        return false;
    const NLSemanticContext *post = nl_checked_context(body);
    NLSemanticPlaceView ended;
    NLSemanticDomainView domain;
    if (!demand(nl_semantic_place_view(post, node->owner_call.root, &ended) &&
                    !ended.live &&
                    nl_semantic_domain_view(post, node->owner_call.domain,
                                            &domain) &&
                    !domain.live,
                "callee actually ended original root and Domain"))
        return false;
    for (size_t i = 1; i < 3; ++i) {
        NLSemanticBindingView parameter;
        if (!demand(nl_semantic_binding_view(
                        post, node->owner_call.parameters[i], &parameter) &&
                        parameter.availability == NL_CONSUMED,
                    "callee consumed non-Copy formal authority"))
            return false;
    }
    printf("R1524 OWNER call=%zu original=%zu incarnation=%zu R=%zu D=%zu "
           "requirements=255 steps=4 actual_formal=1 post_ended=1\n",
           id, node->owner_call.root, node->owner_call.incarnation,
           node->owner_call.range.region, node->owner_call.domain);
    ++audit->owners;
    return true;
}

static bool walk(const NLCheckedFragment *fragment, size_t depth, Audit *audit)
{
    if (!demand(fragment && depth < 32, "finite source-owned graph"))
        return false;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(fragment); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (!demand(node != NULL, "checked node exists"))
            return false;
        if (node->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
            ++audit->trials;
        if (node->kind == NL_CHECKED_FIELD_REF && node->field.present &&
            node->field.index == 1) {
            NLSemanticTypeView reference, target;
            if (!demand(nl_semantic_type_view(nl_checked_context(fragment),
                                              node->type, &reference) &&
                            reference.kind == NL_TYPE_REF &&
                            reference.access == NL_ACCESS_READ &&
                            !reference.is_exclusive &&
                            nl_semantic_type_view(nl_checked_context(fragment),
                                                  reference.target, &target) &&
                            target.kind == NL_TYPE_U8,
                        "read-only H u8 dependent field projection"))
                return false;
            ++audit->scalar_projections;
        }
        if (node->kind == NL_CHECKED_REGISTERED_CALL && node->body_backed) {
            if (named(fragment, node->name, "exchange_setting") &&
                !inspect_copy(fragment, id, audit))
                return false;
            if (named(fragment, node->name, "retire_cell") &&
                !inspect_owner(fragment, id, node, audit))
                return false;
            const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
            if (!demand(body != NULL, "real registered callee body") ||
                !walk(body, depth + 1, audit))
                return false;
        }
        if (node->kind == NL_CHECKED_MATCH) {
            for (size_t arm = 0; arm < 2; ++arm) {
                const NLCheckedFragment *branch =
                    nl_checked_match_arm(fragment, id, arm);
                if (!demand(branch != NULL, "both actual allocation worlds") ||
                    !walk(branch, depth + 1, audit))
                    return false;
            }
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 4) {
        fprintf(stderr, "usage: probe source [C-output payload]\n");
        return 90;
    }
    NLSource *source = NULL;
    NLSource *entry_source = NULL;
    NLParser *parser = NULL;
    NLParser *entry_parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSyntaxTree *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    char *generated = NULL;
    size_t generated_length = 0;
    int result = 90;
    NLParseDiagnostic parse_diagnostic = {0};
    NLFunctionUnitDiagnostic unit_diagnostic = {0};
    NLCheckDiagnostic check_diagnostic = {0};
    if (nl_source_load(argv[1], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK)
        goto done;
    NLParseStatus parsed =
        nl_parser_parse_function_unit(parser, &tree, &parse_diagnostic);
    printf("R1524 LAYER parse status=%d\n", (int)parsed);
    if (parsed != NL_PARSE_OK) {
        nl_parse_diagnostic_render(stderr, source, &parse_diagnostic);
        result = 31;
        goto done;
    }
    if (nl_semantic_create(&context) != NL_CHECK_OK)
        goto done;
    const NLSyntaxTree *inputs[] = {tree};
    NLCheckStatus registered = nl_semantic_register_function_unit(
        context, inputs, 1, &unit_diagnostic);
    printf("R1524 LAYER definition_CHECK status=%d\n", (int)registered);
    if (registered != NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, source, &unit_diagnostic.diagnostic);
        result = 32;
        goto done;
    }
    if (nl_source_create("main()", 6, "R1524 actual entry", &entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_source_fragment(entry_parser, &entry_tree,
                                        &parse_diagnostic) != NL_PARSE_OK)
        goto done;
    NLCheckStatus checked_status = nl_semantic_check_source_fragment(
        context, entry_tree, &checked, &check_diagnostic);
    printf("R1524 LAYER actual_entry_CHECK status=%d\n", (int)checked_status);
    if (checked_status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, entry_source, &check_diagnostic);
        result = 33;
        goto done;
    }
    if (argc == 2) {
        result = 0;
        goto done;
    }
    Audit audit = {0};
    audit.expected_payload = (size_t)strtoul(argv[3], NULL, 10);
    if (!demand(nl_checked_proof_class(checked) == NL_CHECKED_MACHINE_PROVED,
                "source-issued machine proof class") ||
        !walk(checked, 0, &audit) ||
        !demand(audit.copies == 2 && audit.old29 == 1 && audit.old149 == 1 &&
                    audit.owners == 1 && audit.trials == 2 &&
                    audit.scalar_projections == 1,
                "full FIRST static operation membership")) {
        printf("R1524 COUNTS copies=%zu old29=%zu old149=%zu owners=%zu "
               "trials=%zu projections=%zu\n",
               audit.copies, audit.old29, audit.old149, audit.owners,
               audit.trials, audit.scalar_projections);
        result = 34;
        goto done;
    }
    printf("R1524 LAYER public_owned status=0 copies=2 receiver=1 trials=2 "
           "original_p_A_D=1 old_current_sibling=1\n");
    NLNodeCStatus backend =
        nl_checked_c_node(checked, &generated, &generated_length);
    printf("R1524 LAYER backend status=%d length=%zu\n", (int)backend,
           generated_length);
    if (backend != NL_NODE_C_OK) {
        result = 35;
        goto done;
    }
    FILE *output = fopen(argv[2], "wb");
    if (!output)
        goto done;
    bool wrote =
        fwrite(generated, 1, generated_length, output) == generated_length;
    if (fclose(output) != 0)
        wrote = false;
    result = wrote ? 0 : 90;
done:
    free(generated);
    if (checked)
        nl_checked_destroy(checked);
    if (context)
        nl_semantic_destroy(context);
    if (entry_tree)
        nl_syntax_tree_destroy(entry_tree);
    if (entry_parser)
        nl_parser_destroy(entry_parser);
    if (entry_source)
        nl_source_destroy(entry_source);
    if (tree)
        nl_syntax_tree_destroy(tree);
    if (parser)
        nl_parser_destroy(parser);
    if (source)
        nl_source_destroy(source);
    return result;
}
