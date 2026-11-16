/* CHECK-only use of the existing fixed-u8 Change and actual CopyBody proof.
 * The MATCH field records this same source-issued normal-current transition;
 * it is sealed with its canonical H tree, not an additional authority issuer.
 */
static bool allocated_current_same(const NLCheckedField *a,
                                   const NLCheckedField *b)
{
    return a->present && b->present && a->nominal == b->nominal &&
           a->parent == b->parent && a->child == b->child &&
           a->index == b->index &&
           a->parent_incarnation == b->parent_incarnation &&
           a->child_incarnation == b->child_incarnation;
}

static bool allocated_current_candidate(const NLSemanticContext *entry,
                                        const NLSemanticContext *after,
                                        NLCheckedField *out)
{
    *out = (NLCheckedField){0};
    if (after->place_count < entry->place_count ||
        after->binding_count < entry->binding_count)
        return false;
    for (size_t i = 1; i <= entry->place_count; ++i) {
        NLSemanticPlaceView a[3], b[3];
        if (!nl_copy_trio(entry, i, a) || !nl_copy_trio(after, i, b))
            continue;
        if (a[0].current_fact == b[0].current_fact &&
            a[0].current_value == b[0].current_value)
            continue;
        if (out->present || !same_place_frame(a[0], b[0]))
            return false;
        size_t changed = 2;
        for (size_t j = 0; j < 2; ++j) {
            if (!same_place_frame(a[j + 1], b[j + 1]))
                return false;
            if (a[j + 1].current_fact != b[j + 1].current_fact ||
                a[j + 1].current_value != b[j + 1].current_value) {
                if (changed != 2)
                    return false;
                changed = j;
            }
        }
        if (changed == 2)
            return false;
        NLSymbolId base = 0;
        for (size_t j = 0; j < entry->binding_count; ++j)
            if (!entry->bindings[j].hidden &&
                entry->bindings[j].view.place == i)
                base = j + 1;
        if (!base)
            return false;
        *out = (NLCheckedField){.present = true,
                                .dependency_compatible = true,
                                .nominal = a[0].type,
                                .type = a[changed + 1].type,
                                .base = base,
                                .parent = i,
                                .parent_incarnation = a[0].incarnation,
                                .child = a[0].fixed_fields[changed],
                                .child_incarnation = a[changed + 1].incarnation,
                                .index = changed,
                                .access = NL_ACCESS_WRITE,
                                .parent_fact = a[0].current_fact,
                                .child_fact = a[changed + 1].current_fact,
                                .old_value = a[changed + 1].current_value,
                                .new_value = b[changed + 1].current_value,
                                .parent_post_fact = b[0].current_fact,
                                .child_post_fact = b[changed + 1].current_fact};
    }
    return out->present;
}

static bool allocated_current_h_closed(const NLSemanticContext *c)
{
    for (size_t i = 0; i < c->region_count; ++i)
        if (c->regions[i].view.live)
            return false;
    for (size_t i = 0; i < c->domain_count; ++i)
        if (c->domains[i].live)
            return false;
    return true;
}

/* Before comparing the complete frame, prove the one permitted delta from
 * either the real direct named Change or the already checked nested join.
 * Caller node seals are installed first; validation uses its genuine world.
 */
static NLCheckStatus allocated_current_origin(const NLCheckedFragment *arm,
                                              const NLCheckedField *delta)
{
    size_t found = 0;
    for (size_t i = 1; i <= arm->count; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(arm, i);
        NLCopyBodyParameters p;
        if (n->kind == NL_CHECKED_REGISTERED_CALL &&
            nl_checked_copy_body_parameters(arm, i, &p)) {
            NLSemanticPlaceView post[3];
            if (!allocated_current_same(delta, &p.field) ||
                p.field.parent_fact != delta->parent_fact ||
                p.field.child_fact != delta->child_fact ||
                p.field.old_value != delta->old_value ||
                !nl_copy_trio(p.handoff, p.field.parent, post) ||
                post[0].current_fact != delta->parent_post_fact ||
                post[delta->index + 1].current_fact != delta->child_post_fact ||
                post[delta->index + 1].current_value != delta->new_value ||
                !allocated_current_h_closed(p.entry) ||
                !allocated_current_h_closed(p.handoff))
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            NLCheckStatus s = nl_checked_copy_body_validate(arm, i, NULL);
            if (s == NL_CHECK_OK)
                s = nl_checked_copy_body_parameters_validate(arm, i, &p);
            if (s != NL_CHECK_OK)
                return s;
            NLCopyBodyView body;
            NLCopyBodyEventView event;
            if (!nl_checked_copy_body_view(arm, i, &body) ||
                body.event_count != 1 ||
                !nl_checked_copy_body_event(arm, i, 0, &event) ||
                event.kind != NL_COPY_BODY_CHANGE ||
                !copy_old_result(
                    &body, &event,
                    nl_semantic_core_type(arm->context, NL_TYPE_U8)))
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            ++found;
        } else if (n->kind == NL_CHECKED_MATCH && n->field.present) {
            const NLCheckedNodeView *trial =
                nl_checked_node_view(arm, n->initializer);
            if (!trial || trial->kind != NL_CHECKED_TRY_ALLOCATE_ONE ||
                n->normal_arms != 2 || n->item_count != 2 || n->terminates ||
                n->normal_frame_unchanged || n->type != 1 ||
                !allocated_current_same(delta, &n->field) ||
                n->field.parent_fact != delta->parent_fact ||
                n->field.child_fact != delta->child_fact ||
                n->field.old_value != delta->old_value ||
                n->field.new_value != delta->new_value ||
                n->field.parent_post_fact != delta->parent_post_fact ||
                n->field.child_post_fact != delta->child_post_fact)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            ++found;
        }
    }
    return found == 1 ? NL_CHECK_OK : NL_CHECK_ANALYSIS_PRECISION_LIMIT;
}

