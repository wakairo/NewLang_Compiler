#include "semantic_internal.h"

#include <stdlib.h>
#include <string.h>

#define INVALID NL_CHECK_ANALYSIS_PRECISION_LIMIT

typedef struct NLOrdinaryCallProof {
    const NLCheckedFragment *caller;
    NLCheckedNodeId call, loan, arguments[2];
    NLSemanticContext *entry, *post, *closed;
} NLOrdinaryCallProof;

#ifdef NEWLANG_EXPERIMENTAL_ORDINARY_CALL_PROOF
static void destroy(NLCheckedFragment *body)
{
    NLOrdinaryCallProof *p = body->ordinary_call;
    nl_semantic_destroy(p->entry);
    nl_semantic_destroy(p->post);
    nl_semantic_destroy(p->closed);
    free(p);
}
#endif

/* No registry/name recognition, source admission or checker permission. The
 * experimental issuer saves authentic worlds only for an already legal call. */
NLCheckStatus nl_ordinary_call_begin(NLCheckedFragment *body,
                                     const NLCheckedFragment *caller,
                                     NLCheckedNodeId call,
                                     const NLSemanticContext *c,
                                     const NLCheckedNodeId *arguments)
{
#ifndef NEWLANG_EXPERIMENTAL_ORDINARY_CALL_PROOF
    (void)body;
    (void)caller;
    (void)call;
    (void)c;
    (void)arguments;
    return NL_CHECK_OK;
#else
    const NLCheckedNodeView *v = nl_checked_node_view(caller, call);
    if (!caller->body_owner || !v || !v->function ||
        v->function > c->function_count)
        return NL_CHECK_OK;
    const NLFunctionEntry *fn = &c->functions[v->function - 1];
    if (fn->count != 2 || fn->owner_receiver || fn->owner_producer ||
        fn->custody_recipient || !fn->body || fn->body != body->body_owner)
        return NL_CHECK_OK;
    const NLSemanticTypeView t = c->types[fn->parameters[0] - 1].view;
    if (t.kind != NL_TYPE_REF || t.access != NL_ACCESS_WRITE ||
        t.is_exclusive || t.target != fn->parameters[1] ||
        t.target != fn->result || !c->types[t.target - 1].option_target)
        return NL_CHECK_OK;
    const NLCheckedNodeView *a = nl_checked_node_view(caller, arguments[0]);
    if (!a || a->result_count != 1 || !a->results[0].value)
        return NL_CHECK_OK;
    const NLReferenceFacts r = c->values[a->results[0].value - 1].reference;
    if (!r.place || r.place > c->place_count)
        return NL_CHECK_OK;
    const NLSemanticPlaceView f = c->places[r.place - 1];
    if (!f.parent_aggregate || f.parent_field_index != 0)
        return NL_CHECK_OK;
    const NLSemanticPlaceView root = c->places[f.parent_aggregate - 1];
    if (!root.implicit_local || root.fixed_field_count != 2 ||
        !nl_recursive_local_type(c, root.type))
        return NL_CHECK_OK;
    NLCheckedNodeId loan = 0;
    for (size_t i = 1; i <= caller->count; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(caller, i);
        if (n->kind == NL_CHECKED_LOAN_HEADER && n->loan.scope == r.scope &&
            n->loan.place == r.place && n->loan.implicit_local &&
            !n->loan.from_ptr && n->loan.access == NL_ACCESS_WRITE) {
            if (loan)
                return NL_CHECK_OK;
            loan = i;
        }
    }
    if (!loan)
        return NL_CHECK_OK;
    NLOrdinaryCallProof *p = calloc(1, sizeof(*p));
    if (!p)
        return NL_CHECK_OUT_OF_MEMORY;
    body->ordinary_call = p;
    body->destroy_ordinary_call = destroy;
    p->caller = caller;
    p->call = call;
    p->loan = loan;
    p->arguments[0] = arguments[0];
    p->arguments[1] = arguments[1];
    return nl_sem_clone(c, &p->entry);
#endif
}

