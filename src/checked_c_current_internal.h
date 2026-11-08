#ifndef NEWLANG_CHECKED_C_CURRENT_INTERNAL_H
#define NEWLANG_CHECKED_C_CURRENT_INTERNAL_H

#include "copy_body_internal.h"
#include "linear_break_internal.h"

/* A synchronous borrow of existing CHECK evidence, not a new issuer, world,
 * stored proof or general CFG. Only one lexical T and one completed H match
 * in the real main body are composed. The H serializer still checks both
 * allocation arms and every cleanup operation before the loop can emit. */
typedef struct {
    const NLCheckedFragment *body;
    NLSymbolId formal;
    NLCheckedNodeId selection;
} NLPhysicalHelper;

typedef struct {
    const NLCheckedFragment *body;
    NLCheckedNodeId loop, constructor, binding, heap_match;
    NLLinearBreakView loop_view;
    NLPhysicalHelper helper, terminal_helper;
    NLTypeId quantity;
    NLSymbolId result;
    bool original_before_heap;
} NLPhysicalCurrent;

static bool physical_current_literal(const NLCheckedFragment *f,
                                     const NLCheckedNodeView *n)
{
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    NLSemanticValueView v;
    return n && n->kind == NL_CHECKED_U8_LITERAL && n->type == u8 &&
           n->result_count == 1 && n->has_scalar_result &&
           n->scalar_result.known && n->scalar_result.value <= 255 &&
           nl_copy_plain(c, n->results[0].value, u8) &&
           nl_semantic_value_view(c, n->results[0].value, &v) &&
           v.scalar_known && v.scalar_value == n->scalar_result.value;
}

/* Called only on the helper borrowed from full current-cycle validation.
 * That validation authenticates each identifier's source binding/Copy. This
 * additional walk proves the ACTUAL normal-final value ends at the selected
 * IF, rather than a different scalar postlude with coincidentally equal bits.
 * Every edge remains in this one owned body and strictly precedes its use. */
static bool physical_current_normal_result(const NLPhysicalHelper *helper,
                                           const NLCheckedNodeView *body)
{
    const NLTypeId u8 =
        nl_semantic_core_type(nl_checked_context(helper->body), NL_TYPE_U8);
    NLCheckedNodeId value = body->tail;
    for (size_t step = 0; step < nl_checked_node_count(helper->body); ++step) {
        if (value == helper->selection)
            return true;
        const NLCheckedNodeView *use =
            nl_checked_node_view(helper->body, value);
        if (!use || use->kind != NL_CHECKED_IDENTIFIER || use->terminates ||
            use->type != u8 || use->result_count != 1 ||
            use->results[0].type != u8 || use->value_use != NL_VALUE_COPIED)
            return false;
        NLCheckedNodeId initializer = 0;
        for (NLCheckedNodeId id = 1; id < value; ++id) {
            const NLCheckedNodeView *binding =
                nl_checked_node_view(helper->body, id);
            if (!binding)
                return false;
            if (binding->kind == NL_CHECKED_BINDING &&
                binding->symbol == use->symbol) {
                if (initializer || binding->terminates ||
                    !binding->initializer || binding->initializer >= value ||
                    binding->span.end_byte > use->span.start_byte)
                    return false;
                initializer = binding->initializer;
            }
        }
        if (!initializer)
            return false;
        value = initializer;
    }
    return false;
}

static bool physical_current_world(const NLSemanticContext *world,
                                   const NLLinearBreakView *p)
{
    NLSemanticSnapshot snapshot;
    NLSemanticPlaceView root;
    return world && nl_semantic_snapshot(world, &snapshot) &&
           !snapshot.backing_regions && !snapshot.domains &&
           !snapshot.raw_intervals &&
           nl_semantic_place_view(world, p->root, &root) && root.live &&
           root.independent_root && root.implicit_local &&
           !root.governing_domain && root.type == p->nominal &&
           root.incarnation == p->incarnation && root.fixed_field_count == 2 &&
           p->index < 2 && root.fixed_fields[p->index] == p->child;
}

