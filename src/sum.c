#include "semantic_internal.h"

#include <stdlib.h>

bool nl_sum_authority(const NLSemanticContext *c, NLTypeId type)
{
    if (type == 2 || c->types[type - 1].one_backing_target != 0)
        return true;
    const NLSemanticTypeView t = c->types[type - 1].view;
    if (t.kind == NL_TYPE_SLOT || t.kind == NL_TYPE_STORAGE ||
        t.kind == NL_TYPE_ALLOCATION)
        return true;
    if (t.kind == NL_TYPE_SUM)
        for (size_t i = 0; i < t.variant_count; ++i) {
            NLTypeId p = c->types[type - 1].variant_types[i];
            if (p != 0 && nl_sum_authority(c, p))
                return true;
        }
    return false;
}

NLCheckStatus nl_sum_attach(NLSemanticContext *c, NLPlaceId root)
{
    const NLSemanticPlaceView r = c->places[root - 1];
    if (c->types[r.type - 1].view.kind != NL_TYPE_SUM)
        return NL_CHECK_OK;
    const NLSemanticValueView v = c->values[r.current_value - 1];
    if (v.variant == 0)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    if (v.sum_payload == 0)
        return NL_CHECK_OK;
    if (c->occurrence_count >= NL_SEMANTIC_MAX_ENTRIES)
        return NL_CHECK_RESOURCE_LIMIT;
    NLSemanticOccurrenceView *storage =
        realloc(c->occurrences, (c->occurrence_count + 1) * sizeof(*storage));
    if (storage == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    c->occurrences = storage;
    NLCheckStatus status;
    /* Reserve history before child allocation; candidate rollback owns failure.
     */
    const NLOccurrenceId occurrence = ++c->occurrence_count;
    c->occurrences[occurrence - 1] = (NLSemanticOccurrenceView){0};
    NLPlaceId child;
    status = nl_sem_new_place(c, c->values[v.sum_payload - 1].type,
                              r.governing_domain, false, v.sum_payload, &child);
    if (status != NL_CHECK_OK)
        return status;
    c->places[child - 1].parent_sum = root;
    c->places[root - 1].payload_occurrence = occurrence;
    c->occurrences[occurrence - 1] =
        (NLSemanticOccurrenceView){true, root, child, v.variant};
    c->values[v.sum_payload - 1].sum_owner = r.current_value;
    return NL_CHECK_OK;
}

void nl_sum_detach(NLSemanticContext *c, NLPlaceId root)
{
    NLOccurrenceId o = c->places[root - 1].payload_occurrence;
    if (o == 0)
        return;
    NLSemanticOccurrenceView *e = &c->occurrences[o - 1];
    NLPlaceId child = e->payload_place;
    NLValueId payload = c->places[child - 1].current_value;
    e->live = false;
    c->places[child - 1].live = false;
    c->places[child - 1].current_value = 0;
    c->places[child - 1].current_fact = 0;
    c->values[payload - 1].carrier = NL_CARRIER_SUM;
    c->values[payload - 1].owner_place = 0;
    c->places[root - 1].payload_occurrence = 0;
}

NLCheckStatus nl_sum_payload_changed(NLSemanticContext *c, NLPlaceId child)
{
    NLPlaceId root = c->places[child - 1].parent_sum;
    if (root == 0)
        return NL_CHECK_OK;
    NLValueId old = c->places[root - 1].current_value, next;
    NLCheckStatus status = nl_sem_new_value(c, c->values[old - 1], &next);
    if (status != NL_CHECK_OK)
        return status;
    NLValueFactId fact;
    status = nl_sem_fresh_fact(c, &fact);
    if (status != NL_CHECK_OK)
        return status;
    c->values[next - 1].sum_payload = c->places[child - 1].current_value;
    c->values[next - 1].carrier = NL_CARRIER_PLACE;
    c->values[next - 1].owner_place = root;
    c->values[c->places[child - 1].current_value - 1].sum_owner = next;
    c->values[old - 1].carrier =
        NL_CARRIER_ENDED; /* member is transferred, not ended */
    c->values[old - 1].owner_place = 0;
    c->places[root - 1].current_value = next;
    c->places[root - 1].current_fact = fact;
    return NL_CHECK_OK;
}

/* Structural ownership/occurrence invariant for committed states. Historical
 * ended packages may retain inspection IDs; only live carriers own members. */
NLCheckStatus nl_sum_validate(const NLSemanticContext *c)
{
    for (size_t i = 0; i < c->value_count; ++i) {
        const NLSemanticValueView v = c->values[i];
        const NLTypeEntry *t = &c->types[v.type - 1];
        if (v.carrier == NL_CARRIER_ENDED || t->view.kind != NL_TYPE_SUM)
            continue;
        if (v.variant == 0 || v.variant > t->view.variant_count)
            return NL_CHECK_INTERNAL_ERROR;
        NLTypeId payload_type = t->variant_types[v.variant - 1];
        if ((payload_type == 0) != (v.sum_payload == 0))
            return NL_CHECK_INTERNAL_ERROR;
        if (payload_type != 0) {
            if (v.sum_payload > c->value_count)
                return NL_CHECK_INTERNAL_ERROR;
            const NLSemanticValueView p = c->values[v.sum_payload - 1];
            if (p.type != payload_type || p.sum_owner != i + 1 ||
                (p.carrier != NL_CARRIER_PLACE && p.carrier != NL_CARRIER_SUM))
                return NL_CHECK_INTERNAL_ERROR;
            if (v.carrier == NL_CARRIER_LOOSE && p.carrier != NL_CARRIER_SUM)
                return NL_CHECK_INTERNAL_ERROR;
        }
    }
    for (size_t i = 0; i < c->place_count; ++i) {
        const NLSemanticPlaceView r = c->places[i];
        if (c->types[r.type - 1].view.kind != NL_TYPE_SUM || !r.live)
            continue;
        const NLSemanticValueView v = c->values[r.current_value - 1];
        /* A fixed-field place is a borrowed view of an aggregate-owned sum,
         * not a second package owner. Validate the exact ownership path. */
        const bool field_owned =
            r.parent_aggregate != 0 && nl_fixed_live(c, i + 1) &&
            nl_recursive_local_type(c,
                                    c->places[r.parent_aggregate - 1].type) &&
            r.parent_field_index == 0 && v.carrier == NL_CARRIER_AGGREGATE &&
            v.owner_place == 0 &&
            v.aggregate_owner ==
                c->places[r.parent_aggregate - 1].current_value &&
            c->values[v.aggregate_owner - 1].fields[0] == r.current_value;
        if (v.type != r.type ||
            (!field_owned &&
             (v.carrier != NL_CARRIER_PLACE || v.owner_place != i + 1)))
            return NL_CHECK_INTERNAL_ERROR;
        if ((v.sum_payload == 0) != (r.payload_occurrence == 0))
            return NL_CHECK_INTERNAL_ERROR;
        if (r.payload_occurrence != 0) {
            if (r.payload_occurrence > c->occurrence_count)
                return NL_CHECK_INTERNAL_ERROR;
            const NLSemanticOccurrenceView o =
                c->occurrences[r.payload_occurrence - 1];
            if (!o.live || o.root != i + 1 || o.variant != v.variant ||
                o.payload_place == 0 || o.payload_place > c->place_count)
                return NL_CHECK_INTERNAL_ERROR;
            const NLSemanticPlaceView p = c->places[o.payload_place - 1];
            if (!p.live || p.independent_root || p.parent_sum != i + 1 ||
                p.current_value != v.sum_payload || p.placement.region != 0 ||
                p.governing_domain != r.governing_domain)
                return NL_CHECK_INTERNAL_ERROR;
        }
    }
    for (size_t i = 0; i < c->occurrence_count; ++i) {
        const NLSemanticOccurrenceView o = c->occurrences[i];
        if (o.live && (o.root == 0 || o.root > c->place_count ||
                       !c->places[o.root - 1].live ||
                       c->places[o.root - 1].payload_occurrence != i + 1))
            return NL_CHECK_INTERNAL_ERROR;
    }
    return NL_CHECK_OK;
}
