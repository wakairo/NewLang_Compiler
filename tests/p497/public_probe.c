#include "../support/node_checked.h"
#include <stdint.h>

/* Public views only: actual source -> registration -> actual main() child.
 * This observes trusted checked evidence; it is not a new public validator. */
static bool probe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(body != NULL);
    const NLSemanticContext *c = nl_checked_context(body);
    const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE);
    NLSemanticTypeView type;
    CHECK(nl_semantic_type_view(c, usize, &type) && type.is_copy &&
          type.is_discardable && type.kind == NL_TYPE_USIZE);
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    const NLCheckedNodeView *binding =
        nl_checked_node_view(body, root->first_item);
    const NLCheckedNodeView *literal =
        nl_checked_node_view(body, binding->initializer);
    const NLCheckedNodeView *statement =
        nl_checked_node_view(body, binding->next_item);
    const NLCheckedNodeView *copy =
        nl_checked_node_view(body, statement->initializer);
    CHECK(binding->kind == NL_CHECKED_BINDING && binding->symbol != 0 &&
          literal->kind == NL_CHECKED_USIZE_LITERAL && literal->type == usize &&
          literal->result_count == 1 && literal->has_scalar_result &&
          literal->scalar_result.known && literal->scalar_result.type == usize);
    NLSourceView spelling;
    CHECK(nl_source_view(nl_checked_source(body), literal->span, &spelling));
    CHECK(spelling.length >= 8 && memcmp(spelling.bytes, "usize(", 6) == 0);
    CHECK(copy->kind == NL_CHECKED_IDENTIFIER && copy->type == usize &&
          copy->symbol == binding->symbol &&
          copy->value_use == NL_VALUE_COPIED &&
          copy->results[0].value != literal->results[0].value);
    NLSemanticValueView original, copied;
    CHECK(nl_semantic_value_view(c, literal->results[0].value, &original) &&
          nl_semantic_value_view(c, copy->results[0].value, &copied));
    CHECK(original.scalar_known && copied.scalar_known &&
          original.scalar_value == literal->scalar_result.value &&
          original.scalar_value == copied.scalar_value &&
          original.type == usize && copied.type == usize &&
          original.dependencies == NL_DEPENDENCY_FREE &&
          copied.dependencies == NL_DEPENDENCY_FREE && original.domain == 0 &&
          original.allocation_region == 0 && original.reference.place == 0 &&
          original.reference_count == 0 &&
          original.value_dependency_count == 0);
    printf("USIZE value=%zu copied=%zu literal_value=%zu copy_value=%zu "
           "symbol=%zu span=%zu:%zu native_max=%zu authority=0\n",
           original.scalar_value, copied.scalar_value,
           literal->results[0].value, copy->results[0].value, binding->symbol,
           literal->span.start_byte, literal->span.end_byte, SIZE_MAX);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