NLCheckStatus nl_ordinary_call_post(const NLCheckedFragment *caller,
                                    NLCheckedNodeId call,
                                    const NLSemanticContext *c)
{
    const NLCheckedFragment *body = nl_checked_call_body(caller, call);
    NLOrdinaryCallProof *p = body ? body->ordinary_call : NULL;
    if (!p)
        return NL_CHECK_OK;
    if (p->caller != caller || p->call != call || p->post)
        return INVALID;
    return nl_sem_clone(c, &p->post);
}

NLCheckStatus nl_ordinary_call_close(const NLCheckedFragment *caller,
                                     NLCheckedNodeId loan,
                                     const NLSemanticContext *c)
{
    for (size_t i = 0; i < caller->body_count; ++i) {
        NLOrdinaryCallProof *p = caller->bodies[i]->ordinary_call;
        if (!p || p->loan != loan)
            continue;
        if (p->caller != caller || !p->post || p->closed)
            return INVALID;
        NLCheckStatus s = nl_sem_clone(c, &p->closed);
        if (s != NL_CHECK_OK)
            return s;
    }
    return NL_CHECK_OK;
}

static bool reference_same(NLReferenceFacts a, NLReferenceFacts b)
{
    return a.place == b.place && a.incarnation == b.incarnation &&
           a.scope == b.scope && a.provenance == b.provenance &&
           a.readable == b.readable && a.writable == b.writable &&
           a.occurrence_dependency == b.occurrence_dependency;
}

static bool body_shape(const NLCheckedFragment *b, const NLCheckedNodeView **op)
{
    const NLCheckedNodeView *block = nl_checked_node_view(b, b->root);
    if (b->count != 4 || b->body_count || b->arm_count || !block ||
        block->kind != NL_CHECKED_BLOCK || block->item_count ||
        block->first_item || block->terminates || block->result_count != 1 ||
        nl_control_exits_count(b->exits) ||
        nl_control_exits_count(b->loop_returns))
        return false;
    *op = nl_checked_node_view(b, block->tail);
    if (!*op || (*op)->kind != NL_CHECKED_REPLACE ||
        (*op)->argument_count != 2 || (*op)->terminates ||
        (*op)->result_count != 1 ||
        block->results[0].value != (*op)->results[0].value ||
        block->type != (*op)->type)
        return false;
    const NLCheckedNodeView *a = nl_checked_node_view(b, (*op)->first_argument);
    const NLCheckedNodeView *n =
        a ? nl_checked_node_view(b, a->next_argument) : NULL;
    return a && n && a->kind == NL_CHECKED_IDENTIFIER &&
           n->kind == NL_CHECKED_IDENTIFIER && a->result_count == 1 &&
           n->result_count == 1 && !n->next_argument &&
           a->value_use == NL_VALUE_COPIED && n->value_use == NL_VALUE_COPIED &&
           block != *op && block != a && block != n && *op != a && *op != n &&
           a != n;
}

bool nl_checked_ordinary_call_view(const NLCheckedFragment *caller,
                                   NLCheckedNodeId call,
                                   NLOrdinaryCallView *out)
{
    const NLCheckedFragment *b = nl_checked_call_body(caller, call);
    const NLOrdinaryCallProof *p = b ? b->ordinary_call : NULL;
    const NLCheckedNodeView *op = NULL;
    if (!out || !p || p->caller != caller || p->call != call || !p->entry ||
        !p->post || !p->closed || !body_shape(b, &op) ||
        b->field_change_count != 1 || !b->field_changes[0] ||
        !op->field.present)
        return false;
    const NLCheckedNodeView *a = nl_checked_node_view(caller, p->arguments[0]);
    if (!a || !a->results[0].value ||
        a->results[0].value > p->entry->value_count)
        return false;
    NLOrdinaryCallView v = {
        .caller = caller,
        .body = b,
        .call = call,
        .loan = p->loan,
        .arguments = {p->arguments[0], p->arguments[1]},
        .operation = b->field_changes[0]->node,
        .entry = p->entry,
        .before_change = b->field_changes[0]->before,
        .after_change = b->field_changes[0]->after,
        .post = p->post,
        .closed_loan = p->closed,
        .parameters = {p->entry->binding_count + 1,
                       p->entry->binding_count + 2},
        .root = op->field.parent,
        .field = op->field.child,
        .reference = p->entry->values[a->results[0].value - 1].reference,
        .returned = op->results[0].value};
    size_t originals = 0;
    for (size_t i = 0; i < p->entry->place_count; ++i) {
        const NLSemanticPlaceView r = p->entry->places[i];
        if (!r.live || !r.independent_root ||
            !nl_recursive_local_type(p->entry, r.type))
            continue;
        if (originals == 2 || !r.implicit_local || r.placement.region ||
            r.governing_domain || r.fixed_field_count != 2)
            return false;
        v.originals[originals++] = i + 1;
    }
    if (originals != 2)
        return false;
    *out = v;
    return true;
}

