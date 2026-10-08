#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
#include <stdlib.h>
void *__real_malloc(size_t);
static bool fail_malloc;
void *__wrap_malloc(size_t n)
{
    return fail_malloc ? NULL : __real_malloc(n);
}
static bool output(TestNode *n, NLNodeCStatus expected)
{
    char *text = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(n->entry, &text, &length) == expected);
    if (expected == NL_NODE_C_OK)
        CHECK(text != NULL && length == strlen(text));
    else
        CHECK(text == NULL && length == 777);
    free(text);
    return true;
}
static bool walk(TestNode *n, const NLCheckedFragment *f, size_t *attacks)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        NLCheckedNodeView *v = (NLCheckedNodeView *)nl_checked_node_view(f, i);
        const NLCheckedNodeView saved = *v;
        size_t count = v->captured_frame_closed                ? 8u
                       : v->kind == NL_CHECKED_DESTROY         ? 4u
                       : v->kind == NL_CHECKED_DEALLOCATE      ? 2u
                       : v->kind == NL_CHECKED_INITIALIZE      ? 5u
                       : v->kind == NL_CHECKED_DOMAIN_CREATE   ? 1u
                       : v->kind == NL_CHECKED_DOMAIN_FINALIZE ? 1u
                                                               : 0u;
        for (size_t a = 0; a < count; ++a) {
            if (v->captured_frame_closed) {
                switch (a) {
                case 0:
                    v->normal_frame_unchanged = true;
                    break;
                case 1:
                    v->captured_frame_closed = false;
                    break;
                case 2:
                    v->captured_backing = 2;
                    break;
                case 3:
                    ++v->captured_root;
                    break;
                case 4:
                    ++v->captured_incarnation;
                    break;
                case 5:
                    ++v->captured_domain;
                    break;
                case 6:
                    v->match_binding_prefix = SIZE_MAX;
                    break;
                case 7:
                    v->captured_allocation = v->captured_domain_binding;
                    break;
                }
            } else if (v->kind == NL_CHECKED_DESTROY) {
                switch (a) {
                case 0:
                    ++v->lifetime_domain;
                    break;
                case 1:
                    ++v->lifetime_place;
                    break;
                case 2:
                    ++v->lifetime_incarnation;
                    break;
                case 3:
                    v->first_argument = 0;
                    break;
                }
            } else if (v->kind == NL_CHECKED_INITIALIZE) {
                switch (a) {
                case 0:
                    ++v->backing;
                    break;
                case 1:
                    ++v->lifetime_domain;
                    break;
                case 2:
                    v->lifetime_range.length = 23;
                    break;
                case 3:
                    v->has_reference_result = false;
                    break;
                case 4:
                    v->reference_result.writable = false;
                    break;
                }
            } else if (v->kind == NL_CHECKED_DOMAIN_CREATE) {
                ++v->lifetime_domain;
            } else {
                if (a == 0)
                    v->argument_count = 0;
                else
                    v->first_argument = 0;
            }
            CHECK(output(n, NL_NODE_C_UNSUPPORTED));
            *v = saved;
            CHECK(output(n, NL_NODE_C_OK));
            ++*attacks;
        }
        if (v->kind == NL_CHECKED_DEALLOCATE) {
            NLCheckedNodeView *arg =
                (NLCheckedNodeView *)nl_checked_node_view(f, v->first_argument);
            NLSemanticSnapshot snapshot;
            CHECK(arg != NULL &&
                  nl_semantic_snapshot(nl_checked_context(f), &snapshot));
            NLSymbolId original = arg->symbol, stale = 0;
            for (NLSymbolId s = 1; s <= snapshot.bindings; ++s) {
                NLSemanticBindingView b;
                CHECK(nl_semantic_binding_view(nl_checked_context(f), s, &b));
                if (s != original && b.type == arg->type &&
                    b.value == arg->results[0].value)
                    stale = s;
            }
            CHECK(stale != 0);
            arg->symbol =
                stale; /* consumed transfer predecessor, same value ID */
            CHECK(output(n, NL_NODE_C_UNSUPPORTED));
            arg->symbol = original;
            CHECK(output(n, NL_NODE_C_OK));
            ++*attacks;
        }
        const NLCheckedFragment *b = nl_checked_call_body(f, i);
        if (b != NULL)
            CHECK(walk(n, b, attacks));
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a)
                CHECK(walk(n, nl_checked_match_arm(f, i, a), attacks));
    }
    return true;
}
static bool resource(TestNode *n)
{
    const NLCheckedFragment *body =
        nl_checked_call_body(n->entry, nl_checked_root(n->entry));
    CHECK(body != NULL);
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(body, i);
        if (v->kind != NL_CHECKED_MATCH)
            continue;
        for (size_t a = 0; a < v->item_count; ++a) {
            const NLCheckedFragment *arm = nl_checked_match_arm(body, i, a);
            const NLCheckedNodeView *root =
                nl_checked_node_view(arm, nl_checked_root(arm));
            if (root->variant != 1)
                continue;
            NLCheckedNodeView *block =
                (NLCheckedNodeView *)nl_checked_node_view(arm, root->tail);
            CHECK(block != NULL && block->kind == NL_CHECKED_BLOCK);
            NLCheckedNodeView *unit =
                (NLCheckedNodeView *)nl_checked_node_view(arm, block->tail);
            CHECK(unit != NULL && unit->kind == NL_CHECKED_UNIT);
            const NLCheckedNodeView saved_block = *block, saved_unit = *unit;
            block->item_count = 600;
            block->first_item = block->tail;
            unit->next_item = block->tail;
            CHECK(output(n, NL_NODE_C_RESOURCE_LIMIT));
            *block = saved_block;
            *unit = saved_unit;
            CHECK(output(n, NL_NODE_C_OK));
            return true;
        }
    }
    CHECK(false);
}
int main(int argc, char **argv)
{
    TestNode n = {0};
    size_t attacks = 0;
    if (argc != 2 || !node_checked_load(argv[1], &n) ||
        !output(&n, NL_NODE_C_OK))
        return 1;
    fail_malloc = true;
    bool ok = output(&n, NL_NODE_C_OUT_OF_MEMORY);
    fail_malloc = false;
    if (!ok || !output(&n, NL_NODE_C_OK) || !walk(&n, n.entry, &attacks))
        return 1;
    if (attacks != 44) {
        fprintf(stderr, "attacks %zu\n", attacks);
        return 1;
    }
    if (!resource(&n))
        return 1;
    node_checked_destroy(&n);
    return 0;
}
