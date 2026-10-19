#include "../support/node_checked.h"
#include "newlang/usize_addition.h"
#include "newlang/usize_subtraction.h"

/* Actual registered body after original AST teardown; existing public proof
 * consumers only. No new source/value/permission issuer or semantic seed. */
static bool observe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId quantity = nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    size_t adds = 0, subs = 0, comparisons = 0;
    NLUsizeAdditionView add = {0};
    NLUsizeSubtractionView sub = {0};
    const NLCheckedNodeView *cmp = NULL;
    NLCheckedNodeId cmp_id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_USIZE_ADDITION) {
            CHECK(nl_checked_usize_addition_view(f, i, &add) &&
                  nl_checked_usize_addition_valid(f, i, &add) &&
                  !nl_checked_usize_subtraction_valid(f, i, NULL));
            ++adds;
        } else if (v->kind == NL_CHECKED_USIZE_SUBTRACTION) {
            CHECK(nl_checked_usize_subtraction_view(f, i, &sub) &&
                  nl_checked_usize_subtraction_valid(f, i, &sub) &&
                  !nl_checked_usize_addition_valid(f, i, NULL));
            ++subs;
        } else if (v->kind == NL_CHECKED_USIZE_LESS_THAN) {
            cmp = v;
            cmp_id = i;
            CHECK(!nl_checked_usize_addition_valid(f, i, NULL) &&
                  !nl_checked_usize_subtraction_valid(f, i, NULL));
            ++comparisons;
        }
    }
    CHECK(adds == 1 && subs == 1 && comparisons == 1 && cmp != NULL);
    const NLCheckedNodeView *left =
        nl_checked_node_view(f, cmp->first_argument);
    CHECK(left != NULL && left->result_count == 1 && left->type == quantity);
    const NLCheckedNodeView *right =
        nl_checked_node_view(f, left->next_argument);
    CHECK(right != NULL && right->result_count == 1 &&
          right->type == quantity && right->next_argument == 0 &&
          cmp->argument_count == 2 &&
          cmp->first_argument < left->next_argument &&
          left->next_argument < cmp_id && cmp->result_count == 1 &&
          cmp->type == boolean && left->value_use == NL_VALUE_COPIED &&
          right->value_use == NL_VALUE_COPIED);
    NLSemanticValueView a, b, answer;
    CHECK(nl_semantic_value_view(c, left->results[0].value, &a) &&
          nl_semantic_value_view(c, right->results[0].value, &b) &&
          nl_semantic_value_view(c, cmp->results[0].value, &answer));
    CHECK(a.scalar_known && b.scalar_known && answer.scalar_known &&
          a.dependencies == NL_DEPENDENCY_FREE &&
          b.dependencies == NL_DEPENDENCY_FREE &&
          answer.dependencies == NL_DEPENDENCY_FREE &&
          answer.scalar_value == (size_t)(a.scalar_value < b.scalar_value) &&
          answer.domain == 0 && answer.allocation_region == 0 &&
          answer.reference.place == 0 && answer.reference_count == 0 &&
          answer.value_dependency_count == 0 &&
          cmp->results[0].value > left->results[0].value &&
          cmp->results[0].value > right->results[0].value &&
          a.scalar_value == add.sum && b.scalar_value == sub.difference);
    NLSourceView op;
    CHECK(nl_source_view(nl_checked_source(f), cmp->name, &op) &&
          op.length == 1 && op.bytes[0] == '<');
    NLSymbolId symbol = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_BINDING && v->initializer == cmp_id)
            symbol = v->symbol;
    }
    CHECK(symbol != 0);
    printf("{\"left\":%zu,\"right\":%zu,\"bool\":%zu,\"symbols\":[%zu,%zu],"
           "\"bool_symbol\":%zu,\"add\":[%zu,%zu,%zu],\"sub\":[%zu,%zu,%zu],"
           "\"authority\":0}\n",
           a.scalar_value, b.scalar_value, answer.scalar_value, left->symbol,
           right->symbol, symbol, add.numbers[0], add.numbers[1], add.sum,
           sub.numbers[0], sub.numbers[1], sub.difference);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind != NL_CHECKED_BINDING)
            continue;
        const NLCheckedNodeView *init = nl_checked_node_view(f, v->initializer);
        NLSemanticValueView value;
        CHECK(init != NULL && init->result_count == 1 &&
              nl_semantic_value_view(c, init->results[0].value, &value) &&
              value.scalar_known);
        printf("BIND %zu %s %zu\n", v->symbol,
               value.type == boolean ? "bool" : "usize", value.scalar_value);
    }
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
