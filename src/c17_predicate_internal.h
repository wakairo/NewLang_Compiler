#ifndef NEWLANG_C17_PREDICATE_INTERNAL_H
#define NEWLANG_C17_PREDICATE_INTERNAL_H

#include "pure_u8_internal.h"

/* Compose only with a T constructed before this actual scalar loop. The
 * existing loop validator seals the complete caller and exact persistent
 * frame; the pure callee below cannot receive or modify that T. This is a
 * borrowed source-origin check, not a new state bridge or stored receipt. */
static bool predicate_original_frame(const NLCheckedFragment *f,
                                     NLCheckedNodeId call_id,
                                     const V0Program *p)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, call_id);
    NLCheckedNodeId loop = 0;
    V0Program operands = *p;
    if (!call ||
        !quantity_scalar_impl(f, call->first_argument, &operands, true,
                              &loop) ||
        !loop)
        return false;
    const NLCheckedNodeView *origin = nl_checked_node_view(f, loop);
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f));
    if (!origin || !root || root->kind != NL_CHECKED_BLOCK ||
        origin->span.end_byte > call->span.start_byte)
        return false;
    bool original = false;
    size_t visited = 0;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (!n || ++visited > nl_checked_node_count(f))
            return false;
        if (n->kind == NL_CHECKED_BINDING && n->initializer == loop)
            return original;
        const NLCheckedNodeView *receiver =
            nl_checked_node_view(f, n->first_argument);
        if (n->kind == NL_CHECKED_BINDING && n->argument_count == 1 &&
            receiver && receiver->kind == NL_CHECKED_RECEIVER &&
            receiver->type == p->aggregate && receiver->symbol == n->symbol &&
            n->span.end_byte <= origin->span.start_byte &&
            avs_construction(f, n->initializer, &operands))
            original = true;
        id = n->next_item;
    }
    return false;
}

static bool predicate_descriptor_change(const NLCheckedFragment *f,
                                        const NLCheckedNodeView *loan,
                                        NLCheckedNodeId selector, V0Program *p,
                                        NLCheckedField *change)
{
    const NLCheckedNodeView *block = nl_checked_node_view(f, loan->initializer);
    const NLCheckedNodeView *tail =
        block ? nl_checked_node_view(f, block->tail) : NULL;
    if (!block || block->item_count || block->first_item || !tail)
        return false;
    if (tail->kind == NL_CHECKED_REPLACE) {
        const NLCheckedNodeView *ref =
            nl_checked_node_view(f, tail->first_argument);
        const NLCheckedNodeView *input =
            ref ? nl_checked_node_view(f, ref->next_argument) : NULL;
        V0Program local = *p;
        if (f->body_count != 1 || !tail->field.present || !input ||
            bool_source_origin(f, ref->next_argument) != selector ||
            tail->field.new_value != input->results[0].value ||
            !gate_loan(f, loan, &local))
            return false;
        *change = tail->field;
        return true;
    }

    /* The scoped call already owns its actual-to-formal and Change replay.
     * Borrow that single event and check that the helper returns its old Copy.
     */
    NLCopyBodyParameters parameters;
    if (f->body_count != 2 || tail->kind != NL_CHECKED_REGISTERED_CALL ||
        !nl_checked_copy_body_parameters(f, block->tail, &parameters) ||
        nl_checked_node_view(f, parameters.loan) != loan ||
        bool_source_origin(f, parameters.arguments[1]) != selector)
        return false;
    const NLCheckStatus status =
        nl_checked_copy_body_parameters_validate(f, block->tail, &parameters);
    if (status == NL_CHECK_OUT_OF_MEMORY || status == NL_CHECK_RESOURCE_LIMIT)
        p->copy_body_status = status;
    NLCopyBodyView copy;
    NLCopyBodyEventView event;
    if (status != NL_CHECK_OK ||
        !nl_checked_copy_body_view(f, block->tail, &copy) ||
        copy.event_count != 1 ||
        !nl_checked_copy_body_event(f, block->tail, 0, &event) ||
        event.kind != NL_COPY_BODY_CHANGE)
        return false;
    const NLCheckedNodeView *root = nl_checked_node_view(copy.body, copy.root);
    const NLCheckedNodeView *store =
        nl_checked_node_view(copy.body, event.node);
    const NLCheckedNodeView *dest =
        store ? nl_checked_node_view(copy.body, store->first_argument) : NULL;
    const NLCheckedNodeView *incoming =
        dest ? nl_checked_node_view(copy.body, dest->next_argument) : NULL;
    if (!root || root->terminates ||
        copy_origin(copy.body, root->tail) != event.node || !incoming ||
        incoming->kind != NL_CHECKED_IDENTIFIER ||
        incoming->value_use != NL_VALUE_COPIED ||
        incoming->symbol != parameters.parameters[1])
        return false;
    *change = event.field;
    return true;
}

/* Borrow the actual pure call's canonical arm snapshots. The closed caller
 * walk connects their current descriptor to the single later Change. */
