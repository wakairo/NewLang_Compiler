/* Borrowed serializer schedule for the CHECK-issued normal-current join.
 * No new root, state proof or runtime carrier is created here. */
static bool copy_conditional_field(const NLCheckedField *a,
                                   const NLCheckedField *b)
{
    return a->present && b->present && a->parent == b->parent &&
           a->child == b->child && a->nominal == b->nominal &&
           a->index == b->index &&
           a->parent_incarnation == b->parent_incarnation &&
           a->child_incarnation == b->child_incarnation;
}
static bool copy_conditional_join(const Emit *e, const NLCheckedFragment *f,
                                  NLCheckedNodeId id)
{
    const NLCheckedNodeView *m = nl_checked_node_view(f, id),
                            *trial = m ? nl_checked_node_view(f, m->initializer)
                                       : NULL;
    NLSemanticValueView unknown;
    return m && trial && m->kind == NL_CHECKED_MATCH &&
           trial->kind == NL_CHECKED_TRY_ALLOCATE_ONE && m->type == e->unit &&
           !m->result_count && m->normal_arms == 2 && m->item_count == 2 &&
           !m->terminates && !m->normal_frame_unchanged && m->field.present &&
           m->field.dependency_compatible && m->field.nominal == e->copy_type &&
           m->field.type == e->u8 && m->field.index < 2 &&
           m->field.access == NL_ACCESS_WRITE && m->field.parent_fact &&
           m->field.child_fact && m->field.parent_post_fact &&
           m->field.child_post_fact &&
           m->field.parent_fact != m->field.parent_post_fact &&
           m->field.child_fact != m->field.child_post_fact &&
           nl_semantic_value_view(nl_checked_context(f), m->field.new_value,
                                  &unknown) &&
           unknown.type == e->u8 && !unknown.scalar_known &&
           unknown.dependencies == NL_DEPENDENCY_FREE &&
           !unknown.value_dependency_count;
}
static bool copy_conditional_plan(Emit *e, const NLCheckedFragment *body)
{
    /* Classification only: the whole source seal, both Copy proofs, exact
     * arm attachments and real physical lifecycle checks remain mandatory.
     * A missing producer is not replaced by a synthetic owner or LiveTail. */
    const bool direct = !e->live_tail && e->scalar_issued && e->two_heap &&
                        !e->custody && !e->current;
    const NLCheckedNodeView
        *root = nl_checked_node_view(body, nl_checked_root(body)),
        *binding = root ? nl_checked_node_view(body, root->first_item) : NULL,
        *prefix =
            binding ? nl_checked_node_view(body, binding->next_item) : NULL,
        *statement =
            prefix ? nl_checked_node_view(body, prefix->next_item) : NULL,
        *outer = statement ? nl_checked_node_view(body, statement->initializer)
                           : NULL,
        *loan = prefix ? nl_checked_node_view(body, prefix->initializer) : NULL,
        *block = loan ? nl_checked_node_view(body, loan->initializer) : NULL;
    NLCopyBodyParameters first, last;
    if (!root || root->kind != NL_CHECKED_BLOCK || root->terminates ||
        !binding || binding->kind != NL_CHECKED_BINDING || !prefix ||
        prefix->kind != NL_CHECKED_BINDING || !statement ||
        statement->kind != NL_CHECKED_STATEMENT || !outer || !loan ||
        loan->kind != NL_CHECKED_LOAN_HEADER || !block || block->item_count ||
        e->copy_expected_calls != 2 || e->copy_second_function ||
        body != e->copy_issuer || !e->scalar_issued || !e->two_heap ||
        (!e->live_tail && !direct) || e->custody || e->current ||
        !copy_conditional_join(e, body, statement->initializer) ||
        outer->captured_frame_closed ||
        !copy_call_proof(e, body, block->tail, &first) ||
        !copy_call_proof(e, e->copy_conditional_issuer,
                         e->copy_conditional_call, &last) ||
        first.field.base != binding->symbol ||
        !copy_conditional_field(&first.field, &last.field) ||
        !copy_conditional_field(&outer->field, &first.field))
        return reject(e);
    const NLCheckedFragment *none = nl_checked_match_arm(
                                body, statement->initializer, 0),
                            *some = nl_checked_match_arm(
                                body, statement->initializer, 1);
    const NLCheckedNodeView *nr = nl_checked_node_view(none,
                                                       nl_checked_root(none)),
                            *sr = nl_checked_node_view(some,
                                                       nl_checked_root(some));
    if (!none || !some || !nr || !sr || nr->field.present || nr->terminates ||
        !nr->normal_frame_unchanged || !sr->field.present || sr->terminates ||
        !copy_original_prefix(first.handoff, nl_checked_context(none),
                              &first.field) ||
        !copy_original_prefix(nl_checked_context(none), last.entry,
                              &last.field))
        return reject(e);
    size_t inner = 0;
    for (size_t i = 1; i <= nl_checked_node_count(some); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(some, i);
        if (n->kind == NL_CHECKED_MATCH && n->field.present) {
            if (inner || !copy_conditional_join(e, some, i) ||
                !n->captured_frame_closed ||
                !copy_conditional_field(&outer->field, &n->field) ||
                nl_checked_match_arm(some, i, 1) != e->copy_conditional_issuer)
                return reject(e);
            const NLCheckedFragment *a = nl_checked_match_arm(some, i, 0);
            const NLCheckedNodeView *r =
                nl_checked_node_view(a, nl_checked_root(a));
            if (!r || r->field.present || r->terminates)
                return reject(e);
            inner = i;
        }
    }
    if (!inner)
        return reject(e);
    if (direct) {
        /* Keep the exact checked H2 call/owner, never just a call count or
         * a same-number root in another arm. The existing whole-root seal
         * authenticates this graph before this serializer borrows it. */
        const NLCheckedFragment *h2 = e->copy_conditional_issuer;
        for (size_t id = 1; id <= nl_checked_node_count(h2); ++id) {
            const NLCheckedNodeView *n = nl_checked_node_view(h2, id);
            if (!n->owner_call.entry_proved)
                continue;
            if (e->direct_receiver_issuer ||
                n->kind != NL_CHECKED_REGISTERED_CALL || !n->body_backed ||
                !n->owner_call.post_proved ||
                n->owner_call.definition.live_return ||
                n->owner_call.definition.head_link_required ||
                n->span.end_byte >
                    nl_checked_node_view(h2, e->copy_conditional_call)
                        ->span.start_byte)
                return reject(e);
            e->direct_receiver_issuer = h2;
            e->direct_receiver_call = id;
        }
    }
    e->copy_conditional_inner = some;
    e->copy_conditional_inner_match = inner;
    e->copy_conditional_match = statement->initializer;
    e->copy_prefix_call = block->tail;
    e->copy_expected_roots = 1;
    e->direct_h = direct;
    return true;
}
static bool copy_conditional_at(const Emit *e, const Env *env,
                                NLCheckedNodeId id)
{
    return e->copy_conditional_call &&
           ((env->artifact == e->copy_issuer &&
             id == e->copy_conditional_match) ||
            (env->artifact == e->copy_conditional_inner &&
             id == e->copy_conditional_inner_match));
}
static Local *copy_conditional_local(Env *env, const NLCheckedField *f)
{
    for (size_t i = 0; i < env->count; ++i) {
        Local *l = &env->locals[i];
        if (l->symbol == f->base && l->type == f->nominal &&
            l->place == f->parent && l->incarnation == f->parent_incarnation &&
            l->copy_fact == f->parent_fact &&
            l->copy_child[f->index] == f->child &&
            l->copy_inc[f->index] == f->child_incarnation &&
            l->copy_fact_child[f->index] == f->child_fact &&
            l->copy_value[f->index] == f->old_value)
            return l;
    }
    return NULL;
}
