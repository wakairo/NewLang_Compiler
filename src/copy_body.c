#include "newlang/copy_body.h"
#include "newlang/parser.h"
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
               n->kind == NL_CHECKED_USIZE_LESS_THAN ||
               n->kind == NL_CHECKED_USIZE_EQUALITY) {
        const bool comparison = n->kind == NL_CHECKED_USIZE_LESS_THAN ||
                                n->kind == NL_CHECKED_USIZE_EQUALITY;
        NLTypeId type =
            nl_semantic_core_type(c, comparison ? NL_TYPE_BOOL : NL_TYPE_USIZE);
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
                             (size_t)(n->kind == NL_CHECKED_USIZE_EQUALITY
                                          ? left.scalar_value ==
                                                right.scalar_value
                                          : left.scalar_value <
                                                right.scalar_value);
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

/* P896 Stage0: optional NON-PUBLISHING definition-call observations. No public
 * receipt is attached, and no diagnostic status controls source admission. */
struct NLDefinitionCallCapture {
    NLDefinitionCallView view;
    NLSemanticContext *pre, *post;
    NLCheckStatus status;
    NLDefinitionCallObserver observer;
    void *user;
    struct NLDefinitionCallCapture *next;
};
static _Thread_local NLDefinitionCallObserver definition_observer;
static _Thread_local void *definition_user;
void nl_definition_call_observer(NLDefinitionCallObserver observer, void *user)
{
    definition_observer = observer;
    definition_user = user;
}
static bool dc_describe(const NLCheckedFragment *caller, NLCheckedNodeId call,
                        const NLCheckedNodeId args[2], size_t caller_function,
                        const NLSemanticContext *c,
                        NLDefinitionCallView *description)
{
    if (!caller || !caller->if_parent)
        return false;
    /* Stage0 observes lexical T only, never allocated/H or sum ownership. */
    if (c->domain_count || c->region_count || c->occurrence_count)
        return false;
    for (size_t i = 0; i < c->type_count; ++i)
        if (c->types[i].recursive_header)
            return false;
    const NLCheckedNodeView *n = nl_checked_node_view(caller, call);
    if (!n || n->kind != NL_CHECKED_REGISTERED_CALL || !n->function ||
        n->function > c->function_count || !caller_function ||
        caller_function > c->function_count)
        return false;
    const NLFunctionEntry *fn = &c->functions[n->function - 1];
    if (!fn->body || fn->count != 2 || !fn->parameters ||
        fn->parameters[1] != nl_semantic_core_type(c, NL_TYPE_U8) ||
        fn->result != fn->parameters[1] || fn->caller_effects ||
        fn->hidden_dependencies || fn->owner_producer || fn->owner_receiver ||
        fn->custody_recipient || n->function == caller_function)
        return false;
    const NLSemanticTypeView t = c->types[fn->parameters[0] - 1].view;
    const NLCheckedNodeView *a = nl_checked_node_view(caller, args[0]);
    const NLCheckedNodeView *b = nl_checked_node_view(caller, args[1]);
    if (t.kind != NL_TYPE_REF || t.is_exclusive ||
        t.access != NL_ACCESS_WRITE || t.target != fn->result || !a || !b ||
        a->result_count != 1 || b->result_count != 1 ||
        a->type != fn->parameters[0] || b->type != fn->parameters[1] ||
        !a->results[0].value || a->results[0].value > c->value_count)
        return false;
    const NLReferenceFacts r = c->values[a->results[0].value - 1].reference;
    if (!r.place || r.place > c->place_count || !r.scope ||
        r.scope > c->scope_count || !c->scopes[r.scope - 1].active)
        return false;
    const NLPlaceId root = c->places[r.place - 1].parent_aggregate;
    NLSemanticPlaceView parts[3];
    if (!root || !nl_copy_trio(c, root, parts))
        return false;
    NLCheckedNodeId loan = 0;
    for (size_t i = 1; i <= caller->count; ++i) {
        const NLCheckedNodeView *l = nl_checked_node_view(caller, i);
        if (l->kind == NL_CHECKED_LOAN_HEADER && l->field.present &&
            l->loan.ref_symbol == a->symbol && l->loan.scope == r.scope) {
            if (loan)
                return false;
            loan = i;
        }
    }
    if (!loan)
        return false;
    NLPlaceId other = 0;
    size_t roots = 0;
    for (size_t i = 1; i <= c->place_count; ++i) {
        NLSemanticPlaceView trio[3];
        if (nl_copy_trio(c, i, trio)) {
            ++roots;
            if (i != root && trio[0].type == parts[0].type)
                other = i;
        }
    }
    if (roots != 2 || !other)
        return false;
    const NLCheckedField field = nl_checked_node_view(caller, loan)->field;
    *description = (NLDefinitionCallView){
        .caller = caller,
        .parent = caller->if_parent,
        .world = c,
        .body = fn->body,
        .caller_function = caller_function,
        .function = n->function,
        .arm = caller->if_arm_index,
        .call = call,
        .arguments = {args[0], args[1]},
        .loan = loan,
        .field = field,
        .sibling = parts[0].fixed_fields[1 - field.index],
        .other = other,
        .scope = r.scope};
    return true;
}
NLDefinitionCallCapture *
nl_definition_call_begin(const NLCheckedFragment *caller, NLCheckedNodeId call,
                         const NLCheckedNodeId args[2], size_t caller_function,
                         const NLSemanticContext *c)
{
    NLDefinitionCallView description = {0};
    if (!definition_observer ||
        !dc_describe(caller, call, args, caller_function, c, &description))
        return NULL;
    NLDefinitionCallCapture *p = calloc(1, sizeof(*p));
    NLCheckStatus s = p ? nl_sem_clone(c, &p->pre) : NL_CHECK_OUT_OF_MEMORY;
    if (s != NL_CHECK_OK) {
        if (p) {
            nl_semantic_destroy(p->pre);
            free(p);
        }
        definition_observer(NULL, NULL, s, definition_user);
        return NULL;
    }
    p->view = description;
    p->view.pre = p->pre;
    p->observer = definition_observer;
    p->user = definition_user;
    return p;
}
void nl_definition_call_attach(NLDefinitionCallCapture *p,
                               const NLCheckedFragment *body,
                               const NLSymbolId parameters[2],
                               const NLSemanticContext *c)
{
    if (!p)
        return;
    p->view.callee = body;
    memcpy(p->view.formals, parameters, sizeof(p->view.formals));
    p->status = nl_sem_clone(c, &p->post);
    p->view.post = p->post;
}
void nl_definition_call_push(NLDefinitionCallCapture **head,
                             NLDefinitionCallCapture *p)
{
    if (!p)
        return;
    while (*head) {
        ++p->view.ordinal;
        head = &(*head)->next;
    }
    *head = p;
}
void nl_definition_call_finish(NLDefinitionCallCapture **head, bool complete)
{
    size_t count = 0;
    for (NLDefinitionCallCapture *p = *head; p; p = p->next)
        ++count;
    while (*head) {
        NLDefinitionCallCapture *p = *head;
        *head = p->next;
        p->view.pending_count = count;
        NLCheckStatus s = p->status;
        if (!complete && s == NL_CHECK_OK)
            s = INVALID;
        p->observer(p, &p->view, s, p->user);
        nl_semantic_destroy(p->pre);
        nl_semantic_destroy(p->post);
        free(p);
    }
}
static bool dc_span(NLSourceSpan a, NLSourceSpan b)
{
    return a.start_byte == b.start_byte && a.end_byte == b.end_byte;
}
static bool dc_name(const NLSource *s, NLSourceSpan span, const char *name)
{
    NLSourceView text;
    return name && nl_source_view(s, span, &text) &&
           strlen(name) == text.length &&
           !memcmp(text.bytes, name, text.length);
}
static const NLSyntaxView *dc_find_if(const NLSyntaxNode *n, NLSourceSpan span,
                                      size_t depth)
{
    const NLSyntaxView *s = nl_syntax_node_view(n);
    if (!s || depth > 16)
        return NULL;
    if (s->kind == NL_SYNTAX_IF && dc_span(s->span, span))
        return s;
    if (s->kind == NL_SYNTAX_BINDING)
        return dc_find_if(s->data.binding.initializer, span, depth + 1);
    if (s->kind == NL_SYNTAX_STATEMENT)
        return dc_find_if(s->data.statement.expression, span, depth + 1);
    if (s->kind == NL_SYNTAX_BLOCK) {
        for (const NLSyntaxNode *i = s->data.block.items; i;
             i = nl_syntax_next_argument(i)) {
            const NLSyntaxView *found = dc_find_if(i, span, depth + 1);
            if (found)
                return found;
        }
        return dc_find_if(s->data.block.tail, span, depth + 1);
    }
    return NULL;
}
static NLCheckStatus dc_equal(const NLSemanticContext *a,
                              const NLSemanticContext *b)
{
    NLCopyCallAnchor anchor = {0};
    NLCheckStatus s = nl_copy_state_anchor(a, &anchor);
    if (s == NL_CHECK_OK &&
        (!nl_copy_same_types(a, b) || !nl_copy_state_matches(b, &anchor) ||
         a->last_incarnation != b->last_incarnation ||
         a->last_value_fact != b->last_value_fact))
        s = INVALID;
    nl_copy_state_destroy(&anchor);
    return s;
}
static const NLCheckedNodeView *dc_take(const NLCheckedFragment *f,
                                        NLCheckedNodeId id, bool *seen)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (!n || seen[id])
        return NULL;
    seen[id] = true;
    return n;
}
static bool dc_covered(const NLCheckedFragment *f, const bool *seen)
{
    for (size_t i = 1; i <= f->count; ++i)
        if (!seen[i])
            return false;
    return true;
}
static NLCheckStatus dc_bind(NLSemanticContext *c, const NLSource *source,
                             NLSourceSpan span, NLValueId value, size_t floor,
                             NLSymbolId *out)
{
    NLSourceView text;
    char name[256];
    if (!nl_source_view(source, span, &text) || !text.length ||
        text.length >= sizeof(name))
        return INVALID;
    memcpy(name, text.bytes, text.length);
    name[text.length] = '\0';
    return nl_sem_bind_in_scope(c, name, value, floor, out);
}
static bool dc_scalar(const NLSemanticContext *c, NLValueId id)
{
    return nl_copy_plain(c, id, nl_semantic_core_type(c, NL_TYPE_U8));
}
/* Reconstruct the checked operation from its original AST and fresh replay
 * state. No expected scalar, saved POST or issued summary supplies the effect.
 */
