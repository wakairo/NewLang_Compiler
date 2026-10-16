/* Driver-private fault controls on genuinely source-issued owned artifacts.
 * No production hooks, host-created semantic values, or parallel compiler. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

static bool validates(const NLCheckedFragment *f)
{
    V0Program p = {
        .unit = nl_semantic_unit_type(nl_checked_context(f)),
        .u8 = nl_semantic_core_type(nl_checked_context(f), NL_TYPE_U8)};
    NLSemanticSnapshot before = {0}, after = {0};
    if (!nl_semantic_snapshot(nl_checked_context(f), &before))
        return false;
    const bool valid = v0_validate_call(f, nl_checked_root(f), &p);
    if (!nl_semantic_snapshot(nl_checked_context(f), &after) ||
        memcmp(&before, &after, sizeof before) != 0)
        return false;
    return valid;
}

static bool controls(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    CHECK(validates(n.entry));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    size_t equality = 0;
    for (size_t i = 0; i < nl_checked_node_count(f); ++i)
        if (f->nodes[i].kind == NL_CHECKED_USIZE_EQUALITY) {
            CHECK(equality == 0);
            equality = i + 1;
        }
    CHECK(equality != 0);
    NLCheckedNodeView *eq = &f->nodes[equality - 1];
    NLCheckedNodeView *left = &f->nodes[eq->first_argument - 1];
    NLCheckedNodeView *right = &f->nodes[left->next_argument - 1];
    const NLCheckedNodeView saved = *eq, lhs = *left, rhs = *right;
    size_t rejected = 0;
#define REFUSE(change)                                                         \
    do {                                                                       \
        change;                                                                \
        const bool accepted = validates(n.entry);                              \
        *eq = saved;                                                           \
        *left = lhs;                                                           \
        *right = rhs;                                                          \
        CHECK(!accepted && validates(n.entry));                                \
        ++rejected;                                                            \
    } while (0)
    REFUSE(eq->argument_count = 1);
    REFUSE(eq->first_argument = 0);
    REFUSE(eq->first_argument = nl_checked_node_count(f) + 1);
    REFUSE(eq->scalar_result.value ^= 1U);
    REFUSE(eq->has_scalar_result = false);
    REFUSE(eq->results[0].value = lhs.results[0].value);
    REFUSE(eq->name.end_byte += 1);
    REFUSE(eq->span.start_byte += 1);
    REFUSE(right->next_argument = saved.first_argument);
    REFUSE(eq->first_argument = lhs.next_argument);
    /* An earlier genuine quantity literal of the same mathematical value is
     * still not the source child named by this equality's span/edge. */
    NLCheckedNodeId earlier = 0;
    for (size_t i = 1; i < saved.first_argument; ++i)
        if (f->nodes[i - 1].kind == NL_CHECKED_USIZE_LITERAL) {
            earlier = i;
            break;
        }
    if (earlier != 0)
        REFUSE(eq->first_argument = earlier);
    if (left->kind == NL_CHECKED_IDENTIFIER &&
        right->kind == NL_CHECKED_IDENTIFIER && lhs.symbol != rhs.symbol)
        REFUSE(left->symbol = rhs.symbol);
#undef REFUSE
    printf("CONTROLS rejected=%zu original=PASS snapshot=UNCHANGED\n",
           rejected);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && controls(argv[1]) ? 0 : 1;
}
