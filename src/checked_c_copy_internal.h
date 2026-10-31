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
/* Consume the already-current body, never issue or cache authority. Every
 * item is a pure u8 binding except the optional final RETURN. Trace that
 * result through earlier bindings to this exact Change's old result; equal
 * scalars, incoming parameters and another Change are not that origin.
 * The decreasing prefix bounds the walk without another ledger/allocation. */
static bool copy_old_result(const NLCopyBodyView *v,
                            const NLCopyBodyEventView *event, NLTypeId u8)
{
    const NLCheckedFragment *f = v->body;
    const NLCheckedNodeView *root = nl_checked_node_view(f, v->root);
    if (!root || root->kind != NL_CHECKED_BLOCK ||
        root->item_count > nl_checked_node_count(f) || v->result.type != u8)
        return false;
    NLCheckedNodeId result = root->tail, item = root->first_item;
    size_t limit = root->item_count, changes = 0;
    for (size_t i = 0; i < root->item_count; ++i) {
        const NLCheckedNodeView *b = nl_checked_node_view(f, item);
        if (!b)
            return false;
        if (!root->tail && i + 1 == root->item_count) {
            if (b->kind != NL_CHECKED_RETURN || !b->terminates ||
                b->returned.type != u8 || b->returned.value != v->result.value)
                return false;
            result = b->initializer;
            limit = i;
        } else {
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, b->initializer);
            if (b->kind != NL_CHECKED_BINDING || !b->symbol || !init ||
                init->type != u8 || init->result_count != 1)
                return false;
            if (b->initializer == event->node)
                ++changes;
            else if (init->kind != NL_CHECKED_IDENTIFIER ||
                     init->value_use != NL_VALUE_COPIED)
                return false;
        }
        item = b->next_item;
    }
    const NLCheckedNodeView *end = nl_checked_node_view(f, result);
    if (item || !end || end->type != u8 || end->result_count != 1 ||
        end->results[0].value != v->result.value ||
        (root->tail && (root->terminates || root->result_count != 1 ||
                        root->results[0].value != v->result.value)) ||
        (!root->tail && !root->terminates))
        return false;
    changes += result == event->node;
    if (changes != 1)
        return false;
    while (result != event->node) {
        const NLCheckedNodeView *id = nl_checked_node_view(f, result);
        if (!id || id->kind != NL_CHECKED_IDENTIFIER || id->type != u8 ||
            id->value_use != NL_VALUE_COPIED)
            return false;
        NLCheckedNodeId initializer = 0;
        size_t earlier = limit;
        item = root->first_item;
        for (size_t i = 0; i < limit; ++i) {
            const NLCheckedNodeView *b = nl_checked_node_view(f, item);
            if (!b || b->kind != NL_CHECKED_BINDING)
                return false;
            if (b->symbol == id->symbol) {
                if (initializer)
                    return false;
                initializer = b->initializer;
                earlier = i;
            }
            item = b->next_item;
        }
        if (!initializer)
            return false;
        result = initializer;
        limit = earlier;
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
        (e->copy_second_function && e->copy_function != v.function &&
         e->copy_second_function != v.function))
        return reject(e);
    const NLCheckedNodeView *replace = nl_checked_node_view(v.body, event.node);
    const NLCheckedNodeView *ref =
        replace ? nl_checked_node_view(v.body, replace->first_argument) : NULL;
    const NLCheckedNodeView *next =
        ref ? nl_checked_node_view(v.body, ref->next_argument) : NULL;
    if (!copy_old_result(&v, &event, e->u8) || !replace ||
        replace->kind != NL_CHECKED_REPLACE || replace->argument_count != 2 ||
        replace->result_count != 1 || replace->results[0].type != e->u8 ||
        replace->results[0].value != event.result || !ref ||
        ref->kind != NL_CHECKED_IDENTIFIER || ref->symbol != p->parameters[0] ||
        !next || next->kind != NL_CHECKED_IDENTIFIER ||
        next->symbol != p->parameters[1] || next->next_argument ||
        replace->type != e->u8)
        return reject(e);
    e->copy_type = p->field.nominal;
    if (!e->copy_function)
        e->copy_function = v.function;
    else if (e->copy_function != v.function)
        e->copy_second_function = v.function;
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
/* Both calls belong to one source-issued caller already visited by prepare.
 * A nested caller still has to pass the serializer's H carrier-world checks.
 * Distinct lexical originals never share child authority: authenticate each
 * through that caller's actual entry and the evolving Local ledger. Main's
 * prefix and the existing same-original cases retain their old conditions. */
