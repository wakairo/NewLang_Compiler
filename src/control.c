#include "control.h"
#include "h_origin_internal.h"
#include "semantic_internal.h"
#include "stutter_control_internal.h"

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
/* Issuance and destruction use private records, never challenged views.
 * Union carries this source binding forward, while cloning its owned state.
 * None of these identities authorizes caller-world result/current transfer. */
typedef struct {
    NLControlExitKind kind;
    const NLCheckedFragment *owner;
    const NLCheckedFragment *parent;
    const NLSource *source;
    const NLSemanticContext *context;
    const NLCheckedNodeView *nodes;
    NLCheckedNodeId node;
    unsigned char node_seal[sizeof(NLCheckedNodeView)];
    unsigned char operand_seal[sizeof(NLCheckedNodeView)];
    NLCheckedResult value;
} SourceBreak;
typedef struct {
    NLControlExitView view;
    NLControlTarget *target;
    NLControlState *state;
    SourceBreak source;
} Exit;
struct NLControlExits {
    size_t count;
    Exit *edges;
};
struct NLQuantityReturnReceipt {
    const NLCheckedFragment *owner, *caller, *arm;
    const NLFunctionBody *plan;
    const NLSource *source;
    const NLSemanticContext *working, *arm_world;
    const NLCheckedNodeView *arm_nodes;
    NLControlTarget *target;
    Origin *origin;
    NLCheckedNodeId call, conditional, arm_root, block, returned, operand;
    size_t function, arm_index;
    NLCheckedResult slot;
    unsigned char return_seal[sizeof(NLCheckedNodeView)];
    unsigned char operand_seal[sizeof(NLCheckedNodeView)];
};
struct NLLoopHeader {
    NLControlState *entry;
    NLControlTarget *target;
    size_t count;
    NLValueId slots[NL_CONTROL_MAX_SLOTS];
    bool wide, wide_memory;
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
NLCheckStatus nl_control_state_capture(const NLControlState *entry,
                                       const NLSemanticContext *c,
                                       NLControlState **out)
{
    if (entry == NULL)
        return nl_control_state_create(c, out);
    if (c == NULL || out == NULL || *out != NULL)
        return NL_CHECK_INTERNAL_ERROR;
    if (entry->origin->owners == SIZE_MAX)
        return NL_CHECK_RESOURCE_LIMIT;
    NLControlState *s = malloc(sizeof(*s));
    if (s == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    *s = (NLControlState){.origin = entry->origin};
    ++s->origin->owners;
    NLCheckStatus status = nl_sem_clone(c, &s->context);
    if (status != NL_CHECK_OK) {
        nl_control_state_destroy(s);
        return status;
    }
    *out = s;
    return NL_CHECK_OK;
}
NLCheckStatus nl_control_state_project(const NLControlState *entry,
                                       const NLSemanticContext *c,
                                       NLControlState **out)
{
    if (entry == NULL || c == NULL || out == NULL || *out != NULL)
        return NL_CHECK_INTERNAL_ERROR;
    const NLSemanticContext *a = entry->context;
    if (a->binding_count > c->binding_count ||
        a->place_count > c->place_count || a->scope_count > c->scope_count)
        return NL_CHECK_INTERNAL_ERROR;
    NLCheckStatus status =
        nl_sem_function_exit(c, a->scope_count, a->place_count);
    if (status != NL_CHECK_OK)
        return status;
    for (size_t i = a->binding_count; i < c->binding_count; ++i)
        if (!c->bindings[i].hidden ||
            c->bindings[i].view.availability != NL_CONSUMED)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    for (size_t i = a->place_count; i < c->place_count; ++i)
        if (c->places[i].live)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    status = nl_control_state_capture(entry, c, out);
    if (status != NL_CHECK_OK)
        return status;
    NLSemanticContext *b = (*out)->context;
    for (size_t i = a->binding_count; i < b->binding_count; ++i)
        free(b->bindings[i].name);
    b->binding_count = a->binding_count;
    b->place_count = a->place_count;
    b->scope_count = a->scope_count;
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
static bool exit_binding_valid(const Exit *record)
{
    const NLControlExitView *e = &record->view;
    const SourceBreak *s = &record->source;
    if (e->state != record->state || e->target != record->target)
        return false;
    if (!s->owner)
        return !e->source_owner && !e->source_node && !e->source_target;
    return e->kind == s->kind && e->count == 1 && e->source_owner == s->owner &&
           e->source_node == s->node && e->source_target == record->target &&
           e->values[0].type == s->value.type &&
           e->values[0].value == s->value.value;
}
const NLControlExitView *nl_control_exit_view(const NLControlExits *s, size_t i)
{
    return s == NULL || i >= s->count ? NULL : &s->edges[i].view;
}
void nl_control_exits_destroy(NLControlExits *s)
{
    if (s != NULL) {
        for (size_t i = 0; i < s->count; ++i) {
            nl_control_state_destroy(s->edges[i].state);
            nl_control_target_destroy(s->edges[i].target);
        }
        free(s->edges);
        free(s);
    }
}
static NLCheckStatus append(NLControlExits **out, NLControlExitKind kind,
                            NLControlTarget *target,
                            const NLControlState *state,
                            const NLCheckedResult *values, size_t count,
                            const SourceBreak *source)
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
    Exit *edges = set == NULL
                      ? NULL
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
    if (source != NULL) {
        edge.source_owner = source->owner;
        edge.source_node = source->node;
        edge.source_target = target;
    }
    set->edges[set->count++] =
        (Exit){.view = edge,
               .target = target,
               .state = copy,
               .source = source ? *source : (SourceBreak){0}};
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
            const Exit *record = &sets[s]->edges[i];
            const NLControlExitView *e = &record->view;
            if (!exit_binding_valid(record)) {
                nl_control_exits_destroy(candidate);
                return NL_CHECK_INTERNAL_ERROR;
            }
            NLCheckStatus status =
                append(&candidate, e->kind, (NLControlTarget *)e->target,
                       e->state, e->values, e->count,
                       record->source.owner ? &record->source : NULL);
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
    return append(out, kind, target, state, values, count, NULL);
}
/* The sole source of a linked Break is source_control after add() succeeds.
 * Do not let general append/union mint proof for a synthetic checked node. */
NLCheckStatus nl_control_source_break_append(
    NLControlExits **out, NLControlTarget *target, const NLControlState *state,
    NLCheckedResult value, const NLCheckedFragment *owner, NLCheckedNodeId node,
    const NLCheckedFragment *parent)
{
    if (!owner || !parent || !owner->source || !owner->context || !target ||
        target->kind != NL_TARGET_LOOP || node != owner->count)
        return NL_CHECK_INTERNAL_ERROR;
    const NLCheckedNodeView *br = nl_checked_node_view(owner, node);
    const NLCheckedNodeView *operand =
        br ? nl_checked_node_view(owner, br->initializer) : NULL;
    if (!br || br->kind != NL_CHECKED_BREAK || !br->terminates ||
        !nl_source_span_valid(owner->source, br->span) || !operand ||
        br->initializer >= node || operand->type != value.type ||
        (operand->result_count != 1 && value.type != 1) ||
        (operand->result_count == 1 &&
         (operand->results[0].type != value.type ||
          operand->results[0].value != value.value)) ||
        (operand->result_count == 0 && (value.type != 1 || value.value != 0)))
        return NL_CHECK_INTERNAL_ERROR;
    SourceBreak issued = {.kind = NL_EXIT_BREAK,
                          .owner = owner,
                          .parent = parent,
                          .source = owner->source,
                          .context = owner->context,
                          .nodes = owner->nodes,
                          .node = node,
                          .value = value};
    /* Seal object bytes explicitly: struct assignment need not preserve
     * padding, whereas these private byte arrays must compare exactly. */
    memcpy(issued.node_seal, br, sizeof(*br));
    memcpy(issued.operand_seal, operand, sizeof(*operand));
    return append(out, NL_EXIT_BREAK, target, state, &value, 1, &issued);
}

NLCheckStatus nl_control_source_continue_append(
    NLControlExits **out, NLControlTarget *target, const NLControlState *state,
    NLCheckedResult value, const NLCheckedFragment *owner, NLCheckedNodeId node,
    const NLCheckedFragment *parent)
{
    const NLCheckedNodeView *n = nl_checked_node_view(owner, node);
    const NLCheckedNodeView *operand =
        n ? nl_checked_node_view(owner, n->initializer) : NULL;
    if (!owner || !parent || !owner->source || !owner->context || !target ||
        target->kind != NL_TARGET_LOOP || node != owner->count || !n ||
        n->kind != NL_CHECKED_CONTINUE || !n->terminates || !operand ||
        n->initializer >= node || operand->result_count != 1 ||
        operand->results[0].type != value.type ||
        operand->results[0].value != value.value ||
        value.type != nl_semantic_core_type(owner->context, NL_TYPE_USIZE))
        return NL_CHECK_INTERNAL_ERROR;
    SourceBreak issued = {.kind = NL_EXIT_CONTINUE,
                          .owner = owner,
                          .parent = parent,
                          .source = owner->source,
                          .context = owner->context,
                          .nodes = owner->nodes,
                          .node = node,
                          .value = value};
    memcpy(issued.node_seal, n, sizeof(*n));
    memcpy(issued.operand_seal, operand, sizeof(*operand));
    return append(out, NL_EXIT_CONTINUE, target, state, &value, 1, &issued);
}

/* The bounded cyclic relation uses one IF-arm terminal. Canonical custody
 * and the enclosing relation authenticate the live node-array base before
 * this check. Adding the IF_ARM wrapper may have relocated that array. */
bool nl_control_source_terminal_valid(const NLCheckedFragment *body,
                                      const NLControlTarget *target,
                                      NLControlExitKind kind,
                                      const NLCheckedFragment *parent)
{
    if (!body || !target || !parent || nl_control_exits_count(body->exits) != 1)
        return false;
    const Exit *r = &body->exits->edges[0];
    const SourceBreak *s = &r->source;
    const NLControlExitView *e = &r->view;
    const NLCheckedNodeView *arm = nl_checked_node_view(body, body->root);
    const NLCheckedNodeView *block =
        arm ? nl_checked_node_view(body, arm->initializer) : NULL;
    const NLCheckedNodeView *n = nl_checked_node_view(body, s->node);
    const NLCheckedNodeView *v =
        n ? nl_checked_node_view(body, n->initializer) : NULL;
    if (!exit_binding_valid(r) || e->kind != kind || r->target != target ||
        s->owner != body || s->parent != parent || s->source != body->source ||
        s->context != body->context || !arm || arm->kind != NL_CHECKED_IF_ARM ||
        !arm->terminates || !block || block->kind != NL_CHECKED_BLOCK ||
        block->tail || !n || !v || !n->terminates ||
        n->initializer >= s->node ||
        n->kind != (kind == NL_EXIT_CONTINUE ? NL_CHECKED_CONTINUE
                                             : NL_CHECKED_BREAK) ||
        v->result_count != 1 || v->results[0].type != s->value.type ||
        v->results[0].value != s->value.value ||
        memcmp(n, s->node_seal, sizeof(*n)) ||
        memcmp(v, s->operand_seal, sizeof(*v)))
        return false;
    size_t visited = 0, terminals = 0;
    for (NLCheckedNodeId id = block->first_item; id;) {
        const NLCheckedNodeView *item = nl_checked_node_view(body, id);
        if (!item || ++visited > block->item_count)
            return false;
        if (item->terminates) {
            if (id != s->node || ++terminals != 1 || item->next_item)
                return false;
        }
        id = item->next_item;
    }
    return visited == block->item_count && terminals == 1;
}

/* C178: exact one Break as a direct terminating lexical block item.
 * No heap or scalar/current/owner certificate is produced by this check. */
bool nl_control_source_break_body_valid(const NLCheckedFragment *body,
                                        const NLControlTarget *target)
{
    if (!body || !body->source || !body->context || !target ||
        nl_control_exits_count(body->exits) != 1)
        return false;
    const NLControlExitView *e = nl_control_exit_view(body->exits, 0);
    const Exit *record = &body->exits->edges[0];
    const SourceBreak *issued = &record->source;
    if (!e || !exit_binding_valid(record) || target != record->target ||
        record->target->kind != NL_TARGET_LOOP || e->state != record->state ||
        e->target != record->target || issued->owner != body ||
        issued->source != body->source || issued->context != body->context ||
        issued->node != e->source_node || issued->nodes != body->nodes ||
        body->count != issued->node ||
        issued->value.type != e->values[0].type ||
        issued->value.value != e->values[0].value)
        return false;
    const NLCheckedNodeView *br = nl_checked_node_view(body, e->source_node);
    const NLCheckedNodeView *operand =
        br ? nl_checked_node_view(body, br->initializer) : NULL;
    const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
    if (!e || e->kind != NL_EXIT_BREAK || e->count != 1 ||
        !nl_control_target_same(e->target, target) ||
        e->source_target != target || e->source_owner != body || !br ||
        !operand || !root || root->kind != NL_CHECKED_BLOCK || root->tail ||
        !br->terminates || br->kind != NL_CHECKED_BREAK ||
        !nl_source_span_valid(body->source, br->span) ||
        memcmp(br, issued->node_seal, sizeof(*br)) ||
        memcmp(operand, issued->operand_seal, sizeof(*operand)) ||
        br->initializer >= e->source_node ||
        e->values[0].type != operand->type ||
        (operand->result_count == 1
             ? e->values[0].value != operand->results[0].value ||
                   e->values[0].type != operand->results[0].type
             : operand->result_count != 0 || operand->type != 1 ||
                   e->values[0].value != 0))
        return false;
    const NLSemanticContext *post =
        nl_control_state_context((NLControlState *)e->state);
    if (!post || e->values[0].type > post->type_count ||
        (e->values[0].value &&
         (e->values[0].value > post->value_count ||
          post->values[e->values[0].value - 1].type != e->values[0].type ||
          post->values[e->values[0].value - 1].carrier == NL_CARRIER_ENDED)))
        return false;
    size_t found = 0, visited = 0;
    for (NLCheckedNodeId item = root->first_item; item;) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, item);
        if (!n || ++visited > root->item_count)
            return false;
        if (n->kind == NL_CHECKED_BREAK) {
            if (item != e->source_node || ++found != 1)
                return false;
        }
        item = n->next_item;
    }
    if (visited != root->item_count || found != 1)
        return false;
    for (NLCheckedNodeId i = 1; i <= body->count; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, i);
        if ((n->kind == NL_CHECKED_BREAK && i != e->source_node) ||
            n->kind == NL_CHECKED_CONTINUE || n->kind == NL_CHECKED_RETURN ||
            n->kind == NL_CHECKED_LOOP)
            return false;
    }
    return true;
}
bool nl_checked_loop_break_provenance_valid(const NLCheckedFragment *f,
                                            NLCheckedNodeId loop)
{
    if (!f || !f->h_origin)
        return false;
    const NLCheckedNodeView *node = nl_checked_node_view(f, loop);
    const NLCheckedFragment *body = nl_checked_loop_body(f, loop);
    if (!node || node->kind != NL_CHECKED_LOOP || node->argument_count != 0 ||
        node->break_edges != 1 || node->continue_edges || node->return_edges ||
        node->terminates || !body || body->source != f->source)
        return false;
    const NLControlExitView *e = nl_control_exit_view(body->exits, 0);
    return e && body->exits->edges[0].source.parent == f &&
           nl_control_source_break_body_valid(body, e->target);
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
        if (s->edges[i].view.kind != NL_EXIT_RETURN ||
            !nl_control_target_same(s->edges[i].view.target, t))
            return false;
    return true;
}

