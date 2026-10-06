#include "semantic_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static NLCheckStatus append_storage(void *old, size_t count, size_t size,
                                    void **out)
{
    if (count >= NL_SEMANTIC_MAX_ENTRIES || size > SIZE_MAX / (count + 1)) {
        return NL_CHECK_RESOURCE_LIMIT;
    }
    void *const replacement = realloc(old, (count + 1) * size);
    if (replacement == NULL) {
        return NL_CHECK_OUT_OF_MEMORY;
    }
    *out = replacement;
    return NL_CHECK_OK;
}

static NLCheckStatus copy_name(const char *name, char **out)
{
    const size_t length = strlen(name);
    if (length >= (size_t)PTRDIFF_MAX) {
        return NL_CHECK_RESOURCE_LIMIT;
    }
    char *const copy = malloc(length + 1);
    if (copy == NULL) {
        return NL_CHECK_OUT_OF_MEMORY;
    }
    memcpy(copy, name, length + 1);
    *out = copy;
    return NL_CHECK_OK;
}

static NLCheckStatus copy_array(const void *source, size_t count, size_t size,
                                void **out)
{
    if (count == 0) {
        *out = NULL;
        return NL_CHECK_OK;
    }
    void *const copy =
        malloc(count * size); /* counts bounded by module contract */
    if (copy == NULL) {
        return NL_CHECK_OUT_OF_MEMORY;
    }
    memcpy(copy, source, count * size);
    *out = copy;
    return NL_CHECK_OK;
}

void nl_semantic_destroy(NLSemanticContext *context)
{
    if (context == NULL) {
        return;
    }
    for (size_t i = 0; i < context->type_count; ++i) {
        free(context->types[i].name);
        for (size_t v = 0; v < context->types[i].view.variant_count; ++v)
            free(context->types[i].variant_names[v]);
        for (size_t f = 0; f < context->types[i].view.field_count; ++f) {
            free(context->types[i].field_names[f]);
        }
    }
    for (size_t i = 0; i < context->binding_count; ++i) {
        free(context->bindings[i].name);
    }
    for (size_t i = 0; i < context->function_count; ++i) {
        if (context->functions[i].kind == NL_CHECKED_REGISTERED_CALL) {
            free((void *)context->functions[i].name);
            free(context->functions[i].parameters);
            nl_body_release(context->functions[i].body);
        }
    }
    free(context->occurrences);
    free(context->types);
    free(context->bindings);
    free(context->values);
    free(context->places);
    free(context->domains);
    free(context->scopes);
    free(context->functions);
    nl_raw_dispose(context);
    free(context);
}

NLCheckStatus nl_sem_clone(const NLSemanticContext *source,
                           NLSemanticContext **out)
{
    NLSemanticContext *const copy = malloc(sizeof(*copy));
    if (copy == NULL) {
        return NL_CHECK_OUT_OF_MEMORY;
    }
    *copy = (NLSemanticContext){.last_incarnation = source->last_incarnation,
                                .last_value_fact = source->last_value_fact};
    NLCheckStatus status = NL_CHECK_OK;
    void *storage = NULL;
    if (source->type_count != 0) {
        copy->types = malloc(source->type_count * sizeof(*copy->types));
        if (copy->types == NULL) {
            status = NL_CHECK_OUT_OF_MEMORY;
            goto failure;
        }
        for (size_t i = 0; i < source->type_count; ++i) {
            copy->types[i] = (NLTypeEntry){.view = source->types[i].view};
            memcpy(copy->types[i].field_types, source->types[i].field_types,
                   sizeof(copy->types[i].field_types));
            memcpy(copy->types[i].variant_types, source->types[i].variant_types,
                   sizeof(copy->types[i].variant_types));
            ++copy->type_count;
            for (size_t v = 0; v < source->types[i].view.variant_count; ++v) {
                status = copy_name(source->types[i].variant_names[v],
                                   &copy->types[i].variant_names[v]);
                if (status != NL_CHECK_OK)
                    goto failure;
            }
            for (size_t f = 0; f < source->types[i].view.field_count; ++f) {
                status = copy_name(source->types[i].field_names[f],
                                   &copy->types[i].field_names[f]);
                if (status != NL_CHECK_OK)
                    goto failure;
            }
            if (source->types[i].name != NULL &&
                (status = copy_name(source->types[i].name,
                                    &copy->types[i].name)) != NL_CHECK_OK) {
                goto failure;
            }
        }
    }
    if (source->binding_count != 0) {
        copy->bindings =
            malloc(source->binding_count * sizeof(*copy->bindings));
        if (copy->bindings == NULL) {
            status = NL_CHECK_OUT_OF_MEMORY;
            goto failure;
        }
        for (size_t i = 0; i < source->binding_count; ++i) {
            copy->bindings[i] =
                (NLBindingEntry){.view = source->bindings[i].view,
                                 .hidden = source->bindings[i].hidden};
            ++copy->binding_count;
            if ((status = copy_name(source->bindings[i].name,
                                    &copy->bindings[i].name)) != NL_CHECK_OK) {
                goto failure;
            }
        }
    }
    if ((status = copy_array(source->values, source->value_count,
                             sizeof(*copy->values), &storage)) != NL_CHECK_OK) {
        goto failure;
    }
    copy->values = storage;
    copy->value_count = source->value_count;
    if ((status = copy_array(source->places, source->place_count,
                             sizeof(*copy->places), &storage)) != NL_CHECK_OK) {
        goto failure;
    }
    copy->places = storage;
    copy->place_count = source->place_count;
    if ((status = copy_array(source->domains, source->domain_count,
                             sizeof(*copy->domains), &storage)) !=
        NL_CHECK_OK) {
        goto failure;
    }
    copy->domains = storage;
    copy->domain_count = source->domain_count;
    if ((status = copy_array(source->scopes, source->scope_count,
                             sizeof(*copy->scopes), &storage)) != NL_CHECK_OK) {
        goto failure;
    }
    copy->scopes = storage;
    copy->scope_count = source->scope_count;
    if (source->function_count != 0) {
        copy->functions =
            malloc(source->function_count * sizeof(*copy->functions));
        if (copy->functions == NULL) {
            status = NL_CHECK_OUT_OF_MEMORY;
            goto failure;
        }
        for (size_t i = 0; i < source->function_count; ++i) {
            const NLFunctionEntry *const original = &source->functions[i];
            copy->functions[i] = *original;
            ++copy->function_count;
            if (original->kind == NL_CHECKED_REGISTERED_CALL) {
                copy->functions[i].name = NULL;
                copy->functions[i].parameters = NULL;
                copy->functions[i].body = NULL;
                if (original->body != NULL) {
                    status = nl_body_retain(original->body);
                    if (status != NL_CHECK_OK)
                        goto failure;
                    copy->functions[i].body = original->body;
                }
                char *name = NULL;
                status = copy_name(original->name, &name);
                copy->functions[i].name = name;
                if (status != NL_CHECK_OK) {
                    goto failure;
                }
                status = copy_array(original->parameters, original->count,
                                    sizeof(*original->parameters), &storage);
                if (status != NL_CHECK_OK) {
                    goto failure;
                }
                copy->functions[i].parameters = storage;
            }
        }
    }
    status = copy_array(source->occurrences, source->occurrence_count,
                        sizeof(*copy->occurrences), &storage);
    if (status != NL_CHECK_OK)
        goto failure;
    copy->occurrences = storage;
    copy->occurrence_count = source->occurrence_count;
    status = nl_raw_clone(source, copy);
    if (status != NL_CHECK_OK) {
        goto failure;
    }
    *out = copy;
    return NL_CHECK_OK;
failure:
    nl_semantic_destroy(copy);
    return status;
}

