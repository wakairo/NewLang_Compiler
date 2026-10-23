#include "newlang/copy_body.h"
#include "semantic_internal.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define INVALID NL_CHECK_ANALYSIS_PRECISION_LIMIT
/* Issuance anchors are private, not writable public views or host inputs.
 * The completed graph seal authenticates identity, not lost lifetime state:
 * those facts are captured independently while the original is still live. */
#include "copy_body_internal.h"
#include "if_field_internal.h"

void nl_copy_state_destroy(NLCopyCallAnchor *a)
{
    free(a->places);
    free(a->values);
    free(a->scopes);
    free(a->bindings);
}
NLCheckStatus nl_copy_state_anchor(const NLSemanticContext *c,
                                   NLCopyCallAnchor *a)
{
    a->place_count = c->place_count;
    a->value_count = c->value_count;
    a->scope_count = c->scope_count;
    a->binding_count = c->binding_count;
#define SAVE(member, count)                                                    \
    do {                                                                       \
        if (a->count) {                                                        \
            a->member = malloc(a->count * sizeof(*a->member));                 \
            if (!a->member)                                                    \
                return NL_CHECK_OUT_OF_MEMORY;                                 \
            memcpy(a->member, c->member, a->count * sizeof(*a->member));       \
        }                                                                      \
    } while (0)
    SAVE(places, place_count);
    SAVE(values, value_count);
    SAVE(scopes, scope_count);
#undef SAVE
    if (a->binding_count) {
        a->bindings = malloc(a->binding_count * sizeof(*a->bindings));
        if (!a->bindings)
            return NL_CHECK_OUT_OF_MEMORY;
        for (size_t i = 0; i < a->binding_count; ++i)
            a->bindings[i] = c->bindings[i].view;
    }
    return NL_CHECK_OK;
}
bool nl_copy_state_matches(const NLSemanticContext *c,
                           const NLCopyCallAnchor *a)
{
    if (!c || c->place_count != a->place_count ||
        c->value_count != a->value_count || c->scope_count != a->scope_count ||
        c->binding_count != a->binding_count)
        return false;
    if ((a->place_count &&
         memcmp(c->places, a->places, a->place_count * sizeof(*a->places))) ||
        (a->value_count &&
         memcmp(c->values, a->values, a->value_count * sizeof(*a->values))) ||
        (a->scope_count &&
         memcmp(c->scopes, a->scopes, a->scope_count * sizeof(*a->scopes))))
        return false;
    for (size_t i = 0; i < a->binding_count; ++i)
        if (memcmp(&c->bindings[i].view, &a->bindings[i], sizeof(*a->bindings)))
            return false;
    return true;
}
static void destroy(NLCheckedFragment *f)
{
    NLCopyBodyProof *p = f->copy_body;
    if (!p)
        return;
    while (p->first) {
        CopyEvent *e = p->first;
        p->first = e->next;
        nl_semantic_destroy(e->before);
        nl_semantic_destroy(e->after);
        free(e->scopes_before);
        free(e->scopes_after);
        free(e);
    }
    nl_semantic_destroy(p->normal_end);
    nl_copy_state_destroy(&p->normal_end_anchor);
    nl_semantic_destroy(p->entry);
    nl_semantic_destroy(p->handoff);
    nl_copy_state_destroy(&p->entry_anchor);
    nl_copy_state_destroy(&p->handoff_anchor);
    nl_semantic_destroy(p->pending);
    nl_semantic_destroy(p->exit);
    free(p->exit_scopes);
    free(p->nodes);
    free(p->caller_nodes);
    free(p);
}
static bool nominal(const NLSemanticContext *c, NLTypeId t)
{
    return nl_fixed_type(c, t) && !nl_recursive_local_type(c, t) &&
           !nl_array4_type(c, t);
}
static NLCheckStatus scopes(const NLSemanticContext *c,
                            NLSemanticScopeView **out, size_t *count)
{
    *count = c->scope_count;
    if (!*count)
        return NL_CHECK_OK;
    *out = malloc(*count * sizeof(**out));
    if (!*out)
        return NL_CHECK_OUT_OF_MEMORY;
    memcpy(*out, c->scopes, *count * sizeof(**out));
    return NL_CHECK_OK;
}
bool nl_copy_trio(const NLSemanticContext *c, NLPlaceId root,
                  NLSemanticPlaceView out[3])
{
    if (!c || !root || root > c->place_count)
        return false;
    const NLSemanticPlaceView r = c->places[root - 1];
    if (!r.live || !r.independent_root || !r.implicit_local ||
        r.parent_aggregate || r.parent_sum || r.governing_domain ||
        r.placement.region || !r.current_fact || !r.current_value ||
        r.current_value > c->value_count || r.fixed_field_count != 2 ||
        !nominal(c, r.type))
        return false;
    out[0] = r;
    for (size_t i = 0; i < 2; ++i) {
        const NLPlaceId id = r.fixed_fields[i];
        if (!id || id > c->place_count || id == root ||
            (i && id == r.fixed_fields[0]))
            return false;
        out[i + 1] = c->places[id - 1];
        const NLSemanticPlaceView f = out[i + 1];
        if (!nl_fixed_live(c, id) || f.independent_root ||
            f.parent_aggregate != root ||
            f.parent_incarnation != r.incarnation ||
            f.parent_field_index != i || f.governing_domain || f.parent_sum ||
            f.payload_occurrence || f.placement.region || !f.current_fact ||
            !f.current_value || f.current_value > c->value_count ||
            f.type != nl_semantic_core_type(c, NL_TYPE_U8))
            return false;
    }
    return true;
}
NLCheckStatus nl_copy_body_parameters(NLCheckedFragment *f,
                                      const NLCheckedFragment *caller,
                                      NLCheckedNodeId call,
                                      const NLSymbolId parameters[2],
                                      const NLSemanticContext *c)
{
    const NLCheckedNodeView *n = nl_checked_node_view(caller, call);
    if (!n || !n->function || n->function > c->function_count)
        return NL_CHECK_OK;
    const NLFunctionEntry *fn = &c->functions[n->function - 1];
    if (fn->count != 2 ||
        fn->parameters[1] != nl_semantic_core_type(c, NL_TYPE_U8))
        return NL_CHECK_OK;
    const NLSemanticTypeView t = c->types[fn->parameters[0] - 1].view;
    if (t.kind != NL_TYPE_REF || t.target != fn->parameters[1] ||
        t.access != NL_ACCESS_WRITE || t.is_exclusive)
        return NL_CHECK_OK;
    const NLCheckedNodeView *a =
        nl_checked_node_view(caller, n->first_argument);
    const NLCheckedNodeView *b =
        a ? nl_checked_node_view(caller, a->next_argument) : NULL;
    if (!a || !b || a->kind != NL_CHECKED_IDENTIFIER || a->result_count != 1 ||
        b->result_count != 1 || b->next_argument || !a->results[0].value ||
        a->results[0].value > c->value_count)
        return NL_CHECK_OK;
    const NLReferenceFacts ref = c->values[a->results[0].value - 1].reference;
    NLCheckedNodeId loan = 0;
    for (size_t i = 1; i <= caller->count; ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(caller, i);
        if (v->kind == NL_CHECKED_LOAN_HEADER &&
            v->loan.ref_symbol == a->symbol && v->loan.scope == ref.scope &&
            v->field.present) {
            if (loan)
                return NL_CHECK_OK;
            loan = i;
        }
    }
    if (!loan)
        return NL_CHECK_OK;
    const NLCheckedNodeView *l = nl_checked_node_view(caller, loan);
    NLSemanticPlaceView trio_entry[3];
    if (!nl_copy_trio(c, l->field.parent, trio_entry))
        return NL_CHECK_OK;
    NLCopyBodyProof *p = calloc(1, sizeof(*p));
    if (!p)
        return NL_CHECK_OUT_OF_MEMORY;
    f->copy_body = p;
    f->destroy_copy_body = destroy;
    p->owner = f;
    p->caller = caller;
    p->call = call;
    p->body = f->body_owner;
    p->source = f->source;
    p->formal_write = true;
    p->unit_call = fn->result == 1;
    p->binding_floor = parameters[0] - 1;
    p->place_floor = c->bindings[parameters[0] - 1].view.place - 1;
    p->scope_floor = c->scope_count;
    p->parameters = (NLCopyBodyParameters){
        .loan = loan,
        .arguments = {n->first_argument, a->next_argument},
        .parameters = {parameters[0], parameters[1]},
        .field = l->field};
    p->arguments[0] = *a;
    p->arguments[1] = *b;
    p->loan_node = *l;
    p->parameter_types[0] = fn->parameters[0];
    p->parameter_types[1] = fn->parameters[1];
    NLCheckStatus status = nl_sem_clone(c, &p->entry);
    if (status == NL_CHECK_OK)
        status = nl_copy_state_anchor(c, &p->entry_anchor);
    p->parameters.entry = p->entry;
    return status;
}
NLCheckStatus nl_copy_body_before(NLCheckedFragment *f, NLCheckedNodeId id,
                                  const NLSemanticContext *c)
{
    NLCopyBodyProof *p = f->copy_body;
    if (!p)
        return NL_CHECK_OK;
    if (p->pending || p->sealed)
        return NL_CHECK_INTERNAL_ERROR;
    p->pending_node = id;
    return nl_sem_clone(c, &p->pending);
}
NLCheckStatus nl_copy_body_event(NLCheckedFragment *f, NLCheckedNodeId id,
                                 const NLSemanticContext *c, bool placement)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (!n || !f->body_owner)
        return NL_CHECK_OK;
    NLCopyBodyProof *p = f->copy_body;
    NLSymbolId binding = 0;
    NLPlaceId root = 0;
    if (placement) {
        if (n->kind != NL_CHECKED_BINDING || !n->symbol ||
            n->symbol > c->binding_count ||
            !nominal(c, c->bindings[n->symbol - 1].view.type))
            return NL_CHECK_OK;
        binding = n->symbol;
        root = c->bindings[binding - 1].view.place;
        if (!p) {
            p = calloc(1, sizeof(*p));
            if (!p)
                return NL_CHECK_OUT_OF_MEMORY;
            f->copy_body = p;
            f->destroy_copy_body = destroy;
            p->owner = f;
            p->body = f->body_owner;
            p->source = f->source;
        }
    } else {
        if (!p)
            return NL_CHECK_OK;
        root = n->field.parent;
    }
    if (p->sealed || p->view.event_count >= NL_SEMANTIC_MAX_ENTRIES)
        return NL_CHECK_RESOURCE_LIMIT;
    CopyEvent *e = calloc(1, sizeof(*e));
    if (!e)
        return NL_CHECK_OUT_OF_MEMORY;
    if (p->last)
        p->last->next = e;
    else
        p->first = e;
    p->last = e;
    ++p->view.event_count;
    e->view.kind = placement                          ? NL_COPY_BODY_INITIALIZE
                   : n->kind == NL_CHECKED_FIELD_READ ? NL_COPY_BODY_READ
                                                      : NL_COPY_BODY_CHANGE;
    e->view.node = id;
    e->view.binding = binding;
    e->view.field = n->field;
    e->view.result = n->result_count ? n->results[0].value : 0;
    if (placement) {
        e->view.field.parent = root;
        e->view.field.base = binding;
    }
    if (e->view.kind == NL_COPY_BODY_CHANGE) {
        if (!p->pending || p->pending_node != id)
            return NL_CHECK_INTERNAL_ERROR;
        e->before = p->pending;
        p->pending = NULL;
        const NLCheckedNodeView *a = nl_checked_node_view(f, n->first_argument);
        const NLCheckedNodeView *b =
            a ? nl_checked_node_view(f, a->next_argument) : NULL;
        if (!b || b->result_count != 1)
            return NL_CHECK_INTERNAL_ERROR;
        e->view.incoming = b->results[0].value;
    }
    NLCheckStatus s = nl_sem_clone(c, &e->after);
    if (s != NL_CHECK_OK)
        return s;
    e->view.before = e->before;
    e->view.after = e->after;
    e->issued = e->view;
    if (!nl_copy_trio(c, root, e->post) ||
        (e->before && !nl_copy_trio(e->before, root, e->pre)))
        return NL_CHECK_INTERNAL_ERROR;
    for (size_t i = 0; i < 3; ++i) {
        e->post_values[i] = c->values[e->post[i].current_value - 1];
        if (e->before)
            e->pre_values[i] = e->before->values[e->pre[i].current_value - 1];
    }
    if (e->view.result)
        e->result = c->values[e->view.result - 1];
    if (e->view.incoming)
        e->incoming = e->before->values[e->view.incoming - 1];
    s = scopes(c, &e->scopes_after, &e->after_count);
    return s == NL_CHECK_OK && e->before
               ? scopes(e->before, &e->scopes_before, &e->before_count)
               : s;
}
NLCheckStatus nl_copy_body_exit(NLCheckedFragment *f,
                                const NLSemanticContext *c, NLCheckedResult r)
{
    NLCopyBodyProof *p = f->copy_body;
    if (!p)
        return NL_CHECK_OK;
    if (p->exit)
        return NL_CHECK_OK; /* richer branching will not pass the graph gate */
    p->view.result = r;
    NLCheckStatus s = nl_sem_clone(c, &p->exit);
    if (s == NL_CHECK_OK)
        s = scopes(c, &p->exit_scopes, &p->scope_count);
    p->view.live_exit = p->exit;
    return s;
}
NLCheckStatus nl_copy_body_seal_calls(NLCheckedFragment *f)
{
    /* Each completed parent authenticates the entire caller graph. At the
     * inner call's return the surrounding loan/body was still being checked.
     * Stage every direct child's copy before publishing any attachment. This
     * also closes completed match-arm callers without importing parent worlds.
     * No recursion or partially built graph is a completed call witness. */
    if (f->body_count > 64)
        return NL_CHECK_RESOURCE_LIMIT;
    NLCheckedNodeView *pending[64] = {0};
    NLCheckStatus status = NL_CHECK_OK;
    for (size_t i = 0; i < f->body_count; ++i) {
        NLCopyBodyProof *child = f->bodies[i]->copy_body;
        if (!child || !child->formal_write || !child->sealed)
            continue;
        if (child->caller != f || child->caller_nodes || !f->count ||
            f->count > NL_SEMANTIC_MAX_ENTRIES) {
            status = NL_CHECK_INTERNAL_ERROR;
            goto failure;
        }
        pending[i] = malloc(f->count * sizeof(*f->nodes));
        if (!pending[i]) {
            status = NL_CHECK_OUT_OF_MEMORY;
            goto failure;
        }
        memcpy(pending[i], f->nodes, f->count * sizeof(*f->nodes));
    }
    for (size_t i = 0; i < f->body_count; ++i) {
        if (!pending[i])
            continue;
        NLCopyBodyProof *child = f->bodies[i]->copy_body;
        child->caller_nodes = pending[i];
        child->caller_nodes_origin = f->nodes;
        child->caller_count = f->count;
        child->caller_root = f->root;
        child->caller_source = f->source;
        child->caller_body = f->body_owner;
    }
    return NL_CHECK_OK;
failure:
    for (size_t i = 0; i < f->body_count; ++i)
        free(pending[i]);
    return status;
}
NLCheckStatus nl_copy_body_seal(NLCheckedFragment *f,
                                const NLCheckedFragment *caller,
                                NLCheckedNodeId call, size_t function,
                                const NLSemanticContext *c)
{
    const NLCheckStatus completed = nl_copy_body_seal_calls(f);
    if (completed != NL_CHECK_OK)
        return completed;
    NLCopyBodyProof *p = f->copy_body;
    if (!p)
        return NL_CHECK_OK;
    if (!p->exit || p->pending || !f->count ||
        f->count > NL_SEMANTIC_MAX_ENTRIES)
        return NL_CHECK_OK; /* unproved richer body stays backend unsupported */
    if (p->formal_write) {
        NLCheckStatus status = nl_sem_clone(c, &p->handoff);
        if (status == NL_CHECK_OK)
            status = nl_copy_state_anchor(c, &p->handoff_anchor);
        if (status != NL_CHECK_OK)
            return status;
        p->parameters.handoff = p->handoff;
        p->issued_parameters = p->parameters;
        p->call_node = *nl_checked_node_view(caller, call);
    }
    p->nodes = malloc(f->count * sizeof(*p->nodes));
    if (!p->nodes)
        return NL_CHECK_OUT_OF_MEMORY;
    memcpy(p->nodes, f->nodes, f->count * sizeof(*p->nodes));
    p->nodes_origin = f->nodes;
    p->count = f->count;
    p->caller = caller;
    p->call = call;
    p->view.body = f;
    p->view.function = function;
    p->view.root = f->root;
    p->sealed = true;
    return NL_CHECK_OK;
}