static NLCheckStatus dc_replay(const NLDefinitionCallView *v,
                               const NLSemanticContext *pre,
                               const NLSemanticContext *post,
                               NLDefinitionCallResult *out)
{
    *out = (NLDefinitionCallResult){0};
    if (!v || !pre || !post || v->ordinal || v->pending_count != 1 ||
        v->arm > 1)
        return INVALID;
    const NLCheckedFragment *f = v->caller, *b = v->callee;
    if (!f || !b || f->context != v->world || f->if_parent != v->parent ||
        f->if_arm_index != v->arm || !f->if_parent || f->body_count != 1 ||
        f->body_calls[0] != v->call || f->bodies[0] != b ||
        nl_checked_call_body(f, v->call) != b || b->context != f->context ||
        b->copy_body || b->body_owner != v->body ||
        b->source != v->body->source || b->arm_count || b->body_count ||
        !v->caller_function || v->caller_function > pre->function_count)
        return INVALID;
    const NLFunctionEntry *caller_fn = &pre->functions[v->caller_function - 1];
    const NLFunctionEntry *fn = &pre->functions[v->function - 1];
    if (!caller_fn->body || caller_fn->body->source != f->source ||
        fn->body != v->body || fn->count != 2 || !fn->parameters ||
        fn->body->count != 2 || fn->result != fn->parameters[1] ||
        fn->result != nl_semantic_core_type(pre, NL_TYPE_U8))
        return INVALID;
    const NLCheckedNodeView *choice =
        nl_checked_node_view(f->if_parent, f->if_parent_node);
    const NLSyntaxView *conditional =
        choice ? dc_find_if(nl_syntax_tree_root(caller_fn->body->syntax),
                            choice->span, 0)
               : NULL;
    if (!choice || choice->kind != NL_CHECKED_IF || !conditional)
        return INVALID;
    const NLSyntaxView *arm =
        nl_syntax_node_view(v->arm ? conditional->data.conditional.else_block
                                   : conditional->data.conditional.then_block);
    const NLSyntaxView *other_arm =
        nl_syntax_node_view(v->arm ? conditional->data.conditional.then_block
                                   : conditional->data.conditional.else_block);
    if (!arm || !other_arm || arm->kind != NL_SYNTAX_BLOCK ||
        other_arm->kind != NL_SYNTAX_BLOCK || arm->data.block.item_count ||
        other_arm->data.block.item_count)
        return INVALID;
    const NLSyntaxView *loan_ast = nl_syntax_node_view(arm->data.block.tail);
    const NLSyntaxView *other_loan =
        nl_syntax_node_view(other_arm->data.block.tail);
    if (!loan_ast || !other_loan ||
        loan_ast->kind != NL_SYNTAX_LOCAL_WRITE_LOAN ||
        other_loan->kind != NL_SYNTAX_LOCAL_WRITE_LOAN)
        return INVALID;
    const NLSyntaxView *field_ast =
        nl_syntax_node_view(loan_ast->data.loan.source);
    const NLSyntaxView *other_field =
        nl_syntax_node_view(other_loan->data.loan.source);
    const NLSyntaxView *loan_block =
        nl_syntax_node_view(loan_ast->data.loan.body);
    if (!field_ast || !other_field || !loan_block ||
        field_ast->kind != NL_SYNTAX_FIELD_DESIGNATOR ||
        other_field->kind != NL_SYNTAX_FIELD_DESIGNATOR ||
        loan_block->kind != NL_SYNTAX_BLOCK ||
        loan_block->data.block.item_count)
        return INVALID;
    const NLSyntaxView *call_ast =
        nl_syntax_node_view(loan_block->data.block.tail);
    if (!call_ast || call_ast->kind != NL_SYNTAX_EXPR_CALL ||
        call_ast->data.call.argument_count != 2 ||
        !dc_name(f->source, call_ast->data.call.callee, fn->name))
        return INVALID;
    NLIfFieldProof *entry_proof = f->if_parent->if_fields;
    while (entry_proof && entry_proof->view.conditional != f->if_parent_node)
        entry_proof = entry_proof->next;
    if (!entry_proof || entry_proof->owner != f->if_parent ||
        entry_proof->arms[v->arm] != f || !entry_proof->entry)
        return INVALID;
    const NLSemanticContext *entry = entry_proof->entry;
    if (entry->domain_count || entry->region_count || entry->occurrence_count)
        return INVALID;
    NLSemanticContext *m = NULL;
    bool *seen = calloc(f->count + 1, sizeof(*seen));
    bool *body_seen = calloc(b->count + 1, sizeof(*body_seen));
    NLCheckStatus s = NL_CHECK_OUT_OF_MEMORY;
    if (!seen || !body_seen)
        goto done;
#define DC_REQUIRE(x)                                                          \
    do {                                                                       \
        if (!(x)) {                                                            \
            out->failure_line = __LINE__;                                      \
            s = INVALID;                                                       \
            goto done;                                                         \
        }                                                                      \
    } while (0)
#define DC_DO(x)                                                               \
    do {                                                                       \
        s = (x);                                                               \
        if (s != NL_CHECK_OK) {                                                \
            out->failure_line = __LINE__;                                      \
            goto done;                                                         \
        }                                                                      \
    } while (0)
    DC_DO(nl_sem_clone(entry, &m));
    const NLCheckedNodeView *ar = dc_take(f, f->root, seen);
    const NLCheckedNodeView *ab = ar ? dc_take(f, ar->initializer, seen) : NULL;
    const NLCheckedNodeView *loan = ab ? dc_take(f, ab->tail, seen) : NULL;
    const NLCheckedNodeView *lb =
        loan ? dc_take(f, loan->initializer, seen) : NULL;
    const NLCheckedNodeView *call_node = lb ? dc_take(f, lb->tail, seen) : NULL;
    DC_REQUIRE(ar && ab && loan && lb && call_node &&
               ar->kind == NL_CHECKED_IF_ARM && !ar->terminates &&
               dc_span(ar->span, arm->span) && ab->kind == NL_CHECKED_BLOCK &&
               !ab->item_count && loan->kind == NL_CHECKED_LOAN_HEADER &&
               ab->tail == v->loan && dc_span(loan->span, loan_ast->span) &&
               lb->kind == NL_CHECKED_BLOCK && !lb->item_count &&
               lb->tail == v->call &&
               call_node->kind == NL_CHECKED_REGISTERED_CALL &&
               dc_span(call_node->span, call_ast->span) &&
               call_node->function == v->function && call_node->body_backed &&
               call_node->argument_count == 2 && !call_node->terminates &&
               loan->loan.access == NL_ACCESS_WRITE &&
               !loan->loan.is_exclusive && loan->loan.implicit_local &&
               !loan->loan.from_ptr && loan->loan.body_nonescape_proved &&
               loan->loan.normal_result_forwarded &&
               !f->context->scopes[v->scope - 1].active);
    DC_REQUIRE(loan->field.present &&
               !memcmp(&loan->field, &v->field, sizeof(v->field)) &&
               loan->field.index < 2 && field_at(m, &loan->field) &&
               loan->field.child == loan->loan.place &&
               loan->field.child_incarnation == loan->loan.incarnation &&
               loan->loan.scope == v->scope &&
               loan->loan.source == loan->field.base);
    NLSemanticPlaceView trio[3], untouched[3];
    DC_REQUIRE(nl_copy_trio(m, loan->field.parent, trio) &&
               nl_copy_trio(m, v->other, untouched) &&
               v->other != loan->field.parent &&
               untouched[0].type == trio[0].type &&
               v->sibling == trio[0].fixed_fields[1 - loan->field.index] &&
               dc_name(f->source, field_ast->data.field_designator.base,
                       m->bindings[loan->field.base - 1].name) &&
               dc_name(f->source, other_field->data.field_designator.base,
                       m->bindings[loan->field.base - 1].name));
    const char *label =
        m->types[trio[0].type - 1].field_names[loan->field.index];
    DC_REQUIRE(
        dc_name(f->source, field_ast->data.field_designator.field, label) &&
        dc_name(f->source, other_field->data.field_designator.field, label));
    size_t initial_bindings = m->binding_count, initial_scopes = m->scope_count;
    NLScopeId scope = 0;
    NLValueId ref = 0, copied_ref = 0, incoming = 0;
    NLSymbolId binder = 0;
    DC_DO(nl_sem_new_scope(m, 0, true, &scope));
    DC_DO(nl_sem_new_value(
        m,
        (NLSemanticValueView){
            .type = fn->parameters[0],
            .reference = {loan->field.child, loan->field.child_incarnation,
                          scope, NL_PROVENANCE_VALID, true, true, 0}},
        &ref));
    DC_DO(dc_bind(m, f->source, loan_ast->data.loan.binding, ref,
                  initial_bindings, &binder));
    DC_REQUIRE(scope == v->scope && binder == loan->loan.ref_symbol);
    const NLCheckedNodeView *a = dc_take(f, call_node->first_argument, seen);
    const NLCheckedNodeView *new_node =
        a ? dc_take(f, a->next_argument, seen) : NULL;
    const NLSyntaxNode *actual0 = call_ast->data.call.arguments;
    const NLSyntaxView *actual0_ast = nl_syntax_node_view(actual0);
    const NLSyntaxView *actual1_ast =
        nl_syntax_node_view(nl_syntax_next_argument(actual0));
    DC_REQUIRE(a && new_node && !new_node->next_argument &&
               call_node->first_argument == v->arguments[0] &&
               a->next_argument == v->arguments[1] &&
               a->kind == NL_CHECKED_IDENTIFIER && a->symbol == binder &&
               a->value_use == NL_VALUE_COPIED &&
               a->type == fn->parameters[0] && actual0_ast &&
               actual0_ast->kind == NL_SYNTAX_EXPR_NAME &&
               dc_name(f->source, actual0_ast->data.name,
                       m->bindings[binder - 1].name) &&
               actual1_ast && actual1_ast->kind == NL_SYNTAX_U8_LITERAL &&
               new_node->kind == NL_CHECKED_U8_LITERAL &&
               dc_span(a->span, actual0_ast->span) &&
               dc_span(new_node->span, actual1_ast->span));
    NLSourceView digits;
    DC_REQUIRE(
        nl_source_view(f->source, actual1_ast->data.u8_digits, &digits) &&
        digits.length);
    uint64_t number = 0;
    for (size_t i = 0; i < digits.length; ++i) {
        unsigned char ch = (unsigned char)digits.bytes[i];
        DC_REQUIRE(ch >= '0' && ch <= '9' && number <= 25);
        number = number * 10 + (ch - '0');
        DC_REQUIRE(number <= 255);
    }
    DC_DO(nl_sem_copy_value(m, ref, &copied_ref));
    DC_DO(nl_sem_new_value(m,
                           (NLSemanticValueView){.type = fn->result,
                                                 .scalar_known = true,
                                                 .scalar_value = number},
                           &incoming));
    DC_REQUIRE(
        a->result_count == 1 && a->results[0].value == copied_ref &&
        new_node->result_count == 1 && new_node->results[0].value == incoming &&
        new_node->type == fn->result && new_node->scalar_result.known &&
        new_node->scalar_result.value == number && dc_scalar(m, incoming));
    DC_DO(dc_equal(m, pre));
    /* Plain scalar history is not a liveness dependency on the old field.
     * Preserve Unknown explicitly in observations rather than invent a byte. */
    for (size_t i = 1; i < 3; ++i) {
        DC_REQUIRE(dc_scalar(m, trio[i].current_value) &&
                   dc_scalar(m, untouched[i].current_value));
    }
    out->pre_verified = true;
    out->old_known =
        m->values[trio[loan->field.index + 1].current_value - 1].scalar_known;
    out->sibling_known =
        m->values[trio[1 + (1 - loan->field.index)].current_value - 1]
            .scalar_known;
    out->old_value =
        m->values[trio[loan->field.index + 1].current_value - 1].scalar_value;
    out->incoming = number;
    out->sibling =
        m->values[trio[1 + (1 - loan->field.index)].current_value - 1]
            .scalar_value;
    const size_t floor = m->binding_count, places = m->place_count,
                 scopes_floor = m->scope_count;
    NLSymbolId formal[2] = {0};
    DC_DO(nl_sem_bind_in_scope(m, fn->body->parameter_names[0], copied_ref,
                               floor, &formal[0]));
    DC_DO(nl_sem_bind_in_scope(m, fn->body->parameter_names[1], incoming, floor,
                               &formal[1]));
    DC_REQUIRE(!memcmp(formal, v->formals, sizeof(formal)));
    const NLSyntaxView *ast_body =
        nl_syntax_node_view(nl_syntax_tree_root(fn->body->syntax));
    const NLCheckedNodeView *body_root = dc_take(b, b->root, body_seen);
    DC_REQUIRE(ast_body && ast_body->kind == NL_SYNTAX_BLOCK && body_root &&
               body_root->kind == NL_CHECKED_BLOCK && !body_root->tail &&
               !ast_body->data.block.tail &&
               dc_span(body_root->span, ast_body->span));
    NLCheckedNodeId item = body_root->first_item;
    const NLSyntaxNode *ast_item = ast_body->data.block.items;
    bool old_origin[NL_SEMANTIC_MAX_ENTRIES] = {false};
    size_t items = 0;
    bool returned = false;
    while (item && ast_item) {
        const NLCheckedNodeView *n = dc_take(b, item, body_seen);
        const NLSyntaxView *ast = nl_syntax_node_view(ast_item);
        DC_REQUIRE(n && ast && dc_span(n->span, ast->span) && !returned);
        if (ast->kind == NL_SYNTAX_BINDING && n->kind == NL_CHECKED_BINDING) {
            const NLSyntaxView *init =
                nl_syntax_node_view(ast->data.binding.initializer);
            const NLCheckedNodeView *expr =
                dc_take(b, n->initializer, body_seen);
            DC_REQUIRE(init && expr && dc_span(expr->span, init->span));
            NLValueId value = 0;
            bool origin = false;
            if (init->kind == NL_SYNTAX_EXPR_CALL &&
                expr->kind == NL_CHECKED_REPLACE) {
                DC_REQUIRE(
                    !out->replacements &&
                    dc_name(b->source, init->data.call.callee, "replace") &&
                    init->data.call.argument_count == 2 &&
                    expr->argument_count == 2);
                const NLCheckedNodeView *r =
                    dc_take(b, expr->first_argument, body_seen);
                const NLCheckedNodeView *fresh =
                    r ? dc_take(b, r->next_argument, body_seen) : NULL;
                const NLSyntaxNode *source_r = init->data.call.arguments;
                const NLSyntaxView *ra = nl_syntax_node_view(source_r);
                const NLSyntaxView *fa =
                    nl_syntax_node_view(nl_syntax_next_argument(source_r));
                DC_REQUIRE(r && fresh && !fresh->next_argument && ra && fa &&
                           ra->kind == NL_SYNTAX_EXPR_NAME &&
                           fa->kind == NL_SYNTAX_EXPR_NAME &&
                           r->kind == NL_CHECKED_IDENTIFIER &&
                           fresh->kind == NL_CHECKED_IDENTIFIER &&
                           r->symbol == formal[0] &&
                           fresh->symbol == formal[1] &&
                           dc_name(b->source, ra->data.name,
                                   fn->body->parameter_names[0]) &&
                           dc_name(b->source, fa->data.name,
                                   fn->body->parameter_names[1]) &&
                           dc_span(r->span, ra->span) &&
                           dc_span(fresh->span, fa->span));
                NLValueId rcopy = 0, next = 0;
                DC_DO(nl_sem_copy_value(
                    m, m->bindings[formal[0] - 1].view.value, &rcopy));
                DC_DO(nl_sem_copy_value(
                    m, m->bindings[formal[1] - 1].view.value, &next));
                DC_REQUIRE(r->results[0].value == rcopy &&
                           fresh->results[0].value == next &&
                           m->values[rcopy - 1].reference.scope == scope &&
                           m->scopes[scope - 1].active &&
                           m->values[rcopy - 1].reference.place ==
                               loan->field.child &&
                           dc_scalar(m, next));
                const NLSemanticPlaceView child =
                    m->places[loan->field.child - 1];
                NLCheckedField effect = loan->field;
                effect.present = effect.dependency_compatible = true;
                effect.parent_fact = m->places[effect.parent - 1].current_fact;
                effect.child_fact = child.current_fact;
                effect.old_value = value = child.current_value;
                effect.new_value = next;
                DC_DO(nl_fixed_change_dependencies(m, effect.child, 0));
                DC_DO(nl_fixed_change(m, effect.child, next, false));
                effect.parent_post_fact =
                    m->places[effect.parent - 1].current_fact;
                effect.child_post_fact =
                    m->places[effect.child - 1].current_fact;
                effect.post_payload_occurrence =
                    m->places[effect.child - 1].payload_occurrence;
                DC_REQUIRE(!memcmp(&effect, &expr->field, sizeof(effect)) &&
                           expr->result_count == 1 &&
                           expr->results[0].value == value);
                for (size_t k = 0; k < m->binding_count; ++k)
                    if (m->bindings[k].view.place == effect.parent &&
                        m->bindings[k].view.availability == NL_AVAILABLE)
                        m->bindings[k].view.value =
                            m->places[effect.parent - 1].current_value;
                nl_sem_end_value(m, rcopy);
                ++out->replacements;
                origin = true;
            } else if (init->kind == NL_SYNTAX_EXPR_NAME &&
                       expr->kind == NL_CHECKED_IDENTIFIER) {
                DC_REQUIRE(expr->symbol && expr->symbol <= m->binding_count &&
                           dc_name(b->source, init->data.name,
                                   m->bindings[expr->symbol - 1].name));
                DC_DO(nl_sem_copy_value(
                    m, m->bindings[expr->symbol - 1].view.value, &value));
                origin = old_origin[expr->symbol];
                DC_REQUIRE(expr->results[0].value == value);
            } else {
                DC_REQUIRE(false);
            }
            NLSymbolId symbol = 0;
            DC_DO(dc_bind(m, b->source, ast->data.binding.name, value, floor,
                          &symbol));
            /* Ordinary source u8 locals, unlike formal storage, are implicit
             * lexical originals even when immediately ended by RETURN. */
            m->places[m->bindings[symbol - 1].view.place - 1].implicit_local =
                true;
            const NLCheckedNodeView *receiver =
                dc_take(b, n->first_argument, body_seen);
            DC_REQUIRE(n->argument_count == 1 && n->symbol == symbol &&
                       symbol < NL_SEMANTIC_MAX_ENTRIES && receiver &&
                       receiver->kind == NL_CHECKED_RECEIVER &&
                       receiver->symbol == symbol && !receiver->next_argument);
            old_origin[symbol] = origin;
        } else if (ast->kind == NL_SYNTAX_RETURN &&
                   n->kind == NL_CHECKED_RETURN) {
            const NLSyntaxView *ret_ast =
                nl_syntax_node_view(ast->data.statement.expression);
            const NLCheckedNodeView *ret_expr =
                dc_take(b, n->initializer, body_seen);
            DC_REQUIRE(
                ret_ast && ret_expr && ret_ast->kind == NL_SYNTAX_EXPR_NAME &&
                ret_expr->kind == NL_CHECKED_IDENTIFIER && ret_expr->symbol &&
                ret_expr->symbol <= m->binding_count &&
                old_origin[ret_expr->symbol] &&
                dc_name(b->source, ret_ast->data.name,
                        m->bindings[ret_expr->symbol - 1].name) &&
                dc_span(ret_expr->span, ret_ast->span));
            NLValueId old_copy = 0;
            DC_DO(nl_sem_copy_value(
                m, m->bindings[ret_expr->symbol - 1].view.value, &old_copy));
            DC_REQUIRE(ret_expr->results[0].value == old_copy &&
                       n->returned.value == old_copy &&
                       n->returned.type == fn->result && n->terminates &&
                       !n->next_item && call_node->result_count == 1 &&
                       call_node->results[0].value == old_copy &&
                       call_node->type == fn->result);
            out->returned = old_copy;
            returned = true;
        } else {
            DC_REQUIRE(false);
        }
        ++items;
        item = n->next_item;
        ast_item = nl_syntax_next_argument(ast_item);
    }
    DC_REQUIRE(!item && !ast_item && returned && out->replacements == 1 &&
               items == body_root->item_count &&
               items == ast_body->data.block.item_count &&
               dc_covered(f, seen) && dc_covered(b, body_seen));
    /* The checked RETURN edge belongs to the callee's function, not the
     * caller continuation or another graph with equal numerical values. */
    const NLControlExitView *edge = nl_control_exit_view(b->exits, 0);
    DC_REQUIRE(nl_control_exits_count(b->exits) == 1 &&
               !nl_control_exits_count(b->loop_returns) && edge &&
               edge->kind == NL_EXIT_RETURN && edge->count == 1 &&
               edge->target == b->function_target && b->function_target &&
               !nl_control_target_same(b->function_target,
                                       f->if_parent->function_target) &&
               edge->values[0].type == fn->result &&
               edge->values[0].value == out->returned);
    DC_DO(nl_copy_end_locals(m, floor, places, scopes_floor));
    DC_DO(dc_equal(m, nl_control_state_context((NLControlState *)edge->state)));
    DC_DO(dc_equal(m, post));
    out->return_old_verified = true;
    /* Now close the actual enclosing lexical loan; not the callee RETURN. */
    DC_DO(nl_copy_end_locals(m, initial_bindings, m->place_count,
                             initial_scopes));
    for (size_t i = initial_scopes; i < m->scope_count; ++i)
        m->scopes[i].active = false;
    DC_DO(dc_equal(m, f->context));
    out->loan_closed = true;
    s = NL_CHECK_OK;
done:
    nl_semantic_destroy(m);
    free(seen);
    free(body_seen);
#undef DC_REQUIRE
#undef DC_DO
    return s;
}

