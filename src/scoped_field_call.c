#include "semantic_internal.h"

bool nl_scoped_field_call_member(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    return f && id &&
           (f->scoped_field_call == id
#ifdef NEWLANG_EXPERIMENTAL_TWO_SCOPED_FIELD_CALLS
            || f->second_scoped_field_call == id
#endif
           );
}

/* One finite prev->next continuation. No additional grant or summary.
 * The public validator applies the same check to the second actual before
 * world, after each body has separately retained its own Change evidence. */
bool nl_scoped_field_continuation(const NLCheckedFragment *f,
                                  const NLCheckedNodeView *projection,
                                  const NLSemanticContext *next,
                                  NLValueId input)
{
#ifdef NEWLANG_EXPERIMENTAL_TWO_SCOPED_FIELD_CALLS
    NLScopedFieldCallView first;
    if (!f || !projection || !next ||
        !nl_checked_scoped_field_call_view(f, f->scoped_field_call, &first) ||
        projection->kind != NL_CHECKED_FIELD_REF ||
        projection->field.index != 0 || !projection->has_reference_result ||
        !input || input > next->value_count ||
        next->values[input - 1].variant != 1 ||
        next->values[input - 1].sum_payload)
        return false;
    const NLCheckedNodeView *prior = nl_checked_node_view(f, first.projection);
    const NLCheckedNodeView *write =
        nl_checked_node_view(first.body, first.change.operation);
    const NLSemanticContext *after = first.change.after;
    if (!prior || !write || !after || !prior->has_reference_result ||
        prior->field.index != 1 ||
        prior->field.nominal != projection->field.nominal ||
        prior->field.parent != projection->field.parent ||
        prior->field.parent_incarnation !=
            projection->field.parent_incarnation ||
        !prior->field.parent || prior->field.parent > after->place_count ||
        prior->field.parent > next->place_count ||
        !prior->reference_result.scope ||
        prior->reference_result.scope > next->scope_count ||
        next->scopes[prior->reference_result.scope - 1].active ||
        !projection->reference_result.scope ||
        projection->reference_result.scope == prior->reference_result.scope ||
        projection->reference_result.scope > next->scope_count ||
        !next->scopes[projection->reference_result.scope - 1].active)
        return false;
    const NLSemanticPlaceView root = after->places[prior->field.parent - 1];
    if (memcmp(&root, &next->places[prior->field.parent - 1], sizeof(root)) ||
        root.fixed_field_count != 4)
        return false;
    for (size_t i = 0; i < 4; ++i) {
        const NLPlaceId child = root.fixed_fields[i];
        if (!child || child > after->place_count || child > next->place_count ||
            memcmp(&after->places[child - 1], &next->places[child - 1],
                   sizeof(NLSemanticPlaceView)))
            return false;
    }
    const NLPlaceId prev = root.fixed_fields[1];
    const NLValueId value = next->places[prev - 1].current_value;
    return value && value <= next->value_count &&
           value == write->field.new_value &&
           next->values[value - 1].variant == 1 &&
           !next->values[value - 1].sum_payload &&
           !next->places[prev - 1].payload_occurrence;
#else
    (void)f;
    (void)projection;
    (void)next;
    (void)input;
    return false;
#endif
}

/* Finite caller/validator admission only; never a field capability grant. */
bool nl_scoped_field_index(size_t index)
{
#ifdef NEWLANG_EXPERIMENTAL_SCOPED_PREV_FIELD_CALL
    return index == 0 || index == 1;
#else
    return index == 0;
#endif
}

/* This signature supplies type-only formal checking, never caller authority. */
bool nl_scoped_field_signature(const NLSemanticContext *c, const NLTypeId *p,
                               size_t count, NLTypeId result)
{
#ifdef NEWLANG_EXPERIMENTAL_SCOPED_FIELD_CALL
    if (!c || !p || count != 2 || result != 1 || !p[0] ||
        p[0] > c->type_count || !p[1] || p[1] > c->type_count)
        return false;
    const NLSemanticTypeView ref = c->types[p[0] - 1].view;
    const NLTypeEntry option = c->types[p[1] - 1];
    if (ref.kind != NL_TYPE_REF || ref.is_exclusive ||
        ref.access != NL_ACCESS_WRITE || ref.target != p[1] ||
        option.view.kind != NL_TYPE_SUM || !option.view.is_copy ||
        !option.view.is_discardable || option.view.variant_count != 2 ||
        option.variant_types[0] != 0 || !option.variant_types[1])
        return false;
    const NLSemanticTypeView ptr = c->types[option.variant_types[1] - 1].view;
    return ptr.kind == NL_TYPE_PTR && ptr.is_copy && ptr.is_discardable &&
           nl_recursive_local_type(c, ptr.target) &&
           c->types[ptr.target - 1].view.field_count == 4 &&
           c->types[ptr.target - 1].field_types[0] == p[1];
#else
    (void)c;
    (void)p;
    (void)count;
    (void)result;
    return false;
#endif
}

