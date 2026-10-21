#include "newlang/copy_body.h"
#include "semantic_internal.h"
#include <stdlib.h>
#include <string.h>

#define INVALID NL_CHECK_ANALYSIS_PRECISION_LIMIT
/* Issuance anchors are private, not writable public views or host inputs.
 * The completed graph seal authenticates identity, not lost lifetime state:
 * those facts are captured independently while the original is still live. */
#include "copy_body_internal.h"

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
    nl_semantic_destroy(p->pending);
    nl_semantic_destroy(p->exit);
    free(p->exit_scopes);
    free(p->nodes);
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
static bool trio(const NLSemanticContext *c, NLPlaceId root,
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
    if (!trio(c, root, e->post) ||
        (e->before && !trio(e->before, root, e->pre)))
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
NLCheckStatus nl_copy_body_seal(NLCheckedFragment *f,
                                const NLCheckedFragment *caller,
                                NLCheckedNodeId call, size_t function)
{
    NLCopyBodyProof *p = f->copy_body;
    if (!p)
        return NL_CHECK_OK;
    if (!p->exit || p->pending || !f->count ||
        f->count > NL_SEMANTIC_MAX_ENTRIES)
        return NL_CHECK_OK; /* unproved richer body stays backend unsupported */
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
static bool plain(const NLSemanticContext *c, NLValueId id, NLTypeId t)
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
                        const NLSemanticValueView anchor[3])
{
    for (size_t i = 0; i < 3; ++i)
        if (memcmp(&c->values[a[i].current_value - 1], &anchor[i],
                   sizeof(anchor[i])))
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
    if (!trio(e->after, root, after) || !same_places(after, e->post) ||
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
        } else if (!trio(e->before, root, before) ||
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
    if (!trio(c, e->parent, a) || !e->present || !e->dependency_compatible ||
        e->index >= 2 || e->base == 0 || e->base > c->binding_count ||
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
           plain(r->proof->exit, n->results[0].value, r->u8);
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
             plain(e->after, e->view.result, r->u8) &&
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
        if (!loan || n->argument_count != 2 || !scalar(r, id))
            break;
        const NLCheckedNodeView *a = take(r, n->first_argument);
        if (!a || a->kind != NL_CHECKED_IDENTIFIER ||
            a->symbol != loan->loan.ref_symbol || !a->next_argument ||
            !expression(r, a->next_argument, loan) ||
            !scalar(r, a->next_argument))
            break;
        const CopyEvent *e = next_event(r, id, NL_COPY_BODY_CHANGE);
        if (!e || !field_at(e->before, &n->field) ||
            !field_at(e->before, &loan->field) ||
            loan->loan.place != n->field.child ||
            loan->loan.incarnation != n->field.child_incarnation ||
            !loan->loan.scope || loan->loan.scope > e->before->scope_count ||
            !e->before->scopes[loan->loan.scope - 1].active ||
            !a->results[0].value ||
            a->results[0].value > e->before->value_count)
            break;
        const NLReferenceFacts ref =
            e->before->values[a->results[0].value - 1].reference;
        if (!ref.writable || ref.provenance != NL_PROVENANCE_VALID ||
            !ref.readable || ref.occurrence_dependency ||
            ref.scope != loan->loan.scope || ref.place != n->field.child ||
            ref.incarnation != n->field.child_incarnation ||
            e->view.result != n->field.old_value ||
            n->results[0].value != e->view.result ||
            e->view.incoming !=
                r->f->nodes[a->next_argument - 1].results[0].value ||
            n->field.parent_post_fact != e->post[0].current_fact ||
            n->field.child_post_fact !=
                e->post[n->field.index + 1].current_fact)
            break;
        NLSemanticContext *replay = NULL;
        r->status = nl_sem_clone(e->before, &replay);
        if (r->status == NL_CHECK_OK)
            r->status = nl_fixed_change(replay, n->field.child,
                                        e->view.incoming, false);
        NLSemanticPlaceView actual[3];
        ok = r->status == NL_CHECK_OK &&
             trio(replay, n->field.parent, actual) &&
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
static bool same_types(const NLSemanticContext *c,
                       const NLSemanticContext *anchor)
{
    if (!c || !anchor)
        return false;
    for (size_t i = 0;
         i < (anchor->type_count < c->type_count ? anchor->type_count
                                                 : c->type_count);
         ++i) {
        const NLTypeEntry *a = &c->types[i], *b = &anchor->types[i];
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
NLCheckStatus nl_checked_copy_body_validate(const NLCheckedFragment *caller,
                                            NLCheckedNodeId id,
                                            const NLCopyBodyView *candidate)
{
    const NLCopyBodyProof *p = proof(caller, id);
    if (!p || (candidate && !same_body_view(candidate, &p->view)))
        return INVALID;
    const NLCheckedFragment *f = p->owner;
    const NLSemanticContext *c = nl_checked_context(caller);
    const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
    if (!c || !call || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || call->terminates || call->argument_count ||
        call->first_argument || call->result_count != 1 ||
        call->function != p->view.function || !call->function ||
        call->function > c->function_count || nl_checked_context(f) != c ||
        f->body_owner != p->body || f->source != p->source ||
        f->source != p->body->source ||
        c->functions[call->function - 1].body != p->body ||
        c->functions[call->function - 1].kind != NL_CHECKED_REGISTERED_CALL ||
        c->functions[call->function - 1].caller_effects ||
        c->functions[call->function - 1].hidden_dependencies ||
        c->functions[call->function - 1].owner_receiver ||
        c->functions[call->function - 1].owner_producer ||
        c->functions[call->function - 1].custody_recipient ||
        c->functions[call->function - 1].count || p->body->count ||
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
        c->type_count < p->exit->type_count || !same_types(c, p->exit) ||
        nl_sem_validate(p->exit) != NL_CHECK_OK)
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
        if (!same_types(e->after, p->exit))
            goto done;
        changed |= e->view.kind == NL_COPY_BODY_CHANGE;
        if (e->view.kind != NL_COPY_BODY_INITIALIZE)
            continue;
        NLSemanticPlaceView exit[3];
        if (!trio(p->exit, e->view.field.parent, exit) ||
            !modeled(&r, exit, e->view.field.parent))
            goto done;
    }
    if (!changed)
        goto done;
    for (size_t i = 0; i < p->exit->scope_count; ++i)
        if (p->exit->scopes[i].active)
            goto done;
    /* Scalars keep identity/type/value after ordinary scope teardown. A
     * later caller operation cannot rewrite the issued return package. */
    for (size_t i = 0; i < f->count; ++i) {
        const NLCheckedNodeView *n = &f->nodes[i];
        if (n->type != r.u8 || n->result_count != 1)
            continue;
        const NLValueId v = n->results[0].value;
        if (!plain(p->exit, v, r.u8) || !plain(c, v, r.u8) ||
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