void nl_sem_commit(NLSemanticContext *context, NLSemanticContext *candidate)
{
    const NLSemanticContext old = *context;
    *context = *candidate;
    *candidate = old;
    nl_semantic_destroy(candidate);
}

static NLCheckStatus nominal(NLSemanticContext *context, const char *name,
                             bool is_copy, bool discardable, NLTypeId *out)
{
    if (is_copy && !discardable) {
        return NL_CHECK_SEMANTIC_ERROR;
    }
    for (size_t i = 0; i < context->type_count; ++i) {
        if (context->types[i].name != NULL &&
            strcmp(context->types[i].name, name) == 0) {
            return NL_CHECK_SEMANTIC_ERROR;
        }
    }
    char *owned_name = NULL;
    NLCheckStatus status = copy_name(name, &owned_name);
    if (status != NL_CHECK_OK) {
        return status;
    }
    void *storage = NULL;
    status = append_storage(context->types, context->type_count,
                            sizeof(*context->types), &storage);
    if (status != NL_CHECK_OK) {
        free(owned_name);
        return status;
    }
    context->types = storage;
    context->types[context->type_count++] =
        (NLTypeEntry){.name = owned_name,
                      .view = {.kind = NL_TYPE_NOMINAL,
                               .is_copy = is_copy,
                               .is_discardable = discardable}};
    *out = context->type_count;
    return NL_CHECK_OK;
}

