#include "semantic_internal.h"
#include <stdint.h>
#include <string.h>

/* Exactly the completed §16.3 shape, selected by nominal metadata, not its
 * source name. The committed recursive link is declaration index zero. */
bool nl_recursive_local_type(const NLSemanticContext *c, NLTypeId type)
{
    if (type == 0 || type > c->type_count)
        return false;
    const NLTypeEntry *t = &c->types[type - 1];
    if (!t->recursive_header || t->incomplete ||
        t->view.kind != NL_TYPE_NOMINAL || t->view.field_count != 2 ||
        !t->view.is_copy || !t->view.is_discardable)
        return false;
    const NLTypeId link = t->field_types[0];
    if (link == 0 || link > c->type_count)
        return false;
    const NLTypeId ptr = c->types[link - 1].option_target;
    return ptr != 0 && ptr <= c->type_count &&
           c->types[link - 1].view.kind == NL_TYPE_SUM &&
           c->types[ptr - 1].view.kind == NL_TYPE_PTR &&
           c->types[ptr - 1].view.target == type &&
           t->field_types[1] == nl_semantic_core_type(c, NL_TYPE_U8);
}

bool nl_fixed_type(const NLSemanticContext *c, NLTypeId type)
{
    if (nl_recursive_local_type(c, type))
        return true;
    if (type == 0 || type > c->type_count)
        return false;
    const NLTypeEntry *t = &c->types[type - 1];
    if (t->name == NULL || strcmp(t->name, "Pair") != 0 ||
        t->view.kind != NL_TYPE_NOMINAL || t->view.field_count != 2 ||
        !t->view.is_copy || !t->view.is_discardable)
        return false;
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    return t->field_types[0] == u8 && t->field_types[1] == u8 &&
           ((strcmp(t->field_names[0], "left") == 0 &&
             strcmp(t->field_names[1], "right") == 0) ||
            (strcmp(t->field_names[0], "right") == 0 &&
             strcmp(t->field_names[1], "left") == 0));
}

NLCheckStatus nl_fixed_attach(NLSemanticContext *c, NLPlaceId root)
{
    const NLSemanticPlaceView p = c->places[root - 1];
    if (!nl_fixed_type(c, p.type))
        return NL_CHECK_OK;
    const NLSemanticValueView v = c->values[p.current_value - 1];
    if (v.field_count != 2 || p.fixed_field_count != 0)
        return NL_CHECK_INTERNAL_ERROR;
    for (size_t i = 0; i < 2; ++i) {
        NLPlaceId child;
        NLCheckStatus s =
            nl_sem_new_place(c, c->types[p.type - 1].field_types[i],
                             p.governing_domain, false, 0, &child);
        if (s != NL_CHECK_OK)
            return s;
        if (c->last_incarnation == SIZE_MAX)
            return NL_CHECK_RESOURCE_LIMIT;
        NLValueFactId fact;
        s = nl_sem_fresh_fact(c, &fact);
        if (s != NL_CHECK_OK)
            return s;
        c->places[child - 1] =
            (NLSemanticPlaceView){.type = c->types[p.type - 1].field_types[i],
                                  .live = true,
                                  .governing_domain = p.governing_domain,
                                  .incarnation = ++c->last_incarnation,
                                  .current_fact = fact,
                                  .current_value = v.fields[i],
                                  .parent_aggregate = root,
                                  .parent_incarnation = p.incarnation,
                                  .parent_field_index = i};
        c->places[root - 1].fixed_fields[i] = child;
        ++c->places[root - 1].fixed_field_count;
        s = nl_sum_attach(c, child);
        if (s != NL_CHECK_OK)
            return s;
    }
    return NL_CHECK_OK;
}

void nl_fixed_detach(NLSemanticContext *c, NLPlaceId root)
{
    for (size_t i = 0; i < c->places[root - 1].fixed_field_count; ++i) {
        const NLPlaceId id = c->places[root - 1].fixed_fields[i];
        nl_sum_detach(c, id);
        c->places[id - 1].live = false;
        c->places[id - 1].current_value = 0;
        c->places[id - 1].current_fact = 0;
    }
    c->places[root - 1].fixed_field_count = 0;
    memset(c->places[root - 1].fixed_fields, 0,
           sizeof(c->places[root - 1].fixed_fields));
}

