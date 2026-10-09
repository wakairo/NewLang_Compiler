#include "semantic_internal.h"
#include <stdlib.h>
#include <string.h>

/* Declaration state only. Never allocates a value, place, provenance or root.
 */
NLCheckStatus nl_recursive_header(NLSemanticContext *c, const char *name,
                                  NLTypeId *out)
{
    if (name == NULL || !nl_sem_lexical_name_admissible(name, strlen(name)))
        return NL_CHECK_SEMANTIC_ERROR;
    NLCheckStatus s = nl_sem_nominal(c, name, false, false, out);
    if (s == NL_CHECK_OK) {
        c->types[*out - 1].incomplete = true;
        c->types[*out - 1].recursive_header = true;
    }
    return s;
}

static char *owned(const char *s)
{
    char *copy = malloc(strlen(s) + 1);
    if (copy != NULL)
        memcpy(copy, s, strlen(s) + 1);
    return copy;
}

NLCheckStatus nl_recursive_option(NLSemanticContext *c, NLTypeId ptr,
                                  NLTypeId *out)
{
    if (ptr == 0 || ptr > c->type_count)
        return NL_CHECK_INTERNAL_ERROR;
    const NLSemanticTypeView p = c->types[ptr - 1].view;
    if (p.kind != NL_TYPE_PTR || p.target == 0 || p.target > c->type_count ||
        !c->types[p.target - 1].recursive_header)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    for (size_t i = 0; i < c->type_count; ++i)
        if (c->types[i].option_target == ptr) {
            *out = i + 1;
            return NL_CHECK_OK;
        }
    if (c->type_count >= NL_SEMANTIC_MAX_ENTRIES)
        return NL_CHECK_RESOURCE_LIMIT;
    NLTypeEntry option = {.option_target = ptr,
                          .view = {.kind = NL_TYPE_SUM,
                                   .is_copy = p.is_copy,
                                   .is_discardable = p.is_discardable,
                                   .variant_count = 2}};
    option.variant_names[0] = owned("None");
    option.variant_names[1] = owned("Some");
    option.variant_types[1] = ptr;
    NLTypeEntry *table = NULL;
    if (option.variant_names[0] != NULL && option.variant_names[1] != NULL)
        table = realloc(c->types, (c->type_count + 1) * sizeof(*table));
    if (table == NULL) {
        free(option.variant_names[0]);
        free(option.variant_names[1]);
        return NL_CHECK_OUT_OF_MEMORY;
    }
    c->types = table;
    c->types[c->type_count++] = option;
    *out = c->type_count;
    return NL_CHECK_OK;
}

/* Temporary colors bound traversal independently of target/C representation.
 * Only the candidate header's fields are supplied before completion. */
static NLCheckStatus visit(const NLSemanticContext *c, NLTypeId id,
                           NLTypeId header, const NLAggregateField *fields,
                           size_t count, unsigned char *colors)
{
    if (id == 0 || id > c->type_count)
        return NL_CHECK_INTERNAL_ERROR;
    const NLTypeEntry *t = &c->types[id - 1];
    if (t->view.kind == NL_TYPE_PTR)
        return NL_CHECK_OK;
    if (colors[id - 1] == 1)
        return NL_CHECK_SEMANTIC_ERROR;
    if (colors[id - 1] == 2)
        return NL_CHECK_OK;
    if (t->incomplete && id != header)
        return NL_CHECK_SEMANTIC_ERROR;
    colors[id - 1] = 1;
    const size_t n = id == header ? count : t->view.field_count;
    for (size_t i = 0; i < n; ++i) {
        const NLCheckStatus s =
            visit(c, id == header ? fields[i].type : t->field_types[i], header,
                  fields, count, colors);
        if (s != NL_CHECK_OK)
            return s;
    }
    for (size_t i = 0; i < t->view.variant_count; ++i) {
        if (t->variant_types[i] == 0)
            continue;
        const NLCheckStatus s =
            visit(c, t->variant_types[i], header, fields, count, colors);
        if (s != NL_CHECK_OK)
            return s;
    }
    colors[id - 1] = 2;
    return NL_CHECK_OK;
}

