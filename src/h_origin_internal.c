#include "h_origin_internal.h"
#include "unchecked_ref_internal.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "h_result_state_internal.h"

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
    NLCheckedNodeView call_view;
    bool result_sealed;
    const NLCheckedNodeView *call_nodes;
    size_t call_count, call_capacity;
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
    NLCheckedNodeView *issued_nodes;
    const NLCheckedNodeView *issued_node_base;
    const NLFunctionBody *issued_body_owner;
    size_t issued_node_count, issued_node_capacity;
    NLCheckedNodeId issued_root;
    bool nodes_sealed;
    unsigned char *issued_state; /* same result receipt, CHECK-issued bytes */
    size_t issued_state_size;
    NLUncheckedProof *unchecked;
    const NLUncheckedProof *issued_unchecked_base;
    NLUncheckedProof issued_unchecked;
    bool issued_unchecked_present;
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
static bool same_node(const NLCheckedFragment *f, NLCheckedNodeId id,
                      const NLCheckedNodeView *saved);
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
static bool issued_nodes_valid(const CustodyParent *p)
{
    const NLCheckedFragment *f = p ? p->fragment : NULL;
    return p && p->nodes_sealed && f &&
           p->unchecked == p->issued_unchecked_base &&
           ((p->unchecked != NULL) == p->issued_unchecked_present) &&
           (!p->unchecked || !memcmp(p->unchecked, &p->issued_unchecked,
                                     sizeof(*p->unchecked))) &&
           f->context == p->world && f->source == p->source &&
           f->nodes == p->issued_node_base &&
           f->count == p->issued_node_count &&
           f->capacity == p->issued_node_capacity &&
           f->root == p->issued_root && f->body_owner == p->issued_body_owner &&
           (!p->issued_node_count ||
            (f->nodes && p->issued_nodes &&
             !memcmp(f->nodes, p->issued_nodes,
                     p->issued_node_count * sizeof(*p->issued_nodes))));
}
static bool issued_state_valid(const CustodyParent *p,
                               const NLSemanticContext *world)
{
    if (!p->issued_state)
        return true;
    HResultBytes state = {.bytes = p->issued_state,
                          .size = p->issued_state_size,
                          .compare = true};
    return result_state(&state, p->fragment, world) && state.used == state.size;
}
static bool call_nodes_valid(const NLCheckedFragment *f, const CustodyEdge *e)
{
    return !e->call_nodes ||
           (f->nodes == e->call_nodes && f->count == e->call_count &&
            f->capacity == e->call_capacity);
}
static NLCheckStatus seal_nodes(struct NLHOriginOwner *o, NLCheckedFragment *f,
                                const NLSemanticContext *published,
                                const NLSemanticContext *actual)
{
    CustodyParent *p = custody_parent(o, f);
    if (!p)
        return NL_CHECK_INTERNAL_ERROR;
    if (p->nodes_sealed) {
        if (!issued_nodes_valid(p))
            return NL_CHECK_INTERNAL_ERROR;
    } else {
        if ((f->count && !f->nodes) ||
            f->count > SIZE_MAX / sizeof(*p->issued_nodes))
            return NL_CHECK_INTERNAL_ERROR;
        NLCheckedNodeView *nodes = NULL;
        if (f->count) {
            nodes = malloc(f->count * sizeof(*nodes));
            if (!nodes)
                return NL_CHECK_OUT_OF_MEMORY;
            memcpy(nodes, f->nodes, f->count * sizeof(*nodes));
        }
        p->issued_nodes = nodes;
        p->issued_node_base = f->nodes;
        p->issued_body_owner = f->body_owner;
        p->issued_node_count = f->count;
        p->issued_node_capacity = f->capacity;
        p->issued_root = f->root;
        if (p->unchecked && p->unchecked->has_read &&
            !p->unchecked->read.loan_closed)
            return NL_CHECK_INTERNAL_ERROR;
        p->issued_unchecked_base = p->unchecked;
        p->issued_unchecked_present = p->unchecked != NULL;
        if (p->unchecked)
            memcpy(&p->issued_unchecked, p->unchecked,
                   sizeof(p->issued_unchecked));
        p->nodes_sealed = true;
    }
    if (actual && !p->issued_state) {
        /* The root body borrows the destination context. Its genuine CHECK
         * candidate is committed only after this call; owned arm worlds are
         * already independent. Capture the data that will be published. */
        const NLSemanticContext *world =
            f->context == published ? actual : f->context;
        HResultBytes count = {0};
        if (!result_state(&count, f, world))
            return NL_CHECK_RESOURCE_LIMIT;
        p->issued_state = malloc(count.used);
        if (!p->issued_state)
            return NL_CHECK_OUT_OF_MEMORY;
        p->issued_state_size = count.used;
        HResultBytes state = {.bytes = p->issued_state, .size = count.used};
        if (!result_state(&state, f, world) || state.used != state.size)
            return NL_CHECK_INTERNAL_ERROR;
    }
    for (CustodyEdge *e = p->edges; e; e = e->next) {
        if (e->state != COMMITTED)
            return NL_CHECK_INTERNAL_ERROR;
        const NLCheckStatus status = seal_nodes(o, e->child, published, actual);
        if (status != NL_CHECK_OK)
            return status;
    }
    return NL_CHECK_OK;
}
NLCheckStatus nl_h_custody_seal_result(NLCheckedFragment *parent,
                                       NLCheckedFragment *child,
                                       NLCheckedNodeId call,
                                       const NLSemanticContext *ptr_if_world)
{
    if (!parent || !child || !call || !parent->h_origin ||
        child->h_origin != parent->h_origin)
        return NL_CHECK_INTERNAL_ERROR;
    CustodyParent *p = custody_parent(parent->h_origin, parent);
    const NLCheckedNodeView *call_view = nl_checked_node_view(parent, call);
    if (!p || !call_view)
        return NL_CHECK_INTERNAL_ERROR;
    for (CustodyEdge *e = p->edges; e; e = e->next) {
        if (e->state != PENDING || e->role != NL_CUSTODY_BODY ||
            e->child != child || e->site != call || e->result_sealed)
            continue;
        const NLCheckStatus status =
            seal_nodes(parent->h_origin, child, child->context, ptr_if_world);
        if (status != NL_CHECK_OK)
            return status;
        e->call_view = *call_view;
        e->result_sealed = true;
        if (ptr_if_world) {
            e->call_nodes = parent->nodes;
            e->call_count = parent->count;
            e->call_capacity = parent->capacity;
        }
        return NL_CHECK_OK;
    }
    return NL_CHECK_INTERNAL_ERROR;
}
static bool sealed_tree_valid(const struct NLHOriginOwner *o,
                              const NLCheckedFragment *f)
{
    const CustodyParent *p = custody_parent(o, f);
    if (!issued_nodes_valid(p) || !issued_state_valid(p, p->world))
        return false;
    for (const CustodyEdge *e = p->edges; e; e = e->next)
        if (e->state != COMMITTED || !sealed_tree_valid(o, e->child))
            return false;
    return true;
}
bool nl_h_custody_result_valid(const NLCheckedFragment *parent,
                               NLCheckedNodeId call)
{
    if (!parent || !call || !parent->h_origin ||
        !nl_h_custody_borrow_valid(parent))
        return false;
    const struct NLHOriginOwner *o = parent->h_origin;
    const CustodyParent *p = custody_parent(o, parent);
    if (!p)
        return false;
    for (const CustodyEdge *e = p->edges; e; e = e->next)
        if (e->state == COMMITTED && e->role == NL_CUSTODY_BODY &&
            e->site == call && e->result_sealed &&
            call_nodes_valid(parent, e) &&
            same_node(parent, call, &e->call_view))
            return sealed_tree_valid(o, e->child);
    return false;
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
static bool validate_custody(const NLCheckedFragment *root, bool observation,
                             const NLSemanticContext *candidate)
{
    const struct NLHOriginOwner *o = root ? root->h_origin : NULL;
    if (!o || o->root != root || !custody_parent(o, root) ||
        (candidate && o->published))
        return false;
    for (const CustodyParent *p = o->parents; p; p = p->next) {
        const NLCheckedFragment *f = p->fragment;
        /* Self-contained entry wrappers may release only the synthetic call
         * spelling after taking ownership of the context. The actual CHECK
         * root and all retained function/arm sources stay authenticated. Test
         * this state first: the released spelling itself is no longer live. */
        const bool source_matches =
            (f == o->root && !f->source &&
             f->destroy_context == nl_semantic_destroy) ||
            f->source == p->source;
        /* Counts and allocation bases precede every view access. Canonical
         * endpoints alone select the next parent, never a challenged getter. */
        if (f->h_origin != o || f->context != p->world || !source_matches ||
            f->arm_count != p->arm_count || f->body_count != p->body_count ||
            f->arms != p->arms || f->bodies != p->bodies ||
            f->body_calls != p->calls || p->arm_count > p->arm_capacity ||
            p->body_count > p->body_capacity ||
            p->body_count > p->call_capacity)
            return false;
        /* A published result seal cannot fall back to an unsealed emitter
         * profile when its displayed nodes are changed. Historical getters
         * still observe through the separate observation path. Count/base
         * authentication in issued_nodes_valid precedes any node read. */
        const NLSemanticContext *world =
            candidate && p->world == root->context ? candidate : p->world;
        if (!observation && p->nodes_sealed &&
            (!issued_nodes_valid(p) || !issued_state_valid(p, world)))
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
                    p->calls[e->slot] != e->site ||
                    (!observation && !call_nodes_valid(f, e)))
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
    return validate_custody(root, false, NULL);
}
bool nl_h_custody_validate_candidate(const NLCheckedFragment *root,
                                     const NLSemanticContext *candidate)
{
    /* CHECK's final precommit validation uses the real unpublished candidate.
     * Published consumers cannot substitute a world through this entry point.
     */
    return candidate && validate_custody(root, false, candidate);
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
    if (!validate_custody(o->root, true, NULL))
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
        free(p->issued_nodes);
        free(p->issued_state);
        free(p->unchecked);
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

NLCheckStatus nl_h_unchecked_prepare(NLCheckedFragment *f)
{
    CustodyParent *p = f ? custody_parent(f->h_origin, f) : NULL;
    if (!p || p->nodes_sealed || f->h_origin->published)
        return NL_CHECK_INTERNAL_ERROR;
    if (!p->unchecked) {
        p->unchecked = malloc(sizeof(*p->unchecked));
        if (!p->unchecked)
            return NL_CHECK_OUT_OF_MEMORY;
        memset(p->unchecked, 0, sizeof(*p->unchecked));
    }
    return NL_CHECK_OK;
}
NLUncheckedProof *nl_h_unchecked_build(NLCheckedFragment *f)
{
    CustodyParent *p = f ? custody_parent(f->h_origin, f) : NULL;
    return p && !p->nodes_sealed ? p->unchecked : NULL;
}
const NLUncheckedProof *nl_h_unchecked_data(const NLCheckedFragment *f)
{
    const CustodyParent *p = f ? custody_parent(f->h_origin, f) : NULL;
    return p ? p->unchecked : NULL;
}
static bool unchecked_any(const NLCheckedFragment *f, bool assumed_only)
{
    const struct NLHOriginOwner *o = f ? f->h_origin : NULL;
    if (!custody_parent(o, f))
        return false;
    for (const CustodyParent *p = o->parents; p; p = p->next) {
        const NLUncheckedProof *records[] = {
            p->unchecked,
            p->issued_unchecked_present ? &p->issued_unchecked : NULL};
        for (size_t i = 0; i < 2; ++i)
            if (records[i] && records[i]->has_read &&
                (!assumed_only || records[i]->read.assumed_conditions))
                return true;
    }
    return false;
}
bool nl_h_unchecked_machine_only(const NLCheckedFragment *f)
{
    return !unchecked_any(f, true);
}
bool nl_h_unchecked_present(const NLCheckedFragment *f)
{
    return unchecked_any(f, false);
}
bool nl_checked_local_assumption_view(const NLCheckedFragment *f,
                                      NLCheckedNodeId operation,
                                      NLCheckedLocalAssumptionView *out)
{
    /* Historical observation reads the canonical sealed copy, not challenged
     * nodes/world/record storage. It never grants current validity or truth. */
    const CustodyParent *p = f ? custody_parent(f->h_origin, f) : NULL;
    if (!out || !p || !p->nodes_sealed || !p->issued_unchecked_present ||
        !p->issued_unchecked.has_read ||
        p->issued_unchecked.read.operation != operation)
        return false;
    memcpy(out, &p->issued_unchecked.read, sizeof(*out));
    return true;
}
NLCheckedProofClass nl_checked_proof_class(const NLCheckedFragment *f)
{
    const struct NLHOriginOwner *o = f ? f->h_origin : NULL;
    if (!custody_parent(o, f) || !nl_h_custody_validate(o->root) ||
        !nl_h_custody_result_valid(o->root, nl_checked_root(o->root)))
        return NL_CHECKED_PROOF_INVALID;
    return nl_h_unchecked_machine_only(f) ? NL_CHECKED_MACHINE_PROVED
                                          : NL_CHECKED_AUTHOR_PREMISE;
}
NLCheckStatus nl_checked_local_assumption_origin(const NLCheckedFragment *f,
                                                 NLCheckedNodeId operation)
{
    NLCheckedLocalAssumptionView record;
    return nl_checked_local_assumption_view(f, operation, &record) &&
                   nl_checked_proof_class(f) != NL_CHECKED_PROOF_INVALID
               ? NL_CHECK_OK
               : NL_CHECK_SEMANTIC_ERROR;
}
