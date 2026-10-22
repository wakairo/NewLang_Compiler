#include "if_field_internal.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define INVALID NL_CHECK_ANALYSIS_PRECISION_LIMIT
static NLIfFieldProof *find(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    for (NLIfFieldProof *p = f ? f->if_fields : NULL; p; p = p->next)
        if (p->view.conditional == id && p->owner == f)
            return p;
    return NULL;
}
static NLIfFieldProof *parent(const NLCheckedFragment *f)
{
    NLIfFieldProof *p = f ? find(f->if_parent, f->if_parent_node) : NULL;
    if (p && f->if_arm_index < 2 && p->arms[f->if_arm_index] == f)
        return p;
    for (p = f ? f->if_fields : NULL; p; p = p->next)
        if (p->replaying)
            return p;
    return NULL;
}
static size_t track(const NLIfFieldProof *p, const NLCheckedFragment *f)
{
    return f == p->owner ? 2 : f->if_arm_index;
}
static NLIfFieldArm *stream(NLIfFieldProof *p, size_t a)
{
    return a == 2 ? &p->replay : &p->view.arms[a];
}
static void destroy(NLCheckedFragment *f)
{
    while (f->if_fields) {
        NLIfFieldProof *p = f->if_fields;
        f->if_fields = p->next;
        nl_semantic_destroy(p->entry);
        nl_semantic_destroy(p->post);
        nl_copy_state_destroy(&p->entry_anchor);
        nl_copy_state_destroy(&p->post_anchor);
        nl_semantic_destroy(p->return_live);
        nl_copy_state_destroy(&p->return_anchor);
        nl_copy_state_destroy(&p->exit_anchor);
        for (size_t a = 0; a < 2; ++a) {
            nl_copy_state_destroy(&p->arm_anchor[a]);
            free(p->nodes[a]);
        }
        for (size_t a = 0; a < 3; ++a) {
            nl_semantic_destroy(p->pending[a]);
            while (p->first[a]) {
                NLIfFieldTrace *e = p->first[a];
                p->first[a] = e->next;
                nl_semantic_destroy(e->before);
                nl_semantic_destroy(e->after);
                nl_copy_state_destroy(&e->pre_anchor);
                nl_copy_state_destroy(&e->post_anchor);
                free(e);
            }
        }
        free(p->parent_nodes);
        free(p);
    }
}
NLCheckStatus nl_if_field_prepare(NLCheckedFragment *f, NLCheckedNodeId id,
                                  const NLSemanticContext *c)
{
    bool eligible = false;
    NLSemanticPlaceView parts[3];
    for (size_t i = 1; i <= c->place_count; ++i)
        eligible |= nl_copy_trio(c, i, parts);
    if (!eligible)
        return NL_CHECK_OK;
    NLIfFieldProof *p = calloc(1, sizeof(*p));
    if (!p)
        return NL_CHECK_OUT_OF_MEMORY;
    p->owner = f;
    p->source = f->source;
    p->body = f->body_owner;
    p->view.conditional = id;
    p->view.condition = nl_checked_node_view(f, id)->initializer;
    p->next = f->if_fields;
    f->if_fields = p;
    f->destroy_if_fields = destroy;
    NLCheckStatus s = nl_sem_clone(c, &p->entry);
    if (s == NL_CHECK_OK)
        s = nl_copy_state_anchor(c, &p->entry_anchor);
    p->view.entry = p->entry;
    return s;
}
NLCheckStatus nl_if_field_fork(NLCheckedFragment *f, NLCheckedNodeId id,
                               size_t a, NLCheckedFragment *arm)
{
    NLIfFieldProof *p = find(f, id);
    if (!p)
        return NL_CHECK_OK;
    if (a >= 2 || p->arms[a] ||
        !nl_copy_state_matches(arm->context, &p->entry_anchor))
        return NL_CHECK_INTERNAL_ERROR;
    p->arms[a] = arm;
    p->view.arms[a].artifact = arm;
    p->view.arms[a].world = arm->context;
    return NL_CHECK_OK;
}
NLCheckStatus nl_if_field_before(NLCheckedFragment *f, NLCheckedNodeId id,
                                 const NLSemanticContext *c)
{
    NLIfFieldProof *p = parent(f);
    if (!p)
        return NL_CHECK_OK;
    /* Source primitive has not published its field yet. Capture any field
     * Change; planning later rejects foreign/non-nominal target histories. */
    size_t a = track(p, f);
    if (p->pending[a])
        return NL_CHECK_INTERNAL_ERROR;
    p->pending_node[a] = id;
    return nl_sem_clone(c, &p->pending[a]);
}
NLCheckStatus nl_if_field_event(NLCheckedFragment *f, NLCheckedNodeId id,
                                const NLSemanticContext *c)
{
    NLIfFieldProof *p = parent(f);
    if (!p)
        return NL_CHECK_OK;
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (!n || !n->field.present)
        return NL_CHECK_OK;
    if (n->kind != NL_CHECKED_REPLACE && n->kind != NL_CHECKED_FIELD_READ)
        return NL_CHECK_OK;
    const size_t a = track(p, f);
    if (stream(p, a)->event_count >= NL_SEMANTIC_MAX_ENTRIES)
        return NL_CHECK_RESOURCE_LIMIT;
    NLIfFieldTrace *e = calloc(1, sizeof(*e));
    if (!e)
        return NL_CHECK_OUT_OF_MEMORY;
    if (p->last[a])
        p->last[a]->next = e;
    else
        p->first[a] = e;
    p->last[a] = e;
    ++stream(p, a)->event_count;
    e->origin = f;
    e->stream_index = a;
    e->view = (NLIfFieldEvent){.node = id,
                               .change = n->kind == NL_CHECKED_REPLACE,
                               .field = n->field,
                               .result = n->results[0].value};
    if (e->view.change) {
        if (!p->pending[a] || p->pending_node[a] != id)
            return NL_CHECK_INTERNAL_ERROR;
        e->before = p->pending[a];
        p->pending[a] = NULL;
        const NLCheckedNodeView *arg =
            nl_checked_node_view(f, n->first_argument);
        const NLCheckedNodeView *value =
            arg ? nl_checked_node_view(f, arg->next_argument) : NULL;
        if (!value || value->result_count != 1)
            return NL_CHECK_INTERNAL_ERROR;
        e->view.incoming = value->results[0].value;
        ++stream(p, a)->change_count;
    }
    NLCheckStatus s = nl_sem_clone(c, &e->after);
    if (s == NL_CHECK_OK)
        s = nl_copy_state_anchor(c, &e->post_anchor);
    if (s == NL_CHECK_OK && e->before)
        s = nl_copy_state_anchor(e->before, &e->pre_anchor);
    e->view.before = e->before;
    e->view.after = e->after;
    e->issued = e->view;
    return s;
}
static bool frame_state(const NLIfFieldProof *p, const NLSemanticContext *after,
                        bool before_return)
{
    const NLSemanticContext *e = p->entry;
    if (after->place_count < e->place_count ||
        after->binding_count < e->binding_count ||
        after->scope_count < e->scope_count ||
        after->domain_count != e->domain_count ||
        after->region_count != e->region_count ||
        after->occurrence_count != e->occurrence_count)
        return false;
    for (size_t i = 0; i < e->place_count; ++i) {
        NLSemanticPlaceView a = e->places[i], b = after->places[i];
        if (i + 1 == p->view.root || i + 1 == p->view.child) {
            b.current_fact = a.current_fact;
            b.current_value = a.current_value;
        }
        if (memcmp(&a, &b, sizeof(a)))
            return false;
    }
    for (size_t i = 0; i < e->binding_count; ++i) {
        NLSemanticBindingView a = e->bindings[i].view,
                              b = after->bindings[i].view;
        if (a.place == p->view.root)
            b.value = a.value;
        if (memcmp(&a, &b, sizeof(a)))
            return false;
    }
    for (size_t i = 0; i < e->scope_count; ++i)
        if (memcmp(&e->scopes[i], &after->scopes[i], sizeof(e->scopes[i])))
            return false;
    for (size_t i = e->scope_count; i < after->scope_count; ++i)
        if (after->scopes[i].active)
            return false;
    for (size_t i = e->place_count; i < after->place_count; ++i)
        if (after->places[i].live &&
            (!before_return ||
             !after->types[after->places[i].type - 1].view.is_copy ||
             !after->types[after->places[i].type - 1].view.is_discardable))
            return false;
    for (size_t i = 0; i < e->domain_count; ++i)
        if (memcmp(&e->domains[i], &after->domains[i], sizeof(e->domains[i])))
            return false;
    for (size_t i = 0; i < e->region_count; ++i)
        if (memcmp(&e->regions[i].view, &after->regions[i].view,
                   sizeof(e->regions[i].view)) ||
            e->regions[i].count != after->regions[i].count ||
            (e->regions[i].count &&
             memcmp(e->regions[i].intervals, after->regions[i].intervals,
                    e->regions[i].count * sizeof(*e->regions[i].intervals))))
            return false;
    return true;
}
static bool frame(const NLIfFieldProof *p, const NLSemanticContext *after)
{
    return frame_state(p, after, false);
}
NLCheckStatus nl_if_field_plan(NLCheckedFragment *f, NLCheckedNodeId id,
                               const NLSemanticContext *c, NLPlaceId *out)
{
    *out = 0;
    NLIfFieldProof *p = find(f, id);
    if (!p)
        return NL_CHECK_OK;
    const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
    if (!choice || choice->normal_arms != 2)
        return NL_CHECK_OK;
    NLPlaceId root = 0, child = 0;
    for (size_t a = 0; a < 2; ++a) {
        if (!p->arms[a] || p->pending[a])
            return NL_CHECK_OK;
        for (NLIfFieldTrace *e = p->first[a]; e; e = e->next)
            if (e->view.change) {
                if (root && (root != e->view.field.parent ||
                             child != e->view.field.child))
                    return NL_CHECK_OK;
                root = e->view.field.parent;
                child = e->view.field.child;
            }
    }
    if (!root || !child || child > c->place_count)
        return NL_CHECK_OK;
    NLSemanticPlaceView parts[3];
    if (!nl_copy_trio(c, root, parts))
        return NL_CHECK_OK;
    const size_t index = c->places[child - 1].parent_field_index;
    if (index >= 2 || parts[0].fixed_fields[index] != child)
        return NL_CHECK_OK;
    p->view.root = root;
    p->view.child = child;
    p->view.sibling = parts[0].fixed_fields[1 - index];
    p->view.index = index;
    p->view.incarnation = parts[0].incarnation;
    p->view.child_incarnation = parts[index + 1].incarnation;
    p->view.nominal = parts[0].type;
    for (size_t j = 0; j < c->binding_count; ++j)
        if (!c->bindings[j].hidden && c->bindings[j].view.place == root)
            p->view.base = j + 1;
    if (!p->view.base)
        return NL_CHECK_OK;
    for (size_t a = 0; a < 2; ++a)
        if (!frame(p, p->arms[a]->context) ||
            !nl_copy_trio(p->arms[a]->context, root, parts))
            return NL_CHECK_OK;
    /* The common world is still exactly the source-issued entry. */
    if (!nl_copy_state_matches(c, &p->entry_anchor))
        return NL_CHECK_INTERNAL_ERROR;
    p->ready = true;
    *out = root;
    return NL_CHECK_OK;
}
static NLCheckStatus structural_join(NLSemanticContext *c, NLPlaceId child)
{
    NLCheckStatus s = nl_fixed_change_dependencies(c, child, 0);
    if (s != NL_CHECK_OK)
        return s;
    NLValueId value = 0;
    s = nl_sem_new_value(
        c, (NLSemanticValueView){.type = nl_semantic_core_type(c, NL_TYPE_U8)},
        &value);
    return s == NL_CHECK_OK ? nl_fixed_change(c, child, value, false) : s;
}
NLCheckStatus nl_if_field_commit(NLCheckedFragment *f, NLCheckedNodeId id,
                                 NLSemanticContext *c)
{
    NLIfFieldProof *p = find(f, id);
    return p && p->ready ? structural_join(c, p->view.child)
                         : NL_CHECK_INTERNAL_ERROR;
}
NLCheckStatus nl_if_field_finish(NLCheckedFragment *f, NLCheckedNodeId id,
                                 const NLSemanticContext *c)
{
    NLIfFieldProof *p = find(f, id);
    if (!p || !p->ready)
        return NL_CHECK_OK;
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    p->view.result =
        n->result_count ? n->results[0] : (NLCheckedResult){.type = n->type};
    NLCheckStatus s = nl_sem_clone(c, &p->post);
    if (s == NL_CHECK_OK)
        s = nl_copy_state_anchor(c, &p->post_anchor);
    for (size_t a = 0; s == NL_CHECK_OK && a < 2; ++a)
        s = nl_copy_state_anchor(p->arms[a]->context, &p->arm_anchor[a]);
    p->view.post = p->post;
    p->issued = p->view;
    p->complete = s == NL_CHECK_OK;
    return s;
}
NLCheckStatus nl_if_field_seal(NLCheckedFragment *f)
{
    for (NLIfFieldProof *p = f->if_fields; p; p = p->next) {
        if (!p->complete)
            continue;
        for (size_t a = 0; a < 2; ++a) {
            const NLCheckedFragment *arm = p->arms[a];
            p->nodes[a] = malloc(arm->count * sizeof(*arm->nodes));
            if (!p->nodes[a])
                return NL_CHECK_OUT_OF_MEMORY;
            memcpy(p->nodes[a], arm->nodes, arm->count * sizeof(*arm->nodes));
            p->node_origin[a] = arm->nodes;
            p->count[a] = arm->count;
            p->arm_root[a] = arm->root;
        }
        p->parent_nodes = malloc(f->count * sizeof(*f->nodes));
        if (!p->parent_nodes)
            return NL_CHECK_OUT_OF_MEMORY;
        memcpy(p->parent_nodes, f->nodes, f->count * sizeof(*f->nodes));
        p->parent_count = f->count;
        p->parent_origin = f->nodes;
        p->parent_root = f->root;
        p->sealed = true;
    }
    return NL_CHECK_OK;
}
bool nl_checked_if_field_view(const NLCheckedFragment *f, NLCheckedNodeId id,
                              NLIfFieldView *out)
{
    const NLIfFieldProof *p = find(f, id);
    if (!p || !p->sealed || p->one_normal || !out)
        return false;
    *out = p->view;
    return true;
}
static NLIfFieldTrace *event_at(const NLIfFieldProof *p, size_t a, size_t index)
{
    if (!p || a >= 3 ||
        index >= (a == 2 ? p->replay.event_count : p->view.arms[a].event_count))
        return NULL;
    NLIfFieldTrace *e = p->first[a];
    while (e && index--)
        e = e->next;
    return e;
}
bool nl_checked_if_field_event(const NLCheckedFragment *f, NLCheckedNodeId id,
                               size_t a, size_t i, NLIfFieldEvent *out)
{
    const NLIfFieldProof *p = find(f, id);
    const NLIfFieldTrace *e = event_at(p, a, i);
    if (!p || !p->sealed || p->one_normal || a >= 2 || !e || !out)
        return false;
    *out = e->view;
    return true;
}
static bool same_view(const NLIfFieldView *a, const NLIfFieldView *b)
{
    return !memcmp(a, b, sizeof(*a));
}
static bool same_event(const NLIfFieldEvent *a, const NLIfFieldEvent *b)
{
    return !memcmp(a, b, sizeof(*a));
}
static bool intrinsic(NLSemanticValueView a, NLSemanticValueView b)
{
    return a.type == b.type && a.scalar_known == b.scalar_known &&
           a.scalar_value == b.scalar_value;
}
static bool field_at(const NLSemanticContext *c, const NLCheckedField *f)
{
    NLSemanticPlaceView a[3];
    if (!f->present || !f->dependency_compatible || f->index >= 2 ||
        !nl_copy_trio(c, f->parent, a) || !f->base ||
        f->base > c->binding_count ||
        c->bindings[f->base - 1].view.place != f->parent)
        return false;
    const NLSemanticPlaceView b = a[f->index + 1];
    return f->nominal == a[0].type && f->type == b.type &&
           f->child == a[0].fixed_fields[f->index] &&
           f->parent_incarnation == a[0].incarnation &&
           f->child_incarnation == b.incarnation &&
           f->parent_fact == a[0].current_fact &&
           f->child_fact == b.current_fact && f->old_value == b.current_value &&
           !f->payload_occurrence && !f->post_payload_occurrence;
}
typedef struct {
    const NLIfFieldProof *p;
    const NLCheckedFragment *f;
    const NLSemanticContext *world;
    bool terminal, returned;
    size_t arm, events, changes, depth;
    bool *seen;
    NLCheckedNodeId *bound;
    NLIfFieldTrace *next;
    NLSemanticPlaceView model[3];
    NLCheckStatus status;
} Replay;
static const NLCheckedNodeView *take(Replay *r, NLCheckedNodeId id)
{
    if (!id || id > r->f->count || r->seen[id])
        return NULL;
    r->seen[id] = true;
    return &r->f->nodes[id - 1];
}
static bool scalar(Replay *r, const NLCheckedNodeView *n)
{
    const NLTypeId u8 = nl_semantic_core_type(r->world, NL_TYPE_U8);
    return n && !n->terminates && n->type == u8 && n->result_count == 1 &&
           n->results[0].type == u8 &&
           nl_copy_plain(r->world, n->results[0].value, u8);
}
static bool same_parts(const NLSemanticContext *c, NLPlaceId root,
                       const NLSemanticPlaceView model[3])
{
    NLSemanticPlaceView actual[3];
    return nl_copy_trio(c, root, actual) &&
           !memcmp(actual, model, sizeof(actual));
}
static const NLIfFieldTrace *event(Replay *r, NLCheckedNodeId id, bool change)
{
    NLIfFieldTrace *e = r->next;
    if (!e || e->origin != r->f || e->stream_index != r->arm ||
        e->view.node != id || e->view.change != change ||
        !same_event(&e->view, &e->issued) || e->view.before != e->before ||
        e->view.after != e->after ||
        !nl_copy_state_matches(e->after, &e->post_anchor) ||
        (change && !nl_copy_state_matches(e->before, &e->pre_anchor)) ||
        !nl_copy_same_types(e->after, r->world) ||
        (change && !nl_copy_same_types(e->before, r->world)) ||
        nl_sem_validate(e->after) != NL_CHECK_OK ||
        (change && nl_sem_validate(e->before) != NL_CHECK_OK) ||
        !same_parts(change ? e->before : e->after, r->p->view.root, r->model))
        return NULL;
    r->next = e->next;
    ++r->events;
    if (change)
        ++r->changes;
    return e;
}
static bool walk(Replay *r, NLCheckedNodeId id, const NLCheckedNodeView *loan)
{
    if (++r->depth > NL_SEMANTIC_MAX_DEPTH) {
        --r->depth;
        return false;
    }
    const NLCheckedNodeView *n = take(r, id);
    bool ok = false;
    if (!n)
        goto done;
    const NLSemanticContext *c = r->world;
    switch (n->kind) {
    case NL_CHECKED_IF_ARM:
        ok = id == r->f->root && n->terminates == r->terminal &&
             walk(r, n->initializer, NULL);
        break;
    case NL_CHECKED_UNIT:
        ok = n->type == 1 && !n->result_count;
        break;
    case NL_CHECKED_U8_LITERAL:
        ok = scalar(r, n) && n->has_scalar_result && n->scalar_result.known &&
             n->scalar_result.value <= 255 &&
             c->values[n->results[0].value - 1].scalar_known &&
             c->values[n->results[0].value - 1].scalar_value ==
                 n->scalar_result.value;
        break;
    case NL_CHECKED_IDENTIFIER: {
        if (!scalar(r, n) || n->value_use != NL_VALUE_COPIED || !n->symbol ||
            n->symbol > c->binding_count)
            break;
        NLValueId origin = 0;
        if (n->symbol <= r->p->entry->binding_count)
            origin = r->p->entry->bindings[n->symbol - 1].view.value;
        else if (r->bound[n->symbol]) {
            const NLCheckedNodeView *b = &r->f->nodes[r->bound[n->symbol] - 1];
            const NLCheckedNodeView *v = &r->f->nodes[b->initializer - 1];
            if (v->result_count == 1)
                origin = v->results[0].value;
        }
        ok = origin && origin <= c->value_count &&
             intrinsic(c->values[origin - 1],
                       c->values[n->results[0].value - 1]);
        break;
    }
    case NL_CHECKED_FIELD_READ: {
        const NLIfFieldTrace *e = event(r, id, false);
        ok = scalar(r, n) && e && e->view.field.parent == r->p->view.root &&
             !memcmp(&n->field, &e->view.field, sizeof(n->field)) &&
             n->field.access == NL_ACCESS_READ &&
             field_at(e->after, &n->field) &&
             e->view.result == n->results[0].value &&
             e->view.result != n->field.old_value &&
             intrinsic(e->after->values[e->view.result - 1],
                       e->after->values[n->field.old_value - 1]);
        break;
    }
    case NL_CHECKED_BINDING: {
        if (n->argument_count != 1 || !n->symbol ||
            n->symbol > c->binding_count || r->bound[n->symbol] ||
            !walk(r, n->initializer, loan) ||
            !scalar(r, &r->f->nodes[n->initializer - 1]))
            break;
        const NLCheckedNodeView *recv = take(r, n->first_argument);
        ok = recv && recv->kind == NL_CHECKED_RECEIVER &&
             recv->symbol == n->symbol &&
             recv->type == r->f->nodes[n->initializer - 1].type &&
             !recv->next_argument;
        if (ok)
            r->bound[n->symbol] = id;
        break;
    }
    case NL_CHECKED_STATEMENT:
        ok = walk(r, n->initializer, loan);
        break;
    case NL_CHECKED_RETURN:
        ok = r->terminal && !r->returned &&
             id == r->p->return_view.return_node && n->terminates &&
             n->returned.type == 1 && !n->returned.value &&
             walk(r, n->initializer, NULL);
        if (ok)
            r->returned = true;
        break;
    case NL_CHECKED_BLOCK: {
        if (n->terminates && !r->terminal)
            break;
        size_t count = 0;
        ok = true;
        for (NLCheckedNodeId item = n->first_item; item;) {
            if (r->returned || !walk(r, item, loan)) {
                ok = false;
                break;
            }
            ++count;
            item = r->f->nodes[item - 1].next_item;
        }
        ok = ok && count == n->item_count &&
             (!n->tail || (!r->returned && walk(r, n->tail, loan)));
        break;
    }
    case NL_CHECKED_LOAN_HEADER: {
        const NLCheckedNodeView *body =
            nl_checked_node_view(r->f, n->initializer);
        const NLPlaceId child = n->field.child;
        ok =
            n->field.present && child == r->p->view.child &&
            n->field.parent == r->p->view.root &&
            n->field.index == r->p->view.index &&
            n->field.parent_fact == r->model[0].current_fact &&
            n->field.child_fact == r->model[n->field.index + 1].current_fact &&
            n->loan.implicit_local && n->loan.access == NL_ACCESS_WRITE &&
            !n->loan.is_exclusive && !n->loan.from_ptr &&
            n->loan.source == r->p->view.base && n->loan.ref_symbol &&
            n->loan.place == child &&
            n->loan.incarnation == r->p->view.child_incarnation &&
            n->loan.scope > r->p->entry->scope_count &&
            n->loan.scope <= c->scope_count &&
            !c->scopes[n->loan.scope - 1].active &&
            n->loan.body_nonescape_proved && n->loan.normal_result_forwarded &&
            body && body->kind == NL_CHECKED_BLOCK &&
            body->result_count == n->result_count && n->result_count <= 1 &&
            (!n->result_count ||
             (scalar(r, n) && n->results[0].value == body->results[0].value)) &&
            walk(r, n->initializer, n);
        break;
    }
    case NL_CHECKED_REPLACE: {
        if (!loan || !scalar(r, n) || n->argument_count != 2)
            break;
        const NLCheckedNodeView *ref = take(r, n->first_argument);
        if (!ref || ref->kind != NL_CHECKED_IDENTIFIER ||
            ref->symbol != loan->loan.ref_symbol ||
            ref->value_use != NL_VALUE_COPIED || ref->result_count != 1 ||
            !ref->next_argument || !walk(r, ref->next_argument, loan))
            break;
        const NLCheckedNodeView *incoming =
            &r->f->nodes[ref->next_argument - 1];
        const NLIfFieldTrace *e = event(r, id, true);
        if (!e || !scalar(r, incoming) || !field_at(e->before, &n->field) ||
            memcmp(&n->field, &e->view.field, sizeof(n->field)) ||
            n->field.parent != r->p->view.root ||
            n->field.child != r->p->view.child ||
            n->field.index != r->p->view.index ||
            e->view.result != n->field.old_value ||
            e->view.result != n->results[0].value ||
            e->view.incoming != incoming->results[0].value ||
            n->field.new_value != e->view.incoming ||
            e->view.incoming > e->before->value_count ||
            !intrinsic(e->before->values[e->view.incoming - 1],
                       c->values[incoming->results[0].value - 1]) ||
            !intrinsic(e->before->values[n->field.old_value - 1],
                       c->values[n->results[0].value - 1]) ||
            !ref->results[0].value ||
            ref->results[0].value > e->before->value_count)
            break;
        const NLSemanticValueView v =
            e->before->values[ref->results[0].value - 1];
        const NLSemanticTypeView t = e->before->types[v.type - 1].view;
        if (t.kind != NL_TYPE_REF || t.access != NL_ACCESS_WRITE ||
            t.is_exclusive || t.target != n->type || v.reference_count ||
            v.dependencies != NL_DEPENDENCY_FREE || v.value_dependency_count ||
            v.reference.place != n->field.child ||
            v.reference.incarnation != n->field.child_incarnation ||
            !v.reference.readable || !v.reference.writable ||
            v.reference.provenance != NL_PROVENANCE_VALID ||
            v.reference.occurrence_dependency ||
            v.reference.scope != loan->loan.scope ||
            loan->loan.scope > e->before->scope_count ||
            !e->before->scopes[loan->loan.scope - 1].active)
            break;
        NLSemanticContext *replay = NULL;
        r->status = nl_sem_clone(e->before, &replay);
        if (r->status == NL_CHECK_OK)
            r->status = nl_fixed_change_dependencies(replay, n->field.child, 0);
        if (r->status == NL_CHECK_OK)
            r->status = nl_fixed_change(replay, n->field.child,
                                        e->view.incoming, false);
        NLSemanticPlaceView parts[3];
        ok = r->status == NL_CHECK_OK &&
             nl_copy_trio(e->after, r->p->view.root, parts) &&
             same_parts(replay, r->p->view.root, parts) &&
             n->field.parent_post_fact == parts[0].current_fact &&
             n->field.child_post_fact == parts[n->field.index + 1].current_fact;
        if (ok)
            memcpy(r->model, parts, sizeof(parts));
        nl_semantic_destroy(replay);
        break;
    }
    default:
        break;
    }
done:
    --r->depth;
    return ok;
}
static NLCheckStatus validate_arm(const NLIfFieldProof *p, size_t a)
{
    const NLCheckedFragment *f = p->arms[a];
    if (!f || f->context != p->view.arms[a].world || f->if_parent != p->owner ||
        f->if_parent_node != p->view.conditional || f->if_arm_index != a ||
        f->source != p->source || f->root != p->arm_root[a] ||
        f->nodes != p->node_origin[a] || f->count != p->count[a] ||
        !p->nodes[a] || !f->nodes ||
        memcmp(f->nodes, p->nodes[a], f->count * sizeof(*f->nodes)) ||
        !nl_copy_state_matches(f->context, &p->arm_anchor[a]) ||
        !nl_copy_same_types(f->context, p->entry) ||
        nl_sem_validate(f->context) != NL_CHECK_OK || !frame(p, f->context))
        return INVALID;
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    if (!root || root->kind != NL_CHECKED_IF_ARM || root->terminates ||
        nl_control_exits_count(f->exits) ||
        nl_control_exits_count(f->loop_returns) || f->arm_count ||
        f->body_count || f->branch_value_prefix != p->entry->value_count ||
        f->branch_fact_prefix != p->entry->last_value_fact)
        return INVALID;
    Replay r = {
        .p = p, .f = f, .world = f->context, .arm = a, .next = p->first[a]};
    r.seen = calloc(f->count + 1, sizeof(*r.seen));
    r.bound = calloc(f->context->binding_count + 1, sizeof(*r.bound));
    NLCheckStatus s = NL_CHECK_OUT_OF_MEMORY;
    if (!r.seen || !r.bound)
        goto done;
    s = INVALID;
    if (!nl_copy_trio(p->entry, p->view.root, r.model) ||
        !walk(&r, f->root, NULL) || r.next ||
        r.events != p->view.arms[a].event_count ||
        r.changes != p->view.arms[a].change_count ||
        !same_parts(f->context, p->view.root, r.model))
        goto done;
    for (size_t i = 1; i <= f->count; ++i)
        if (!r.seen[i])
            goto done;
    if (root->type != p->view.result.type ||
        root->result_count != (p->view.result.value ? 1 : 0))
        goto done;
    if (root->result_count && !scalar(&r, root))
        goto done;
    s = NL_CHECK_OK;
done:
    if (r.status != NL_CHECK_OK)
        s = r.status;
    free(r.seen);
    free(r.bound);
    return s;
}
/* Verify direct, subsequent reads until another source operation may change
 * memory. The immutable caller graph authenticates order and source origin;
 * the immediate join checkpoint supplies the exact current facts/packages. */
