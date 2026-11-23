#include "../../src/division_usize_internal.h"
#include "../../src/h_origin_internal.h"
#include "../support/node_checked.h"

#include <stdlib.h>

static bool refuses_without_output(const NLCheckedFragment *entry)
{
    char original[] = "preserved";
    char *out = original;
    size_t length = sizeof(original) - 1;
    const NLCheckStatus status = nl_quantity_usize_c17(entry, &out, &length);
    const bool refused = status == NL_CHECK_SEMANTIC_UNSUPPORTED &&
                         out == original && length == sizeof(original) - 1 &&
                         !strcmp(original, "preserved");
    if (out != original)
        free(out);
    return refused;
}

static bool inspect(const char *path)
{
    TestNode loaded = {0};
    CHECK(node_checked_load(path, &loaded));
    const NLCheckedNodeId entry_call = nl_checked_root(loaded.entry);
    NLDivisionUsizePlan entry;
    CHECK(nl_quantity_usize_call_valid(loaded.entry, entry_call, &entry) &&
          entry.body->body_count == 3 &&
          nl_h_custody_result_world_present(loaded.entry, entry_call));

    const NLCheckedNodeView *unit =
        nl_checked_node_view(entry.body, nl_checked_root(entry.body));
    NLCheckedNodeId item = unit->first_item;
    NLValueId values[3] = {0};
    const NLCheckedFragment *bodies[3] = {0};
    size_t function = 0;
    for (size_t i = 0; i < 3; ++i) {
        const NLCheckedNodeView *statement =
            nl_checked_node_view(entry.body, item);
        const NLCheckedNodeId id = entry.body->body_calls[i];
        const NLCheckedNodeView *call = nl_checked_node_view(entry.body, id);
        NLDivisionUsizePlan helper;
        CHECK(statement && statement->initializer == id &&
              statement->kind ==
                  (i == 1 ? NL_CHECKED_STATEMENT : NL_CHECKED_BINDING));
        CHECK(call && !call->argument_count && !call->first_argument &&
              call->result_count == 1 &&
              nl_quantity_usize_call_valid(entry.body, id, &helper) &&
              nl_h_custody_result_valid(entry.body, id) && !helper.formal &&
              !helper.second_formal);
        if (!i)
            function = helper.function;
        CHECK(helper.function == function &&
              helper.body->context == entry.body->context);
        bodies[i] = helper.body;
        const NLCheckedNodeView *root =
            nl_checked_node_view(helper.body, nl_checked_root(helper.body));
        const NLCheckedNodeView *product =
            nl_checked_node_view(helper.body, helper.result);
        CHECK(root && product && root->tail == helper.result &&
              nl_usize_multiplication_valid(helper.body, helper.result) &&
              product->results[0].value == root->results[0].value &&
              root->results[0].value == call->results[0].value &&
              product->scalar_result.known &&
              product->scalar_result.value == 391);
        const NLCheckedNodeView *left =
            nl_checked_node_view(helper.body, product->first_argument);
        const NLCheckedNodeView *right =
            left ? nl_checked_node_view(helper.body, left->next_argument)
                 : NULL;
        NLSemanticValueView a, b;
        CHECK(left && right && product->first_argument < left->next_argument &&
              left->next_argument < helper.result &&
              nl_semantic_value_view(helper.body->context,
                                     left->results[0].value, &a) &&
              nl_semantic_value_view(helper.body->context,
                                     right->results[0].value, &b) &&
              a.scalar_known && a.scalar_value == 23 && b.scalar_known &&
              b.scalar_value == 17 && a.dependencies == NL_DEPENDENCY_FREE &&
              b.dependencies == NL_DEPENDENCY_FREE);
        values[i] = call->results[0].value;
        if (i)
            CHECK(values[i] > values[i - 1] && bodies[i] != bodies[i - 1]);
        printf(
            "call %zu: owned zero-argument product 23 * 17 = 391, result=%zu\n",
            i, values[i]);
        item = statement->next_item;
    }

    /* Each real call keeps its own result identity even for equal values. */
    NLCheckedNodeView *checked_calls = entry.body->nodes;
    NLCheckedNodeView *last = &checked_calls[entry.body->body_calls[2] - 1];
    last->results[0].value = values[0];
    const bool stale_rejected = refuses_without_output(loaded.entry);
    last->results[0].value = values[2];
    CHECK(stale_rejected);

    NLCheckedFragment *first = (NLCheckedFragment *)bodies[0];
    struct NLUsizeAdditionProof *old = first->usize_additions;
    first->usize_additions = bodies[1]->usize_additions;
    const bool foreign_rejected = refuses_without_output(loaded.entry);
    first->usize_additions = old;
    CHECK(foreign_rejected);

    char *out = NULL;
    size_t length = 0;
    CHECK(nl_quantity_usize_c17(loaded.entry, &out, &length) == NL_CHECK_OK &&
          out && length);
    free(out);
    puts("other-call result and foreign receipt rejected without C17 output; "
         "restored plan passes");
    node_checked_destroy(&loaded);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
