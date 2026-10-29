#ifndef P1030_SUPPORT_H
#define P1030_SUPPORT_H
#include "../p965/shared.h"
#include "newlang/copy_body.h"

typedef struct {
    NLCheckedFragment *caller, *body;
    NLCheckedNodeId call;
    NLCopyBodyView view;
    NLCopyBodyParameters parameters;
} P1030Call;
static inline bool p1030_calls(TestNode *node, P1030Call out[2])
{
    NLCheckedFragment *caller = (NLCheckedFragment *)nl_checked_call_body(
        node->entry, nl_checked_root(node->entry));
    CHECK(caller && nl_h_custody_validate(node->entry));
    size_t count = 0;
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        NLCopyBodyParameters p;
        if (!nl_checked_copy_body_parameters(caller, id, &p))
            continue;
        CHECK(count < 2);
        P1030Call *c = &out[count++];
        CHECK(nl_checked_copy_body_view(caller, id, &c->view));
        c->caller = caller;
        c->body = (NLCheckedFragment *)c->view.body;
        c->call = id;
        c->parameters = p;
    }
    CHECK(count == 2 && out[0].view.function != out[1].view.function &&
          out[0].parameters.field.parent == out[1].parameters.field.parent &&
          out[0].parameters.field.child != out[1].parameters.field.child);
    return true;
}
static inline bool p1030_valid(const P1030Call out[2])
{
    for (size_t i = 0; i < 2; ++i) {
        CHECK(nl_checked_copy_body_validate(out[i].caller, out[i].call,
                                            &out[i].view) == NL_CHECK_OK &&
              nl_checked_copy_body_parameters_validate(
                  out[i].caller, out[i].call, &out[i].parameters) ==
                  NL_CHECK_OK);
    }
    return true;
}
static inline bool p1030_exact(TestNode *node, const char *expected,
                               size_t bytes)
{
    char *text = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(node->entry, &text, &length) == NL_NODE_C_OK &&
          text && length == bytes && !memcmp(text, expected, bytes));
    free(text);
    return true;
}
static inline bool p1030_unpublished(TestNode *node)
{
    char *text = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(node->entry, &text, &length) ==
              NL_NODE_C_UNSUPPORTED &&
          !text && length == 777);
    return true;
}
#endif