/* Schedule only a source-owned outer H MATCH followed by two fresh literal
 * originals. This is no completion proof: allocated_match must separately
 * validate every arm, including None cleanup, before copy_closed_world can
 * admit the common main continuation. No H-success snapshot is reused. */
static bool copy_closed_plan(Emit *e, const NLCheckedFragment *body)
{
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    const NLCheckedNodeView *statement =
        root ? nl_checked_node_view(body, root->first_item) : NULL;
    const NLCheckedNodeView *match =
        statement ? nl_checked_node_view(body, statement->initializer) : NULL;
    if (body != e->copy_issuer ||
        body != nl_checked_call_body(e->origin_root,
                                     nl_checked_root(e->origin_root)) ||
        e->copy_expected_roots != 2 || e->copy_expected_calls != 2 || !root ||
        root->kind != NL_CHECKED_BLOCK ||
        (root->terminates &&
         !final_unit_return(body, nl_checked_root(body), e->unit)) ||
        !statement || statement->kind != NL_CHECKED_STATEMENT || !match ||
        match->kind != NL_CHECKED_MATCH || !match->normal_frame_unchanged ||
        match->captured_frame_closed || match->normal_arms != 2 ||
        match->item_count != 2 || match->terminates || match->type != e->unit)
        return false;
    NLCheckedNodeId item = statement->next_item;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *binding = nl_checked_node_view(body, item),
                                *init = binding
                                            ? nl_checked_node_view(
                                                  body, binding->initializer)
                                            : NULL;
        if (!binding || binding->kind != NL_CHECKED_BINDING || !init ||
            init->kind != NL_CHECKED_AGGREGATE || init->type != e->copy_type ||
            match->span.end_byte >= init->span.start_byte)
            return false;
        item = binding->next_item;
    }
    e->copy_after_match = statement->initializer;
    return true;
}
static bool copy_two_helpers(Emit *e, const NLCheckedFragment *body)
{
    if (!e->copy_second_function)
        return true;
    if (e->copy_expected_calls != 2 || !e->copy_issuer)
        return reject(e);
    const NLCheckedFragment *caller = e->copy_issuer;
    size_t first_trial = SIZE_MAX, last_call = 0, calls = 0;
    size_t function = 0;
    NLCheckedNodeId loan = 0;
    NLCheckedField field = {0};
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(caller, id);
        if (!node)
            return reject(e);
        if (node->kind == NL_CHECKED_TRY_ALLOCATE_ONE &&
            node->span.start_byte < first_trial)
            first_trial = node->span.start_byte;
        NLCopyBodyParameters p;
        if (!nl_checked_copy_body_parameters(caller, id, &p))
            continue;
        if (++calls > 2 || node->kind != NL_CHECKED_REGISTERED_CALL)
            return reject(e);
        if (calls == 1) {
            field = p.field;
            loan = p.loan;
            function = node->function;
        } else {
            if (p.field.nominal != field.nominal || p.loan == loan ||
                node->function == function)
                return reject(e);
            e->copy_expected_roots = p.field.parent == field.parent ? 1 : 2;
            if (p.field.parent != field.parent) {
                NLSemanticPlaceView roots[2], children[2];
                const NLCheckedField *fields[2] = {&field, &p.field};
                for (size_t i = 0; i < 2; ++i) {
                    const NLCheckedField *f = fields[i];
                    if (f->index > 1 ||
                        !nl_semantic_place_view(p.entry, f->parent,
                                                &roots[i]) ||
                        !roots[i].live || !roots[i].independent_root ||
                        !roots[i].implicit_local || roots[i].governing_domain ||
                        roots[i].type != f->nominal ||
                        roots[i].incarnation != f->parent_incarnation ||
                        roots[i].fixed_field_count != 2 ||
                        roots[i].fixed_fields[f->index] != f->child ||
                        !nl_semantic_place_view(p.entry, f->child,
                                                &children[i]) ||
                        !children[i].live ||
                        children[i].incarnation != f->child_incarnation ||
                        children[i].parent_aggregate != f->parent ||
                        children[i].parent_incarnation !=
                            roots[i].incarnation ||
                        children[i].parent_field_index != f->index)
                        return reject(e);
                }
                if (field.child == p.field.child)
                    return reject(e);
            } else if (p.field.parent_incarnation != field.parent_incarnation) {
                return reject(e);
            } else if (p.field.index == field.index) {
                if (p.field.child != field.child ||
                    p.field.child_incarnation != field.child_incarnation)
                    return reject(e);
            } else {
                NLSemanticPlaceView root, first, second;
                if (field.index > 1 || p.field.index > 1 ||
                    !nl_semantic_place_view(p.entry, p.field.parent, &root) ||
                    !root.live || root.type != field.nominal ||
                    root.incarnation != field.parent_incarnation ||
                    root.fixed_field_count != 2 ||
                    root.fixed_fields[field.index] != field.child ||
                    root.fixed_fields[p.field.index] != p.field.child ||
                    field.child == p.field.child ||
                    !nl_semantic_place_view(p.entry, field.child, &first) ||
                    !nl_semantic_place_view(p.entry, p.field.child, &second) ||
                    !first.live || !second.live ||
                    first.incarnation != field.child_incarnation ||
                    second.incarnation != p.field.child_incarnation ||
                    first.parent_aggregate != p.field.parent ||
                    second.parent_aggregate != p.field.parent ||
                    first.parent_incarnation != root.incarnation ||
                    second.parent_incarnation != root.incarnation ||
                    first.parent_field_index != field.index ||
                    second.parent_field_index != p.field.index)
                    return reject(e);
            }
        }
        if (node->span.end_byte > last_call)
            last_call = node->span.end_byte;
    }
    return (calls == 2 &&
            (caller != body ||
             (first_trial != SIZE_MAX && last_call <= first_trial) ||
             copy_closed_plan(e, body))) ||
           reject(e);
}
static bool copy_prepare(Emit *e, const NLCheckedFragment *body)
{
    return copy_prepare_at(e, body, 0) && copy_two_helpers(e, body);
}
/* A nested Copy original is allowed only in the exact publicly validated
 * caller fragment after its one producer has returned the original H owner.
 * All fields still use that fragment's entry/handoff identities. */