static NLPlaceId parent(const NLSemanticContext *c, NLPlaceId id)
{
    const NLSemanticPlaceView p = c->places[id - 1];
    return p.parent_aggregate != 0 ? p.parent_aggregate : p.parent_sum;
}
static bool contains(const NLSemanticContext *c, NLPlaceId root, NLPlaceId id)
{
    for (size_t n = 0; id != 0 && id <= c->place_count && n <= c->place_count;
         ++n) {
        if (id == root)
            return true;
        id = parent(c, id);
    }
    return false;
}
bool nl_fixed_overlap(const NLSemanticContext *c, NLPlaceId a, NLPlaceId b)
{
    if (a == 0 || b == 0 || a > c->place_count || b > c->place_count)
        return true; /* unknown never establishes disjointness */
    return contains(c, a, b) || contains(c, b, a);
}
bool nl_fixed_live(const NLSemanticContext *c, NLPlaceId id)
{
    if (id == 0 || id > c->place_count || !c->places[id - 1].live)
        return false;
    const NLSemanticPlaceView p = c->places[id - 1];
    if (p.parent_aggregate == 0)
        return true;
    const NLPlaceId root = p.parent_aggregate;
    if (root > c->place_count)
        return false;
    const NLSemanticPlaceView r = c->places[root - 1];
    return r.live && r.incarnation == p.parent_incarnation &&
           p.parent_field_index < r.fixed_field_count &&
           r.fixed_fields[p.parent_field_index] == id;
}

NLCheckStatus nl_fixed_dependencies(const NLSemanticContext *c)
{
    for (size_t i = 0; i < c->value_count; ++i) {
        const NLSemanticValueView v = c->values[i];
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        if (v.dependencies == NL_HIDDEN_DEPENDENCIES ||
            v.dependencies == NL_DEPENDENCIES_UNKNOWN)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        if (v.dependencies == NL_DEPENDENCY_FREE) {
            if (v.value_dependency_count != 0)
                return NL_CHECK_INTERNAL_ERROR;
            continue;
        }
        if (v.dependencies != NL_EXACT_VALUE_DEPENDENCIES ||
            v.value_dependency_count == 0)
            return NL_CHECK_INTERNAL_ERROR;
        if (v.value_dependency_count > NL_SEMANTIC_MAX_VALUE_DEPENDENCIES)
            return NL_CHECK_RESOURCE_LIMIT;
        for (size_t j = 0; j < v.value_dependency_count; ++j) {
            NLValueDependency d = v.value_dependencies[j];
            if (!nl_fixed_live(c, d.place) || d.fact == 0 ||
                c->places[d.place - 1].current_fact != d.fact)
                return NL_CHECK_SEMANTIC_ERROR;
        }
    }
    return NL_CHECK_OK;
}

NLCheckStatus nl_fixed_change_dependencies(const NLSemanticContext *c,
                                           NLPlaceId place, NLValueId discarded)
{
    NLCheckStatus status = nl_fixed_dependencies(c);
    if (status != NL_CHECK_OK)
        return status;
    for (size_t i = 0; i < c->value_count; ++i) {
        const NLSemanticValueView v = c->values[i];
        if (v.carrier == NL_CARRIER_ENDED || i + 1 == discarded)
            continue;
        for (size_t j = 0; j < v.value_dependency_count; ++j)
            if (nl_fixed_overlap(c, place, v.value_dependencies[j].place))
                return NL_CHECK_SEMANTIC_ERROR;
    }
    return NL_CHECK_OK;
}

NLCheckStatus nl_fixed_end_dependencies(const NLSemanticContext *c,
                                        NLPlaceId place, size_t floor)
{
    for (size_t i = 0; i < c->value_count; ++i) {
        const NLSemanticValueView v = c->values[i];
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        NLValueId owner = i + 1;
        for (size_t n = 0;
             c->values[owner - 1].aggregate_owner != 0 && n < c->value_count;
             ++n)
            owner = c->values[owner - 1].aggregate_owner;
        const NLPlaceId p = c->values[owner - 1].owner_place;
        if (p != 0 && contains(c, place, p))
            continue; /* package is unconditionally ended with root */
        bool ending = false;
        for (size_t b = floor; b < c->binding_count; ++b)
            if (p != 0 && c->bindings[b].view.place == p &&
                c->types[c->bindings[b].view.type - 1].view.is_discardable)
                ending = true;
        if (ending)
            continue;
        for (size_t j = 0; j < v.value_dependency_count; ++j)
            if (contains(c, place, v.value_dependencies[j].place))
                return NL_CHECK_SEMANTIC_ERROR;
    }
    return NL_CHECK_OK;
}