typedef struct {
    const NLCheckedFragment *arm;
    const NLCheckedNodeView *returned, *operand;
    const NLControlExitView *arm_edge;
    NLCheckedNodeId conditional, block_id, returned_id, operand_id;
    size_t arm_index;
    NLCheckedResult slot;
} QuantityReturnShape;

static bool quantity_return_signature(const NLCheckedFragment *body,
                                      const NLCheckedFragment *caller,
                                      NLCheckedNodeId call,
                                      const NLSemanticContext *working,
                                      const NLFunctionEntry **out)
{
    const NLCheckedNodeView *node = nl_checked_node_view(caller, call);
    if (!body || !caller || !working || !node ||
        node->kind != NL_CHECKED_REGISTERED_CALL || !node->body_backed ||
        node->function == 0 || node->function > working->function_count)
        return false;
    const NLFunctionEntry *fn = &working->functions[node->function - 1];
    const NLTypeId usize = nl_semantic_core_type(working, NL_TYPE_USIZE);
    const NLTypeId boolean = nl_semantic_core_type(working, NL_TYPE_BOOL);
    if (fn->kind != NL_CHECKED_REGISTERED_CALL || fn->count != 2 ||
        !fn->parameters || fn->parameters[0] != usize ||
        fn->parameters[1] != boolean || fn->result != usize ||
        fn->caller_effects || fn->hidden_dependencies || fn->owner_receiver ||
        fn->owner_producer || fn->custody_recipient || !fn->body ||
        body->body_owner != fn->body || body->source != fn->body->source ||
        !body->context)
        return false;
    *out = fn;
    return true;
}

