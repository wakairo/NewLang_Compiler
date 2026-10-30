#ifndef P1036_SUPPORT_H
#define P1036_SUPPORT_H
#include "../p965/shared.h"
#include "newlang/copy_body.h"

typedef struct {
    NLCheckedFragment *caller, *body;
    NLCheckedNodeId call;
    NLCopyBodyView view;
    NLCopyBodyParameters parameters;
} P1036Call;
static inline bool p1036_calls(TestNode *node, P1036Call out[3])
{
    NLCheckedFragment *caller = (NLCheckedFragment *)nl_checked_call_body(
        node->entry, nl_checked_root(node->entry));
    CHECK(caller && nl_h_custody_validate(node->entry));
    size_t count = 0;
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        NLCopyBodyParameters p;
        if (!nl_checked_copy_body_parameters(caller, id, &p))
            continue;
        CHECK(count < 3);
        P1036Call *c = &out[count++];
        CHECK(nl_checked_copy_body_view(caller, id, &c->view));
        c->caller = caller;
        c->body = (NLCheckedFragment *)c->view.body;
        c->call = id;
        c->parameters = p;
    }
    CHECK(count == 3);
    for (size_t i = 0; i < 3; ++i) {
        CHECK(out[i].parameters.field.parent == out[0].parameters.field.parent);
        for (size_t j = 0; j < i; ++j)
            CHECK(out[i].view.function != out[j].view.function &&
                  out[i].parameters.loan != out[j].parameters.loan);
    }
    return true;
}
static inline bool p1036_valid(const P1036Call out[3])
{
    for (size_t i = 0; i < 3; ++i) {
        CHECK(nl_checked_copy_body_validate(out[i].caller, out[i].call,
                                            &out[i].view) == NL_CHECK_OK &&
              nl_checked_copy_body_parameters_validate(
                  out[i].caller, out[i].call, &out[i].parameters) ==
                  NL_CHECK_OK);
    }
    return true;
}
static inline bool p1036_exact(TestNode *node, const char *expected,
                               size_t bytes)
{
    char *text = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(node->entry, &text, &length) == NL_NODE_C_OK &&
          text && length == bytes && !memcmp(text, expected, bytes));
    free(text);
    return true;
}
static inline bool p1036_unpublished(TestNode *node)
{
    char *text = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(node->entry, &text, &length) ==
              NL_NODE_C_UNSUPPORTED &&
          !text && length == 777);
    return true;
}
#endif
