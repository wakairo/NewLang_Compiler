/* Observations only. Source-issued qualified arms, no host seed/authority. */
#include "../support/node_checked.h"
static bool observe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSemanticContext *c = nl_checked_context(f);
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(c, &snapshot) && snapshot.domains == 0 &&
          snapshot.backing_regions == 0 && snapshot.occurrences == 0 &&
          snapshot.scopes == 0);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    NLCheckedNodeId id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_IF) {
            CHECK(id == 0);
            id = i;
        }
    const NLCheckedNodeView *join = nl_checked_node_view(f, id);
    CHECK(join != NULL && join->normal_arms == 2 && join->type == u8 &&
          join->result_count == 1);
    NLSemanticValueView result, condition;
    const NLCheckedNodeView *cond = nl_checked_node_view(f, join->initializer);
    CHECK(cond != NULL &&
          nl_semantic_value_view(c, cond->results[0].value, &condition) &&
          nl_semantic_value_view(c, join->results[0].value, &result));
    CHECK(condition.scalar_known && condition.scalar_value <= 1 &&
          !result.scalar_known && result.type == u8 &&
          result.dependencies == NL_DEPENDENCY_FREE &&
          result.value_dependency_count == 0 && result.domain == 0 &&
          result.allocation_region == 0 && result.reference.place == 0);
    size_t arm_values[2], arm_symbols[2];
    NLSymbolId selected = 0, copied = 0, equal_symbol = 0;
    const NLCheckedNodeView *eq = NULL;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_USIZE_EQUALITY)
            eq = v;
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, v->initializer);
            if (v->initializer == id)
                selected = v->symbol;
            if (init != NULL && init->kind == NL_CHECKED_USIZE_EQUALITY)
                equal_symbol = v->symbol;
            if (init != NULL && init->kind == NL_CHECKED_IDENTIFIER &&
                init->symbol == selected && selected != 0)
                copied = v->symbol;
        }
    }
    CHECK(eq != NULL && selected != 0 && copied != 0 && equal_symbol != 0);
    const NLCheckedNodeView *lhs = nl_checked_node_view(f, eq->first_argument);
    const NLCheckedNodeView *rhs =
        lhs == NULL ? NULL : nl_checked_node_view(f, lhs->next_argument);
    CHECK(lhs != NULL && rhs != NULL);
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, a);
        CHECK(arm != NULL && nl_checked_context(arm) != c &&
              nl_checked_source(arm) == nl_checked_source(f));
        const NLCheckedNodeView *root =
            nl_checked_node_view(arm, nl_checked_root(arm));
        NLSemanticValueView value;
        CHECK(nl_semantic_snapshot(nl_checked_context(arm), &snapshot) &&
              snapshot.domains == 0 && snapshot.backing_regions == 0 &&
              snapshot.occurrences == 0 && snapshot.scopes == 0);
        CHECK(root != NULL && root->kind == NL_CHECKED_IF_ARM &&
              !root->terminates && root->result_count == 1 &&
              root->type == u8 &&
              nl_semantic_value_view(nl_checked_context(arm),
                                     root->results[0].value, &value));
        CHECK(value.scalar_known && value.type == u8 &&
              value.dependencies == NL_DEPENDENCY_FREE &&
              value.value_dependency_count == 0 && value.domain == 0 &&
              value.allocation_region == 0 && value.reference.place == 0);
        arm_values[a] = value.scalar_value;
        const NLCheckedNodeView *body =
            nl_checked_node_view(arm, root->initializer);
        const NLCheckedNodeView *tail =
            body == NULL ? NULL : nl_checked_node_view(arm, body->tail);
        CHECK(tail != NULL);
        arm_symbols[a] = tail->symbol;
    }
    printf("{\"if\":%zu,\"condition\":%zu,\"condition_symbol\":%zu,"
           "\"bool\":%zu,\"arms\":[%zu,%zu],\"arm_symbols\":[%zu,%zu],"
           "\"selected_symbol\":%zu,\"copied_symbol\":%zu,\"equal_symbol\":%zu,"
           "\"operand_symbols\":[%zu,%zu],\"joined_unknown\":true,"
           "\"authority\":0}\n",
           id, join->initializer, cond->symbol, condition.scalar_value,
           arm_values[0], arm_values[1], arm_symbols[0], arm_symbols[1],
           selected, copied, equal_symbol, lhs->symbol, rhs->symbol);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *b = nl_checked_node_view(f, i);
        if (b->kind != NL_CHECKED_BINDING)
            continue;
        const NLCheckedNodeView *init = nl_checked_node_view(f, b->initializer);
        NLSemanticValueView value;
        CHECK(init != NULL && init->result_count == 1 &&
              nl_semantic_value_view(c, init->results[0].value, &value));
        const char *type =
            value.type == u8                                        ? "u8"
            : value.type == nl_semantic_core_type(c, NL_TYPE_USIZE) ? "usize"
                                                                    : "bool";
        printf("BIND %zu %s %d %zu\n", b->symbol, type, value.scalar_known,
               value.scalar_value);
    }
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
