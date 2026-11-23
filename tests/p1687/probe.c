#include "../../src/division_usize_internal.h"
#include "../../src/h_origin_internal.h"
#include "../support/node_checked.h"

static bool inspect_product(const NLDivisionUsizePlan *helper,
                            const NLCheckedNodeView *call)
{
    const NLCheckedFragment *body = helper->body;
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    const NLCheckedNodeView *tail = nl_checked_node_view(body, helper->result);
    CHECK(root && tail && root->tail == helper->result &&
          tail->kind == NL_CHECKED_IDENTIFIER &&
          tail->value_use == NL_VALUE_COPIED &&
          root->results[0].value == tail->results[0].value &&
          tail->results[0].value == call->results[0].value);
    const NLCheckedNodeView *product = NULL;
    NLSymbolId previous = 0;
    NLValueId previous_value = 0;
    size_t copies = 0;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *item = nl_checked_node_view(body, id);
        CHECK(item && item->kind == NL_CHECKED_BINDING);
        const NLCheckedNodeView *init =
            nl_checked_node_view(body, item->initializer);
        CHECK(init && init->result_count == 1);
        if (product) {
            CHECK(init->kind == NL_CHECKED_IDENTIFIER &&
                  init->value_use == NL_VALUE_COPIED &&
                  init->symbol == previous &&
                  init->results[0].value > previous_value);
            ++copies;
        } else if (init->kind == NL_CHECKED_USIZE_MULTIPLICATION) {
            CHECK(nl_usize_multiplication_valid(body, item->initializer) &&
                  init->scalar_result.known &&
                  init->scalar_result.value == 589);
            product = init;
        }
        previous = item->symbol;
        previous_value = init->results[0].value;
        id = item->next_item;
    }
    CHECK(product && copies == 2 && tail->symbol == previous &&
          tail->results[0].value > previous_value);
    return true;
}

static bool inspect(const char *path)
{
    TestNode loaded = {0};
    CHECK(node_checked_load(path, &loaded));
    NLDivisionUsizePlan entry;
    const NLCheckedNodeId root = nl_checked_root(loaded.entry);
    CHECK(nl_quantity_usize_call_valid(loaded.entry, root, &entry) &&
          nl_h_custody_result_world_present(loaded.entry, root) &&
          entry.body->body_count == 3);
    NLValueId previous_value = 0;
    const NLCheckedFragment *previous_body = NULL;
    for (size_t i = 0; i < entry.body->body_count; ++i) {
        const NLCheckedNodeId id = entry.body->body_calls[i];
        const NLCheckedNodeView *call = nl_checked_node_view(entry.body, id);
        NLDivisionUsizePlan helper;
        CHECK(call && !call->argument_count && call->result_count == 1 &&
              call->results[0].value > previous_value &&
              nl_quantity_usize_call_valid(entry.body, id, &helper) &&
              nl_h_custody_result_valid(entry.body, id) && !helper.formal &&
              helper.body != previous_body && inspect_product(&helper, call));
        previous_value = call->results[0].value;
        previous_body = helper.body;
    }
    puts("three owned results: product binding, two fresh Copies, Copy tail");
    node_checked_destroy(&loaded);
    return true;
}

int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