static bool quantity_return_shape(const NLCheckedFragment *body,
                                  const NLCheckedFragment *caller,
                                  NLCheckedNodeId call,
                                  const NLSemanticContext *working,
                                  QuantityReturnShape *out)
{
    const NLFunctionEntry *fn = NULL;
    if (!quantity_return_signature(body, caller, call, working, &fn) ||
        body->arm_count != 2 || !body->function_target ||
        nl_control_exits_count(body->exits) != 1)
        return false;
    NLCheckedNodeId conditional = 0;
    for (NLCheckedNodeId id = 1; id <= body->count; ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, id);
        if (node && node->kind == NL_CHECKED_IF && node->normal_arms == 1) {
            if (conditional)
                return false;
            conditional = id;
        }
    }
    if (!conditional)
        return false;
    const NLCheckedFragment *arms[2] = {
        nl_checked_if_arm(body, conditional, 0),
        nl_checked_if_arm(body, conditional, 1)};
    if (!arms[0] || !arms[1] || arms[0] == arms[1])
        return false;
    const NLCheckedNodeView *roots[2] = {
        nl_checked_node_view(arms[0], arms[0]->root),
        nl_checked_node_view(arms[1], arms[1]->root)};
    if (!roots[0] || !roots[1] || roots[0]->kind != NL_CHECKED_IF_ARM ||
        roots[1]->kind != NL_CHECKED_IF_ARM ||
        roots[0]->terminates == roots[1]->terminates)
        return false;
    const size_t arm_index = roots[0]->terminates ? 0 : 1;
    const size_t normal_index = arm_index ^ 1u;
    const NLCheckedFragment *arm = arms[arm_index];
    const NLCheckedNodeView *block =
        nl_checked_node_view(arm, roots[arm_index]->initializer);
    if (nl_control_exits_count(arm->exits) != 1 ||
        nl_control_exits_count(arms[normal_index]->exits) != 0 ||
        arm->if_parent != body || arm->if_parent_node != conditional ||
        arm->if_arm_index != arm_index || arm->source != body->source ||
        roots[arm_index]->initializer == 0 ||
        roots[arm_index]->initializer >= arm->root || !block ||
        block->kind != NL_CHECKED_BLOCK || !block->terminates)
        return false;
    NLCheckedNodeId returned_id = 0;
    for (NLCheckedNodeId id = 1; id <= arm->count; ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(arm, id);
        if (node && node->kind == NL_CHECKED_RETURN) {
            if (returned_id)
                return false;
            returned_id = id;
        }
    }
    const NLCheckedNodeView *returned = nl_checked_node_view(arm, returned_id);
    const NLCheckedNodeView *operand =
        returned ? nl_checked_node_view(arm, returned->initializer) : NULL;
    const NLCheckedResult slot =
        returned ? returned->returned : (NLCheckedResult){0};
    const NLControlExitView *arm_edge = nl_control_exit_view(arm->exits, 0);
    const NLControlExitView *outer_edge = nl_control_exit_view(body->exits, 0);
    NLCheckedNodeId invocation = 0;
    size_t function = 0;
    const NLCheckedFragment *actual_caller =
        nl_h_custody_body_caller(body, body->context, &invocation);
    const NLCheckedFragment *actual_body =
        nl_h_custody_if_body(arm, body, &function);
    if (!returned_id || !returned || !operand || !returned->terminates ||
        returned->initializer == 0 || returned->initializer >= returned_id ||
        returned->result_count || returned->next_item ||
        operand->result_count != 1 || slot.type == 0 || slot.value == 0 ||
        operand->results[0].type != slot.type ||
        operand->results[0].value != slot.value ||
        roots[arm_index]->returned.type != slot.type ||
        roots[arm_index]->returned.value != slot.value ||
        slot.type != nl_semantic_core_type(arm->context, NL_TYPE_USIZE) ||
        !arm_edge || !outer_edge || arm_edge->kind != NL_EXIT_RETURN ||
        outer_edge->kind != NL_EXIT_RETURN || arm_edge->count != 1 ||
        outer_edge->count != 1 || arm_edge->values[0].type != slot.type ||
        arm_edge->values[0].value != slot.value ||
        outer_edge->values[0].type != slot.type ||
        outer_edge->values[0].value != slot.value ||
        arm_edge->target != body->function_target ||
        outer_edge->target != body->function_target || !arm_edge->state ||
        !outer_edge->state ||
        ((const NLControlState *)arm_edge->state)->origin !=
            ((const NLControlState *)outer_edge->state)->origin ||
        actual_caller != caller || invocation != call || actual_body != body ||
        function != nl_checked_node_view(caller, call)->function ||
        fn->body != body->body_owner)
        return false;
    *out = (QuantityReturnShape){.arm = arm,
                                 .returned = returned,
                                 .operand = operand,
                                 .arm_edge = arm_edge,
                                 .conditional = conditional,
                                 .block_id = roots[arm_index]->initializer,
                                 .returned_id = returned_id,
                                 .operand_id = returned->initializer,
                                 .arm_index = arm_index,
                                 .slot = slot};
    return true;
}

