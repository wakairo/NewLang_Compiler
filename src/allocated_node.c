#include "semantic_internal.h"
#include <stdlib.h>
#include <string.h>

NLCheckStatus nl_allocated_write_access(const NLSemanticContext *c,
                                        NLValueId pointer)
{
    if (pointer == 0 || pointer > c->value_count)
        return NL_CHECK_SEMANTIC_ERROR;
    const NLSemanticValueView v = c->values[pointer - 1];
    const NLSemanticTypeView t = c->types[v.type - 1].view;
    const NLReferenceFacts f = v.reference;
    if (v.reference_count != 0 || f.provenance == NL_PROVENANCE_UNKNOWN)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    if (t.kind != NL_TYPE_PTR || !nl_recursive_local_type(c, t.target) ||
        f.provenance != NL_PROVENANCE_VALID || !f.writable || !f.readable ||
        !nl_fixed_live(c, f.place))
        return NL_CHECK_SEMANTIC_ERROR;
    const NLSemanticPlaceView root = c->places[f.place - 1];
    const NLSemanticTypeView h = c->types[t.target - 1].view;
    if (root.type != t.target || root.incarnation != f.incarnation ||
        !root.independent_root || root.parent_aggregate != 0 ||
        root.parent_sum != 0 || root.governing_domain == 0 ||
        !c->domains[root.governing_domain - 1].live ||
        root.placement.region == 0 || root.placement.region > c->region_count ||
        !h.layout_known || h.alignment == 0 ||
        root.placement.length != h.size ||
        root.placement.start % h.alignment != 0)
        return NL_CHECK_SEMANTIC_ERROR;
    const NLSemanticBackingView backing =
        c->regions[root.placement.region - 1].view;
    return backing.live && backing.ordinary_write &&
                   backing.alignment >= h.alignment &&
                   root.placement.start <= backing.size &&
                   root.placement.length <= backing.size - root.placement.start
               ? NL_CHECK_OK
               : NL_CHECK_SEMANTIC_ERROR;
}

/* Private candidate operations. The caller owns clone/rollback on any failure.
 * No host pointer, address, allocation event or live authority at registration.
 * The existing Linux x86_64 execution target uses one word tag + one pointer
 * word per link, followed by u8 and alignment padding: private size 24 for
 * one link, 56 for three links, alignment 8. The three-link plan is used only
 * by this semantic gate; no native three-link lowering is enabled here.
 * This is a target representation plan, never a source ABI/layout guarantee. */