static bool view_same(const NLOrdinaryCallView *a, const NLOrdinaryCallView *b)
{
    return a->caller == b->caller && a->body == b->body && a->call == b->call &&
           a->loan == b->loan && a->arguments[0] == b->arguments[0] &&
           a->arguments[1] == b->arguments[1] && a->operation == b->operation &&
           a->entry == b->entry && a->before_change == b->before_change &&
           a->after_change == b->after_change && a->post == b->post &&
           a->closed_loan == b->closed_loan &&
           a->parameters[0] == b->parameters[0] &&
           a->parameters[1] == b->parameters[1] &&
           a->originals[0] == b->originals[0] &&
           a->originals[1] == b->originals[1] && a->root == b->root &&
           a->field == b->field && reference_same(a->reference, b->reference) &&
           a->returned == b->returned;
}

/* The enclosing loan must be an actually connected top-level binding, with
 * just this call as its source body tail. Not an arm/loop/dead nested graph. */
static bool call_path(const NLOrdinaryCallView *v)
{
    const NLCheckedFragment *f = v->caller;
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    const NLCheckedNodeView *loan = nl_checked_node_view(f, v->loan);
    const NLCheckedNodeView *call = nl_checked_node_view(f, v->call);
    const NLCheckedNodeView *block =
        loan ? nl_checked_node_view(f, loan->initializer) : NULL;
    if (!root || root->kind != NL_CHECKED_BLOCK || !loan ||
        loan->kind != NL_CHECKED_LOAN_HEADER || !call || !block ||
        block->kind != NL_CHECKED_BLOCK || block->item_count ||
        block->first_item || block->tail != v->call || block->terminates ||
        block->result_count != 1 || block->results[0].value != v->returned ||
        loan->result_count != 1 || loan->results[0].value != v->returned ||
        loan->span.start_byte > call->span.start_byte ||
        loan->span.end_byte < call->span.end_byte)
        return false;
    NLCheckedNodeId item = root->first_item;
    size_t found = 0;
    for (size_t i = 0; i < root->item_count; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, item);
        if (!n)
            return false;
        if (n->kind == NL_CHECKED_BINDING && n->initializer == v->loan)
            ++found;
        item = n->next_item;
    }
    return !item && found == 1;
}

/* Full-world comparison is stricter than just checking the changed field.
 * Replay on private clones preserves scopes, hidden dependencies, sibling
 * values, nominal registries, fresh F/RF and all occurrence history. */
static NLCheckStatus equal_world(const NLSemanticContext *a,
                                 const NLSemanticContext *b)
{
    return nl_packet_same_entry(a, b) ? NL_CHECK_OK : INVALID;
}

static void refresh(NLSemanticContext *c, NLPlaceId parent)
{
    for (size_t i = 0; i < c->binding_count; ++i)
        if (c->bindings[i].view.place == parent &&
            c->bindings[i].view.availability == NL_AVAILABLE)
            c->bindings[i].view.value = c->places[parent - 1].current_value;
}

static NLCheckStatus end_locals(NLSemanticContext *c, size_t floor)
{
    for (size_t i = floor; i < c->binding_count; ++i) {
        NLBindingEntry *b = &c->bindings[i];
        if (b->hidden)
            continue;
        if (b->view.availability != NL_AVAILABLE ||
            !c->types[b->view.type - 1].view.is_discardable)
            return INVALID;
        NLCheckStatus s = nl_fixed_end_dependencies(c, b->view.place, floor);
        if (s != NL_CHECK_OK)
            return s;
        nl_sum_detach(c, b->view.place);
        nl_fixed_detach(c, b->view.place);
        nl_sem_end_value(c, b->view.value);
        c->places[b->view.place - 1].live = false;
        c->places[b->view.place - 1].current_value = 0;
        c->places[b->view.place - 1].current_fact = 0;
        c->places[b->view.place - 1].governing_domain = 0;
        b->view.availability = NL_CONSUMED;
        b->hidden = true;
    }
    return NL_CHECK_OK;
}

