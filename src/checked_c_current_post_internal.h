/* Included after NLPhysicalCurrent. This is a synchronous, finite consumer of
 * existing Copy/loop receipts. Historical entry/handoff worlds describe only
 * the original's successive field states, never loan or caller authority. */
static bool physical_current_same(const NLSemanticContext *a,
                                  const NLSemanticContext *b,
                                  const NLLinearBreakView *loop)
{
    NLSemanticPlaceView x[3], y[3];
    if (!physical_current_world(a, loop) ||
        !physical_current_world(b, loop) || !nl_copy_trio(a, loop->root, x) ||
        !nl_copy_trio(b, loop->root, y))
        return false;
    for (size_t i = 0; i < 3; ++i)
        if (x[i].type != y[i].type ||
            x[i].incarnation != y[i].incarnation ||
            x[i].current_fact != y[i].current_fact ||
            x[i].current_value != y[i].current_value ||
            x[i].parent_aggregate != y[i].parent_aggregate ||
            x[i].parent_incarnation != y[i].parent_incarnation ||
            x[i].parent_field_index != y[i].parent_field_index)
            return false;
    return x[0].fixed_fields[0] == y[0].fixed_fields[0] &&
           x[0].fixed_fields[1] == y[0].fixed_fields[1];
}

static const NLPhysicalPostCall *physical_current_post_call(
    const NLPhysicalCurrent *p, const NLCheckedFragment *body,
    NLCheckedNodeId call)
{
    if (p && body == p->body)
        for (size_t i = 0; i < p->post_count; ++i)
            if (p->post[i].call == call)
                return &p->post[i];
    return NULL;
}

static NLCheckStatus physical_current_post_prepare(
    NLPhysicalCurrent *p, NLCheckedNodeId id,
    const NLSemanticContext *before)
{
    const NLCheckStatus invalid = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLLinearBreakView *loop = &p->loop_view;
    const NLTypeId u8 = nl_semantic_core_type(before, NL_TYPE_U8);
    const NLCheckedNodeView *loan = nl_checked_node_view(p->body, id);
    const NLCheckedNodeView *block =
        loan ? nl_checked_node_view(p->body, loan->initializer) : NULL;
    const NLCheckedNodeView *call =
        block ? nl_checked_node_view(p->body, block->tail) : NULL;
    NLSemanticScopeView closed;
    if (p->post_count == 2 || !loop->terminal_old || !loan ||
        loan->kind != NL_CHECKED_LOAN_HEADER || loan->type != u8 ||
        loan->terminates || loan->result_count != 1 ||
        !loan->loan.body_nonescape_proved ||
        !loan->loan.normal_result_forwarded || !loan->loan.implicit_local ||
        loan->loan.from_ptr || loan->loan.is_exclusive ||
        loan->loan.access != NL_ACCESS_WRITE ||
        loan->loan.source != loop->base || !loan->loan.scope ||
        !nl_semantic_scope_view(nl_checked_context(p->body),
                                loan->loan.scope, &closed) ||
        closed.active || !block || block->kind != NL_CHECKED_BLOCK ||
        block->item_count || block->first_item || block->terminates ||
        block->type != u8 || block->result_count != 1 || !call ||
        call->kind != NL_CHECKED_REGISTERED_CALL || call->terminates ||
        call->argument_count != 2 || call->result_count != 1 ||
        call->type != u8 ||
        loan->results[0].value != call->results[0].value ||
        block->results[0].value != call->results[0].value)
        return invalid;
    NLPhysicalPostCall post = {.loan = id,
                               .call = block->tail,
                               .before = before};
    NLCopyBodyView view;
    if (!nl_checked_copy_body_view(p->body, post.call, &view) ||
        !nl_checked_copy_body_parameters(p->body, post.call,
                                         &post.parameters))
        return invalid;
    NLCheckStatus status =
        nl_checked_copy_body_validate(p->body, post.call, &view);
    if (status == NL_CHECK_OK)
        status = nl_checked_copy_body_parameters_validate(
            p->body, post.call, &post.parameters);
    if (status != NL_CHECK_OK)
        return status;
    const NLCopyBodyParameters *actual = &post.parameters;
    const NLCheckedField *field = &actual->field;
    NLCopyBodyEventView event;
    NLSemanticPlaceView entry[3], end[3];
    if (actual->loan != id || view.event_count != 1 ||
        !nl_checked_copy_body_event(p->body, post.call, 0, &event) ||
        event.kind != NL_COPY_BODY_CHANGE ||
        !copy_old_result(&view, &event, u8) || !field->present ||
        !field->dependency_compatible || field->access != NL_ACCESS_WRITE ||
        field->nominal != loop->nominal || field->type != u8 ||
        field->base != loop->base || field->parent != loop->root ||
        field->parent_incarnation != loop->incarnation || field->index > 1 ||
        !physical_current_same(before, actual->entry, loop) ||
        !nl_copy_trio(actual->entry, loop->root, entry) ||
        !nl_copy_trio(actual->handoff, loop->root, end) ||
        !physical_current_same(event.after, actual->handoff, loop) ||
        field->child != entry[0].fixed_fields[field->index] ||
        field->child_incarnation != entry[field->index + 1].incarnation ||
        field->parent_fact != entry[0].current_fact ||
        field->child_fact != entry[field->index + 1].current_fact ||
        field->old_value != entry[field->index + 1].current_value ||
        loan->loan.place != field->child ||
        loan->loan.incarnation != field->child_incarnation ||
        end[0].incarnation != loop->incarnation ||
        end[0].fixed_fields[0] != entry[0].fixed_fields[0] ||
        end[0].fixed_fields[1] != entry[0].fixed_fields[1])
        return invalid;
    const NLCheckedNodeView *ref =
        nl_checked_node_view(view.body, event.node);
    const NLCheckedNodeView *lhs =
        ref ? nl_checked_node_view(view.body, ref->first_argument) : NULL;
    const NLCheckedNodeView *rhs =
        lhs ? nl_checked_node_view(view.body, lhs->next_argument) : NULL;
    const NLCheckedNodeView *argument =
        nl_checked_node_view(p->body, actual->arguments[1]);
    if (!ref || ref->kind != NL_CHECKED_REPLACE || ref->argument_count != 2 ||
        ref->type != u8 || ref->result_count != 1 ||
        ref->results[0].value != event.result || !lhs ||
        lhs->kind != NL_CHECKED_IDENTIFIER ||
        lhs->symbol != actual->parameters[0] || !rhs ||
        rhs->kind != NL_CHECKED_IDENTIFIER ||
        rhs->symbol != actual->parameters[1] || rhs->next_argument ||
        !argument || argument->type != u8 || argument->result_count != 1 ||
        (argument->kind != NL_CHECKED_U8_LITERAL &&
         (argument->kind != NL_CHECKED_IDENTIFIER ||
          argument->value_use != NL_VALUE_COPIED)) ||
        !nl_copy_plain(actual->entry, argument->results[0].value, u8))
        return invalid;
    p->post[p->post_count++] = post;
    return NL_CHECK_OK;
}
