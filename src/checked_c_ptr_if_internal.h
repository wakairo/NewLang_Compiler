#ifndef NEWLANG_CHECKED_C_PTR_IF_INTERNAL_H
#define NEWLANG_CHECKED_C_PTR_IF_INTERNAL_H

/* A physical consumer of the existing sealed CHECK subtree. These counters
 * describe emitted operations; they do not prove a pointer, original or loan.
 * Effect-free ptr/initial-link observations keep the old topology protocol
 * unchanged for all existing entries. */
static bool ptr_copy_enabled(const Emit *e)
{
    return e->scalar_issued && e->allocated && e->two_heap && !e->copy_type &&
           !e->live_tail && !e->custody && !e->current;
}
static bool ptr_copy_value(const NLSemanticContext *c, NLValueId id,
                            NLTypeId type, NLSemanticValueView *v)
{
    return nl_semantic_value_view(c, id, v) && v->type == type &&
           v->dependencies == NL_DEPENDENCY_FREE &&
           !v->value_dependency_count && !v->reference_count &&
           v->reference.provenance == NL_PROVENANCE_VALID &&
           v->reference.readable && v->reference.place &&
           v->reference.incarnation && !v->reference.scope &&
           !v->reference.occurrence_dependency && !v->domain &&
           !v->allocation_region && !v->slot_place && !v->occupancy.region &&
           !v->field_count && !v->sum_payload && !v->scalar_known;
}
static const NLCheckedNodeView *
ptr_copy_observation(const Emit *e, const Env *env, NLCheckedNodeId id)
{
    const NLCheckedNodeView *b = get(env, id);
    const NLCheckedNodeView *n = b ? get(env, b->first_item) : NULL;
    const NLCheckedNodeView *r = n ? get(env, n->first_argument) : NULL;
    const NLCheckedNodeView *ref = n ? get(env, n->initializer) : NULL;
    const NLCheckedNodeView *tail = b ? get(env, b->tail) : NULL;
    if (!ptr_copy_enabled(e) || !b || b->kind != NL_CHECKED_BLOCK ||
        b->terminates || b->item_count != 1 || b->result_count != 1 || !n ||
        n->kind != NL_CHECKED_BINDING || n->next_item ||
        n->argument_count != 1 || !r || r->kind != NL_CHECKED_RECEIVER ||
        r->symbol != n->symbol || r->next_argument || !ref ||
        ref->kind != NL_CHECKED_REF_FROM_PTR || !ref->has_reference_result ||
        !tail || tail->terminates || tail->result_count != 1 ||
        tail->type != b->type)
        return NULL;
    const NLCheckedNodeView *operand = get(env, tail->first_argument);
    if (tail->kind == NL_CHECKED_LINK_READ) {
        if (tail->type != e->option || tail->argument_count != 1 ||
            tail->field.index || tail->field.access != NL_ACCESS_READ ||
            tail->field.base != n->symbol ||
            ref->reference_result.writable || !operand ||
            operand->kind != NL_CHECKED_FIELD_REF)
            return NULL;
    } else if (tail->kind != NL_CHECKED_PTR_FROM_REF ||
               tail->type != e->ptr || tail->argument_count != 1 || !operand ||
               operand->kind != NL_CHECKED_IDENTIFIER ||
               operand->symbol != n->symbol || operand->next_argument) {
        return NULL;
    }
    return ref;
}
static bool ptr_copy_from_ref(Emit *e, Env *env, const NLCheckedNodeView *v,
                               Value *out)
{
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    const NLCheckedNodeView *arg = get(env, v->first_argument);
    NLSemanticTypeView type;
    NLSemanticValueView r, p;
    Value actual = {0};
    if (!ptr_copy_enabled(e) || !env->ptr_observation ||
        ++e->ptr_roundtrips != 1 || v->type != e->ptr ||
        v->argument_count != 1 || v->result_count != 1 || !arg ||
        arg->next_argument || !v->has_reference_result ||
        !nl_semantic_type_view(c, arg->type, &type) ||
        type.kind != NL_TYPE_REF || type.target != e->node || type.is_exclusive ||
        !selected_reference(env, arg, &r) ||
        r.reference.scope != env->stability_scope ||
        !same_reference(r.reference, env->ptr_observation->reference_result) ||
        !ptr_copy_value(c, v->results[0].value, e->ptr, &p) ||
        !same_reference(p.reference, v->reference_result))
        return reject(e);
    NLReferenceFacts plain = r.reference;
    plain.scope = 0;
    if (!same_reference(plain, p.reference) ||
        !expr(e, env, v->first_argument, &actual))
        return reject(e);
    out->temp = ++e->temp;
    return put(e, "const nl_node *nl_v_%zu = nl_v_%zu;\n",
               out->temp, actual.temp);
}
static bool ptr_copy_if(Emit *e, const Env *env, NLCheckedNodeId id,
                         const NLCheckedNodeView *v, Value *out)
{
    const NLCheckedFragment *f = env->artifact;
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE),
                   boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    const NLCheckedNodeView *condition = get(env, v->initializer);
    if (!ptr_copy_enabled(e) || ++e->ptr_ifs != 1 || v->terminates ||
        v->normal_arms != 2 || v->item_count != 2 || v->tail ||
        v->has_scalar_result || v->result_count != 1 || !condition ||
        (condition->kind != NL_CHECKED_USIZE_EQUALITY &&
         condition->kind != NL_CHECKED_USIZE_LESS_THAN) ||
        condition->type != boolean || condition->argument_count != 2 ||
        condition->result_count != 1 || condition->terminates)
        return reject(e);
    const NLCheckedNodeView *left = get(env, condition->first_argument),
                            *right = left ? get(env, left->next_argument) : NULL;
    size_t lhs, rhs;
    NLSemanticValueView result;
    if (!left || !right || left->kind != NL_CHECKED_USIZE_LITERAL ||
        right->kind != NL_CHECKED_USIZE_LITERAL || right->next_argument ||
        !copy_if_literal(f, left, usize, SIZE_MAX, &lhs) ||
        !copy_if_literal(f, right, usize, SIZE_MAX, &rhs) ||
        !ptr_copy_value(c, v->results[0].value, e->ptr, &result))
        return reject(e);
    const Local *carriers[2];
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, a);
        const NLCheckedNodeView *root =
            arm ? nl_checked_node_view(arm, nl_checked_root(arm)) : NULL;
        const NLCheckedNodeView *block =
            root ? nl_checked_node_view(arm, root->initializer) : NULL;
        const NLCheckedNodeView *value =
            block ? nl_checked_node_view(arm, block->tail) : NULL;
        NLSemanticBindingView parent, branch;
        NLSemanticValueView p;
        if (!arm || arm->if_parent != f || arm->if_parent_node != id ||
            arm->if_arm_index != a || !root ||
            root->kind != NL_CHECKED_IF_ARM || root->terminates ||
            root->result_count != 1 || !block ||
            block->kind != NL_CHECKED_BLOCK || block->item_count ||
            block->first_item || block->terminates || block->result_count != 1 ||
            !value || value->kind != NL_CHECKED_IDENTIFIER ||
            value->type != e->ptr || value->value_use != NL_VALUE_COPIED ||
            value->result_count != 1 ||
            root->results[0].value != value->results[0].value ||
            block->results[0].value != value->results[0].value ||
            !ptr_copy_value(nl_checked_context(arm), value->results[0].value,
                            e->ptr, &p) ||
            !same_reference(result.reference, p.reference) ||
            !nl_semantic_binding_view(c, value->symbol, &parent) ||
            !nl_semantic_binding_view(nl_checked_context(arm), value->symbol,
                                      &branch) ||
            parent.type != e->ptr || branch.type != e->ptr ||
            parent.value != branch.value || parent.place != branch.place ||
            !(carriers[a] = local(env, value->symbol, e->ptr)))
            return reject(e);
    }
    const size_t l = ++e->temp, r = ++e->temp;
    out->temp = ++e->temp;
    return put(e,
               "_Static_assert(SIZE_MAX == UINTMAX_C(%zu),\"usize width\");\n"
               "const size_t nl_v_%zu=(size_t)UINTMAX_C(%zu);\n"
               "const size_t nl_v_%zu=(size_t)UINTMAX_C(%zu);\n"
               "const nl_node *nl_v_%zu;\n"
               "if(nl_v_%zu %s nl_v_%zu){\n"
               "nl_v_%zu=nl_b_%zu_%zu;NL_NODE_ARM(0,nl_v_%zu);\n"
               "}else{\nnl_v_%zu=nl_b_%zu_%zu;NL_NODE_ARM(1,nl_v_%zu);\n}\n",
               SIZE_MAX, l, lhs, r, rhs, out->temp, l,
               condition->kind == NL_CHECKED_USIZE_EQUALITY ? "==" : "<", r,
               out->temp, carriers[0]->frame, carriers[0]->symbol, out->temp,
               out->temp, carriers[1]->frame, carriers[1]->symbol, out->temp);
}
#endif
