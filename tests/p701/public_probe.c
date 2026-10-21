#include "../support/node_checked.h"
#include "newlang/copy_body.h"
static bool probe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *main_body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(main_body);
    NLCheckedNodeId call = 0, caller_change = 0;
    for (size_t i = 1; i <= nl_checked_node_count(main_body); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(main_body, i);
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            CHECK(!call);
            call = i;
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(!caller_change);
            caller_change = i;
        }
    }
    CHECK(call && caller_change);
    NLCopyBodyView p;
    CHECK(nl_checked_copy_body_view(main_body, call, &p) &&
          nl_checked_copy_body_validate(main_body, call, &p) == NL_CHECK_OK);
    const NLCheckedFragment *body = p.body;
    NLCopyBodyEventView init;
    CHECK(nl_checked_copy_body_event(main_body, call, 0, &init) &&
          init.kind == NL_COPY_BODY_INITIALIZE);
    const NLCheckedNodeView *write =
        nl_checked_node_view(main_body, caller_change);
    const NLCheckedNodeView *ref =
        nl_checked_node_view(main_body, write->first_argument);
    CHECK(write->field.parent != init.field.parent && ref);
    const NLCheckedNodeView *incoming =
        nl_checked_node_view(main_body, ref->next_argument);
    CHECK(incoming);
    NLSemanticValueView value;
    CHECK(nl_semantic_value_view(n.context, p.result.value, &value));
    printf("{\"call\":%zu,\"function\":%zu,\"callee_symbol\":%zu,\"callee_"
           "root\":%zu,\"caller_symbol\":%zu,\"caller_root\":%zu,\"caller_"
           "ref\":%zu,\"caller_incoming\":%zu,\"caller_change\":%zu,\"caller_"
           "index\":%zu,\"result\":%zu,\"changes\":[",
           call, p.function, init.binding, init.field.parent, write->field.base,
           write->field.parent, ref->symbol, incoming->symbol, caller_change,
           write->field.index, value.scalar_value);
    bool comma = false;
    for (size_t i = 0; i < p.event_count; ++i) {
        NLCopyBodyEventView e;
        CHECK(nl_checked_copy_body_event(main_body, call, i, &e) &&
              nl_checked_copy_body_event_validate(main_body, call, i, &e) ==
                  NL_CHECK_OK);
        if (e.kind != NL_COPY_BODY_CHANGE)
            continue;
        const NLCheckedNodeView *op = nl_checked_node_view(body, e.node);
        const NLCheckedNodeView *arg =
            nl_checked_node_view(body, op->first_argument);
        CHECK(arg);
        const NLCheckedNodeView *new_value =
            nl_checked_node_view(body, arg->next_argument);
        CHECK(new_value);
        printf("%s{\"node\":%zu,\"ref\":%zu,\"index\":%zu,\"incoming_symbol\":%"
               "zu,\"parent_fact\":%zu,\"parent_post\":%zu,\"child_fact\":%zu,"
               "\"child_post\":%zu}",
               comma ? "," : "", e.node, arg->symbol, e.field.index,
               new_value->symbol, e.field.parent_fact, e.field.parent_post_fact,
               e.field.child_fact, e.field.child_post_fact);
        comma = true;
    }
    puts("]}");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
