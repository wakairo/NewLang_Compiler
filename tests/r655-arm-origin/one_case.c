/* R #664: independent test-private source-issued IF-world identity check.
 * Reuses R662's proven private admission idiom, not the node fallback. */
#define main r664_compiler_driver_main
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
    NLCheckedFragment *changed = NULL;
    const NLSemanticContext *original_world = NULL;
    bool dirty = false, success = false;
    REQUIRE(node_checked_load(path, &fixture));
    REQUIRE(admission(&fixture));

    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        fixture.entry, nl_checked_root(fixture.entry));
    REQUIRE(body != NULL);
    NLCheckedNodeId choice = 0;
    for (size_t id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, id);
        REQUIRE(node != NULL);
        if (node->kind == NL_CHECKED_IF) {
            REQUIRE(choice == 0);
            choice = id;
        }
    }
    REQUIRE(choice != 0 && body->arm_count == 2);
    NLCheckedFragment *first = body->arms[0].artifact;
    const NLCheckedFragment *second = body->arms[1].artifact;
    REQUIRE(first != NULL && second != NULL && first != second);
    REQUIRE(nl_checked_if_arm(body, choice, 0) == first &&
            nl_checked_if_arm(body, choice, 1) == second);
    for (size_t index = 0; index < 2; ++index) {
        const NLCheckedFragment *arm = body->arms[index].artifact;
        REQUIRE(body->arms[index].match == choice);
        REQUIRE(arm->if_parent == body && arm->if_parent_node == choice &&
                arm->if_arm_index == index && arm->source == body->source);
        REQUIRE(copy_if_arm_origin(body, choice, index, arm));
    }

    original_world = nl_checked_context(first);
    const NLSemanticContext *other_world = nl_checked_context(second);
    REQUIRE(original_world != NULL && other_world != NULL &&
            original_world != other_world &&
            original_world != nl_checked_context(body) &&
            other_world != nl_checked_context(body));
    REQUIRE(body->arms[0].context_origin == original_world &&
            body->arms[1].context_origin == other_world);
    REQUIRE(nl_sem_validate(original_world) == NL_CHECK_OK &&
            nl_sem_validate(other_world) == NL_CHECK_OK);

    unsigned char child_bytes[sizeof(*first)];
    unsigned char issuer_bytes[2 * sizeof(NLCheckedArm)];
    memcpy(child_bytes, first, sizeof(child_bytes));
    memcpy(issuer_bytes, body->arms, sizeof(issuer_bytes));

    /* Swap only the first child association to the other valid live world.
     * The parent-owned issuance records are not edited. */
    changed = first;
    changed->context = other_world;
    dirty = true;
    REQUIRE(memcmp(child_bytes, changed, sizeof(child_bytes)) != 0);
    REQUIRE(memcmp(issuer_bytes, body->arms, sizeof(issuer_bytes)) == 0);
    const bool inconsistent_accepted = admission(&fixture);

    changed->context = original_world;
    dirty = false;
    REQUIRE(memcmp(child_bytes, changed, sizeof(child_bytes)) == 0);
    REQUIRE(memcmp(issuer_bytes, body->arms, sizeof(issuer_bytes)) == 0);
    REQUIRE(!inconsistent_accepted);
    REQUIRE(admission(&fixture));

    puts("R664 genuine V0: ACCEPT -> one arm world mismatch REJECT -> "
         "restored ACCEPT");
    success = true;

cleanup:
    if (dirty)
        changed->context = original_world;
    node_checked_destroy(&fixture);
    return success;
}

int main(int argc, char **argv)
{
    return argc == 2 && one_case(argv[1]) ? 0 : 1;
}