NLCheckStatus nl_control_quantity_return_receipt_issue(
    NLCheckedFragment *body, const NLCheckedFragment *caller,
    NLCheckedNodeId call, const NLSemanticContext *working)
{
    QuantityReturnShape shape;
    if (!quantity_return_shape(body, caller, call, working, &shape))
        return NL_CHECK_OK;
    if (body->quantity_return_receipt)
        return NL_CHECK_INTERNAL_ERROR;
    NLQuantityReturnReceipt *receipt = malloc(sizeof(*receipt));
    if (!receipt)
        return NL_CHECK_OUT_OF_MEMORY;
    *receipt = (NLQuantityReturnReceipt){
        .owner = body,
        .caller = caller,
        .arm = shape.arm,
        .plan = body->body_owner,
        .source = body->source,
        .working = body->context,
        .arm_world = shape.arm->context,
        .arm_nodes = shape.arm->nodes,
        .call = call,
        .conditional = shape.conditional,
        .arm_root = shape.arm->root,
        .block = shape.block_id,
        .returned = shape.returned_id,
        .operand = shape.operand_id,
        .function = nl_checked_node_view(caller, call)->function,
        .arm_index = shape.arm_index,
        .slot = shape.slot};
    memcpy(receipt->return_seal, shape.returned, sizeof(receipt->return_seal));
    memcpy(receipt->operand_seal, shape.operand, sizeof(receipt->operand_seal));
    NLCheckStatus status = retain_target(body->function_target);
    if (status != NL_CHECK_OK) {
        free(receipt);
        return status;
    }
    receipt->target = body->function_target;
    Origin *origin = ((const NLControlState *)shape.arm_edge->state)->origin;
    if (!origin || origin->owners == SIZE_MAX) {
        nl_control_target_destroy(receipt->target);
        free(receipt);
        return origin ? NL_CHECK_RESOURCE_LIMIT : NL_CHECK_INTERNAL_ERROR;
    }
    ++origin->owners;
    receipt->origin = origin;
    body->quantity_return_receipt = receipt;
    return NL_CHECK_OK;
}

