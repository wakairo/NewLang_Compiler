#ifndef NEWLANG_TEST_P365_OWNED8_CUSTODY
#error "isolated test archive only"
#endif
#include <stdint.h>
#include <stdio.h>
/* Research cursor. No field/issuer is seeded from an endpoint world. */
typedef struct {
    const NLCheckedFragment *f;
    NLSemanticContext *c;
    bool *seen;
    NLCheckedNodeId stop;
    const char *reason;
} P365Cursor;
static NLCheckStatus p365_stop(P365Cursor *p, NLCheckedNodeId id, const char *s)
{
    p->stop = id;
    p->reason = s;
    fprintf(stderr, "P365_HOLD node=%zu reason=%s\n", id, s);
    return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
}
static NLCheckStatus p365_end_place(P365Cursor *p, NLSymbolId sym, size_t floor)
{
    NLSemanticContext *c = p->c;
    if (!sym || sym > c->binding_count)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLSemanticBindingView b = c->bindings[sym - 1].view;
    NLCheckStatus s = nl_fixed_end_dependencies(c, b.place, floor);
    if (s != NL_CHECK_OK)
        return s;
    for (size_t i = 0; i < c->value_count; i++) {
        NLSemanticValueView v = c->values[i];
        if (v.carrier == NL_CARRIER_ENDED ||
            c->types[v.type - 1].view.kind != NL_TYPE_REF)
            continue;
        if (c->types[v.type - 1].view.is_exclusive)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        for (size_t j = 0; j < nl_sem_ref_count(v); j++) {
            NLReferenceFacts q = nl_sem_ref_fact(v, j);
            if (q.scope && q.scope <= c->scope_count &&
                !c->scopes[q.scope - 1].active)
                continue;
            if (!q.place || !q.scope)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            if (q.occurrence_dependency &&
                c->occurrences[q.occurrence_dependency - 1].live &&
                c->occurrences[q.occurrence_dependency - 1].root == b.place)
                return NL_CHECK_SEMANTIC_ERROR;
            if (nl_fixed_overlap(c, q.place, b.place))
                return NL_CHECK_SEMANTIC_ERROR;
        }
    }
    nl_sum_detach(c, b.place);
    nl_fixed_detach(c, b.place);
    c->bindings[sym - 1].view.availability = NL_CONSUMED;
    c->places[b.place - 1].live = false;
    c->places[b.place - 1].current_value = 0;
    c->places[b.place - 1].current_fact = 0;
    c->places[b.place - 1].governing_domain = 0;
    c->values[b.value - 1].carrier = NL_CARRIER_LOOSE;
    c->values[b.value - 1].owner_place = 0;
    return NL_CHECK_OK;
}
static NLCheckStatus p365_expr(P365Cursor *, NLCheckedNodeId, NLValueId *);
static NLCheckStatus p365_receive(P365Cursor *p, NLCheckedNodeId id,
                                  NLValueId value)
{
    const NLCheckedNodeView *n = nl_checked_node_view(p->f, id);
    if (!n || p->seen[id] || n->kind != NL_CHECKED_RECEIVER ||
        n->value_use != NL_VALUE_RECEIVED || value > p->c->value_count ||
        n->type != p->c->values[value - 1].type)
        return p365_stop(p, id, "RECEIVER_SOURCE_IDENTITY");
    p->seen[id] = true;
    NLSourceView name;
    if (!nl_source_view(p->f->source, n->name, &name) || name.length > 255)
        return p365_stop(p, id, "RECEIVER_NAME");
    char text[256];
    memcpy(text, name.bytes, name.length);
    text[name.length] = 0;
    NLSymbolId symbol = 0;
    NLCheckStatus s = nl_sem_bind_in_scope(p->c, text, value, 0, &symbol);
    if (s != NL_CHECK_OK)
        return s;
    if (symbol != n->symbol)
        return p365_stop(p, id, "RECEIVER_BINDING_ID");
    if (p->c->values[value - 1].type == 2)
        p->c->places[p->c->bindings[symbol - 1].view.place - 1].implicit_local =
            true;
    return NL_CHECK_OK;
}
static NLCheckStatus p365_expr(P365Cursor *p, NLCheckedNodeId id,
                               NLValueId *out)
{
    const NLCheckedNodeView *n = nl_checked_node_view(p->f, id);
    if (!n || id > p->f->count || p->seen[id])
        return p365_stop(p, id, "MISSING_OR_DUPLICATE_DEPENDENCY");
    p->seen[id] = true;
    NLSemanticContext *c = p->c;
    NLValueId v = 0;
    NLCheckStatus s;
    if (n->kind == NL_CHECKED_IDENTIFIER) {
        if (!n->symbol || n->symbol > c->binding_count || n->result_count != 1)
            return p365_stop(p, id, "IDENTIFIER_INPUT");
        NLSemanticBindingView b = c->bindings[n->symbol - 1].view;
        if (b.availability != NL_AVAILABLE ||
            c->bindings[n->symbol - 1].hidden || !c->places[b.place - 1].live ||
            c->places[b.place - 1].current_value != b.value ||
            c->values[b.value - 1].owner_place != b.place)
            return p365_stop(p, id, "CONSUMED_OR_NONCURRENT_BINDING");
        if (c->types[b.type - 1].view.is_copy) {
            if (n->value_use != NL_VALUE_COPIED)
                return p365_stop(p, id, "COPY_USE");
            s = nl_sem_copy_value(c, b.value, &v);
        } else {
            if (n->value_use != NL_VALUE_CONSUMED ||
                n->results[0].value != b.value)
                return p365_stop(p, id, "NONCOPY_CONSUMING_RESULT");
            s = p365_end_place(p, n->symbol, SIZE_MAX);
            v = b.value;
        }
        if (s != NL_CHECK_OK)
            return s;
    } else if (n->kind == NL_CHECKED_AGGREGATE) {
        if (!n->type || n->type > c->type_count ||
            n->argument_count != c->types[n->type - 1].view.field_count ||
            n->argument_count > 4)
            return p365_stop(p, id, "AGGREGATE_TYPE");
        NLSemanticValueView package = {.type = n->type,
                                       .field_count = n->argument_count};
        bool fields[4] = {false};
        NLCheckedNodeId arg = n->first_argument;
        for (size_t j = 0; j < n->argument_count; j++) {
            const NLCheckedNodeView *a = nl_checked_node_view(p->f, arg);
            if (!a || p->seen[arg] || a->kind != NL_CHECKED_AGGREGATE_FIELD ||
                a->field_index >= n->argument_count || fields[a->field_index])
                return p365_stop(p, arg, "FIELD_PRODUCER_EDGE");
            fields[a->field_index] = true;
            p->seen[arg] = true;
            NLValueId member = 0;
            s = p365_expr(p, a->initializer, &member);
            if (s != NL_CHECK_OK)
                return s;
            if (c->values[member - 1].carrier != NL_CARRIER_LOOSE ||
                c->values[member - 1].type !=
                    c->types[n->type - 1].field_types[a->field_index])
                return p365_stop(p, arg, "MEMBER_LOOSE_TYPE");
            package.fields[a->field_index] = member;
            arg = a->next_argument;
        }
        if (arg)
            return p365_stop(p, id, "EXTRA_FIELD_EDGE");
        s = nl_sem_new_value(c, package, &v);
        if (s != NL_CHECK_OK)
            return s;
        for (size_t j = 0; j < package.field_count; j++) {
            NLValueId member = package.fields[j];
            c->values[member - 1].carrier = NL_CARRIER_AGGREGATE;
            c->values[member - 1].aggregate_owner = v;
            c->values[member - 1].owner_place = 0;
        }
    } else if (n->kind == NL_CHECKED_REF_FROM_PTR ||
               n->kind == NL_CHECKED_FIELD_REF) {
        NLValueId args[2] = {0};
        NLCheckedNodeId child = n->first_argument;
        size_t expected = n->kind == NL_CHECKED_FIELD_REF ? 1 : 2;
        if (n->argument_count != expected)
            return p365_stop(p, id, "REF_OPERAND_ARITY");
        for (size_t j = 0; j < expected; j++) {
            const NLCheckedNodeView *a = nl_checked_node_view(p->f, child);
            if (!a)
                return p365_stop(p, child, "REF_OPERAND_EDGE");
            s = p365_expr(p, child, &args[j]);
            if (s != NL_CHECK_OK)
                return s;
            child = a->next_argument;
        }
        if (child)
            return p365_stop(p, id, "EXTRA_REF_OPERAND");
        NLReferenceFacts facts = c->values[args[0] - 1].reference;
        NLTypeId target = 0;
        if (n->kind == NL_CHECKED_REF_FROM_PTR) {
            const NLSemanticValueView ptr = c->values[args[0] - 1],
                                      stable = c->values[args[1] - 1];
            if (c->types[ptr.type - 1].view.kind != NL_TYPE_PTR ||
                c->types[stable.type - 1].view.kind != NL_TYPE_REF ||
                c->types[stable.type - 1].view.target != 2 ||
                !stable.reference.scope ||
                stable.reference.scope > c->scope_count ||
                !c->scopes[stable.reference.scope - 1].active ||
                !stable.reference.place ||
                stable.reference.place > c->place_count ||
                !nl_fixed_live(c, stable.reference.place))
                return p365_stop(p, id, "STABILITY_REFERENCE");
            const NLSemanticPlaceView carrier =
                c->places[stable.reference.place - 1];
            if (carrier.incarnation != stable.reference.incarnation ||
                !carrier.current_value ||
                c->values[carrier.current_value - 1].type != 2)
                return p365_stop(p, id, "ORIGINAL_DOMAIN_CURRENT");
            NLDomainId domain = c->values[carrier.current_value - 1].domain;
            if (!facts.place || facts.place > c->place_count ||
                !nl_fixed_live(c, facts.place) ||
                c->places[facts.place - 1].incarnation != facts.incarnation ||
                c->places[facts.place - 1].governing_domain != domain ||
                !facts.readable || facts.provenance != NL_PROVENANCE_VALID ||
                n->lifetime_domain != domain)
                return p365_stop(p, id, "ORIGINAL_PTR_DOMAIN");
            s = nl_allocated_write_access(c, args[0]);
            if (s != NL_CHECK_OK)
                return s;
            facts.scope = stable.reference.scope;
            facts.writable = true;
            target = c->types[ptr.type - 1].view.target;
        } else {
            const NLCheckedField f = n->field;
            if (!f.present || !facts.place || f.parent != facts.place ||
                f.parent > c->place_count || f.child > c->place_count ||
                f.access != NL_ACCESS_WRITE ||
                c->types[c->values[args[0] - 1].type - 1].view.kind !=
                    NL_TYPE_REF)
                return p365_stop(p, id, "FIXED_FIELD_PARENT");
            const NLSemanticPlaceView root = c->places[f.parent - 1],
                                      place = c->places[f.child - 1];
            if (!nl_fixed_live(c, f.child) ||
                f.index >= root.fixed_field_count ||
                root.fixed_fields[f.index] != f.child ||
                root.incarnation != f.parent_incarnation ||
                place.incarnation != f.child_incarnation ||
                root.current_fact != f.parent_fact ||
                place.current_fact != f.child_fact ||
                place.current_value != f.old_value ||
                place.payload_occurrence != f.payload_occurrence)
                return p365_stop(p, id, "CURRENT_PROJECTION_FACT");
            facts.place = f.child;
            facts.incarnation = f.child_incarnation;
            target = place.type;
        }
        NLTypeId type = 0;
        s = nl_sem_compound(c, NL_TYPE_REF, target, NL_ACCESS_WRITE, false,
                            &type);
        if (s != NL_CHECK_OK)
            return s;
        if (type != n->type || !n->has_reference_result ||
            memcmp(&facts, &n->reference_result, sizeof(facts)))
            return p365_stop(p, id, "DERIVED_REF_RESULT");
        s = nl_sem_new_value(
            c, (NLSemanticValueView){.type = type, .reference = facts}, &v);
        if (s != NL_CHECK_OK)
            return s;
        for (size_t j = 0; j < expected; j++)
            nl_sem_end_value(c, args[j]);
    } else if (n->kind == NL_CHECKED_SUM_CONSTRUCTOR) {
        if (n->argument_count || n->variant != 1 || !n->type ||
            n->type > c->type_count ||
            c->types[n->type - 1].view.kind != NL_TYPE_SUM)
            return p365_stop(p, id, "NONE_SOURCE_CONSTRUCTOR");
        s = nl_sem_new_value(
            c, (NLSemanticValueView){.type = n->type, .variant = n->variant},
            &v);
        if (s != NL_CHECK_OK)
            return s;
    } else if (n->kind == NL_CHECKED_BINDING ||
               n->kind == NL_CHECKED_AGGREGATE_BINDING) {
        NLValueId init = 0;
        s = p365_expr(p, n->initializer, &init);
        if (s != NL_CHECK_OK)
            return s;
        NLSemanticValueView before = c->values[init - 1];
        bool destruct = n->kind == NL_CHECKED_AGGREGATE_BINDING;
        if (before.carrier != NL_CARRIER_LOOSE ||
            (destruct && before.field_count != n->argument_count))
            return p365_stop(p, id, "WHOLE_DESTRUCTURE_INPUT");
        if (destruct) {
            c->values[init - 1].carrier = NL_CARRIER_ENDED;
            for (size_t j = 0; j < before.field_count; j++) {
                NLValueId member = before.fields[j];
                if (c->values[member - 1].carrier != NL_CARRIER_AGGREGATE ||
                    c->values[member - 1].aggregate_owner != init)
                    return p365_stop(p, id, "MEMBER_CUSTODY");
                c->values[member - 1].carrier = NL_CARRIER_LOOSE;
                c->values[member - 1].aggregate_owner = 0;
            }
        }
        bool fields[4] = {false};
        NLCheckedNodeId recv = n->first_argument;
        for (size_t j = 0; j < n->argument_count; j++) {
            const NLCheckedNodeView *a = nl_checked_node_view(p->f, recv);
            if (!a || a->field_index >= 4 || fields[a->field_index])
                return p365_stop(p, recv, "RECEIVING_BIJECTION");
            fields[a->field_index] = true;
            s = p365_receive(p, recv,
                             destruct ? before.fields[a->field_index] : init);
            if (s != NL_CHECK_OK)
                return s;
            recv = a->next_argument;
        }
        if (recv)
            return p365_stop(p, id, "EXTRA_RECEIVER");
        *out = 0;
        return NL_CHECK_OK;
    } else
        return p365_stop(p, id, "UNMODELLED_SOURCE_NODE");
    if (n->result_count != 1 || n->results[0].value != v ||
        n->results[0].type != c->values[v - 1].type)
        return p365_stop(p, id, "REPLAY_PRODUCER_RESULT");
    *out = v;
    return NL_CHECK_OK;
}
static NLCheckStatus p365_edge(const NLCheckedFragment *f)
{
    if (f->field_change_count != 8 || !f->scoped_field_call)
        return NL_CHECK_OK;
    NLScopedFieldCallView call;
    NLCapturedChangeView d;
    if (!nl_checked_scoped_field_call_view(f, f->scoped_field_call, &call) ||
        !nl_checked_captured_change_view(f, 5, &d))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    P365Cursor p = {.f = f};
    NLCheckStatus s = nl_sem_clone(d.after, &p.c);
    if (s != NL_CHECK_OK)
        return s;
    p.seen = calloc(f->count + 1, sizeof(bool));
    if (!p.seen) {
        nl_semantic_destroy(p.c);
        return NL_CHECK_OUT_OF_MEMORY;
    }
    /* Resolve parents, never order by IDs/spans. */
    NLCheckedNodeId block = 0, loan = 0, binding = 0;
    for (size_t i = 1; i <= f->count; i++) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_BLOCK && n->tail == d.operation) {
            if (block) {
                s = p365_stop(&p, i, "AMBIGUOUS_D6_BLOCK");
                goto done;
            }
            block = i;
        }
    }
    for (size_t i = 1; i <= f->count; i++) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_LOAN_HEADER && n->initializer == block) {
            if (loan) {
                s = p365_stop(&p, i, "AMBIGUOUS_D6_LOAN");
                goto done;
            }
            loan = i;
        }
    }
    for (size_t i = 1; i <= f->count; i++) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_BINDING && n->initializer == loan) {
            if (binding) {
                s = p365_stop(&p, i, "AMBIGUOUS_D6_BINDING");
                goto done;
            }
            binding = i;
        }
    }
    if (!block || !loan || !binding) {
        s = p365_stop(&p, 0, "D6_SOURCE_PARENT");
        goto done;
    }
    const NLCheckedNodeView *header = nl_checked_node_view(f, loan);
    fprintf(stderr,
            "P365_ANCHOR D6=%zu block=%zu loan=%zu binding=%zu values=%zu "
            "bindings=%zu scopes=%zu\n",
            d.operation, block, loan, binding, p.c->value_count,
            p.c->binding_count, p.c->scope_count);
    if (!header->loan.ref_symbol || !header->loan.scope ||
        header->loan.scope > p.c->scope_count ||
        !header->loan.body_nonescape_proved) {
        s = p365_stop(&p, loan, "D6_LOAN_BOUNDARY");
        goto done;
    }
    const NLCheckedNodeView *op = nl_checked_node_view(f, d.operation);
    NLCheckedNodeId arg = op->first_argument;
    for (size_t i = 0; i < op->argument_count; i++) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, arg);
        if (!n || n->result_count != 1) {
            s = p365_stop(&p, arg, "D6_ARGUMENT");
            goto done;
        }
        NLValueId v = n->results[0].value;
        if (v <= p.c->value_count &&
            p.c->values[v - 1].carrier == NL_CARRIER_LOOSE &&
            p.c->types[p.c->values[v - 1].type - 1].view.kind == NL_TYPE_REF)
            nl_sem_end_value(p.c, v);
        arg = n->next_argument;
    }
    for (size_t i = header->loan.ref_symbol - 1; i < p.c->binding_count; i++) {
        if (p.c->bindings[i].hidden)
            continue;
        if (p.c->bindings[i].view.availability != NL_AVAILABLE ||
            !p.c->types[p.c->bindings[i].view.type - 1].view.is_discardable) {
            s = p365_stop(&p, loan, "BLOCK_END_AFFINE_OBLIGATION");
            goto done;
        }
        NLValueId v = p.c->bindings[i].view.value;
        s = p365_end_place(&p, i + 1, header->loan.ref_symbol - 1);
        if (s != NL_CHECK_OK)
            goto done;
        nl_sem_end_value(p.c, v);
        p.c->bindings[i].hidden = true;
    }
    s = nl_sem_function_exit(p.c, header->loan.scope - 1, p.c->place_count);
    if (s != NL_CHECK_OK)
        goto done;
    p.c->scopes[header->loan.scope - 1].active = false;
    if (header->result_count != 1 ||
        header->results[0].value != op->results[0].value) {
        s = p365_stop(&p, loan, "LOAN_RESULT_FORWARDING");
        goto done;
    }
    /* The enclosing source binding receives the already evaluated tail. */
    const NLCheckedNodeView *bn = nl_checked_node_view(f, binding);
    s = p365_receive(&p, bn->first_argument, header->results[0].value);
    if (s != NL_CHECK_OK)
        goto done;
    p.seen[binding] = true;
    p.seen[loan] = true;
    p.seen[block] = true;
    NLCheckedNodeId item = bn->next_item;
    while (item) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, item);
        if (!n) {
            s = p365_stop(&p, item, "MISSING_STATEMENT");
            goto done;
        }
        if (n->kind == NL_CHECKED_STATEMENT) {
            const NLCheckedNodeView *h =
                nl_checked_node_view(f, n->initializer);
            if (!h || h->kind != NL_CHECKED_LOAN_HEADER) {
                s = p365_stop(&p, item, "UNKNOWN_CONTINUATION");
                goto done;
            }
            fprintf(stderr,
                    "P365_REPLAY M bindings=%zu values=%zu next_statement=%zu "
                    "loan=%zu\n",
                    p.c->binding_count, p.c->value_count, item, n->initializer);
            if (h->loan.access != NL_ACCESS_READ || h->loan.is_exclusive ||
                !h->loan.source || h->loan.source > p.c->binding_count) {
                s = p365_stop(&p, n->initializer, "FRESH_DOMAIN_LOAN");
                goto done;
            }
            NLSemanticBindingView lender =
                p.c->bindings[h->loan.source - 1].view;
            if (lender.availability != NL_AVAILABLE || lender.type != 2 ||
                p.c->values[lender.value - 1].owner_place != lender.place ||
                h->loan.place != lender.place ||
                h->loan.incarnation !=
                    p.c->places[lender.place - 1].incarnation ||
                h->loan.domain != p.c->values[lender.value - 1].domain) {
                s = p365_stop(&p, n->initializer, "MOVED_DOMAIN_LOAN_IDENTITY");
                goto done;
            }
            NLScopeId scope = 0;
            s = nl_sem_new_scope(p.c, 0, true, &scope);
            if (s != NL_CHECK_OK)
                goto done;
            if (scope != h->loan.scope) {
                s = p365_stop(&p, n->initializer, "FRESH_SCOPE_ID");
                goto done;
            }
            NLTypeId type = 0;
            s = nl_sem_compound(p.c, NL_TYPE_REF, 2, NL_ACCESS_READ, false,
                                &type);
            if (s != NL_CHECK_OK)
                goto done;
            NLValueId stable = 0;
            s = nl_sem_new_value(
                p.c,
                (NLSemanticValueView){
                    .type = type,
                    .reference = {.place = lender.place,
                                  .incarnation = h->loan.incarnation,
                                  .scope = scope,
                                  .provenance = NL_PROVENANCE_VALID,
                                  .readable = true}},
                &stable);
            if (s != NL_CHECK_OK)
                goto done;
            NLSourceView name;
            if (!nl_source_view(f->source, h->loan.binding, &name) ||
                name.length > 255) {
                s = p365_stop(&p, n->initializer, "LOAN_BINDING_NAME");
                goto done;
            }
            char text[256];
            memcpy(text, name.bytes, name.length);
            text[name.length] = 0;
            NLSymbolId sym = 0;
            s = nl_sem_bind_in_scope(p.c, text, stable, p.c->binding_count,
                                     &sym);
            if (s != NL_CHECK_OK)
                goto done;
            if (sym != h->loan.ref_symbol) {
                s = p365_stop(&p, n->initializer, "LOAN_BINDING_ID");
                goto done;
            }
            const NLCheckedNodeView *body =
                nl_checked_node_view(f, h->initializer);
            if (!body || body->kind != NL_CHECKED_BLOCK) {
                s = p365_stop(&p, n->initializer, "LOAN_BODY");
                goto done;
            }
            const NLCheckedNodeView *setup =
                nl_checked_node_view(f, body->first_item);
            NLValueId unused = 0;
            s = p365_expr(&p, body->first_item, &unused);
            if (s != NL_CHECK_OK)
                goto done;
            const NLCheckedNodeView *stmt =
                nl_checked_node_view(f, setup->next_item);
            if (!stmt || stmt->kind != NL_CHECKED_STATEMENT ||
                stmt->initializer != f->scoped_field_call) {
                s = p365_stop(&p, setup->next_item, "FIRST_HELPER_LINKAGE");
                goto done;
            }
            const NLCheckedNodeView *cn =
                nl_checked_node_view(f, stmt->initializer);
            if (cn->kind != NL_CHECKED_REGISTERED_CALL ||
                cn->argument_count != 2 || !cn->function ||
                cn->function > p.c->function_count) {
                s = p365_stop(&p, stmt->initializer, "KNOWN_CALL");
                goto done;
            }
            NLValueId values[2];
            NLCheckedNodeId argument = cn->first_argument;
            for (size_t j = 0; j < 2; j++) {
                const NLCheckedNodeView *an = nl_checked_node_view(f, argument);
                if (!an) {
                    s = p365_stop(&p, argument, "CALL_ARGUMENT");
                    goto done;
                }
                s = p365_expr(&p, argument, &values[j]);
                if (s != NL_CHECK_OK)
                    goto done;
                argument = an->next_argument;
            }
            const NLFunctionEntry fn = p.c->functions[cn->function - 1];
            size_t floor = p.c->binding_count;
            if (argument || fn.count != 2 || !fn.body) {
                s = p365_stop(&p, stmt->initializer, "FORMAL_ENTRY");
                goto done;
            }
            for (size_t j = 0; j < 2; j++) {
                if (p.c->values[values[j] - 1].type != fn.parameters[j]) {
                    s = p365_stop(&p, stmt->initializer, "FORMAL_TYPE");
                    goto done;
                }
                s = nl_sem_bind_in_scope(p.c, fn.body->parameter_names[j],
                                         values[j], floor, &sym);
                if (s != NL_CHECK_OK)
                    goto done;
            }
            /* Stop immediately BEFORE the genuine callee primitive. */
            const NLCheckedNodeView *replace =
                nl_checked_node_view(call.body, call.change.operation);
            if (!replace || replace->kind != NL_CHECKED_REPLACE ||
                replace->argument_count != 2) {
                s = p365_stop(&p, 0, "C1_BEFORE_PRIMITIVE");
                goto done;
            }
            bool *caller_seen = p.seen;
            p.seen = calloc(call.body->count + 1, sizeof(bool));
            if (!p.seen) {
                p.seen = caller_seen;
                s = NL_CHECK_OUT_OF_MEMORY;
                goto done;
            }
            p.f = call.body;
            argument = replace->first_argument;
            for (size_t j = 0; j < 2; j++) {
                const NLCheckedNodeView *an =
                    nl_checked_node_view(call.body, argument);
                if (!an) {
                    s = p365_stop(&p, argument, "C1_ARGUMENT");
                    break;
                }
                NLValueId ignored = 0;
                s = p365_expr(&p, argument, &ignored);
                if (s != NL_CHECK_OK)
                    break;
                argument = an->next_argument;
            }
            free(p.seen);
            p.seen = caller_seen;
            p.f = f;
            if (s != NL_CHECK_OK)
                goto done;
            const NLSemanticContext *goal = call.change.before;
            NLSemanticSnapshot actual, expected;
            nl_semantic_snapshot(p.c, &actual);
            nl_semantic_snapshot(goal, &expected);
            if (memcmp(&actual, &expected, sizeof(actual))) {
                fprintf(stderr,
                        "P365_ENDPOINT counts=%zu:%zu:%zu:%zu:%zu:%zu "
                        "expected=%zu:%zu:%zu:%zu:%zu:%zu\n",
                        actual.types, actual.bindings, actual.values,
                        actual.places, actual.scopes, actual.last_value_fact,
                        expected.types, expected.bindings, expected.values,
                        expected.places, expected.scopes,
                        expected.last_value_fact);
                s = p365_stop(&p, stmt->initializer, "ENDPOINT_SNAPSHOT");
                goto done;
            }
            for (size_t j = 0; j < p.c->place_count; j++)
                if (memcmp(&p.c->places[j], &goal->places[j],
                           sizeof(*p.c->places))) {
                    fprintf(stderr, "P365_ENDPOINT place=%zu\n", j + 1);
                    s = p365_stop(&p, stmt->initializer, "ENDPOINT_PLACE");
                    goto done;
                }
            for (size_t j = 0; j < p.c->value_count; j++)
                if (memcmp(&p.c->values[j], &goal->values[j],
                           sizeof(*p.c->values))) {
                    fprintf(stderr,
                            "P365_ENDPOINT value=%zu carrier=%d expected=%d\n",
                            j + 1, p.c->values[j].carrier,
                            goal->values[j].carrier);
                    s = p365_stop(&p, stmt->initializer, "ENDPOINT_VALUE");
                    goto done;
                }
            for (size_t j = 0; j < p.c->binding_count; j++)
                if (memcmp(&p.c->bindings[j].view, &goal->bindings[j].view,
                           sizeof(NLSemanticBindingView)) ||
                    p.c->bindings[j].hidden != goal->bindings[j].hidden) {
                    s = p365_stop(&p, stmt->initializer, "ENDPOINT_BINDING");
                    goto done;
                }
            if (memcmp(p.c->scopes, goal->scopes,
                       p.c->scope_count * sizeof(*p.c->scopes))) {
                s = p365_stop(&p, stmt->initializer, "ENDPOINT_SCOPE");
                goto done;
            }
            fprintf(stderr,
                    "P365_REPLAY endpoint_equal=true bindings=%zu values=%zu "
                    "scopes=%zu\n",
                    p.c->binding_count, p.c->value_count, p.c->scope_count);
            s = NL_CHECK_OK;
            goto done;
        }
        NLValueId ignored = 0;
        s = p365_expr(&p, item, &ignored);
        if (s != NL_CHECK_OK)
            goto done;
        item = n->next_item;
    }
    s = p365_stop(&p, 0, "NO_C1_CONTINUATION");
done:
    free(p.seen);
    nl_semantic_destroy(p.c);
    return s;
}
static NLCheckStatus p365_walk(const NLCheckedFragment *f,
                               NLCheckedNodeId match)
{
    NLCheckStatus s = p365_edge(f);
    if (s != NL_CHECK_OK)
        return s;
    for (size_t i = 0; i < 2; i++) {
        const NLCheckedFragment *a = nl_checked_match_arm(f, match, i);
        if (!a)
            continue;
        s = p365_edge(a);
        if (s != NL_CHECK_OK)
            return s;
        for (size_t j = 1; j <= nl_checked_node_count(a); j++) {
            const NLCheckedNodeView *n = nl_checked_node_view(a, j);
            if (n->kind == NL_CHECKED_MATCH && n->allocation_trial) {
                s = p365_walk(a, j);
                if (s != NL_CHECK_OK)
                    return s;
            }
        }
    }
    return NL_CHECK_OK;
}
