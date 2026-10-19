/* Equal scalar values do not identify an earlier source binding. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
static bool valid(const TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}
static bool probe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n) && valid(&n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    size_t id = 0;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_IF)
            id = i + 1;
    CHECK(id);
    NLCheckedFragment *arm = (NLCheckedFragment *)nl_checked_if_arm(f, id, 0);
    const NLCheckedNodeView *r = nl_checked_node_view(arm, arm->root),
                            *b = nl_checked_node_view(arm, r->initializer),
                            *d = nl_checked_node_view(arm, b->first_item);
    NLCheckedNodeView *init = &arm->nodes[d->initializer - 1];
    CHECK(init->kind == NL_CHECKED_IDENTIFIER);
    const NLCheckedNodeView saved = *init;
    NLSemanticValueView value;
    CHECK(
        nl_semantic_value_view(arm->context, init->results[0].value, &value) &&
        value.scalar_known);
    bool found = false;
    for (size_t i = 0; i < f->count; ++i) {
        const NLCheckedNodeView *candidate = &f->nodes[i];
        if (candidate->kind != NL_CHECKED_BINDING ||
            candidate->symbol == saved.symbol ||
            candidate->span.end_byte >= init->span.start_byte)
            continue;
        const NLCheckedNodeView *v =
            nl_checked_node_view(f, candidate->initializer);
        NLSemanticValueView other;
        if (!v || v->type != init->type ||
            !nl_semantic_value_view(f->context, v->results[0].value, &other) ||
            !other.scalar_known || other.scalar_value != value.scalar_value)
            continue;
        init->symbol = candidate->symbol;
        const bool accepted = valid(&n);
        *init = saved;
        CHECK(!accepted && valid(&n));
        init->symbol = candidate->symbol;
        init->name = candidate->name;
        init->span = candidate->name;
        const bool moved_span = valid(&n);
        *init = saved;
        CHECK(!moved_span && valid(&n));
        found = true;
        break;
    }
    CHECK(found);
    puts("P633 equal_scalar_different_outer_derivation=REJECT "
         "symbol_and_span_controls=2 original=PASS");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
