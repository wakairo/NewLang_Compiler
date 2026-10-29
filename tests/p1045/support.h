#ifndef P1045_SUPPORT_H
#define P1045_SUPPORT_H
#include "../p965/shared.h"
#include "newlang/copy_body.h"

typedef struct {
    NLCheckedFragment *caller, *body;
    NLCheckedNodeId call;
    NLCopyBodyView view;
    NLCopyBodyParameters parameters;
} P1045Call;
static inline bool p1045_collect(NLCheckedFragment *caller, size_t depth,
                                 P1045Call out[2], size_t *count)
{
    CHECK(caller && depth <= 2);
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(caller); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(caller, id);
        CHECK(node);
        if (node->kind == NL_CHECKED_MATCH && node->item_count == 2) {
            const NLCheckedNodeView *trial =
                nl_checked_node_view(caller, node->initializer);
            if (trial && trial->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
                for (size_t arm = 0; arm < 2; ++arm)
                    CHECK(
                        p1045_collect((NLCheckedFragment *)nl_checked_match_arm(
                                          caller, id, arm),
                                      depth + 1, out, count));
        }
        NLCopyBodyParameters p;
        if (!nl_checked_copy_body_parameters(caller, id, &p))
            continue;
        CHECK(*count < 2);
        P1045Call *c = &out[(*count)++];
        CHECK(nl_checked_copy_body_view(caller, id, &c->view));
        c->caller = caller;
        c->body = (NLCheckedFragment *)c->view.body;
        c->call = id;
        c->parameters = p;
    }
    return true;
}
static inline bool p1045_calls(TestNode *node, P1045Call out[2])
{
    size_t count = 0;
    CHECK(nl_h_custody_validate(node->entry) &&
          p1045_collect((NLCheckedFragment *)nl_checked_call_body(
                            node->entry, nl_checked_root(node->entry)),
                        0, out, &count) &&
          count == 2);
    CHECK(out[0].caller == out[1].caller &&
          out[0].view.function != out[1].view.function &&
          out[0].parameters.field.parent != out[1].parameters.field.parent &&
          out[0].parameters.field.child != out[1].parameters.field.child &&
          out[0].parameters.loan != out[1].parameters.loan);
    return true;
}
static inline bool p1045_valid(const P1045Call out[2])
{
    for (size_t i = 0; i < 2; ++i) {
        NLCopyBodyEventView event;
        CHECK(
            nl_checked_copy_body_validate(out[i].caller, out[i].call,
                                          &out[i].view) == NL_CHECK_OK &&
            nl_checked_copy_body_parameters_validate(out[i].caller, out[i].call,
                                                     &out[i].parameters) ==
                NL_CHECK_OK &&
            nl_checked_copy_body_event(out[i].caller, out[i].call, 0, &event) &&
            nl_checked_copy_body_event_validate(out[i].caller, out[i].call, 0,
                                                &event) == NL_CHECK_OK);
    }
    return true;
}
static inline bool p1045_exact(TestNode *node, const char *expected,
                               size_t bytes)
{
    char *text = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(node->entry, &text, &length) == NL_NODE_C_OK &&
          text && length == bytes && !memcmp(text, expected, bytes));
    free(text);
    return true;
}
static inline bool p1045_unpublished(TestNode *node)
{
    char *text = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(node->entry, &text, &length) ==
              NL_NODE_C_UNSUPPORTED &&
          !text && length == 777);
    return true;
}
#endif
