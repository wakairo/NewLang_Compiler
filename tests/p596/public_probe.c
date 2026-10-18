#include "../support/node_checked.h"
#include "newlang/usize_subtraction.h"
#include <stdint.h>

static bool observe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n)); /* original source/AST destroyed */
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f);
    NLUsizeSubtractionView v;
    size_t count = 0;
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(f, id);
        if (node->kind != NL_CHECKED_USIZE_SUBTRACTION)
            continue;
        CHECK(nl_checked_usize_subtraction_view(f, id, &v));
        CHECK(nl_checked_usize_subtraction_valid(f, id, &v));
        CHECK(v.numbers[0] >= v.numbers[1]);
        CHECK(v.difference == v.numbers[0] - v.numbers[1]);
        CHECK(v.values[0] != v.values[1] && v.result != v.values[0] &&
              v.result != v.values[1]);
        CHECK(v.span.start_byte == v.operand_spans[0].start_byte &&
              v.span.end_byte == v.operand_spans[1].end_byte &&
              v.operand_spans[0].end_byte <= v.operator_span.start_byte &&
              v.operator_span.end_byte <= v.operand_spans[1].start_byte);
        NLSemanticValueView result;
        CHECK(nl_semantic_value_view(nl_checked_context(f), v.result, &result));
        CHECK(result.scalar_value == v.difference && result.domain == 0 &&
              result.reference.place == 0 && result.reference.scope == 0 &&
              result.reference.incarnation == 0 && !result.reference_count &&
              !result.allocation_region && !result.slot_place &&
              !result.value_dependency_count && !result.field_count &&
              !result.sum_payload && !result.sum_owner &&
              !result.aggregate_owner);
        printf("{\"node\":%zu,\"operands\":[%zu,%zu],\"values\":[%zu,%zu,%zu],"
               "\"numbers\":[%zu,%zu],\"difference\":%zu,\"symbols\":[%zu,%zu],"
               "\"places\":[%zu,%zu],\"incarnations\":[%zu,%zu],\"facts\":[%zu,"
               "%zu],"
               "\"span\":[%zu,%zu],\"operator\":[%zu,%zu],\"type\":%zu,"
               "\"dependency_free\":true,\"authority\":0}\n",
               id, v.operands[0], v.operands[1], v.values[0], v.values[1],
               v.result, v.numbers[0], v.numbers[1], v.difference, v.symbols[0],
               v.symbols[1], v.places[0], v.places[1], v.incarnations[0],
               v.incarnations[1], v.facts[0], v.facts[1], v.span.start_byte,
               v.span.end_byte, v.operator_span.start_byte,
               v.operator_span.end_byte, v.type);
        ++count;
    }
    CHECK(count == 1);
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *b = nl_checked_node_view(f, id);
        if (b->kind != NL_CHECKED_BINDING)
            continue;
        const NLCheckedNodeView *init = nl_checked_node_view(f, b->initializer);
        NLSemanticValueView value;
        CHECK(init && init->result_count == 1 &&
              nl_semantic_value_view(nl_checked_context(f),
                                     init->results[0].value, &value));
        printf("BIND %zu %zu %zu\n", b->symbol, init->type, value.scalar_value);
    }
    NLUsizeSubtractionView untouched = v;
    CHECK(!nl_checked_usize_subtraction_view(f, 0, &v) &&
          memcmp(&untouched, &v, sizeof v) == 0);
    CHECK(!nl_checked_usize_subtraction_valid(NULL, 0, NULL));
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
