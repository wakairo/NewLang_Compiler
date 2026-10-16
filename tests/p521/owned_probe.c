/* Source-issued public observations and read-only existing driver predicates.
 * No semantic seed, replacement artifact, or new production verifier. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
static bool observe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    const NLSemanticContext *c = nl_checked_context(f);
    TestState *before = calloc(1, sizeof(*before));
    CHECK(before && test_state(n.context, before));
    V0Program p = {.unit = nl_semantic_unit_type(c),
                   .u8 = nl_semantic_core_type(c, NL_TYPE_U8),
                   .count = 1};
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_AGGREGATE)
            CHECK(avs_shape(f, v->type, &p));
    }
    NLCheckedNodeId selected = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_IF) {
            selected = i;
            NLSemanticValueView value;
            CHECK(nl_semantic_value_view(c, v->results[0].value, &value));
            CHECK(!value.scalar_known &&
                  value.dependencies == NL_DEPENDENCY_FREE);
            printf("IF node=%zu span=%zu-%zu joined=%zu unknown=1 arms=%zu "
                   "backend_selection=%d\n",
                   i, v->span.start_byte, v->span.end_byte, v->results[0].value,
                   v->normal_arms, copy_if_selection(f, i, &p));
        }
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(f, i);
            const NLCheckedNodeView *arg =
                nl_checked_node_view(f, v->first_argument);
            CHECK(body && nl_checked_context(body) == c);
            NLSymbolId formal = 0;
            CHECK(v1_call_parameter(f, i, &p, &formal));
            NLSemanticBindingView binding;
            NLSemanticValueView actual, result;
            CHECK(nl_semantic_binding_view(c, formal, &binding));
            CHECK(binding.value == arg->results[0].value);
            CHECK(nl_semantic_value_view(c, binding.value, &actual));
            CHECK(nl_semantic_value_view(c, v->results[0].value, &result));
            CHECK(actual.scalar_known == result.scalar_known &&
                  actual.dependencies == NL_DEPENDENCY_FREE &&
                  result.dependencies == NL_DEPENDENCY_FREE);
            CHECK(v0_validate_call(f, i, &p));
            printf("CALL node=%zu span=%zu-%zu function=%zu actual=%zu "
                   "formal=%zu formal_value=%zu returned=%zu known=%d "
                   "old_chain=%d body_nodes=%zu\n",
                   i, v->span.start_byte, v->span.end_byte, v->function,
                   arg->results[0].value, formal, binding.value,
                   v->results[0].value, result.scalar_known,
                   selected ? copy_if_u8(f, i, selected, p.u8) : -1,
                   nl_checked_node_count(body));
            for (size_t j = 1; j <= nl_checked_node_count(body); ++j) {
                const NLCheckedNodeView *b = nl_checked_node_view(body, j);
                printf("BODY node=%zu kind=%d symbol=%zu value=%zu\n", j,
                       b->kind, b->symbol,
                       b->result_count ? b->results[0].value : 0);
            }
        }
        if (v->kind == NL_CHECKED_IDENTIFIER || v->kind == NL_CHECKED_IF ||
            v->kind == NL_CHECKED_REGISTERED_CALL ||
            v->kind == NL_CHECKED_U8_LITERAL ||
            v->kind == NL_CHECKED_USIZE_LITERAL ||
            v->kind == NL_CHECKED_USIZE_EQUALITY)
            printf("NODE id=%zu kind=%d value=%zu origin=%zu\n", i, v->kind,
                   v->result_count ? v->results[0].value : 0,
                   bool_source_origin(f, i));
    }
    CHECK(test_unchanged(n.context, before));
    printf("PUBLIC source-issued PASS; existing call-body predicate PASS; "
           "observer snapshot UNCHANGED; program_functions=%zu\n",
           p.count);
    free(before);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