static const NLCopyBodyProof *proof(const NLCheckedFragment *caller,
                                    NLCheckedNodeId id)
{
    const NLCheckedFragment *f = nl_checked_call_body(caller, id);
    const NLCopyBodyProof *p = f ? f->copy_body : NULL;
    return p && p->sealed && p->owner == f && p->caller == caller &&
                   p->call == id
               ? p
               : NULL;
}
bool nl_checked_copy_body_view(const NLCheckedFragment *c, NLCheckedNodeId id,
                               NLCopyBodyView *out)
{
    const NLCopyBodyProof *p = proof(c, id);
    if (!p || !out)
        return false;
    *out = p->view;
    return true;
}
bool nl_checked_copy_body_parameters(const NLCheckedFragment *c,
                                     NLCheckedNodeId id,
                                     NLCopyBodyParameters *out)
{
    const NLCopyBodyProof *p = proof(c, id);
    if (!p || !p->formal_write || !out)
        return false;
    *out = p->parameters;
    return true;
}
static bool same_parameters(const NLCopyBodyParameters *a,
                            const NLCopyBodyParameters *b)
{
    return a->entry == b->entry && a->handoff == b->handoff &&
           a->loan == b->loan &&
           !memcmp(a->arguments, b->arguments, sizeof(a->arguments)) &&
           !memcmp(a->parameters, b->parameters, sizeof(a->parameters)) &&
           !memcmp(&a->field, &b->field, sizeof(a->field));
}
NLCheckStatus
nl_checked_copy_body_parameters_validate(const NLCheckedFragment *c,
                                         NLCheckedNodeId id,
                                         const NLCopyBodyParameters *v)
{
    const NLCopyBodyProof *p = proof(c, id);
    if (!p || !p->formal_write ||
        (v && !same_parameters(v, &p->issued_parameters)))
        return INVALID;
    return p->unit_call ? nl_checked_copy_unit_validate(c, id, NULL)
                        : nl_checked_copy_body_validate(c, id, NULL);
}
static const CopyEvent *event_at(const NLCopyBodyProof *p, size_t index)
{
    if (!p || index >= p->view.event_count)
        return NULL;
    const CopyEvent *e = p->first;
    for (size_t i = 0; e && i < index; ++i)
        e = e->next;
    return e;
}
bool nl_checked_copy_body_event(const NLCheckedFragment *c, NLCheckedNodeId id,
                                size_t index, NLCopyBodyEventView *out)
{
    const CopyEvent *e = event_at(proof(c, id), index);
    if (!e || !out)
        return false;
    *out = e->view;
    return true;
}
static bool same_event(const NLCopyBodyEventView *a,
                       const NLCopyBodyEventView *b)
{
    return a->kind == b->kind && a->node == b->node && a->before == b->before &&
           a->after == b->after && a->binding == b->binding &&
           a->result == b->result && a->incoming == b->incoming &&
           memcmp(&a->field, &b->field, sizeof(a->field)) == 0;
}
bool nl_copy_plain(const NLSemanticContext *c, NLValueId id, NLTypeId t)
{
    if (!id || id > c->value_count)
        return false;
    const NLSemanticValueView v = c->values[id - 1];
    return v.type == t && v.dependencies == NL_DEPENDENCY_FREE &&
           !v.value_dependency_count && !v.reference_count &&
           !v.reference.place && !v.reference.scope && !v.domain &&
           !v.slot_place && !v.allocation_region && !v.occupancy.region &&
           !v.field_count && !v.sum_payload && !v.sum_owner;
}
static bool same_places(const NLSemanticPlaceView a[3],
                        const NLSemanticPlaceView b[3])
{
    return memcmp(a, b, 3 * sizeof(*a)) == 0;
}
static bool same_values(const NLSemanticContext *c,
                        const NLSemanticPlaceView a[3],
                        const NLSemanticValueView issued[3])
{
    for (size_t i = 0; i < 3; ++i)
        if (memcmp(&c->values[a[i].current_value - 1], &issued[i],
                   sizeof(issued[i])))
            return false;
    return true;
}
static bool intrinsic(const NLSemanticValueView *a,
                      const NLSemanticValueView *b)
{
    return a->type == b->type && a->scalar_known == b->scalar_known &&
           a->scalar_value == b->scalar_value;
}
typedef struct {
    const NLCopyBodyProof *proof;
    const NLCheckedFragment *f;
    const CopyEvent *next;
    bool *seen;
    NLSemanticPlaceView *model;
    NLSymbolId *bound;
    size_t events, depth;
    NLTypeId u8;
    NLCheckStatus status;
} Replay;
static const NLCheckedNodeView *take(Replay *r, NLCheckedNodeId id)
{
    if (!id || id > r->f->count || r->seen[id])
        return NULL;
    r->seen[id] = true;
    return &r->f->nodes[id - 1];
}
static bool modeled(Replay *r, const NLSemanticPlaceView a[3], NLPlaceId root)
{
    return root && root <= r->proof->exit->place_count && r->model[root].live &&
           memcmp(&r->model[root], &a[0], sizeof(a[0])) == 0 &&
           memcmp(&r->model[a[0].fixed_fields[0]], &a[1], sizeof(a[1])) == 0 &&
           memcmp(&r->model[a[0].fixed_fields[1]], &a[2], sizeof(a[2])) == 0;
}
static void install(Replay *r, NLPlaceId root, const NLSemanticPlaceView a[3])
{
    r->model[root] = a[0];
    for (size_t i = 0; i < 2; ++i)
        r->model[a[0].fixed_fields[i]] = a[i + 1];
}
static const CopyEvent *next_event(Replay *r, NLCheckedNodeId id,
                                   NLCopyBodyEventKind kind)
{
    const CopyEvent *e = r->next;
    if (!e || e->view.node != id || e->view.kind != kind ||
        !same_event(&e->view, &e->issued) || e->view.after != e->after ||
        e->view.before != e->before ||
        e->after->scope_count != e->after_count ||
        (e->after_count && memcmp(e->after->scopes, e->scopes_after,
                                  e->after_count * sizeof(*e->scopes_after))) ||
        (e->before &&
         (e->before->scope_count != e->before_count ||
          (e->before_count &&
           memcmp(e->before->scopes, e->scopes_before,
                  e->before_count * sizeof(*e->scopes_before))))) ||
        nl_sem_validate(e->after) != NL_CHECK_OK)
        return NULL;
    NLSemanticPlaceView after[3];
    const NLPlaceId root = e->view.field.parent;
    if (!nl_copy_trio(e->after, root, after) || !same_places(after, e->post) ||
        !same_values(e->after, after, e->post_values))
        return NULL;
    if (e->view.result &&
        (e->view.result > e->after->value_count ||
         memcmp(&e->after->values[e->view.result - 1], &e->result,
                sizeof(e->result)) ||
         e->view.result > r->proof->exit->value_count ||
         !intrinsic(&e->result, &r->proof->exit->values[e->view.result - 1])))
        return NULL;
    if (kind != NL_COPY_BODY_INITIALIZE) {
        NLSemanticPlaceView before[3];
        if (kind == NL_COPY_BODY_READ) {
            if (!modeled(r, after, root))
                return NULL;
        } else if (!nl_copy_trio(e->before, root, before) ||
                   !same_places(before, e->pre) ||
                   !same_values(e->before, before, e->pre_values) ||
                   !modeled(r, before, root) || !e->view.incoming ||
                   e->view.incoming > e->before->value_count ||
                   memcmp(&e->before->values[e->view.incoming - 1],
                          &e->incoming, sizeof(e->incoming))) {
            return NULL;
        }
    }
    r->next = e->next;
    ++r->events;
    return e;
}
static bool field_at(const NLSemanticContext *c, const NLCheckedField *e)
{
    NLSemanticPlaceView a[3];
    if (!nl_copy_trio(c, e->parent, a) || !e->present ||
        !e->dependency_compatible || e->index >= 2 || e->base == 0 ||
        e->base > c->binding_count ||
        c->bindings[e->base - 1].view.place != e->parent)
        return false;
    const NLSemanticPlaceView child = a[e->index + 1];
    return e->nominal == a[0].type && e->type == child.type &&
           e->child == a[0].fixed_fields[e->index] &&
           e->parent_incarnation == a[0].incarnation &&
           e->child_incarnation == child.incarnation &&
           e->parent_fact == a[0].current_fact &&
           e->child_fact == child.current_fact &&
           e->old_value == child.current_value && !e->payload_occurrence &&
           !e->post_payload_occurrence;
}
static bool expression(Replay *, NLCheckedNodeId, const NLCheckedNodeView *);
static bool scalar(Replay *r, NLCheckedNodeId id)
{
    const NLCheckedNodeView *n = &r->f->nodes[id - 1];
    return n->type == r->u8 && n->result_count == 1 &&
           n->results[0].type == r->u8 &&
           nl_copy_plain(r->proof->exit, n->results[0].value, r->u8);
}
static bool expression(Replay *r, NLCheckedNodeId id,
                       const NLCheckedNodeView *loan)
{
    if (++r->depth > NL_SEMANTIC_MAX_DEPTH)
        return false;
    const NLCheckedNodeView *n = take(r, id);
    bool ok = false;
    if (!n)
        goto done;
    switch (n->kind) {
    case NL_CHECKED_U8_LITERAL:
        ok = scalar(r, id) && n->has_scalar_result && n->scalar_result.known &&
             n->scalar_result.value <= 255 &&
             r->proof->exit->values[n->results[0].value - 1].scalar_known &&
             n->scalar_result.value ==
                 r->proof->exit->values[n->results[0].value - 1].scalar_value;
        break;
    case NL_CHECKED_IDENTIFIER:
        if (r->proof->formal_write &&
            n->symbol == r->proof->parameters.parameters[1]) {
            const NLValueId v =
                r->proof->entry->bindings[n->symbol - 1].view.value;
            ok = scalar(r, id) && n->value_use == NL_VALUE_COPIED &&
                 intrinsic(&r->proof->exit->values[n->results[0].value - 1],
                           &r->proof->entry->values[v - 1]);
            break;
        }
        ok = scalar(r, id) && n->value_use == NL_VALUE_COPIED && n->symbol &&
             n->symbol <= r->proof->exit->binding_count && r->bound[n->symbol];
        if (ok) {
            const NLCheckedNodeView *b = &r->f->nodes[r->bound[n->symbol] - 1];
            const NLCheckedNodeView *v = &r->f->nodes[b->initializer - 1];
            ok = v->result_count == 1 &&
                 intrinsic(&r->proof->exit->values[n->results[0].value - 1],
                           &r->proof->exit->values[v->results[0].value - 1]);
        }
        break;
    case NL_CHECKED_FIELD_READ: {
        const CopyEvent *e = next_event(r, id, NL_COPY_BODY_READ);
        ok = scalar(r, id) && e && n->field.access == NL_ACCESS_READ &&
             field_at(e->after, &n->field) &&
             n->field.old_value != n->results[0].value &&
             e->view.result == n->results[0].value &&
             nl_copy_plain(e->after, e->view.result, r->u8) &&
             e->after->values[e->view.result - 1].scalar_value ==
                 e->after->values[n->field.old_value - 1].scalar_value &&
             memcmp(&n->field, &e->view.field, sizeof(n->field)) == 0;
        break;
    }
    case NL_CHECKED_BINDING: {
        if (!expression(r, n->initializer, loan))
            break;
        const NLCheckedNodeView *v = &r->f->nodes[n->initializer - 1];
        const NLCheckedNodeView *recv = take(r, n->first_argument);
        if (!recv || recv->kind != NL_CHECKED_RECEIVER || recv->next_argument ||
            n->argument_count != 1 || recv->symbol != n->symbol ||
            recv->type != v->type || !n->symbol ||
            n->symbol > r->proof->exit->binding_count || r->bound[n->symbol])
            break;
        r->bound[n->symbol] = id;
        if (v->kind == NL_CHECKED_AGGREGATE) {
            const CopyEvent *e = next_event(r, id, NL_COPY_BODY_INITIALIZE);
            if (!e || e->view.binding != n->symbol ||
                e->after->bindings[n->symbol - 1].view.value !=
                    v->results[0].value ||
                e->post[0].current_value != v->results[0].value ||
                e->post[0].type != v->type ||
                r->model[e->view.field.parent].live)
                break;
            for (NLCheckedNodeId a = v->first_argument; a;) {
                const NLCheckedNodeView *member = &r->f->nodes[a - 1];
                if (e->post[member->field_index + 1].current_value !=
                    r->f->nodes[member->initializer - 1].results[0].value)
                    goto done;
                a = member->next_argument;
            }
            install(r, e->view.field.parent, e->post);
        } else if (!scalar(r, n->initializer)) {
            break;
        }
        ok = true;
        break;
    }
    case NL_CHECKED_AGGREGATE: {
        if (!nominal(r->proof->exit, n->type) || n->argument_count != 2)
            break;
        bool indices[2] = {false, false};
        size_t count = 0;
        ok = true;
        for (NLCheckedNodeId a = n->first_argument; a;) {
            const NLCheckedNodeView *v = take(r, a);
            if (!v || v->kind != NL_CHECKED_AGGREGATE_FIELD ||
                v->field_index >= 2 || indices[v->field_index] ||
                !expression(r, v->initializer, loan) ||
                !scalar(r, v->initializer)) {
                ok = false;
                break;
            }
            indices[v->field_index] = true;
            ++count;
            a = v->next_argument;
        }
        ok = ok && count == 2;
        break;
    }
    case NL_CHECKED_STATEMENT:
        ok = expression(r, n->initializer, loan);
        break;
    case NL_CHECKED_BLOCK: {
        if (id != r->proof->view.root && (!loan || id != loan->initializer))
            break;
        size_t count = 0;
        ok = true;
        for (NLCheckedNodeId i = n->first_item; i;) {
            if (!expression(r, i, loan)) {
                ok = false;
                break;
            }
            ++count;
            i = r->f->nodes[i - 1].next_item;
        }
        ok = ok && count == n->item_count &&
             (!n->tail || expression(r, n->tail, loan));
        break;
    }
    case NL_CHECKED_LOAN_HEADER: {
        const NLCheckedNodeView *block =
            nl_checked_node_view(r->f, n->initializer);
        const NLCheckedNodeView *tail =
            block ? nl_checked_node_view(r->f, block->tail) : NULL;
        ok = n->field.present && n->loan.implicit_local &&
             n->loan.access == NL_ACCESS_WRITE && !n->loan.is_exclusive &&
             !n->loan.from_ptr && n->loan.body_nonescape_proved &&
             n->loan.normal_result_forwarded && n->loan.scope &&
             n->loan.scope <= r->proof->exit->scope_count &&
             !r->proof->exit->scopes[n->loan.scope - 1].active && block &&
             !block->terminates && tail && tail->kind == NL_CHECKED_REPLACE &&
             block->result_count == 1 && n->result_count == 1 &&
             n->results[0].value == block->results[0].value &&
             n->results[0].value == tail->results[0].value &&
             n->loan.ref_symbol && expression(r, n->initializer, n) &&
             scalar(r, id);
        break;
    }
    case NL_CHECKED_REPLACE: {
        const bool formal = r->proof->formal_write && !loan;
        if ((!loan && !formal) || n->argument_count != 2 || !scalar(r, id))
            break;
        const NLCheckedNodeView *a = take(r, n->first_argument);
        if (!a || a->kind != NL_CHECKED_IDENTIFIER ||
            a->symbol != (formal ? r->proof->parameters.parameters[0]
                                 : loan->loan.ref_symbol) ||
            a->value_use != NL_VALUE_COPIED || a->result_count != 1 ||
            !a->next_argument || !expression(r, a->next_argument, loan) ||
            !scalar(r, a->next_argument))
            break;
        const CopyEvent *e = next_event(r, id, NL_COPY_BODY_CHANGE);
        if (!e || !field_at(e->before, &n->field) ||
            (!formal && !field_at(e->before, &loan->field)) ||
            (formal ? r->proof->parameters.field.child : loan->loan.place) !=
                n->field.child ||
            (formal ? r->proof->parameters.field.child_incarnation
                    : loan->loan.incarnation) != n->field.child_incarnation ||
            !a->results[0].value ||
            a->results[0].value > e->before->value_count)
            break;
        const NLReferenceFacts ref =
            e->before->values[a->results[0].value - 1].reference;
        const NLScopeId scope =
            formal ? r->proof->loan_node.loan.scope : loan->loan.scope;
        if (!scope || scope > e->before->scope_count ||
            !e->before->scopes[scope - 1].active || !ref.writable ||
            ref.provenance != NL_PROVENANCE_VALID || !ref.readable ||
            ref.occurrence_dependency || ref.scope != scope ||
            ref.place != n->field.child ||
            ref.incarnation != n->field.child_incarnation ||
            e->view.result != n->field.old_value ||
            n->results[0].value != e->view.result ||
            e->view.incoming !=
                r->f->nodes[a->next_argument - 1].results[0].value ||
            n->field.parent_post_fact != e->post[0].current_fact ||
            n->field.child_post_fact !=
                e->post[n->field.index + 1].current_fact)
            break;
        if (formal) {
            const NLSemanticContext *entry = r->proof->entry;
            const NLValueId fv =
                entry->bindings[r->proof->parameters.parameters[0] - 1]
                    .view.value;
            const NLSemanticValueView v =
                e->before->values[a->results[0].value - 1];
            if (v.type != entry->values[fv - 1].type || v.reference_count ||
                v.dependencies != NL_DEPENDENCY_FREE ||
                v.value_dependency_count ||
                memcmp(&v.reference, &entry->values[fv - 1].reference,
                       sizeof(v.reference)))
                break;
        }
        NLSemanticContext *replay = NULL;
        r->status = nl_sem_clone(e->before, &replay);
        if (r->status == NL_CHECK_OK)
            r->status = nl_fixed_change(replay, n->field.child,
                                        e->view.incoming, false);
        NLSemanticPlaceView actual[3];
        ok = r->status == NL_CHECK_OK &&
             nl_copy_trio(replay, n->field.parent, actual) &&
             same_places(actual, e->post);
        nl_semantic_destroy(replay);
        if (ok)
            install(r, n->field.parent, e->post);
        break;
    }
    case NL_CHECKED_RETURN:
        ok = n->terminates && !n->next_item &&
             expression(r, n->initializer, loan) && scalar(r, n->initializer) &&
             n->returned.type == r->proof->view.result.type &&
             n->returned.value == r->proof->view.result.value &&
             n->returned.value ==
                 r->f->nodes[n->initializer - 1].results[0].value;
        break;
    default:
        break;
    }
done:
    --r->depth;
    return ok;
}
static bool same_body_view(const NLCopyBodyView *a, const NLCopyBodyView *b)
{
    return a->body == b->body && a->function == b->function &&
           a->event_count == b->event_count && a->root == b->root &&
           a->result.type == b->result.type &&
           a->result.value == b->result.value && a->live_exit == b->live_exit;
}
bool nl_copy_same_types(const NLSemanticContext *c,
                        const NLSemanticContext *reference)
{
    if (!c || !reference)
        return false;
    for (size_t i = 0;
         i < (reference->type_count < c->type_count ? reference->type_count
                                                    : c->type_count);
         ++i) {
        const NLTypeEntry *a = &c->types[i], *b = &reference->types[i];
        if (memcmp(&a->view, &b->view, sizeof(a->view)) ||
            memcmp(a->field_types, b->field_types, sizeof(a->field_types)) ||
            memcmp(a->variant_types, b->variant_types,
                   sizeof(a->variant_types)) ||
            a->recursive_header != b->recursive_header ||
            a->incomplete != b->incomplete ||
            a->view.field_count > NL_SEMANTIC_MAX_FIELDS ||
            a->view.variant_count > NL_SEMANTIC_MAX_VARIANTS)
            return false;
        for (size_t j = 0; j < a->view.field_count; ++j)
            if (!a->field_names[j] || !b->field_names[j] ||
                strcmp(a->field_names[j], b->field_names[j]))
                return false;
    }
    return true;
}
/* Validate actual->formal substitution against the issuance-time caller
 * world, not final ended locals, equal scalars, or names as authority. */