NLCheckStatus nl_recursive_complete(NLSemanticContext *c, NLTypeId header,
                                    const NLAggregateField *fields,
                                    size_t count)
{
    if (header == 0 || header > c->type_count || fields == NULL)
        return NL_CHECK_INTERNAL_ERROR;
    if (!c->types[header - 1].recursive_header ||
        !c->types[header - 1].incomplete)
        return NL_CHECK_SEMANTIC_ERROR; /* duplicate/inconsistent never
                                           idempotent */
    if (count != 2 && count != 4)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    for (size_t i = 0; i < count; ++i)
        if (fields[i].name == NULL || fields[i].name[0] == 0 ||
            fields[i].type == 0 || fields[i].type > c->type_count)
            return NL_CHECK_INTERNAL_ERROR;
    for (size_t i = 0; i < count; ++i)
        for (size_t j = 0; j < i; ++j)
            if (strcmp(fields[i].name, fields[j].name) == 0)
                return NL_CHECK_SEMANTIC_ERROR;
    if (c->type_count > NL_SEMANTIC_MAX_ENTRIES)
        return NL_CHECK_RESOURCE_LIMIT;
    unsigned char *colors = malloc(c->type_count);
    if (colors == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    memset(colors, 0, c->type_count);
    NLCheckStatus status = visit(c, header, header, fields, count, colors);
    free(colors);
    if (status != NL_CHECK_OK)
        return status;
    const char *const profile[] = {"next", "prev", "child", "payload"};
    for (size_t i = 0; i < count - 1; ++i) {
        const NLTypeId ptr = c->types[fields[i].type - 1].option_target;
        if (ptr == 0 || c->types[ptr - 1].view.target != header ||
            (count == 4 && strcmp(fields[i].name, profile[i]) != 0))
            return NL_CHECK_SEMANTIC_UNSUPPORTED;
    }
    if (fields[count - 1].type != nl_semantic_core_type(c, NL_TYPE_U8) ||
        (count == 4 && strcmp(fields[3].name, profile[3]) != 0))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    char *names[4] = {0};
    for (size_t i = 0; i < count; ++i) {
        names[i] = owned(fields[i].name);
        if (names[i] == NULL) {
            for (size_t j = 0; j < i; ++j)
                free(names[j]);
            return NL_CHECK_OUT_OF_MEMORY;
        }
    }
    NLTypeEntry *t = &c->types[header - 1];
    t->view.field_count = count;
    t->view.is_copy = true;
    t->view.is_discardable = true;
    for (size_t i = 0; i < count; ++i) {
        t->field_names[i] = names[i];
        t->field_types[i] = fields[i].type;
        t->view.is_copy =
            t->view.is_copy && c->types[fields[i].type - 1].view.is_copy;
        t->view.is_discardable =
            t->view.is_discardable &&
            c->types[fields[i].type - 1].view.is_discardable;
    }
    t->incomplete = false; /* same TypeId, properties now established */
    return NL_CHECK_OK;
}

bool nl_recursive_value_type(const NLSemanticContext *c, NLTypeId type)
{
    if (type == 0 || type > c->type_count || c->types[type - 1].incomplete)
        return false;
    const NLTypeEntry t = c->types[type - 1];
    if (t.option_target != 0)
        return nl_recursive_value_type(c, t.option_target);
    if (t.view.kind == NL_TYPE_PTR || t.view.kind == NL_TYPE_REF ||
        t.view.kind == NL_TYPE_SLOT)
        return nl_recursive_value_type(c, t.view.target);
    return true;
}
NLCheckStatus nl_recursive_validate(const NLSemanticContext *c)
{
    for (size_t i = 0; i < c->type_count; ++i)
        if (c->types[i].incomplete)
            return NL_CHECK_SEMANTIC_ERROR;
    return NL_CHECK_OK;
}
bool nl_semantic_type_completion(const NLSemanticContext *c, NLTypeId type,
                                 bool *out)
{
    if (c == NULL || out == NULL || type == 0 || type > c->type_count ||
        c->types[type - 1].view.kind != NL_TYPE_NOMINAL)
        return false;
    *out = !c->types[type - 1].incomplete;
    return true;
}
