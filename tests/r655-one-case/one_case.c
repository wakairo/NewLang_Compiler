/* R #662: one isolated authentic V0 admission regression.
 * This test-private translation unit includes the fixed CLI implementation
 * once; no writable checked-owned API or production issuer is exported. */
#define main r662_compiler_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

#define REQUIRE(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);    \
            goto cleanup;                                                      \
        }                                                                      \
    } while (0)

static bool admission(const TestNode *fixture)
{
    V0Program program = {
        .unit = nl_semantic_unit_type(fixture->context),
        .u8 = nl_semantic_core_type(fixture->context, NL_TYPE_U8)};
    return v0_validate_call(fixture->entry, nl_checked_root(fixture->entry),
                            &program);
}

static bool one_case(const char *path)
{
    TestNode fixture = {0};
    NLCheckedNodeView *changed = NULL;
    NLIncarnationId original_incarnation = 0;
    bool dirty = false, success = false;
    REQUIRE(node_checked_load(path, &fixture));
    REQUIRE(admission(&fixture));

    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        fixture.entry, nl_checked_root(fixture.entry));
    REQUIRE(body != NULL);
    const NLCheckedNodeView *declarations[2] = {0};
    NLCheckedNodeView *writes[2] = {0};
    size_t roots = 0, loans = 0;
    NLCheckedNodeId choice = 0;
    for (size_t id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, id);
        REQUIRE(node != NULL);
        if (node->kind == NL_CHECKED_IF) {
            REQUIRE(choice == 0);
            choice = id;
        }
        if (node->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *initial =
                nl_checked_node_view(body, node->initializer);
            if (initial != NULL && initial->kind == NL_CHECKED_AGGREGATE) {
                REQUIRE(roots < 2);
                declarations[roots++] = node;
            }
        }
        if (node->kind == NL_CHECKED_LOAN_HEADER) {
            REQUIRE(loans < 2);
            const NLCheckedNodeView *loan_block =
                nl_checked_node_view(body, node->initializer);
            REQUIRE(loan_block != NULL &&
                    loan_block->kind == NL_CHECKED_BLOCK &&
                    loan_block->tail > 0 &&
                    loan_block->tail <= nl_checked_node_count(body));
            NLCheckedNodeView *write = &body->nodes[loan_block->tail - 1];
            REQUIRE(write->kind == NL_CHECKED_REPLACE && write->field.present);
            writes[loans++] = write;
        }
    }
    REQUIRE(roots == 2 && loans == 2 && choice != 0);
    const NLCheckedFragment *arm = nl_checked_if_arm(body, choice, 0);
    REQUIRE(arm != NULL);
    const NLSemanticContext *world = nl_checked_context(arm);
    REQUIRE(world != NULL);
    for (size_t k = 0; k < 2; ++k) {
        NLSemanticBindingView binding;
        NLSemanticPlaceView root;
        REQUIRE(nl_semantic_binding_view(world, declarations[k]->symbol, &binding));
        REQUIRE(nl_semantic_place_view(world, binding.place, &root));
        REQUIRE(declarations[k]->span.start_byte < writes[k]->span.start_byte);
        REQUIRE(writes[k]->field.base == declarations[k]->symbol &&
                writes[k]->field.index == 0 &&
                writes[k]->field.parent == binding.place &&
                root.fixed_field_count == 2 &&
                writes[k]->field.child == root.fixed_fields[0] &&
                writes[k]->field.parent_incarnation == root.incarnation);
    }
    REQUIRE(writes[0]->field.parent != writes[1]->field.parent);
    REQUIRE(writes[0]->field.parent_incarnation !=
            writes[1]->field.parent_incarnation);

    /* One in-range but wrong original Pair incarnation relationship.
     * Only the FIRST write's parent_incarnation is changed, not an issuer,
     * a public view, the source, the final semantic world or its oracle. */
    changed = writes[0];
    original_incarnation = changed->field.parent_incarnation;
    const NLIncarnationId other = writes[1]->field.parent_incarnation;
    unsigned char original_bytes[sizeof(*changed)];
    memcpy(original_bytes, changed, sizeof(original_bytes));
    changed->field.parent_incarnation = other;
    dirty = true;
    REQUIRE(memcmp(original_bytes, changed, sizeof(original_bytes)) != 0);
    const bool inconsistent_accepted = admission(&fixture);
    changed->field.parent_incarnation = original_incarnation;
    dirty = false;
    REQUIRE(memcmp(original_bytes, changed, sizeof(original_bytes)) == 0);
    REQUIRE(!inconsistent_accepted);
    REQUIRE(admission(&fixture));

    puts("R662 genuine V0: ACCEPT -> one inconsistent field REJECT -> restored "
         "ACCEPT");
    success = true;

cleanup:
    if (dirty)
        changed->field.parent_incarnation = original_incarnation;
    node_checked_destroy(&fixture);
    return success;
}

int main(int argc, char **argv)
{
    return argc == 2 && one_case(argv[1]) ? 0 : 1;
}
