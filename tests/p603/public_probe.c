#include "../support/node_checked.h"

/* Source-linked public observations, not a new independent validator. */
static bool literal_origin(const NLCheckedFragment *f, NLCheckedNodeId id,
                           size_t expected)
{
    while (id != 0) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        CHECK(v != NULL);
        if (v->kind == NL_CHECKED_USIZE_LITERAL)
            return v->has_scalar_result && v->scalar_result.known &&
                   v->scalar_result.value == expected;
        CHECK(v->kind == NL_CHECKED_IDENTIFIER &&
              v->value_use == NL_VALUE_COPIED && v->symbol != 0);
        NLCheckedNodeId origin = 0;
        for (size_t i = 1; i < id; ++i) {
            const NLCheckedNodeView *b = nl_checked_node_view(f, i);
            if (b->kind == NL_CHECKED_BINDING && b->symbol == v->symbol &&
                b->initializer != 0 && b->initializer < i) {
                origin = b->initializer;
                break;
            }
        }
        CHECK(origin != 0);
        id = origin;
    }
    return false;
}

static bool observe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    NLCheckedNodeId equality_id = 0;
    const NLCheckedNodeView *eq = NULL;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_USIZE_LESS_THAN) {
            CHECK(eq == NULL);
            eq = v;
            equality_id = i;
        }
    }
    CHECK(eq != NULL && eq->argument_count == 2 && eq->type == boolean &&
          eq->result_count == 1 && eq->results[0].type == boolean &&
          eq->has_scalar_result && eq->scalar_result.type == boolean &&
          eq->scalar_result.known);
    const NLCheckedNodeView *left = nl_checked_node_view(f, eq->first_argument);
    CHECK(left != NULL && left->next_argument != 0);
    const NLCheckedNodeView *right =
        nl_checked_node_view(f, left->next_argument);
    CHECK(right != NULL && right->next_argument == 0 &&
          eq->first_argument < left->next_argument &&
          left->next_argument < equality_id && left->type == usize &&
          right->type == usize &&
          (left->kind == NL_CHECKED_USIZE_LITERAL ||
           left->value_use == NL_VALUE_COPIED) &&
          (right->kind == NL_CHECKED_USIZE_LITERAL ||
           right->value_use == NL_VALUE_COPIED));
    NLSourceView op;
    CHECK(nl_source_view(nl_checked_source(f), eq->name, &op) &&
          op.length == 1 && memcmp(op.bytes, "<", 1) == 0 &&
          eq->span.start_byte == left->span.start_byte &&
          eq->span.end_byte == right->span.end_byte &&
          left->span.end_byte <= eq->name.start_byte &&
          eq->name.end_byte <= right->span.start_byte);
    NLSemanticValueView a, b, result;
    CHECK(nl_semantic_value_view(c, left->results[0].value, &a) &&
          nl_semantic_value_view(c, right->results[0].value, &b) &&
          nl_semantic_value_view(c, eq->results[0].value, &result));
    CHECK(a.type == usize && b.type == usize && result.type == boolean &&
          a.scalar_known && b.scalar_known && result.scalar_known &&
          a.dependencies == NL_DEPENDENCY_FREE &&
          b.dependencies == NL_DEPENDENCY_FREE &&
          result.dependencies == NL_DEPENDENCY_FREE &&
          result.scalar_value == (a.scalar_value < b.scalar_value ? 1U : 0U) &&
          result.scalar_value == eq->scalar_result.value &&
          eq->results[0].value > left->results[0].value &&
          eq->results[0].value > right->results[0].value &&
          result.value_dependency_count == 0 && result.domain == 0 &&
          result.allocation_region == 0 && result.slot_place == 0 &&
          result.reference.place == 0 && result.reference.scope == 0 &&
          result.reference.incarnation == 0 && result.reference_count == 0 &&
          result.field_count == 0 && result.sum_payload == 0 &&
          result.sum_owner == 0 && result.aggregate_owner == 0);
    CHECK(literal_origin(f, eq->first_argument, a.scalar_value) &&
          literal_origin(f, left->next_argument, b.scalar_value));
    NLSymbolId equal_symbol = 0;
    for (size_t i = equality_id + 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_BINDING && v->initializer == equality_id)
            equal_symbol = v->symbol;
    }
    CHECK(equal_symbol != 0);
    size_t copies = 0, conditionals = 0;
    for (size_t i = equality_id + 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_IDENTIFIER && v->symbol == equal_symbol) {
            NLSemanticValueView copy;
            CHECK(v->value_use == NL_VALUE_COPIED && v->type == boolean &&
                  nl_semantic_value_view(c, v->results[0].value, &copy) &&
                  copy.scalar_known &&
                  copy.scalar_value == result.scalar_value &&
                  v->results[0].value != eq->results[0].value);
            ++copies;
        }
        if (v->kind == NL_CHECKED_IF) {
            const NLCheckedNodeView *condition =
                nl_checked_node_view(f, v->initializer);
            CHECK(condition != NULL &&
                  condition->kind == NL_CHECKED_IDENTIFIER &&
                  condition->symbol == equal_symbol &&
                  condition->type == boolean);
            for (size_t j = 0; j < 2; ++j) {
                const NLCheckedFragment *arm = nl_checked_if_arm(f, i, j);
                CHECK(arm != NULL && nl_checked_context(arm) != c);
                const NLCheckedNodeView *body =
                    nl_checked_node_view(arm, nl_checked_root(arm));
                CHECK(body != NULL && body->kind == NL_CHECKED_IF_ARM &&
                      !body->terminates);
            }
            ++conditionals;
        }
    }
    CHECK(copies == 1);
    printf("{\"left\":%zu,\"right\":%zu,\"bool\":%zu,\"comparison\":%zu,"
           "\"children\":[%zu,%zu],\"symbols\":[%zu,%zu],"
           "\"values\":[%zu,%zu,%zu],\"operator\":[%zu,%zu],"
           "\"bool_symbol\":%zu,\"if_checks\":%zu,\"authority\":0}\n",
           a.scalar_value, b.scalar_value, result.scalar_value, equality_id,
           eq->first_argument, left->next_argument, left->symbol, right->symbol,
           left->results[0].value, right->results[0].value,
           eq->results[0].value, eq->name.start_byte, eq->name.end_byte,
           equal_symbol, conditionals);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *binding = nl_checked_node_view(f, i);
        if (binding->kind != NL_CHECKED_BINDING)
            continue;
        const NLCheckedNodeView *init =
            nl_checked_node_view(f, binding->initializer);
        if (init == NULL || init->result_count != 1 ||
            (init->type != usize && init->type != boolean))
            continue;
        NLSemanticValueView package;
        CHECK(nl_semantic_value_view(c, init->results[0].value, &package) &&
              package.scalar_known);
        printf("BIND %zu %s %zu\n", binding->symbol,
               init->type == boolean ? "bool" : "usize", package.scalar_value);
    }
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