static bool physical_current_constructor(const NLPhysicalCurrent *p)
{
    const NLLinearBreakView *v = &p->loop_view;
    const NLSemanticContext *entry = v->entry_world;
    const NLCheckedNodeView *n = nl_checked_node_view(p->body, p->constructor);
    NLSemanticBindingView binding;
    NLSemanticPlaceView root;
    NLSemanticTypeView type;
    if (!n || n->kind != NL_CHECKED_AGGREGATE || n->type != v->nominal ||
        n->argument_count != 2 || n->result_count != 1 ||
        !nl_semantic_type_view(entry, v->nominal, &type) ||
        type.kind != NL_TYPE_NOMINAL || !type.is_copy || !type.is_discardable ||
        type.field_count != 2 ||
        nl_semantic_recursive_local_type(entry, v->nominal) ||
        !nl_semantic_binding_view(entry, v->base, &binding) ||
        binding.type != v->nominal || binding.availability != NL_AVAILABLE ||
        binding.place != v->root ||
        !nl_semantic_place_view(entry, v->root, &root))
        return false;
    bool seen[2] = {false, false};
    NLCheckedNodeId member = n->first_argument;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *field = nl_checked_node_view(p->body, member);
        const NLCheckedNodeView *literal =
            field ? nl_checked_node_view(p->body, field->initializer) : NULL;
        NLAggregateField declared;
        NLSemanticPlaceView child;
        NLSemanticValueView current;
        if (!field || field->kind != NL_CHECKED_AGGREGATE_FIELD ||
            field->field_index > 1 || seen[field->field_index] || !literal ||
            !physical_current_literal(p->body, literal) ||
            !nl_semantic_aggregate_field_view(entry, v->nominal,
                                              field->field_index, &declared) ||
            declared.type != literal->type ||
            declared.type != nl_semantic_core_type(entry, NL_TYPE_U8) ||
            !nl_semantic_place_view(
                entry, root.fixed_fields[field->field_index], &child) ||
            !child.live || child.parent_aggregate != v->root ||
            child.parent_incarnation != v->incarnation ||
            child.parent_field_index != field->field_index ||
            !nl_semantic_value_view(entry, child.current_value, &current) ||
            current.aggregate_owner != root.current_value ||
            !current.scalar_known ||
            current.scalar_value != literal->scalar_result.value)
            return false;
        seen[field->field_index] = true;
        member = field->next_argument;
    }
    return member == 0;
}

/* The full validator authenticates the owner/source/world/target/ordinal and
 * current entry -> inductive header -> Break summary before any borrow below.
 * Declining this lane does not relax the old stateful-Copy/H consumers. */