static bool readback(const NLIfFieldProof *p)
{
    const NLCheckedFragment *f = p->owner;
    const NLCheckedNodeView *choice =
        nl_checked_node_view(f, p->view.conditional);
    size_t stop = SIZE_MAX;
    for (size_t i = 0; i < f->count; ++i) {
        const NLCheckedNodeView *n = &f->nodes[i];
        if (n->span.start_byte >= choice->span.end_byte &&
            (n->kind == NL_CHECKED_REPLACE ||
             n->kind == NL_CHECKED_REGISTERED_CALL ||
             n->kind == NL_CHECKED_IF) &&
            n->span.start_byte < stop)
            stop = n->span.start_byte;
    }
    for (size_t i = 0; i < f->count; ++i) {
        const NLCheckedNodeView *n = &f->nodes[i];
        if (n->kind != NL_CHECKED_FIELD_READ ||
            n->field.parent != p->view.root ||
            n->span.start_byte < choice->span.end_byte ||
            n->span.start_byte >= stop)
            continue;
        if (!field_at(p->post, &n->field) ||
            n->field.access != NL_ACCESS_READ || n->result_count != 1 ||
            !nl_copy_plain(f->context, n->results[0].value, n->type) ||
            !intrinsic(f->context->values[n->results[0].value - 1],
                       p->post->values[n->field.old_value - 1]))
            return false;
    }
    if (p->view.result.value &&
        (!nl_copy_plain(f->context, p->view.result.value,
                        p->view.result.type) ||
         !intrinsic(f->context->values[p->view.result.value - 1],
                    p->post->values[p->view.result.value - 1])))
        return false;
    return true;
}
NLCheckStatus nl_checked_if_field_validate(const NLCheckedFragment *f,
                                           NLCheckedNodeId id,
                                           const NLIfFieldView *candidate)
{
    const NLIfFieldProof *p = find(f, id);
    if (!p || p->one_normal || !p->sealed || !p->complete || !p->ready ||
        !same_view(&p->view, &p->issued) ||
        (candidate && !same_view(candidate, &p->issued)) ||
        f->source != p->source || f->body_owner != p->body ||
        f->nodes != p->parent_origin || !f->nodes ||
        f->root != p->parent_root || f->count != p->parent_count ||
        !p->parent_nodes ||
        memcmp(f->nodes, p->parent_nodes, f->count * sizeof(*f->nodes)) ||
        p->entry != p->view.entry || p->post != p->view.post ||
        !nl_copy_state_matches(p->entry, &p->entry_anchor) ||
        !nl_copy_state_matches(p->post, &p->post_anchor) ||
        nl_sem_validate(p->entry) != NL_CHECK_OK ||
        nl_sem_validate(p->post) != NL_CHECK_OK ||
        !nl_copy_same_types(p->entry, p->post) ||
        !nl_copy_same_types(f->context, p->post) ||
        nl_sem_validate(f->context) != NL_CHECK_OK)
        return INVALID;
    const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
    const NLCheckedNodeView *cond =
        choice ? nl_checked_node_view(f, choice->initializer) : NULL;
    if (!choice || choice->kind != NL_CHECKED_IF || choice->normal_arms != 2 ||
        choice->terminates || choice->initializer != p->view.condition ||
        !cond ||
        cond->type != nl_semantic_core_type(f->context, NL_TYPE_BOOL) ||
        cond->result_count != 1 || choice->type != p->view.result.type ||
        choice->result_count != (p->view.result.value ? 1 : 0) ||
        (choice->result_count &&
         choice->results[0].value != p->view.result.value))
        return INVALID;
    for (size_t a = 0; a < 2; ++a) {
        if (nl_checked_if_arm(f, id, a) != p->arms[a])
            return INVALID;
        NLCheckStatus s = validate_arm(p, a);
        if (s != NL_CHECK_OK)
            return s;
    }
    if ((!p->view.arms[0].change_count && !p->view.arms[1].change_count) ||
        !readback(p))
        return INVALID;
    NLSemanticContext *replay = NULL;
    NLCheckStatus s = nl_sem_clone(p->entry, &replay);
    if (s == NL_CHECK_OK)
        s = structural_join(replay, p->view.child);
    if (s == NL_CHECK_OK) {
        for (size_t i = 0; i < replay->binding_count; ++i)
            if (replay->bindings[i].view.place == p->view.root &&
                replay->bindings[i].view.availability == NL_AVAILABLE)
                replay->bindings[i].view.value =
                    replay->places[p->view.root - 1].current_value;
        if (p->view.result.value) {
            NLValueId output = 0;
            s = nl_sem_new_value(
                replay, (NLSemanticValueView){.type = p->view.result.type},
                &output);
            if (s == NL_CHECK_OK && output != p->view.result.value)
                s = INVALID;
        }
    }
    if (s == NL_CHECK_OK && !nl_copy_state_matches(replay, &p->post_anchor))
        s = INVALID;
    nl_semantic_destroy(replay);
    return s;
}
NLCheckStatus nl_checked_if_field_event_validate(const NLCheckedFragment *f,
                                                 NLCheckedNodeId id, size_t a,
                                                 size_t i,
                                                 const NLIfFieldEvent *v)
{
    const NLIfFieldProof *p = find(f, id);
    const NLIfFieldTrace *e = event_at(p, a, i);
    return !e || (v && !same_event(v, &e->issued))
               ? INVALID
               : nl_checked_if_field_validate(f, id, NULL);
}