NLCheckStatus nl_semantic_create(NLSemanticContext **out_context)
{
    if (out_context == NULL || *out_context != NULL) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    NLSemanticContext *const context = malloc(sizeof(*context));
    if (context == NULL) {
        return NL_CHECK_OUT_OF_MEMORY;
    }
    *context = (NLSemanticContext){0};
    NLTypeId type_id;
    NLCheckStatus status = nominal(context, "unit", true, true, &type_id);
    if (status != NL_CHECK_OK) {
        goto failure;
    }
    context->types[0].view.kind = NL_TYPE_UNIT;
    status = nominal(context, "LifetimeDomain", false, false, &type_id);
    if (status != NL_CHECK_OK) {
        goto failure;
    }
    static const struct {
        const char *name;
        NLSemanticTypeKind kind;
        bool copy;
    } core[] = {{"Allocation", NL_TYPE_ALLOCATION, false},
                {"Storage", NL_TYPE_STORAGE, false},
                {"byte", NL_TYPE_BYTE, true},
                {"u8", NL_TYPE_U8, true},
                {"usize", NL_TYPE_USIZE, true},
                {"addr", NL_TYPE_ADDR, true}};
    for (size_t i = 0; i < sizeof(core) / sizeof(core[0]); ++i) {
        status = nominal(context, core[i].name, core[i].copy, core[i].copy,
                         &type_id);
        if (status != NL_CHECK_OK) {
            goto failure;
        }
        NLSemanticTypeView *const type = &context->types[type_id - 1].view;
        type->kind = core[i].kind;
        if (core[i].kind == NL_TYPE_BYTE || core[i].kind == NL_TYPE_U8) {
            type->layout_known = true;
            type->size = 1;
            type->alignment = 1;
        }
    }
    static const NLFunctionEntry prelude[] = {
        {.name = "LifetimeDomain",
         .kind = NL_CHECKED_DOMAIN_CREATE,
         .count = 0},
        {.name = "ptr_from_ref", .kind = NL_CHECKED_PTR_FROM_REF, .count = 1},
        {.name = "finalize_domain",
         .kind = NL_CHECKED_DOMAIN_FINALIZE,
         .count = 1},
        {.name = "initialize", .kind = NL_CHECKED_INITIALIZE, .count = 3},
        {.name = "take", .kind = NL_CHECKED_TAKE, .count = 2},
        {.name = "destroy", .kind = NL_CHECKED_DESTROY, .count = 2},
        {.name = "replace", .kind = NL_CHECKED_REPLACE, .count = 2},
        {.name = "store", .kind = NL_CHECKED_STORE, .count = 2},
        {.name = "swap", .kind = NL_CHECKED_SWAP, .count = 2}};
    context->functions = malloc(sizeof(prelude));
    if (context->functions == NULL) {
        status = NL_CHECK_OUT_OF_MEMORY;
        goto failure;
    }
    memcpy(context->functions, prelude, sizeof(prelude));
    context->function_count = sizeof(prelude) / sizeof(prelude[0]);
    *out_context = context;
    return NL_CHECK_OK;
failure:
    nl_semantic_destroy(context);
    return status;
}

NLTypeId nl_semantic_unit_type(const NLSemanticContext *context)
{
    return context == NULL ? 0 : 1;
}
NLTypeId nl_semantic_domain_type(const NLSemanticContext *context)
{
    return context == NULL ? 0 : 2;
}

bool nl_semantic_type_view(const NLSemanticContext *c, NLTypeId id,
                           NLSemanticTypeView *out)
{
    if (c == NULL || out == NULL || id == 0 || id > c->type_count) {
        return false;
    }
    *out = c->types[id - 1].view;
    return true;
}
bool nl_semantic_binding_view(const NLSemanticContext *c, NLSymbolId id,
                              NLSemanticBindingView *out)
{
    if (c == NULL || out == NULL || id == 0 || id > c->binding_count) {
        return false;
    }
    *out = c->bindings[id - 1].view;
    return true;
}
bool nl_semantic_value_view(const NLSemanticContext *c, NLValueId id,
                            NLSemanticValueView *out)
{
    if (c == NULL || out == NULL || id == 0 || id > c->value_count) {
        return false;
    }
    *out = c->values[id - 1];
    return true;
}
bool nl_semantic_place_view(const NLSemanticContext *c, NLPlaceId id,
                            NLSemanticPlaceView *out)
{
    if (c == NULL || out == NULL || id == 0 || id > c->place_count) {
        return false;
    }
    *out = c->places[id - 1];
    return true;
}
bool nl_semantic_domain_view(const NLSemanticContext *c, NLDomainId id,
                             NLSemanticDomainView *out)
{
    if (c == NULL || out == NULL || id == 0 || id > c->domain_count) {
        return false;
    }
    *out = c->domains[id - 1];
    return true;
}
bool nl_semantic_scope_view(const NLSemanticContext *c, NLScopeId id,
                            NLSemanticScopeView *out)
{
    if (c == NULL || out == NULL || id == 0 || id > c->scope_count) {
        return false;
    }
    *out = c->scopes[id - 1];
    return true;
}
bool nl_semantic_snapshot(const NLSemanticContext *c, NLSemanticSnapshot *out)
{
    if (c == NULL || out == NULL) {
        return false;
    }
    *out = (NLSemanticSnapshot){.types = c->type_count,
                                .bindings = c->binding_count,
                                .values = c->value_count,
                                .places = c->place_count,
                                .domains = c->domain_count,
                                .scopes = c->scope_count,
                                .functions = c->function_count,
                                .last_incarnation = c->last_incarnation,
                                .last_value_fact = c->last_value_fact,
                                .backing_regions = c->region_count,
                                .raw_intervals = c->raw_interval_count,
                                .occurrences = c->occurrence_count};
    return true;
}
NLSymbolId nl_semantic_find_binding(const NLSemanticContext *c,
                                    const char *name)
{
    if (c != NULL && name != NULL) {
        for (size_t i = c->binding_count; i > 0; --i) {
            if (!c->bindings[i - 1].hidden &&
                strcmp(c->bindings[i - 1].name, name) == 0) {
                return i;
            }
        }
    }
    return 0;
}