NLCheckStatus nl_definition_call_validate(const NLDefinitionCallCapture *p,
                                          const NLDefinitionCallView *candidate,
                                          NLDefinitionCallResult *out)
{
    if (!out)
        return INVALID;
    *out = (NLDefinitionCallResult){0};
    if (!p || !candidate || p->status != NL_CHECK_OK ||
        memcmp(&p->view, candidate, sizeof(*candidate)))
        return INVALID;
    return dc_replay(&p->view, p->pre, p->post, out);
}

/* A private construction session owns ORIGINAL checker graphs. It never
 * retains a Stage0 capture/view, calls an observer, or creates a public event.
 * Pending references are used only after membership in the retained original
 * tree has been established. No graph with invented/rebased IDs is built. */
struct NLDefinitionOwnedPending {
    NLDefinitionOwnedLink link;
    NLSemanticContext *pre, *post;
    struct NLDefinitionOwnedPending *next;
};
struct NLDefinitionOwned {
    NLDefinitionOwnedPending *first, *last;
    NLCheckedFragment *root;
    size_t count;
    NLCheckStatus status;
    bool closed;
    bool prefix, unit_done;
    const NLSource *unit_input; /* registration-scope token ONLY, then NULL */
    NLSource *unit;             /* OWNED original full raw unit bytes */
    struct {
        NLFunctionBody *body; /* owned retain of actual created plan */
        NLSourceSpan span;    /* original unit BODY span */
        size_t function;
    } plans[2];
    size_t plan_count;
};
static _Thread_local NLDefinitionOwned *owned_session;

