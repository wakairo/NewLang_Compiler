#ifndef NEWLANG_CHECKED_C_COPY_INTERNAL_H
#define NEWLANG_CHECKED_C_COPY_INTERNAL_H
/* Backend-only consumer of existing source-issued Copy observations. No
 * semantic state or capabilities are created by this carrier ledger. */
static bool copy_plain(const NLSemanticContext *c, NLValueId id, NLTypeId u8,
                       NLSemanticValueView *out)
{
    return nl_semantic_value_view(c, id, out) && out->type == u8 &&
           out->scalar_known && out->scalar_value <= 255 &&
           out->dependencies == NL_DEPENDENCY_FREE &&
           !out->value_dependency_count && !out->reference_count &&
           !out->reference.place && !out->domain && !out->slot_place &&
           !out->allocation_region && !out->field_count;
}
static bool copy_shape(Emit *e, const NLSemanticContext *c, NLTypeId type)
{
    NLSemanticTypeView t;
    if (!type || type == e->node || !nl_semantic_type_view(c, type, &t) ||
        t.kind != NL_TYPE_NOMINAL || t.field_count != 2 || !t.is_copy ||
        !t.is_discardable || nl_semantic_recursive_local_type(c, type))
        return false;
    for (size_t i = 0; i < 2; ++i) {
        NLAggregateField f;
        if (!nl_semantic_aggregate_field_view(c, type, i, &f) ||
            f.type != e->u8)
            return false;
    }
    return true;
}
static bool copy_call_proof(Emit *e, const NLCheckedFragment *f,
                            NLCheckedNodeId id, NLCopyBodyParameters *p)
{
    NLCopyBodyView v;
    NLCopyBodyEventView event;
    if (!nl_checked_copy_body_view(f, id, &v) ||
        !nl_checked_copy_body_parameters(f, id, p))
        return reject(e);
    NLCheckStatus s = nl_checked_copy_body_validate(f, id, &v);
    if (s == NL_CHECK_OK)
        s = nl_checked_copy_body_parameters_validate(f, id, p);
    if (s != NL_CHECK_OK) {
        if (s == NL_CHECK_OUT_OF_MEMORY)
            e->status = NL_NODE_C_OUT_OF_MEMORY;
        else if (s == NL_CHECK_RESOURCE_LIMIT)
            e->status = NL_NODE_C_RESOURCE_LIMIT;
        return reject(e);
    }
    if (v.event_count != 1 || !nl_checked_copy_body_event(f, id, 0, &event) ||
        event.kind != NL_COPY_BODY_CHANGE ||
        !copy_shape(e, nl_checked_context(f), p->field.nominal) ||
        (e->copy_type && e->copy_type != p->field.nominal) ||
        (e->copy_function && e->copy_function != v.function))
        return reject(e);
    const NLCheckedNodeView *root = nl_checked_node_view(v.body, v.root);
    const NLCheckedNodeView *binding =
        root ? nl_checked_node_view(v.body, root->first_item) : NULL;
    const NLCheckedNodeView *ret =
        binding ? nl_checked_node_view(v.body, binding->next_item) : NULL;
    const NLCheckedNodeView *replace =
        binding ? nl_checked_node_view(v.body, binding->initializer) : NULL;
    const NLCheckedNodeView *old =
        ret ? nl_checked_node_view(v.body, ret->initializer) : NULL;
    const NLCheckedNodeView *ref =
        replace ? nl_checked_node_view(v.body, replace->first_argument) : NULL;
    const NLCheckedNodeView *next =
        ref ? nl_checked_node_view(v.body, ref->next_argument) : NULL;
    if (!root || root->kind != NL_CHECKED_BLOCK || root->item_count != 2 ||
        root->tail || !binding || binding->kind != NL_CHECKED_BINDING || !ret ||
        ret->kind != NL_CHECKED_RETURN || ret->next_item || !replace ||
        replace->kind != NL_CHECKED_REPLACE || replace->argument_count != 2 ||
        event.node != binding->initializer || !old ||
        old->kind != NL_CHECKED_IDENTIFIER || old->symbol != binding->symbol ||
        old->value_use != NL_VALUE_COPIED || !ref ||
        ref->kind != NL_CHECKED_IDENTIFIER || ref->symbol != p->parameters[0] ||
        !next || next->kind != NL_CHECKED_IDENTIFIER ||
        next->symbol != p->parameters[1] || next->next_argument ||
        replace->type != e->u8 || ret->returned.type != e->u8 ||
        ret->returned.value != v.result.value)
        return reject(e);
    e->copy_type = p->field.nominal;
    e->copy_function = v.function;
    return true;
}
static bool copy_prepare_at(Emit *e, const NLCheckedFragment *body,
                            size_t depth)
{
    if (!body || depth > 2)
        return reject(e);
    for (size_t id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, id);
        if (node && node->kind == NL_CHECKED_MATCH && node->item_count == 2) {
            const NLCheckedNodeView *trial =
                nl_checked_node_view(body, node->initializer);
            if (trial && trial->kind == NL_CHECKED_TRY_ALLOCATE_ONE) {
                for (size_t arm = 0; arm < 2; ++arm) {
                    const NLCheckedFragment *a =
                        nl_checked_match_arm(body, id, arm);
                    if (!copy_prepare_at(e, a, depth + 1))
                        return false;
                }
            }
        }
        NLCopyBodyParameters p;
        if (!nl_checked_copy_body_parameters(body, id, &p))
            continue;
        if (!e->two_heap || !e->live_tail || !copy_call_proof(e, body, id, &p))
            return reject(e);
        if (e->copy_issuer && e->copy_issuer != body)
            return reject(e);
        if (!e->copy_anchor) {
            e->copy_anchor = p.entry;
            e->copy_issuer = body;
        }
        ++e->copy_expected_calls;
    }
    return true;
}
static bool copy_prepare(Emit *e, const NLCheckedFragment *body)
{
    return copy_prepare_at(e, body, 0);
}
/* A nested Copy original is allowed only in the exact publicly validated
 * caller fragment after its one producer has returned the original H owner.
 * All fields still use that fragment's entry/handoff identities. */
