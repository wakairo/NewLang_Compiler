#include "semantic_internal.h"
#include <stdlib.h>

/* This is a single inherited packet, not an owner-carrying sum join. The
 * original producer certificate remains in its ancestor-owned world. */
static bool packet_relation(const NLCheckedFragment *f, NLCheckedNodeId match,
                            const NLSemanticContext *c, bool moved)
{
    const NLCheckedNodeView *m = nl_checked_node_view(f, match);
    if (m == NULL || m->packet_fork.producer == 0 ||
        !nl_producer_valid(f, m->packet_fork.producer, false) ||
        nl_sem_validate(c) != NL_CHECK_OK)
        return false;
    const NLCheckedNodeView *p =
        nl_checked_node_view(f, m->packet_fork.producer);
    const NLSemanticContext *returned = p->producer.return_world;
    const NLValueId packet = m->packet_fork.packet;
    const NLSymbolId symbol = m->packet_fork.binding;
    if (packet != p->producer.result || packet > c->value_count ||
        symbol == 0 || symbol > c->binding_count)
        return false;
    const NLSemanticBindingView b = c->bindings[symbol - 1].view;
    const NLSemanticValueView v = c->values[packet - 1];
    if (b.availability != (moved ? NL_CONSUMED : NL_AVAILABLE) ||
        b.value != packet || c->bindings[symbol - 1].hidden ||
        v.carrier != (moved ? NL_CARRIER_LOOSE : NL_CARRIER_PLACE) ||
        v.owner_place != (moved ? 0 : b.place) ||
        v.type != p->results[0].type || v.field_count != 3 ||
        v.dependencies != NL_DEPENDENCY_FREE || v.value_dependency_count != 0 ||
        b.place == 0 || b.place > c->place_count ||
        c->places[b.place - 1].live == moved ||
        c->places[b.place - 1].current_value != (moved ? 0 : packet) ||
        p->producer.root > c->place_count ||
        p->producer.range.region > c->region_count ||
        p->producer.domain > c->domain_count)
        return false;
    const NLSemanticPlaceView root = c->places[p->producer.root - 1],
                              old = returned->places[p->producer.root - 1];
    const NLSemanticBackingView r =
        c->regions[p->producer.range.region - 1].view;
    if (!root.live || !nl_fixed_live(c, p->producer.root) ||
        root.incarnation != p->producer.incarnation ||
        root.current_value != old.current_value ||
        root.current_fact != old.current_fact ||
        root.governing_domain != p->producer.domain ||
        root.placement.region != p->producer.range.region ||
        root.placement.start != 0 || root.placement.length != r.size ||
        root.placement.length != p->producer.range.length || !r.live ||
        !r.ordinary_read || !r.ordinary_write ||
        !c->domains[p->producer.domain - 1].live)
        return false;
    for (size_t i = 0; i < 3; ++i) {
        const NLValueId field = v.fields[i];
        if (field == 0 || field > c->value_count ||
            field != returned->values[packet - 1].fields[i])
            return false;
        const NLSemanticValueView a = c->values[field - 1],
                                  z = returned->values[field - 1];
        if (a.carrier != NL_CARRIER_AGGREGATE || a.aggregate_owner != packet ||
            a.type != z.type || a.dependencies != NL_DEPENDENCY_FREE ||
            a.value_dependency_count != 0 || a.owner_place != 0)
            return false;
        if (i == 0) {
            const NLReferenceFacts x = a.reference, y = z.reference;
            if (a.reference_count != 0 || x.provenance != NL_PROVENANCE_VALID ||
                x.place != y.place || x.incarnation != y.incarnation ||
                x.scope != y.scope || x.readable != y.readable ||
                x.writable != y.writable ||
                x.occurrence_dependency != y.occurrence_dependency)
                return false;
        } else if (i == 1) {
            if (field != p->producer.inputs[2] ||
                a.allocation_region != root.placement.region)
                return false;
        } else if (field != p->producer.inputs[3] ||
                   a.domain != p->producer.domain ||
                   c->domains[a.domain - 1].value != field)
            return false;
    }
    return true;
}

