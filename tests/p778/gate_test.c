/* Reuse existing immutable P774 H/T tamper and selected OOM checks; additional
 * mutations exercise the new backend-only caller packet carrier. */
#define main p774_historical_gate_main
#include "../p774/gate_test.c"
#undef main
#define MUTATE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(refused(n.entry));                                               \
        restore;                                                               \
        char *again = NULL;                                                    \
        size_t size = 0;                                                       \
        CHECK(nl_checked_c_node(n.entry, &again, &size) == NL_NODE_C_OK &&     \
              size == bytes && !memcmp(again, baseline, bytes));               \
        CHECK(test_unchanged(world, state));                                   \
        free(again);                                                           \
        ++mutations;                                                           \
    } while (0)
static bool caller_carrier(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    NLCheckedFragment *h = NULL;
    NLCheckedNodeId hid = 0;
    CHECK(producer(main_body(&n), &h, &hid));
    const NLTypeId packet = h->nodes[hid - 1].type;
    NLCheckedNodeId binding = 0, use = 0, split = 0;
    NLSymbolId symbol = 0;
    for (size_t i = 1; i <= h->count; ++i) {
        NLCheckedNodeView *b = &h->nodes[i - 1];
        const NLCheckedNodeView *r = nl_checked_node_view(h, b->first_argument);
        if (b->kind == NL_CHECKED_BINDING && r && r->type == packet) {
            CHECK(!binding);
            binding = i;
            symbol = b->symbol;
        }
    }
    for (size_t i = 1; i <= h->count; ++i)
        if (h->nodes[i - 1].kind == NL_CHECKED_IDENTIFIER &&
            h->nodes[i - 1].symbol == symbol) {
            CHECK(!use);
            use = i;
        }
    for (size_t i = 1; i <= h->count; ++i)
        if (h->nodes[i - 1].kind == NL_CHECKED_AGGREGATE_BINDING &&
            h->nodes[i - 1].initializer == use)
            split = i;
    CHECK(binding && symbol && use && split);
    NLSemanticContext *world = (NLSemanticContext *)nl_checked_context(h);
    TestState *state = calloc(1, sizeof(*state));
    CHECK(state && test_state(world, state));
    char *baseline = NULL;
    size_t bytes = 0;
    CHECK(nl_checked_c_node(n.entry, &baseline, &bytes) == NL_NODE_C_OK);
    size_t mutations = 0;
    NLCheckedNodeView b = h->nodes[binding - 1], u = h->nodes[use - 1],
                      s = h->nodes[split - 1];
    NLCheckedNodeId receiver = b.first_argument;
    NLCheckedNodeView r = h->nodes[receiver - 1];
    MUTATE(h->nodes[binding - 1].symbol++, h->nodes[binding - 1] = b);
    MUTATE(h->nodes[receiver - 1].symbol++, h->nodes[receiver - 1] = r);
    MUTATE(h->nodes[use - 1].value_use = NL_VALUE_COPIED,
           h->nodes[use - 1] = u);
    MUTATE(h->nodes[use - 1].results[0].value++, h->nodes[use - 1] = u);
    MUTATE(h->nodes[use - 1].symbol++, h->nodes[use - 1] = u);
    MUTATE(h->nodes[split - 1].initializer = b.initializer,
           h->nodes[split - 1] = s);
    MUTATE(h->nodes[split - 1].argument_count = 2, h->nodes[split - 1] = s);
    NLCheckedNodeId first = s.first_argument;
    NLCheckedNodeView member = h->nodes[first - 1];
    MUTATE(h->nodes[first - 1].field_index = 3, h->nodes[first - 1] = member);
    MUTATE(h->nodes[first - 1].type = packet, h->nodes[first - 1] = member);
    /* Also snapshot the actual H caller arm, beyond the five historical
     * entry/handoff/return worlds checked by the reused gate. */
    size_t positions[] = {0, 1, 7};
    for (size_t i = 0; i < 3; ++i) {
        allocations = 0;
        fail_at = positions[i];
        hit = false;
        char *out = NULL;
        size_t size = 997;
        injecting = true;
        NLNodeCStatus status = nl_checked_c_node(n.entry, &out, &size);
        injecting = false;
        CHECK(hit && !out && size == 997 &&
              (status == NL_NODE_C_OUT_OF_MEMORY ||
               status == NL_NODE_C_UNSUPPORTED));
        CHECK(test_unchanged(world, state));
        CHECK(nl_checked_c_node(n.entry, &out, &size) == NL_NODE_C_OK &&
              size == bytes && !memcmp(out, baseline, bytes));
        free(out);
    }
    printf("P778 caller custody mutations=%zu arm_world_unchanged=PASS "
           "arm_OOM_positions=3 retry=PASS source=%s\n",
           mutations, path);
    free(state);
    free(baseline);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && gate(argv[1]) && caller_carrier(argv[1]) ? 0 : 1;
}
