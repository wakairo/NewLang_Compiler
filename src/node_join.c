#include "semantic_internal.h"
#include <stdlib.h>

static NLNodeJoinProof *proof(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    for (NLNodeJoinProof *p = f == NULL ? NULL : f->node_joins; p != NULL;
         p = p->next)
        if (p->view.match == id)
            return p;
    return NULL;
}

void nl_node_join_destroy(NLCheckedFragment *f)
{
    while (f->node_joins != NULL) {
        NLNodeJoinProof *p = f->node_joins;
        f->node_joins = p->next;
        nl_semantic_destroy(p->entry_owned);
        nl_semantic_destroy(p->post_owned);
        for (size_t i = 0; i < 2; ++i)
            nl_semantic_destroy(p->arm_entry[i]);
        free(p);
    }
}

NLCheckStatus nl_node_join_prepare(NLCheckedFragment *f, NLCheckedNodeId id,
                                   const NLSemanticContext *c, NLValueId input)
{
    NLPlaceId originals[2] = {0};
    size_t n = 0;
    for (size_t i = 0; i < c->place_count; ++i) {
        const NLSemanticPlaceView r = c->places[i];
        if (!r.live || !r.independent_root ||
            !nl_recursive_local_type(c, r.type))
            continue;
        if (n == 2 || !r.implicit_local || r.placement.region != 0 ||
            r.fixed_field_count != 2)
            return NL_CHECK_OK; /* unsupported proof, no source restriction */
        originals[n++] = i + 1;
    }
    if (n != 2 || proof(f, id) != NULL)
        return NL_CHECK_OK;
    NLNodeJoinProof *p = malloc(sizeof(*p));
    if (p == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    *p = (NLNodeJoinProof){0};
    NLCheckStatus s = nl_sem_clone(c, &p->entry_owned);
    if (s != NL_CHECK_OK) {
        free(p);
        return s;
    }
    p->view = (NLNodeJoinView){.entry = p->entry_owned,
                               .match = id,
                               .input = input,
                               .originals = {originals[0], originals[1]}};
    p->parent = f;
    p->next = f->node_joins;
    f->node_joins = p;
    f->destroy_node_joins = nl_node_join_destroy;
    return NL_CHECK_OK;
}

NLCheckStatus nl_node_join_fork(NLCheckedFragment *f, NLCheckedNodeId id,
                                size_t index, NLCheckedFragment *arm)
{
    NLNodeJoinProof *p = proof(f, id);
    if (p == NULL)
        return NL_CHECK_OK;
    if (index >= 2 || p->arm_entry[index] != NULL)
        return NL_CHECK_INTERNAL_ERROR;
    NLCheckStatus s = nl_sem_clone(arm->context, &p->arm_entry[index]);
    if (s == NL_CHECK_OK) {
        arm->node_join_parent = p;
        p->view.arms[index].artifact = arm;
        p->view.arms[index].entry = p->arm_entry[index];
        p->view.arms[index].world = arm->context;
        p->arm_origin[index] = arm->context;
    }
    return s;
}

NLCheckStatus nl_node_join_finish(NLCheckedFragment *f, NLCheckedNodeId id,
                                  NLPlaceId changed,
                                  const NLSemanticContext *post)
{
    NLNodeJoinProof *p = proof(f, id);
    if (p == NULL)
        return NL_CHECK_OK;
    NLCheckStatus s = nl_sem_clone(post, &p->post_owned);
    if (s != NL_CHECK_OK)
        return s;
    p->view.post = p->post_owned;
    p->view.changed = changed;
    const NLPlaceId parent = post->places[changed - 1].parent_aggregate;
    p->view.sibling = post->places[parent - 1].fixed_fields[1];
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_match_arm(f, id, i);
        if (arm != p->view.arms[i].artifact)
            return NL_CHECK_INTERNAL_ERROR;
        p->view.arms[i].variant = arm->nodes[arm->root - 1].variant;
        for (size_t k = 0; k < arm->count; ++k)
            if (arm->nodes[k].kind == NL_CHECKED_REPLACE) {
                p->view.arms[i].operation = k + 1;
                p->view.arms[i].change = arm->nodes[k].field;
                if (arm->field_change_count == 1) {
                    p->view.arms[i].before_change =
                        arm->field_changes[0]->before;
                    p->view.arms[i].after_change = arm->field_changes[0]->after;
                }
            }
    }
    p->complete = true;
    return NL_CHECK_OK;
}