static bool current_packet(const NLCheckedFragment *f, NLCheckedNodeId match,
                           const NLSemanticContext *c, bool moved)
{
    if (!packet_relation(f, match, c, moved))
        return false;
    /* A deliberately small entry profile. Unknown/dependent capabilities or
     * an enclosing loan require additional evidence, never erased blockers. */
    for (size_t i = 0; i < c->scope_count; ++i)
        if (c->scopes[i].active)
            return false;
    for (size_t i = 0; i < c->value_count; ++i)
        if (c->values[i].carrier != NL_CARRIER_ENDED &&
            (c->values[i].dependencies != NL_DEPENDENCY_FREE ||
             c->values[i].value_dependency_count != 0))
            return false;
    return true;
}

NLCheckStatus nl_packet_fork_prepare(NLCheckedFragment *f, NLCheckedNodeId id,
                                     const NLSemanticContext *c)
{
    if (f == NULL || c == NULL || id == 0 || id > f->count)
        return NL_CHECK_INTERNAL_ERROR;
    if (f->packet_entry != NULL || f->packet_parent != NULL ||
        f->producer_call == 0 || c->region_count != 2 || c->domain_count != 2)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLCheckedNodeView *p = nl_checked_node_view(f, f->producer_call);
    if (p == NULL)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLSymbolId binding = 0;
    for (size_t i = 0; i < c->binding_count; ++i)
        if (c->bindings[i].view.availability == NL_AVAILABLE &&
            c->bindings[i].view.value == p->producer.result) {
            if (binding != 0)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            binding = i + 1;
        }
    NLCheckedNodeView *m = &f->nodes[id - 1];
    m->packet_fork.producer = f->producer_call;
    m->packet_fork.packet = p->producer.result;
    m->packet_fork.binding = binding;
    NLCheckStatus integrity = nl_raw_validate(c);
    if (integrity == NL_CHECK_OK)
        integrity = nl_raw_validate(f->producer_entry);
    if (integrity == NL_CHECK_OK)
        integrity = nl_raw_validate(f->producer_return);
    if (integrity != NL_CHECK_OK)
        return integrity;
    if (!current_packet(f, id, c, false))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLCheckStatus s = nl_sem_clone(c, &f->packet_entry);
    if (s != NL_CHECK_OK)
        return s;
    f->destroy_packet_world = nl_semantic_destroy;
    f->packet_match = id;
    m->packet_fork.entry_world = f->packet_entry;
    return NL_CHECK_OK;
}

static bool lineage(const NLCheckedFragment *arm)
{
    const NLCheckedFragment *f = arm->packet_parent;
    const NLCheckedNodeView *m = nl_checked_node_view(f, arm->packet_match);
    return f != NULL && arm != f && arm->context != f->context &&
           arm->packet_entry != NULL && m != NULL &&
           f->packet_match == arm->packet_match &&
           f->packet_entry == m->packet_fork.entry_world &&
           arm->packet_entry != f->packet_entry &&
           current_packet(f, arm->packet_match, f->packet_entry, false) &&
           nl_packet_same_entry(f->packet_entry, arm->packet_entry);
}

/* Read-only pre-consumption relation. Unlike whole receiving, a recipient
 * preflight has a live caller-local sink loan. Its caller must independently
 * check that scope and all blockers; this predicate grants no transfer and
 * does not relax the existing unloaned fork/receiving/closure predicates. */
bool nl_packet_available_inherited(const NLCheckedFragment *arm,
                                   NLValueId value)
{
    if (arm == NULL || !lineage(arm))
        return false;
    const NLCheckedNodeView *m =
        nl_checked_node_view(arm->packet_parent, arm->packet_match);
    return value == m->packet_fork.packet &&
           packet_relation(arm->packet_parent, arm->packet_match, arm->context,
                           false);
}

/* Called after the ordinary identifier has consumed the packet placement,
 * but before whole receiving transfers its fields. Authority is inherited
 * only for the exact prefix package proven Available at this child's entry. */
