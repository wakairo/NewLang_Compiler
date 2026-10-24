#include "../support/node_checked.h"
#include "newlang/copy_body.h"
#include "newlang/if_field.h"

static bool inspect(const char *path, const char *foreign_path, size_t arm,
                    size_t before_value, size_t after_value)
{
    TestNode n = {0}, foreign = {0};
    CHECK(node_checked_load(path, &n));
    CHECK(node_checked_load(foreign_path, &foreign));
    const NLCheckedFragment *caller =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    const NLCheckedFragment *foreign_caller =
        nl_checked_call_body(foreign.entry, nl_checked_root(foreign.entry));
    NLCopyUnitView foreign_view;
    bool found_foreign = false;
    for (size_t id = 1; id <= nl_checked_node_count(foreign_caller); ++id)
        if (nl_checked_copy_unit_view(foreign_caller, id, &foreign_view))
            found_foreign = true;
    CHECK(found_foreign);
    size_t found = 0;
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        NLCopyUnitView u;
        if (!nl_checked_copy_unit_view(caller, id, &u))
            continue;
        ++found;
        NLIfReturnView branch_view;
        CHECK(nl_checked_if_return_view(u.body, u.conditional, &branch_view));
        CHECK(nl_checked_copy_unit_validate(caller, id, &u) == NL_CHECK_OK);
        CHECK(nl_checked_copy_body_parameters_validate(
                  caller, id, &u.parameters) == NL_CHECK_OK);
        CHECK(nl_checked_if_return_validate(u.body, u.conditional,
                                            &branch_view) == NL_CHECK_OK);
        CHECK(branch_view.normal_arm == arm && branch_view.return_node &&
              branch_view.replay_events == 1 &&
              branch_view.field.arms[arm].event_count == 1 &&
              branch_view.field.arms[1 - arm].event_count == 0 &&
              u.parameters.loan && u.parameters.arguments[0] &&
              u.parameters.arguments[1] && u.parameters.parameters[0] &&
              u.parameters.parameters[1]);
        const NLCheckedNodeView *arg =
            nl_checked_node_view(caller, u.parameters.arguments[0]);
        CHECK(arg && arg->symbol);
        NLIfFieldEvent event;
        CHECK(
            nl_checked_if_return_event(u.body, u.conditional, arm, 0, &event));
        CHECK(event.change &&
              nl_checked_if_return_event_validate(u.body, u.conditional, arm, 0,
                                                  &event) == NL_CHECK_OK);
        NLSemanticValueView old, incoming, unknown;
        CHECK(nl_semantic_value_view(event.before, event.result, &old));
        CHECK(nl_semantic_value_view(event.before, event.incoming, &incoming));
        CHECK(old.scalar_known && incoming.scalar_known &&
              old.scalar_value == before_value &&
              incoming.scalar_value == after_value);
        NLSemanticPlaceView start_root, end_root, child, before_sibling,
            after_sibling, returned_root;
        CHECK(nl_semantic_place_view(u.parameters.entry, branch_view.field.root,
                                     &start_root));
        CHECK(nl_semantic_place_view(u.postcall, branch_view.field.root,
                                     &end_root));
        CHECK(nl_semantic_place_view(u.postcall, branch_view.field.child,
                                     &child));
        CHECK(nl_semantic_place_view(
            u.parameters.entry, branch_view.field.sibling, &before_sibling));
        CHECK(nl_semantic_place_view(u.postcall, branch_view.field.sibling,
                                     &after_sibling));
        CHECK(nl_semantic_place_view(branch_view.return_ended,
                                     branch_view.field.root, &returned_root));
        CHECK(
            nl_semantic_value_view(u.postcall, child.current_value, &unknown));
        CHECK(start_root.live && end_root.live && returned_root.live &&
              start_root.incarnation == end_root.incarnation &&
              start_root.incarnation == returned_root.incarnation &&
              start_root.current_fact != end_root.current_fact &&
              !unknown.scalar_known &&
              before_sibling.current_value == after_sibling.current_value &&
              before_sibling.current_fact == after_sibling.current_fact &&
              child.parent_aggregate == branch_view.field.root &&
              start_root.fixed_fields[branch_view.field.index] ==
                  branch_view.field.child);
        NLCopyUnitView changed = u;
        changed.parameters.entry = foreign_view.parameters.entry;
        CHECK(nl_checked_copy_unit_validate(caller, id, &changed) !=
              NL_CHECK_OK);
        changed = u;
        changed.parameters.loan = 0;
        CHECK(nl_checked_copy_unit_validate(caller, id, &changed) !=
              NL_CHECK_OK);
        changed = u;
        changed.parameters.arguments[1] = u.parameters.arguments[0];
        CHECK(nl_checked_copy_unit_validate(caller, id, &changed) !=
              NL_CHECK_OK);
        changed = u;
        changed.parameters.field.child++;
        CHECK(nl_checked_copy_unit_validate(caller, id, &changed) !=
              NL_CHECK_OK);
        NLIfReturnView changed_branch = branch_view;
        changed_branch.normal_arm = 1 - arm;
        CHECK(nl_checked_if_return_validate(u.body, u.conditional,
                                            &changed_branch) != NL_CHECK_OK);
        NLIfFieldEvent changed_event = event;
        changed_event.change = false;
        CHECK(nl_checked_if_return_event_validate(u.body, u.conditional, arm, 0,
                                                  &changed_event) !=
              NL_CHECK_OK);
        size_t receiver = 0;
        for (size_t k = 1; k <= nl_checked_node_count(caller); ++k) {
            const NLCheckedNodeView *node = nl_checked_node_view(caller, k);
            if (node->kind == NL_CHECKED_REPLACE &&
                node->field.base != branch_view.field.base) {
                CHECK(!receiver);
                receiver = node->field.base;
            }
        }
        CHECK(receiver && receiver != branch_view.field.base);
        printf("{\"normal\":%zu,\"base\":%zu,\"receiver\":%zu,"
               "\"index\":%zu,\"function\":%zu,\"ref\":%zu,"
               "\"old\":%zu,\"incoming\":%zu,\"unknown\":true}\n",
               arm, branch_view.field.base, receiver, branch_view.field.index,
               u.function, arg->symbol, (size_t)old.scalar_value,
               (size_t)incoming.scalar_value);
    }
    CHECK(found == 1);
    node_checked_destroy(&n);
    node_checked_destroy(&foreign);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 1;
    if (!inspect(argv[1], argv[2], 0, 31, 72))
        return 1;
    if (!inspect(argv[2], argv[1], 1, 57, 94))
        return 1;
    return 0;
}