static NLCheckStatus physical_current_prepare(const NLCheckedFragment *body,
                                              NLPhysicalCurrent *out)
{
    NLPhysicalCurrent p = {.body = body};
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, id);
        if (n->kind == NL_CHECKED_LOOP) {
            if (p.loop)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            p.loop = id;
        }
    }
    if (!p.loop)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLCheckStatus validated =
        nl_linear_break_validate(body, p.loop, &p.loop_view);
    if (validated != NL_CHECK_OK)
        return validated;
    if (!p.loop_view.changing_current ||
        p.loop_view.scalar_loop == p.loop_view.terminal_old ||
        !p.loop_view.incoming_call || !p.loop_view.entry_world ||
        !p.loop_view.post_world ||
        !physical_current_world(p.loop_view.entry_world, &p.loop_view) ||
        !physical_current_world(p.loop_view.post_world, &p.loop_view) ||
        !p.loop_view.helper_body || !p.loop_view.helper_formal ||
        !p.loop_view.helper_selection)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    p.helper =
        (NLPhysicalHelper){p.loop_view.helper_body, p.loop_view.helper_formal,
                           p.loop_view.helper_selection};
    if (p.loop_view.terminal_call) {
        p.terminal_helper =
            (NLPhysicalHelper){p.loop_view.terminal_helper_body,
                               p.loop_view.terminal_helper_formal,
                               p.loop_view.terminal_helper_selection};
        if (!p.loop_view.terminal_old || !p.terminal_helper.body ||
            !p.terminal_helper.formal || !p.terminal_helper.selection)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        const NLCheckedNodeView *root = nl_checked_node_view(
            p.terminal_helper.body, nl_checked_root(p.terminal_helper.body));
        const NLCheckedNodeView *choice = nl_checked_node_view(
            p.terminal_helper.body, p.terminal_helper.selection);
        if (!root || !choice || root->terminates ||
            root->tail != p.terminal_helper.selection ||
            choice->normal_arms != 2)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    const NLCheckedNodeView *choice =
        nl_checked_node_view(p.helper.body, p.helper.selection);
    const NLCheckedNodeView *callee =
        nl_checked_node_view(p.helper.body, nl_checked_root(p.helper.body));
    if (!choice || !callee || callee->terminates ||
        !((choice->normal_arms == 2 && callee->tail == p.helper.selection) ||
          (choice->normal_arms == 1 &&
           nl_control_exits_count(p.helper.body->exits) == 1 &&
           physical_current_normal_result(&p.helper, callee))))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    /* The current-cycle validator sealed every actual RETURN target, ordinal
     * and exit world. The separate normal-result walk above additionally
     * proves that the actual normal-final binding/Copy path ends at this IF.
     * The pure walk alone permits other postludes and does not prove that
     * identity. Both paths yield a u8 at the genuine function-call boundary;
     * the source IF still has only one normal arm.
     */
    p.quantity = nl_semantic_core_type(p.loop_view.entry_world, NL_TYPE_USIZE);
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    if (!root || root->kind != NL_CHECKED_BLOCK || root->terminates ||
        root->result_count || !root->tail)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    size_t count = 0;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, id);
        const NLCheckedNodeView *init =
            n ? nl_checked_node_view(body, n->initializer) : NULL;
        if (!n || !init || ++count > nl_checked_node_count(body))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        if (n->kind == NL_CHECKED_STATEMENT && init->kind == NL_CHECKED_MATCH) {
            if (p.heap_match || p.binding || init->item_count != 2 ||
                init->normal_arms != 2 || !init->normal_frame_unchanged ||
                init->captured_frame_closed || init->terminates)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            const NLCheckedNodeView *trial =
                nl_checked_node_view(body, init->initializer);
            if (!trial || trial->kind != NL_CHECKED_TRY_ALLOCATE_ONE)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            p.heap_match = n->initializer;
        } else if (n->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *receiver =
                nl_checked_node_view(body, n->first_argument);
            if (!receiver || receiver->kind != NL_CHECKED_RECEIVER ||
                n->argument_count != 1 || receiver->next_argument ||
                receiver->symbol != n->symbol || receiver->type != init->type)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            if (init->kind == NL_CHECKED_AGGREGATE) {
                if (p.constructor || p.binding ||
                    receiver->symbol != p.loop_view.base ||
                    init->type != p.loop_view.nominal)
                    return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
                p.constructor = n->initializer;
                p.original_before_heap = !p.heap_match;
            } else if (n->initializer == p.loop) {
                if (!p.constructor || !p.heap_match || p.binding ||
                    init->type != p.loop_view.result.type ||
                    init->type !=
                        (p.loop_view.terminal_old
                             ? nl_semantic_core_type(p.loop_view.entry_world,
                                                     NL_TYPE_U8)
                             : p.quantity))
                    return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
                p.binding = id;
                p.result = n->symbol;
            } else if ((init->type != p.quantity &&
                        init->type !=
                            nl_semantic_core_type(p.loop_view.entry_world,
                                                  NL_TYPE_U8)) ||
                       (init->kind != NL_CHECKED_USIZE_LITERAL &&
                        init->kind != NL_CHECKED_U8_LITERAL &&
                        init->kind != NL_CHECKED_IDENTIFIER &&
                        (init->kind != NL_CHECKED_FIELD_READ || !p.binding)))
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        } else if (n->kind != NL_CHECKED_STATEMENT ||
                   (init->kind != NL_CHECKED_IDENTIFIER &&
                    init->kind != NL_CHECKED_U8_LITERAL &&
                    init->kind != NL_CHECKED_USIZE_LITERAL))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        id = n->next_item;
    }
    const NLCheckedNodeView *tail = nl_checked_node_view(body, root->tail);
    if (count != root->item_count || !tail || tail->kind != NL_CHECKED_UNIT ||
        !p.binding || !physical_current_constructor(&p))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    *out = p;
    return NL_CHECK_OK;
}
#endif
