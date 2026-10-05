#include "semantic_internal.h"

#include <stdlib.h>

NLCheckStatus nl_checked_add(NLCheckedFragment *fragment,
                             NLCheckedNodeView view, NLCheckedNodeId *out)
{
    if (fragment->count >= NL_SEMANTIC_MAX_ENTRIES) {
        return NL_CHECK_RESOURCE_LIMIT;
    }
    if (fragment->count == fragment->capacity) {
        const size_t capacity =
            fragment->capacity == 0 ? 16 : fragment->capacity * 2;
        NLCheckedNodeView *const nodes =
            realloc(fragment->nodes, capacity * sizeof(*nodes));
        if (nodes == NULL) {
            return NL_CHECK_OUT_OF_MEMORY;
        }
        fragment->nodes = nodes;
        fragment->capacity = capacity;
    }
    fragment->nodes[fragment->count++] = view;
    *out = fragment->count;
    return NL_CHECK_OK;
}

void nl_checked_destroy(NLCheckedFragment *fragment)
{
    if (fragment != NULL) {
        for (size_t i = 0; i < fragment->arm_count; ++i)
            nl_checked_destroy(fragment->arms[i].artifact);
        free(fragment->arms);
        if (fragment->destroy_context != NULL)
            fragment->destroy_context((NLSemanticContext *)fragment->context);
        free(fragment->nodes);
        free(fragment);
    }
}

NLCheckedNodeId nl_checked_root(const NLCheckedFragment *fragment)
{
    return fragment == NULL ? 0 : fragment->root;
}

size_t nl_checked_node_count(const NLCheckedFragment *fragment)
{
    return fragment == NULL ? 0 : fragment->count;
}

const NLCheckedNodeView *nl_checked_node_view(const NLCheckedFragment *fragment,
                                              NLCheckedNodeId id)
{
    return fragment == NULL || id == 0 || id > fragment->count
               ? NULL
               : &fragment->nodes[id - 1];
}

const NLSource *nl_checked_source(const NLCheckedFragment *fragment)
{
    return fragment == NULL ? NULL : fragment->source;
}

const NLSemanticContext *nl_checked_context(const NLCheckedFragment *fragment)
{
    return fragment == NULL ? NULL : fragment->context;
}

const NLCheckedFragment *nl_checked_match_arm(const NLCheckedFragment *f,
                                              NLCheckedNodeId match,
                                              size_t index)
{
    if (f != NULL)
        for (size_t i = 0; i < f->arm_count; ++i)
            if (f->arms[i].match == match) {
                if (index == 0)
                    return f->arms[i].artifact;
                --index;
            }
    return NULL;
}
