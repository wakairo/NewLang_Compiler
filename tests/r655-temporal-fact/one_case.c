/* R #662: one isolated authentic V0 admission regression.
 * This test-private translation unit includes the fixed CLI implementation
 * once; no writable checked-owned API or production issuer is exported. */
#define main r666_compiler_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

#define REQUIRE(condition)                                                     \
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
    NLValueFactId original_post_fact = 0;
    bool dirty = false, success = false;
    REQUIRE(node_checked_load(path, &fixture));
    REQUIRE(admission(&fixture));

    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        fixture.entry, nl_checked_root(fixture.entry));
    REQUIRE(body != NULL);
    const NLCheckedNodeView *declarations[2] = {0};
    NLCheckedNodeView *writes[2] = {0};
    const NLCheckedNodeView *loan_records[2] = {0};
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
            loan_records[loans] = node;
            writes[loans++] = write;
        }
    }
    REQUIRE(roots == 2 && loans == 2 && choice != 0);
    const NLCheckedFragment *arm = nl_checked_if_arm(body, choice, 0);
    REQUIRE(arm != NULL);
    const NLSemanticContext *world = nl_checked_context(arm);
    REQUIRE(world != NULL);
    const NLSemanticContext *post = nl_checked_context(body);
    REQUIRE(post != NULL && nl_sem_validate(world) == NL_CHECK_OK &&
            nl_sem_validate(post) == NL_CHECK_OK && body->arm_count == 2);
    for (size_t k = 0; k < 2; ++k) {
        NLSemanticBindingView binding;
        NLSemanticPlaceView root;
        REQUIRE(
            nl_semantic_binding_view(world, declarations[k]->symbol, &binding));
        REQUIRE(nl_semantic_place_view(world, binding.place, &root));
        REQUIRE(declarations[k]->span.start_byte < writes[k]->span.start_byte);
        NLSemanticPlaceView child;
        REQUIRE(nl_semantic_place_view(world, root.fixed_fields[0], &child));
        NLSemanticScopeView scope;
        REQUIRE(nl_semantic_scope_view(post, loan_records[k]->loan.scope,
                                       &scope) &&
                !scope.active && !scope.parent && !scope.parent_authority);
        REQUIRE(writes[k]->field.parent_fact == root.current_fact &&
                writes[k]->field.child_fact == child.current_fact);
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

    /* Freeze the ONE independent oracle before the changed admission:
     * actual source-issued ACCEPT; replacing the second write's child
     * postfact with the first write's issued postfact REJECT; exact restore
     * ACCEPT. This is not a zero/old/out-of-range gate probe. */
    const NLValueFactId first_post = writes[0]->field.child_post_fact;
    const NLValueFactId second_pre = writes[1]->field.child_fact;
    const NLValueFactId second_post = writes[1]->field.child_post_fact;
    REQUIRE(writes[0]->span.start_byte < writes[1]->span.start_byte &&
            writes[0]->field.parent != writes[1]->field.parent &&
            first_post > 0 && first_post <= post->last_value_fact &&
            second_post <= post->last_value_fact &&
            first_post != second_pre && first_post < second_post &&
            first_post < writes[0]->field.parent_post_fact &&
            second_post < writes[1]->field.parent_post_fact);

    changed = writes[1];
    original_post_fact = second_post;
    unsigned char original_bytes[sizeof(*changed)];
    unsigned char first_bytes[sizeof(*writes[0])];
    unsigned char parent_bytes[sizeof(*body)];
    unsigned char issuer_bytes[2 * sizeof(NLCheckedArm)];
    NLCheckedNodeView expected;
    memcpy(original_bytes, changed, sizeof(original_bytes));
    memcpy(first_bytes, writes[0], sizeof(first_bytes));
    memcpy(parent_bytes, body, sizeof(parent_bytes));
    memcpy(issuer_bytes, body->arms, sizeof(issuer_bytes));
    memcpy(&expected, original_bytes, sizeof(expected));
    expected.field.child_post_fact = first_post;

    changed->field.child_post_fact = first_post;
    dirty = true;
    REQUIRE(memcmp(&expected, changed, sizeof(expected)) == 0 &&
            memcmp(first_bytes, writes[0], sizeof(first_bytes)) == 0 &&
            memcmp(parent_bytes, body, sizeof(parent_bytes)) == 0 &&
            memcmp(issuer_bytes, body->arms, sizeof(issuer_bytes)) == 0);
    const bool inconsistent_accepted = admission(&fixture);
    changed->field.child_post_fact = original_post_fact;
    dirty = false;
    REQUIRE(memcmp(original_bytes, changed, sizeof(original_bytes)) == 0 &&
            memcmp(first_bytes, writes[0], sizeof(first_bytes)) == 0 &&
            memcmp(parent_bytes, body, sizeof(parent_bytes)) == 0 &&
            memcmp(issuer_bytes, body->arms, sizeof(issuer_bytes)) == 0);
    REQUIRE(!inconsistent_accepted);
    REQUIRE(admission(&fixture));

    printf("R666 issued facts first child post=%zu, second child pre=%zu, "
           "second child post=%zu; ACCEPT/REJECT/ACCEPT\n",
           (size_t)first_post, (size_t)second_pre, (size_t)second_post);
    success = true;

cleanup:
    if (dirty)
        changed->field.child_post_fact = original_post_fact;
    node_checked_destroy(&fixture);
    return success;
}

int main(int argc, char **argv)
{
    return argc == 2 && one_case(argv[1]) ? 0 : 1;
}