static void owned_discard(NLDefinitionOwned *w)
{
    while (w->first) {
        NLDefinitionOwnedPending *p = w->first;
        w->first = p->next;
        nl_semantic_destroy(p->pre);
        nl_semantic_destroy(p->post);
        free(p);
    }
    w->last = NULL;
    w->count = 0;
}
NLCheckStatus nl_definition_owned_open(NLDefinitionOwned **out)
{
    if (!out || owned_session)
        return INVALID;
    *out = NULL;
    NLDefinitionOwned *w = calloc(1, sizeof(*w));
    if (!w)
        return NL_CHECK_OUT_OF_MEMORY;
    owned_session = *out = w;
    return NL_CHECK_OK;
}
void nl_definition_owned_close(NLDefinitionOwned *w)
{
    if (!w || owned_session != w)
        return;
    owned_session = NULL;
    w->closed = true;
    if (!w->root && w->status == NL_CHECK_OK)
        w->status = INVALID;
    if (w->status != NL_CHECK_OK)
        owned_discard(w);
}
void nl_definition_owned_destroy(NLDefinitionOwned *w)
{
    if (!w)
        return;
    if (owned_session == w)
        nl_definition_owned_close(w);
    /* PRE/POST retain registered source plans independently of the tree.
     * Drop records first, then nested graph/control owners, root context last.
     */
    owned_discard(w);
    nl_checked_destroy(w->root);
    for (size_t i = 0; i < w->plan_count; ++i)
        nl_body_release(w->plans[i].body);
    nl_source_destroy(w->unit);
    free(w);
}
NLCheckStatus nl_definition_owned_status(const NLDefinitionOwned *w)
{
    return w && w->closed ? w->status : INVALID;
}
size_t nl_definition_owned_count(const NLDefinitionOwned *w)
{
    return nl_definition_owned_status(w) == NL_CHECK_OK ? w->count : 0;
}
const NLDefinitionOwnedLink *
nl_definition_owned_link(const NLDefinitionOwned *w, size_t index)
{
    if (index >= nl_definition_owned_count(w))
        return NULL;
    const NLDefinitionOwnedPending *p = w->first;
    while (index--)
        p = p->next;
    return &p->link;
}
NLDefinitionOwnedPending *
nl_definition_owned_call(const NLCheckedFragment *f, NLCheckedNodeId call,
                         const NLCheckedNodeId arguments[2], size_t function,
                         const NLSemanticContext *c)
{
    NLDefinitionOwned *w = owned_session;
    if (!w || w->status != NL_CHECK_OK)
        return NULL;
    if (w->root) { /* Exactly one definition instance, no reentry. */
        w->status = INVALID;
        return NULL;
    }
    NLDefinitionCallView d = {0}; /* temporary descriptor, never an observer */
    if (!dc_describe(f, call, arguments, function, c, &d)) {
        w->status = INVALID;
        return NULL;
    }
    NLDefinitionOwnedPending *p = calloc(1, sizeof(*p));
    NLCheckStatus s = p ? nl_sem_clone(c, &p->pre) : NL_CHECK_OUT_OF_MEMORY;
    if (s != NL_CHECK_OK) {
        free(p);
        w->status = s;
        return NULL;
    }
    p->link = (NLDefinitionOwnedLink){.caller = f,
                                      .parent = f->if_parent,
                                      .world = c,
                                      .pre = p->pre,
                                      .body = d.body,
                                      .caller_function = function,
                                      .function = d.function,
                                      .arm = d.arm,
                                      .call = call,
                                      .arguments = {arguments[0], arguments[1]},
                                      .loan = d.loan,
                                      .field = d.field,
                                      .sibling = d.sibling,
                                      .other = d.other,
                                      .scope = d.scope};
    if (w->last)
        w->last->next = p;
    else
        w->first = p;
    w->last = p;
    ++w->count;
    return p;
}
void nl_definition_owned_attach(NLDefinitionOwnedPending *p,
                                const NLCheckedFragment *body,
                                const NLSymbolId formals[2],
                                const NLSemanticContext *c)
{
    if (!p || !owned_session)
        return;
    NLCheckStatus s = nl_sem_clone(c, &p->post);
    if (s != NL_CHECK_OK) {
        owned_session->status = s;
        return;
    }
    p->link.callee = body;
    p->link.post = p->post;
    memcpy(p->link.formals, formals, sizeof(p->link.formals));
}
/* Authenticate the actual parent-owned, source-ordered IF edges, not merely
 * the set of children or the separate proof/backpointer view. Check BOTH arms
 * on transfer and on EVERY replay; equal NodeIds/world contents are no seal. */