static bool predicate_live_descriptor(const NLCheckedFragment *f,
                                      NLCheckedNodeId call_id,
                                      const NLPureU8Call *pure, V0Program *p)
{
    enum {
        ORIGINAL,
        BYTES,
        SELECTED,
        OLD,
        UPDATED,
        SIBLING,
        OLD_COPY,
        COUNT
    };
    const NLCheckedKind kinds[COUNT] = {
        NL_CHECKED_AGGREGATE,       NL_CHECKED_USIZE_LITERAL,
        NL_CHECKED_REGISTERED_CALL, NL_CHECKED_LOAN_HEADER,
        NL_CHECKED_FIELD_READ,      NL_CHECKED_FIELD_READ,
        NL_CHECKED_IDENTIFIER};
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f));
    if (!root || root->kind != NL_CHECKED_BLOCK || !root->terminates ||
        root->tail || root->item_count != COUNT + 1)
        return false;
    const NLCheckedNodeView *bindings[COUNT], *initializers[COUNT];
    NLCheckedNodeId item = root->first_item;
    for (size_t i = 0; i < COUNT; ++i) {
        const NLCheckedNodeView *binding = nl_checked_node_view(f, item);
        if (!binding || binding->kind != NL_CHECKED_BINDING ||
            binding->argument_count != 1 || !binding->symbol ||
            binding->next_item <= item)
            return false;
        const NLCheckedNodeView *init =
            nl_checked_node_view(f, binding->initializer);
        if (!init || init->kind != kinds[i])
            return false;
        bindings[i] = binding;
        initializers[i] = init;
        item = binding->next_item;
    }
    const NLCheckedNodeView *ret = nl_checked_node_view(f, item);
    const NLCheckedNodeView *unit =
        ret ? nl_checked_node_view(f, ret->initializer) : NULL;
    const NLCheckedNodeView *call = initializers[SELECTED];
    if (!ret || ret->kind != NL_CHECKED_RETURN || ret->next_item ||
        ret->returned.type != p->unit || ret->returned.value || !unit ||
        unit->kind != NL_CHECKED_UNIT || unit->type != p->unit ||
        bindings[SELECTED]->initializer != call_id ||
        bool_source_origin(f, call->first_argument) !=
            bindings[BYTES]->initializer ||
        bool_source_origin(f, bindings[OLD_COPY]->initializer) !=
            bindings[OLD]->initializer)
        return false;

    const NLCheckedNodeView *loan = initializers[OLD];
    NLCheckedField update;
    if (!predicate_descriptor_change(f, loan, call_id, p, &update))
        return false;
    const NLCheckedField *change = &update;
    const NLCheckedField *projection = &loan->field;
    const NLCheckedField *target = &initializers[UPDATED]->field;
    const NLCheckedField *sibling = &initializers[SIBLING]->field;
    if (change->base != bindings[ORIGINAL]->symbol ||
        projection->base != change->base ||
        projection->index != change->index ||
        projection->parent != change->parent ||
        projection->parent_incarnation != change->parent_incarnation ||
        projection->parent_fact != change->parent_fact ||
        projection->child != change->child ||
        projection->child_incarnation != change->child_incarnation ||
        projection->child_fact != change->child_fact ||
        projection->old_value != change->old_value ||
        target->index != change->index || target->child != change->child ||
        target->child_incarnation != change->child_incarnation ||
        target->child_fact != change->child_post_fact ||
        target->old_value != change->new_value ||
        sibling->index == change->index || sibling->child == change->child)
        return false;
    const NLCheckedField *reads[] = {target, sibling};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedField *read = reads[i];
        if (!bounded_field(f, read, p) || read->access != NL_ACCESS_READ ||
            read->base != change->base || read->parent != change->parent ||
            read->parent_incarnation != change->parent_incarnation ||
            read->parent_fact != change->parent_post_fact)
            return false;
    }

    const NLCheckedNodeView *ctor = initializers[ORIGINAL];
    if (ctor->type != p->aggregate || ctor->result_count != 1)
        return false;
    const NLSemanticContext *c = nl_checked_context(f);
    NLSemanticScopeView scope;
    NLSemanticBindingView original;
    NLSemanticValueView value;
    if (!nl_semantic_scope_view(c, loan->loan.scope, &scope) || scope.active ||
        !nl_semantic_binding_view(c, change->base, &original) ||
        original.place != change->parent ||
        !nl_semantic_value_view(c, original.value, &value) ||
        value.type != p->aggregate || value.field_count != 2 ||
        value.fields[change->index] != change->new_value ||
        value.fields[sibling->index] != sibling->old_value)
        return false;
    for (size_t i = 0; i < 2; ++i)
        if (!nl_c17_descriptor_frame(nl_checked_context(nl_checked_if_arm(
                                         pure->body, pure->selection, i)),
                                     change->base, ctor->results[0].value,
                                     change, sibling))
            return false;
    return true;
}

static bool predicate_call(const NLCheckedFragment *f, NLCheckedNodeId id,
                           V0Program *p)
{
    if (!nl_h_custody_result_valid(f, id))
        return false;
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (!call || !c || !body || !c->functions || call->type != p->u8 ||
        call->argument_count != 1 || call->result_count != 1 ||
        call->results[0].type != p->u8 || !call->function ||
        call->function > c->function_count || p->scalar_depth ||
        p->formal_body || p->unit_parameters || p->u8_export || p->linear_break)
        return false;
    NLPureU8Call pure;
    NLSymbolId formal;
    if (!nl_pure_u8_call_mode(f, id, !p->aggregate, &pure) ||
        (p->aggregate && !predicate_original_frame(f, id, p) &&
         !predicate_live_descriptor(f, id, &pure, p)) ||
        !v1_call_parameter_kind(f, id, p, &formal, true) ||
        formal != pure.formal)
        return false;
    V0Function *prior = v0_find_function(p, call->function);
    if (prior)
        return prior->quantity_parameter && prior->result == p->u8;
    if (p->count == NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS)
        return false;
    p->functions[p->count++] =
        (V0Function){call->function, body, p->u8, formal, 0, true, false};
    p->uses_usize = true;
    return true;
}
#endif
