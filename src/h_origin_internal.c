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
typedef enum {
    PENDING,
    COMMITTED,
    DETACHED
} CustodyState;
typedef struct CustodyEdge {
    struct CustodyEdge *next;
    NLCheckedFragment *child;
    const NLSemanticContext *world;
    NLCustodyRole role;
    NLCheckedNodeId site;
    size_t slot;
    CustodyState state;
} CustodyEdge;
typedef struct CustodyParent {
    struct CustodyParent *next;
    NLCheckedFragment *fragment;
    const NLSemanticContext *world;
    const NLSource *source;
    CustodyEdge *edges;
    NLCheckedArm *arms;
    NLCheckedFragment **bodies;
    NLCheckedNodeId *calls;
    size_t arm_count, body_count, arm_capacity, body_capacity, call_capacity;
} CustodyParent;
struct NLHOriginOwner {
    NLCheckedFragment *root, *invocation;
    CustodyParent *parents;
    const NLSemanticContext *registration;
    NLFunctionBody *plan; /* retained once, without a graph ownership cycle */
    const NLSource *source;
    NLCheckedNodeId call;
    size_t body_slot;
    NLCheckedNodeView call_view;
    HSite sites[2];
    size_t count;
    bool body_owned, closed, aborted, published;
};
static CustodyParent *custody_parent(const struct NLHOriginOwner *o,
                                     const NLCheckedFragment *f)
{
    if (o)
        for (CustodyParent *p = o->parents; p; p = p->next)
            if (p->fragment == f)
                return p;
    return NULL;
}
const NLCheckedFragment *
nl_h_custody_observe_body(const NLCheckedFragment *parent, NLCheckedNodeId call)
{
    const struct NLHOriginOwner *o = parent ? parent->h_origin : NULL;
    const CustodyParent *p = custody_parent(o, parent);
    if (!p || !custody_parent(o, o->root))
        return NULL;
    /* Do not inspect mutable parent nodes/world/source or body tables. This
     * borrows already owned history and does not authenticate present access.
     * Pending children remain local; detached children may already be dead. */
    for (const CustodyEdge *e = p->edges; e; e = e->next)
        if (e->state == COMMITTED && e->role == NL_CUSTODY_BODY &&
            e->site == call && custody_parent(o, e->child))
            return e->child;
    return NULL;
}
NLCheckStatus nl_h_custody_attach(NLCheckedFragment *parent,
                                  NLCheckedFragment *child)
{
    if (!child || child->h_origin)
        return NL_CHECK_INTERNAL_ERROR;
    struct NLHOriginOwner *o = parent ? parent->h_origin : NULL;
    if (parent && !custody_parent(o, parent))
        return NL_CHECK_INTERNAL_ERROR;
    if (!parent) {
        o = calloc(1, sizeof(*o));
        if (!o)
            return NL_CHECK_OUT_OF_MEMORY;
        o->root = child;
        o->registration = child->context;
    }
    CustodyParent *p = calloc(1, sizeof(*p));
    if (!p) {
        if (!parent)
            free(o);
        return NL_CHECK_OUT_OF_MEMORY;
    }
    *p = (CustodyParent){.next = o->parents,
                         .fragment = child,
                         .world = child->context,
                         .source = child->source};
    o->parents = p;
    child->h_origin = o;
    return NL_CHECK_OK;
}
NLCheckStatus nl_h_custody_reserve(NLCheckedFragment *parent,
                                   NLCheckedFragment *child, NLCustodyRole role,
                                   NLCheckedNodeId site, size_t slot)
{
    CustodyParent *p = custody_parent(parent->h_origin, parent);
    if (!p || !child || child->h_origin != parent->h_origin ||
        !custody_parent(parent->h_origin, child) ||
        (role == NL_CUSTODY_ARM && slot != p->arm_count) ||
        (role == NL_CUSTODY_BODY && slot != p->body_count) ||
        (role == NL_CUSTODY_CONTINUATION && slot >= 2))
        return NL_CHECK_INTERNAL_ERROR;
    for (CustodyEdge *e = p->edges; e; e = e->next)
        if (e->child == child || (e->role == role && e->slot == slot))
            return NL_CHECK_INTERNAL_ERROR;
    CustodyEdge *e = calloc(1, sizeof(*e));
    if (!e)
        return NL_CHECK_OUT_OF_MEMORY;
    *e = (CustodyEdge){.next = p->edges,
                       .child = child,
                       .world = child->context,
                       .role = role,
                       .site = site,
                       .slot = slot,
                       .state = PENDING};
    p->edges = e;
    /* The child remains local throughout reservation. Each successful realloc
     * immediately updates the canonical buffer owner, even if phase 2 or a
     * later seal fails. Counts advance only in the infallible commit below. */
    if (role == NL_CUSTODY_ARM) {
        NLCheckedArm *arms = realloc(p->arms, (slot + 1) * sizeof(*arms));
        if (!arms)
            return NL_CHECK_OUT_OF_MEMORY;
        p->arms = parent->arms = arms;
        p->arm_capacity = slot + 1;
    } else if (role == NL_CUSTODY_BODY) {
        NLCheckedFragment **bodies =
            realloc(p->bodies, (slot + 1) * sizeof(*bodies));
        if (!bodies)
            return NL_CHECK_OUT_OF_MEMORY;
        p->bodies = parent->bodies = bodies;
        p->body_capacity = slot + 1;
        NLCheckedNodeId *calls = realloc(p->calls, (slot + 1) * sizeof(*calls));
        if (!calls)
            return NL_CHECK_OUT_OF_MEMORY;
        p->calls = parent->body_calls = calls;
        p->call_capacity = slot + 1;
    }
    return NL_CHECK_OK;
}
void nl_h_custody_commit(NLCheckedFragment *parent, NLCheckedFragment *child,
                         NLCustodyRole role, size_t slot)
{
    CustodyParent *p = custody_parent(parent->h_origin, parent);
    for (CustodyEdge *e = p->edges; e; e = e->next)
        if (e->state == PENDING && e->child == child && e->role == role &&
            e->slot == slot) {
            if (role == NL_CUSTODY_ARM) {
                p->arms[slot] = (NLCheckedArm){e->site, child, e->world};
                parent->arm_count = ++p->arm_count;
            } else if (role == NL_CUSTODY_BODY) {
                p->bodies[slot] = child;
                p->calls[slot] = e->site;
                parent->body_count = ++p->body_count;
            } else {
                parent->custody_continuations[slot] = child;
            }
            e->state = COMMITTED;
            return;
        }
}
static bool observed_arm(const struct NLHOriginOwner *o, const CustodyEdge *e,
                         const NLCheckedFragment *view)
{
    const NLCheckedFragment *original = e->child;
    /* A borrowed structural observation is not a second CHECK child. A real
     * member of either owner, a different world, or a different child table
     * cannot substitute for this original. Node contents remain inputs to the
     * existing semantic consumer; this grants no ownership or H authority. */
    if (!view || custody_parent(o, view) || view->h_origin != o ||
        view->context != e->world || view->source != original->source ||
        view->root != original->root || view->count != original->count ||
        view->capacity != original->capacity || (view->count && !view->nodes) ||
        view->body_owner != original->body_owner ||
        view->if_parent != original->if_parent ||
        view->if_parent_node != original->if_parent_node ||
        view->if_arm_index != original->if_arm_index ||
        view->branch_value_prefix != original->branch_value_prefix ||
        view->branch_fact_prefix != original->branch_fact_prefix ||
        view->arms != original->arms ||
        view->arm_count != original->arm_count ||
        view->bodies != original->bodies ||
        view->body_calls != original->body_calls ||
        view->body_count != original->body_count)
        return false;
    for (size_t i = 0; i < 2; ++i)
        if (view->custody_continuations[i] !=
            original->custody_continuations[i])
            return false;
    return true;
}
static bool validate_custody(const NLCheckedFragment *root, bool observation)
{
    const struct NLHOriginOwner *o = root ? root->h_origin : NULL;
    if (!o || o->root != root || !custody_parent(o, root))
        return false;
    for (const CustodyParent *p = o->parents; p; p = p->next) {
        const NLCheckedFragment *f = p->fragment;
        /* Counts and allocation bases precede every view access. Canonical
         * endpoints alone select the next parent, never a challenged getter. */
        if (f->h_origin != o || f->context != p->world ||
            f->source != p->source || f->arm_count != p->arm_count ||
            f->body_count != p->body_count || f->arms != p->arms ||
            f->bodies != p->bodies || f->body_calls != p->calls ||
            p->arm_count > p->arm_capacity ||
            p->body_count > p->body_capacity ||
            p->body_count > p->call_capacity)
            return false;
        size_t arms = 0, bodies = 0;
        bool continuation[2] = {false, false};
        for (const CustodyEdge *e = p->edges; e; e = e->next) {
            if (e->state != COMMITTED || !custody_parent(o, e->child))
                return false;
            if (e->role == NL_CUSTODY_ARM) {
                if (e->slot >= p->arm_count)
                    return false;
                const NLCheckedArm *v = &p->arms[e->slot];
                if ((v->artifact != e->child &&
                     (!observation || o->count ||
                      !observed_arm(o, e, v->artifact))) ||
                    v->match != e->site || v->context_origin != e->world)
                    return false;
                ++arms;
            } else if (e->role == NL_CUSTODY_BODY) {
                if (e->slot >= p->body_count ||
                    p->bodies[e->slot] != e->child ||
                    p->calls[e->slot] != e->site)
                    return false;
                ++bodies;
            } else {
                if (e->slot >= 2 || continuation[e->slot] ||
                    f->custody_continuations[e->slot] != e->child)
                    return false;
                continuation[e->slot] = true;
            }
            if (e->child->context != e->world)
                return false;
        }
        if (arms != p->arm_count || bodies != p->body_count)
            return false;
        for (size_t i = 0; i < 2; ++i)
            if (!continuation[i] && f->custody_continuations[i])
                return false;
    }
    return true;
}
bool nl_h_custody_validate(const NLCheckedFragment *root)
{
    /* Emission/source authority always requires actual canonical membership,
     * including zero-H-site entries. Observations never satisfy this check. */
    return validate_custody(root, false);
}
bool nl_h_origin_has_sites(const NLCheckedFragment *root)
{
    return root && root->h_origin && root->h_origin->count != 0;
}
bool nl_h_custody_borrow_valid(const NLCheckedFragment *f)
{
    /* Getters lend a view to the existing structural verifier, not an owned
     * child receipt. Count/base guards and all BODY/CONTINUATION identities
     * remain strict. H-site entries retain strict ARM identity too; the H/C17
     * entry and branch consumers separately use nl_h_custody_validate and the
     * original H witness. CHECK's unpublished local prefixes stay trusted. */
    if (!f || !f->h_origin || !f->h_origin->published)
        return true;
    const struct NLHOriginOwner *o = f->h_origin;
    if (!validate_custody(o->root, true))
        return false;
    if (custody_parent(o, f))
        return true;
    /* A nonmember may only be the already checked current ARM observation.
     * An unrelated shallow parent cannot borrow the root's valid counts. */
    for (const CustodyParent *p = o->parents; p; p = p->next)
        for (const CustodyEdge *e = p->edges; e; e = e->next)
            if (e->role == NL_CUSTODY_ARM && p->arms[e->slot].artifact == f)
                return true;
    return false;
}
bool nl_h_custody_destroy_children(NLCheckedFragment *f, bool continuations)
{
    if (!f->h_origin)
        return false;
    CustodyParent *p = custody_parent(f->h_origin, f);
    if (!p)
        return true;
    for (CustodyEdge *e = p->edges; e; e = e->next)
        if (e->state == COMMITTED &&
            (e->role == NL_CUSTODY_CONTINUATION) == continuations) {
            e->state = DETACHED;
            nl_checked_destroy(e->child);
        }
    if (!continuations) {
        free(p->arms);
        free(p->bodies);
        free(p->calls);
        p->arms = NULL;
        p->bodies = NULL;
        p->calls = NULL;
    }
    return true;
}
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
    if (!o || parent->h_origin != o || invocation->h_origin != o)
        return NL_CHECK_INTERNAL_ERROR;
    if (!o->invocation) {
        NLCheckStatus s = nl_body_retain(invocation->body_owner);
        if (s != NL_CHECK_OK)
            return s;
        o->invocation = invocation;
        o->plan = invocation->body_owner;
        o->source = invocation->source;
        o->call = call;
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
    if (o && o->root == root)
        o->published = true;
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
    if (!nl_h_custody_validate(root) || !o->closed || o->aborted ||
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
void nl_h_origin_release_fragment(NLCheckedFragment *f)
{
    struct NLHOriginOwner *o = f->h_origin;
    if (!o)
        return;
    CustodyParent **link = &o->parents;
    while (*link && (*link)->fragment != f)
        link = &(*link)->next;
    if (*link) {
        CustodyParent *p = *link;
        *link = p->next;
        while (p->edges) {
            CustodyEdge *e = p->edges;
            p->edges = e->next;
            free(e);
        }
        free(p);
    }
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
