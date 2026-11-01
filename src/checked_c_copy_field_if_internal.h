#ifndef NEWLANG_CHECKED_C_COPY_FIELD_IF_INTERNAL_H
#define NEWLANG_CHECKED_C_COPY_FIELD_IF_INTERNAL_H

/* Consume an existing two-normal-arm receipt in the fully closed H world.
 * Arm-local IDs never enter Env. The public validator authenticates each
 * Change in its own world; only the common post checkpoint advances Local.
 */
static bool copy_if_scalar(const NLSemanticContext *c, NLValueId id,
                           NLTypeId u8, NLSemanticValueView *out)
{
    return nl_semantic_value_view(c, id, out) && out->type == u8 &&
           (!out->scalar_known || out->scalar_value <= 255) &&
           out->dependencies == NL_DEPENDENCY_FREE &&
           !out->value_dependency_count && !out->reference_count &&
           !out->reference.place && !out->reference.scope && !out->domain &&
           !out->slot_place && !out->allocation_region &&
           !out->occupancy.region && !out->field_count && !out->sum_payload &&
           !out->sum_owner;
}
typedef struct {
    const Local *local;
    size_t literal;
} CopyIfInput;

static bool copy_if_input(Emit *e, const Env *env, const NLIfFieldView *view,
                          const NLCheckedFragment *arm,
                          const NLCheckedNodeView *n, CopyIfInput *out)
{
    if (!n || n->type != e->u8 || n->result_count != 1 || n->terminates)
        return false;
    if (n->kind == NL_CHECKED_U8_LITERAL)
        return copy_if_literal(arm, n, e->u8, 255, &out->literal);
    NLSemanticBindingView binding, caller;
    NLSemanticValueView value, actual;
    if (n->kind != NL_CHECKED_IDENTIFIER || n->value_use != NL_VALUE_COPIED ||
        !(out->local = local(env, n->symbol, e->u8)) ||
        !nl_semantic_binding_view(view->entry, n->symbol, &binding) ||
        !nl_semantic_binding_view(nl_checked_context(env->artifact), n->symbol,
                                  &caller) ||
        binding.type != e->u8 || caller.type != e->u8 ||
        binding.availability != NL_AVAILABLE || binding.value != caller.value ||
        !copy_plain(view->entry, binding.value, e->u8, &value) ||
        !copy_plain(nl_checked_context(arm), n->results[0].value, e->u8,
                    &actual) ||
        value.scalar_value != actual.scalar_value)
        return false;
    /* Entry is the authenticated use point. The completed function context
     * has already retired lexical bindings; it supplies identity, not a new
     * availability claim for that earlier operation. */
    return true;
}

