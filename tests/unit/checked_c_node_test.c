#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>

void *__real_malloc(size_t);
static bool fail_malloc;
void *__wrap_malloc(size_t n)
{
    return fail_malloc ? NULL : __real_malloc(n);
}
static bool emit(TestNode *n, NLNodeCStatus expected)
{
    char *text = NULL;
    size_t length = 17;
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(n->context, &before));
    CHECK(nl_checked_c_node(n->entry, &text, &length) == expected);
    CHECK(nl_semantic_snapshot(n->context, &after));
    CHECK(before.bindings == after.bindings && before.values == after.values &&
          before.places == after.places &&
          before.last_incarnation == after.last_incarnation &&
          before.last_value_fact == after.last_value_fact);
    if (expected == NL_NODE_C_OK) {
        CHECK(text != NULL && length == strlen(text) && length > 0);
        free(text);
    } else {
        CHECK(text == NULL && length == 17);
    }
    return true;
}
static bool controls(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    CHECK(emit(&n, NL_NODE_C_OK));
    fail_malloc = true;
    CHECK(emit(&n, NL_NODE_C_OUT_OF_MEMORY));
    fail_malloc = false;
    CHECK(emit(&n, NL_NODE_C_OK));
    NLCheckedFragment *f = n.entry->bodies[0];
    size_t tested = 0;
    for (size_t i = 0; i < f->count; ++i) {
        NLCheckedNodeView saved = f->nodes[i];
        if (saved.kind == NL_CHECKED_MATCH) {
            f->nodes[i].normal_frame_unchanged = false;
            CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
            f->nodes[i] = saved;
            NLCheckedFragment *arm =
                (NLCheckedFragment *)nl_checked_match_arm(f, i + 1, 1);
            CHECK(arm != NULL);
            NLCheckedNodeView a = arm->nodes[arm->root - 1];
            arm->nodes[arm->root - 1].symbol = saved.match_binding_prefix;
            CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
            arm->nodes[arm->root - 1] = a;
            ++tested;
        }
        if (saved.kind == NL_CHECKED_AGGREGATE) {
            NLTypeEntry *type = &n.context->types[saved.type - 1];
            bool incomplete = type->incomplete;
            type->incomplete = true;
            CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
            type->incomplete = incomplete;
            ++tested;
        }
        if (saved.kind == NL_CHECKED_PTR_FROM_REF) {
            f->nodes[i].reference_result.provenance = NL_PROVENANCE_INVALID;
            CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
            f->nodes[i] = saved;
            ++tested;
        }
        if (saved.kind == NL_CHECKED_REPLACE) {
            f->nodes[i].field.child_post_fact = 0;
            CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
            f->nodes[i] = saved;
            ++tested;
        }
        if (saved.kind == NL_CHECKED_LOAN_HEADER) {
            f->nodes[i].loan.body_nonescape_proved = false;
            CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
            f->nodes[i] = saved;
            ++tested;
        }
        if (saved.kind == NL_CHECKED_BLOCK && i + 1 == f->root) {
            f->nodes[i].next_item =
                0; /* root's list is bounded independently */
            f->nodes[i].item_count = 10000;
            CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
            f->nodes[i] = saved;
            ++tested;
        }
    }
    CHECK(tested >= 6);
    NLCheckedNodeView *b = &f->nodes[f->root - 1];
    NLCheckedNodeView block = *b;
    NLCheckedNodeView *tail = &f->nodes[block.tail - 1];
    NLCheckedNodeView saved_tail = *tail;
    b->first_item = block.tail;
    b->item_count = 10000;
    tail->next_item = block.tail;
    CHECK(emit(&n, NL_NODE_C_RESOURCE_LIMIT));
    *tail = saved_tail;
    *b = block;
    CHECK(emit(&n, NL_NODE_C_OK));
    CHECK(nl_checked_c_node(NULL, NULL, NULL) == NL_NODE_C_UNSUPPORTED);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && controls(argv[1]) ? 0 : 1;
}