bool nl_checked_node_join_view(const NLCheckedFragment *f, NLCheckedNodeId id,
                               NLNodeJoinView *out)
{
    const NLNodeJoinProof *p = proof(f, id);
    if (p == NULL || !p->complete || out == NULL)
        return false;
    *out = p->view;
    return true;
}

static bool field_same(NLCheckedField a, NLCheckedField b)
{
    return a.present == b.present &&
           a.dependency_compatible == b.dependency_compatible &&
           a.base == b.base && a.nominal == b.nominal && a.type == b.type &&
           a.index == b.index && a.parent == b.parent && a.child == b.child &&
           a.parent_incarnation == b.parent_incarnation &&
           a.child_incarnation == b.child_incarnation &&
           a.parent_fact == b.parent_fact && a.child_fact == b.child_fact &&
           a.parent_post_fact == b.parent_post_fact &&
           a.child_post_fact == b.child_post_fact && a.access == b.access &&
           a.old_value == b.old_value && a.new_value == b.new_value &&
           a.payload_occurrence == b.payload_occurrence &&
           a.post_payload_occurrence == b.post_payload_occurrence;
}

static bool view_same(const NLNodeJoinView *a, const NLNodeJoinView *b)
{
    if (a->entry != b->entry || a->post != b->post || a->match != b->match ||
        a->input != b->input || a->originals[0] != b->originals[0] ||
        a->originals[1] != b->originals[1] || a->changed != b->changed ||
        a->sibling != b->sibling)
        return false;
    for (size_t i = 0; i < 2; ++i)
        if (a->arms[i].artifact != b->arms[i].artifact ||
            a->arms[i].entry != b->arms[i].entry ||
            a->arms[i].world != b->arms[i].world ||
            a->arms[i].before_change != b->arms[i].before_change ||
            a->arms[i].after_change != b->arms[i].after_change ||
            a->arms[i].variant != b->arms[i].variant ||
            a->arms[i].operation != b->arms[i].operation ||
            !field_same(a->arms[i].change, b->arms[i].change))
            return false;
    return true;
}

/* Finite checked-expression connectivity, not a CFG. A retained operation
 * outside the arm's actual body cannot certify an executed source effect. */
static bool reachable(const NLCheckedFragment *arm, NLCheckedNodeId id,
                      unsigned char *seen, size_t depth)
{
    if (id == 0)
        return true;
    const NLCheckedNodeView *n = nl_checked_node_view(arm, id);
    if (n == NULL || depth >= NL_SEMANTIC_MAX_DEPTH || seen[id] == 1)
        return false;
    if (seen[id] == 2)
        return n->kind != NL_CHECKED_REPLACE;
    seen[id] = 1;
    if (!reachable(arm, n->initializer, seen, depth + 1) ||
        !reachable(arm, n->tail, seen, depth + 1))
        return false;
    NLCheckedNodeId child = n->first_argument;
    for (size_t i = 0; i < n->argument_count; ++i) {
        const NLCheckedNodeView *a = nl_checked_node_view(arm, child);
        if (a == NULL || !reachable(arm, child, seen, depth + 1))
            return false;
        child = a->next_argument;
    }
    if (child != 0)
        return false;
    child = n->first_item;
    const size_t items = n->kind == NL_CHECKED_MATCH ? 0 : n->item_count;
    for (size_t i = 0; i < items; ++i) {
        const NLCheckedNodeView *a = nl_checked_node_view(arm, child);
        if (a == NULL || !reachable(arm, child, seen, depth + 1))
            return false;
        child = a->next_item;
    }
    if (child != 0)
        return false;
    seen[id] = 2;
    return true;
}

/* This extra verifier checks the source-issued write path, not just an issuer
 * boolean or equal final values. It requires the actual ref operand and its
 * exact enclosing scoped lexical loan. Local roots have implicit D identity
 * (root,incarnation), not a synthetic nonCopy LifetimeDomain value. */
