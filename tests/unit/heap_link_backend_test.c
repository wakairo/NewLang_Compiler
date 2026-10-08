#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>
void *__real_malloc(size_t);
static bool fail_malloc;
void *__wrap_malloc(size_t n)
{
    return fail_malloc ? NULL : __real_malloc(n);
}
typedef struct {
    const NLCheckedFragment *world;
    NLCheckedNodeView *root, *field, *read, *change, *reset, *grant;
} Nodes;
static bool walk(const NLCheckedFragment *f, Nodes *n)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        NLCheckedNodeView *v = (NLCheckedNodeView *)nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_REF_FROM_PTR && !n->root) {
            n->root = v;
            n->world = f;
        }
        if (v->kind == NL_CHECKED_FIELD_REF && !n->field)
            n->field = v;
        if (v->kind == NL_CHECKED_LINK_READ)
            n->read = v;
        if (v->kind == NL_CHECKED_REPLACE) {
            if (!n->change)
                n->change = v;
            else
                n->reset = v;
        }
        if (v->kind == NL_CHECKED_TRY_ALLOCATE_ONE && v->allocation_success)
            n->grant = v;
        const NLCheckedFragment *b = nl_checked_call_body(f, i);
        if (b)
            CHECK(walk(b, n));
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a)
                CHECK(walk(nl_checked_match_arm(f, i, a), n));
    }
    return true;
}
static bool rejected(TestNode *n)
{
    char *text = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(n->entry, &text, &length) == NL_NODE_C_UNSUPPORTED);
    CHECK(text == NULL && length == 777);
    return true;
}
static bool success(TestNode *n)
{
    char *text = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(n->entry, &text, &length) == NL_NODE_C_OK);
    CHECK(text != NULL && length == strlen(text));
    free(text);
    return true;
}
static bool run(const char *path)
{
    TestNode n = {0};
    Nodes nodes = {0};
    CHECK(node_checked_load(path, &n) && walk(n.entry, &nodes));
    CHECK(nodes.root && nodes.field && nodes.read && nodes.change &&
          nodes.reset && nodes.grant);
    CHECK(success(&n));
    for (size_t k = 0; k < 32; ++k) {
        NLCheckedNodeView *v = k < 9    ? nodes.root
                               : k < 21 ? nodes.field
                               : k < 25 ? nodes.read
                               : k < 29 ? nodes.change
                               : k < 31 ? nodes.reset
                                        : nodes.grant;
        const NLCheckedNodeView saved = *v;
        switch (k) {
        case 0:
            v->argument_count = 0;
            break;
        case 1:
            v->first_argument = 0;
            break;
        case 2:
            v->lifetime_domain = 0;
            break;
        case 3:
            ++v->lifetime_place;
            break;
        case 4:
            ++v->lifetime_incarnation;
            break;
        case 5:
            v->reference_result.scope = 0;
            break;
        case 6:
            v->reference_result.writable = false;
            break;
        case 7:
            v->reference_result.provenance = NL_PROVENANCE_UNKNOWN;
            break;
        case 8:
            v->lifetime_range.region = 0;
            break;
        case 9:
            v->field.present = false;
            break;
        case 10:
            v->field.dependency_compatible = false;
            break;
        case 11:
            v->field.nominal = 0;
            break;
        case 12:
            v->field.index = 1;
            break;
        case 13:
            v->field.parent = 0;
            break;
        case 14:
            ++v->field.child;
            break;
        case 15:
            ++v->field.child_incarnation;
            break;
        case 16:
            v->field.access = NL_ACCESS_READ;
            break;
        case 17:
            v->field.parent_fact = 0;
            break;
        case 18:
            v->field.child_fact = 0;
            break;
        case 19:
            v->first_argument = 0;
            break;
        case 20:
            v->reference_result.readable = false;
            break;
        case 21:
            v->value_use = NL_VALUE_CONSUMED;
            break;
        case 22:
            v->field.index = 1;
            break;
        case 23:
            v->field.old_value = 0;
            break;
        case 24:
            v->results[0].value = 0;
            break;
        case 25:
            v->field.parent_post_fact = v->field.parent_fact;
            break;
        case 26:
            v->field.post_payload_occurrence = 0;
            break;
        case 27:
            v->field.new_value = 0;
            break;
        case 28:
            v->field.old_value = 0;
            break;
        case 29:
            v->field.payload_occurrence = 0;
            break;
        case 30:
            v->field.post_payload_occurrence = 1;
            break;
        case 31:
            v->backing = 0;
            break;
        }
        CHECK(rejected(&n));
        *v = saved;
        CHECK(success(&n));
    }
    NLCheckedNodeView *arg = (NLCheckedNodeView *)nl_checked_node_view(
        nodes.world, nodes.root->first_argument);
    CHECK(arg != NULL);
    NLSymbolId original = arg->symbol, other = 0;
    const NLSemanticContext *c = nl_checked_context(nodes.world);
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(c, &snapshot));
    for (NLSymbolId i = 1; i <= snapshot.bindings; ++i) {
        NLSemanticBindingView b;
        NLSemanticValueView v;
        CHECK(nl_semantic_binding_view(c, i, &b));
        if (b.type == arg->type && nl_semantic_value_view(c, b.value, &v) &&
            v.reference.place != nodes.root->lifetime_place)
            other = i;
    }
    CHECK(other != 0);
    arg->symbol = other;
    CHECK(rejected(&n));
    arg->symbol = original;
    CHECK(success(&n));
    char *text = NULL;
    size_t length = 777;
    fail_malloc = true;
    NLNodeCStatus status = nl_checked_c_node(n.entry, &text, &length);
    fail_malloc = false;
    CHECK(status == NL_NODE_C_OUT_OF_MEMORY && text == NULL && length == 777);
    CHECK(success(&n));
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && run(argv[1]) ? 0 : 1;
}
