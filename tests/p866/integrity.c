/* Test-private direct prefix entry isolates operation checks from graph seals.
 * Every positive is actual source-issued; no synthetic semantic world. */
#include "../../src/copy_body.c"
#include "support.h"
static bool run_case(const char *path)
{
    P866 o = {0}, foreign = {0};
    CHECK(p866_load(path, &o) && p866_load(path, &foreign) && p866_valid(&o) &&
          p866_valid(&foreign));
    NLCopyBodyProof *p = o.body->copy_body;
    NLCheckedNodeId condition =
        o.body->nodes[o.unit.conditional - 1].initializer;
    NLCheckedNodeView *n = &o.body->nodes[condition - 1];
    CHECK(n->kind == NL_CHECKED_USIZE_EQUALITY);
    NLSemanticContext *world = (NLSemanticContext *)o.returned.field.entry;
    size_t rejected = 0;
#define PREFIX(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(unit_prefix_validate(p, &o.returned) ==                          \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        restore;                                                               \
        CHECK(p866_valid(&o));                                                 \
        ++rejected;                                                            \
    } while (0)
    NLSemanticValueView truth = world->values[n->results[0].value - 1];
    PREFIX(world->values[n->results[0].value - 1].scalar_value =
               !truth.scalar_value,
           world->values[n->results[0].value - 1] = truth);
    PREFIX(world->values[n->results[0].value - 1].scalar_known = false,
           world->values[n->results[0].value - 1] = truth);
    PREFIX(n->kind = NL_CHECKED_USIZE_LITERAL,
           n->kind = NL_CHECKED_USIZE_EQUALITY);
    NLCheckedNodeView saved = *n;
    PREFIX(n->type = nl_semantic_core_type(world, NL_TYPE_USIZE), *n = saved);
    PREFIX(n->argument_count = 1, *n = saved);
    PREFIX(n->first_argument = 0, *n = saved);
    NLCheckedNodeId a = n->first_argument;
    NLCheckedNodeId b = o.body->nodes[a - 1].next_argument;
    NLCheckedNodeView operand = o.body->nodes[a - 1];
    PREFIX(o.body->nodes[a - 1].kind = NL_CHECKED_UNIT,
           o.body->nodes[a - 1] = operand);
    PREFIX(o.body->nodes[a - 1].type = truth.type,
           o.body->nodes[a - 1] = operand);
    PREFIX(o.body->nodes[a - 1].results[0].value = 0,
           o.body->nodes[a - 1] = operand);
    NLValueId left_id = operand.results[0].value;
    NLSemanticValueView left = world->values[left_id - 1];
    PREFIX(world->values[left_id - 1].scalar_known = false,
           world->values[left_id - 1] = left);
    PREFIX(world->values[left_id - 1].scalar_value++,
           world->values[left_id - 1] = left);
    NLCheckedNodeView right = o.body->nodes[b - 1];
    PREFIX(o.body->nodes[b - 1].next_argument = a,
           o.body->nodes[b - 1] = right);
#undef PREFIX
    NLCopyUnitView bad = o.unit;
    bad.parameters.entry = foreign.unit.parameters.entry;
    CHECK(nl_checked_copy_unit_validate(o.caller, o.call, &bad) != NL_CHECK_OK);
    CHECK(nl_checked_copy_unit_validate(o.caller, o.call, &foreign.unit) !=
          NL_CHECK_OK);
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(!p866_valid(&o));                                                \
        restore;                                                               \
        CHECK(p866_valid(&o));                                                 \
        ++rejected;                                                            \
    } while (0)
    const NLCheckedFragment *owner = p->caller;
    REFUSE(p->caller = foreign.caller, p->caller = owner);
    const NLSource *source = o.body->source;
    REFUSE(o.body->source = foreign.body->source, o.body->source = source);
    NLCheckedNodeId loan = o.unit.parameters.loan;
    NLCheckedNodeView prior_loan = o.caller->nodes[loan - 1];
    REFUSE(o.caller->nodes[loan - 1].loan.scope++,
           o.caller->nodes[loan - 1] = prior_loan);
    REFUSE(o.caller->nodes[loan - 1].field.child_fact++,
           o.caller->nodes[loan - 1] = prior_loan);
    bad = o.unit;
    bad.parameters.field.parent++;
    CHECK(nl_checked_copy_unit_validate(o.caller, o.call, &bad) != NL_CHECK_OK);
    bad = o.unit;
    bad.parameters.parameters[0]++;
    CHECK(nl_checked_copy_unit_validate(o.caller, o.call, &bad) != NL_CHECK_OK);
    NLIfFieldEvent event;
    CHECK(nl_checked_if_return_event(o.body, o.unit.conditional,
                                     o.returned.normal_arm, 0, &event));
    event.result++;
    CHECK(nl_checked_if_return_event_validate(o.body, o.unit.conditional,
                                              o.returned.normal_arm, 0,
                                              &event) != NL_CHECK_OK);
#undef REFUSE
    printf("P866 prefix/authority mutations=%zu plus foreign "
           "views/root/formals/old Copy; recovery PASS\n",
           rejected);
    node_checked_destroy(&o.source);
    node_checked_destroy(&foreign.source);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && run_case(argv[1]) && run_case(argv[2]) ? 0 : 1;
}
