#include "h_origin_internal.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    NLCheckedFragment *child;
    const NLSemanticContext *world;
    const NLSource *source;
    NLCheckedNodeId root, grant;
    NLCheckedNodeView arm_view, grant_view;
    size_t slot;
    bool checked, owned;
} HEdge;
typedef struct {
    NLCheckedFragment *parent;
    const NLSemanticContext *world_owner;
    const NLSource *source;
    NLCheckedNodeId match, trial;
    NLCheckedNodeView match_view, trial_view;
    /* The actual pre-grant clone event's finite source cut. These counters
     * describe that event; they are never an independently mintable world. */
    size_t bindings, values, places, regions, domains, scopes;
    NLIncarnationId incarnation;
    NLValueFactId fact;
    HEdge edges[2];
    bool complete, released;
} HSite;
struct NLHOriginOwner {
    NLCheckedFragment *root, *invocation;
    const NLSemanticContext *registration;
    NLFunctionBody *plan; /* retained once, without a graph ownership cycle */
    const NLSource *source;
    NLCheckedNodeId call;
    size_t body_slot;
    NLCheckedNodeView call_view;
    HSite sites[2];
    size_t count;
    bool body_owned, closed, aborted;
};
static HSite *site(struct NLHOriginOwner *o, const NLCheckedFragment *parent,
                   NLCheckedNodeId match)
{
    if (o)
        for (size_t i = 0; i < o->count; ++i)
            if (o->sites[i].parent == parent && o->sites[i].match == match)
                return &o->sites[i];
    return NULL;
}
NLCheckStatus nl_h_origin_site(NLCheckedFragment *root,
                               NLCheckedFragment *invocation,
                               NLCheckedNodeId call, NLCheckedFragment *parent,
                               NLCheckedNodeId match, NLCheckedNodeId trial,
                               const NLSemanticContext *cut)
{
    if (!root || !invocation || !invocation->body_owner || !parent || !cut)
        return NL_CHECK_INTERNAL_ERROR;
    struct NLHOriginOwner *o = root->h_origin;
    if (!o) {
        o = calloc(1, sizeof(*o));
        if (!o)
            return NL_CHECK_OUT_OF_MEMORY;
        NLCheckStatus s = nl_body_retain(invocation->body_owner);
        if (s != NL_CHECK_OK) {
            free(o);
            return s;
        }
        o->root = root;
        o->registration = root->context;
        o->invocation = invocation;
        o->plan = invocation->body_owner;
        o->source = invocation->source;
        o->call = call;
        root->h_origin = o;
        invocation->h_origin = o;
    }
    /* A non-single-entry composition remains CHECK-valid but cannot produce
     * this bounded backend witness. Never import its additional graph. */
    if (o->invocation != invocation || o->count == 2) {
        o->aborted = true;
        return NL_CHECK_OK;
    }
    HSite *p = &o->sites[o->count++];
    *p = (HSite){.parent = parent,
                 .world_owner = parent->context,
                 .source = parent->source,
                 .match = match,
                 .trial = trial,
                 .trial_view = *nl_checked_node_view(parent, trial),
                 .bindings = cut->binding_count,
                 .values = cut->value_count,
                 .places = cut->place_count,
                 .regions = cut->region_count,
                 .domains = cut->domain_count,
                 .scopes = cut->scope_count,
                 .incarnation = cut->last_incarnation,
                 .fact = cut->last_value_fact};
    parent->h_origin = o;
    return NL_CHECK_OK;
}
void nl_h_origin_fork(NLCheckedFragment *parent, NLCheckedNodeId match,
                      size_t ordinal, NLCheckedFragment *child)
{
    HSite *p = site(parent->h_origin, parent, match);
    if (!p || ordinal > 1)
        return;
    p->edges[ordinal] = (HEdge){
        .child = child, .world = child->context, .source = child->source};
    child->h_origin = parent->h_origin;
}
void nl_h_origin_arm(NLCheckedFragment *parent, NLCheckedNodeId match,
                     size_t ordinal, NLCheckedFragment *child)
{
    HSite *p = site(parent->h_origin, parent, match);
    if (!p || ordinal > 1)
        return;
    HEdge *a = &p->edges[ordinal];
    if (a->child != child || a->world != child->context)
        return;
    a->root = child->root;
    a->arm_view = *nl_checked_node_view(child, a->root);
    a->grant = a->arm_view.initializer;
    a->grant_view = *nl_checked_node_view(child, a->grant);
    a->checked = true;
}
void nl_h_origin_take_arm(NLCheckedFragment *parent, NLCheckedNodeId match,
                          size_t ordinal, size_t slot)
{
    HSite *p = site(parent->h_origin, parent, match);
    if (p && ordinal < 2 && p->edges[ordinal].checked) {
        p->edges[ordinal].slot = slot;
        p->edges[ordinal].owned = true;
    }
}
void nl_h_origin_finish_site(NLCheckedFragment *parent, NLCheckedNodeId match)
{
    HSite *p = site(parent->h_origin, parent, match);
    if (p) {
        p->match_view = *nl_checked_node_view(parent, match);
        p->complete = p->edges[0].owned && p->edges[1].owned;
    }
}
void nl_h_origin_take_body(NLCheckedFragment *parent, NLCheckedFragment *body,
                           NLCheckedNodeId call, size_t slot)
{
    struct NLHOriginOwner *o = parent->h_origin;
    if (o && o->root == parent && o->invocation == body && o->call == call) {
        o->body_slot = slot;
        o->call_view = *nl_checked_node_view(parent, call);
        o->body_owned = true;
    }
}
void nl_h_origin_close(NLCheckedFragment *root)
{
    struct NLHOriginOwner *o = root->h_origin;
    if (o && o->root == root && o->body_owned && !o->aborted &&
        root->root == o->call) {
        bool complete = o->count > 0;
        for (size_t i = 0; i < o->count; ++i)
            complete &= o->sites[i].complete && !o->sites[i].released;
        o->closed = complete;
    }
}
static bool same_node(const NLCheckedFragment *f, NLCheckedNodeId id,
                      const NLCheckedNodeView *saved)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, id);
    return v && !memcmp(v, saved, sizeof(*v));
}
bool nl_h_origin_validate(const NLCheckedFragment *root)
{
    const struct NLHOriginOwner *o = root ? root->h_origin : NULL;
    if (!o || o->root != root || !o->closed || o->aborted ||
        root->context != o->registration || root->root != o->call ||
        !o->body_owned || o->body_slot >= root->body_count ||
        root->bodies[o->body_slot] != o->invocation ||
        root->body_calls[o->body_slot] != o->call ||
        !same_node(root, o->call, &o->call_view))
        return false;
    /* Authenticate the whole expected ancestry first, before dereferencing
     * any child endpoint selected by the mutable view. */
    for (size_t i = 0; i < o->count; ++i) {
        const HSite *p = &o->sites[i];
        const NLCheckedFragment *expected =
            i == 0 ? o->invocation : o->sites[0].edges[1].child;
        if (!p->complete || p->released || p->parent != expected)
            return false;
        for (size_t j = 0; j < 2; ++j) {
            const HEdge *a = &p->edges[j];
            if (!a->owned || !a->checked || a->slot >= expected->arm_count)
                return false;
            const NLCheckedArm *view = &expected->arms[a->slot];
            if (view->match != p->match || view->artifact != a->child ||
                view->context_origin != a->world ||
                nl_checked_match_arm(expected, p->match, j) != a->child)
                return false;
        }
    }
    if (o->invocation->body_owner != o->plan ||
        o->invocation->source != o->source || o->plan->source != o->source)
        return false;
    for (size_t i = 0; i < o->count; ++i) {
        const HSite *p = &o->sites[i];
        if (p->parent->context != p->world_owner ||
            p->parent->source != p->source || p->source != o->source ||
            !same_node(p->parent, p->trial, &p->trial_view) ||
            !same_node(p->parent, p->match, &p->match_view) ||
            p->match_view.kind != NL_CHECKED_MATCH ||
            p->match_view.initializer != p->trial ||
            p->trial_view.kind != NL_CHECKED_TRY_ALLOCATE_ONE)
            return false;
        for (size_t j = 0; j < 2; ++j) {
            const HEdge *a = &p->edges[j];
            if (a->child->context != a->world ||
                a->child->source != a->source || a->source != p->source ||
                a->child->root != a->root ||
                !same_node(a->child, a->root, &a->arm_view) ||
                !same_node(a->child, a->grant, &a->grant_view) ||
                a->arm_view.kind != NL_CHECKED_MATCH_ARM ||
                a->arm_view.variant != j + 1 ||
                a->grant_view.allocation_success != (j == 1))
                return false;
        }
    }
    return true;
}
bool nl_h_origin_edge(const NLCheckedFragment *root,
                      const NLCheckedFragment *parent, NLCheckedNodeId match,
                      size_t ordinal, const NLCheckedFragment *child)
{
    if (ordinal > 1 || !nl_h_origin_validate(root))
        return false;
    HSite *p = site(root->h_origin, parent, match);
    return p && p->edges[ordinal].child == child;
}
NLCheckedFragment *nl_h_origin_owned_child(NLCheckedFragment *parent,
                                           size_t slot, bool body)
{
    struct NLHOriginOwner *o = parent->h_origin;
    if (o) {
        if (body && o->root == parent && o->body_owned && o->body_slot == slot)
            return o->invocation;
        if (!body)
            for (size_t i = 0; i < o->count; ++i) {
                HSite *p = &o->sites[i];
                if (p->parent != parent || p->released)
                    continue;
                for (size_t j = 0; j < 2; ++j)
                    if (p->edges[j].owned && p->edges[j].slot == slot)
                        return p->edges[j].child;
            }
    }
    return body ? parent->bodies[slot] : parent->arms[slot].artifact;
}
void nl_h_origin_release_fragment(NLCheckedFragment *f)
{
    struct NLHOriginOwner *o = f->h_origin;
    if (!o)
        return;
    /* No dereferences of child/world/parent endpoints during bookkeeping:
     * earlier local failure cleanup may already have released those objects. */
    if (f == o->root) {
        nl_body_release(o->plan);
        free(o);
    } else {
        for (size_t i = 0; i < o->count; ++i)
            if (o->sites[i].parent == f)
                o->sites[i].released = true;
        o->closed = false;
        o->aborted = true;
    }
    f->h_origin = NULL;
}