/* M5.1: a unit call may be the sole statement of the caller's scoped body.
 * parameter_entry first authenticates this entire source-issued caller graph.
 * Still reconstruct its exact effect/order boundary here: no tail, bindings,
 * second statement, shared call, or orphan operation inside this loan. The
 * wrapper discards no Copy/ref result and cannot terminate the caller. The
 * callee's RETURN target is checked separately by the unit/IF replay proof. */
static bool unit_statement_link(const NLCopyBodyProof *p,
                                const NLCheckedFragment *caller,
                                const NLCheckedNodeView *block,
                                const NLCheckedNodeView *call)
{
    if (!p->unit_call || block->tail || block->item_count != 1 ||
        block->next_item || !block->first_item || call->next_item ||
        call->next_argument)
        return false;
    const NLCheckedNodeView *statement =
        nl_checked_node_view(caller, block->first_item);
    if (!statement || statement->kind != NL_CHECKED_STATEMENT ||
        statement->initializer != p->call || statement->type != 1 ||
        statement->result_count || statement->terminates ||
        statement->next_item || statement->tail || statement->first_item ||
        statement->item_count || statement->first_argument ||
        statement->argument_count || statement->next_argument ||
        statement->has_scalar_result || statement->has_reference_result ||
        !(p->parameters.loan < p->call &&
          p->call < p->parameters.arguments[0] &&
          p->parameters.arguments[0] < p->parameters.arguments[1] &&
          p->parameters.arguments[1] < block->first_item) ||
        statement->span.start_byte != call->span.start_byte ||
        statement->span.end_byte <= call->span.end_byte ||
        block->span.start_byte >= statement->span.start_byte ||
        block->span.end_byte <= statement->span.end_byte)
        return false;
    NLSourceView suffix;
    if (!nl_source_view(
            caller->source,
            (NLSourceSpan){call->span.end_byte, statement->span.end_byte},
            &suffix) ||
        !suffix.length || suffix.bytes[suffix.length - 1] != ';')
        return false;
    const NLCheckedNodeView *loan =
        nl_checked_node_view(caller, p->parameters.loan);
    const NLCheckedNodeId allowed[] = {
        loan->initializer, p->call, p->parameters.arguments[0],
        p->parameters.arguments[1], block->first_item};
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedNodeView *arg =
            nl_checked_node_view(caller, p->parameters.arguments[a]);
        if (!arg || arg->span.start_byte <= call->span.start_byte ||
            arg->span.end_byte >= call->span.end_byte ||
            arg->span.end_byte <= arg->span.start_byte || arg->initializer ||
            arg->first_item || arg->tail || arg->item_count ||
            arg->first_argument || arg->argument_count ||
            (a == 0 ? arg->kind != NL_CHECKED_IDENTIFIER
                    : (arg->kind != NL_CHECKED_U8_LITERAL &&
                       arg->kind != NL_CHECKED_IDENTIFIER &&
                       arg->kind != NL_CHECKED_FIELD_READ)))
            return false;
    }
    if (p->arguments[0].span.end_byte > p->arguments[1].span.start_byte)
        return false;
    for (size_t i = 0; i < caller->count; ++i) {
        const NLCheckedNodeView *n = &caller->nodes[i];
        const NLCheckedNodeId id = i + 1;
        if (n->span.start_byte >= block->span.start_byte &&
            n->span.end_byte <= block->span.end_byte) {
            bool found = false;
            for (size_t j = 0; j < sizeof(allowed) / sizeof(*allowed); ++j)
                found = found || id == allowed[j];
            if (!found)
                return false;
        }
        /* A call is executed once through this wrapper, never also through
         * a tail/argument/second item in the same source graph. */
        if (n->tail == p->call || n->first_item == p->call ||
            n->next_item == p->call || n->first_argument == p->call ||
            n->next_argument == p->call ||
            (n->initializer == p->call && id != block->first_item))
            return false;
    }
    return true;
}