/* Authenticate an earlier constructor through the completed caller seal and
 * an exact forward, H-effect-free prefix, not a future producer receipt. */
static bool copy_pre_h(Emit *e, const Env *q, const NLSemanticContext *entry)
{
    if (!q || q->region_slot > 1 || !q->backing || !q->heap_symbol ||
        q->stability_scope || q->stability_domain || q->stability_place)
        return false;
    const HeapLifecycle *life = &e->lifecycle[q->region_slot];
    NLSemanticPlaceView root;
    NLSemanticBackingView backing;
    NLSemanticDomainView domain;
    NLSemanticBindingView a, d, p;
    NLSemanticValueView av, dv, pv, value;
    if (life->stage != 4 || life->owner_frame != q->frame ||
        life->domain_frame != q->frame ||
        !nl_semantic_place_view(entry, q->heap_place, &root) || !root.live ||
        root.type != e->node || !root.independent_root || root.implicit_local ||
        root.incarnation != q->heap_incarnation || !root.current_fact ||
        root.governing_domain != q->heap_domain ||
        root.placement.region != q->backing || root.placement.start ||
        root.placement.length != 24 || root.fixed_field_count != 2 ||
        !nl_semantic_backing_view(entry, q->backing, &backing) ||
        !backing.live || backing.size != 24 || backing.alignment != 8 ||
        !backing.ordinary_read || !backing.ordinary_write ||
        !nl_semantic_domain_view(entry, q->heap_domain, &domain) ||
        !domain.live ||
        !nl_semantic_binding_view(entry, life->owner_symbol, &a) ||
        !nl_semantic_binding_view(entry, life->domain_symbol, &d) ||
        !nl_semantic_binding_view(entry, q->heap_symbol, &p) ||
        a.availability != NL_AVAILABLE || d.availability != NL_AVAILABLE ||
        p.availability != NL_AVAILABLE ||
        !nl_semantic_value_view(entry, a.value, &av) ||
        !nl_semantic_value_view(entry, d.value, &dv) ||
        !nl_semantic_value_view(entry, p.value, &pv) ||
        av.allocation_region != q->backing || dv.domain != q->heap_domain ||
        domain.value != d.value || pv.reference.place != q->heap_place ||
        pv.reference.incarnation != q->heap_incarnation || pv.reference.scope ||
        pv.reference.occurrence_dependency ||
        pv.reference.provenance != NL_PROVENANCE_VALID ||
        !pv.reference.readable || !pv.reference.writable ||
        !nl_semantic_value_view(entry, root.current_value, &value) ||
        value.field_count != 2)
        return false;
    /* Locate the actual earlier initialization in this Env's own issuer.
     * Complete reference facts, not cross-world numeric equality alone. */
    const NLCheckedNodeView *init = NULL;
    for (size_t i = 1; i <= nl_checked_node_count(q->artifact); ++i) {
        const NLCheckedNodeView *b = nl_checked_node_view(q->artifact, i);
        if (b->kind != NL_CHECKED_BINDING || b->symbol != q->heap_symbol)
            continue;
        const NLCheckedNodeView *loan = get(q, b->initializer),
                                *block =
                                    loan ? get(q, loan->initializer) : NULL;
        if (init || !loan || loan->kind != NL_CHECKED_LOAN_HEADER ||
            !loan->loan.body_nonescape_proved || !block || block->item_count)
            return false;
        init = get(q, block->tail);
    }
    if (!init || init->kind != NL_CHECKED_INITIALIZE ||
        init->lifetime_place != q->heap_place ||
        init->lifetime_incarnation != q->heap_incarnation ||
        init->lifetime_domain != q->heap_domain ||
        init->backing != q->backing ||
        !same_reference(init->reference_result, pv.reference))
        return false;
    const NLCheckedNodeView *slot = get(q, init->first_argument),
                            *aggregate =
                                slot ? get(q, slot->next_argument) : NULL;
    NLSemanticValueView born;
    if (!aggregate || aggregate->kind != NL_CHECKED_AGGREGATE ||
        aggregate->result_count != 1 ||
        !nl_semantic_value_view(nl_checked_context(q->artifact),
                                aggregate->results[0].value, &born) ||
        born.field_count != 2 ||
        memcmp(born.fields, value.fields, 2 * sizeof(*born.fields)))
        return false;
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticPlaceView child;
        NLSemanticValueView current;
        if (!nl_semantic_place_view(entry, root.fixed_fields[i], &child) ||
            !child.live || child.parent_aggregate != q->heap_place ||
            child.parent_incarnation != root.incarnation ||
            child.parent_field_index != i || !child.current_fact ||
            child.current_value != value.fields[i] ||
            !nl_semantic_value_view(entry, child.current_value, &current) ||
            (i == 0 && (current.variant != 1 || current.sum_payload ||
                        child.payload_occurrence)))
            return false;
    }
    size_t allocations = 0, domains = 0;
    for (size_t i = 1; i <= entry->value_count; ++i) {
        const NLSemanticValueView *v = &entry->values[i - 1];
        if (v->carrier == NL_CARRIER_ENDED)
            continue;
        if (v->allocation_region == q->backing) {
            if (i != a.value)
                return false;
            ++allocations;
        }
        if (v->domain == q->heap_domain) {
            if (i != d.value)
                return false;
            ++domains;
        }
        if (v->occupancy.region == q->backing || v->slot_place == q->heap_place)
            return false;
    }
    return allocations == 1 && domains == 1;
}
static bool copy_pre_constructor(Emit *e, const Env *env,
                                 NLCheckedNodeId binding, NLPlaceId *place,
                                 NLCheckedNodeId *constructor)
{
    const NLCheckedNodeView *b = get(env, binding),
                            *v = b ? get(env, b->initializer) : NULL,
                            *receiver = b ? get(env, b->first_argument) : NULL;
    NLSemanticBindingView local;
    NLSemanticPlaceView root;
    if (!b || b->kind != NL_CHECKED_BINDING || b->argument_count != 1 ||
        b->type != e->unit || !receiver ||
        receiver->kind != NL_CHECKED_RECEIVER ||
        receiver->type != e->copy_type || receiver->symbol != b->symbol ||
        receiver->next_argument || !v || v->kind != NL_CHECKED_AGGREGATE ||
        v->type != e->copy_type || v->argument_count != 2 ||
        !nl_semantic_binding_view(e->copy_anchor, b->symbol, &local) ||
        local.availability != NL_AVAILABLE || local.type != e->copy_type ||
        !nl_semantic_place_view(e->copy_anchor, local.place, &root) ||
        !root.live || !root.implicit_local || !root.independent_root ||
        root.governing_domain || root.fixed_field_count != 2 ||
        root.type != e->copy_type || !root.current_fact || !root.incarnation)
        return false;
    bool seen[2] = {false, false};
    NLCheckedNodeId next = v->first_argument;
    size_t end = v->span.start_byte;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *f = get(env, next),
                                *literal = f ? get(env, f->initializer) : NULL;
        NLSemanticPlaceView child;
        size_t scalar;
        if (!f || f->kind != NL_CHECKED_AGGREGATE_FIELD || f->field_index > 1 ||
            seen[f->field_index] || !literal ||
            literal->kind != NL_CHECKED_U8_LITERAL ||
            literal->span.start_byte < end ||
            !copy_if_literal(env->artifact, literal, e->u8, 255, &scalar) ||
            !nl_semantic_place_view(
                e->copy_anchor, root.fixed_fields[f->field_index], &child) ||
            !child.live || child.parent_aggregate != local.place ||
            child.parent_incarnation != root.incarnation ||
            child.parent_field_index != f->field_index || !child.current_fact ||
            child.current_value != literal->results[0].value)
            return false;
        seen[f->field_index] = true;
        end = literal->span.end_byte;
        next = f->next_argument;
    }
    if (next)
        return false;
    *place = local.place;
    *constructor = b->initializer;
    return true;
}
static bool copy_pre_world(Emit *e, const Env *env)
{
    const bool two_calls =
        e->copy_expected_calls == 2 && e->copy_second_function;
    if (env->artifact != e->copy_issuer || !e->copy_anchor || !e->two_heap ||
        !e->live_tail || e->custody || e->allocations != 2 ||
        e->producer_calls || e->owner_calls || e->link_updates || e->reloans ||
        (e->copy_expected_calls != 1 && !two_calls) ||
        e->copy_calls > e->copy_expected_calls || e->lifecycle[0].stage != 4 ||
        e->lifecycle[1].stage != 4)
        return false;
    if (e->pre_copy_issuer)
        return e->pre_copy_issuer == env->artifact;
    const Env *head = env->parent;
    if (!head || head->region_slot != 0 || env->region_slot != 1 ||
        head->backing == env->backing ||
        head->heap_domain == env->heap_domain ||
        !copy_pre_h(e, head, e->copy_anchor) ||
        !copy_pre_h(e, env, e->copy_anchor))
        return false;
    if (!nl_h_origin_validate(e->origin_root))
        return false;
    const NLCheckedNodeView *root = get(env, nl_checked_root(env->artifact)),
                            *block = root ? get(env, root->tail) : NULL;
    if (!block || block->kind != NL_CHECKED_BLOCK)
        return false;
    NLCheckedNodeId item = block->first_item, after = 0;
    for (size_t i = 0; i < block->item_count; ++i) {
        const NLCheckedNodeView *b = get(env, item);
        if (!b)
            return false;
        if (b->kind == NL_CHECKED_BINDING && b->symbol == env->heap_symbol) {
            if (after)
                return false;
            after = b->next_item;
        }
        item = b->next_item;
    }
    NLPlaceId places[2];
    NLCheckedNodeId constructors[2];
    for (size_t i = 0; i < 2; ++i) {
        if (!copy_pre_constructor(e, env, after, &places[i], &constructors[i]))
            return false;
        after = get(env, after)->next_item;
    }
    if (places[0] == places[1])
        return false;
    const NLCheckedNodeView *binding = get(env, after);
    if (!two_calls) {
        const NLCheckedNodeView *choice = binding,
                                *conditional =
                                    choice ? get(env, choice->initializer)
                                           : NULL;
        if (!choice || choice->kind != NL_CHECKED_BINDING || !conditional ||
            conditional->kind != NL_CHECKED_IF || conditional->normal_arms != 2)
            return false;
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedFragment *a =
                nl_checked_if_arm(env->artifact, choice->initializer, i);
            const NLCheckedNodeView *r = nl_checked_node_view(
                                        a, nl_checked_root(a)),
                                    *b = r ? nl_checked_node_view(
                                                 a, r->initializer)
                                           : NULL,
                                    *v = b ? nl_checked_node_view(a, b->tail)
                                           : NULL;
            if (!a || nl_checked_node_count(a) != 3 || !r || r->terminates ||
                !b || b->item_count || !v || v->kind != NL_CHECKED_U8_LITERAL)
                return false;
        }
        binding = get(env, choice->next_item);
    }
    const NLCheckedNodeView *loan =
                                binding ? get(env, binding->initializer) : NULL,
                            *body = loan ? get(env, loan->initializer) : NULL,
                            *call = body ? get(env, body->tail) : NULL;
    NLCopyBodyParameters p;
    if (!binding || binding->kind != NL_CHECKED_BINDING || !loan ||
        loan->kind != NL_CHECKED_LOAN_HEADER || !body || body->item_count ||
        !call || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !copy_call_proof(e, env->artifact, body->tail, &p) ||
        p.entry != e->copy_anchor || p.loan != binding->initializer ||
        p.field.parent != places[0])
        return false;
    for (size_t i = 0; i < p.entry->scope_count; ++i)
        if (p.entry->scopes[i].active && i + 1 != loan->loan.scope)
            return false;
    if (two_calls) {
        /* Two adjacent ordinary calls follow the two authenticated literal
         * originals, before any link/owner effect. Each actual call still
         * supplies its own current proof and later evolves its own Local. */
        const NLCheckedNodeView *second = get(env, binding->next_item),
                                *second_loan =
                                    second ? get(env, second->initializer)
                                           : NULL,
                                *second_body =
                                    second_loan
                                        ? get(env, second_loan->initializer)
                                        : NULL,
                                *second_call = second_body
                                                   ? get(env, second_body->tail)
                                                   : NULL;
        NLCopyBodyParameters q;
        if (!second || second->kind != NL_CHECKED_BINDING || !second_loan ||
            second_loan->kind != NL_CHECKED_LOAN_HEADER || !second_body ||
            second_body->item_count || !second_call ||
            second_call->kind != NL_CHECKED_REGISTERED_CALL ||
            !copy_call_proof(e, env->artifact, second_body->tail, &q) ||
            q.loan != second->initializer || q.field.parent != places[1] ||
            q.field.parent == p.field.parent)
            return false;
    }
    memcpy(e->pre_copy_constructors, constructors, sizeof(constructors));
    memcpy(e->pre_copy_places, places, sizeof(places));
    e->pre_copy_issuer = env->artifact;
    return true;
}
/* The present H callable must still be the retained source callable. Its own
 * CHECK-issued entry snapshot, not a numeric function ID, supplies history. */
