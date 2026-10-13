/* Actual-source nested frames, including the unchanged-emitter boundary. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

static bool frames(const NLCheckedFragment *f, V0Program p, size_t *calls)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body == NULL)
            continue;
        NLSymbolId formal = 0;
        CHECK(v1_call_parameter(f, i, &p, &formal));
        if (formal != 0) {
            const NLCheckedNodeView *arg =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *root =
                nl_checked_node_view(body, nl_checked_root(body));
            NLSemanticBindingView binding;
            NLSemanticValueView result;
            CHECK(nl_semantic_binding_view(nl_checked_context(body), formal,
                                           &binding));
            CHECK(binding.value == arg->results[0].value &&
                  binding.type == p.u8);
            CHECK(root->result_count == 1 && v->result_count == 1);
            CHECK(root->results[0].value == v->results[0].value);
            CHECK(nl_semantic_value_view(nl_checked_context(f),
                                         v->results[0].value, &result));
            CHECK(result.type == p.u8 &&
                  result.dependencies == NL_DEPENDENCY_FREE);
            printf("FRAME function=%zu node=%zu formal=%zu actual=%zu "
                   "bound=%zu returned=%zu known=%d scalar=%zu\n",
                   v->function, i, formal, arg->results[0].value, binding.value,
                   v->results[0].value, result.scalar_known,
                   result.scalar_value);
            ++*calls;
        }
        V0Program inner = p;
        inner.formal_body = body;
        inner.formal = formal;
        CHECK(frames(body, inner, calls));
    }
    return true;
}

int main(int argc, char **argv)
{
    TestNode n = {0};
    size_t calls = 0;
    if (argc != 2 || !node_checked_load(argv[1], &n))
        return 1;
    V0Program p = {.unit = nl_semantic_unit_type(n.context),
                   .u8 = nl_semantic_core_type(n.context, NL_TYPE_U8)};
    if (!frames(n.entry, p, &calls) || calls != 4)
        return 1;
    printf("SOURCE CHECK OWNED PASS frames=%zu emitter=%d\n", calls,
           v0_validate_call(n.entry, nl_checked_root(n.entry), &p));
    node_checked_destroy(&n);
    return 0;
}
