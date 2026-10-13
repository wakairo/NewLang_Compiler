#include "../support/node_checked.h"

/* Read-only expectations from the actual owned source artifact. No emitter,
 * prelude, seeded values, or reconstruction of source literal spellings. */
static bool inspect(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(body != NULL);
    printf("{\"parse\":0,\"register\":0,\"check\":0,\"owned\":true}\n");
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(body, i);
        if (v->kind != NL_CHECKED_REGISTERED_CALL)
            continue;
        NLSemanticTypeView t;
        CHECK(nl_semantic_type_view(nl_checked_context(body), v->type, &t));
        if (t.kind != NL_TYPE_U8)
            continue;
        NLSemanticValueView value;
        CHECK(v->body_backed && v->argument_count == 0 &&
              v->result_count == 1 && nl_checked_call_body(body, i) != NULL &&
              nl_semantic_value_view(nl_checked_context(body),
                                     v->results[0].value, &value) &&
              value.type == v->type && value.scalar_known);
        printf("{\"call\":%zu,\"function\":%zu,\"type\":%zu,"
               "\"value\":%zu}\n",
               i, v->function, v->type, value.scalar_value);
    }
    node_checked_destroy(&n);
    return true;
}

int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
