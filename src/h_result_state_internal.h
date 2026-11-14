#ifndef NEWLANG_H_RESULT_STATE_INTERNAL_H
#define NEWLANG_H_RESULT_STATE_INTERNAL_H

/* The existing custody result seal also freezes the CHECK-issued storage
 * consumed by a ptr IF. This is byte authentication, not another semantic
 * checker, world, graph, or authority issuer. Verify all array headers before
 * following their original pointed storage. No mutable string is strlen'd
 * during validation. Shared plan reference counts are deliberately excluded. */
typedef struct {
    unsigned char *bytes;
    size_t size, used;
    bool compare;
} HResultBytes;

static bool result_bytes(HResultBytes *s, const void *data, size_t size)
{
    if (size > SIZE_MAX - s->used)
        return false;
    if (s->bytes) {
        if (s->used > s->size || size > s->size - s->used || (size && !data))
            return false;
        if (size) {
            if (s->compare) {
                if (memcmp(s->bytes + s->used, data, size))
                    return false;
            } else {
                memcpy(s->bytes + s->used, data, size);
            }
        }
    }
    s->used += size;
    return true;
}

static bool result_array(HResultBytes *s, const void *data, size_t count,
                          size_t width)
{
    return (!count || width <= SIZE_MAX / count) &&
           result_bytes(s, data, count * width);
}

static bool result_string(HResultBytes *s, const char *text)
{
    size_t size = 0;
    if (s->compare) {
        if (s->used > s->size || sizeof(size) > s->size - s->used)
            return false;
        memcpy(&size, s->bytes + s->used, sizeof(size));
        s->used += sizeof(size);
        if ((size == 0) != (text == NULL))
            return false;
    } else {
        if (text)
            size = strlen(text) + 1;
        if (!result_bytes(s, &size, sizeof(size)))
            return false;
    }
    return result_bytes(s, text, size);
}

static bool result_state(HResultBytes *s, const NLCheckedFragment *f,
                          const NLSemanticContext *c)
{
#define PART(x)                                                                \
    do {                                                                       \
        if (!result_bytes(s, &(x), sizeof(x)))                                 \
            return false;                                                      \
    } while (0)
    PART(f->if_parent);
    PART(f->if_parent_node);
    PART(f->if_arm_index);
    PART(f->branch_value_prefix);
    PART(f->branch_fact_prefix);
    if (!c || !result_bytes(s, c, sizeof(*c)))
        return false;
#define ARRAY(member, count)                                                   \
    do {                                                                       \
        if (!result_array(s, c->member, c->count, sizeof(*c->member)))          \
            return false;                                                      \
    } while (0)
    /* Their bases/counts are authenticated by the context header above.
     * All pointed-element headers must match before the nested reads below. */
    ARRAY(types, type_count);
    ARRAY(bindings, binding_count);
    ARRAY(values, value_count);
    ARRAY(places, place_count);
    ARRAY(domains, domain_count);
    ARRAY(scopes, scope_count);
    ARRAY(functions, function_count);
    ARRAY(regions, region_count);
    ARRAY(occurrences, occurrence_count);
#undef ARRAY
    for (size_t i = 0; i < c->region_count; ++i)
        if (!result_array(s, c->regions[i].intervals, c->regions[i].count,
                           sizeof(*c->regions[i].intervals)))
            return false;
    for (size_t i = 0; i < c->type_count; ++i) {
        const NLTypeEntry *t = &c->types[i];
        if (!result_string(s, t->name))
            return false;
        for (size_t j = 0; j < NL_SEMANTIC_MAX_FIELDS; ++j)
            if (!result_string(s, t->field_names[j]))
                return false;
        for (size_t j = 0; j < NL_SEMANTIC_MAX_VARIANTS; ++j)
            if (!result_string(s, t->variant_names[j]))
                return false;
    }
    for (size_t i = 0; i < c->binding_count; ++i)
        if (!result_string(s, c->bindings[i].name))
            return false;
    for (size_t i = 0; i < c->function_count; ++i) {
        const NLFunctionEntry *fn = &c->functions[i];
        if (!result_string(s, fn->name) ||
            (fn->parameters &&
             !result_array(s, fn->parameters, fn->count,
                            sizeof(*fn->parameters))))
            return false;
        if (fn->body) {
            const NLFunctionBody *body = fn->body;
            const size_t start = offsetof(NLFunctionBody, source);
            if (!result_bytes(s, (const unsigned char *)body + start,
                                sizeof(*body) - start))
                return false;
            for (size_t j = 0; j < NL_SEMANTIC_MAX_PARAMETERS; ++j)
                if (!result_string(s, body->parameter_names[j]))
                    return false;
        }
    }
#undef PART
    return true;
}
#endif
