#include "unchecked_ref_internal.h"

/* This guard concerns only the current explicit source event. Domains may
 * govern many objects in the language; this initial UNKNOWN-target lane
 * refuses an ambiguous anchor rather than inventing a universal identity. */
NLCheckStatus nl_unchecked_domain_target(const NLSemanticContext *c,
                                         NLTypeId type, NLDomainId domain,
                                         NLPlaceId *out)
{
    if (!c || !out || !domain || domain > c->domain_count ||
        !c->domains[domain - 1].live || !type || type > c->type_count ||
        !nl_recursive_local_type(c, type))
        return NL_CHECK_SEMANTIC_ERROR;
    NLPlaceId target = 0;
    for (size_t i = 0; i < c->place_count; ++i) {
        const NLSemanticPlaceView p = c->places[i];
        if (!p.live || !p.independent_root || p.type != type ||
            p.governing_domain != domain)
            continue;
        if (target)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        target = i + 1;
    }
    if (!target)
        return NL_CHECK_SEMANTIC_ERROR;
    const NLSemanticPlaceView p = c->places[target - 1];
    const NLSemanticTypeView layout = c->types[type - 1].view;
    if (!nl_fixed_live(c, target) || p.parent_sum || p.parent_aggregate ||
        !p.placement.region || p.placement.region > c->region_count ||
        !layout.layout_known || !layout.alignment ||
        p.placement.length != layout.size ||
        p.placement.start % layout.alignment ||
        p.fixed_field_count != layout.field_count)
        return NL_CHECK_SEMANTIC_ERROR;
    const NLSemanticBackingView backing =
        c->regions[p.placement.region - 1].view;
    if (!backing.live || !backing.ordinary_read ||
        backing.alignment < layout.alignment ||
        p.placement.start > backing.size ||
        p.placement.length > backing.size - p.placement.start)
        return NL_CHECK_SEMANTIC_ERROR;
    size_t owners = 0;
    const NLTypeId allocation = nl_semantic_core_type(c, NL_TYPE_ALLOCATION);
    for (size_t i = 0; i < c->binding_count; ++i) {
        const NLSemanticBindingView b = c->bindings[i].view;
        if (b.availability == NL_AVAILABLE && b.type == allocation && b.value &&
            b.value <= c->value_count &&
            c->values[b.value - 1].allocation_region == p.placement.region &&
            c->values[b.value - 1].carrier != NL_CARRIER_ENDED)
            ++owners;
    }
    if (owners != 1)
        return NL_CHECK_SEMANTIC_ERROR;
    *out = target;
    return NL_CHECK_OK;
}