NLCheckStatus nl_sem_compound(NLSemanticContext *c, NLSemanticTypeKind kind,
                              NLTypeId target, NLAccessSyntax access,
                              bool exclusive, NLTypeId *out)
{
    if (target == 0 || target > c->type_count ||
        (kind != NL_TYPE_PTR && kind != NL_TYPE_REF && kind != NL_TYPE_SLOT) ||
        (access != NL_ACCESS_READ && access != NL_ACCESS_WRITE) ||
        (kind != NL_TYPE_REF && (exclusive || access != NL_ACCESS_READ))) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    for (size_t i = 0; i < c->type_count; ++i) {
        const NLSemanticTypeView view = c->types[i].view;
        if (view.kind == kind && view.target == target &&
            view.access == access && view.is_exclusive == exclusive) {
            *out = i + 1;
            return NL_CHECK_OK;
        }
    }
    void *storage = NULL;
    NLCheckStatus status =
        append_storage(c->types, c->type_count, sizeof(*c->types), &storage);
    if (status != NL_CHECK_OK) {
        return status;
    }
    c->types = storage;
    c->types[c->type_count++] =
        (NLTypeEntry){.view = {.kind = kind,
                               .is_copy = kind == NL_TYPE_PTR ||
                                          (kind == NL_TYPE_REF && !exclusive),
                               .is_discardable = kind != NL_TYPE_SLOT,
                               .target = target,
                               .access = access,
                               .is_exclusive = exclusive}};
    *out = c->type_count;
    return NL_CHECK_OK;
}

NLCheckStatus nl_sem_new_value(NLSemanticContext *c, NLSemanticValueView value,
                               NLValueId *out)
{
    void *storage = NULL;
    NLCheckStatus status =
        append_storage(c->values, c->value_count, sizeof(*c->values), &storage);
    if (status != NL_CHECK_OK) {
        return status;
    }
    c->values = storage;
    value.carrier = NL_CARRIER_LOOSE;
    value.owner_place = 0;
    value.aggregate_owner = 0;
    value.sum_owner = 0;
    c->values[c->value_count++] = value;
    *out = c->value_count;
    return NL_CHECK_OK;
}

NLCheckStatus nl_sem_fresh_fact(NLSemanticContext *c, NLValueFactId *out)
{
    if (c->last_value_fact == SIZE_MAX) {
        return NL_CHECK_RESOURCE_LIMIT;
    }
    *out = ++c->last_value_fact;
    return NL_CHECK_OK;
}

NLCheckStatus nl_sem_install(NLSemanticContext *c, NLPlaceId place,
                             NLValueId value, NLDomainId domain)
{
    if (c->last_incarnation == SIZE_MAX) {
        return NL_CHECK_RESOURCE_LIMIT;
    }
    NLValueFactId fact;
    NLCheckStatus status = nl_sem_fresh_fact(c, &fact);
    if (status != NL_CHECK_OK) {
        return status;
    }
    NLSemanticPlaceView *const p = &c->places[place - 1];
    p->live = true;
    p->incarnation = ++c->last_incarnation;
    p->current_fact = fact;
    p->current_value = value;
    p->governing_domain = domain;
    c->values[value - 1].carrier = NL_CARRIER_PLACE;
    c->values[value - 1].owner_place = place;
    return nl_sum_attach(c, place);
}

NLCheckStatus nl_sem_new_place(NLSemanticContext *c, NLTypeId type,
                               NLDomainId domain, bool root, NLValueId value,
                               NLPlaceId *out)
{
    void *storage = NULL;
    NLCheckStatus status =
        append_storage(c->places, c->place_count, sizeof(*c->places), &storage);
    if (status != NL_CHECK_OK) {
        return status;
    }
    c->places = storage;
    c->places[c->place_count++] =
        (NLSemanticPlaceView){.type = type, .independent_root = root};
    *out = c->place_count;
    return value == 0 ? NL_CHECK_OK : nl_sem_install(c, *out, value, domain);
}

bool nl_sem_lexical_name_admissible(const void *bytes, size_t length)
{
    return length != 4 || memcmp(bytes, "unit", 4) != 0;
}

NLCheckStatus nl_sem_bind_in_scope(NLSemanticContext *c, const char *name,
                                   NLValueId value, size_t floor,
                                   NLSymbolId *out)
{
    if (!nl_sem_lexical_name_admissible(name, strlen(name)))
        return NL_CHECK_SEMANTIC_ERROR;
    for (size_t i = floor; i < c->binding_count; ++i) {
        if (!c->bindings[i].hidden && strcmp(c->bindings[i].name, name) == 0)
            return NL_CHECK_SEMANTIC_ERROR;
    }
    if (value == 0 || value > c->value_count ||
        c->values[value - 1].carrier != NL_CARRIER_LOOSE) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    char *owned_name = NULL;
    NLCheckStatus status = copy_name(name, &owned_name);
    if (status != NL_CHECK_OK) {
        return status;
    }
    NLPlaceId place;
    status =
        nl_sem_new_place(c, c->values[value - 1].type, 0, true, value, &place);
    if (status != NL_CHECK_OK) {
        free(owned_name);
        return status;
    }
    void *storage = NULL;
    status = append_storage(c->bindings, c->binding_count, sizeof(*c->bindings),
                            &storage);
    if (status != NL_CHECK_OK) {
        free(owned_name);
        return status;
    }
    c->bindings = storage;
    c->bindings[c->binding_count++] = (NLBindingEntry){
        .name = owned_name,
        .view = {NL_AVAILABLE, c->values[value - 1].type, value, place}};
    *out = c->binding_count;
    return NL_CHECK_OK;
}

NLCheckStatus nl_sem_bind(NLSemanticContext *c, const char *name,
                          NLValueId value, NLSymbolId *out)
{
    return nl_sem_bind_in_scope(c, name, value, 0, out);
}