NLCheckStatus
nl_checked_ordinary_call_validate(const NLCheckedFragment *caller,
                                  NLCheckedNodeId call_id,
                                  const NLOrdinaryCallView *candidate)
{
    NLOrdinaryCallView v;
    if (!nl_checked_ordinary_call_view(caller, call_id, &v) ||
        (candidate && !view_same(&v, candidate)) || !call_path(&v))
        return INVALID;
    const NLCheckedFragment *b = v.body;
    const NLSemanticContext *entry = v.entry;
    const NLCheckedNodeView *call = nl_checked_node_view(caller, call_id);
    const NLCheckedNodeView *op = nl_checked_node_view(b, v.operation);
    const NLCheckedNodeView *loan = nl_checked_node_view(caller, v.loan);
    const NLCheckedNodeView *args[2] = {
        nl_checked_node_view(caller, v.arguments[0]),
        nl_checked_node_view(caller, v.arguments[1])};
    if (!call || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || call->argument_count != 2 ||
        call->first_argument != v.arguments[0] || call->result_count != 1 ||
        call->results[0].value != v.returned || !call->function ||
        call->function > entry->function_count || call->terminates ||
        b->context != caller->context || !args[0] || !args[1] ||
        args[0]->next_argument != v.arguments[1] || args[1]->next_argument ||
        args[0]->result_count != 1 || args[1]->result_count != 1 ||
        args[0]->kind != NL_CHECKED_IDENTIFIER ||
        args[0]->value_use != NL_VALUE_COPIED || args[0]->reborrow_scope ||
        args[1]->reborrow_scope ||
        args[1]->kind != NL_CHECKED_SUM_CONSTRUCTOR || args[1]->variant != 2 ||
        !op || op->kind != NL_CHECKED_REPLACE || !op->field.present ||
        !op->field.dependency_compatible || op->field.child != v.field ||
        op->field.parent != v.root || !v.root || v.root > entry->place_count ||
        !v.field || v.field > entry->place_count ||
        (v.root != v.originals[0] && v.root != v.originals[1]))
        return INVALID;
    const NLFunctionEntry *fn = &entry->functions[call->function - 1];
    if (fn->body != b->body_owner || !fn->body ||
        fn->body->source != b->source || fn->count != 2 ||
        fn->body->count != 2 || fn->owner_receiver || fn->owner_producer ||
        fn->custody_recipient)
        return INVALID;
    const NLSemanticTypeView ref_type =
        entry->types[fn->parameters[0] - 1].view;
    if (ref_type.kind != NL_TYPE_REF || ref_type.access != NL_ACCESS_WRITE ||
        ref_type.is_exclusive || ref_type.target != fn->parameters[1] ||
        ref_type.target != fn->result || fn->result != call->type ||
        fn->result != op->type || !entry->types[fn->result - 1].view.is_copy ||
        !entry->types[fn->result - 1].view.is_discardable)
        return INVALID;
    const NLSemanticPlaceView root = entry->places[v.root - 1],
                              field = entry->places[v.field - 1];
    if (!root.live || !field.live || root.fixed_fields[0] != v.field ||
        field.parent_aggregate != v.root || field.parent_field_index ||
        root.incarnation != op->field.parent_incarnation ||
        field.incarnation != op->field.child_incarnation ||
        op->field.nominal != root.type || op->field.type != field.type ||
        op->field.index != 0 || op->field.access != NL_ACCESS_WRITE ||
        op->field.old_value != v.returned ||
        op->field.payload_occurrence != field.payload_occurrence ||
        field.current_value != v.returned ||
        field.current_fact != op->field.child_fact ||
        root.current_fact != op->field.parent_fact ||
        field.payload_occurrence ||
        entry->values[v.returned - 1].variant != 1 || loan->loan.from_ptr ||
        !loan->loan.implicit_local || loan->loan.is_exclusive ||
        loan->loan.access != NL_ACCESS_WRITE ||
        !loan->loan.body_nonescape_proved ||
        !loan->loan.normal_result_forwarded || loan->loan.place != v.field ||
        loan->loan.incarnation != field.incarnation || loan->loan.domain ||
        loan->loan.ref_symbol != args[0]->symbol ||
        loan->loan.scope != v.reference.scope || v.reference.place != v.field ||
        v.reference.incarnation != field.incarnation ||
        v.reference.provenance != NL_PROVENANCE_VALID ||
        !v.reference.readable || !v.reference.writable ||
        v.reference.occurrence_dependency || !v.reference.scope ||
        v.reference.scope > entry->scope_count ||
        !entry->scopes[v.reference.scope - 1].active ||
        v.reference.scope != entry->scope_count ||
        entry->scopes[v.reference.scope - 1].parent ||
        entry->scopes[v.reference.scope - 1].parent_authority)
        return INVALID;
    const NLSemanticBindingView donor =
        entry->bindings[args[0]->symbol - 1].view;
    if (entry->bindings[args[0]->symbol - 1].hidden ||
        donor.availability != NL_AVAILABLE || donor.type != fn->parameters[0] ||
        !reference_same(entry->values[donor.value - 1].reference,
                        v.reference) ||
        loan->loan.source != op->field.base ||
        entry->bindings[loan->loan.source - 1].view.place != v.root)
        return INVALID;
    const NLCheckedNodeView *payload =
        nl_checked_node_view(caller, args[1]->initializer);
    const NLValueId actual_sum = args[1]->results[0].value;
    if (!payload || payload->kind != NL_CHECKED_IDENTIFIER ||
        payload->value_use != NL_VALUE_COPIED ||
        !payload->has_reference_result || payload->result_count != 1 ||
        !actual_sum || actual_sum > entry->value_count ||
        entry->values[actual_sum - 1].variant != 2 ||
        entry->values[actual_sum - 1].sum_payload !=
            payload->results[0].value ||
        !reference_same(
            payload->reference_result,
            entry->values[payload->results[0].value - 1].reference) ||
        !reference_same(
            entry->values[entry->bindings[payload->symbol - 1].view.value - 1]
                .reference,
            payload->reference_result))
        return INVALID;
    /* This finite profile has no opaque/exact value-dependency uncertainty;
     * ordinary ref scopes remain nonempty and must survive until loan close. */
    for (size_t i = 0; i < entry->value_count; ++i) {
        const NLSemanticValueView value = entry->values[i];
        if (value.carrier == NL_CARRIER_ENDED)
            continue;
        if (value.dependencies != NL_DEPENDENCY_FREE ||
            value.value_dependency_count ||
            (entry->types[value.type - 1].view.kind == NL_TYPE_REF &&
             entry->types[value.type - 1].view.is_exclusive))
            return INVALID;
    }
    for (size_t i = 0; i < 5; ++i) {
        const NLSemanticContext *worlds[] = {
            v.entry, v.before_change, v.after_change, v.post, v.closed_loan};
        if (!worlds[i] || nl_sem_validate(worlds[i]) != NL_CHECK_OK)
            return INVALID;
    }
    const NLCapturedChange *checkpoint = b->field_changes[0];
    if (checkpoint->node != v.operation || checkpoint->world != b->context ||
        checkpoint->before != checkpoint->before_origin ||
        checkpoint->after != checkpoint->after_origin)
        return INVALID;
    NLSemanticContext *expected = NULL;
    NLCheckStatus s = nl_sem_clone(entry, &expected);
    if (s != NL_CHECK_OK)
        return s;
#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            s = INVALID;                                                       \
            goto done;                                                         \
        }                                                                      \
    } while (0)
