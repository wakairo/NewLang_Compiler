#include "../support/node_checked.h"
static size_t head_value, tail_value;
static NLSymbolId pointer, stability;
static bool scalar(const NLCheckedFragment *f, NLCheckedNodeId id, size_t *out)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, id);
    CHECK(v != NULL && v->kind == NL_CHECKED_AGGREGATE);
    NLCheckedNodeId field = v->first_argument;
    for (size_t i = 0; i < v->argument_count; ++i) {
        const NLCheckedNodeView *a = nl_checked_node_view(f, field);
        CHECK(a != NULL);
        if (a->field_index == 1) {
            const NLCheckedNodeView *n =
                nl_checked_node_view(f, a->initializer);
            CHECK(n != NULL && n->has_scalar_result && n->scalar_result.known);
            *out = n->scalar_result.value;
            return true;
        }
        field = a->next_argument;
    }
    CHECK(false);
}
static bool walk(const NLCheckedFragment *f)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_INITIALIZE) {
            const NLCheckedNodeView *a =
                nl_checked_node_view(f, v->first_argument);
            CHECK(a != NULL);
            CHECK(scalar(f, a->next_argument, &tail_value));
        }
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *a =
                nl_checked_node_view(f, v->first_argument);
            if (a != NULL && nl_semantic_recursive_local_type(
                                 nl_checked_context(f), a->type))
                CHECK(scalar(f, v->initializer, &head_value));
        }
        if (v->kind == NL_CHECKED_REF_FROM_PTR) {
            const NLCheckedNodeView *p = nl_checked_node_view(
                                        f, v->first_argument),
                                    *s = p == NULL ? NULL
                                                   : nl_checked_node_view(
                                                         f, p->next_argument);
            CHECK(v->argument_count == 2 && p != NULL && s != NULL &&
                  p->value_use == NL_VALUE_COPIED &&
                  s->value_use == NL_VALUE_COPIED && p->symbol != 0 &&
                  s->symbol != 0);
            pointer = p->symbol;
            stability = s->symbol;
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body != NULL)
            CHECK(walk(body));
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a) {
                body = nl_checked_match_arm(f, i, a);
                if (body != NULL)
                    CHECK(walk(body));
            }
    }
    return true;
}
static bool run(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    CHECK(walk(n.entry));
    CHECK(pointer != 0 && stability != 0);
    printf("%zu %zu %zu %zu\n", head_value, tail_value, pointer, stability);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && run(argv[1]) ? 0 : 1;
}