NLCheckStatus nl_if_return_before(NLCheckedFragment *f,
                                  const NLSemanticContext *c, size_t bindings,
                                  size_t places, size_t scopes)
{
    NLIfFieldProof *p = parent(f);
    if (!p || f == p->owner || p->return_live)
        return NL_CHECK_OK;
    p->binding_floor = bindings;
    p->place_floor = places;
    p->scope_floor = scopes;
    p->return_arm = f->if_arm_index;
    NLCheckStatus s = nl_sem_clone(c, &p->return_live);
    if (s == NL_CHECK_OK)
        s = nl_copy_state_anchor(c, &p->return_anchor);
    return s;
}
NLCheckStatus nl_if_return_after(NLCheckedFragment *f, NLCheckedNodeId id,
                                 const NLSemanticContext *c)
{
    NLIfFieldProof *p = parent(f);
    if (!p || !p->return_live || f == p->owner ||
        f->if_arm_index != p->return_arm || p->return_view.return_node)
        return NL_CHECK_OK;
    const NLControlExitView *edge = nl_control_exit_view(f->exits, 0);
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (!edge || !n)
        return NL_CHECK_INTERNAL_ERROR;
    p->exit_target = edge->target;
    p->exit_state = edge->state;
    p->return_view.return_node = id;
    p->return_view.return_live = p->return_live;
    p->return_view.return_ended = c;
    p->return_view.returned = n->returned;
    return nl_copy_state_anchor(c, &p->exit_anchor);
}
NLCheckStatus nl_if_return_begin(NLCheckedFragment *f, NLCheckedNodeId id,
                                 size_t normal)
{
    NLIfFieldProof *p = find(f, id);
    if (!p)
        return NL_CHECK_OK;
    p->one_normal = true;
    p->replaying = true;
    p->return_view.normal_arm = normal;
    p->replay.artifact = f;
    p->replay.world = f->context;
    return NL_CHECK_OK;
}
NLCheckStatus nl_if_return_finish(NLCheckedFragment *f, NLCheckedNodeId id,
                                  const NLSemanticContext *c)
{
    NLIfFieldProof *p = find(f, id);
    if (!p)
        return NL_CHECK_OK;
    p->replaying = false;
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    size_t normal = p->return_view.normal_arm;
    if (!p->return_live || normal > 1 || normal == p->return_arm ||
        !p->return_view.return_node || !p->replay.change_count ||
        n->normal_arms != 1 || !n->tail)
        return NL_CHECK_OK;
    NLPlaceId root = 0, child = 0;
    for (size_t a = 0; a < 3; ++a)
        for (NLIfFieldTrace *e = p->first[a]; e; e = e->next)
            if (e->view.change) {
                if (root && (root != e->view.field.parent ||
                             child != e->view.field.child))
                    return NL_CHECK_OK;
                root = e->view.field.parent;
                child = e->view.field.child;
            }
    NLSemanticPlaceView parts[3];
    if (!root || !child || child > p->entry->place_count ||
        !nl_copy_trio(p->entry, root, parts))
        return NL_CHECK_OK;
    size_t index = p->entry->places[child - 1].parent_field_index;
    if (index > 1 || parts[0].fixed_fields[index] != child)
        return NL_CHECK_OK;
    p->view.root = root;
    p->view.child = child;
    p->view.sibling = parts[0].fixed_fields[1 - index];
    p->view.index = index;
    p->view.incarnation = parts[0].incarnation;
    p->view.child_incarnation = parts[index + 1].incarnation;
    p->view.nominal = parts[0].type;
    for (size_t j = 0; j < p->entry->binding_count; ++j)
        if (!p->entry->bindings[j].hidden &&
            p->entry->bindings[j].view.place == root)
            p->view.base = j + 1;
    if (!p->view.base || !frame(p, c) || !frame(p, p->arms[normal]->context) ||
        !frame_state(p, p->return_live, true))
        return NL_CHECK_OK;
    p->ready = true;
    NLCheckStatus s = nl_if_field_finish(f, id, c);
    p->return_view.field = p->view;
    p->return_view.replay_root = n->tail;
    p->return_view.replay_events = p->replay.event_count;
    p->return_view.replay_changes = p->replay.change_count;
    p->return_issued = p->return_view;
    return s;
}
bool nl_checked_if_return_view(const NLCheckedFragment *f, NLCheckedNodeId id,
                               NLIfReturnView *out)
{
    const NLIfFieldProof *p = find(f, id);
    if (!p || !p->one_normal || !p->sealed || !p->complete || !out)
        return false;
    *out = p->return_view;
    return true;
}
bool nl_checked_if_return_event(const NLCheckedFragment *f, NLCheckedNodeId id,
                                size_t a, size_t index, NLIfFieldEvent *out)
{
    const NLIfFieldProof *p = find(f, id);
    const NLIfFieldTrace *e = event_at(p, a, index);
    if (!p || !p->one_normal || !p->sealed || !e || !out)
        return false;
    *out = e->view;
    return true;
}
/* Strict terminal subset of the existing end_bindings rule. All references
 * are already ended and all scopes closed; no heap/sum/affine teardown is
 * inferred. Use core dependency/detach/value-exit operations, not byte
 * equality. */
