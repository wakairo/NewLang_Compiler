/* Private backend guard test on real source-owned artifacts, never fixtures. */
#define main p878_embedded_driver_main
#include "../../src/main.c"
#undef main
#include "../p866/support.h"

static bool admitted(P866 *o)
{
    V0Program p = {.unit = nl_semantic_unit_type(o->source.context),
                   .u8 = nl_semantic_core_type(o->source.context, NL_TYPE_U8)};
    return v0_validate_call(o->source.entry, nl_checked_root(o->source.entry),
                            &p);
}
static bool exercise(const char *path)
{
    P866 o = {0}, foreign = {0};
    CHECK(p866_load(path, &o) && p866_load(path, &foreign) && p866_valid(&o) &&
          admitted(&o));
    NLCheckedNodeView *binding = NULL;
    for (size_t i = 0; i < o.caller->count; ++i) {
        NLCheckedNodeView *n = &o.caller->nodes[i];
        if (n->kind == NL_CHECKED_BINDING &&
            n->initializer == o.unit.parameters.loan) {
            CHECK(binding == NULL);
            binding = n;
        }
    }
    CHECK(binding && v0_unit_loan_binding(o.caller, binding));
    NLCheckedNodeView *loan = &o.caller->nodes[o.unit.parameters.loan - 1];
    NLCheckedNodeView *receiver = &o.caller->nodes[binding->first_argument - 1];
    const NLCheckedNodeView old_binding = *binding;
    const NLCheckedNodeView old_loan = *loan;
    const NLCheckedNodeView old_receiver = *receiver;
    size_t rejected = 0;
#define REJECT(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(!admitted(&o));                                                  \
        restore;                                                               \
        CHECK(p866_valid(&o) && admitted(&o));                                 \
        ++rejected;                                                            \
    } while (0)
    REJECT(binding->result_count = 1, *binding = old_binding);
    REJECT(binding->argument_count = 0, *binding = old_binding);
    REJECT(binding->symbol = 0, *binding = old_binding);
    REJECT(binding->initializer = 0, *binding = old_binding);
    REJECT(receiver->symbol++, *receiver = old_receiver);
    REJECT(receiver->type = nl_semantic_core_type(o.source.context, NL_TYPE_U8),
           *receiver = old_receiver);
    REJECT(loan->result_count = 1, *loan = old_loan);
    REJECT(loan->loan.scope = 0, *loan = old_loan);
    REJECT(loan->loan.place++, *loan = old_loan);
    REJECT(loan->field.child_fact++, *loan = old_loan);
    const NLSource *source = o.caller->source;
    REJECT(o.caller->source = foreign.caller->source,
           o.caller->source = source);
#undef REJECT
    printf("P878 source-owned unit binding symbol=%zu scope=%zu "
           "zero-result; %zu backend mutations rejected/recovered\n",
           binding->symbol, loan->loan.scope, rejected);
    node_checked_destroy(&o.source);
    node_checked_destroy(&foreign.source);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && exercise(argv[1]) && exercise(argv[2]) ? 0 : 1;
}