static bool owned_arm_edges(const NLCheckedFragment *root,
                            const NLDefinitionOwnedLink *v)
{
    /* Pending callers may have been freed on an incomplete-check path. Do not
     * dereference one until its exact stored parent edge is established. */
    if (!root || !v || v->parent != root || root->arm_count != 2 ||
        !root->arms || v->arm > 1 || !v->caller ||
        root->arms[v->arm].artifact != v->caller)
        return false;
    const NLCheckedNodeId id = v->caller->if_parent_node;
    const NLCheckedNodeView *choice = nl_checked_node_view(root, id);
    if (!choice || choice->kind != NL_CHECKED_IF || choice->normal_arms != 2)
        return false;
    const NLIfFieldProof *p = root->if_fields;
    while (p && p->view.conditional != id)
        p = p->next;
    if (!p || p->owner != root || p->source != root->source || !p->entry ||
        p->view.entry != p->entry)
        return false;
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedArm *edge = &root->arms[a];
        const NLCheckedFragment *f = edge->artifact;
        if (!f || edge->match != id || nl_checked_if_arm(root, id, a) != f ||
            p->arms[a] != f || p->view.arms[a].artifact != f ||
            f->if_parent != root || f->if_parent_node != id ||
            f->if_arm_index != a || f->source != root->source || !f->context ||
            edge->context_origin != f->context ||
            p->view.arms[a].world != f->context)
            return false;
    }
    return root->arms[0].artifact != root->arms[1].artifact &&
           root->arms[0].context_origin != root->arms[1].context_origin &&
           v->world == root->arms[v->arm].context_origin;
}
void nl_definition_owned_take(NLCheckedFragment **root, NLSemanticContext **c)
{
    NLDefinitionOwned *w = owned_session;
    if (!w || !w->first)
        return;
    /* Check membership by pointer comparison before dereferencing a pending
     * graph: incomplete branches may already have been destroyed on error. */
    if (w->status != NL_CHECK_OK || w->root || !root || !*root || !c || !*c ||
        (*root)->context != *c || w->count != 2 || (*root)->arm_count != 2)
        goto fail;
    bool arm_seen[2] = {false, false};
    for (NLDefinitionOwnedPending *p = w->first; p; p = p->next) {
        if (!owned_arm_edges(*root, &p->link) || !p->pre || !p->post ||
            arm_seen[p->link.arm])
            goto fail;
        arm_seen[p->link.arm] = true;
        p->link.pending_count = 1;
    }
    w->root = *root;
    w->root->destroy_context = nl_semantic_destroy;
    *root = NULL;
    *c = NULL;
    return;
fail:
    if (w->status == NL_CHECK_OK)
        w->status = INVALID;
    owned_discard(w);
}
NLCheckStatus nl_definition_owned_validate(const NLDefinitionOwned *w,
                                           size_t index,
                                           const NLDefinitionOwnedLink *v,
                                           NLDefinitionCallResult *out)
{
    if (!out)
        return INVALID;
    *out = (NLDefinitionCallResult){0};
    const NLDefinitionOwnedLink *issued = nl_definition_owned_link(w, index);
    if (!issued || !v || memcmp(issued, v, sizeof(*v)) || !w->root ||
        v->parent != w->root || v->arm > 1)
        return INVALID;
    if (!owned_arm_edges(w->root, v))
        return INVALID;
    const NLDefinitionCallView replay = {
        .caller = v->caller,
        .callee = v->callee,
        .parent = v->parent,
        .world = v->world,
        .pre = v->pre,
        .post = v->post,
        .body = v->body,
        .caller_function = v->caller_function,
        .function = v->function,
        .arm = v->arm,
        .ordinal = v->ordinal,
        .pending_count = v->pending_count,
        .call = v->call,
        .arguments = {v->arguments[0], v->arguments[1]},
        .loan = v->loan,
        .formals = {v->formals[0], v->formals[1]},
        .field = v->field,
        .sibling = v->sibling,
        .other = v->other,
        .scope = v->scope};
    /* No cached successful result: walk source/graph and independently execute
     * from common entry on EVERY invocation, comparing PRE/POST only afterward.
     */
    return dc_replay(&replay, v->pre, v->post, out);
}