bool nl_packet_inherited(const NLCheckedFragment *arm, NLValueId value)
{
    if (!lineage(arm))
        return false;
    const NLCheckedNodeView *m =
        nl_checked_node_view(arm->packet_parent, arm->packet_match);
    if (value != m->packet_fork.packet || value > arm->context->value_count)
        return false;
    return current_packet(arm->packet_parent, arm->packet_match, arm->context,
                          true);
}

/* Derive the common closed-tail proof target from the parent alone. This is
 * not a source operation or authority grant: BOTH arms must prove it before
 * it can be committed. Neither child context is an input to construction. */
NLCheckStatus nl_packet_closed(const NLCheckedFragment *f, NLCheckedNodeId id,
                               const NLSemanticContext *before,
                               NLSemanticContext **out)
{
    if (out == NULL || *out != NULL)
        return NL_CHECK_INTERNAL_ERROR;
    const NLCheckedNodeView *m = nl_checked_node_view(f, id);
    if (m == NULL || before == NULL || !current_packet(f, id, before, false))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLCheckedNodeView *p =
        nl_checked_node_view(f, m->packet_fork.producer);
    NLSemanticContext *c = NULL;
    NLCheckStatus s = nl_sem_clone(before, &c);
    if (s != NL_CHECK_OK)
        return s;
    NLSemanticBindingView *b = &c->bindings[m->packet_fork.binding - 1].view;
    b->availability = NL_CONSUMED;
    NLPlaceId places[] = {b->place, p->producer.root};
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticPlaceView *place = &c->places[places[i] - 1];
        nl_sum_detach(c, places[i]);
        nl_fixed_detach(c, places[i]);
        nl_sem_end_value(c, place->current_value);
        place->live = false;
        place->current_value = 0;
        place->current_fact = 0;
        place->governing_domain = 0;
        if (i == 1)
            place->placement = (NLBackingRange){0};
    }
    c->domains[p->producer.domain - 1].live = false;
    NLRawRegionEntry *r = &c->regions[p->producer.range.region - 1];
    r->view.live = false;
    c->raw_interval_count -= r->count;
    free(r->intervals);
    r->intervals = NULL;
    r->count = 0;
    s = nl_sem_validate(c);
    if (s == NL_CHECK_OK)
        s = nl_raw_validate(c);
    if (s != NL_CHECK_OK) {
        nl_semantic_destroy(c);
        return s;
    }
    *out = c;
    return NL_CHECK_OK;
}

/* Parent-only common LIVE target. The sole projection is the owned Copy
 * scrutinee temporary, not a source binding, owner or branch-local fact. */
NLCheckStatus nl_packet_retained(const NLCheckedFragment *f, NLCheckedNodeId id,
                                 const NLSemanticContext *before,
                                 NLSemanticContext **out)
{
    if (out == NULL || *out != NULL)
        return NL_CHECK_INTERNAL_ERROR;
    const NLCheckedNodeView *m = nl_checked_node_view(f, id);
    const NLCheckedNodeView *init =
        m == NULL ? NULL : nl_checked_node_view(f, m->initializer);
    if (m == NULL || m->kind != NL_CHECKED_MATCH || before == NULL ||
        init == NULL || init->result_count != 1 ||
        !current_packet(f, id, before, false))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLValueId input = init->results[0].value;
    if (input == 0 || input > before->value_count ||
        input == m->packet_fork.packet ||
        before->values[input - 1].carrier != NL_CARRIER_LOOSE ||
        !before->types[before->values[input - 1].type - 1].view.is_copy ||
        before->types[before->values[input - 1].type - 1].view.kind !=
            NL_TYPE_SUM ||
        nl_sum_authority(before, before->values[input - 1].type))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLSemanticContext *c = NULL;
    NLCheckStatus s = nl_sem_clone(before, &c);
    if (s != NL_CHECK_OK)
        return s;
    nl_sem_end_value(c, input);
    c->values[input - 1].sum_payload = 0;
    s = nl_sem_validate(c);
    if (s == NL_CHECK_OK)
        s = nl_raw_validate(c);
    if (s != NL_CHECK_OK) {
        nl_semantic_destroy(c);
        return s;
    }
    *out = c;
    return NL_CHECK_OK;
}