static NLCheckStatus replay_return_exit(const NLIfFieldProof *p)
{
    if (p->binding_floor || p->place_floor || p->scope_floor ||
        p->return_live->domain_count || p->return_live->region_count ||
        p->return_live->occurrence_count)
        return INVALID;
    NLSemanticContext *c = NULL;
    NLCheckStatus s = nl_sem_clone(p->return_live, &c);
    if (s != NL_CHECK_OK)
        return s;
    for (size_t i = 0; i < c->scope_count; ++i)
        if (c->scopes[i].active) {
            s = INVALID;
            goto done;
        }
    for (size_t i = 0; i < c->value_count; ++i)
        if (c->values[i].carrier != NL_CARRIER_ENDED &&
            (c->types[c->values[i].type - 1].view.kind == NL_TYPE_REF ||
             c->values[i].dependencies != NL_DEPENDENCY_FREE)) {
            s = INVALID;
            goto done;
        }
    for (size_t i = 0; i < c->binding_count; ++i) {
        NLBindingEntry *b = &c->bindings[i];
        if (b->hidden)
            continue;
        if (b->view.availability == NL_AVAILABLE) {
            const NLSemanticTypeView t = c->types[b->view.type - 1].view;
            if (!t.is_copy || !t.is_discardable ||
                (t.kind != NL_TYPE_U8 && t.kind != NL_TYPE_USIZE &&
                 t.kind != NL_TYPE_BOOL && !nl_fixed_type(c, b->view.type))) {
                s = INVALID;
                goto done;
            }
            s = nl_fixed_end_dependencies(c, b->view.place, 0);
            if (s != NL_CHECK_OK)
                goto done;
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
    s = nl_sem_function_exit(c, 0, 0);
    if (s == NL_CHECK_OK && !nl_copy_state_matches(c, &p->exit_anchor))
        s = INVALID;
done:
    nl_semantic_destroy(c);
    return s;
}
static NLCheckStatus return_track(const NLIfFieldProof *p, size_t a)
{
    const bool common = a == 2, terminal = a == p->return_arm;
    const NLCheckedFragment *f = common ? p->owner : p->arms[a];
    const NLSemanticContext *world = common     ? p->post
                                     : terminal ? p->return_live
                                                : f->context;
    const NLIfFieldArm *counts = common ? &p->replay : &p->view.arms[a];
    const NLCheckedNodeId root_id =
        common ? p->return_view.replay_root : f->root;
    const NLCheckedNodeView *root = nl_checked_node_view(f, root_id);
    if (!root ||
        (!common &&
         (root->kind != NL_CHECKED_IF_ARM || root->terminates != terminal ||
          f->if_parent != p->owner ||
          f->if_parent_node != p->view.conditional || f->if_arm_index != a ||
          f->source != p->source || f->root != p->arm_root[a] ||
          f->nodes != p->node_origin[a] || f->count != p->count[a] ||
          memcmp(f->nodes, p->nodes[a], f->count * sizeof(*f->nodes)) ||
          !nl_copy_state_matches(f->context, &p->arm_anchor[a]) ||
          f->branch_value_prefix != p->entry->value_count ||
          f->branch_fact_prefix != p->entry->last_value_fact ||
          nl_control_exits_count(f->exits) != (terminal ? 1U : 0U) ||
          nl_control_exits_count(f->loop_returns) || f->arm_count ||
          f->body_count)))
        return INVALID;
    if (!nl_copy_same_types(world, p->entry) ||
        nl_sem_validate(world) != NL_CHECK_OK ||
        !frame_state(p, world, terminal))
        return INVALID;
    Replay r = {.p = p,
                .f = f,
                .world = world,
                .arm = a,
                .terminal = terminal,
                .next = p->first[a]};
    r.seen = calloc(f->count + 1, sizeof(*r.seen));
    r.bound = calloc(world->binding_count + 1, sizeof(*r.bound));
    NLCheckStatus s = NL_CHECK_OUT_OF_MEMORY;
    if (!r.seen || !r.bound)
        goto done;
    s = INVALID;
    if (!nl_copy_trio(p->entry, p->view.root, r.model) ||
        !walk(&r, root_id, NULL) || r.next || r.events != counts->event_count ||
        r.changes != counts->change_count || r.returned != terminal ||
        !same_parts(world, p->view.root, r.model))
        goto done;
    for (size_t i = 1; i <= f->count; ++i) {
        const NLCheckedNodeView *node = &f->nodes[i - 1];
        bool included =
            !common || (node->span.start_byte >= root->span.start_byte &&
                        node->span.end_byte <= root->span.end_byte);
        if (r.seen[i] != included)
            goto done;
    }
    if (terminal) {
        if (root->returned.type != 1 || root->returned.value)
            goto done;
    } else if (root->type != p->view.result.type ||
               root->result_count != (p->view.result.value ? 1U : 0U) ||
               (root->result_count && !scalar(&r, root)))
        goto done;
    s = NL_CHECK_OK;
done:
    if (r.status != NL_CHECK_OK)
        s = r.status;
    free(r.seen);
    free(r.bound);
    return s;
}
/* Correlate the independently issued normal graphs by ordered checked source
 * nodes, not by post-fork numeric ValueID equality. Each graph has already
 * passed its own world-qualified operation replay. */
static bool same_normal_source(const NLIfFieldProof *p)
{
    const NLCheckedFragment *a = p->arms[p->return_view.normal_arm];
    const NLCheckedFragment *b = p->owner;
    const NLCheckedNodeView *body =
        nl_checked_node_view(b, p->return_view.replay_root);
    size_t cursor = 0, matched = 0;
    for (size_t i = 0; i < b->count; ++i) {
        const NLCheckedNodeView *right = &b->nodes[i];
        if (right->span.start_byte < body->span.start_byte ||
            right->span.end_byte > body->span.end_byte)
            continue;
        while (cursor < a->count && cursor + 1 == a->root)
            ++cursor;
        if (cursor == a->count)
            return false;
        const NLCheckedNodeView *left = &a->nodes[cursor++];
        if (left->kind != right->kind || left->type != right->type ||
            memcmp(&left->span, &right->span, sizeof(left->span)) ||
            memcmp(&left->name, &right->name, sizeof(left->name)) ||
            left->terminates != right->terminates ||
            left->result_count != right->result_count ||
            left->field.present != right->field.present ||
            (left->field.present &&
             (left->field.parent != right->field.parent ||
              left->field.child != right->field.child ||
              left->field.index != right->field.index ||
              left->field.parent_incarnation !=
                  right->field.parent_incarnation ||
              left->field.child_incarnation != right->field.child_incarnation)))
            return false;
        if (left->result_count &&
            !intrinsic(a->context->values[left->results[0].value - 1],
                       p->post->values[right->results[0].value - 1]))
            return false;
        ++matched;
    }
    return matched + 1 == a->count;
}
NLCheckStatus nl_checked_if_return_validate(const NLCheckedFragment *f,
                                            NLCheckedNodeId id,
                                            const NLIfReturnView *candidate)
{
    const NLIfFieldProof *p = find(f, id);
    if (!p || !p->one_normal || !p->ready || !p->complete || !p->sealed ||
        memcmp(&p->return_view, &p->return_issued, sizeof(p->return_view)) ||
        (candidate &&
         memcmp(candidate, &p->return_issued, sizeof(*candidate))) ||
        !same_view(&p->view, &p->issued) ||
        !same_view(&p->view, &p->return_view.field) || f->source != p->source ||
        f->body_owner != p->body || !p->body || p->body->count ||
        f->nodes != p->parent_origin || !f->nodes ||
        f->root != p->parent_root || f->count != p->parent_count ||
        !p->parent_nodes ||
        memcmp(f->nodes, p->parent_nodes, f->count * sizeof(*f->nodes)) ||
        !nl_copy_state_matches(p->entry, &p->entry_anchor) ||
        !nl_copy_state_matches(p->post, &p->post_anchor) ||
        !nl_copy_state_matches(p->return_live, &p->return_anchor) ||
        nl_sem_validate(f->context) != NL_CHECK_OK ||
        nl_sem_validate(p->entry) != NL_CHECK_OK ||
        nl_sem_validate(p->post) != NL_CHECK_OK ||
        !nl_copy_same_types(f->context, p->post) ||
        !nl_copy_same_types(p->return_live, p->entry))
        return INVALID;
    const NLIfReturnView *v = &p->return_view;
    if (v->normal_arm > 1 || p->return_arm != 1 - v->normal_arm ||
        p->entry != v->field.entry || p->post != v->field.post ||
        p->return_live != v->return_live ||
        v->return_ended != p->arms[p->return_arm]->context || p->replaying ||
        !p->replay.change_count || v->replay_events != p->replay.event_count ||
        v->replay_changes != p->replay.change_count)
        return INVALID;
    const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
    const NLCheckedNodeView *cond =
        choice ? nl_checked_node_view(f, choice->initializer) : NULL;
    if (!choice || choice->kind != NL_CHECKED_IF || choice->normal_arms != 1 ||
        choice->terminates || choice->tail != v->replay_root ||
        choice->initializer != p->view.condition ||
        choice->type != p->view.result.type ||
        choice->result_count != (p->view.result.value ? 1U : 0U) ||
        (choice->result_count &&
         choice->results[0].value != p->view.result.value) ||
        !cond ||
        cond->type != nl_semantic_core_type(f->context, NL_TYPE_BOOL) ||
        cond->result_count != 1)
        return INVALID;
    for (size_t a = 0; a < 2; ++a)
        if (nl_checked_if_arm(f, id, a) != p->arms[a])
            return INVALID;
    for (size_t a = 0; a < 3; ++a) {
        NLCheckStatus s = return_track(p, a);
        if (s != NL_CHECK_OK)
            return s;
    }
    const NLControlExitView *edge =
        nl_control_exit_view(p->arms[p->return_arm]->exits, 0);
    const NLControlExitView *outer = nl_control_exit_view(f->exits, 0);
    if (!edge || !outer || nl_control_exits_count(f->exits) != 1 ||
        edge->kind != NL_EXIT_RETURN || outer->kind != NL_EXIT_RETURN ||
        edge->target != p->exit_target || outer->target != p->exit_target ||
        edge->state != p->exit_state || edge->count != 1 || outer->count != 1 ||
        edge->values[0].type != 1 || edge->values[0].value ||
        outer->values[0].type != 1 || outer->values[0].value ||
        v->returned.type != 1 || v->returned.value ||
        !nl_copy_state_matches(v->return_ended, &p->exit_anchor) ||
        !nl_copy_state_matches(
            nl_control_state_context((NLControlState *)edge->state),
            &p->exit_anchor) ||
        !nl_copy_state_matches(
            nl_control_state_context((NLControlState *)outer->state),
            &p->exit_anchor) ||
        nl_sem_validate(v->return_ended) != NL_CHECK_OK || !readback(p) ||
        !same_normal_source(p))
        return INVALID;
    NLCheckStatus s = replay_return_exit(p);
    if (s != NL_CHECK_OK)
        return s;
    /* Same source is genuinely rechecked in the common world. Exact state
     * agreement is only a consistency check after both world-qualified walks,
     * never permission to import an arm-private package. */
    if (!nl_copy_state_matches(p->post, &p->arm_anchor[v->normal_arm]))
        return INVALID;
    const NLCheckedNodeView *arm = nl_checked_node_view(
        p->arms[v->normal_arm], p->arms[v->normal_arm]->root);
    if (arm->result_count &&
        (arm->results[0].value != p->view.result.value ||
         !intrinsic(
             p->arms[v->normal_arm]->context->values[arm->results[0].value - 1],
             p->post->values[p->view.result.value - 1])))
        return INVALID;
    return NL_CHECK_OK;
}
NLCheckStatus
nl_checked_if_return_event_validate(const NLCheckedFragment *f,
                                    NLCheckedNodeId id, size_t a, size_t index,
                                    const NLIfFieldEvent *candidate)
{
    const NLIfFieldTrace *e = event_at(find(f, id), a, index);
    return !e || (candidate && !same_event(candidate, &e->issued))
               ? INVALID
               : nl_checked_if_return_validate(f, id, NULL);
}