static bool parameter_entry(const NLCopyBodyProof *p,
                            const NLCheckedFragment *caller,
                            const NLCheckedNodeView *call)
{
    if (!p->caller_nodes || caller->source != p->caller_source ||
        caller->body_owner != p->caller_body ||
        caller->nodes != p->caller_nodes_origin ||
        caller->count != p->caller_count || caller->root != p->caller_root ||
        memcmp(caller->nodes, p->caller_nodes,
               p->caller_count * sizeof(*caller->nodes)) ||
        !same_parameters(&p->parameters, &p->issued_parameters) ||
        p->parameters.entry != p->entry ||
        p->parameters.handoff != p->handoff ||
        !nl_copy_state_matches(p->entry, &p->entry_anchor) ||
        !nl_copy_state_matches(p->handoff, &p->handoff_anchor) ||
        nl_sem_validate(p->entry) != NL_CHECK_OK ||
        nl_sem_validate(p->handoff) != NL_CHECK_OK ||
        memcmp(call, &p->call_node, sizeof(*call)) ||
        !field_at(p->entry, &p->parameters.field))
        return false;
    const NLSemanticContext *entry = p->entry;
    const NLFunctionEntry *fn = &entry->functions[call->function - 1];
    if (fn->count != 2 || !fn->parameters || !fn->body ||
        fn->body->count != 2 || fn->caller_effects ||
        memcmp(fn->parameters, p->parameter_types,
               sizeof(p->parameter_types)) ||
        fn->parameters[0] == 0 || fn->parameters[0] > entry->type_count ||
        call->argument_count != 2 ||
        call->first_argument != p->parameters.arguments[0] ||
        fn->parameters[1] != nl_semantic_core_type(entry, NL_TYPE_U8))
        return false;
    const NLSemanticContext *current = nl_checked_context(caller);
    const NLFunctionEntry *now = &current->functions[call->function - 1];
    if (now->count != 2 || !now->parameters ||
        memcmp(now->parameters, p->parameter_types, sizeof(p->parameter_types)))
        return false;
    const NLSemanticTypeView t = entry->types[fn->parameters[0] - 1].view;
    if (t.kind != NL_TYPE_REF || t.target != fn->parameters[1] ||
        t.access != NL_ACCESS_WRITE || t.is_exclusive)
        return false;
    const NLCheckedNodeView *loan =
        nl_checked_node_view(caller, p->parameters.loan);
    if (!loan || loan->kind != NL_CHECKED_LOAN_HEADER ||
        memcmp(&loan->field, &p->parameters.field, sizeof(loan->field)) ||
        loan->loan.source != p->loan_node.loan.source ||
        loan->loan.ref_symbol != p->loan_node.loan.ref_symbol ||
        loan->loan.place != p->parameters.field.child ||
        loan->loan.incarnation != p->parameters.field.child_incarnation ||
        loan->loan.scope != p->loan_node.loan.scope || !loan->loan.scope ||
        loan->loan.scope > entry->scope_count ||
        !entry->scopes[loan->loan.scope - 1].active ||
        !loan->loan.implicit_local || loan->loan.from_ptr ||
        loan->loan.is_exclusive || loan->loan.access != NL_ACCESS_WRITE ||
        !loan->loan.body_nonescape_proved ||
        !loan->loan.normal_result_forwarded)
        return false;
    const NLCheckedNodeView *block =
        nl_checked_node_view(caller, loan->initializer);
    if (p->unit_call) {
        size_t uses = 0;
        for (size_t i = 0; i < caller->count; ++i) {
            const NLCheckedNodeView *n = &caller->nodes[i];
            uses += (n->tail == p->call) + (n->initializer == p->call) +
                    (n->first_item == p->call) + (n->next_item == p->call) +
                    (n->first_argument == p->call) +
                    (n->next_argument == p->call);
        }
        /* In either spelling, the genuine call has exactly one executing
         * parent. A wrapper plus a second tail would execute it twice. */
        if (uses != 1)
            return false;
    }
    /* Keep the original direct-tail contract. The separate unit-statement
     * route proves its wrapper and complete scoped effect boundary. */
    if (!block || block->kind != NL_CHECKED_BLOCK ||
        (block->tail != p->call &&
         !unit_statement_link(p, caller, block, call)) ||
        block->terminates || block->result_count != (p->unit_call ? 0U : 1U) ||
        loan->result_count != block->result_count ||
        (p->unit_call ? (block->type != 1 || loan->type != 1 ||
                         call->type != 1 || call->result_count)
                      : (block->results[0].value != call->results[0].value ||
                         loan->results[0].value != call->results[0].value)))
        return false;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *arg =
            nl_checked_node_view(caller, p->parameters.arguments[i]);
        const NLSymbolId symbol = p->parameters.parameters[i];
        if (!arg || memcmp(arg, &p->arguments[i], sizeof(*arg)) ||
            arg->result_count != 1 ||
            arg->parameter_type != fn->parameters[i] || !symbol ||
            symbol > entry->binding_count ||
            entry->bindings[symbol - 1].view.type != fn->parameters[i] ||
            entry->bindings[symbol - 1].view.value != arg->results[0].value ||
            entry->bindings[symbol - 1].view.availability != NL_AVAILABLE ||
            symbol > p->handoff->binding_count ||
            p->handoff->bindings[symbol - 1].view.availability != NL_CONSUMED ||
            !p->handoff->bindings[symbol - 1].hidden ||
            strcmp(entry->bindings[symbol - 1].name,
                   fn->body->parameter_names[i]) ||
            arg->results[0].value > entry->value_count)
            return false;
    }
    if (p->parameters.parameters[0] == p->parameters.parameters[1] ||
        p->arguments[0].kind != NL_CHECKED_IDENTIFIER ||
        p->arguments[0].symbol != loan->loan.ref_symbol ||
        p->arguments[0].value_use != NL_VALUE_COPIED ||
        p->arguments[0].next_argument != p->parameters.arguments[1] ||
        p->arguments[1].next_argument)
        return false;
    for (size_t i = 0; i < 2; ++i) {
        NLValueId v = p->arguments[i].results[0].value;
        if (!v || v > current->value_count ||
            !intrinsic(&entry->values[v - 1], &current->values[v - 1]))
            return false;
    }
    const NLSemanticValueView v =
        entry->values[p->arguments[0].results[0].value - 1];
    return v.type == fn->parameters[0] && !v.reference_count &&
           v.dependencies == NL_DEPENDENCY_FREE && !v.value_dependency_count &&
           !v.domain && !v.allocation_region && !v.slot_place &&
           !v.occupancy.region && v.reference.place == loan->loan.place &&
           v.reference.incarnation == loan->loan.incarnation &&
           v.reference.scope == loan->loan.scope && v.reference.readable &&
           v.reference.writable &&
           v.reference.provenance == NL_PROVENANCE_VALID &&
           !v.reference.occurrence_dependency &&
           nl_copy_plain(entry, p->arguments[1].results[0].value,
                         fn->parameters[1]);
}
NLCheckStatus nl_checked_copy_body_validate(const NLCheckedFragment *caller,
                                            NLCheckedNodeId id,
                                            const NLCopyBodyView *candidate)
{
    const NLCopyBodyProof *p = proof(caller, id);
    if (!p || !caller->nodes ||
        (candidate && !same_body_view(candidate, &p->view)))
        return INVALID;
    const NLCheckedFragment *f = p->owner;
    const NLSemanticContext *c = nl_checked_context(caller);
    const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
    if (!c || !call || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || call->terminates ||
        (!p->formal_write && (call->argument_count || call->first_argument)) ||
        call->result_count != 1 || call->function != p->view.function ||
        !call->function || call->function > c->function_count ||
        nl_checked_context(f) != c || f->body_owner != p->body ||
        f->source != p->source || f->source != p->body->source ||
        c->functions[call->function - 1].body != p->body ||
        c->functions[call->function - 1].kind != NL_CHECKED_REGISTERED_CALL ||
        c->functions[call->function - 1].caller_effects ||
        c->functions[call->function - 1].hidden_dependencies ||
        c->functions[call->function - 1].owner_receiver ||
        c->functions[call->function - 1].owner_producer ||
        c->functions[call->function - 1].custody_recipient ||
        (!p->formal_write &&
         (c->functions[call->function - 1].count || p->body->count)) ||
        call->type != nl_semantic_core_type(c, NL_TYPE_U8) ||
        c->functions[call->function - 1].result != call->type ||
        call->results[0].type != p->view.result.type ||
        call->results[0].value != p->view.result.value ||
        f->root != p->view.root || f->count != p->count ||
        f->nodes != p->nodes_origin || !p->nodes ||
        p->count > NL_SEMANTIC_MAX_ENTRIES ||
        memcmp(f->nodes, p->nodes, p->count * sizeof(*p->nodes)) ||
        f->arm_count || f->body_count || p->pending ||
        p->exit != p->view.live_exit || !p->exit ||
        p->exit->place_count > NL_SEMANTIC_MAX_ENTRIES ||
        p->exit->binding_count > NL_SEMANTIC_MAX_ENTRIES ||
        p->exit->scope_count != p->scope_count ||
        (p->scope_count && memcmp(p->exit->scopes, p->exit_scopes,
                                  p->scope_count * sizeof(*p->exit_scopes))) ||
        c->type_count < p->exit->type_count ||
        !nl_copy_same_types(c, p->exit) || nl_sem_validate(c) != NL_CHECK_OK ||
        nl_sem_validate(p->exit) != NL_CHECK_OK)
        return INVALID;
    if (p->formal_write && (!nl_copy_same_types(p->entry, p->exit) ||
                            !nl_copy_same_types(p->handoff, p->exit) ||
                            !parameter_entry(p, caller, call)))
        return INVALID;
    Replay r = {.proof = p,
                .f = f,
                .next = p->first,
                .u8 = nl_semantic_core_type(c, NL_TYPE_U8)};
    r.seen = calloc(f->count + 1, sizeof(*r.seen));
    r.model = calloc(p->exit->place_count + 1, sizeof(*r.model));
    r.bound = calloc(p->exit->binding_count + 1, sizeof(*r.bound));
    NLCheckStatus status = NL_CHECK_OUT_OF_MEMORY;
    if (!r.seen || !r.model || !r.bound)
        goto done;
    status = INVALID;
    if (p->formal_write) {
        NLSemanticPlaceView initial[3];
        if (!nl_copy_trio(p->entry, p->parameters.field.parent, initial))
            goto done;
        install(&r, p->parameters.field.parent, initial);
    }
    if (!expression(&r, f->root, NULL) || r.next ||
        r.events != p->view.event_count)
        goto done;
    for (size_t i = 1; i <= f->count; ++i)
        if (!r.seen[i])
            goto done;
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    if (!root || root->kind != NL_CHECKED_BLOCK)
        goto done;
    if (!root->terminates) {
        const NLCheckedNodeView *tail = nl_checked_node_view(f, root->tail);
        if (!tail || tail->result_count != 1 ||
            tail->results[0].value != p->view.result.value ||
            root->result_count != 1 ||
            root->results[0].value != p->view.result.value)
            goto done;
    }
    bool changed = false;
    for (const CopyEvent *e = p->first; e; e = e->next) {
        if (!nl_copy_same_types(e->after, p->exit))
            goto done;
        changed |= e->view.kind == NL_COPY_BODY_CHANGE;
        if (e->view.kind != NL_COPY_BODY_INITIALIZE)
            continue;
        NLSemanticPlaceView exit[3];
        if (!nl_copy_trio(p->exit, e->view.field.parent, exit) ||
            !modeled(&r, exit, e->view.field.parent))
            goto done;
    }
    if (!changed)
        goto done;
    for (size_t i = 0; i < p->exit->scope_count; ++i)
        if (p->exit->scopes[i].active &&
            (!p->formal_write || i >= p->entry->scope_count ||
             !p->entry->scopes[i].active))
            goto done;
    if (p->formal_write) {
        NLSemanticPlaceView end[3];
        const NLPlaceId original = p->parameters.field.parent;
        if (!nl_copy_trio(p->exit, original, end) ||
            !modeled(&r, end, original) ||
            !nl_copy_trio(p->handoff, original, end) ||
            !modeled(&r, end, original))
            goto done;
        /* Every caller place retains its identity/state except the traced
         * root+child. Formal storage is callee-owned, not a caller change. */
        const NLPlaceId child = p->parameters.field.child;
        for (size_t i = 0; i < p->entry->place_count; ++i) {
            bool formal = false;
            for (size_t j = 0; j < 2; ++j)
                formal |= p->entry->bindings[p->parameters.parameters[j] - 1]
                              .view.place == i + 1;
            if (formal || i + 1 == original || i + 1 == child)
                continue;
            if (i >= p->handoff->place_count ||
                memcmp(&p->entry->places[i], &p->handoff->places[i],
                       sizeof(end[0])))
                goto done;
        }
        for (size_t i = 0; i < p->handoff->scope_count; ++i)
            if (p->handoff->scopes[i].active !=
                (i < p->entry->scope_count && p->entry->scopes[i].active))
                goto done;
    }
    /* Scalars keep identity/type/value after ordinary scope teardown. A
     * later caller operation cannot rewrite the issued return package. */
    for (size_t i = 0; i < f->count; ++i) {
        const NLCheckedNodeView *n = &f->nodes[i];
        if (n->type != r.u8 || n->result_count != 1)
            continue;
        const NLValueId v = n->results[0].value;
        if (!nl_copy_plain(p->exit, v, r.u8) || !nl_copy_plain(c, v, r.u8) ||
            p->exit->values[v - 1].scalar_known !=
                c->values[v - 1].scalar_known ||
            p->exit->values[v - 1].scalar_value !=
                c->values[v - 1].scalar_value)
            goto done;
    }
    status = NL_CHECK_OK;
done:
    if (r.status != NL_CHECK_OK)
        status = r.status;
    free(r.seen);
    free(r.model);
    free(r.bound);
    return status;
}
NLCheckStatus
nl_checked_copy_body_event_validate(const NLCheckedFragment *caller,
                                    NLCheckedNodeId call, size_t index,
                                    const NLCopyBodyEventView *candidate)
{
    const CopyEvent *e = event_at(proof(caller, call), index);
    if (!e || (candidate && !same_event(candidate, &e->issued)))
        return INVALID;
    return nl_checked_copy_body_validate(caller, call, NULL);
}

