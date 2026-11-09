#include "../p1280/support.h"

typedef struct {
    const NLCheckedFragment *owner;
    NLCheckedNodeId id;
} Site;
typedef struct {
    const NLCheckedFragment *seen[256];
    size_t seen_count, match_count, reads, calls;
    Site matches[16], read, call;
} Observation;

static void healthy(const PInput *p)
{
    assert(p->entry && nl_h_origin_has_sites(p->entry) &&
           nl_h_origin_validate(p->entry) && nl_h_custody_validate(p->entry) &&
           nl_h_custody_result_valid(p->entry, nl_checked_root(p->entry)));
}
static size_t byte(const NLSemanticContext *c, NLValueId id)
{
    NLSemanticValueView v;
    assert(nl_semantic_value_view(c, id, &v) && v.scalar_known &&
           v.type == nl_semantic_core_type(c, NL_TYPE_U8) &&
           v.dependencies == NL_DEPENDENCY_FREE && !v.reference_count &&
           !v.value_dependency_count && v.scalar_value <= 255);
    return v.scalar_value;
}
static void visit(Observation *o, const NLCheckedFragment *f)
{
    if (!f)
        return;
    for (size_t i = 0; i < o->seen_count; ++i)
        if (o->seen[i] == f)
            return;
    assert(o->seen_count < 256);
    o->seen[o->seen_count++] = f;
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        assert(n);
        NLSemanticBindingView binding;
        if (n->kind == NL_CHECKED_BINDING &&
            nl_semantic_binding_view(c, n->symbol, &binding) &&
            binding.type == u8) {
            NLSourceView name;
            if (nl_source_view(f->source, n->name, &name))
                printf("BIND %.*s %zu\n", (int)name.length,
                       (const char *)name.bytes, n->symbol);
        }
        if (n->kind == NL_CHECKED_LINK_READ && n->type == u8) {
            ++o->reads;
            o->read = (Site){f, id};
        }
        NLCopyBodyParameters p;
        if (nl_checked_copy_body_parameters(f, id, &p)) {
            ++o->calls;
            o->call = (Site){f, id};
        }
        visit(o, nl_checked_call_body(f, id));
        if (n->kind != NL_CHECKED_MATCH)
            continue;
        assert(o->match_count < 16 && n->type == u8 && n->normal_arms == 2 &&
               n->item_count == 2 && n->result_count == 1 &&
               n->results[0].type == u8);
        o->matches[o->match_count++] = (Site){f, id};
        NLSemanticValueView joined;
        assert(nl_semantic_value_view(c, n->results[0].value, &joined) &&
               joined.type == u8 && !joined.scalar_known &&
               joined.dependencies == NL_DEPENDENCY_FREE &&
               !joined.value_dependency_count && !joined.reference_count &&
               !joined.reference.place && !joined.owner_place &&
               !joined.domain && !joined.allocation_region);
        for (size_t a = 0; a < 2; ++a) {
            const NLCheckedFragment *arm = nl_checked_match_arm(f, id, a);
            assert(arm && nl_checked_context(arm) != c);
            const NLCheckedNodeView *r =
                nl_checked_node_view(arm, nl_checked_root(arm));
            assert(r && !r->terminates && r->type == u8 &&
                   r->result_count == 1 && r->results[0].type == u8);
            const NLCheckedNodeView *body =
                nl_checked_node_view(arm, r->tail ? r->tail : r->initializer);
            assert(body && body->kind == NL_CHECKED_BLOCK && body->type == u8 &&
                   body->result_count == 1 &&
                   body->results[0].value == r->results[0].value &&
                   body->results[0].type == r->results[0].type);
            printf("EDGE match=%zu arm=%zu variant=%zu actual-tail=%zu "
                   "parent-result=%zu distinct-world=1\n",
                   id, a, r->variant, r->results[0].value, n->results[0].value);
            visit(o, arm);
        }
    }
}
static void destruction_controls(const char *source)
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
        assert(aa && bb && aa->arm_count && bb->arm_count &&
               a.entry->body_count && b.entry->body_count);
        if (fault == 0)
            aa->arms = bb->arms;
        else if (fault == 1)
            aa->arm_count = 0;
        else if (fault == 2)
            aa->arm_count += 17;
        else if (fault == 3)
            a.entry->body_count = 0;
        else if (fault == 4)
            a.entry->body_count += 17;
        else
            a.entry->bodies = b.entry->bodies;
        assert(!nl_h_custody_validate(a.entry) &&
               !nl_h_custody_result_valid(a.entry, nl_checked_root(a.entry)));
        /* Do not restore A's view. Canonical child custody must never free
         * B's genuine allocations or trust a caller-supplied cardinality. */
        p_dispose(&a);
        healthy(&b);
        p_dispose(&b);
        printf("DESTROY-REFUSAL %zu A-unrestored B-live=1\n", fault);
    }
}
int main(int argc, char **argv)
{
    assert(argc == 6);
    setvbuf(stdout, NULL, _IONBF, 0);
    const size_t old = strtoul(argv[2], NULL, 10),
                 incoming = strtoul(argv[3], NULL, 10),
                 sibling = strtoul(argv[4], NULL, 10),
                 field = strtoul(argv[5], NULL, 10);
    assert(field < 2);
    PInput p = {0};
    p_load(&p, argv[1]);
    const NLCheckStatus status = p_check(&p);
    printf("CHECK=%d owned=%u\n", status, p.entry != NULL);
    assert(status == NL_CHECK_OK);
    healthy(&p);
    Observation o = {0};
    visit(&o, p.entry);
    assert(o.match_count == 3 && o.reads == 1 && o.calls == 1 &&
           o.read.owner == o.call.owner);
    NLCopyBodyView body;
    NLCopyBodyParameters parameters;
    NLCopyBodyEventView event;
    const Site call = o.call;
    assert(nl_checked_copy_body_view(call.owner, call.id, &body) &&
           nl_checked_copy_body_parameters(call.owner, call.id, &parameters) &&
           nl_checked_copy_body_event(call.owner, call.id, 0, &event) &&
           nl_checked_copy_body_validate(call.owner, call.id, &body) ==
               NL_CHECK_OK &&
           nl_checked_copy_body_parameters_validate(
               call.owner, call.id, &parameters) == NL_CHECK_OK &&
           nl_checked_copy_body_event_validate(call.owner, call.id, 0,
                                               &event) == NL_CHECK_OK &&
           body.event_count == 1 && event.kind == NL_COPY_BODY_CHANGE &&
           parameters.field.index == field);
    NLSemanticPlaceView pre, post, before, after, sb, sa, heap;
    const NLCheckedNodeView *read =
        nl_checked_node_view(o.read.owner, o.read.id);
    const NLCheckedNodeView *ref =
        nl_checked_node_view(o.read.owner, read->first_argument);
    assert(
        nl_semantic_place_view(parameters.entry, read->field.parent, &heap) &&
        heap.live && heap.independent_root &&
        heap.incarnation == read->field.parent_incarnation &&
        heap.governing_domain == ref->lifetime_domain);
    assert(nl_semantic_place_view(parameters.entry, parameters.field.parent,
                                  &pre) &&
           nl_semantic_place_view(parameters.handoff, parameters.field.parent,
                                  &post) &&
           pre.live && post.live && pre.implicit_local &&
           pre.independent_root && pre.incarnation == post.incarnation &&
           pre.current_fact != post.current_fact &&
           pre.fixed_fields[field] == parameters.field.child &&
           post.fixed_fields[field] == parameters.field.child &&
           nl_semantic_place_view(parameters.entry, parameters.field.child,
                                  &before) &&
           nl_semantic_place_view(parameters.handoff, parameters.field.child,
                                  &after) &&
           before.incarnation == after.incarnation &&
           before.current_fact != after.current_fact &&
           byte(parameters.entry, before.current_value) == old &&
           byte(parameters.handoff, after.current_value) == incoming &&
           nl_semantic_place_view(parameters.entry, pre.fixed_fields[1 - field],
                                  &sb) &&
           nl_semantic_place_view(parameters.handoff,
                                  post.fixed_fields[1 - field], &sa) &&
           sb.incarnation == sa.incarnation &&
           sb.current_fact == sa.current_fact &&
           sb.current_value == sa.current_value &&
           byte(parameters.handoff, sa.current_value) == sibling);
    const NLCheckedNodeView *op = nl_checked_node_view(call.owner, call.id),
                            *loan = nl_checked_node_view(call.owner,
                                                         parameters.loan);
    NLSemanticScopeView scope;
    assert(byte(nl_checked_context(call.owner), op->results[0].value) == old &&
           loan->loan.body_nonescape_proved &&
           loan->loan.normal_result_forwarded &&
           nl_semantic_scope_view(nl_checked_context(call.owner),
                                  loan->loan.scope, &scope) &&
           !scope.active);
    size_t rejected = 0;
    for (size_t m = 0; m < o.match_count; ++m) {
        NLCheckedNodeView *n = &((NLCheckedFragment *)o.matches[m].owner)
                                    ->nodes[o.matches[m].id - 1];
        const NLCheckedNodeView saved = *n;
        for (size_t fault = 0; fault < 3; ++fault) {
            if (fault == 0)
                n->type = 1;
            else if (fault == 1)
                n->result_count = 0;
            else
                ++n->results[0].value;
            assert(
                !nl_h_custody_validate(p.entry) &&
                !nl_h_custody_result_valid(p.entry, nl_checked_root(p.entry)));
            *n = saved;
            healthy(&p);
            ++rejected;
        }
    }
    NLCopyBodyParameters bad = parameters;
    bad.field.index ^= 1;
    assert(nl_checked_copy_body_parameters_validate(call.owner, call.id,
                                                    &bad) != NL_CHECK_OK);
    ++rejected;
    NLSemanticContext *clone = NULL;
    assert(nl_sem_clone(nl_checked_context(call.owner), &clone) == NL_CHECK_OK);
    NLCheckedFragment *mutable = (NLCheckedFragment *)call.owner;
    const NLSemanticContext *world = mutable->context;
    mutable->context = clone;
    assert(nl_checked_copy_body_validate(call.owner, call.id, &body) !=
               NL_CHECK_OK &&
           !nl_h_custody_validate(p.entry));
    mutable->context = world;
    nl_semantic_destroy(clone);
    healthy(&p);
    ++rejected;
    printf("OWNED-PASS matches=%zu same-H/T-caller=1 old=%zu current=%zu "
           "sibling=%zu field=%zu closed-loan=1 refusals=%zu\n",
           o.match_count, old, incoming, sibling, field, rejected);
    p_dispose(&p);
    destruction_controls(argv[1]);
    puts("CANONICAL-DESTROY-PASS controls=6");
    return 0;
}
