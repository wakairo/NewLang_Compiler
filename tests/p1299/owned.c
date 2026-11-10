#include "../p1280/support.h"
#include "copy_old_result_internal.h"

typedef struct {
    const NLCheckedFragment *owner;
    NLCheckedNodeId id;
    NLCopyBodyView body;
    NLCopyBodyParameters parameters;
    NLCopyBodyEventView event;
} Call;
typedef struct {
    const NLCheckedFragment *seen[128];
    size_t seen_count, call_count, matches;
    Call calls[2];
} Observation;
#ifdef P1299_EMIT
static char *baseline;
static size_t baseline_length;
#endif
static void healthy(const PInput *p)
{
    assert(nl_h_origin_validate(p->entry) && nl_h_custody_validate(p->entry) &&
           nl_h_custody_result_valid(p->entry, nl_checked_root(p->entry)));
#ifdef P1299_EMIT
    char *text = NULL;
    size_t length = 98765;
    assert(nl_checked_c_node(p->entry, &text, &length) == NL_NODE_C_OK && text);
    if (!baseline) {
        baseline = text;
        baseline_length = length;
    } else {
        assert(length == baseline_length && !memcmp(text, baseline, length));
        free(text);
    }
#endif
}
static void refused(const PInput *p)
{
    assert(!nl_h_custody_validate(p->entry));
#ifdef P1299_EMIT
    char *text = NULL;
    size_t length = 98765;
    assert(nl_checked_c_node(p->entry, &text, &length) ==
               NL_NODE_C_UNSUPPORTED && !text && length == 98765);
#endif
}
static NLSemanticValueView scalar(const NLSemanticContext *c, NLValueId id)
{
    NLSemanticValueView v;
    assert(nl_semantic_value_view(c, id, &v) &&
           v.type == nl_semantic_core_type(c, NL_TYPE_U8) &&
           v.dependencies == NL_DEPENDENCY_FREE && !v.reference_count &&
           !v.reference.place && !v.value_dependency_count && !v.owner_place &&
           !v.domain && !v.allocation_region);
    return v;
}
static void visit(Observation *o, const NLCheckedFragment *f)
{
    if (!f)
        return;
    for (size_t i = 0; i < o->seen_count; ++i)
        if (o->seen[i] == f)
            return;
    assert(o->seen_count < 128);
    o->seen[o->seen_count++] = f;
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        NLSemanticBindingView b;
        if (n->kind == NL_CHECKED_BINDING &&
            nl_semantic_binding_view(nl_checked_context(f), n->symbol, &b) &&
            b.type == nl_semantic_core_type(nl_checked_context(f), NL_TYPE_U8)) {
            NLSourceView name;
            if (nl_source_view(f->source, n->name, &name))
                printf("BIND %.*s %zu\n", (int)name.length,
                       (const char *)name.bytes, n->symbol);
        }
        NLCopyBodyParameters parameters;
        if (nl_checked_copy_body_parameters(f, id, &parameters)) {
            assert(o->call_count < 2);
            Call *call = &o->calls[o->call_count++];
            call->owner = f;
            call->id = id;
            call->parameters = parameters;
            assert(nl_checked_copy_body_view(f, id, &call->body) &&
                   nl_checked_copy_body_event(f, id, 0, &call->event) &&
                   nl_checked_copy_body_validate(f, id, &call->body) ==
                       NL_CHECK_OK &&
                   nl_checked_copy_body_parameters_validate(f, id,
                                                            &parameters) ==
                       NL_CHECK_OK &&
                   nl_checked_copy_body_event_validate(f, id, 0,
                                                       &call->event) ==
                       NL_CHECK_OK && call->body.event_count == 1 &&
                   call->event.kind == NL_COPY_BODY_CHANGE &&
                   copy_old_result(&call->body, &call->event,
                                   nl_semantic_core_type(nl_checked_context(f),
                                                         NL_TYPE_U8)));
        }
        visit(o, nl_checked_call_body(f, id));
        if (n->kind != NL_CHECKED_MATCH)
            continue;
        ++o->matches;
        assert(n->type == nl_semantic_core_type(nl_checked_context(f), NL_TYPE_U8) &&
               n->normal_arms == 2 && n->item_count == 2 &&
               !scalar(nl_checked_context(f), n->results[0].value).scalar_known);
        for (size_t arm = 0; arm < 2; ++arm)
            visit(o, nl_checked_match_arm(f, id, arm));
    }
}
static void facts(const Call *c, bool outer)
{
    const NLCopyBodyParameters *p = &c->parameters;
    NLSemanticPlaceView before, after, old, now, sibling_before, sibling_after;
    assert(p->field.index == 0 &&
           nl_semantic_place_view(p->entry, p->field.parent, &before) &&
           nl_semantic_place_view(p->handoff, p->field.parent, &after) &&
           before.live && after.live && before.independent_root &&
           before.implicit_local && !before.governing_domain &&
           before.incarnation == after.incarnation &&
           before.current_fact != after.current_fact &&
           before.fixed_fields[0] == p->field.child &&
           after.fixed_fields[0] == p->field.child &&
           nl_semantic_place_view(p->entry, p->field.child, &old) &&
           nl_semantic_place_view(p->handoff, p->field.child, &now) &&
           old.incarnation == now.incarnation &&
           old.current_fact != now.current_fact &&
           nl_semantic_place_view(p->entry, before.fixed_fields[1],
                                  &sibling_before) &&
           nl_semantic_place_view(p->handoff, after.fixed_fields[1],
                                  &sibling_after) &&
           sibling_before.current_fact == sibling_after.current_fact &&
           sibling_before.current_value == sibling_after.current_value);
    const size_t expected_old = outer ? 103 : 67,
                 expected_sibling = outer ? 227 : 193;
    const NLSemanticValueView previous = scalar(p->entry, old.current_value),
                              next = scalar(p->handoff, now.current_value),
                              sibling = scalar(p->handoff,
                                               sibling_after.current_value);
    assert(previous.scalar_known && previous.scalar_value == expected_old &&
           sibling.scalar_known && sibling.scalar_value == expected_sibling &&
           (outer ? !next.scalar_known
                  : next.scalar_known && next.scalar_value == 149));
    const NLCheckedNodeView *node = nl_checked_node_view(c->owner, c->id),
                            *loan = nl_checked_node_view(c->owner, p->loan);
    NLSemanticScopeView scope;
    const NLSemanticValueView result =
        scalar(nl_checked_context(c->owner), node->results[0].value);
    assert(result.scalar_known && result.scalar_value == expected_old &&
           loan->loan.source == p->field.base &&
           loan->loan.place == p->field.child &&
           loan->loan.incarnation == p->field.child_incarnation &&
           loan->loan.body_nonescape_proved &&
           loan->loan.normal_result_forwarded &&
           nl_semantic_scope_view(nl_checked_context(c->owner),
                                  loan->loan.scope, &scope) && !scope.active);
    const NLSemanticContext *body = nl_checked_context(c->body.body);
    NLSemanticBindingView formal;
    assert(nl_semantic_binding_view(body, p->parameters[1], &formal));
    const NLSemanticValueView incoming = scalar(body, formal.value);
    assert(outer ? !incoming.scalar_known
                 : incoming.scalar_known && incoming.scalar_value == 149);
    if (outer) {
        NLSemanticSnapshot closed;
        assert(nl_semantic_snapshot(p->entry, &closed) &&
               !closed.backing_regions && !closed.domains && !closed.raw_intervals);
    }
    printf("CALL role=%s function=%zu old=%zu current=%s sibling=%zu "
           "same-original=1 field=0 closed-loan=1 actual-formal=1\n",
           outer ? "postmatch" : "inside", c->body.function, expected_old,
           outer ? "runtime-u8" : "149", expected_sibling);
}
static void prefix(const PInput *p, const NLCheckedFragment *body,
                   const Call *call)
{
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body)),
                            *binding = nl_checked_node_view(body, root->first_item),
                            *result = nl_checked_node_view(body, binding->next_item),
                            *match = nl_checked_node_view(body, result->initializer);
    assert(binding->kind == NL_CHECKED_BINDING &&
           binding->symbol == call->parameters.field.base &&
           match->kind == NL_CHECKED_MATCH && match->normal_frame_unchanged &&
           match->match_binding_prefix >= binding->symbol);
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *a =
            nl_checked_match_arm(body, result->initializer, i);
        const NLCheckedNodeView *arm =
            nl_checked_node_view(a, nl_checked_root(a));
        assert(nl_h_origin_edge(p->entry, body, result->initializer, i, a) &&
               arm->normal_frame_unchanged);
        if (arm->variant != 1)
            continue;
        NLSemanticBindingView inherited;
        NLSemanticPlaceView before, entry;
        assert(nl_semantic_binding_view(nl_checked_context(a), binding->symbol,
                                        &inherited) &&
               inherited.place == call->parameters.field.parent &&
               nl_semantic_place_view(nl_checked_context(a), inherited.place,
                                      &before) &&
               nl_semantic_place_view(call->parameters.entry, inherited.place,
                                      &entry) && before.live && entry.live &&
               before.incarnation == entry.incarnation &&
               before.current_fact == entry.current_fact &&
               before.current_value == entry.current_value);
        for (size_t j = 0; j < 2; ++j) {
            NLSemanticPlaceView x, y;
            assert(before.fixed_fields[j] == entry.fixed_fields[j] &&
                   nl_semantic_place_view(nl_checked_context(a),
                                          before.fixed_fields[j], &x) &&
                   nl_semantic_place_view(call->parameters.entry,
                                          entry.fixed_fields[j], &y) &&
                   x.incarnation == y.incarnation &&
                   x.current_fact == y.current_fact &&
                   x.current_value == y.current_value);
        }
    }
    puts("PREFIX-PASS original-before-H-to-real-call-entry=1");
}
static void destruction(const char *source)
{
    for (size_t fault = 0; fault < 6; ++fault) {
        PInput a = {0}, b = {0};
        p_load(&a, source);
        p_load(&b, source);
        assert(p_check(&a) == NL_CHECK_OK && p_check(&b) == NL_CHECK_OK);
        healthy(&a);
        healthy(&b);
        NLCheckedFragment *aa = (NLCheckedFragment *)nl_checked_call_body(
                              a.entry, nl_checked_root(a.entry)),
                          *bb = (NLCheckedFragment *)nl_checked_call_body(
                              b.entry, nl_checked_root(b.entry));
        assert(aa->arm_count && bb->arm_count && aa->body_count && bb->body_count);
        if (fault == 0)
            aa->arms = bb->arms;
        else if (fault == 1)
            aa->arm_count = 0;
        else if (fault == 2)
            aa->arm_count += 17;
        else if (fault == 3)
            aa->body_count = 0;
        else if (fault == 4)
            aa->body_count += 17;
        else
            aa->bodies = bb->bodies;
        refused(&a);
        p_dispose(&a); /* Deliberately unrestored; B must remain a true owner. */
        healthy(&b);
        p_dispose(&b);
        printf("CANONICAL-DESTROY fault=%zu unrestored-A B-live=1\n", fault);
    }
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    setvbuf(stdout, NULL, _IONBF, 0);
    PInput p = {0};
    p_load(&p, argv[1]);
    assert(p_check(&p) == NL_CHECK_OK);
    healthy(&p);
    const NLCheckedFragment *main_body =
        nl_checked_call_body(p.entry, nl_checked_root(p.entry));
    Observation o = {0};
    visit(&o, p.entry);
    assert(o.call_count == 2 && o.matches == 3 &&
           o.calls[0].owner != o.calls[1].owner &&
           o.calls[0].body.function != o.calls[1].body.function);
    for (size_t i = 0; i < 2; ++i) {
        Call *c = &o.calls[i], *other = &o.calls[1 - i];
        facts(c, c->owner == main_body);
        if (c->owner == main_body)
            prefix(&p, main_body, c);
        NLCopyBodyParameters bad = c->parameters;
        bad.field.index ^= 1;
        assert(nl_checked_copy_body_parameters_validate(c->owner, c->id,
                                                        &bad) != NL_CHECK_OK);
        bad = c->parameters;
        bad.entry = other->parameters.entry;
        assert(nl_checked_copy_body_parameters_validate(c->owner, c->id,
                                                        &bad) != NL_CHECK_OK);
        NLCopyBodyView foreign = c->body;
        foreign.body = other->body.body;
        assert(nl_checked_copy_body_validate(c->owner, c->id, &foreign) !=
               NL_CHECK_OK);
        NLCheckedFragment *f = (NLCheckedFragment *)c->owner;
        NLSemanticContext *clone = NULL;
        assert(nl_sem_clone(f->context, &clone) == NL_CHECK_OK);
        const NLSemanticContext *saved = f->context;
        f->context = clone;
        assert(nl_checked_copy_body_validate(f, c->id, &c->body) != NL_CHECK_OK);
        refused(&p);
        f->context = saved;
        nl_semantic_destroy(clone);
        healthy(&p);
        NLCheckedNodeView *node = &f->nodes[c->id - 1];
        const NLCheckedNodeView original = *node;
        for (size_t fault = 0; fault < 3; ++fault) {
            if (fault == 0)
                node->function = other->body.function;
            else if (fault == 1)
                node->result_count = 0;
            else
                ++node->results[0].value;
            assert(nl_checked_copy_body_validate(f, c->id, &c->body) !=
                   NL_CHECK_OK);
            refused(&p);
            *node = original;
            healthy(&p);
        }
    }
    puts("OWNED-PASS two-source-callers=1 public-refusals=14");
    p_dispose(&p);
    destruction(argv[1]);
#ifdef P1299_EMIT
    free(baseline);
#endif
    return 0;
}