static bool scoped_change(const NLCheckedFragment *arm,
                          const NLSemanticContext *entry,
                          NLCheckedNodeId operation, NLCheckedField f)
{
    const NLSemanticContext *c = arm->context;
    const NLCheckedNodeView *op = nl_checked_node_view(arm, operation);
    if (op == NULL || op->kind != NL_CHECKED_REPLACE ||
        op->argument_count != 2 || !field_same(op->field, f) || !f.present ||
        !f.dependency_compatible || f.access != NL_ACCESS_WRITE ||
        f.child == 0 || f.child > entry->place_count || f.parent == 0 ||
        f.parent > entry->place_count)
        return false;
    const NLCheckedNodeView *arg =
        nl_checked_node_view(arm, op->first_argument);
    const NLCheckedNodeView *incoming =
        arg == NULL ? NULL : nl_checked_node_view(arm, arg->next_argument);
    if (arg == NULL || incoming == NULL || arg->result_count != 1 ||
        incoming->result_count != 1 || arg->results[0].value == 0 ||
        arg->results[0].value > c->value_count ||
        incoming->results[0].value != f.new_value)
        return false;
    if (incoming->kind != NL_CHECKED_SUM_CONSTRUCTOR || f.new_value == 0 ||
        f.new_value > c->value_count)
        return false;
    const NLSemanticValueView sum = c->values[f.new_value - 1];
    if (incoming->variant != sum.variant || incoming->type != sum.type)
        return false;
    if (sum.sum_payload != 0) {
        const NLCheckedNodeView *payload =
            nl_checked_node_view(arm, incoming->initializer);
        if (payload == NULL || payload->result_count != 1 ||
            payload->results[0].value != sum.sum_payload ||
            !payload->has_reference_result)
            return false;
        const NLReferenceFacts a = payload->reference_result,
                               b = c->values[sum.sum_payload - 1].reference;
        if (a.place != b.place || a.incarnation != b.incarnation ||
            a.scope != b.scope || a.provenance != b.provenance ||
            a.readable != b.readable || a.writable != b.writable ||
            a.occurrence_dependency != b.occurrence_dependency)
            return false;
    } else if (incoming->initializer != 0)
        return false;
    const NLSemanticValueView v = c->values[arg->results[0].value - 1];
    const NLReferenceFacts r = v.reference;
    const NLSemanticTypeView t = c->types[v.type - 1].view;
    if (t.kind != NL_TYPE_REF || t.target != f.type ||
        t.access != NL_ACCESS_WRITE || t.is_exclusive ||
        v.reference_count != 0 || v.dependencies != NL_DEPENDENCY_FREE ||
        v.value_dependency_count != 0 || r.provenance != NL_PROVENANCE_VALID ||
        !r.readable || !r.writable || r.place != f.child ||
        r.incarnation != f.child_incarnation || r.occurrence_dependency != 0 ||
        r.scope <= entry->scope_count || r.scope > c->scope_count ||
        c->scopes[r.scope - 1].active ||
        f.child_post_fact <= entry->last_value_fact ||
        f.parent_post_fact <= entry->last_value_fact ||
        f.child_post_fact == f.parent_post_fact)
        return false;
    if (f.payload_occurrence != 0 &&
        (f.payload_occurrence > c->occurrence_count ||
         c->occurrences[f.payload_occurrence - 1].live))
        return false;
    if (f.post_payload_occurrence != 0 &&
        (f.post_payload_occurrence <= entry->occurrence_count ||
         f.post_payload_occurrence > c->occurrence_count ||
         !c->occurrences[f.post_payload_occurrence - 1].live))
        return false;
    size_t loans = 0;
    for (size_t k = 1; k <= arm->count; ++k) {
        const NLCheckedNodeView *n = nl_checked_node_view(arm, k);
        if (n->kind != NL_CHECKED_LOAN_HEADER || n->loan.scope != r.scope)
            continue;
        if (!n->loan.implicit_local || n->loan.from_ptr ||
            !n->loan.body_nonescape_proved ||
            n->loan.access != NL_ACCESS_WRITE || n->loan.is_exclusive ||
            n->loan.place != f.child ||
            n->loan.incarnation != f.child_incarnation ||
            n->loan.domain != entry->places[f.parent - 1].governing_domain ||
            n->loan.source != f.base || n->loan.ref_symbol != arg->symbol ||
            n->span.start_byte > op->span.start_byte ||
            n->span.end_byte < op->span.end_byte)
            return false;
        ++loans;
    }
    return loans == 1;
}

/* Reuse source-issued point-in-time Change snapshots. Replaying the actual
 * core operation authenticates both child and enclosing root facts, rather
 * than accepting an arbitrary fresh number from a branch's final context. */