bool nl_packet_arm_retained(const NLCheckedFragment *f, NLCheckedNodeId match,
                            const NLCheckedFragment *arm,
                            const NLCheckedNodeView *result)
{
    return arm != NULL && arm->packet_parent == f &&
           arm->packet_match == match && lineage(arm) &&
           f->packet_retained_post != NULL &&
           nl_control_exits_count(arm->exits) == 0 &&
           nl_control_exits_count(arm->loop_returns) == 0 &&
           current_packet(f, match, arm->context, false) &&
           nl_allocated_post_matches(f->packet_retained_post, arm->context,
                                     result);
}

bool nl_packet_receiving_after_join(const NLCheckedFragment *f,
                                    const NLSemanticContext *c,
                                    NLValueId packet)
{
    const NLCheckedNodeView *m = nl_checked_node_view(f, f->packet_match);
    return m != NULL && m->packet_fork.retained && !m->packet_fork.closed &&
           packet == m->packet_fork.packet &&
           m->packet_fork.post_world == f->packet_retained_post &&
           current_packet(f, f->packet_match, c, true);
}

static bool packet_terminal(const NLCheckedFragment *f, NLCheckedNodeId match,
                            const NLCheckedFragment *arm,
                            const NLSemanticContext *receiving_world)
{
    const NLCheckedNodeView *m = nl_checked_node_view(f, match),
                            *p = nl_checked_node_view(f,
                                                      m->packet_fork.producer),
                            *call = nl_checked_node_view(arm,
                                                         arm->owner_entry_call);
    const NLSemanticContext *entry =
        nl_checked_owner_entry(arm, arm->owner_entry_call);
    if (call == NULL || entry == NULL || !call->owner_call.entry_proved ||
        !call->owner_call.post_proved || !call->body_backed ||
        call->owner_call.root != p->producer.root ||
        call->owner_call.incarnation != p->producer.incarnation ||
        call->owner_call.range.region != p->producer.range.region ||
        call->owner_call.domain != p->producer.domain ||
        call->owner_call.inputs[1] != p->producer.inputs[2] ||
        call->owner_call.inputs[2] != p->producer.inputs[3] ||
        nl_owner_relations(entry, call->owner_call.inputs,
                           &call->owner_call.definition, call->span,
                           NULL) != NL_CHECK_OK)
        return false;
    const NLCheckedFragment *body =
        nl_checked_call_body(arm, arm->owner_entry_call);
    if (call->kind != NL_CHECKED_REGISTERED_CALL || call->argument_count != 3 ||
        call->function == 0 || call->function > entry->function_count ||
        !entry->functions[call->function - 1].owner_receiver || body == NULL ||
        body->context != arm->context ||
        body->body_owner != entry->functions[call->function - 1].body)
        return false;
    const NLTypedOwnerDefinition d = body->body_owner->owner_definition,
                                 certificate = call->owner_call.definition;
    if (!d.definition_checked || d.live_return || d.head_link_required ||
        d.target != certificate.target ||
        d.definition_checked != certificate.definition_checked ||
        d.live_return != certificate.live_return ||
        d.head_link_required != certificate.head_link_required ||
        d.requirements != certificate.requirements ||
        d.step_count != certificate.step_count || d.step_count < 4 ||
        d.step_count > 5 ||
        call->owner_call.range.start != p->producer.range.start ||
        call->owner_call.range.length != p->producer.range.length)
        return false;
    for (size_t i = 0; i < d.step_count; ++i)
        if (d.steps[i] != certificate.steps[i])
            return false;
    NLCheckedNodeId arg = call->first_argument;
    for (size_t i = 0; i < 3; ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(arm, arg);
        const NLSymbolId parameter = call->owner_call.parameters[i],
                         donor = call->owner_call.donor[i];
        if (v == NULL || v->kind != NL_CHECKED_IDENTIFIER ||
            v->symbol != donor ||
            v->value_use != (i == 0 ? NL_VALUE_COPIED : NL_VALUE_CONSUMED) ||
            v->result_count != 1 ||
            v->results[0].value != call->owner_call.inputs[i] ||
            parameter == 0 || parameter > arm->context->binding_count ||
            donor == 0 || donor > entry->binding_count || parameter == donor ||
            arm->context->bindings[parameter - 1].view.value !=
                call->owner_call.inputs[i] ||
            (i != 0 &&
             (entry->bindings[donor - 1].view.availability != NL_CONSUMED ||
              arm->context->bindings[parameter - 1].view.availability !=
                  NL_CONSUMED ||
              arm->context->bindings[donor - 1].view.availability !=
                  NL_CONSUMED)))
            return false;
        arg = v->next_argument;
    }
    if (arg != 0)
        return false;
    size_t receives = 0;
    for (NLCheckedNodeId i = 1; i <= arm->count; ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(arm, i);
        const NLCheckedNodeView *init =
            nl_checked_node_view(arm, v->initializer);
        if (v->kind == NL_CHECKED_AGGREGATE_BINDING && init != NULL &&
            init->result_count == 1 &&
            init->results[0].value == m->packet_fork.packet &&
            init->kind == NL_CHECKED_IDENTIFIER &&
            init->value_use == NL_VALUE_CONSUMED &&
            init->symbol == m->packet_fork.binding &&
            v->packet_origin.ancestor == f && v->packet_origin.match == match &&
            v->packet_origin.world == arm->context &&
            v->packet_origin.entry_world == receiving_world &&
            v->packet_origin.packet == m->packet_fork.packet) {
            if (v->argument_count != 3)
                return false;
            NLCheckedNodeId r = v->first_argument;
            bool fields[3] = {false, false, false};
            for (size_t j = 0; j < 3; ++j) {
                const NLCheckedNodeView *receiver =
                    nl_checked_node_view(arm, r);
                if (receiver == NULL || receiver->kind != NL_CHECKED_RECEIVER ||
                    receiver->field_index >= 3 ||
                    fields[receiver->field_index] || receiver->symbol == 0 ||
                    receiver->symbol > arm->context->binding_count ||
                    arm->context->bindings[receiver->symbol - 1].view.value !=
                        f->packet_entry->values[m->packet_fork.packet - 1]
                            .fields[receiver->field_index])
                    return false;
                fields[receiver->field_index] = true;
                r = receiver->next_argument;
            }
            if (r != 0)
                return false;
            ++receives;
        }
    }
    return receives == 1;
}