static bool copy_carrier_world(Emit *e, const Env *env)
{
    return env->artifact == e->copy_issuer &&
           (!e->allocations || (e->allocations == 2 && e->producer_calls == 1 &&
                                e->owner_calls == 0));
}
static bool copy_bind(Emit *e, Env *env, Local *l, NLCheckedNodeId constructor)
{
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    NLSemanticBindingView b;
    NLSemanticPlaceView root;
    const NLCheckedNodeView *init = get(env, constructor);
    if (!copy_carrier_world(e, env) || !init ||
        init->kind != NL_CHECKED_AGGREGATE ||
        !nl_semantic_binding_view(e->copy_anchor, l->symbol, &b) ||
        b.type != e->copy_type ||
        !nl_semantic_place_view(e->copy_anchor, b.place, &root) ||
        root.type != e->copy_type || root.incarnation != l->incarnation ||
        b.place != l->place || !root.live || !root.independent_root ||
        !root.implicit_local || root.governing_domain || ++e->copy_roots > 2)
        return reject(e);
    l->copy_fact = root.current_fact;
    NLCheckedNodeId member = init->first_argument;
    bool seen[2] = {0};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *field = get(env, member);
        const NLCheckedNodeView *value =
            field ? get(env, field->initializer) : NULL;
        NLSemanticPlaceView child;
        NLSemanticValueView actual, expected;
        if (!field || field->field_index > 1 || seen[field->field_index] ||
            !value || value->result_count != 1 ||
            !nl_semantic_place_view(e->copy_anchor,
                                    root.fixed_fields[field->field_index],
                                    &child) ||
            child.parent_aggregate != b.place || !child.live ||
            !copy_plain(c, value->results[0].value, e->u8, &actual) ||
            !copy_plain(e->copy_anchor, child.current_value, e->u8,
                        &expected) ||
            actual.scalar_value != expected.scalar_value)
            return reject(e);
        size_t index = field->field_index;
        seen[index] = true;
        l->copy_child[index] = root.fixed_fields[index];
        l->copy_inc[index] = child.incarnation;
        l->copy_fact_child[index] = child.current_fact;
        l->copy_value[index] = child.current_value;
        member = field->next_argument;
    }
    return member == 0 ? true : reject(e);
}
static Local *copy_field(Emit *e, Env *env, const NLCheckedField *f)
{
    if (!copy_carrier_world(e, env) || !f->present ||
        !f->dependency_compatible || f->nominal != e->copy_type ||
        f->type != e->u8 || f->index > 1)
        return NULL;
    for (size_t i = 0; i < env->count; ++i) {
        Local *l = &env->locals[i];
        if (l->symbol == f->base && l->type == e->copy_type &&
            l->place == f->parent && l->incarnation == f->parent_incarnation &&
            l->copy_fact == f->parent_fact &&
            l->copy_child[f->index] == f->child &&
            l->copy_inc[f->index] == f->child_incarnation &&
            l->copy_fact_child[f->index] == f->child_fact)
            return l;
    }
    return NULL;
}
static bool copy_constructor(Emit *e, Env *env, const NLCheckedNodeView *v,
                             Value *out)
{
    if (!copy_carrier_world(e, env) || v->type != e->copy_type ||
        v->argument_count != 2)
        return reject(e);
    Value values[2] = {0};
    bool seen[2] = {0};
    NLCheckedNodeId field = v->first_argument;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *n = get(env, field);
        if (!n || n->kind != NL_CHECKED_AGGREGATE_FIELD || n->type != e->u8 ||
            n->field_index > 1 || seen[n->field_index])
            return reject(e);
        seen[n->field_index] = true;
        if (!expr(e, env, n->initializer, &values[n->field_index]) ||
            values[n->field_index].type != e->u8)
            return reject(e);
        field = n->next_argument;
    }
    if (field)
        return reject(e);
    out->type = e->copy_type;
    out->temp = ++e->temp;
    return put(e, "nl_copy_record nl_v_%zu = {nl_v_%zu,nl_v_%zu};\n", out->temp,
               values[0].temp, values[1].temp);
}
static bool copy_loan(Emit *e, Env *env, const NLCheckedNodeView *v, Value *out)
{
    Local *l = copy_field(e, env, &v->field);
    const NLCheckedNodeView *body = get(env, v->initializer),
                            *tail = body ? get(env, body->tail) : NULL;
    const NLCheckedLoanPlan *loan = &v->loan;
    if (!l || !loan->implicit_local || loan->from_ptr || loan->is_exclusive ||
        loan->access != NL_ACCESS_WRITE || v->field.access != NL_ACCESS_WRITE ||
        !loan->body_nonescape_proved || !loan->normal_result_forwarded ||
        !loan->scope || loan->source != l->symbol ||
        loan->place != v->field.child ||
        loan->incarnation != v->field.child_incarnation || !body ||
        body->kind != NL_CHECKED_BLOCK || body->item_count || !tail ||
        body->terminates || v->type != e->u8 || v->result_count != 1 ||
        tail->result_count != 1 ||
        tail->results[0].value != v->results[0].value)
        return reject(e);
    const NLCheckedNodeView *ref = get(env, tail->first_argument),
                            *next = ref ? get(env, ref->next_argument) : NULL;
    NLSemanticTypeView reference;
    if (!ref || ref->kind != NL_CHECKED_IDENTIFIER ||
        ref->symbol != loan->ref_symbol || !next || next->next_argument ||
        !nl_semantic_type_view(nl_checked_context(env->artifact), ref->type,
                               &reference) ||
        reference.kind != NL_TYPE_REF || reference.target != e->u8 ||
        reference.access != NL_ACCESS_WRITE || reference.is_exclusive)
        return reject(e);
    const size_t index = v->field.index;
    NLCopyBodyParameters p;
    NLSemanticPlaceView after, parent, sibling;
    bool call = tail->kind == NL_CHECKED_REGISTERED_CALL;
    if (call) {
        if (!copy_call_proof(e, env->artifact, body->tail, &p) ||
            get(env, p.loan) != v || p.arguments[0] != tail->first_argument ||
            p.arguments[1] != ref->next_argument ||
            !copy_field(e, env, &p.field) ||
            !nl_semantic_place_view(p.entry, p.field.child, &after) ||
            after.current_value != l->copy_value[index] ||
            !nl_semantic_place_view(p.handoff, p.field.parent, &parent) ||
            !nl_semantic_place_view(p.handoff, p.field.child, &after) ||
            !nl_semantic_place_view(p.handoff, l->copy_child[1 - index],
                                    &sibling) ||
            sibling.current_fact != l->copy_fact_child[1 - index] ||
            sibling.current_value != l->copy_value[1 - index])
            return reject(e);
    } else if (tail->kind != NL_CHECKED_REPLACE ||
               tail->field.access != NL_ACCESS_WRITE ||
               !copy_field(e, env, &tail->field) ||
               tail->field.old_value != l->copy_value[index] ||
               tail->field.old_value != tail->results[0].value ||
               !tail->field.parent_post_fact || !tail->field.child_post_fact ||
               tail->field.parent_post_fact == l->copy_fact ||
               tail->field.child_post_fact == l->copy_fact_child[index])
        return reject(e);
    if (!call) {
        NLSemanticValueView value;
        if (!nl_semantic_value_view(nl_checked_context(env->artifact),
                                    ref->results[0].value, &value) ||
            value.reference.place != v->field.child ||
            value.reference.incarnation != v->field.child_incarnation ||
            value.reference.scope != loan->scope ||
            value.reference.provenance != NL_PROVENANCE_VALID ||
            !value.reference.readable || !value.reference.writable)
            return reject(e);
    }
    Value incoming = {0};
    if (!expr(e, env, ref->next_argument, &incoming) || incoming.type != e->u8)
        return reject(e);
    out->type = e->u8;
    out->temp = ++e->temp;
    if (call) {
        if (!put(e,
                 "uint8_t nl_v_%zu = "
                 "nl_copy_state_%zu(&nl_b_%zu_%zu.f%zu,nl_v_%zu);\n",
                 out->temp, e->copy_function, l->frame, l->symbol, index,
                 incoming.temp))
            return false;
        ++e->copy_calls;
        l->copy_fact = parent.current_fact;
        l->copy_fact_child[index] = after.current_fact;
        l->copy_value[index] = after.current_value;
    } else {
        if (!put(e,
                 "uint8_t nl_v_%zu = "
                 "nl_b_%zu_%zu.f%zu;\nnl_b_%zu_%zu.f%zu=nl_v_%zu;\nNL_MIXED_"
                 "COPY_STORE(&nl_b_%zu_%zu.f%zu,nl_v_%zu,nl_v_%zu);\n",
                 out->temp, l->frame, l->symbol, index, l->frame, l->symbol,
                 index, incoming.temp, l->frame, l->symbol, index, out->temp,
                 incoming.temp))
            return false;
        l->copy_fact = tail->field.parent_post_fact;
        l->copy_fact_child[index] = tail->field.child_post_fact;
        l->copy_value[index] = tail->field.new_value;
    }
    return true;
}
#endif
