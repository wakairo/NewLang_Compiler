/* Track R / Issue 653: independently issued actual-source owned observations.
 * Mutants alter untrusted views only; they do not fabricate valid receipts. */
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"

#include <stdint.h>
#include <stdlib.h>

static bool reject_mutation(const NLCheckedFragment *entry)
{
    char *c = NULL;
    size_t length = 0;
    NLNodeCStatus status = nl_checked_c_node(entry, &c, &length);
    free(c);
    return status != NL_NODE_C_OK;
}

static bool run_probe(const char *source, const size_t expected_old[2],
                      const size_t expected_field[2],
                      const size_t expected_arm[2])
{
    TestNode n = {0};
    CHECK(node_checked_load(source, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSemanticContext *main_world = nl_checked_context(f);
    NLCheckedNodeId if_id = 0;
    const NLCheckedNodeView *writes[2] = {0};
    const NLCheckedNodeView *loans[2] = {0};
    size_t count_writes = 0, count_loans = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_IF) {
            CHECK(if_id == 0);
            if_id = i;
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(count_writes < 2);
            writes[count_writes++] = v;
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(count_loans < 2);
            loans[count_loans++] = v;
        }
    }
    CHECK(if_id && count_writes == 2 && count_loans == 2);
    const NLCheckedNodeView *join = nl_checked_node_view(f, if_id);
    CHECK(join->normal_arms == 2 && join->result_count == 1);
    const NLCheckedFragment *arms[2] = {nl_checked_if_arm(f, if_id, 0),
                                        nl_checked_if_arm(f, if_id, 1)};
    CHECK(arms[0] && arms[1] &&
          nl_checked_context(arms[0]) != nl_checked_context(arms[1]));
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = arms[a];
        const NLCheckedNodeView *last =
            nl_checked_node_view(arm, nl_checked_root(arm));
        NLSemanticValueView val;
        CHECK(last && last->result_count == 1 &&
              nl_semantic_value_view(nl_checked_context(arm),
                                     last->results[0].value, &val) &&
              val.scalar_known && val.scalar_value == expected_arm[a]);
    }
    for (size_t i = 0; i < 2; ++i) {
        NLCheckedField field = writes[i]->field;
        CHECK(field.present && field.index == expected_field[i] &&
              field.parent && field.child && field.old_value &&
              field.new_value && field.parent_fact && field.child_fact &&
              field.parent_post_fact && field.child_post_fact);
        NLSemanticValueView old;
        CHECK(nl_semantic_value_view(main_world, field.old_value, &old) &&
              old.scalar_known && old.scalar_value == expected_old[i]);
        NLSemanticPlaceView p, child;
        CHECK(nl_semantic_place_view(main_world, field.parent, &p) &&
              nl_semantic_place_view(main_world, field.child, &child) &&
              p.independent_root && p.implicit_local &&
              p.fixed_field_count == 2 &&
              p.fixed_fields[field.index] == field.child &&
              child.parent_aggregate == field.parent &&
              child.parent_field_index == field.index &&
              field.parent_post_fact == p.current_fact &&
              field.child_post_fact == child.current_fact);
        for (size_t a = 0; a < 2; ++a) {
            NLSemanticPlaceView before, before_child;
            const NLSemanticContext *w = nl_checked_context(arms[a]);
            CHECK(nl_semantic_place_view(w, field.parent, &before) &&
                  nl_semantic_place_view(w, field.child, &before_child) &&
                  before.live && before_child.live &&
                  before.incarnation == field.parent_incarnation &&
                  before_child.incarnation == field.child_incarnation &&
                  before.current_fact == field.parent_fact &&
                  before_child.current_fact == field.child_fact);
        }
        NLSemanticScopeView loan_scope;
        CHECK(loans[i]->loan.scope &&
              nl_semantic_scope_view(main_world, loans[i]->loan.scope,
                                     &loan_scope) &&
              !loan_scope.active && loans[i]->loan.body_nonescape_proved &&
              loans[i]->loan.normal_result_forwarded);
    }
    CHECK(writes[0]->field.parent != writes[1]->field.parent &&
          writes[0]->field.child != writes[1]->field.child);
    char *generated = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(n.entry, &generated, &length) == NL_NODE_C_OK &&
          generated != NULL && length > 0);
    free(generated);
    NLCheckedNodeView *mut = (NLCheckedNodeView *)(uintptr_t)writes[0];
    const NLCheckedNodeView clean = *mut;
    mut->field.parent = 0;
    CHECK(reject_mutation(n.entry));
    *mut = clean;
    mut->field.child = writes[1]->field.child;
    CHECK(reject_mutation(n.entry));
    *mut = clean;
    mut->field.old_value = 0;
    CHECK(reject_mutation(n.entry));
    *mut = clean;
    mut->field.parent_fact = writes[1]->field.parent_fact;
    CHECK(reject_mutation(n.entry));
    *mut = clean;
    mut->field.parent_post_fact = 0;
    CHECK(reject_mutation(n.entry));
    *mut = clean;
    NLCheckedNodeView *if_mut = (NLCheckedNodeView *)(uintptr_t)join;
    const NLCheckedNodeView good_if = *if_mut;
    if_mut->normal_arms = 0;
    CHECK(reject_mutation(n.entry));
    *if_mut = good_if;
    printf("R653_OWNED_OK: genuine=2 originals, two arm worlds, "
           "two closed writes; invalid parent/child/old/facts/if rejected\n");
    node_checked_destroy(&n);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 8)
        return 2;
    size_t old[2] = {(size_t)strtoul(argv[2], NULL, 10),
                     (size_t)strtoul(argv[3], NULL, 10)};
    size_t field[2] = {(size_t)strtoul(argv[4], NULL, 10),
                       (size_t)strtoul(argv[5], NULL, 10)};
    size_t arms[2] = {(size_t)strtoul(argv[6], NULL, 10),
                      (size_t)strtoul(argv[7], NULL, 10)};
    return run_probe(argv[1], old, field, arms) ? 0 : 1;
}
