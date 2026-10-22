#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
static bool valid(TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}
/* Corrupt both the observed graph and its private graph copy: a byte seal
 * alone must not substitute for the independent wrapper/effect contract. */
static bool wrappers(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n) && valid(&n));
    NLCheckedFragment *caller = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    NLCopyUnitView v;
    size_t call = 0;
    for (size_t i = 1; i <= caller->count; ++i)
        if (nl_checked_copy_unit_view(caller, i, &v))
            call = i;
    CHECK(call);
    NLCopyBodyProof *p = ((NLCheckedFragment *)v.body)->copy_body;
    size_t loan = v.parameters.loan;
    size_t block = caller->nodes[loan - 1].initializer;
    size_t statement = caller->nodes[block - 1].first_item;
    size_t rejected = 0;
#define LINK(node, alteration)                                                 \
    do {                                                                       \
        NLCheckedNodeView saved = caller->nodes[(node) - 1];                   \
        caller->nodes[(node) - 1].alteration;                                  \
        p->caller_nodes[(node) - 1] = caller->nodes[(node) - 1];               \
        CHECK(nl_checked_copy_unit_validate(caller, call, NULL) ==             \
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT &&                         \
              nl_checked_copy_body_parameters_validate(caller, call, NULL) ==  \
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT &&                         \
              !valid(&n));                                                     \
        caller->nodes[(node) - 1] = saved;                                     \
        p->caller_nodes[(node) - 1] = saved;                                   \
        CHECK(nl_checked_copy_unit_validate(caller, call, NULL) ==             \
                  NL_CHECK_OK &&                                               \
              valid(&n));                                                      \
        ++rejected;                                                            \
    } while (0)
    LINK(block, first_item = 0);
    LINK(block, item_count = 0);
    LINK(block, item_count = 2);
    LINK(block, tail = call);
    LINK(block, first_item = call);
    LINK(block, terminates = true);
    LINK(block, result_count = 1);
    LINK(statement, kind = NL_CHECKED_BINDING);
    LINK(statement, initializer = 0);
    LINK(statement, initializer = v.parameters.arguments[0]);
    LINK(statement, next_item = statement);
    LINK(statement, next_item = call);
    LINK(statement, tail = call);
    LINK(statement, first_argument = call);
    LINK(statement, argument_count = 1);
    LINK(statement, has_scalar_result = true);
    LINK(statement, has_reference_result = true);
    LINK(statement, result_count = 1);
    LINK(statement, type = nl_semantic_core_type(n.context, NL_TYPE_U8));
    LINK(statement, terminates = true);
    LINK(statement, span.start_byte++);
    LINK(statement, span.end_byte--);
    LINK(caller->root, initializer = call);
#undef LINK
    /* Copying a different caller's complete graph/seal cannot qualify equal
     * numeric IDs: issuer source/fragment/loan/world identity remains exact. */
    TestNode foreign = {0};
    CHECK(node_checked_load(path, &foreign));
    const NLCheckedFragment *foreign_caller =
        nl_checked_call_body(foreign.entry, nl_checked_root(foreign.entry));
    const NLCheckedFragment *owner = p->caller;
    p->caller = foreign_caller;
    CHECK(nl_checked_copy_unit_validate(caller, call, NULL) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          !valid(&n));
    p->caller = owner;
    CHECK(valid(&n));
    node_checked_destroy(&foreign);
    node_checked_destroy(&n);
    printf("wrapper structural rejects=%zu plus foreign caller issuer; "
           "recovery=PASS\n",
           rejected);
    return true;
}
int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;
    for (int i = 1; i < argc; ++i)
        if (!wrappers(argv[i]))
            return 1;
    return 0;
}
