#include "source_call.h"
#include <stdlib.h>

static bool observe(const char *path, const char *expression, size_t expected)
{
    TestNode n = {0};
    CHECK(selected_load(path, expression, &n));
    const NLCheckedNodeView *call =
        nl_checked_node_view(n.entry, nl_checked_root(n.entry));
    const NLCheckedFragment *body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(call && body && call->kind == NL_CHECKED_REGISTERED_CALL &&
          call->body_backed && !call->argument_count && !call->first_argument &&
          call->result_count == 1 && nl_checked_context(body) == n.context);
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    CHECK(root);
    NLValueId returned = 0;
    if (!root->terminates) {
        const NLCheckedNodeView *tail = nl_checked_node_view(body, root->tail);
        CHECK(tail && tail->result_count == 1 && root->result_count == 1 &&
              root->results[0].value == tail->results[0].value);
        returned = tail->results[0].value;
    } else {
        for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
            const NLCheckedNodeView *v = nl_checked_node_view(body, i);
            if (v->kind == NL_CHECKED_RETURN) {
                CHECK(!returned);
                returned = v->returned.value;
            }
        }
    }
    NLSemanticValueView value;
    CHECK(returned && returned == call->results[0].value &&
          nl_semantic_value_view(n.context, returned, &value) &&
          value.type == nl_semantic_core_type(n.context, NL_TYPE_U8) &&
          value.scalar_known && value.scalar_value == expected &&
          value.dependencies == NL_DEPENDENCY_FREE &&
          !value.value_dependency_count);
    printf("{\"function\":%zu,\"result_value\":%zu,\"value\":%zu,"
           "\"inputs_destroyed\":true,\"genuine_call_body_equal\":true}\n",
           call->function, returned, expected);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 4 && observe(argv[1], argv[2], strtoul(argv[3], NULL, 10))
               ? 0
               : 1;
}
