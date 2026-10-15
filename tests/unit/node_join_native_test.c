#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include "newlang/node_join.h"
#include <stdlib.h>
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
static bool injecting;
static size_t allocation, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && allocation++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && allocation++ == fail_at ? NULL : __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t size)
{
    return injecting && allocation++ == fail_at ? NULL : __real_calloc(n, size);
}
static bool emit(TestNode *n, NLNodeCStatus expected)
{
    char *out = NULL;
    size_t length = 123;
    CHECK(nl_checked_c_node(n->entry, &out, &length) == expected);
    if (expected == NL_NODE_C_OK) {
        CHECK(out != NULL && length == strlen(out));
        CHECK(strstr(out, "switch (") && strstr(out, "case 0:") &&
              strstr(out, "case 1:"));
        free(out);
    } else
        CHECK(out == NULL && length == 123);
    return true;
}
static bool controls(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    NLNodeJoinProof *p = f->node_joins;
    CHECK(p && p->complete);
    CHECK(nl_checked_node_join_validate(f, p->view.match, NULL) == NL_CHECK_OK);
    CHECK(emit(&n, NL_NODE_C_OK));
    /* The unused original has no pre-match pointer/field operation. Thus
     * proof remains VALID, but its actual constructed carrier incarnation
     * disagrees. This must be caught by the NEW original-carrier gate. */
    NLPlaceId unused = p->view.originals[1];
    NLIncarnationId incarnation = n.context->places[unused - 1].incarnation;
    n.context->places[unused - 1].incarnation += 100;
    CHECK(nl_checked_node_join_validate(f, p->view.match, NULL) == NL_CHECK_OK);
    CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
    n.context->places[unused - 1].incarnation = incarnation;
    CHECK(emit(&n, NL_NODE_C_OK));
    puts("proof VALID but physical original carrier wrong: emission refused");
    NLPlaceId parent =
        p->entry_owned->places[p->view.changed - 1].parent_aggregate;
    NLValueFactId rf = p->post_owned->places[parent - 1].current_fact;
    p->post_owned->places[parent - 1].current_fact =
        p->entry_owned->places[parent - 1].current_fact;
    CHECK(nl_checked_node_join_validate(f, p->view.match, NULL) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
    p->post_owned->places[parent - 1].current_fact = rf;
    puts("poisoned owned joined parent RF: validator and emitter refused");
    NLCheckedFragment *arm = (NLCheckedFragment *)p->view.arms[0].artifact;
    const NLSemanticContext *world = arm->context;
    arm->context = p->view.arms[1].world;
    CHECK(nl_checked_node_join_validate(f, p->view.match, NULL) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
    arm->context = world;
    puts("foreign owned arm world: validator and emitter refused");
    NLCheckedNodeView *match = &f->nodes[p->view.match - 1];
    match->normal_frame_unchanged = true;
    CHECK(nl_checked_node_join_validate(f, p->view.match, NULL) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
    p->complete = false;
    CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
    p->complete = true;
    match->normal_frame_unchanged = false;
    puts("forged unchanged flag with/without witness: emission refused");
    bool complete = p->complete;
    p->complete = false;
    CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
    p->complete = complete;
    size_t count = match->normal_arms;
    match->normal_arms = 3;
    CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
    match->normal_arms = count;
    CHECK(emit(&n, NL_NODE_C_OK));
    node_checked_destroy(&n);
    return true;
}
static bool oom(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    NLNodeJoinView v = {0};
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        (void)nl_checked_node_join_view(f, i, &v);
    CHECK(v.match != 0);
    const NLSemanticContext *worlds[] = {n.context,
                                         v.entry,
                                         v.post,
                                         v.arms[0].entry,
                                         v.arms[0].world,
                                         v.arms[1].entry,
                                         v.arms[1].world,
                                         v.arms[0].before_change,
                                         v.arms[0].after_change,
                                         v.arms[1].before_change,
                                         v.arms[1].after_change};
    size_t count = sizeof(worlds) / sizeof(worlds[0]);
    TestState *states = malloc(count * sizeof(*states));
    CHECK(states != NULL);
    for (size_t i = 0; i < count; ++i)
        if (worlds[i])
            CHECK(test_state((NLSemanticContext *)worlds[i], &states[i]));
    bool success = false;
    for (fail_at = 0; fail_at < 4096; ++fail_at) {
        char *out = NULL;
        size_t length = 123;
        allocation = 0;
        injecting = true;
        NLNodeCStatus status = nl_checked_c_node(n.entry, &out, &length);
        injecting = false;
        for (size_t i = 0; i < count; ++i)
            if (worlds[i])
                CHECK(
                    test_unchanged((NLSemanticContext *)worlds[i], &states[i]));
        if (status == NL_NODE_C_OK) {
            CHECK(out && length == strlen(out));
            free(out);
            success = true;
            break;
        }
        CHECK(status == NL_NODE_C_OUT_OF_MEMORY && out == NULL &&
              length == 123);
    }
    CHECK(success);
    printf("emitter OOM positions=%zu output sentinel and all owned worlds "
           "unchanged\n",
           fail_at);
    free(states);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    return (strcmp(argv[1], "oom") == 0 ? oom(argv[2]) : controls(argv[2])) ? 0
                                                                            : 1;
}