NLCheckStatus nl_fixed_change(NLSemanticContext *c, NLPlaceId child,
                              NLValueId incoming, bool discard)
{
    const NLSemanticPlaceView p = c->places[child - 1];
    const NLPlaceId root = p.parent_aggregate;
    if (!nl_fixed_live(c, child) || root == 0 || incoming == 0 ||
        incoming > c->value_count ||
        c->values[incoming - 1].carrier != NL_CARRIER_LOOSE)
        return NL_CHECK_INTERNAL_ERROR;
    const NLValueId old = c->places[root - 1].current_value;
    NLSemanticValueView next = c->values[old - 1];
    next.fields[p.parent_field_index] = incoming;
    NLValueId id;
    NLCheckStatus s = nl_sem_new_value(c, next, &id);
    if (s != NL_CHECK_OK)
        return s;
    NLValueFactId cf, rf;
    if ((s = nl_sem_fresh_fact(c, &cf)) != NL_CHECK_OK ||
        (s = nl_sem_fresh_fact(c, &rf)) != NL_CHECK_OK)
        return s;
    /* The aggregate remains the sole owner of the Option package. Its fixed
     * child hosts a conditional occurrence, which whole-Option Change ends. */
    nl_sum_detach(c, child);
    c->values[id - 1].carrier = NL_CARRIER_PLACE;
    c->values[id - 1].owner_place = root;
    for (size_t i = 0; i < next.field_count; ++i) {
        const NLValueId member = next.fields[i];
        c->values[member - 1].carrier = NL_CARRIER_AGGREGATE;
        c->values[member - 1].aggregate_owner = id;
        c->values[member - 1].owner_place = 0;
    }
    c->values[p.current_value - 1].carrier =
        discard ? NL_CARRIER_ENDED : NL_CARRIER_LOOSE;
    c->values[p.current_value - 1].aggregate_owner = 0;
    c->values[p.current_value - 1].owner_place = 0;
    if (discard)
        nl_sem_end_value(c, p.current_value);
    c->values[old - 1].carrier =
        NL_CARRIER_ENDED; /* transfer, not recursive end */
    c->values[old - 1].owner_place = 0;
    c->places[root - 1].current_value = id;
    c->places[root - 1].current_fact = rf;
    c->places[child - 1].current_value = incoming;
    c->places[child - 1].current_fact = cf;
    return nl_sum_attach(c, child);
}

NLCheckStatus nl_fixed_validate(const NLSemanticContext *c)
{
    for (size_t i = 0; i < c->value_count; ++i) {
        const NLSemanticValueView a = c->values[i];
        if (a.carrier == NL_CARRIER_ENDED || a.field_count == 0)
            continue;
        if (a.field_count > NL_SEMANTIC_MAX_FIELDS ||
            a.field_count != c->types[a.type - 1].view.field_count)
            return NL_CHECK_INTERNAL_ERROR;
        for (size_t f = 0; f < a.field_count; ++f) {
            if (a.fields[f] == 0 || a.fields[f] > c->value_count)
                return NL_CHECK_INTERNAL_ERROR;
            const NLSemanticValueView m = c->values[a.fields[f] - 1];
            if (m.carrier != NL_CARRIER_AGGREGATE ||
                m.aggregate_owner != i + 1 || m.owner_place != 0 ||
                m.type != c->types[a.type - 1].field_types[f])
                return NL_CHECK_INTERNAL_ERROR;
            for (size_t j = 0; j < f; ++j)
                if (a.fields[j] == a.fields[f])
                    return NL_CHECK_INTERNAL_ERROR;
        }
    }
    for (size_t i = 0; i < c->place_count; ++i) {
        const NLSemanticPlaceView p = c->places[i];
        if (p.live && nl_fixed_type(c, p.type) && p.fixed_field_count != 2)
            return NL_CHECK_INTERNAL_ERROR;
        if (p.parent_aggregate == 0)
            continue;
        if (!p.live)
            continue;
        if (!nl_fixed_live(c, i + 1) || p.independent_root || p.parent_sum != 0)
            return NL_CHECK_INTERNAL_ERROR;
        const NLSemanticPlaceView r = c->places[p.parent_aggregate - 1];
        const NLSemanticValueView a = c->values[r.current_value - 1];
        if (a.carrier != NL_CARRIER_PLACE ||
            a.owner_place != p.parent_aggregate ||
            a.field_count != r.fixed_field_count ||
            p.type != c->types[r.type - 1].field_types[p.parent_field_index] ||
            p.current_value != a.fields[p.parent_field_index] ||
            p.governing_domain != r.governing_domain)
            return NL_CHECK_INTERNAL_ERROR;
        const NLSemanticValueView m = c->values[p.current_value - 1];
        if (m.carrier != NL_CARRIER_AGGREGATE ||
            m.aggregate_owner != r.current_value || m.owner_place != 0 ||
            m.type != p.type)
            return NL_CHECK_INTERNAL_ERROR;
    }
    return nl_fixed_dependencies(c);
}
NLCheckStatus nl_sem_validate(const NLSemanticContext *c)
{
    NLCheckStatus status = nl_fixed_validate(c);
    return status == NL_CHECK_OK ? nl_sum_validate(c) : status;
}
