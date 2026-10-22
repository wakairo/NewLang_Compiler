#include "../support/node_checked.h"
#include "newlang/if_field.h"
#include <stdio.h>
#include <string.h>

/* Independent public, borrowed-view-only R probe: no private issuer writes. */
static bool rejected(const NLCheckedFragment *f, size_t id, NLIfReturnView x)
{
    return nl_checked_if_return_validate(f, id, &x) != NL_CHECK_OK;
}
static bool inspect(const char *path, bool family_a)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f);
    const size_t expect_normal = family_a ? 1U : 0U;
    const size_t expect_index = family_a ? 1U : 0U;
    const size_t expect_old = family_a ? 142U : 201U;
    const size_t expect_incoming = family_a ? 58U : 166U;
    size_t found = 0;
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        NLIfReturnView v;
        if (!nl_checked_if_return_view(f, id, &v))
            continue;
        CHECK(++found == 1 && v.normal_arm == expect_normal &&
              v.field.index == expect_index && v.field.root &&
              v.field.child && v.field.sibling && v.replay_root &&
              v.return_node && v.return_live && v.return_ended &&
              v.field.entry && v.field.post &&
              v.return_live != v.field.post &&
              v.field.arms[0].world != v.field.arms[1].world &&
              v.field.arms[0].world != v.field.post &&
              v.field.arms[1].world != v.field.post &&
              v.replay_changes == 1 && v.replay_events >= 1 &&
              v.returned.type ==
                  nl_semantic_core_type(v.return_live, NL_TYPE_UNIT) &&
              v.returned.value == 0);
        CHECK(nl_checked_if_return_validate(f, id, &v) == NL_CHECK_OK &&
              nl_checked_if_return_validate(f, id, NULL) == NL_CHECK_OK);
        const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
        const NLCheckedFragment *terminal =
            nl_checked_if_arm(f, id, 1U - expect_normal);
        const NLCheckedNodeView *return_node =
            nl_checked_node_view(terminal, v.return_node);
        CHECK(choice && choice->kind == NL_CHECKED_IF &&
              choice->normal_arms == 1 && !choice->terminates &&
              return_node && return_node->kind == NL_CHECKED_RETURN &&
              return_node->terminates && return_node->result_count == 0);
        NLSemanticPlaceView entry_root, post_root, entry_sib, post_sib, child;
        CHECK(nl_semantic_place_view(v.field.entry, v.field.root, &entry_root) &&
              nl_semantic_place_view(v.field.post, v.field.root, &post_root) &&
              nl_semantic_place_view(v.field.entry, v.field.sibling, &entry_sib) &&
              nl_semantic_place_view(v.field.post, v.field.sibling, &post_sib) &&
              nl_semantic_place_view(v.field.post, v.field.child, &child) &&
              entry_root.incarnation == post_root.incarnation &&
              entry_root.incarnation == v.field.incarnation &&
              entry_root.current_fact != post_root.current_fact &&
              entry_sib.current_fact == post_sib.current_fact &&
              entry_sib.current_value == post_sib.current_value &&
              post_root.fixed_fields[expect_index] == v.field.child &&
              post_root.fixed_fields[1U - expect_index] == v.field.sibling &&
              child.incarnation == v.field.child_incarnation);
        NLSemanticValueView post_value;
        CHECK(nl_semantic_value_view(v.field.post, child.current_value,
                                     &post_value) &&
              post_value.type ==
                  nl_semantic_core_type(v.field.post, NL_TYPE_U8) &&
              post_value.dependencies == NL_DEPENDENCY_FREE &&
              !post_value.reference.place && !post_value.allocation_region &&
              !post_value.domain);
        CHECK(v.field.arms[expect_normal].change_count == 1 &&
              v.field.arms[1U - expect_normal].change_count ==
                  (family_a ? 0U : 1U));
        for (size_t stream = 0; stream < 3; ++stream) {
            size_t count = stream == 2 ? v.replay_events
                                       : v.field.arms[stream].event_count;
            size_t changes = 0;
            for (size_t i = 0; i < count; ++i) {
                NLIfFieldEvent e;
                CHECK(nl_checked_if_return_event(f, id, stream, i, &e));
                CHECK(nl_checked_if_return_event_validate(f, id, stream, i, &e) ==
                      NL_CHECK_OK);
                if (e.change) {
                    ++changes;
                    CHECK(e.field.parent == v.field.root &&
                          e.field.child == v.field.child &&
                          e.field.index == expect_index &&
                          e.field.parent_incarnation == v.field.incarnation &&
                          e.field.child_incarnation ==
                              v.field.child_incarnation);
                    NLSemanticValueView previous;
                    CHECK(nl_semantic_value_view(e.before, e.field.old_value,
                                                 &previous) &&
                          previous.scalar_known &&
                          previous.scalar_value == expect_old);
                    NLIfFieldEvent changed = e;
                    changed.node += 100000U;
                    CHECK(nl_checked_if_return_event_validate(
                              f, id, stream, i, &changed) != NL_CHECK_OK);
                    changed = e;
                    changed.field.parent += 100000U;
                    CHECK(nl_checked_if_return_event_validate(
                              f, id, stream, i, &changed) != NL_CHECK_OK);
                    changed = e;
                    changed.field.index = 1U - e.field.index;
                    CHECK(nl_checked_if_return_event_validate(
                              f, id, stream, i, &changed) != NL_CHECK_OK);
                    changed = e;
                    changed.result += 100000U;
                    CHECK(nl_checked_if_return_event_validate(
                              f, id, stream, i, &changed) != NL_CHECK_OK);
                    changed = e;
                    changed.incoming += 100000U;
                    CHECK(nl_checked_if_return_event_validate(
                              f, id, stream, i, &changed) != NL_CHECK_OK);
                }
            }
            CHECK(changes == (stream == 2 ? 1U
                              : stream == expect_normal ? 1U
                              : family_a ? 0U : 1U));
            NLIfFieldEvent extra;
            CHECK(!nl_checked_if_return_event(f, id, stream, count, &extra));
        }
        NLSemanticSnapshot scopes;
        CHECK(nl_semantic_snapshot(v.return_live, &scopes));
        for (size_t k = 1; k <= scopes.scopes; ++k) {
            NLSemanticScopeView s;
            CHECK(nl_semantic_scope_view(v.return_live, k, &s) && !s.active);
        }
        NLIfReturnView bad = v;
        bad.normal_arm = 1U - v.normal_arm;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.field.root += 100000U;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.field.child += 100000U;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.field.index = 1U - v.field.index;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.field.sibling += 100000U;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.return_node += 100000U;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.returned.type += 100000U;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.return_live = v.field.post;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.return_ended = v.field.entry;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.replay_root += 100000U;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.replay_events += 1U;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.field.arms[expect_normal].world = v.field.entry;
        CHECK(rejected(f, id, bad));
        bad = v;
        bad.field.result.value += 100000U;
        CHECK(rejected(f, id, bad));
        CHECK(nl_checked_if_return_validate(f, id, &v) == NL_CHECK_OK);
        /* public borrowed historical view is NOT a transferable live authority */
        printf("{\"base\":%zu,\"if\":%zu,\"normal\":%zu,\"index\":%zu,"
               "\"old\":%zu,\"incoming\":%zu,\"tamper\":13}\n",
               v.field.base, id, v.normal_arm, v.field.index, expect_old,
               expect_incoming);
    }
    CHECK(found == 1);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && inspect(argv[1], strcmp(argv[2], "A") == 0) ? 0 : 1;
}