static NLCheckStatus allocated_current_frame(const NLSemanticContext *entry,
                                             const NLCheckedFragment *arm,
                                             const NLCheckedField *delta)
{
    const NLSemanticContext *after = arm->context;
    const NLCheckedNodeView unit = {.type = 1};
    if (!allocated_suffix_closed(entry, after, &unit))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    for (size_t i = entry->scope_count; i < after->scope_count; ++i)
        if (after->scopes[i].active)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLCheckStatus s = allocated_current_origin(arm, delta);
    if (s != NL_CHECK_OK)
        return s;
    NLSemanticPlaceView before[3], now[3];
    if (!nl_copy_trio(entry, delta->parent, before) ||
        !nl_copy_trio(after, delta->parent, now))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const size_t sibling = 2 - delta->index;
    if (before[sibling].current_fact != now[sibling].current_fact ||
        before[sibling].current_value != now[sibling].current_value)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLSemanticContext *comparison = NULL;
    s = nl_sem_clone(after, &comparison);
    if (s != NL_CHECK_OK)
        return s;
    /* Comparison only, never a committed world: restore just the bookkeeping
     * whose actual Change was replay-validated above. Every unrelated value,
     * scope, binding, dependency and original still compares exactly. */
    const NLValueId old[2] = {before[0].current_value, delta->old_value};
    bool valid = true;
    for (size_t i = 0; i < 2 && valid; ++i) {
        NLSemanticValueView v = comparison->values[old[i] - 1];
        const NLSemanticValueView original = entry->values[old[i] - 1];
        v.carrier = original.carrier;
        v.owner_place = original.owner_place;
        v.aggregate_owner = original.aggregate_owner;
        valid = node_unit_value_same(v, original);
        if (valid)
            comparison->values[old[i] - 1] = original;
    }
    const NLValueId unchanged = before[sibling].current_value;
    if (comparison->values[unchanged - 1].aggregate_owner !=
        now[0].current_value)
        valid = false;
    comparison->values[unchanged - 1].aggregate_owner = before[0].current_value;
    comparison->places[delta->parent - 1].current_fact = before[0].current_fact;
    comparison->places[delta->parent - 1].current_value =
        before[0].current_value;
    comparison->places[delta->child - 1].current_fact =
        before[delta->index + 1].current_fact;
    comparison->places[delta->child - 1].current_value = delta->old_value;
    for (size_t i = 0; i < entry->binding_count; ++i)
        if (entry->bindings[i].view.place == delta->parent) {
            if (comparison->bindings[i].view.value != now[0].current_value)
                valid = false;
            comparison->bindings[i].view.value = entry->bindings[i].view.value;
        }
    comparison->region_count = entry->region_count;
    comparison->domain_count = entry->domain_count;
    valid = valid && nl_node_unit_arm_frame(entry, comparison, 0, &unit);
    nl_semantic_destroy(comparison);
    return valid ? NL_CHECK_OK : NL_CHECK_ANALYSIS_PRECISION_LIMIT;
}

static NLCheckStatus allocated_current_join(const NLSemanticContext *entry,
                                            NLCheckedField *field,
                                            NLSemanticContext **out)
{
    NLSemanticContext *c = NULL;
    NLCheckStatus s = nl_sem_clone(entry, &c);
    NLValueId unknown = 0;
    if (s == NL_CHECK_OK)
        s = nl_fixed_change_dependencies(c, field->child, 0);
    if (s == NL_CHECK_OK)
        s = nl_sem_new_value(c, (NLSemanticValueView){.type = field->type},
                             &unknown);
    if (s == NL_CHECK_OK)
        s = nl_fixed_change(c, field->child, unknown, false);
    if (s == NL_CHECK_OK) {
        Check refresh = {.context = c};
        refresh_bindings(&refresh, field->parent);
        s = nl_sem_validate(c);
    }
    if (s != NL_CHECK_OK) {
        nl_semantic_destroy(c);
        return s;
    }
    field->new_value = unknown;
    field->parent_post_fact = c->places[field->parent - 1].current_fact;
    field->child_post_fact = c->places[field->child - 1].current_fact;
    *out = c;
    return NL_CHECK_OK;
}