static bool quantity_return_slot_valid(const NLControlExitView *edge,
                                       const NLQuantityReturnReceipt *receipt)
{
    if (!edge || edge->kind != NL_EXIT_RETURN || edge->count != 1 ||
        edge->target != receipt->target || !edge->state ||
        edge->values[0].type != receipt->slot.type ||
        edge->values[0].value != receipt->slot.value)
        return false;
    const NLControlState *state = (const NLControlState *)edge->state;
    const NLSemanticContext *world = state->context;
    return state->origin == receipt->origin && world &&
           receipt->slot.value <= world->value_count &&
           world->values[receipt->slot.value - 1].type == receipt->slot.type &&
           world->values[receipt->slot.value - 1].carrier == NL_CARRIER_LOOSE;
}

bool nl_control_quantity_return_receipt_valid(const NLCheckedFragment *body,
                                              const NLSemanticContext *working,
                                              const NLCheckedFragment *arm,
                                              const NLCheckedNodeView *root,
                                              const NLCheckedNodeView *returned)
{
    const NLQuantityReturnReceipt *receipt =
        body ? body->quantity_return_receipt : NULL;
    if (!working || !arm || !receipt || receipt->owner != body ||
        receipt->working != body->context || receipt->caller == NULL ||
        receipt->arm != arm || receipt->plan != body->body_owner ||
        receipt->source != body->source ||
        receipt->target != body->function_target ||
        receipt->arm_world != arm->context ||
        receipt->arm_nodes != arm->nodes || receipt->arm_root != arm->root ||
        root != nl_checked_node_view(arm, receipt->block) ||
        returned != nl_checked_node_view(arm, receipt->returned) ||
        nl_checked_if_arm(body, receipt->conditional, receipt->arm_index) !=
            arm)
        return false;
    NLCheckedNodeId call = 0;
    size_t function = 0;
    const NLCheckedFragment *caller =
        nl_h_custody_body_caller(body, body->context, &call);
    const NLCheckedFragment *owner = nl_h_custody_if_body(arm, body, &function);
    const NLCheckedNodeView *call_node =
        caller ? nl_checked_node_view(caller, call) : NULL;
    const NLCheckedNodeView *operand =
        nl_checked_node_view(arm, receipt->operand);
    const NLCheckedNodeView *arm_root =
        nl_checked_node_view(arm, receipt->arm_root);
    if (caller != receipt->caller || call != receipt->call || owner != body ||
        function != receipt->function || !call_node ||
        !call_node->body_backed ||
        call_node->kind != NL_CHECKED_REGISTERED_CALL ||
        call_node->function != receipt->function || receipt->function == 0 ||
        receipt->function > working->function_count ||
        working->functions[receipt->function - 1].body != receipt->plan ||
        arm->if_parent != body || arm->if_parent_node != receipt->conditional ||
        arm->if_arm_index != receipt->arm_index || !root || !returned ||
        !operand || !arm_root || arm_root->kind != NL_CHECKED_IF_ARM ||
        !arm_root->terminates || arm_root->initializer != receipt->block ||
        arm_root->returned.type != receipt->slot.type ||
        arm_root->returned.value != receipt->slot.value ||
        memcmp(receipt->return_seal, returned, sizeof(receipt->return_seal)) ||
        memcmp(receipt->operand_seal, operand, sizeof(receipt->operand_seal)) ||
        returned->initializer != receipt->operand ||
        returned->returned.type != receipt->slot.type ||
        returned->returned.value != receipt->slot.value ||
        operand->result_count != 1 ||
        operand->results[0].type != receipt->slot.type ||
        operand->results[0].value != receipt->slot.value ||
        root->returned.type != receipt->slot.type ||
        root->returned.value != receipt->slot.value ||
        !quantity_return_slot_valid(nl_control_exit_view(arm->exits, 0),
                                    receipt) ||
        !quantity_return_slot_valid(nl_control_exit_view(body->exits, 0),
                                    receipt))
        return false;
    return nl_control_exits_count(arm->exits) == 1 &&
           nl_control_exits_count(body->exits) == 1;
}

