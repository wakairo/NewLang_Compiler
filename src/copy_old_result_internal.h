#ifndef NEWLANG_COPY_OLD_RESULT_INTERNAL_H
#define NEWLANG_COPY_OLD_RESULT_INTERNAL_H
#include "newlang/copy_body.h"
/* Consume the already-current body, never issue or cache authority. Every
 * item is a pure u8 binding except the optional final RETURN. Trace that
 * result through earlier bindings to this exact Change's old result; equal
 * scalars, incoming parameters and another Change are not that origin.
 * The decreasing prefix bounds the walk without another ledger/allocation. */
static bool copy_old_result(const NLCopyBodyView *v,
                            const NLCopyBodyEventView *event, NLTypeId u8)
{
    const NLCheckedFragment *f = v->body;
    const NLCheckedNodeView *root = nl_checked_node_view(f, v->root);
    if (!root || root->kind != NL_CHECKED_BLOCK ||
        root->item_count > nl_checked_node_count(f) || v->result.type != u8)
        return false;
    NLCheckedNodeId result = root->tail, item = root->first_item;
    size_t limit = root->item_count, changes = 0;
    for (size_t i = 0; i < root->item_count; ++i) {
        const NLCheckedNodeView *b = nl_checked_node_view(f, item);
        if (!b)
            return false;
        if (!root->tail && i + 1 == root->item_count) {
            if (b->kind != NL_CHECKED_RETURN || !b->terminates ||
                b->returned.type != u8 || b->returned.value != v->result.value)
                return false;
            result = b->initializer;
            limit = i;
        } else {
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, b->initializer);
            if (b->kind != NL_CHECKED_BINDING || !b->symbol || !init ||
                init->type != u8 || init->result_count != 1)
                return false;
            if (b->initializer == event->node)
                ++changes;
            else if (init->kind != NL_CHECKED_IDENTIFIER ||
                     init->value_use != NL_VALUE_COPIED)
                return false;
        }
        item = b->next_item;
    }
    const NLCheckedNodeView *end = nl_checked_node_view(f, result);
    if (item || !end || end->type != u8 || end->result_count != 1 ||
        end->results[0].value != v->result.value ||
        (root->tail && (root->terminates || root->result_count != 1 ||
                        root->results[0].value != v->result.value)) ||
        (!root->tail && !root->terminates))
        return false;
    changes += result == event->node;
    if (changes != 1)
        return false;
    while (result != event->node) {
        const NLCheckedNodeView *id = nl_checked_node_view(f, result);
        if (!id || id->kind != NL_CHECKED_IDENTIFIER || id->type != u8 ||
            id->value_use != NL_VALUE_COPIED)
            return false;
        NLCheckedNodeId initializer = 0;
        size_t earlier = limit;
        item = root->first_item;
        for (size_t i = 0; i < limit; ++i) {
            const NLCheckedNodeView *b = nl_checked_node_view(f, item);
            if (!b || b->kind != NL_CHECKED_BINDING)
                return false;
            if (b->symbol == id->symbol) {
                if (initializer)
                    return false;
                initializer = b->initializer;
                earlier = i;
            }
            item = b->next_item;
        }
        if (!initializer)
            return false;
        result = initializer;
        limit = earlier;
    }
    return true;
}
#endif
