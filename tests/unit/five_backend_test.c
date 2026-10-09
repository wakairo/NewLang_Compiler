#include "../support/node_checked.h"
#include "newlang/captured_closure.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool counting, injecting;
static size_t calls, failure;
static bool fail(void)
{
    if (!counting)
        return false;
    size_t i = calls++;
    return injecting && i == failure;
}
void *__wrap_malloc(size_t n)
{
    return fail() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return fail() ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return fail() ? NULL : __real_realloc(p, n);
}
static bool rejection(TestNode *n)
{
    char *out = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(n->entry, &out, &length) == NL_NODE_C_UNSUPPORTED);
    CHECK(out == NULL && length == 777);
    return true;
}
static bool coverage(TestNode *n, const NLCheckedFragment *f, size_t *attacks)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        NLCheckedNodeView *v = (NLCheckedNodeView *)nl_checked_node_view(f, i);
        const NLCheckedNodeView saved = *v;
        if (v->kind == NL_CHECKED_BLOCK && v->item_count != 0) {
            /* Authenticated operation exists in node array but a poisoned
             * traversal skips its containing block's effectful items. */
            v->item_count = 0;
            v->first_item = 0;
            CHECK(rejection(n));
            *v = saved;
            ++*attacks;
        }
        if (v->kind == NL_CHECKED_U8_LITERAL) {
            v->scalar_result.value ^= 1u;
            CHECK(rejection(n));
            *v = saved;
            ++*attacks;
        }
        if (v->kind == NL_CHECKED_DOMAIN_CREATE) {
            ++v->lifetime_domain;
            CHECK(rejection(n));
            *v = saved;
            ++*attacks;
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER) {
            ++v->loan.domain;
            CHECK(rejection(n));
            *v = saved;
            v->loan.source = 0;
            CHECK(rejection(n));
            *v = saved;
            v->loan.is_exclusive = !saved.loan.is_exclusive;
            CHECK(rejection(n));
            *v = saved;
            *attacks += 3;
        }
        if (v->kind == NL_CHECKED_SUM_CONSTRUCTOR && v->variant == 2) {
            /* Point the C traversal at another valid same-type source operand,
             * while the immutable semantic payload still names the original. */
            for (size_t j = 1; j <= nl_checked_node_count(f); ++j) {
                const NLCheckedNodeView *other = nl_checked_node_view(f, j);
                if (other->kind != NL_CHECKED_SUM_CONSTRUCTOR ||
                    other->variant != 2 ||
                    other->initializer == saved.initializer)
                    continue;
                v->initializer = other->initializer;
                CHECK(rejection(n));
                *v = saved;
                ++*attacks;
                break;
            }
        }
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a)
                CHECK(coverage(n, nl_checked_match_arm(f, i, a), attacks));
    }
    return true;
}
static bool test(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    size_t match = 0;
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i)
        if (nl_checked_node_view(body, i)->kind == NL_CHECKED_MATCH) {
            match = i;
            break;
        }
    NLCapturedClosureView before, after;
    CHECK(nl_checked_captured_closure_view(body, match, &before));
    NLSemanticSnapshot snapshot, current;
    CHECK(nl_semantic_snapshot(nl_checked_context(body), &snapshot));
    char *baseline = NULL;
    size_t length = 0;
    counting = true;
    NLNodeCStatus status = nl_checked_c_node(n.entry, &baseline, &length);
    counting = false;
    size_t total = calls;
    CHECK(status == NL_NODE_C_OK && total != 0 && baseline &&
          length == strlen(baseline));
    for (failure = 0; failure < total; ++failure) {
        char *out = NULL;
        size_t size = 777;
        calls = 0;
        counting = injecting = true;
        status = nl_checked_c_node(n.entry, &out, &size);
        counting = injecting = false;
        CHECK(status == NL_NODE_C_OUT_OF_MEMORY && out == NULL && size == 777);
        CHECK(nl_checked_captured_closure_view(body, match, &after) &&
              memcmp(&before, &after, sizeof(before)) == 0);
        CHECK(nl_semantic_snapshot(nl_checked_context(body), &current) &&
              memcmp(&snapshot, &current, sizeof(snapshot)) == 0);
        CHECK(nl_checked_c_node(n.entry, &out, &size) == NL_NODE_C_OK);
        CHECK(size == length && memcmp(out, baseline, length + 1) == 0);
        free(out);
    }
    size_t attacks = 0;
    NLCheckedNodeView *entry = (NLCheckedNodeView *)nl_checked_node_view(
        n.entry, nl_checked_root(n.entry));
    const NLCheckedNodeView saved_entry = *entry;
    entry->type = nl_semantic_core_type(n.context, NL_TYPE_U8);
    CHECK(rejection(&n));
    *entry = saved_entry;
    ++attacks;
    CHECK(coverage(&n, body, &attacks) && attacks != 0);
    char *retry = NULL;
    size_t size = 0;
    CHECK(nl_checked_c_node(n.entry, &retry, &size) == NL_NODE_C_OK &&
          size == length && memcmp(retry, baseline, length + 1) == 0);
    free(retry);
    free(baseline);
    node_checked_destroy(&n);
    printf("five emitter: exhaustive %zu malloc/calloc/realloc failures; %zu "
           "traversal/scalar/payload/entry/domain/loan poisons; immutable "
           "snapshot, unchanged "
           "output/length, byte-identical retry\n",
           total, attacks);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && test(argv[1]) ? 0 : 1;
}