NLCheckStatus nl_definition_prefix_open(NLDefinitionOwned **out)
{
    NLCheckStatus s = nl_definition_owned_open(out);
    if (s == NL_CHECK_OK)
        (*out)->prefix = true;
    return s;
}
void nl_definition_prefix_unit_begin(const NLSource *source, size_t count)
{
    NLDefinitionOwned *w = owned_session;
    if (!w || !w->prefix || w->status != NL_CHECK_OK)
        return;
    if (count != 1 || !source || w->unit || w->unit_done) {
        w->status = INVALID;
        return;
    }
    NLSourceView bytes;
    if (!nl_source_view(source, (NLSourceSpan){0, nl_source_length(source)},
                        &bytes)) {
        w->status = INVALID;
        return;
    }
    NLSourceStatus s = nl_source_create(bytes.bytes, bytes.length,
                                        nl_source_name(source), &w->unit);
    if (s != NL_SOURCE_OK) {
        w->status = s == NL_SOURCE_OUT_OF_MEMORY ? NL_CHECK_OUT_OF_MEMORY
                                                 : NL_CHECK_RESOURCE_LIMIT;
        return;
    }
    w->unit_input = source;
}
void nl_definition_prefix_plan(const NLSource *source, NLSourceSpan span,
                               size_t function, NLFunctionBody *body)
{
    NLDefinitionOwned *w = owned_session;
    if (!w || !w->prefix || w->status != NL_CHECK_OK)
        return;
    if (!w->unit || w->unit_done || source != w->unit_input || !body ||
        !function || w->plan_count == 2) {
        w->status = INVALID;
        return;
    }
    NLCheckStatus s = nl_body_retain(body);
    if (s != NL_CHECK_OK) {
        w->status = s;
        return;
    }
    const size_t i = w->plan_count++;
    w->plans[i].function = function;
    w->plans[i].span = span;
    w->plans[i].body = body;
}
void nl_definition_prefix_unit_end(void)
{
    NLDefinitionOwned *w = owned_session;
    if (!w || !w->prefix)
        return;
    w->unit_input = NULL; /* no original input borrow survives this boundary */
    w->unit_done = true;
    if (w->status == NL_CHECK_OK && (!w->unit || w->plan_count != 2))
        w->status = INVALID;
}

/* Independently reparse the OWNED WHOLE unit, not a retained body fragment or
 * observed entry. Only existing primitive constructors operate on the fresh
 * scratch machine. This is partial typed C memory, NOT rich Domain issuance. */