static NLCheckStatus string_copy(const char *text, char **out)
{
    *out = malloc(strlen(text) + 1);
    if (*out == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    memcpy(*out, text, strlen(text) + 1);
    return NL_CHECK_OK;
}
NLCheckStatus nl_allocated_registry(NLSemanticContext *c, NLTypeId h,
                                    NLTypeId *option)
{
    if (!nl_recursive_local_type(c, h))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
#if !defined(__linux__) || !defined(__x86_64__)
    return NL_CHECK_SEMANTIC_UNSUPPORTED;
#endif
    for (size_t i = 0; i < c->type_count; ++i) {
        if (c->types[i].allocated_target != 0) {
            if (c->types[i].allocated_target != h)
                return NL_CHECK_SEMANTIC_UNSUPPORTED;
            *option = i + 1;
            return NL_CHECK_OK;
        }
    }
    const size_t size = c->types[h - 1].view.field_count == 4 ? 56 : 24;
    if (c->types[h - 1].view.layout_known &&
        (c->types[h - 1].view.size != size ||
         c->types[h - 1].view.alignment != 8))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    c->types[h - 1].view.layout_known = true;
    c->types[h - 1].view.size = size;
    c->types[h - 1].view.alignment = 8;
    NLTypeId bundle, sum;
    NLCheckStatus s = nl_sem_nominal(c, "OneBacking", false, false, &bundle);
    if (s != NL_CHECK_OK)
        return s;
    c->types[bundle - 1].one_backing_target = h;
    c->types[bundle - 1].view.field_count = 2;
    c->types[bundle - 1].field_types[0] =
        nl_semantic_core_type(c, NL_TYPE_ALLOCATION);
    c->types[bundle - 1].field_types[1] =
        nl_semantic_core_type(c, NL_TYPE_STORAGE);
    s = string_copy("allocation", &c->types[bundle - 1].field_names[0]);
    if (s == NL_CHECK_OK)
        s = string_copy("raw", &c->types[bundle - 1].field_names[1]);
    if (s != NL_CHECK_OK)
        return s;
    s = nl_sem_nominal(c, "<allocated-option>", false, false, &sum);
    if (s != NL_CHECK_OK)
        return s;
    c->types[sum - 1].allocated_target = h;
    c->types[sum - 1].view.kind = NL_TYPE_SUM;
    c->types[sum - 1].view.variant_count = 2;
    c->types[sum - 1].variant_types[1] = bundle;
    s = string_copy("None", &c->types[sum - 1].variant_names[0]);
    if (s == NL_CHECK_OK)
        s = string_copy("Some", &c->types[sum - 1].variant_names[1]);
    if (s == NL_CHECK_OK)
        *option = sum;
    return s;
}
NLCheckStatus nl_allocated_grant(NLSemanticContext *c, NLTypeId option,
                                 bool success, NLCheckedNodeView *event,
                                 NLCheckDiagnostic *diagnostic)
{
    NLTypeId h = c->types[option - 1].allocated_target;
    if (h == 0 || !nl_recursive_local_type(c, h))
        return NL_CHECK_INTERNAL_ERROR;
    event->kind = NL_CHECKED_TRY_ALLOCATE_ONE;
    event->type = option;
    event->allocation_trial = true;
    event->allocation_success = success;
    event->allocation_target = h;
    event->allocation_size = c->types[h - 1].view.size;
    event->allocation_alignment = c->types[h - 1].view.alignment;
    NLValueId bundle = 0;
    if (success) {
        NLRawOperation op = {
            .kind = NL_RAW_ALLOCATE,
            .span = event->span,
            .data.allocate = {.size = {true, event->allocation_size},
                              .alignment = {true, event->allocation_alignment},
                              .ordinary_read = true,
                              .ordinary_write = true}};
        NLCheckedNodeView raw = {.kind = NL_CHECKED_ALLOCATE};
        const NLValueId inputs[2] = {0};
        NLCheckStatus s = nl_raw_apply(c, &op, inputs, &raw, diagnostic);
        if (s != NL_CHECK_OK)
            return s;
        event->allocation_authority = raw.results[0].value;
        event->storage_authority = raw.results[1].value;
        event->backing =
            c->values[event->allocation_authority - 1].allocation_region;
        NLTypeId type = c->types[option - 1].variant_types[1];
        s = nl_sem_new_value(
            c,
            (NLSemanticValueView){.type = type,
                                  .field_count = 2,
                                  .fields = {event->allocation_authority,
                                             event->storage_authority}},
            &bundle);
        if (s != NL_CHECK_OK)
            return s;
        for (size_t i = 0; i < 2; ++i) {
            NLValueId member = c->values[bundle - 1].fields[i];
            c->values[member - 1].carrier = NL_CARRIER_AGGREGATE;
            c->values[member - 1].aggregate_owner = bundle;
        }
    }
    NLValueId sum;
    NLCheckStatus s = nl_sem_new_value(
        c,
        (NLSemanticValueView){
            .type = option, .variant = success ? 2 : 1, .sum_payload = bundle},
        &sum);
    if (s != NL_CHECK_OK)
        return s;
    if (bundle != 0) {
        c->values[bundle - 1].carrier = NL_CARRIER_SUM;
        c->values[bundle - 1].sum_owner = sum;
    }
    event->result_count = 1;
    event->results[0] = (NLCheckedResult){option, sum};
    return NL_CHECK_OK;
}

/* Type registration only: no concrete root, region, domain, value or grant. */
NLCheckStatus nl_custody_registry(NLSemanticContext *c, NLTypeId h,
                                  NLTypeId *out)
{
    NLTypeId packet = 0;
    NLCheckStatus status = nl_live_tail_registry(c, h, &packet);
    if (status != NL_CHECK_OK)
        return status;
    for (size_t i = 0; i < c->type_count; ++i)
        if (c->types[i].option_target == packet) {
            *out = i + 1;
            return NL_CHECK_OK;
        }
    NLTypeId sum = 0;
    status = nl_sem_nominal(c, "<custody-option>", false, false, &sum);
    if (status != NL_CHECK_OK)
        return status;
    c->types[sum - 1].option_target = packet;
    c->types[sum - 1].view.kind = NL_TYPE_SUM;
    c->types[sum - 1].view.variant_count = 2;
    c->types[sum - 1].variant_types[1] = packet;
    status = string_copy("None", &c->types[sum - 1].variant_names[0]);
    if (status == NL_CHECK_OK)
        status = string_copy("Some", &c->types[sum - 1].variant_names[1]);
    if (status == NL_CHECK_OK)
        *out = sum;
    return status;
}

NLCheckStatus nl_live_tail_registry(NLSemanticContext *c, NLTypeId h,
                                    NLTypeId *out)
{
    if (!nl_recursive_local_type(c, h))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    for (size_t i = 0; i < c->type_count; ++i) {
        if (c->types[i].live_tail_target == h) {
            *out = i + 1;
            return NL_CHECK_OK;
        }
        if (c->types[i].name != NULL &&
            strcmp(c->types[i].name, "LiveTail") == 0)
            return NL_CHECK_SEMANTIC_ERROR;
    }
    for (size_t i = 0; i < c->function_count; ++i)
        if (strcmp(c->functions[i].name, "LiveTail") == 0)
            return NL_CHECK_SEMANTIC_ERROR;
    for (size_t i = 0; i < c->binding_count; ++i)
        if (!c->bindings[i].hidden &&
            strcmp(c->bindings[i].name, "LiveTail") == 0)
            return NL_CHECK_SEMANTIC_ERROR;
    NLTypeId ptr, t;
    NLCheckStatus s =
        nl_sem_compound(c, NL_TYPE_PTR, h, NL_ACCESS_READ, false, &ptr);
    if (s != NL_CHECK_OK)
        return s;
    s = nl_sem_nominal(c, "LiveTail", false, false, &t);
    if (s != NL_CHECK_OK)
        return s;
    c->types[t - 1].live_tail_target = h;
    c->types[t - 1].view.field_count = 3;
    const NLTypeId fields[] = {ptr,
                               nl_semantic_core_type(c, NL_TYPE_ALLOCATION),
                               nl_semantic_domain_type(c)};
    const char *names[] = {"owned_ptr", "owned_allocation", "owned_domain"};
    for (size_t i = 0; i < 3; ++i) {
        c->types[t - 1].field_types[i] = fields[i];
        s = string_copy(names[i], &c->types[t - 1].field_names[i]);
        if (s != NL_CHECK_OK)
            return s;
    }
    *out = t;
    return NL_CHECK_OK;
}