static bool copy_post_callable(const NLCheckedFragment *f, NLCheckedNodeId id,
                               const NLSemanticContext *entry, size_t count)
{
    const NLSemanticContext *current = nl_checked_context(f);
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (!entry || !current || !entry->functions || !current->functions ||
        !call || !call->function || call->function > entry->function_count ||
        call->function > current->function_count || !body ||
        nl_checked_context(body) != current)
        return false;
    const NLFunctionEntry *old = &entry->functions[call->function - 1],
                          *now = &current->functions[call->function - 1];
    if (!old->body || now->body != old->body || body->body_owner != old->body ||
        body->source != old->body->source ||
        old->kind != NL_CHECKED_REGISTERED_CALL || now->kind != old->kind ||
        old->result != call->type || now->result != old->result ||
        old->count != count || now->count != count ||
        old->body->count != count || call->argument_count != count ||
        !old->parameters || !now->parameters ||
        memcmp(old->parameters, now->parameters, count * sizeof(NLTypeId)) ||
        old->caller_effects != now->caller_effects ||
        old->hidden_dependencies != now->hidden_dependencies ||
        old->owner_producer != now->owner_producer ||
        old->owner_receiver != now->owner_receiver ||
        old->custody_recipient != now->custody_recipient)
        return false;
    NLCheckedNodeId argument = call->first_argument;
    for (size_t i = 0; i < count; ++i) {
        const NLCheckedNodeView *a = nl_checked_node_view(f, argument);
        if (!a || a->parameter_type != old->parameters[i])
            return false;
        argument = a->next_argument;
    }
    return !argument;
}
static bool copy_post_h(const Emit *e, const Env *q,
                        const NLSemanticContext *entry)
{
    NLSemanticPlaceView root;
    NLSemanticBackingView backing;
    NLSemanticDomainView domain;
    if (!q || q->region_slot > 1 || !q->backing || !q->heap_place ||
        !q->heap_domain || q->stability_scope || q->stability_domain ||
        q->stability_place || e->lifecycle[q->region_slot].stage != 8 ||
        !nl_semantic_place_view(entry, q->heap_place, &root) || root.live ||
        root.type != e->node || root.incarnation != q->heap_incarnation ||
        root.current_fact || root.current_value || root.governing_domain ||
        root.placement.region ||
        !nl_semantic_backing_view(entry, q->backing, &backing) ||
        backing.live || backing.size != 24 || backing.alignment != 8 ||
        !nl_semantic_domain_view(entry, q->heap_domain, &domain) || domain.live)
        return false;
    for (size_t i = 0; i < entry->value_count; ++i) {
        const NLSemanticValueView *v = &entry->values[i];
        if (v->carrier != NL_CARRIER_ENDED &&
            (v->allocation_region == q->backing ||
             v->occupancy.region == q->backing ||
             v->slot_place == q->heap_place || v->domain == q->heap_domain))
            return false;
    }
    return true;
}
static bool copy_post_world(Emit *e, const Env *env)
{
    if (!env || env->artifact != e->copy_issuer || !e->copy_anchor ||
        !e->two_heap || !e->live_tail || e->custody || e->pre_copy_issuer ||
        e->allocations != 2 || e->producer_calls != 1 || e->owner_calls != 1 ||
        e->pending_live || e->active_owner_call || e->active_owner_body ||
        e->producer_issuer != env->artifact ||
        e->terminal_issuer != env->artifact || e->copy_expected_calls != 1 ||
        (e->post_copy_issuer && e->post_copy_issuer != env->artifact) ||
        (!e->post_copy_issuer && (e->copy_roots || e->copy_calls)))
        return false;
    const Env *head = env->parent;
    const NLCheckedNodeView *producer = get(env, e->producer_node),
                            *terminal = get(env, e->terminal_node);
    NLSemanticSnapshot snapshot;
    if (!head || head->region_slot != 0 || env->region_slot != 1 ||
        head->backing == env->backing || head->heap_place == env->heap_place ||
        head->heap_domain == env->heap_domain ||
        !nl_semantic_snapshot(e->copy_anchor, &snapshot) ||
        snapshot.backing_regions != 2 || snapshot.domains != 2 ||
        snapshot.raw_intervals || !copy_post_h(e, head, e->copy_anchor) ||
        !copy_post_h(e, env, e->copy_anchor) ||
        !nl_h_origin_validate(e->origin_root) || !producer || !terminal ||
        !nl_checked_producer_valid(env->artifact, e->producer_node) ||
        !producer->producer.return_proved ||
        producer->span.end_byte >= terminal->span.start_byte ||
        !terminal->owner_call.entry_proved ||
        !terminal->owner_call.post_proved ||
        terminal->owner_call.root != env->heap_place ||
        terminal->owner_call.incarnation != env->heap_incarnation ||
        terminal->owner_call.domain != env->heap_domain ||
        terminal->owner_call.range.region != env->backing ||
        terminal->owner_call.range.start ||
        terminal->owner_call.range.length != 24 ||
        producer->producer.root != terminal->owner_call.root ||
        producer->producer.incarnation != terminal->owner_call.incarnation ||
        producer->producer.domain != terminal->owner_call.domain ||
        memcmp(&producer->producer.range, &terminal->owner_call.range,
               sizeof(producer->producer.range)) ||
        !copy_post_callable(
            env->artifact, e->producer_node,
            nl_checked_producer_entry(env->artifact, e->producer_node), 4) ||
        !copy_post_callable(
            env->artifact, e->terminal_node,
            nl_checked_owner_entry(env->artifact, e->terminal_node), 3))
        return false;
    e->post_copy_issuer = env->artifact;
    e->post_copy_regions[0] = head->backing;
    e->post_copy_regions[1] = env->backing;
    return true;
}
static bool copy_closed_world(const Emit *e, const Env *env)
{
    NLSemanticSnapshot common;
    if (!e->copy_after_match || !e->copy_h_closed || env->parent ||
        env->backing || env->heap_place || env->heap_domain ||
        env->artifact != e->copy_issuer || e->copy_expected_roots != 2 ||
        e->copy_expected_calls != 2 || e->allocations != 2 ||
        e->producer_calls != 1 || e->owner_calls != 1 || e->pending_live ||
        e->active_owner_call || e->active_owner_body ||
        !nl_semantic_snapshot(e->copy_anchor, &common) ||
        common.backing_regions || common.domains || common.raw_intervals)
        return false;
    for (size_t i = 0; i < 2; ++i)
        if (e->lifecycle[i].stage || e->lifecycle[i].owner_symbol ||
            e->lifecycle[i].domain_symbol)
            return false;
    return true;
}
static bool copy_carrier_world(Emit *e, const Env *env)
{
    return env->artifact == e->copy_issuer &&
           (!e->allocations ||
            (e->allocations == 2 && e->producer_calls == 1 &&
             e->owner_calls == 0) ||
            copy_post_world(e, env) || copy_pre_world(e, env) ||
            copy_closed_world(e, env));
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
    if (e->pre_copy_issuer && !e->producer_calls &&
        constructor != e->pre_copy_constructors[0] &&
        constructor != e->pre_copy_constructors[1])
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
    if (e->pre_copy_issuer && !e->producer_calls &&
        f->parent != e->pre_copy_places[0] &&
        f->parent != e->pre_copy_places[1])
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
    if (e->pre_copy_issuer && !e->producer_calls &&
        v != get(env, e->pre_copy_constructors[0]) &&
        v != get(env, e->pre_copy_constructors[1]))
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
                 out->temp, tail->function, l->frame, l->symbol, index,
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