/* P5 members are flat; copies allocate fresh packages for every member. */
NLCheckStatus nl_sem_copy_value(NLSemanticContext *c, NLValueId source,
                                NLValueId *out)
{
    const NLSemanticValueView original = c->values[source - 1];
    NLValueId id;
    NLCheckStatus status = nl_sem_new_value(c, original, &id);
    if (status != NL_CHECK_OK)
        return status;
    for (size_t f = 0; f < original.field_count; ++f) {
        NLValueId member;
        status =
            nl_sem_new_value(c, c->values[original.fields[f] - 1], &member);
        if (status != NL_CHECK_OK)
            return status;
        c->values[member - 1].carrier = NL_CARRIER_AGGREGATE;
        c->values[member - 1].aggregate_owner = id;
        c->values[id - 1].fields[f] = member;
    }
    if (original.sum_payload != 0) {
        NLValueId payload;
        status = nl_sem_copy_value(c, original.sum_payload, &payload);
        if (status != NL_CHECK_OK)
            return status;
        c->values[payload - 1].carrier = NL_CARRIER_SUM;
        c->values[payload - 1].sum_owner = id;
        c->values[id - 1].sum_payload = payload;
    }
    *out = id;
    return NL_CHECK_OK;
}

void nl_sem_end_value(NLSemanticContext *c, NLValueId value)
{
    const NLSemanticValueView original = c->values[value - 1];
    if (original.sum_payload != 0)
        nl_sem_end_value(c, original.sum_payload);
    for (size_t f = 0; f < original.field_count; ++f) {
        c->values[original.fields[f] - 1].carrier = NL_CARRIER_ENDED;
        c->values[original.fields[f] - 1].aggregate_owner = 0;
    }
    c->values[value - 1].carrier = NL_CARRIER_ENDED;
    c->values[value - 1].owner_place = 0;
    c->values[value - 1].aggregate_owner = 0;
    c->values[value - 1].sum_owner = 0;
}

NLCheckStatus nl_sem_new_domain(NLSemanticContext *c, NLDomainId *out_domain,
                                NLValueId *out_value)
{
    void *storage = NULL;
    NLCheckStatus status = append_storage(c->domains, c->domain_count,
                                          sizeof(*c->domains), &storage);
    if (status != NL_CHECK_OK) {
        return status;
    }
    c->domains = storage;
    c->domains[c->domain_count++] = (NLSemanticDomainView){.live = true};
    const NLDomainId domain = c->domain_count;
    NLValueId value;
    status = nl_sem_new_value(
        c, (NLSemanticValueView){.type = 2, .domain = domain}, &value);
    if (status != NL_CHECK_OK) {
        return status;
    }
    c->domains[domain - 1].value = value;
    *out_domain = domain;
    *out_value = value;
    return NL_CHECK_OK;
}

NLCheckStatus nl_sem_new_scope(NLSemanticContext *c, NLScopeId parent,
                               bool active, NLScopeId *out)
{
    if (parent > c->scope_count) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    void *storage = NULL;
    NLCheckStatus status =
        append_storage(c->scopes, c->scope_count, sizeof(*c->scopes), &storage);
    if (status != NL_CHECK_OK) {
        return status;
    }
    c->scopes = storage;
    c->scopes[c->scope_count++] =
        (NLSemanticScopeView){.active = active, .parent = parent};
    *out = c->scope_count;
    return NL_CHECK_OK;
}

static NLCheckStatus finish_candidate(NLSemanticContext *c,
                                      NLSemanticContext *candidate,
                                      NLCheckStatus status)
{
    if (status == NL_CHECK_OK)
        status = nl_sum_validate(candidate);
    if (status == NL_CHECK_OK) {
        nl_sem_commit(c, candidate);
    } else {
        nl_semantic_destroy(candidate);
    }
    return status;
}

NLCheckStatus nl_semantic_nominal(NLSemanticContext *c, const char *name,
                                  bool copy, bool discardable, NLTypeId *out)
{
    if (c == NULL || name == NULL || name[0] == 0 || out == NULL) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLTypeId id = 0;
    if (status == NL_CHECK_OK) {
        status = nominal(candidate, name, copy, discardable, &id);
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK) {
        *out = id;
    }
    return status;
}

NLCheckStatus nl_semantic_compound_type(NLSemanticContext *c,
                                        NLSemanticTypeKind kind,
                                        NLTypeId target, NLAccessSyntax access,
                                        bool exclusive, NLTypeId *out)
{
    if (c == NULL || out == NULL) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLTypeId id = 0;
    if (status == NL_CHECK_OK) {
        status =
            nl_sem_compound(candidate, kind, target, access, exclusive, &id);
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK) {
        *out = id;
    }
    return status;
}

