/* R725 independently issued borrowed view tests; not a source of authority. */
#include "../support/node_checked.h"
#include "newlang/copy_body.h"
#include "newlang/if_field.h"
#include <stdio.h>
#include <stdlib.h>
static bool verify(const char *path, size_t index)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *caller =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(caller != NULL);
    size_t seen = 0;
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        NLCopyUnitView v;
        if (!nl_checked_copy_unit_view(caller, id, &v))
            continue;
        CHECK(nl_checked_copy_unit_validate(caller, id, &v) == NL_CHECK_OK);
        CHECK(nl_checked_copy_body_parameters_validate(
                  caller, id, &v.parameters) == NL_CHECK_OK);
        NLIfReturnView ir;
        CHECK(nl_checked_if_return_view(v.body, v.conditional, &ir));
        CHECK(nl_checked_if_return_validate(v.body, v.conditional, &ir) ==
              NL_CHECK_OK);
        CHECK(ir.field.index == index);
        NLSemanticPlaceView before, after, sibling_old, sibling_new, child;
        NLSemanticValueView unknown;
        CHECK(
            nl_semantic_place_view(v.parameters.entry, ir.field.root, &before));
        CHECK(nl_semantic_place_view(v.postcall, ir.field.root, &after));
        CHECK(nl_semantic_place_view(v.parameters.entry, ir.field.sibling,
                                     &sibling_old));
        CHECK(
            nl_semantic_place_view(v.postcall, ir.field.sibling, &sibling_new));
        CHECK(nl_semantic_place_view(v.postcall, ir.field.child, &child));
        CHECK(
            nl_semantic_value_view(v.postcall, child.current_value, &unknown));
        CHECK(before.live && after.live &&
              before.incarnation == after.incarnation);
        CHECK(before.current_fact != after.current_fact);
        CHECK(before.fixed_fields[index] == ir.field.child);
        CHECK(sibling_old.current_fact == sibling_new.current_fact);
        CHECK(sibling_old.current_value == sibling_new.current_value);
        CHECK(!unknown.scalar_known);
        size_t rejected = 0;
#define BAD_VIEW(expr)                                                         \
    do {                                                                       \
        NLCopyUnitView bad = v;                                                \
        expr;                                                                  \
        CHECK(nl_checked_copy_unit_validate(caller, id, &bad) != NL_CHECK_OK); \
        CHECK(nl_checked_copy_unit_validate(caller, id, &v) == NL_CHECK_OK);   \
        ++rejected;                                                            \
    } while (0)
        BAD_VIEW(bad.function++);
        BAD_VIEW(bad.body = NULL);
        BAD_VIEW(bad.conditional++);
        BAD_VIEW(bad.normal_end = NULL);
        BAD_VIEW(bad.postcall = NULL);
        BAD_VIEW(bad.parameters.entry = NULL);
        BAD_VIEW(bad.parameters.handoff = NULL);
        BAD_VIEW(bad.parameters.loan++);
        BAD_VIEW(bad.parameters.arguments[0]++);
        BAD_VIEW(bad.parameters.arguments[1]++);
        BAD_VIEW(bad.parameters.parameters[0]++);
        BAD_VIEW(bad.parameters.parameters[1]++);
        BAD_VIEW(bad.parameters.field.parent++);
        BAD_VIEW(bad.parameters.field.child++);
        BAD_VIEW(bad.parameters.field.index = 1 - index);
        BAD_VIEW(bad.parameters.field.parent_incarnation++);
        BAD_VIEW(bad.parameters.field.child_incarnation++);
        BAD_VIEW(bad.parameters.field.parent_fact++);
        BAD_VIEW(bad.parameters.field.child_fact++);
#undef BAD_VIEW
        NLIfReturnView fork = ir;
        fork.normal_arm = 1 - ir.normal_arm;
        CHECK(nl_checked_if_return_validate(v.body, v.conditional, &fork) !=
              NL_CHECK_OK);
        CHECK(nl_checked_if_return_validate(v.body, v.conditional, &ir) ==
              NL_CHECK_OK);
        ++rejected;
        fork = ir;
        fork.return_node++;
        CHECK(nl_checked_if_return_validate(v.body, v.conditional, &fork) !=
              NL_CHECK_OK);
        ++rejected;
        for (size_t stream = 0; stream < 3; ++stream) {
            size_t events = stream == 2 ? ir.replay_events
                                        : ir.field.arms[stream].event_count;
            for (size_t j = 0; j < events; ++j) {
                NLIfFieldEvent valid;
                CHECK(nl_checked_if_return_event(v.body, v.conditional, stream,
                                                 j, &valid));
                CHECK(nl_checked_if_return_event_validate(v.body, v.conditional,
                                                          stream, j, &valid) ==
                      NL_CHECK_OK);
                NLIfFieldEvent bad = valid;
                bad.field.child++;
                CHECK(nl_checked_if_return_event_validate(v.body, v.conditional,
                                                          stream, j,
                                                          &bad) != NL_CHECK_OK);
                ++rejected;
                bad = valid;
                bad.result++;
                CHECK(nl_checked_if_return_event_validate(v.body, v.conditional,
                                                          stream, j,
                                                          &bad) != NL_CHECK_OK);
                ++rejected;
            }
        }
        printf("{\"base\":%zu,\"index\":%zu,\"function\":%zu,"
               "\"formal\":%zu,\"caller_ref\":%zu,\"root\":%zu,\"child\":%zu,"
               "\"normal_arm\":%zu,\"rejected\":%zu}\n",
               ir.field.base, ir.field.index, v.function,
               v.parameters.parameters[0],
               nl_checked_node_view(caller, v.parameters.arguments[0])->symbol,
               ir.field.root, ir.field.child, ir.normal_arm, rejected);
        CHECK(rejected >= 21);
        ++seen;
    }
    CHECK(seen == 1);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    return verify(argv[1], (size_t)strtoul(argv[2], NULL, 10)) ? 0 : 1;
}