#define STEP(x)                                                                \
    do {                                                                       \
        s = (x);                                                               \
        if (s != NL_CHECK_OK)                                                  \
            goto done;                                                         \
    } while (0)
    NLValueId inputs[2], copied[2];
    for (size_t i = 0; i < 2; ++i) {
        inputs[i] = args[i]->results[0].value;
        REQUIRE(inputs[i] && inputs[i] <= entry->value_count &&
                args[i]->type == fn->parameters[i] &&
                expected->values[inputs[i] - 1].type == fn->parameters[i]);
        NLSymbolId symbol = 0;
        STEP(nl_sem_bind_in_scope(expected, fn->body->parameter_names[i],
                                  inputs[i], entry->binding_count, &symbol));
        REQUIRE(symbol == v.parameters[i]);
    }
    const NLCheckedNodeView *operands[2];
    operands[0] = nl_checked_node_view(b, op->first_argument);
    operands[1] = nl_checked_node_view(b, operands[0]->next_argument);
    for (size_t i = 0; i < 2; ++i) {
        REQUIRE(operands[i]->symbol == v.parameters[i] &&
                operands[i]->type == fn->parameters[i]);
        STEP(nl_sem_copy_value(
            expected, expected->bindings[v.parameters[i] - 1].view.value,
            &copied[i]));
        REQUIRE(copied[i] == operands[i]->results[0].value);
    }
    STEP(equal_world(expected, v.before_change));
    REQUIRE(
        reference_same(expected->values[copied[0] - 1].reference, v.reference));
    const NLSemanticValueView incoming = expected->values[copied[1] - 1];
    REQUIRE(incoming.type == field.type && incoming.variant == 2 &&
            incoming.sum_payload && op->field.new_value == copied[1]);
    const NLSemanticValueView locator =
        expected->values[incoming.sum_payload - 1];
    REQUIRE(locator.reference_count == 0 && locator.reference.scope == 0 &&
            locator.reference.provenance == NL_PROVENANCE_VALID &&
            (locator.reference.place == v.originals[0] ||
             locator.reference.place == v.originals[1]) &&
            locator.reference.incarnation ==
                expected->places[locator.reference.place - 1].incarnation &&
            expected->places[locator.reference.place - 1].live);
    STEP(nl_fixed_change_dependencies(expected, v.field, 0));
    STEP(nl_fixed_change(expected, v.field, copied[1], false));
    refresh(expected, v.root);
    STEP(equal_world(expected, v.after_change));
    REQUIRE(expected->places[v.root - 1].current_fact ==
                op->field.parent_post_fact &&
            expected->places[v.field - 1].current_fact ==
                op->field.child_post_fact &&
            expected->places[v.field - 1].payload_occurrence ==
                op->field.post_payload_occurrence &&
            op->field.post_payload_occurrence > entry->occurrence_count &&
            op->field.parent_post_fact > entry->last_value_fact &&
            op->field.child_post_fact > entry->last_value_fact &&
            op->results[0].value == v.returned);
    /* Primitive argument cleanup, then callee formal/local cleanup. */
    nl_sem_end_value(expected, copied[0]);
    STEP(end_locals(expected, entry->binding_count));
    STEP(
        nl_sem_function_exit(expected, entry->scope_count, entry->place_count));
    for (size_t i = entry->scope_count; i < expected->scope_count; ++i)
        expected->scopes[i].active = false;
    nl_sem_end_value(expected, inputs[0]);
    nl_sem_end_value(expected, inputs[1]);
    STEP(equal_world(expected, v.post));
    REQUIRE(expected->scopes[v.reference.scope - 1].active &&
            expected->values[v.returned - 1].carrier == NL_CARRIER_LOOSE &&
            expected->values[v.returned - 1].variant == 1);
    /* Source loan body contains only the already validated Copy-returning
     * call. End its real binder/scope and compare the actual closed checkpoint.
     */
    REQUIRE(loan->loan.ref_symbol &&
            loan->loan.ref_symbol == entry->binding_count);
    STEP(end_locals(expected, loan->loan.ref_symbol - 1));
    STEP(nl_sem_function_exit(expected, v.reference.scope - 1,
                              expected->place_count));
    for (size_t i = v.reference.scope - 1; i < expected->scope_count; ++i)
        expected->scopes[i].active = false;
    STEP(equal_world(expected, v.closed_loan));
    REQUIRE(!expected->scopes[v.reference.scope - 1].active);
    s = NL_CHECK_OK;
done:
    nl_semantic_destroy(expected);
    return s == NL_CHECK_INTERNAL_ERROR ? INVALID : s;
#undef REQUIRE
#undef STEP
}