static NLCheckStatus replay_change(const NLCheckedFragment *arm,
                                   const NLNodeJoinView *v, size_t index)
{
    if (arm->field_change_count != 1 || arm->field_changes[0] == NULL)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLCapturedChange *change = arm->field_changes[0];
    const NLCheckedField f = v->arms[index].change;
    if (change->node != v->arms[index].operation ||
        change->world != arm->context ||
        change->before != change->before_origin ||
        change->after != change->after_origin ||
        change->before != v->arms[index].before_change ||
        change->after != v->arms[index].after_change ||
        change->before == change->after ||
        nl_sem_validate(change->before) != NL_CHECK_OK ||
        nl_sem_validate(change->after) != NL_CHECK_OK || f.child == 0 ||
        f.child > change->before->place_count || f.parent == 0 ||
        f.parent > change->before->place_count || f.new_value == 0 ||
        f.new_value > change->before->value_count)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLSemanticPlaceView child = change->before->places[f.child - 1],
                              parent = change->before->places[f.parent - 1];
    if (child.current_value != f.old_value ||
        child.current_fact != f.child_fact ||
        child.payload_occurrence != f.payload_occurrence ||
        parent.current_fact != f.parent_fact)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLSemanticContext *expected = NULL;
    NLCheckStatus s = nl_sem_clone(change->before, &expected);
    if (s != NL_CHECK_OK)
        return s;
    s = nl_fixed_change(expected, f.child, f.new_value, false);
    if (s == NL_CHECK_OK) {
        for (size_t k = 0; k < expected->binding_count; ++k)
            if (expected->bindings[k].view.place == f.parent &&
                expected->bindings[k].view.availability == NL_AVAILABLE)
                expected->bindings[k].view.value =
                    expected->places[f.parent - 1].current_value;
        if (!nl_packet_same_entry(expected, change->after) ||
            expected->places[f.parent - 1].current_fact != f.parent_post_fact ||
            expected->places[f.child - 1].current_fact != f.child_post_fact ||
            expected->places[f.child - 1].payload_occurrence !=
                f.post_payload_occurrence)
            s = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    nl_semantic_destroy(expected);
    return s == NL_CHECK_INTERNAL_ERROR ? NL_CHECK_ANALYSIS_PRECISION_LIMIT : s;
}

/* Independent parent replay, never selecting a concrete arm or importing its
 * private suffix. Only ordinary core Change/Reset on a clone of common entry.
 * The complete expected context is then compared to the immediate issued post,
 * including root RF, occurrence retirement, sibling and hidden dependencies. */
static NLCheckStatus replay_post(const NLNodeJoinView *v)
{
    NLSemanticContext *expected = NULL;
    NLCheckStatus s = nl_sem_clone(v->entry, &expected);
    if (s != NL_CHECK_OK)
        return s;
    NLValueId top = 0;
    s = nl_sem_new_value(
        expected,
        (NLSemanticValueView){.type = expected->places[v->changed - 1].type},
        &top);
    if (s == NL_CHECK_OK)
        s = nl_fixed_change(expected, v->changed, top, true);
    if (s == NL_CHECK_OK) {
        const NLPlaceId root =
            expected->places[v->changed - 1].parent_aggregate;
        for (size_t k = 0; k < expected->binding_count; ++k)
            if (expected->bindings[k].view.place == root &&
                expected->bindings[k].view.availability == NL_AVAILABLE)
                expected->bindings[k].view.value =
                    expected->places[root - 1].current_value;
        nl_sem_end_value(expected, v->input);
        if (!nl_packet_same_entry(expected, v->post))
            s = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    nl_semantic_destroy(expected);
    return s;
}

NLCheckStatus nl_checked_node_join_validate(const NLCheckedFragment *f,
                                            NLCheckedNodeId id,
                                            const NLNodeJoinView *candidate)
{
    const NLNodeJoinProof *p = proof(f, id);
    if (p == NULL || !p->complete || p->parent != f ||
        (candidate != NULL && !view_same(candidate, &p->view)))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLNodeJoinView *v = &p->view;
    const NLCheckedNodeView *m = nl_checked_node_view(f, id);
    const NLCheckedNodeView *init =
        m == NULL ? NULL : nl_checked_node_view(f, m->initializer);
    if (m == NULL || m->kind != NL_CHECKED_MATCH || m->terminates ||
        m->normal_frame_unchanged || m->normal_arms != 2 ||
        m->item_count != 2 || init == NULL || init->result_count != 1 ||
        init->results[0].value != v->input || v->entry != p->entry_owned ||
        v->post != p->post_owned || v->entry == v->post ||
        v->entry == f->context || v->post == f->context ||
        nl_sem_validate(v->entry) != NL_CHECK_OK ||
        nl_sem_validate(v->post) != NL_CHECK_OK || v->input == 0 ||
        v->input > v->entry->value_count || v->changed == 0 ||
        v->changed > v->entry->place_count ||
        m->match_binding_prefix != v->entry->binding_count)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    unsigned char parent_connected[NL_SEMANTIC_MAX_ENTRIES + 1] = {0};
    if (f->count > NL_SEMANTIC_MAX_ENTRIES ||
        !reachable(f, f->root, parent_connected, 0) ||
        parent_connected[id] != 2)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLSemanticContext *entry = v->entry;
    const NLPlaceId parent = entry->places[v->changed - 1].parent_aggregate;
    size_t originals = 0;
    for (size_t i = 0; i < entry->place_count; ++i) {
        const NLSemanticPlaceView r = entry->places[i];
        if (r.live && r.independent_root &&
            nl_recursive_local_type(entry, r.type)) {
            if (originals == 2 || v->originals[originals++] != i + 1 ||
                !r.implicit_local || r.placement.region != 0 ||
                r.governing_domain != 0 || r.fixed_field_count != 2)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        }
    }
    if (originals != 2 || parent == 0 || parent > entry->place_count ||
        (parent != v->originals[0] && parent != v->originals[1]) ||
        entry->places[parent - 1].fixed_fields[0] != v->changed ||
        entry->places[parent - 1].fixed_fields[1] != v->sibling ||
        nl_checked_match_arm(f, id, 2) != NULL)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLPlaceId changed = 0;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_match_arm(f, id, i);
        const NLCheckedNodeView *a =
            arm == NULL ? NULL : nl_checked_node_view(arm, arm->root);
        if (arm == NULL || arm != v->arms[i].artifact ||
            arm->source != f->source || arm->node_join_parent != p ||
            arm->context != p->arm_origin[i] ||
            arm->context != v->arms[i].world ||
            v->arms[i].entry != p->arm_entry[i] || arm->context == entry ||
            arm->context == v->post || arm->context == f->context ||
            arm->context == v->arms[1 - i].world ||
            !nl_packet_same_entry(entry, p->arm_entry[i]) || a == NULL ||
            a->kind != NL_CHECKED_MATCH_ARM || a->terminates ||
            a->variant != v->arms[i].variant ||
            (a->variant != 1 && a->variant != 2) ||
            a->variant == v->arms[1 - i].variant || a->type != 1 ||
            a->result_count > 1 ||
            (a->result_count == 1 &&
             (a->results[0].value == 0 ||
              a->results[0].value > arm->context->value_count)) ||
            nl_sem_validate(arm->context) != NL_CHECK_OK ||
            arm->context->value_count < entry->value_count ||
            arm->context->place_count < entry->place_count ||
            arm->context->binding_count < entry->binding_count ||
            arm->context->scope_count < entry->scope_count ||
            arm->context->occurrence_count < entry->occurrence_count)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        unsigned char connected[NL_SEMANTIC_MAX_ENTRIES + 1] = {0};
        if (arm->count > NL_SEMANTIC_MAX_ENTRIES ||
            !reachable(arm, arm->root, connected, 0))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        size_t writes = 0;
        for (size_t k = 1; k <= arm->count; ++k) {
            const NLCheckedNodeView *n = nl_checked_node_view(arm, k);
            if (!nl_source_span_valid(arm->source, n->span))
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            if (n->kind == NL_CHECKED_REPLACE) {
                ++writes;
                if (connected[k] != 2 || k != v->arms[i].operation ||
                    !scoped_change(arm, entry, k, v->arms[i].change))
                    return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            }
        }
        if (writes != arm->field_change_count || writes > 1 ||
            (writes == 0 &&
             (v->arms[i].operation != 0 || v->arms[i].before_change != NULL ||
              v->arms[i].after_change != NULL)))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        if (writes == 1) {
            NLCheckStatus effect = replay_change(arm, v, i);
            if (effect != NL_CHECK_OK)
                return effect;
        }
        NLCheckStatus s =
            nl_node_join_arm_check(entry, arm, v->input, &changed);
        if (s != NL_CHECK_OK)
            return s;
    }
    if (changed != v->changed)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    return replay_post(v);
}