/* Replay the selected Copy/ref local teardown. Prefix caller places/scopes
 * are retained; ending a callee must not end the caller's scoped write loan. */
NLCheckStatus nl_copy_end_locals(NLSemanticContext *c, size_t bindings,
                                 size_t places, size_t scopes_floor)
{
    if (!c || bindings > c->binding_count || places > c->place_count ||
        scopes_floor > c->scope_count || c->domain_count || c->region_count ||
        c->occurrence_count)
        return INVALID;
    for (size_t i = bindings; i < c->binding_count; ++i) {
        NLBindingEntry *b = &c->bindings[i];
        if (b->hidden)
            continue;
        if (b->view.availability == NL_AVAILABLE) {
            if (!b->view.type || b->view.type > c->type_count ||
                !c->types[b->view.type - 1].view.is_copy ||
                !c->types[b->view.type - 1].view.is_discardable)
                return INVALID;
            NLCheckStatus s =
                nl_fixed_end_dependencies(c, b->view.place, bindings);
            if (s != NL_CHECK_OK)
                return s;
            nl_fixed_detach(c, b->view.place);
            nl_sem_end_value(c, b->view.value);
            NLSemanticPlaceView *place = &c->places[b->view.place - 1];
            place->live = false;
            place->current_value = place->current_fact =
                place->governing_domain = 0;
            b->view.availability = NL_CONSUMED;
        }
        b->hidden = true;
    }
    return nl_sem_function_exit(c, scopes_floor, places);
}
NLCheckStatus nl_copy_unit_entry_validate(const NLCheckedFragment *f)
{
    const NLCopyBodyProof *p = f ? f->copy_body : NULL;
    const NLCheckedNodeView *call =
        p ? nl_checked_node_view(p->caller, p->call) : NULL;
    if (!p || !p->unit_call || !p->formal_write || !p->sealed ||
        p->owner != f || proof(p->caller, p->call) != p || !call ||
        !parameter_entry(p, p->caller, call))
        return INVALID;
    return NL_CHECK_OK;
}
bool nl_checked_copy_unit_view(const NLCheckedFragment *caller,
                               NLCheckedNodeId id, NLCopyUnitView *out)
{
    const NLCopyBodyProof *p = proof(caller, id);
    if (!p || !p->unit_call || !out || !p->normal_end)
        return false;
    *out = (NLCopyUnitView){.parameters = p->parameters,
                            .body = p->owner,
                            .conditional = p->conditional,
                            .normal_end = p->normal_end,
                            .postcall = p->handoff,
                            .function = p->view.function};
    return true;
}
/* Rewalk the complete caller-independent callee prefix. Each value is read
 * in the actual IF-entry world; equal IDs in its forks are irrelevant. The
 * only effectful node here is the separately replay-validated IF. */