static bool copy_field_if(Emit *e, Env *env, NLCheckedNodeId id,
                          const NLCheckedNodeView *choice,
                          const NLIfFieldView *view, Value *out)
{
    if (!copy_closed_world(e, env))
        return reject(e);
    NLCheckStatus status =
        nl_checked_if_field_validate(env->artifact, id, view);
    if (status != NL_CHECK_OK) {
        if (status == NL_CHECK_OUT_OF_MEMORY)
            e->status = NL_NODE_C_OUT_OF_MEMORY;
        else if (status == NL_CHECK_RESOURCE_LIMIT)
            e->status = NL_NODE_C_RESOURCE_LIMIT;
        return reject(e);
    }
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE),
                   boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    const NLCheckedNodeView *condition = get(env, choice->initializer),
                            *left = condition
                                        ? get(env, condition->first_argument)
                                        : NULL,
                            *right =
                                left ? get(env, left->next_argument) : NULL;
    size_t lhs, rhs, truth;
    NLSemanticValueView result;
    if (choice->type != e->u8 || choice->result_count != 1 ||
        choice->results[0].value != view->result.value ||
        !copy_if_scalar(view->post, view->result.value, e->u8, &result) ||
        result.scalar_known || !condition || condition->argument_count != 2 ||
        (condition->kind != NL_CHECKED_USIZE_LESS_THAN &&
         condition->kind != NL_CHECKED_USIZE_EQUALITY) ||
        !left || left->kind != NL_CHECKED_USIZE_LITERAL || !right ||
        right->kind != NL_CHECKED_USIZE_LITERAL || right->next_argument ||
        !copy_if_literal(env->artifact, left, usize, SIZE_MAX, &lhs) ||
        !copy_if_literal(env->artifact, right, usize, SIZE_MAX, &rhs) ||
        !copy_if_literal(env->artifact, condition, boolean, 1, &truth) ||
        truth != (size_t)(condition->kind == NL_CHECKED_USIZE_EQUALITY
                              ? lhs == rhs
                              : lhs < rhs) ||
        view->index > 1 || view->nominal != e->copy_type)
        return reject(e);
    CopyIfInput inputs[2] = {0};
    Local *original = NULL;
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = view->arms[a].artifact;
        const NLCheckedNodeView *root =
            nl_checked_node_view(arm, nl_checked_root(arm));
        const NLCheckedNodeView *block =
            root ? nl_checked_node_view(arm, root->initializer) : NULL;
        const NLCheckedNodeView *loan =
            block ? nl_checked_node_view(arm, block->tail) : NULL;
        const NLCheckedNodeView *body =
            loan ? nl_checked_node_view(arm, loan->initializer) : NULL;
        const NLCheckedNodeView *change =
            body ? nl_checked_node_view(arm, body->tail) : NULL;
        const NLCheckedNodeView *ref =
            change ? nl_checked_node_view(arm, change->first_argument) : NULL;
        const NLCheckedNodeView *next =
            ref ? nl_checked_node_view(arm, ref->next_argument) : NULL;
        NLIfFieldEvent event;
        if (view->arms[a].event_count != 1 || view->arms[a].change_count != 1 ||
            !root || !block || block->kind != NL_CHECKED_BLOCK ||
            block->item_count || block->first_item || !loan ||
            loan->kind != NL_CHECKED_LOAN_HEADER || !body ||
            body->kind != NL_CHECKED_BLOCK || body->item_count ||
            body->first_item || !change || change->kind != NL_CHECKED_REPLACE ||
            !ref || !next || next->next_argument ||
            !nl_checked_if_field_event(env->artifact, id, a, 0, &event) ||
            !event.change || nl_checked_node_view(arm, event.node) != change ||
            event.field.parent != view->root ||
            event.field.child != view->child ||
            event.field.base != view->base ||
            event.field.index != view->index || root->result_count != 1 ||
            block->result_count != 1 || loan->result_count != 1 ||
            body->result_count != 1 || root->results[0].value != event.result ||
            block->results[0].value != event.result ||
            loan->results[0].value != event.result ||
            body->results[0].value != event.result ||
            !copy_if_input(e, env, view, arm, next, &inputs[a]))
            return reject(e);
        Local *l = copy_field(e, env, &event.field);
        if (!l || (original && original != l) ||
            event.field.old_value != l->copy_value[view->index])
            return reject(e);
        original = l;
    }
    NLSemanticPlaceView before[3], post[3];
    const NLPlaceId places[3] = {view->root, view->child, view->sibling};
    const size_t index = view->index;
    for (size_t i = 0; i < 3; ++i)
        if (!nl_semantic_place_view(view->entry, places[i], &before[i]) ||
            !nl_semantic_place_view(view->post, places[i], &post[i]))
            return reject(e);
    if (before[0].current_fact != original->copy_fact ||
        before[1].current_fact != original->copy_fact_child[index] ||
        before[1].current_value != original->copy_value[index] ||
        before[2].current_fact != original->copy_fact_child[1 - index] ||
        before[2].current_value != original->copy_value[1 - index] ||
        memcmp(&before[2], &post[2], sizeof(before[2])) ||
        !post[0].current_fact || !post[1].current_fact ||
        post[0].current_fact == before[0].current_fact ||
        post[1].current_fact == before[1].current_fact)
        return reject(e);
    const size_t l = ++e->temp, r = ++e->temp;
    out->type = e->u8;
    out->temp = ++e->temp;
    if (!put(e,
             "_Static_assert(SIZE_MAX == UINTMAX_C(%zu), "
             "\"checked usize target width\");\n"
             "const size_t nl_v_%zu=(size_t)UINTMAX_C(%zu);\n"
             "const size_t nl_v_%zu=(size_t)UINTMAX_C(%zu);\n"
             "uint8_t nl_v_%zu;\nif(nl_v_%zu %s nl_v_%zu){\n",
             SIZE_MAX, l, lhs, r, rhs, out->temp, l,
             condition->kind == NL_CHECKED_USIZE_EQUALITY ? "==" : "<", r))
        return false;
    for (size_t a = 0; a < 2; ++a) {
        const size_t incoming = ++e->temp;
        if (inputs[a].local) {
            if (!put(e, "uint8_t nl_v_%zu=nl_b_%zu_%zu;\n", incoming,
                     inputs[a].local->frame, inputs[a].local->symbol))
                return false;
        } else if (!put(e, "uint8_t nl_v_%zu=%zu;\n", incoming,
                        inputs[a].literal))
            return false;
        if (!put(e,
                 "nl_v_%zu=nl_b_%zu_%zu.f%zu;\n"
                 "nl_b_%zu_%zu.f%zu=nl_v_%zu;\n"
                 "NL_MIXED_COPY_STORE(&nl_b_%zu_%zu.f%zu,nl_v_%zu,nl_v_%zu);\n",
                 out->temp, original->frame, original->symbol, index,
                 original->frame, original->symbol, index, incoming,
                 original->frame, original->symbol, index, out->temp,
                 incoming) ||
            !put(e, a == 0 ? "}else{\n" : "}\n"))
            return false;
    }
    original->copy_fact = post[0].current_fact;
    original->copy_fact_child[index] = post[1].current_fact;
    original->copy_value[index] = post[1].current_value;
    return true;
}
#endif