/* Independently checked formals, or the owned actual body: one replace only.
 * No call, branch, acquisition, result capability or owner operation. */
bool nl_scoped_field_body(const NLCheckedFragment *f,
                          const NLSymbolId parameters[2])
{
#ifdef NEWLANG_EXPERIMENTAL_SCOPED_FIELD_CALL
    if (!f || !parameters)
        return false;
    const NLCheckedNodeView *block = nl_checked_node_view(f, f->root);
    if (!block || block->kind != NL_CHECKED_BLOCK || block->terminates ||
        block->type != 1 || block->item_count != 1)
        return false;
    const NLCheckedNodeView *tail = nl_checked_node_view(f, block->tail);
    const NLCheckedNodeView *old = nl_checked_node_view(f, block->first_item);
    if (!tail || tail->kind != NL_CHECKED_UNIT || !old ||
        old->kind != NL_CHECKED_BINDING)
        return false;
    const NLCheckedNodeView *write = nl_checked_node_view(f, old->initializer);
    if (!write || write->kind != NL_CHECKED_REPLACE ||
        write->argument_count != 2)
        return false;
    const NLCheckedNodeView *ref =
        nl_checked_node_view(f, write->first_argument);
    const NLCheckedNodeView *value =
        ref ? nl_checked_node_view(f, ref->next_argument) : NULL;
    if (!ref || !value || ref->kind != NL_CHECKED_IDENTIFIER ||
        value->kind != NL_CHECKED_IDENTIFIER || ref->symbol != parameters[0] ||
        value->symbol != parameters[1] || value->next_argument != 0)
        return false;
    const NLCheckedNodeView *receiver =
        nl_checked_node_view(f, old->first_argument);
    if (old->argument_count != 1 || !receiver ||
        receiver->kind != NL_CHECKED_RECEIVER ||
        receiver->symbol != old->symbol || !receiver->symbol ||
        receiver->symbol > f->context->binding_count ||
        write->result_count != 1 ||
        f->context->bindings[receiver->symbol - 1].view.value !=
            write->results[0].value)
        return false;
    size_t identifiers = 0, writes = 0, bindings = 0, blocks = 0, units = 0,
           receivers = 0;
    for (size_t n = 1; n <= f->count; ++n) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, n);
        switch (v->kind) {
        case NL_CHECKED_IDENTIFIER:
            ++identifiers;
            break;
        case NL_CHECKED_REPLACE:
            ++writes;
            break;
        case NL_CHECKED_BINDING:
            ++bindings;
            break;
        case NL_CHECKED_RECEIVER:
            ++receivers;
            break;
        case NL_CHECKED_BLOCK:
            ++blocks;
            break;
        case NL_CHECKED_UNIT:
            ++units;
            break;
        default:
            return false;
        }
    }
    return identifiers == 2 && writes == 1 && bindings == 1 && blocks == 1 &&
           units == 1 && receivers == 1;
#else
    (void)f;
    (void)parameters;
    return false;
#endif
}

bool nl_checked_scoped_field_call_view(const NLCheckedFragment *f,
                                       NLCheckedNodeId id,
                                       NLScopedFieldCallView *out)
{
#ifdef NEWLANG_EXPERIMENTAL_SCOPED_FIELD_CALL
    if (!out || !nl_scoped_field_call_member(f, id))
        return false;
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (!call || call->kind != NL_CHECKED_REGISTERED_CALL || !body ||
        body->field_change_count != 1 || !body->field_changes[0])
        return false;
    NLCapturedChangeView change;
    if (!nl_checked_captured_change_view(body, 0, &change))
        return false;
    *out = (NLScopedFieldCallView){body, call->first_argument, change};
    return true;
#else
    (void)f;
    (void)id;
    (void)out;
    return false;
#endif
}
