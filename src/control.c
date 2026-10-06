#include "control.h"
#include "semantic_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct NLControlTarget {
    NLControlTargetKind kind;
    size_t owners;
};
typedef struct {
    size_t owners, values;
    NLValueFactId facts;
} Origin;
struct NLControlState {
    Origin *origin;
    NLSemanticContext *context;
};
struct NLControlExits {
    size_t count;
    NLControlExitView *edges;
};
struct NLLoopHeader {
    NLControlState *entry;
    NLControlTarget *target;
    size_t count;
    NLValueId slots[NL_CONTROL_MAX_SLOTS];
    bool wide;
};

NLCheckStatus nl_control_target_create(NLControlTargetKind kind,
                                       NLControlTarget **out)
{
    if (out == NULL || *out != NULL ||
        (kind != NL_TARGET_FUNCTION && kind != NL_TARGET_LOOP))
        return NL_CHECK_INTERNAL_ERROR;
    NLControlTarget *t = malloc(sizeof(*t));
    if (t == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    *t = (NLControlTarget){kind, 1};
    *out = t;
    return NL_CHECK_OK;
}
static NLCheckStatus retain_target(NLControlTarget *t)
{
    if (t == NULL)
        return NL_CHECK_INTERNAL_ERROR;
    if (t->owners == SIZE_MAX)
        return NL_CHECK_RESOURCE_LIMIT;
    ++t->owners;
    return NL_CHECK_OK;
}
void nl_control_target_destroy(NLControlTarget *t)
{
    if (t != NULL && --t->owners == 0)
        free(t);
}
bool nl_control_target_same(const NLControlTarget *a, const NLControlTarget *b)
{
    return a != NULL && a == b;
}

NLCheckStatus nl_control_state_create(const NLSemanticContext *c,
                                      NLControlState **out)
{
    if (c == NULL || out == NULL || *out != NULL)
        return NL_CHECK_INTERNAL_ERROR;
    NLControlState *s = malloc(sizeof(*s));
    if (s == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    *s = (NLControlState){0};
    s->origin = malloc(sizeof(*s->origin));
    if (s->origin == NULL) {
        free(s);
        return NL_CHECK_OUT_OF_MEMORY;
    }
    *s->origin = (Origin){
        .owners = 1, .values = c->value_count, .facts = c->last_value_fact};
    NLCheckStatus status = nl_sem_clone(c, &s->context);
    if (status != NL_CHECK_OK) {
        nl_control_state_destroy(s);
        return status;
    }
    *out = s;
    return NL_CHECK_OK;
}
NLCheckStatus nl_control_state_fork(const NLControlState *s,
                                    NLControlState **out)
{
    if (s == NULL || out == NULL || *out != NULL)
        return NL_CHECK_INTERNAL_ERROR;
    if (s->origin->owners == SIZE_MAX)
        return NL_CHECK_RESOURCE_LIMIT;
    NLControlState *next = malloc(sizeof(*next));
    if (next == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    *next = (NLControlState){.origin = s->origin};
    ++next->origin->owners;
    NLCheckStatus status = nl_sem_clone(s->context, &next->context);
    if (status != NL_CHECK_OK) {
        nl_control_state_destroy(next);
        return status;
    }
    *out = next;
    return NL_CHECK_OK;
}
NLSemanticContext *nl_control_state_context(NLControlState *s)
{
    return s == NULL ? NULL : s->context;
}
void nl_control_state_destroy(NLControlState *s)
{
    if (s != NULL) {
        nl_semantic_destroy(s->context);
        if (--s->origin->owners == 0)
            free(s->origin);
        free(s);
    }
}
size_t nl_control_exits_count(const NLControlExits *s)
{
    return s == NULL ? 0 : s->count;
}
const NLControlExitView *nl_control_exit_view(const NLControlExits *s, size_t i)
{
    return s == NULL || i >= s->count ? NULL : &s->edges[i];
}
void nl_control_exits_destroy(NLControlExits *s)
{
    if (s != NULL) {
        for (size_t i = 0; i < s->count; ++i) {
            nl_control_state_destroy((NLControlState *)s->edges[i].state);
            nl_control_target_destroy((NLControlTarget *)s->edges[i].target);
        }
        free(s->edges);
        free(s);
    }
}
static NLCheckStatus append(NLControlExits **out, NLControlExitKind kind,
                            NLControlTarget *target,
                            const NLControlState *state,
                            const NLCheckedResult *values, size_t count)
{
    if (count > NL_CONTROL_MAX_SLOTS)
        return NL_CHECK_RESOURCE_LIMIT;
    if (out == NULL || state == NULL || target == NULL ||
        (count != 0 && values == NULL) ||
        (kind != NL_EXIT_RETURN && kind != NL_EXIT_CONTINUE &&
         kind != NL_EXIT_BREAK) ||
        (kind == NL_EXIT_RETURN ? target->kind != NL_TARGET_FUNCTION
                                : target->kind != NL_TARGET_LOOP) ||
        (kind != NL_EXIT_CONTINUE && count != 1))
        return NL_CHECK_INTERNAL_ERROR;
    for (size_t i = 0; i < count; ++i) {
        const NLCheckedResult v = values[i];
        if (v.type == 0 || v.type > state->context->type_count ||
            (v.value == 0
                 ? v.type != 1
                 : v.value > state->context->value_count ||
                       state->context->values[v.value - 1].type != v.type ||
                       state->context->values[v.value - 1].carrier ==
                           NL_CARRIER_ENDED))
            return NL_CHECK_INTERNAL_ERROR;
    }
    for (size_t i = 0; i < count; ++i) {
        if (values[i].value != 0 &&
            !state->context->types[values[i].type - 1].view.is_copy) {
            if (state->context->values[values[i].value - 1].carrier !=
                NL_CARRIER_LOOSE)
                return NL_CHECK_SEMANTIC_ERROR;
            for (size_t j = 0; j < i; ++j)
                if (values[j].value == values[i].value)
                    return NL_CHECK_SEMANTIC_ERROR;
        }
    }
    if (nl_control_exits_count(*out) >= NL_CONTROL_MAX_EXITS)
        return NL_CHECK_RESOURCE_LIMIT;
    NLControlState *copy = NULL;
    NLCheckStatus status = nl_control_state_fork(state, &copy);
    if (status != NL_CHECK_OK)
        return status;
    status = retain_target(target);
    if (status != NL_CHECK_OK) {
        nl_control_state_destroy(copy);
        return status;
    }
    NLControlExits *set = *out;
    bool fresh = set == NULL;
    if (fresh) {
        set = malloc(sizeof(*set));
        if (set != NULL)
            *set = (NLControlExits){0};
    }
    NLControlExitView *edges =
        set == NULL ? NULL
                    : realloc(set->edges, (set->count + 1) * sizeof(*edges));
    if (edges == NULL) {
        if (fresh)
            free(set);
        nl_control_state_destroy(copy);
        nl_control_target_destroy(target);
        return NL_CHECK_OUT_OF_MEMORY;
    }
    set->edges = edges;
    NLControlExitView edge = {
        .kind = kind, .target = target, .state = copy, .count = count};
    if (count != 0)
        memcpy(edge.values, values, count * sizeof(*values));
    set->edges[set->count++] = edge;
    *out = set;
    return NL_CHECK_OK;
}
/* Union uses a fresh candidate so OOM cannot publish even an earlier edge. */
NLCheckStatus nl_control_exits_union(NLControlExits **out,
                                     const NLControlExits *src)
{
    if (out == NULL)
        return NL_CHECK_INTERNAL_ERROR;
    if (nl_control_exits_count(*out) + nl_control_exits_count(src) >
        NL_CONTROL_MAX_EXITS)
        return NL_CHECK_RESOURCE_LIMIT;
    NLControlExits *candidate = NULL;
    const NLControlExits *sets[] = {*out, src};
    for (size_t s = 0; s < 2; ++s)
        for (size_t i = 0; i < nl_control_exits_count(sets[s]); ++i) {
            const NLControlExitView *e = &sets[s]->edges[i];
            NLCheckStatus status =
                append(&candidate, e->kind, (NLControlTarget *)e->target,
                       e->state, e->values, e->count);
            if (status != NL_CHECK_OK) {
                nl_control_exits_destroy(candidate);
                return status;
            }
        }
    nl_control_exits_destroy(*out);
    *out = candidate;
    return NL_CHECK_OK;
}
NLCheckStatus
nl_control_exit_append(NLControlExits **out, NLControlExitKind kind,
                       NLControlTarget *target, const NLControlState *state,
                       const NLCheckedResult *values, size_t count)
{
    return append(out, kind, target, state, values, count);
}
static NLCheckStatus outcome(const NLControlOutcome *a,
                             const NLControlOutcome *b, bool sequence,
                             NLControlOutcome *out)
{
    if (a == NULL || b == NULL || out == NULL || out == a || out == b ||
        out->exits != NULL || out->normal)
        return NL_CHECK_INTERNAL_ERROR;
    NLControlOutcome candidate = {0};
    NLCheckStatus status = nl_control_exits_union(&candidate.exits, a->exits);
    if (status == NL_CHECK_OK && (!sequence || a->normal))
        status = nl_control_exits_union(&candidate.exits, b->exits);
    if (status != NL_CHECK_OK) {
        nl_control_outcome_destroy(&candidate);
        return status;
    }
    candidate.normal =
        sequence ? a->normal && b->normal : a->normal || b->normal;
    *out = candidate;
    return NL_CHECK_OK;
}
NLCheckStatus nl_control_outcome_join(const NLControlOutcome *a,
                                      const NLControlOutcome *b,
                                      NLControlOutcome *out)
{
    return outcome(a, b, false, out);
}
NLCheckStatus nl_control_outcome_then(const NLControlOutcome *a,
                                      const NLControlOutcome *b,
                                      NLControlOutcome *out)
{
    return outcome(a, b, true, out);
}
void nl_control_outcome_destroy(NLControlOutcome *o)
{
    if (o != NULL) {
        nl_control_exits_destroy(o->exits);
        *o = (NLControlOutcome){0};
    }
}

bool nl_control_exits_only_return(const NLControlExits *s,
                                  const NLControlTarget *t)
{
    if (nl_control_exits_count(s) == 0)
        return false;
    for (size_t i = 0; i < s->count; ++i)
        if (s->edges[i].kind != NL_EXIT_RETURN ||
            !nl_control_target_same(s->edges[i].target, t))
            return false;
    return true;
}
NLCheckStatus nl_control_function_boundary(const NLControlExits *s,
                                           const NLControlTarget *target,
                                           NLTypeId result, size_t scopes,
                                           size_t places)
{
    if (target == NULL || target->kind != NL_TARGET_FUNCTION || result == 0)
        return NL_CHECK_INTERNAL_ERROR;
    for (size_t i = 0; i < nl_control_exits_count(s); ++i) {
        const NLControlExitView *e = &s->edges[i];
        if (e->kind != NL_EXIT_RETURN ||
            !nl_control_target_same(e->target, target))
            return NL_CHECK_INTERNAL_ERROR;
        if (result > e->state->context->type_count)
            return NL_CHECK_INTERNAL_ERROR;
        if (e->state->context->types[result - 1].view.kind == NL_TYPE_REF)
            return NL_CHECK_SEMANTIC_UNSUPPORTED;
        if (e->count != 1 || e->values[0].type != result)
            return NL_CHECK_SEMANTIC_ERROR;
        NLCheckStatus status =
            nl_sem_function_exit(e->state->context, scopes, places);
        if (status != NL_CHECK_OK)
            return status;
    }
    return NL_CHECK_OK;
}

static bool flat(const NLSemanticContext *c, NLTypeId id)
{
    if (id == 0 || id > c->type_count)
        return false;
    NLSemanticTypeView t = c->types[id - 1].view;
    return id != nl_semantic_domain_type(c) && t.field_count == 0 &&
           (t.kind == NL_TYPE_UNIT || t.kind == NL_TYPE_NOMINAL ||
            t.kind == NL_TYPE_BOOL || t.kind == NL_TYPE_BYTE ||
            t.kind == NL_TYPE_U8 || t.kind == NL_TYPE_USIZE ||
            t.kind == NL_TYPE_ADDR);
}
static bool same_ref(NLReferenceFacts a, NLReferenceFacts b)
{
    return a.place == b.place && a.incarnation == b.incarnation &&
           a.scope == b.scope && a.provenance == b.provenance &&
           a.readable == b.readable && a.writable == b.writable &&
           a.occurrence_dependency == b.occurrence_dependency;
}
static bool same_refs(NLSemanticValueView a, NLSemanticValueView b)
{
    if (a.type != b.type || a.carrier != b.carrier ||
        a.owner_place != b.owner_place || a.dependencies != b.dependencies ||
        a.reference_count != b.reference_count)
        return false;
    for (size_t i = 0; i < nl_sem_ref_count(a); ++i)
        if (!same_ref(nl_sem_ref_fact(a, i), nl_sem_ref_fact(b, i)))
            return false;
    return true;
}
static NLCheckStatus slots(const NLControlState *s, const NLValueId *ids,
                           size_t count)
{
    if (count > NL_CONTROL_MAX_SLOTS)
        return NL_CHECK_RESOURCE_LIMIT;
    if (count != 0 && ids == NULL)
        return NL_CHECK_INTERNAL_ERROR;
    for (size_t i = 0; i < count; ++i) {
        if (ids[i] == 0 || ids[i] > s->context->value_count)
            return NL_CHECK_INTERNAL_ERROR;
        NLSemanticValueView v = s->context->values[ids[i] - 1];
        if (v.carrier != NL_CARRIER_LOOSE)
            return NL_CHECK_SEMANTIC_ERROR;
        if (v.dependencies != NL_DEPENDENCY_FREE || !flat(s->context, v.type))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        if (!s->context->types[v.type - 1].view.is_copy)
            for (size_t j = 0; j < i; ++j)
                if (ids[i] == ids[j])
                    return NL_CHECK_SEMANTIC_ERROR;
    }
    return NL_CHECK_OK;
}
NLCheckStatus nl_loop_header_create(const NLControlState *entry,
                                    NLControlTarget *target,
                                    const NLValueId *ids, size_t count,
                                    bool wide, NLLoopHeader **out)
{
    if (entry == NULL || target == NULL || target->kind != NL_TARGET_LOOP ||
        out == NULL || *out != NULL)
        return NL_CHECK_INTERNAL_ERROR;
    NLCheckStatus status = slots(entry, ids, count);
    if (status != NL_CHECK_OK)
        return status;
    NLLoopHeader *h = malloc(sizeof(*h));
    if (h == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    *h = (NLLoopHeader){.count = count, .wide = wide};
    if (count != 0)
        memcpy(h->slots, ids, count * sizeof(*ids));
    status = nl_control_state_fork(entry, &h->entry);
    if (status == NL_CHECK_OK)
        status = retain_target(target);
    if (status != NL_CHECK_OK) {
        nl_loop_header_destroy(h);
        return status;
    }
    h->target = target;
    status = nl_loop_header_includes(h, entry, ids, count);
    if (status != NL_CHECK_OK) {
        nl_loop_header_destroy(h);
        return status;
    }
    *out = h;
    return NL_CHECK_OK;
}
void nl_loop_header_destroy(NLLoopHeader *h)
{
    if (h != NULL) {
        nl_control_state_destroy(h->entry);
        nl_control_target_destroy(h->target);
        free(h);
    }
}
NLCheckStatus nl_loop_header_includes(const NLLoopHeader *h,
                                      const NLControlState *state,
                                      const NLValueId *ids, size_t count)
{
    if (h == NULL || state == NULL)
        return NL_CHECK_INTERNAL_ERROR;
    if (count != h->count)
        return NL_CHECK_SEMANTIC_ERROR;
    NLCheckStatus status = slots(state, ids, count);
    if (status != NL_CHECK_OK)
        return status;
    const NLSemanticContext *a = h->entry->context, *b = state->context;
    const Origin *o = h->entry->origin;
    if (state->origin != o || a->type_count != b->type_count)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    for (size_t i = 0; i < count; ++i) {
        NLSemanticValueView x = a->values[h->slots[i] - 1],
                            y = b->values[ids[i] - 1];
        if (x.type != y.type)
            return NL_CHECK_SEMANTIC_ERROR;
        if (!a->types[x.type - 1].view.is_copy) {
            if (h->slots[i] != ids[i] || ids[i] > o->values)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        } else if (!h->wide && x.scalar_known && y.scalar_known &&
                   x.scalar_value != y.scalar_value) {
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        } else if (!h->wide && (!x.scalar_known || !y.scalar_known ||
                                x.scalar_value != y.scalar_value)) {
            if (h->slots[i] != ids[i] || ids[i] > o->values)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        }
    }
    if (a->binding_count != b->binding_count ||
        a->place_count != b->place_count || a->scope_count != b->scope_count ||
        a->domain_count != b->domain_count ||
        a->region_count != b->region_count ||
        a->occurrence_count != b->occurrence_count ||
        a->function_count != b->function_count)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    for (size_t i = 0; i < a->binding_count; ++i) {
        NLSemanticBindingView x = a->bindings[i].view, y = b->bindings[i].view;
        if (x.type != y.type || x.place != y.place ||
            a->bindings[i].hidden != b->bindings[i].hidden)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        if (!a->types[x.type - 1].view.is_copy &&
            x.availability != y.availability)
            return NL_CHECK_SEMANTIC_ERROR;
        if (!a->types[x.type - 1].view.is_copy && x.value != y.value)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    for (size_t i = 0; i < a->place_count; ++i) {
        NLSemanticPlaceView x = a->places[i], y = b->places[i];
        if (x.type == y.type && x.live == y.live &&
            x.independent_root == y.independent_root &&
            x.governing_domain == y.governing_domain &&
            x.incarnation == y.incarnation &&
            x.current_fact == y.current_fact &&
            x.current_value == y.current_value &&
            x.payload_occurrence == y.payload_occurrence &&
            x.parent_sum == y.parent_sum &&
            x.placement.region == y.placement.region &&
            x.placement.start == y.placement.start &&
            x.placement.length == y.placement.length &&
            x.current_value <= o->values && x.current_fact <= o->facts)
            continue;
        if (x.type != y.type || x.live != y.live ||
            x.incarnation != y.incarnation ||
            x.independent_root != y.independent_root ||
            x.parent_sum != y.parent_sum ||
            x.payload_occurrence != y.payload_occurrence ||
            x.governing_domain != y.governing_domain ||
            (x.placement.region != y.placement.region ||
             x.placement.start != y.placement.start ||
             x.placement.length != y.placement.length))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        if (!h->wide || !flat(a, x.type) ||
            !a->types[x.type - 1].view.is_copy || x.current_value == 0 ||
            y.current_value == 0 ||
            a->values[x.current_value - 1].dependencies != NL_DEPENDENCY_FREE ||
            b->values[y.current_value - 1].dependencies != NL_DEPENDENCY_FREE)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    /* Rich correlated frames are exact-only in this prerequisite. No blocker
     * is erased merely because slot/current Copy contents were widened. */
    for (size_t i = 0; i < a->scope_count; ++i)
        if (a->scopes[i].active != b->scopes[i].active ||
            a->scopes[i].parent != b->scopes[i].parent ||
            a->scopes[i].parent_authority != b->scopes[i].parent_authority)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    for (size_t i = 0; i < a->domain_count; ++i)
        if (a->domains[i].live != b->domains[i].live ||
            a->domains[i].value != b->domains[i].value)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    if (a->region_count != 0 || a->occurrence_count != 0)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    for (size_t i = 0; i < a->value_count; ++i) {
        const NLSemanticValueView v = a->values[i];
        if (v.carrier != NL_CARRIER_ENDED &&
            !a->types[v.type - 1].view.is_copy &&
            (i >= b->value_count || b->values[i].type != v.type ||
             b->values[i].carrier != v.carrier ||
             b->values[i].owner_place != v.owner_place))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    for (size_t i = 0; i < b->value_count; ++i) {
        const NLSemanticValueView v = b->values[i];
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        if (v.dependencies != NL_DEPENDENCY_FREE)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        if (b->types[v.type - 1].view.kind == NL_TYPE_REF) {
            if (i >= o->values || i >= a->value_count ||
                !same_refs(a->values[i], v))
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        } else if (!flat(b, v.type))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        if (!b->types[v.type - 1].view.is_copy &&
            (i >= o->values || i >= a->value_count ||
             a->values[i].type != v.type || a->values[i].carrier != v.carrier ||
             a->values[i].owner_place != v.owner_place))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    return NL_CHECK_OK;
}
NLCheckStatus nl_loop_header_closure(const NLLoopHeader *h,
                                     const NLControlExits *set)
{
    if (h == NULL)
        return NL_CHECK_INTERNAL_ERROR;
    for (size_t i = 0; i < nl_control_exits_count(set); ++i) {
        const NLControlExitView *e = &set->edges[i];
        if (e->kind != NL_EXIT_CONTINUE)
            continue;
        if (!nl_control_target_same(e->target, h->target))
            return NL_CHECK_INTERNAL_ERROR;
        NLValueId ids[NL_CONTROL_MAX_SLOTS] = {0};
        for (size_t j = 0; j < e->count; ++j)
            ids[j] = e->values[j].value;
        NLCheckStatus status =
            nl_loop_header_includes(h, e->state, ids, e->count);
        if (status != NL_CHECK_OK)
            return status;
    }
    return NL_CHECK_OK;
}
