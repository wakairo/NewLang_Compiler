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
        if (fragment->destroy_owner_entry != NULL)
            fragment->destroy_owner_entry(fragment->owner_entry);
        if (fragment->destroy_captured_post != NULL)
            fragment->destroy_captured_post(fragment->captured_post);
        nl_control_exits_destroy(fragment->loop_returns);
        nl_control_exits_destroy(fragment->exits);
        nl_control_target_destroy(fragment->function_target);
        for (size_t i = 0; i < fragment->arm_count; ++i)
            nl_checked_destroy(fragment->arms[i].artifact);
        free(fragment->arms);
        for (size_t i = 0; i < fragment->body_count; ++i)
            nl_checked_destroy(fragment->bodies[i]);
        free(fragment->bodies);
        free(fragment->body_calls);
        if (fragment->release_body != NULL)
            fragment->release_body(fragment->body_owner);
        if (fragment->destroy_context != NULL)
            fragment->destroy_context((NLSemanticContext *)fragment->context);
        free(fragment->nodes);
        free(fragment);
    }
}

const NLSemanticContext *nl_checked_owner_entry(const NLCheckedFragment *f,
                                                NLCheckedNodeId call)
{
    return f != NULL && call != 0 && f->owner_entry_call == call
               ? f->owner_entry
               : NULL;
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

const NLSemanticContext *nl_checked_captured_post(const NLCheckedFragment *f,
                                                  NLCheckedNodeId match)
{
    return f != NULL && f->captured_match == match ? f->captured_post : NULL;
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

const NLCheckedFragment *nl_checked_if_arm(const NLCheckedFragment *f,
                                           NLCheckedNodeId conditional,
                                           size_t index)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, conditional);
    return v != NULL && v->kind == NL_CHECKED_IF
               ? nl_checked_match_arm(f, conditional, index)
               : NULL;
}

const NLCheckedFragment *nl_checked_loop_body(const NLCheckedFragment *f,
                                              NLCheckedNodeId loop)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, loop);
    return v != NULL && v->kind == NL_CHECKED_LOOP
               ? nl_checked_match_arm(f, loop, 0)
               : NULL;
}

const NLCheckedFragment *nl_checked_call_body(const NLCheckedFragment *f,
                                              NLCheckedNodeId call)
{
    if (f != NULL)
        for (size_t i = 0; i < f->body_count; ++i)
            if (f->body_calls[i] == call)
                return f->bodies[i];
    return NULL;
}

const NLControlExits *nl_checked_control_exits(const NLCheckedFragment *f)
{
    return f == NULL ? NULL : f->exits;
}