bool nl_packet_arm_closed(const NLCheckedFragment *f, NLCheckedNodeId match,
                          const NLCheckedFragment *arm,
                          const NLCheckedNodeView *result)
{
    if (arm == NULL || arm->packet_parent != f || arm->packet_match != match ||
        !lineage(arm) || f->packet_post == NULL ||
        nl_control_exits_count(arm->exits) != 0 ||
        nl_control_exits_count(arm->loop_returns) != 0 ||
        !nl_allocated_post_matches(f->packet_post, arm->context, result))
        return false;
    return packet_terminal(f, match, arm, arm->packet_entry);
}

bool nl_checked_packet_fork_valid(const NLCheckedFragment *f,
                                  NLCheckedNodeId id)
{
    const NLCheckedNodeView *m = nl_checked_node_view(f, id);
    if (m == NULL || m->kind != NL_CHECKED_MATCH || !m->packet_fork.closed ||
        m->packet_fork.retained || m->normal_frame_unchanged ||
        m->captured_frame_closed || m->normal_arms != 2 || m->item_count != 2 ||
        f->packet_match != id ||
        f->packet_entry != m->packet_fork.entry_world ||
        f->packet_post != m->packet_fork.post_world ||
        f->packet_entry == NULL || f->packet_post == NULL ||
        f->packet_post == f->packet_entry ||
        !current_packet(f, id, f->packet_entry, false) ||
        !nl_checked_producer_valid(f, m->packet_fork.producer) ||
        nl_raw_validate(f->packet_entry) != NL_CHECK_OK ||
        nl_raw_validate(f->packet_post) != NL_CHECK_OK)
        return false;
    bool seen[2] = {false, false};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_match_arm(f, id, i);
        const NLCheckedNodeView *r =
            nl_checked_node_view(arm, nl_checked_root(arm));
        if (r == NULL || r->kind != NL_CHECKED_MATCH_ARM || r->variant < 1 ||
            r->variant > 2 || seen[r->variant - 1] ||
            !nl_packet_arm_closed(f, id, arm, r) ||
            nl_raw_validate(arm->context) != NL_CHECK_OK ||
            nl_raw_validate(arm->packet_entry) != NL_CHECK_OK ||
            nl_raw_validate(arm->owner_entry) != NL_CHECK_OK)
            return false;
        seen[r->variant - 1] = true;
    }
    return true;
}