typedef struct {
    const NLCheckedFragment *f;
    const NLSemanticContext *world;
    NLCheckedNodeId conditional;
    bool *seen;
    NLValueId *bound;
    size_t depth;
} UnitPrefix;
static bool unit_prefix_walk(UnitPrefix *r, NLCheckedNodeId id)
{
    if (!id || id > r->f->count || r->seen[id] ||
        ++r->depth > NL_SEMANTIC_MAX_DEPTH) {
        if (id && id <= r->f->count && !r->seen[id])
            --r->depth;
        return false;
    }
    r->seen[id] = true;
    const NLCheckedNodeView *n = &r->f->nodes[id - 1];
    const NLSemanticContext *c = r->world;
    bool ok = false;
    if (id == r->conditional) {
        ok = unit_prefix_walk(r, n->initializer);
        /* Common replay subtree has already passed its own full graph walk. */
        for (size_t i = 0; ok && i < r->f->count; ++i) {
            const NLCheckedNodeView *a = &r->f->nodes[i];
            if (i + 1 != id && a->span.start_byte >= n->span.start_byte &&
                a->span.end_byte <= n->span.end_byte)
                r->seen[i + 1] = true;
        }
    } else if (n->kind == NL_CHECKED_BLOCK && id == r->f->root) {
        size_t count = 0;
        ok = !n->terminates && n->type == 1 && !n->result_count;
        for (NLCheckedNodeId item = n->first_item; ok && item;) {
            ok = unit_prefix_walk(r, item);
            ++count;
            if (!ok)
                break;
            item = r->f->nodes[item - 1].next_item;
        }
        ok = ok && count == n->item_count && unit_prefix_walk(r, n->tail);
    } else if (n->kind == NL_CHECKED_STATEMENT) {
        ok = n->initializer == r->conditional &&
             unit_prefix_walk(r, n->initializer);
    } else if (n->kind == NL_CHECKED_UNIT) {
        ok = n->type == 1 && !n->result_count;
    } else if (n->kind == NL_CHECKED_BINDING) {
        if (n->symbol && n->symbol <= c->binding_count &&
            !r->bound[n->symbol] && n->argument_count == 1 &&
            unit_prefix_walk(r, n->initializer)) {
            const NLCheckedNodeView *value =
                nl_checked_node_view(r->f, n->initializer);
            const NLCheckedNodeView *recv =
                nl_checked_node_view(r->f, n->first_argument);
            ok = recv && !r->seen[n->first_argument] &&
                 recv->kind == NL_CHECKED_RECEIVER &&
                 recv->symbol == n->symbol && !recv->next_argument &&
                 recv->type == value->type && value->result_count == 1 &&
                 value->type == nl_semantic_core_type(c, NL_TYPE_USIZE) &&
                 c->bindings[n->symbol - 1].view.value ==
                     value->results[0].value;
            if (ok) {
                r->seen[n->first_argument] = true;
                r->bound[n->symbol] = value->results[0].value;
            }
        }
    } else if (n->kind == NL_CHECKED_USIZE_LITERAL ||
               n->kind == NL_CHECKED_IDENTIFIER ||
               n->kind == NL_CHECKED_USIZE_LESS_THAN) {
        NLTypeId type = nl_semantic_core_type(
            c, n->kind == NL_CHECKED_USIZE_LESS_THAN ? NL_TYPE_BOOL
                                                     : NL_TYPE_USIZE);
        if (n->type == type && n->result_count == 1 &&
            n->results[0].type == type &&
            nl_copy_plain(c, n->results[0].value, type)) {
            NLSemanticValueView value = c->values[n->results[0].value - 1];
            if (n->kind == NL_CHECKED_USIZE_LITERAL) {
                ok = n->has_scalar_result && n->scalar_result.known &&
                     n->scalar_result.type == type && value.scalar_known &&
                     value.scalar_value == n->scalar_result.value;
            } else if (n->kind == NL_CHECKED_IDENTIFIER) {
                NLValueId origin = n->symbol && n->symbol <= c->binding_count
                                       ? r->bound[n->symbol]
                                       : 0;
                ok = origin && n->value_use == NL_VALUE_COPIED &&
                     intrinsic(&value, &c->values[origin - 1]);
            } else {
                const NLCheckedNodeView *a =
                    nl_checked_node_view(r->f, n->first_argument);
                const NLCheckedNodeView *b =
                    a ? nl_checked_node_view(r->f, a->next_argument) : NULL;
                ok = n->argument_count == 2 && a && b && !b->next_argument &&
                     a->type == nl_semantic_core_type(c, NL_TYPE_USIZE) &&
                     b->type == a->type &&
                     unit_prefix_walk(r, n->first_argument) &&
                     unit_prefix_walk(r, a->next_argument);
                if (ok) {
                    NLSemanticValueView left =
                                            c->values[a->results[0].value - 1],
                                        right =
                                            c->values[b->results[0].value - 1];
                    ok = left.scalar_known && right.scalar_known &&
                         value.scalar_known &&
                         value.scalar_value ==
                             (size_t)(left.scalar_value < right.scalar_value);
                }
            }
        }
    }
    --r->depth;
    return ok;
}
static NLCheckStatus unit_prefix_validate(const NLCopyBodyProof *p,
                                          const NLIfReturnView *returned)
{
    UnitPrefix r = {.f = p->owner,
                    .world = returned->field.entry,
                    .conditional = p->conditional};
    r.seen = calloc(r.f->count + 1, sizeof(*r.seen));
    r.bound = calloc(r.world->binding_count + 1, sizeof(*r.bound));
    NLCheckStatus s = NL_CHECK_OUT_OF_MEMORY;
    if (r.seen && r.bound) {
        s = unit_prefix_walk(&r, r.f->root) ? NL_CHECK_OK : INVALID;
        for (size_t i = 1; s == NL_CHECK_OK && i <= r.f->count; ++i)
            if (!r.seen[i])
                s = INVALID;
    }
    free(r.seen);
    free(r.bound);
    return s;
}
NLCheckStatus nl_checked_copy_unit_validate(const NLCheckedFragment *caller,
                                            NLCheckedNodeId id,
                                            const NLCopyUnitView *candidate)
{
    NLCopyUnitView v;
    if (!nl_checked_copy_unit_view(caller, id, &v) ||
        (candidate &&
         (candidate->body != v.body ||
          candidate->conditional != v.conditional ||
          candidate->normal_end != v.normal_end ||
          candidate->postcall != v.postcall ||
          candidate->function != v.function ||
          !same_parameters(&candidate->parameters, &v.parameters))))
        return INVALID;
    const NLCopyBodyProof *p = proof(caller, id);
    const NLCheckedFragment *f = p->owner;
    const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
    if (!call || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || call->terminates || call->type != 1 ||
        call->result_count || call->function != v.function || !v.function ||
        v.function > p->entry->function_count ||
        p->entry->functions[v.function - 1].result != 1 ||
        p->entry->functions[v.function - 1].body != p->body ||
        f->source != p->source || f->body_owner != p->body ||
        p->source != p->body->source || f->count != p->count ||
        f->nodes != p->nodes_origin || !p->nodes ||
        memcmp(f->nodes, p->nodes, p->count * sizeof(*p->nodes)) ||
        f->root != p->view.root || f->body_count || f->arm_count != 2 ||
        !nl_copy_state_matches(p->normal_end, &p->normal_end_anchor) ||
        nl_copy_unit_entry_validate(f) != NL_CHECK_OK)
        return INVALID;
    NLIfReturnView returned;
    if (!nl_checked_if_return_view(f, p->conditional, &returned) ||
        returned.field.root != v.parameters.field.parent ||
        returned.field.child != v.parameters.field.child ||
        returned.field.index != v.parameters.field.index)
        return INVALID;
    const NLCheckedNodeView *loan =
        nl_checked_node_view(caller, v.parameters.loan);
    if (!loan || !loan->loan.scope ||
        loan->loan.scope > caller->context->scope_count ||
        caller->context->scopes[loan->loan.scope - 1].active)
        return INVALID;
    /* Caller-visible readback uses the conservative postcall facts, not
     * either arm's numeric IDs. Stop at the next actual mutation. */
    size_t stop = SIZE_MAX;
    for (size_t i = 0; i < caller->count; ++i) {
        const NLCheckedNodeView *n = &caller->nodes[i];
        if (n->span.start_byte >= loan->span.end_byte &&
            (n->kind == NL_CHECKED_REPLACE ||
             n->kind == NL_CHECKED_REGISTERED_CALL ||
             n->kind == NL_CHECKED_IF) &&
            n->span.start_byte < stop)
            stop = n->span.start_byte;
    }
    for (size_t i = 0; i < caller->count; ++i) {
        const NLCheckedNodeView *n = &caller->nodes[i];
        if (n->kind != NL_CHECKED_FIELD_READ ||
            n->field.parent != v.parameters.field.parent ||
            n->span.start_byte < loan->span.end_byte ||
            n->span.start_byte >= stop)
            continue;
        if (!field_at(p->handoff, &n->field) || n->result_count != 1 ||
            !nl_copy_plain(caller->context, n->results[0].value, n->type) ||
            !intrinsic(&caller->context->values[n->results[0].value - 1],
                       &p->handoff->values[n->field.old_value - 1]))
            return INVALID;
    }
    NLCheckStatus s =
        nl_checked_if_return_validate(f, p->conditional, &returned);
    if (s != NL_CHECK_OK)
        return s;
    s = unit_prefix_validate(p, &returned);
    if (s != NL_CHECK_OK || !p->exit || p->pending)
        return s == NL_CHECK_OK ? INVALID : s;
    NLSemanticPlaceView a[3], b[3];
    if (!nl_copy_trio(p->exit, v.parameters.field.parent, a) ||
        !nl_copy_trio(returned.field.post, v.parameters.field.parent, b) ||
        memcmp(a, b, sizeof(a)))
        return INVALID;
    NLSemanticContext *replay = NULL;
    s = nl_sem_clone(p->exit, &replay);
    if (s == NL_CHECK_OK)
        s = nl_copy_end_locals(replay, p->binding_floor, p->place_floor,
                               p->scope_floor);
    if (s == NL_CHECK_OK &&
        !nl_copy_state_matches(replay, &p->normal_end_anchor))
        s = INVALID;
    if (s == NL_CHECK_OK) {
        NLValueId value = 0;
        s = nl_fixed_change_dependencies(replay, v.parameters.field.child, 0);
        if (s == NL_CHECK_OK)
            s = nl_sem_new_value(
                replay,
                (NLSemanticValueView){
                    .type = nl_semantic_core_type(replay, NL_TYPE_U8)},
                &value);
        if (s == NL_CHECK_OK)
            s = nl_fixed_change(replay, v.parameters.field.child, value, false);
        for (size_t j = 0; s == NL_CHECK_OK && j < replay->binding_count; ++j)
            if (replay->bindings[j].view.place == v.parameters.field.parent &&
                replay->bindings[j].view.availability == NL_AVAILABLE)
                replay->bindings[j].view.value =
                    replay->places[v.parameters.field.parent - 1].current_value;
        if (s == NL_CHECK_OK &&
            !nl_copy_state_matches(replay, &p->handoff_anchor))
            s = INVALID;
    }
    nl_semantic_destroy(replay);
    return s;
}