static bool prefix_name(const NLSource *source, NLSourceSpan span,
                        char out[256])
{
    NLSourceView v;
    if (!nl_source_view(source, span, &v) || !v.length || v.length >= 256)
        return false;
    memcpy(out, v.bytes, v.length);
    out[v.length] = 0;
    return true;
}
static NLCheckStatus prefix_type(NLSemanticContext *m, const NLSource *source,
                                 const NLSyntaxNode *node, NLTypeId *out)
{
    const NLSyntaxView *s = nl_syntax_node_view(node);
    if (!s)
        return INVALID;
    if (s->kind == NL_SYNTAX_TYPE_REF) {
        const NLSyntaxView *target =
            nl_syntax_node_view(s->data.ref_type.target);
        if (!target || target->kind != NL_SYNTAX_TYPE_NAME ||
            !dc_name(source, target->data.name, "u8") ||
            s->data.ref_type.access != NL_ACCESS_WRITE ||
            s->data.ref_type.is_exclusive)
            return INVALID;
        return nl_sem_compound(m, NL_TYPE_REF,
                               nl_semantic_core_type(m, NL_TYPE_U8),
                               NL_ACCESS_WRITE, false, out);
    }
    if (s->kind != NL_SYNTAX_TYPE_NAME)
        return INVALID;
    if (dc_name(source, s->data.name, "u8"))
        *out = nl_semantic_core_type(m, NL_TYPE_U8);
    else if (dc_name(source, s->data.name, "unit"))
        *out = nl_semantic_unit_type(m);
    else
        return INVALID;
    return NL_CHECK_OK;
}
typedef struct {
    const NLSource *unit;
    const NLCheckedFragment *graph;
    NLSemanticContext *machine;
    bool *seen;
    size_t offset;
} PrefixWalk;
static bool prefix_span(const PrefixWalk *p, NLSourceSpan actual,
                        NLSourceSpan raw)
{
    return raw.start_byte >= p->offset && raw.end_byte >= p->offset &&
           actual.start_byte == raw.start_byte - p->offset &&
           actual.end_byte == raw.end_byte - p->offset;
}
static const NLCheckedNodeView *prefix_take(PrefixWalk *p, NLCheckedNodeId id,
                                            NLCheckedKind kind,
                                            NLSourceSpan raw)
{
    const NLCheckedNodeView *n = dc_take(p->graph, id, p->seen);
    return n && n->kind == kind && prefix_span(p, n->span, raw) ? n : NULL;
}
static NLCheckStatus prefix_literal(PrefixWalk *p, const NLSyntaxNode *node,
                                    NLCheckedNodeId id, NLValueId *out)
{
    const NLSyntaxView *s = nl_syntax_node_view(node);
    if (!s ||
        (s->kind != NL_SYNTAX_U8_LITERAL && s->kind != NL_SYNTAX_USIZE_LITERAL))
        return INVALID;
    const bool quantity = s->kind == NL_SYNTAX_USIZE_LITERAL;
    const NLCheckedNodeView *n = prefix_take(
        p, id, quantity ? NL_CHECKED_USIZE_LITERAL : NL_CHECKED_U8_LITERAL,
        s->span);
    NLSourceView digits;
    if (!n ||
        !nl_source_view(p->unit,
                        quantity ? s->data.usize_digits : s->data.u8_digits,
                        &digits) ||
        !digits.length)
        return INVALID;
    const size_t maximum = quantity ? SIZE_MAX : 255;
    size_t number = 0;
    for (size_t i = 0; i < digits.length; ++i) {
        if (digits.bytes[i] < '0' || digits.bytes[i] > '9')
            return INVALID;
        const size_t d = (size_t)(digits.bytes[i] - '0');
        if (number > (maximum - d) / 10)
            return INVALID;
        number = number * 10 + d;
    }
    const NLTypeId type = nl_semantic_core_type(
        p->machine, quantity ? NL_TYPE_USIZE : NL_TYPE_U8);
    NLCheckStatus status = nl_sem_new_value(
        p->machine,
        (NLSemanticValueView){
            .type = type, .scalar_known = true, .scalar_value = number},
        out);
    if (status != NL_CHECK_OK)
        return status;
    return n->type == type && n->result_count == 1 &&
                   n->results[0].type == type && n->results[0].value == *out &&
                   n->has_scalar_result && n->scalar_result.type == type &&
                   n->scalar_result.known && n->scalar_result.value == number
               ? NL_CHECK_OK
               : INVALID;
}
static NLCheckStatus prefix_binding(PrefixWalk *p, const NLSyntaxView *binding,
                                    NLCheckedNodeId id, NLTypeId nominal,
                                    const char *nominal_name, NLPlaceId *root)
{
    if (!binding || binding->kind != NL_SYNTAX_BINDING)
        return INVALID;
    const NLSyntaxView *ast =
        nl_syntax_node_view(binding->data.binding.initializer);
    const NLCheckedNodeView *b =
        prefix_take(p, id, NL_CHECKED_BINDING, binding->span);
    if (!b || !ast || ast->kind != NL_SYNTAX_AGGREGATE ||
        ast->data.aggregate.count != 2 ||
        !dc_name(p->unit, ast->data.aggregate.type_name, nominal_name))
        return INVALID;
    const NLCheckedNodeView *aggregate =
        prefix_take(p, b->initializer, NL_CHECKED_AGGREGATE, ast->span);
    if (!aggregate || aggregate->type != nominal ||
        aggregate->argument_count != 2)
        return INVALID;
    NLSemanticValueView package = {.type = nominal, .field_count = 2};
    NLCheckedNodeId field_id = aggregate->first_argument;
    size_t used = 0;
    NLCheckStatus status = NL_CHECK_OK;
    for (const NLSyntaxNode *a = ast->data.aggregate.fields; a;
         a = nl_syntax_next_argument(a)) {
        const NLSyntaxView *field_ast = nl_syntax_node_view(a);
        const NLCheckedNodeView *field =
            field_ast ? prefix_take(p, field_id, NL_CHECKED_AGGREGATE_FIELD,
                                    field_ast->span)
                      : NULL;
        if (!field)
            return INVALID;
        size_t index = 2;
        for (size_t i = 0; i < 2; ++i)
            if (dc_name(p->unit, field_ast->data.binding.name,
                        p->machine->types[nominal - 1].field_names[i]))
                index = i;
        if (index == 2 || (used & (1U << index)) ||
            field->field_index != index ||
            !prefix_span(p, field->name, field_ast->data.binding.name) ||
            field->type != nl_semantic_core_type(p->machine, NL_TYPE_U8))
            return INVALID;
        used |= 1U << index;
        status = prefix_literal(p, field_ast->data.binding.initializer,
                                field->initializer, &package.fields[index]);
        if (status != NL_CHECK_OK)
            return status;
        field_id = field->next_argument;
    }
    if (used != 3 || field_id)
        return INVALID;
    NLValueId value = 0;
    status = nl_sem_new_value(p->machine, package, &value);
    if (status != NL_CHECK_OK)
        return status;
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticValueView *member =
            &p->machine->values[package.fields[i] - 1];
        member->carrier = NL_CARRIER_AGGREGATE;
        member->aggregate_owner = value;
    }
    char name[256];
    if (!prefix_name(p->unit, binding->data.binding.name, name))
        return INVALID;
    NLSymbolId symbol = 0;
    status = nl_sem_bind_in_scope(p->machine, name, value, 0, &symbol);
    if (status != NL_CHECK_OK)
        return status;
    *root = p->machine->bindings[symbol - 1].view.place;
    /* Existing ordinary lexical binding rule; NOT a Domain/Allocation grant. */
    p->machine->places[*root - 1].implicit_local = true;
    const NLCheckedNodeView *receiver = prefix_take(
        p, b->first_argument, NL_CHECKED_RECEIVER, binding->data.binding.name);
    return aggregate->result_count == 1 &&
                   aggregate->results[0].type == nominal &&
                   aggregate->results[0].value == value &&
                   b->symbol == symbol && b->argument_count == 1 && receiver &&
                   !receiver->next_argument && receiver->symbol == symbol &&
                   receiver->type == nominal &&
                   receiver->value_use == NL_VALUE_RECEIVED &&
                   prefix_span(p, b->name, binding->data.binding.name)
               ? NL_CHECK_OK
               : INVALID;
}
NLCheckStatus nl_definition_prefix_validate(const NLDefinitionOwned *w,
                                            NLDefinitionPrefixResult *out)
{
    if (!out)
        return INVALID;
    *out = (NLDefinitionPrefixResult){0};
    if (!w || !w->prefix || !w->closed)
        return INVALID;
    if (w->status != NL_CHECK_OK)
        return w->status;
    if (!w->unit_done || w->unit_input || !w->unit || w->plan_count != 2 ||
        nl_definition_owned_count(w) != 2)
        return INVALID;
    NLSemanticContext *m = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    PrefixWalk walk = {.unit = w->unit, .graph = w->root};
    NLCheckStatus status = INVALID;
#define PR_REQUIRE(x)                                                          \
    do {                                                                       \
        if (!(x)) {                                                            \
            out->failure_line = __LINE__;                                      \
            status = INVALID;                                                  \
            goto done;                                                         \
        }                                                                      \
    } while (0)
#define PR_DO(x)                                                               \
    do {                                                                       \
        status = (x);                                                          \
        if (status != NL_CHECK_OK) {                                           \
            out->failure_line = __LINE__;                                      \
            goto done;                                                         \
        }                                                                      \
    } while (0)
    NLParseStatus parse = nl_parser_create(w->unit, &parser);
    if (parse == NL_PARSE_OK)
        parse = nl_parser_parse_function_unit(parser, &tree, NULL);
    if (parse != NL_PARSE_OK) {
        status =
            parse == NL_PARSE_OUT_OF_MEMORY ? NL_CHECK_OUT_OF_MEMORY : INVALID;
        goto done;
    }
    PR_DO(nl_semantic_create(&m)); /* no observed registry/state seeds */
    walk.machine = m;
    const size_t core_functions = m->function_count;
    const NLSyntaxView *unit = nl_syntax_node_view(nl_syntax_tree_root(tree));
    PR_REQUIRE(unit && unit->kind == NL_SYNTAX_FUNCTION_UNIT &&
               unit->data.function_unit.count == 3);
    const NLSyntaxNode *decl = unit->data.function_unit.declarations;
    const NLSyntaxView *nominal_ast = nl_syntax_node_view(decl);
    PR_REQUIRE(nominal_ast && nominal_ast->kind == NL_SYNTAX_AVS_STRUCT &&
               nominal_ast->data.avs_struct.count == 2);
    char nominal_name[256], labels[2][256];
    PR_REQUIRE(
        prefix_name(w->unit, nominal_ast->data.avs_struct.name, nominal_name));
    NLAggregateField fields[2];
    const NLSyntaxNode *field = nominal_ast->data.avs_struct.fields;
    for (size_t i = 0; i < 2; ++i) {
        const NLSyntaxView *f = nl_syntax_node_view(field);
        NLTypeId type = 0;
        PR_REQUIRE(f &&
                   prefix_name(w->unit, f->data.parameter.name, labels[i]));
        PR_DO(prefix_type(m, w->unit, f->data.parameter.type, &type));
        PR_REQUIRE(type == nl_semantic_core_type(m, NL_TYPE_U8));
        fields[i] = (NLAggregateField){labels[i], type};
        field = nl_syntax_next_argument(field);
    }
    PR_REQUIRE(!field);
    NLTypeId nominal = 0;
    PR_DO(nl_semantic_register_aggregate(m, nominal_name, fields, 2, &nominal));
    const NLSyntaxView *functions[2];
    char names[2][256];
    decl = nl_syntax_next_argument(decl);
    for (size_t i = 0; i < 2; ++i) {
        functions[i] = nl_syntax_node_view(decl);
        PR_REQUIRE(
            functions[i] && functions[i]->kind == NL_SYNTAX_FUNCTION &&
            prefix_name(w->unit, functions[i]->data.function.name, names[i]));
        decl = nl_syntax_next_argument(decl);
    }
    PR_REQUIRE(!decl && strcmp(names[0], names[1]));
    size_t order[2] = {0, 1};
    if (strcmp(names[0], names[1]) > 0) {
        order[0] = 1;
        order[1] = 0;
    }
    const NLSyntaxView *main_body = NULL;
    size_t main_id = 0;
    const NLSemanticContext *observed = NULL;
    PR_REQUIRE(w->root && !w->root->root && w->root->if_fields &&
               !w->root->if_fields->next);
    const NLIfFieldProof *entry = w->root->if_fields;
    PR_REQUIRE(entry->owner == w->root && entry->source == w->root->source &&
               entry->view.entry == entry->entry && entry->entry);
    observed = entry->entry; /* target ONLY, never a machine initializer */
    for (size_t ordinal = 0; ordinal < 2; ++ordinal) {
        const size_t i = order[ordinal];
        const NLSyntaxView *f = functions[i];
        NLTypeId parameters[2] = {0}, result = 0;
        char parameter_names[2][256];
        size_t count = 0;
        for (const NLSyntaxNode *p = f->data.function.parameters; p;
             p = nl_syntax_next_argument(p)) {
            const NLSyntaxView *v = nl_syntax_node_view(p);
            PR_REQUIRE(v && count < 2 &&
                       prefix_name(w->unit, v->data.parameter.name,
                                   parameter_names[count]));
            PR_DO(prefix_type(m, w->unit, v->data.parameter.type,
                              &parameters[count]));
            ++count;
        }
        PR_DO(prefix_type(m, w->unit, f->data.function.result, &result));
        PR_REQUIRE(count == f->data.function.count);
        const bool is_main = !strcmp(names[i], "main");
        PR_REQUIRE(
            is_main ? (!count && result == 1)
                    : (count == 2 &&
                       m->types[parameters[0] - 1].view.kind == NL_TYPE_REF &&
                       parameters[1] == nl_semantic_core_type(m, NL_TYPE_U8) &&
                       result == parameters[1]));
        PR_DO(nl_semantic_register_function(m, names[i], parameters, count,
                                            result, false, false));
        const size_t id = m->function_count;
        PR_REQUIRE(id <= observed->function_count &&
                   w->plans[ordinal].function == id && w->plans[ordinal].body);
        const NLFunctionEntry *actual = &observed->functions[id - 1];
        const NLFunctionBody *body = w->plans[ordinal].body;
        const NLSyntaxView *body_ast =
            nl_syntax_node_view(f->data.function.body);
        PR_REQUIRE(body_ast && body_ast->kind == NL_SYNTAX_BLOCK &&
                   actual->body == body && body->count == count &&
                   actual->kind == NL_CHECKED_REGISTERED_CALL && actual->name &&
                   !strcmp(actual->name, names[i]) && actual->count == count &&
                   actual->result == result && !actual->caller_effects &&
                   !actual->hidden_dependencies && !actual->owner_receiver &&
                   !actual->owner_producer && !actual->custody_recipient &&
                   (!count || (actual->parameters &&
                               !memcmp(actual->parameters, parameters,
                                       count * sizeof(*parameters)))) &&
                   dc_span(w->plans[ordinal].span, body_ast->span) &&
                   nl_syntax_tree_source(body->syntax) == body->source);
        NLSourceView original, copied;
        PR_REQUIRE(nl_source_view(w->unit, body_ast->span, &original) &&
                   original.length == nl_source_length(body->source) &&
                   nl_source_view(body->source,
                                  (NLSourceSpan){0, original.length},
                                  &copied) &&
                   !memcmp(original.bytes, copied.bytes, original.length));
        for (size_t j = 0; j < count; ++j)
            PR_REQUIRE(!strcmp(body->parameter_names[j], parameter_names[j]));
        if (is_main) {
            PR_REQUIRE(!main_body && w->root->source == body->source);
            main_body = body_ast;
            main_id = id;
        }
    }
    PR_REQUIRE(main_body && observed->function_count == core_functions + 2 &&
               m->type_count == observed->type_count &&
               nl_copy_same_types(m, observed));
    for (size_t i = 0; i < m->type_count; ++i) {
        PR_REQUIRE(m->types[i].option_target ==
                       observed->types[i].option_target &&
                   m->types[i].allocated_target ==
                       observed->types[i].allocated_target &&
                   m->types[i].one_backing_target ==
                       observed->types[i].one_backing_target &&
                   m->types[i].live_tail_target ==
                       observed->types[i].live_tail_target);
        PR_REQUIRE((!m->types[i].name && !observed->types[i].name) ||
                   (m->types[i].name && observed->types[i].name &&
                    !strcmp(m->types[i].name, observed->types[i].name)));
    }
    for (size_t i = 0; i < 2; ++i) {
        const NLDefinitionOwnedLink *v = nl_definition_owned_link(w, i);
        PR_REQUIRE(v && v->caller_function == main_id &&
                   owned_arm_edges(w->root, v) &&
                   v->pre->functions[main_id - 1].body ==
                       observed->functions[main_id - 1].body);
    }
    out->registry_verified = true;
    walk.offset = main_body->span.start_byte;
    walk.seen = calloc(w->root->count + 1, sizeof(*walk.seen));
    if (!walk.seen) {
        status = NL_CHECK_OUT_OF_MEMORY;
        goto done;
    }
    const NLCheckedNodeView *block =
        prefix_take(&walk, 1, NL_CHECKED_BLOCK, main_body->span);
    PR_REQUIRE(block && block->item_count == 2 && !block->tail &&
               main_body->data.block.item_count == 3);
    const NLSyntaxNode *item = main_body->data.block.items;
    NLCheckedNodeId checked = block->first_item;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *binding =
            nl_checked_node_view(w->root, checked);
        PR_REQUIRE(binding);
        PR_DO(prefix_binding(&walk, nl_syntax_node_view(item), checked, nominal,
                             nominal_name, &out->roots[i]));
        checked = binding->next_item;
        item = nl_syntax_next_argument(item);
    }
    PR_REQUIRE(!checked && out->roots[0] != out->roots[1]);
    out->roots_verified = true;
    const NLSyntaxView *s = nl_syntax_node_view(item);
    PR_REQUIRE(
        s && !nl_syntax_next_argument(item) &&
        (s->kind == NL_SYNTAX_STATEMENT || s->kind == NL_SYNTAX_BINDING));
    const NLSyntaxView *conditional = nl_syntax_node_view(
        s->kind == NL_SYNTAX_STATEMENT ? s->data.statement.expression
                                       : s->data.binding.initializer);
    PR_REQUIRE(conditional && conditional->kind == NL_SYNTAX_IF);
    out->conditional = entry->view.conditional;
    const NLCheckedNodeView *choice =
        prefix_take(&walk, out->conditional, NL_CHECKED_IF, conditional->span);
    PR_REQUIRE(choice && choice->normal_arms == 2 && choice->item_count == 2 &&
               out->conditional == w->root->count);
    out->condition = choice->initializer;
    const NLSyntaxView *condition =
        nl_syntax_node_view(conditional->data.conditional.condition);
    PR_REQUIRE(condition && (condition->kind == NL_SYNTAX_USIZE_EQUALITY ||
                             condition->kind == NL_SYNTAX_USIZE_LESS_THAN));
    const bool less = condition->kind == NL_SYNTAX_USIZE_LESS_THAN;
    const NLCheckedNodeView *comparison = prefix_take(
        &walk, out->condition,
        less ? NL_CHECKED_USIZE_LESS_THAN : NL_CHECKED_USIZE_EQUALITY,
        condition->span);
    PR_REQUIRE(comparison && comparison->argument_count == 2 &&
               prefix_span(&walk, comparison->name,
                           condition->data.equality.operator_span));
    const NLCheckedNodeView *left =
        nl_checked_node_view(w->root, comparison->first_argument);
    PR_REQUIRE(left && left->next_argument);
    const NLCheckedNodeView *right =
        nl_checked_node_view(w->root, left->next_argument);
    PR_REQUIRE(right && !right->next_argument);
    NLValueId a = 0, b = 0, value = 0;
    PR_DO(prefix_literal(&walk, condition->data.equality.left,
                         comparison->first_argument, &a));
    PR_DO(prefix_literal(&walk, condition->data.equality.right,
                         left->next_argument, &b));
    const NLTypeId quantity = nl_semantic_core_type(m, NL_TYPE_USIZE);
    PR_REQUIRE(m->values[a - 1].type == quantity &&
               m->values[b - 1].type == quantity);
    out->truth =
        less ? m->values[a - 1].scalar_value < m->values[b - 1].scalar_value
             : m->values[a - 1].scalar_value == m->values[b - 1].scalar_value;
    const NLTypeId boolean = nl_semantic_core_type(m, NL_TYPE_BOOL);
    PR_DO(nl_sem_new_value(m,
                           (NLSemanticValueView){.type = boolean,
                                                 .scalar_known = true,
                                                 .scalar_value = out->truth},
                           &value));
    PR_REQUIRE(comparison->type == boolean && comparison->result_count == 1 &&
               comparison->results[0].type == boolean &&
               comparison->results[0].value == value &&
               comparison->has_scalar_result &&
               comparison->scalar_result.type == boolean &&
               comparison->scalar_result.known &&
               comparison->scalar_result.value == out->truth);
    nl_sem_end_value(m, a);
    nl_sem_end_value(m, b);
    nl_sem_end_value(m, value);
    PR_REQUIRE(dc_covered(w->root, walk.seen));
    out->condition_verified = true;
    PR_DO(dc_equal(m, observed));
    PR_REQUIRE(!observed->domain_count && !observed->scope_count &&
               !observed->region_count && !observed->occurrence_count &&
               observed->binding_count == m->binding_count);
    for (size_t i = 0; i < m->binding_count; ++i)
        PR_REQUIRE(m->bindings[i].hidden == observed->bindings[i].hidden &&
                   !strcmp(m->bindings[i].name, observed->bindings[i].name));
    out->entry_verified = true;
    status = NL_CHECK_OK;
done:
    if (status != NL_CHECK_OK && status != NL_CHECK_OUT_OF_MEMORY &&
        status != NL_CHECK_RESOURCE_LIMIT)
        status = INVALID;
    free(walk.seen);
    nl_semantic_destroy(m);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
#undef PR_REQUIRE
#undef PR_DO
    return status;
}