bool nl_checked_packet_retaining_join_valid(const NLCheckedFragment *f,
                                            NLCheckedNodeId id)
{
    const NLCheckedNodeView *m = nl_checked_node_view(f, id);
    if (m == NULL || m->kind != NL_CHECKED_MATCH || !m->packet_fork.retained ||
        m->packet_fork.closed || m->normal_frame_unchanged ||
        m->captured_frame_closed || m->normal_arms != 2 || m->item_count != 2 ||
        f->packet_match != id || f->packet_entry == NULL ||
        f->packet_retained_post == NULL ||
        m->packet_fork.entry_world != f->packet_entry ||
        m->packet_fork.post_world != f->packet_retained_post ||
        !nl_checked_producer_valid(f, m->packet_fork.producer))
        return false;
    NLSemanticContext *expected = NULL;
    if (nl_packet_retained(f, id, f->packet_entry, &expected) != NL_CHECK_OK)
        return false;
    const bool same = nl_packet_same_entry(expected, f->packet_retained_post);
    nl_semantic_destroy(expected);
    if (!same || nl_raw_validate(f->packet_retained_post) != NL_CHECK_OK)
        return false;
    bool seen[2] = {false, false};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_match_arm(f, id, i);
        const NLCheckedNodeView *r =
            nl_checked_node_view(arm, nl_checked_root(arm));
        if (r == NULL || r->kind != NL_CHECKED_MATCH_ARM || r->variant < 1 ||
            r->variant > 2 || seen[r->variant - 1] ||
            r->packet_origin.ancestor != f || r->packet_origin.match != id ||
            r->packet_origin.packet != m->packet_fork.packet ||
            r->packet_origin.world != arm->context ||
            r->packet_origin.entry_world != arm->packet_entry ||
            !nl_packet_arm_retained(f, id, arm, r) ||
            nl_raw_validate(arm->context) != NL_CHECK_OK)
            return false;
        seen[r->variant - 1] = true;
    }
    return true;
}

/* The retaining join and its later whole receiving/release are separate
 * certificates. Validate the continuation in the parent's own current world. */
bool nl_checked_packet_retention_release_valid(const NLCheckedFragment *f,
                                               NLCheckedNodeId id)
{
    if (!nl_checked_packet_retaining_join_valid(f, id) ||
        !packet_terminal(f, id, f, f->packet_retained_post) ||
        nl_sem_validate(f->context) != NL_CHECK_OK ||
        nl_raw_validate(f->context) != NL_CHECK_OK)
        return false;
    const NLCheckedNodeView *m = nl_checked_node_view(f, id),
                            *p = nl_checked_node_view(f,
                                                      m->packet_fork.producer);
    return !f->context->places[p->producer.root - 1].live &&
           !f->context->regions[p->producer.range.region - 1].view.live &&
           !f->context->domains[p->producer.domain - 1].live &&
           f->context->bindings[m->packet_fork.binding - 1].view.availability ==
               NL_CONSUMED &&
           f->context->values[m->packet_fork.packet - 1].carrier ==
               NL_CARRIER_ENDED;
}