void nl_control_quantity_return_receipt_destroy(NLCheckedFragment *body)
{
    NLQuantityReturnReceipt *receipt =
        body ? body->quantity_return_receipt : NULL;
    if (!receipt)
        return;
    body->quantity_return_receipt = NULL;
    nl_control_target_destroy(receipt->target);
    if (--receipt->origin->owners == 0)
        free(receipt->origin);
    free(receipt);
}

NLCheckStatus nl_control_function_boundary(const NLControlExits *s,
                                           const NLControlTarget *target,
                                           NLTypeId result, size_t scopes,
                                           size_t places)
{
    if (target == NULL || target->kind != NL_TARGET_FUNCTION || result == 0)
        return NL_CHECK_INTERNAL_ERROR;
    for (size_t i = 0; i < nl_control_exits_count(s); ++i) {
        const NLControlExitView *e = &s->edges[i].view;
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
    return nl_loop_header_create_bounded(entry, target, ids, count, wide, wide,
                                         out);
}
NLCheckStatus nl_loop_header_create_bounded(const NLControlState *entry,
                                            NLControlTarget *target,
                                            const NLValueId *ids, size_t count,
                                            bool wide, bool wide_memory,
                                            NLLoopHeader **out)
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
    *h = (NLLoopHeader){
        .count = count, .wide = wide, .wide_memory = wide_memory};
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
        if (nl_fixed_frame_same(x, y) && x.type == y.type && x.live == y.live &&
            x.independent_root == y.independent_root &&
            x.governing_domain == y.governing_domain &&
            x.implicit_local == y.implicit_local &&
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
        if (!nl_fixed_frame_same(x, y) || x.type != y.type ||
            x.live != y.live || x.incarnation != y.incarnation ||
            x.independent_root != y.independent_root ||
            x.parent_sum != y.parent_sum ||
            x.payload_occurrence != y.payload_occurrence ||
            x.governing_domain != y.governing_domain ||
            x.implicit_local != y.implicit_local ||
            (x.placement.region != y.placement.region ||
             x.placement.start != y.placement.start ||
             x.placement.length != y.placement.length))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        if (!h->wide_memory || !flat(a, x.type) ||
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
        const NLControlExitView *e = &set->edges[i].view;
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