NLCheckStatus nl_semantic_seed_value(NLSemanticContext *c, const char *name,
                                     NLTypeId type,
                                     NLDependencyKnowledge dependencies,
                                     NLSymbolId *out)
{
    if (c == NULL || name == NULL || name[0] == 0 || out == NULL || type == 0 ||
        type > c->type_count || dependencies > NL_DEPENDENCIES_UNKNOWN ||
        (dependencies != NL_DEPENDENCY_FREE &&
         dependencies != NL_HIDDEN_DEPENDENCIES &&
         dependencies != NL_DEPENDENCIES_UNKNOWN)) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    const NLSemanticTypeKind kind = c->types[type - 1].view.kind;
    if ((kind != NL_TYPE_NOMINAL && kind != NL_TYPE_BYTE &&
         kind != NL_TYPE_U8 && kind != NL_TYPE_USIZE && kind != NL_TYPE_ADDR &&
         type != 1) ||
        type == 2 || c->types[type - 1].view.field_count != 0) {
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLValueId value = 0;
    NLSymbolId symbol = 0;
    if (status == NL_CHECK_OK) {
        status = nl_sem_new_value(
            candidate,
            (NLSemanticValueView){.type = type, .dependencies = dependencies},
            &value);
    }
    if (status == NL_CHECK_OK) {
        status = nl_sem_bind(candidate, name, value, &symbol);
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK) {
        *out = symbol;
    }
    return status;
}

NLCheckStatus nl_semantic_seed_domain(NLSemanticContext *c, const char *name,
                                      NLSymbolId *out, NLDomainId *out_domain)
{
    if (c == NULL || name == NULL || name[0] == 0 || out == NULL ||
        out_domain == NULL) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLDomainId domain = 0;
    NLValueId value = 0;
    NLSymbolId symbol = 0;
    if (status == NL_CHECK_OK) {
        status = nl_sem_new_domain(candidate, &domain, &value);
    }
    if (status == NL_CHECK_OK) {
        status = nl_sem_bind(candidate, name, value, &symbol);
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK) {
        *out = symbol;
        *out_domain = domain;
    }
    return status;
}

NLCheckStatus nl_semantic_seed_root(NLSemanticContext *c, NLTypeId type,
                                    NLDomainId domain, bool root,
                                    NLDependencyKnowledge dependencies,
                                    NLPlaceId *out, NLValueId *out_value)
{
    if (c == NULL || out == NULL || out_value == NULL || type == 0 ||
        type > c->type_count || domain == 0 || domain > c->domain_count ||
        !c->domains[domain - 1].live ||
        (dependencies != NL_DEPENDENCY_FREE &&
         dependencies != NL_HIDDEN_DEPENDENCIES &&
         dependencies != NL_DEPENDENCIES_UNKNOWN)) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    if (c->types[type - 1].view.kind != NL_TYPE_NOMINAL || type == 2 ||
        c->types[type - 1].view.field_count != 0) {
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLValueId value = 0;
    NLPlaceId place = 0;
    if (status == NL_CHECK_OK) {
        status = nl_sem_new_value(
            candidate,
            (NLSemanticValueView){.type = type, .dependencies = dependencies},
            &value);
    }
    if (status == NL_CHECK_OK) {
        status = nl_sem_new_place(candidate, type, domain, root, value, &place);
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK) {
        *out = place;
        *out_value = value;
    }
    return status;
}

NLCheckStatus nl_semantic_seed_slot(NLSemanticContext *c, const char *name,
                                    NLTypeId target, NLSymbolId *out,
                                    NLPlaceId *out_place)
{
    if (c == NULL || name == NULL || name[0] == 0 || out == NULL ||
        out_place == NULL || target == 0 || target > c->type_count) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLTypeId type = 0;
    NLPlaceId place = 0;
    NLValueId value = 0;
    NLSymbolId symbol = 0;
    if (status == NL_CHECK_OK) {
        status = nl_sem_compound(candidate, NL_TYPE_SLOT, target,
                                 NL_ACCESS_READ, false, &type);
    }
    if (status == NL_CHECK_OK) {
        status = nl_sem_new_place(candidate, target, 0, true, 0, &place);
    }
    if (status == NL_CHECK_OK) {
        status = nl_sem_new_value(
            candidate, (NLSemanticValueView){.type = type, .slot_place = place},
            &value);
    }
    if (status == NL_CHECK_OK) {
        status = nl_sem_bind(candidate, name, value, &symbol);
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK) {
        *out = symbol;
        *out_place = place;
    }
    return status;
}

NLCheckStatus nl_semantic_seed_reference(NLSemanticContext *c, const char *name,
                                         NLTypeId type, NLReferenceFacts facts,
                                         NLSymbolId *out)
{
    if (c == NULL || name == NULL || name[0] == 0 || out == NULL || type == 0 ||
        type > c->type_count || facts.place > c->place_count ||
        facts.scope > c->scope_count ||
        facts.occurrence_dependency > c->occurrence_count ||
        (facts.provenance != NL_PROVENANCE_UNKNOWN &&
         facts.provenance != NL_PROVENANCE_VALID &&
         facts.provenance != NL_PROVENANCE_INVALID)) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    const NLSemanticTypeKind kind = c->types[type - 1].view.kind;
    if ((kind != NL_TYPE_PTR && kind != NL_TYPE_REF) ||
        (kind == NL_TYPE_PTR &&
         (facts.scope != 0 || facts.occurrence_dependency != 0))) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLValueId value = 0;
    NLSymbolId symbol = 0;
    if (status == NL_CHECK_OK) {
        status = nl_sem_new_value(
            candidate, (NLSemanticValueView){.type = type, .reference = facts},
            &value);
    }
    if (status == NL_CHECK_OK) {
        status = nl_sem_bind(candidate, name, value, &symbol);
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK) {
        *out = symbol;
    }
    return status;
}

NLCheckStatus nl_semantic_scope(NLSemanticContext *c, NLScopeId parent,
                                bool active, NLScopeId *out)
{
    if (c == NULL || out == NULL) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLScopeId scope = 0;
    if (status == NL_CHECK_OK) {
        status = nl_sem_new_scope(candidate, parent, active, &scope);
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK) {
        *out = scope;
    }
    return status;
}

NLCheckStatus nl_semantic_end_scope(NLSemanticContext *c, NLScopeId scope)
{
    if (c == NULL || scope == 0 || scope > c->scope_count) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    for (size_t i = 0; i < c->scope_count; ++i) {
        if (c->scopes[i].active && c->scopes[i].parent == scope) {
            return NL_CHECK_SEMANTIC_ERROR;
        }
    }
    c->scopes[scope - 1].active = false;
    return NL_CHECK_OK;
}

NLCheckStatus nl_semantic_bind_result(NLSemanticContext *c, const char *name,
                                      NLValueId value, NLSymbolId *out)
{
    if (c == NULL || name == NULL || name[0] == 0 || out == NULL) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLSymbolId symbol = 0;
    if (status == NL_CHECK_OK) {
        status = nl_sem_bind(candidate, name, value, &symbol);
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK) {
        *out = symbol;
    }
    return status;
}

NLCheckStatus nl_semantic_register_function(NLSemanticContext *c,
                                            const char *name,
                                            const NLTypeId *parameters,
                                            size_t count, NLTypeId result,
                                            bool effects, bool dependencies)
{
    if (c == NULL || name == NULL || name[0] == 0 ||
        (count != 0 && parameters == NULL) || result == 0 ||
        result > c->type_count) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    if (!nl_sem_lexical_name_admissible(name, strlen(name)))
        return NL_CHECK_SEMANTIC_ERROR;
    if (count > NL_SEMANTIC_MAX_PARAMETERS) {
        return NL_CHECK_RESOURCE_LIMIT;
    }
    if (c->types[result - 1].view.field_count != 0) {
        return NL_CHECK_SEMANTIC_UNSUPPORTED; /* no opaque aggregate result mint
                                               */
    }
    for (size_t i = 0; i < count; ++i) {
        if (parameters[i] == 0 || parameters[i] > c->type_count) {
            return NL_CHECK_INTERNAL_ERROR;
        }
    }
    for (size_t i = 0; i < c->function_count; ++i) {
        if (strcmp(c->functions[i].name, name) == 0) {
            return NL_CHECK_SEMANTIC_ERROR;
        }
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    char *owned_name = NULL;
    void *params = NULL;
    void *storage = NULL;
    if (status == NL_CHECK_OK) {
        status = copy_name(name, &owned_name);
    }
    if (status == NL_CHECK_OK) {
        status = copy_array(parameters, count, sizeof(*parameters), &params);
    }
    if (status == NL_CHECK_OK) {
        status = append_storage(candidate->functions, candidate->function_count,
                                sizeof(*candidate->functions), &storage);
    }
    if (status == NL_CHECK_OK) {
        candidate->functions = storage;
        candidate->functions[candidate->function_count++] =
            (NLFunctionEntry){.name = owned_name,
                              .kind = NL_CHECKED_REGISTERED_CALL,
                              .parameters = params,
                              .count = count,
                              .result = result,
                              .caller_effects = effects,
                              .hidden_dependencies = dependencies};
    } else {
        free(owned_name);
        free(params);
    }
    return finish_candidate(c, candidate, status);
}

NLCheckStatus nl_semantic_register_aggregate(NLSemanticContext *c,
                                             const char *name,
                                             const NLAggregateField *fields,
                                             size_t count, NLTypeId *out)
{
    if (c == NULL || name == NULL || name[0] == 0 || fields == NULL ||
        out == NULL)
        return NL_CHECK_INTERNAL_ERROR;
    if (count == 0)
        return NL_CHECK_SEMANTIC_ERROR;
    if (count > NL_SEMANTIC_MAX_FIELDS)
        return NL_CHECK_RESOURCE_LIMIT;
    bool copy = true, discard = true;
    for (size_t f = 0; f < count; ++f) {
        if (fields[f].name == NULL || fields[f].name[0] == 0 ||
            fields[f].type == 0 || fields[f].type > c->type_count)
            return NL_CHECK_INTERNAL_ERROR;
        const NLSemanticTypeView t = c->types[fields[f].type - 1].view;
        if (t.field_count != 0 || fields[f].type == 2 ||
            (t.kind != NL_TYPE_NOMINAL && t.kind != NL_TYPE_BYTE &&
             t.kind != NL_TYPE_U8 && t.kind != NL_TYPE_USIZE &&
             t.kind != NL_TYPE_ADDR))
            return NL_CHECK_SEMANTIC_UNSUPPORTED;
        for (size_t j = 0; j < f; ++j)
            if (strcmp(fields[f].name, fields[j].name) == 0)
                return NL_CHECK_SEMANTIC_ERROR;
        copy = copy && t.is_copy;
        discard = discard && t.is_discardable;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLTypeId id = 0;
    if (status == NL_CHECK_OK)
        status = nominal(candidate, name, copy, discard, &id);
    if (status == NL_CHECK_OK) {
        NLTypeEntry *entry = &candidate->types[id - 1];
        entry->view.field_count = count;
        for (size_t f = 0; f < count; ++f) {
            entry->field_types[f] = fields[f].type;
            status = copy_name(fields[f].name, &entry->field_names[f]);
            if (status != NL_CHECK_OK)
                break;
        }
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK)
        *out = id;
    return status;
}

NLTypeId nl_semantic_core_type(const NLSemanticContext *c,
                               NLSemanticTypeKind kind)
{
    if (c != NULL && kind != NL_TYPE_NOMINAL && kind != NL_TYPE_PTR &&
        kind != NL_TYPE_REF && kind != NL_TYPE_SLOT && kind != NL_TYPE_SUM) {
        for (size_t i = 0; i < c->type_count; ++i) {
            if (c->types[i].view.kind == kind) {
                return i + 1;
            }
        }
    }
    return 0;
}

NLCheckStatus nl_semantic_set_layout(NLSemanticContext *c, NLTypeId type,
                                     size_t size, size_t alignment)
{
    if (c == NULL || type == 0 || type > c->type_count) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    if (size == 0 || alignment == 0) {
        return NL_CHECK_SEMANTIC_ERROR;
    }
    const NLSemanticTypeView old = c->types[type - 1].view;
    if (old.layout_known) {
        return old.size == size && old.alignment == alignment
                   ? NL_CHECK_OK
                   : NL_CHECK_SEMANTIC_ERROR;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    if (status == NL_CHECK_OK) {
        candidate->types[type - 1].view.layout_known = true;
        candidate->types[type - 1].view.size = size;
        candidate->types[type - 1].view.alignment = alignment;
    }
    return finish_candidate(c, candidate, status);
}

NLCheckStatus nl_semantic_seed_scalar(NLSemanticContext *c, const char *name,
                                      NLScalarValue scalar, NLSymbolId *out)
{
    if (c == NULL || name == NULL || name[0] == 0 || out == NULL ||
        scalar.type == 0 || scalar.type > c->type_count) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    const NLSemanticTypeKind kind = c->types[scalar.type - 1].view.kind;
    if (kind != NL_TYPE_BYTE && kind != NL_TYPE_U8 && kind != NL_TYPE_USIZE &&
        kind != NL_TYPE_ADDR) {
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    }
    if (scalar.known && (kind == NL_TYPE_BYTE || kind == NL_TYPE_U8) &&
        scalar.value > 255) {
        return NL_CHECK_SEMANTIC_ERROR;
    }
    NLSemanticContext *candidate = NULL;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    NLValueId value = 0;
    NLSymbolId symbol = 0;
    if (status == NL_CHECK_OK) {
        status = nl_sem_new_value(
            candidate,
            (NLSemanticValueView){.type = scalar.type,
                                  .scalar_known = scalar.known,
                                  .scalar_value =
                                      scalar.known ? scalar.value : 0},
            &value);
    }
    if (status == NL_CHECK_OK) {
        status = nl_sem_bind(candidate, name, value, &symbol);
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK) {
        *out = symbol;
    }
    return status;
}

NLCheckStatus nl_semantic_register_sum(NLSemanticContext *c, const char *name,
                                       const NLSumVariant *variants,
                                       size_t count, NLTypeId *out)
{
    if (c == NULL || name == NULL || name[0] == 0 || variants == NULL ||
        out == NULL)
        return NL_CHECK_INTERNAL_ERROR;
    if (count == 0)
        return NL_CHECK_SEMANTIC_ERROR;
    if (count > NL_SEMANTIC_MAX_VARIANTS)
        return NL_CHECK_RESOURCE_LIMIT;
    bool copy = true, discard = true;
    for (size_t i = 0; i < count; ++i) {
        if (variants[i].name == NULL || variants[i].name[0] == 0 ||
            variants[i].payload > c->type_count)
            return NL_CHECK_INTERNAL_ERROR;
        for (size_t j = 0; j < i; ++j)
            if (strcmp(variants[i].name, variants[j].name) == 0)
                return NL_CHECK_SEMANTIC_ERROR;
        if (variants[i].payload != 0) {
            const NLSemanticTypeView t = c->types[variants[i].payload - 1].view;
            if (t.kind == NL_TYPE_SUM || t.field_count != 0 ||
                t.kind == NL_TYPE_REF || t.kind == NL_TYPE_PTR)
                return NL_CHECK_SEMANTIC_UNSUPPORTED;
            copy = copy && t.is_copy;
            discard = discard && t.is_discardable;
        }
    }
    NLSemanticContext *candidate = NULL;
    NLTypeId id = 0;
    NLCheckStatus status = nl_sem_clone(c, &candidate);
    if (status == NL_CHECK_OK)
        status = nominal(candidate, name, copy, discard, &id);
    if (status == NL_CHECK_OK) {
        NLTypeEntry *e = &candidate->types[id - 1];
        e->view.kind = NL_TYPE_SUM;
        e->view.variant_count = count;
        for (size_t i = 0; i < count; ++i) {
            e->variant_types[i] = variants[i].payload;
            status = copy_name(variants[i].name, &e->variant_names[i]);
            if (status != NL_CHECK_OK)
                break;
        }
    }
    status = finish_candidate(c, candidate, status);
    if (status == NL_CHECK_OK)
        *out = id;
    return status;
}

bool nl_semantic_occurrence_view(const NLSemanticContext *c, NLOccurrenceId id,
                                 NLSemanticOccurrenceView *out)
{
    if (c == NULL || out == NULL || id == 0 || id > c->occurrence_count)
        return false;
    *out = c->occurrences[id - 1];
    return true;
}
