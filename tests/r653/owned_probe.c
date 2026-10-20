/* Track R / Issue 653: independently issued actual-source owned observations.
 * Mutants alter untrusted views only; they do not fabricate valid receipts. */
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* This macro is local to R's probe, not the shared checked loader. */
#undef CHECK
#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "R653 owned check failed at %s:%d: %s\n",          \
                    __FILE__, __LINE__, #condition);                           \
            goto cleanup;                                                      \
        }                                                                      \
    } while (0)

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
    bool success = false;
    char *generated = NULL;
    size_t length = 0;
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
        CHECK(field.present);
        CHECK(field.index == expected_field[i]);
        CHECK(field.parent != 0);
        CHECK(field.child != 0);
        CHECK(field.old_value != 0);
        CHECK(field.new_value != 0);
        CHECK(field.parent_fact != 0);
        CHECK(field.child_fact != 0);
        CHECK(field.parent_post_fact != 0);
        CHECK(field.child_post_fact != 0);
        CHECK(field.parent_fact != field.parent_post_fact);
        CHECK(field.child_fact != field.child_post_fact);
        NLSemanticValueView old;
        CHECK(nl_semantic_value_view(main_world, field.old_value, &old) &&
              old.scalar_known && old.scalar_value == expected_old[i]);
        NLSemanticPlaceView p, child;
        CHECK(nl_semantic_place_view(main_world, field.parent, &p));
        CHECK(nl_semantic_place_view(main_world, field.child, &child));
        CHECK(p.independent_root);
        CHECK(p.implicit_local);
        CHECK(p.fixed_field_count == 2);
        CHECK(p.fixed_fields[field.index] == field.child);
        CHECK(child.parent_aggregate == field.parent);
        CHECK(child.parent_field_index == field.index);
        /* Final closed contexts may reset/nonlive a place. Issued post-facts
         * belong to the transition, not necessarily final current_fact. */
        printf("R653_CLOSED_STATE write=%zu parent_live=%d child_live=%d "
               "parent_final=%zu parent_issued_post=%zu "
               "child_final=%zu child_issued_post=%zu\\n",
               i, (int)p.live, (int)child.live, (size_t)p.current_fact,
               (size_t)field.parent_post_fact, (size_t)child.current_fact,
               (size_t)field.child_post_fact);
        for (size_t a = 0; a < 2; ++a) {
            NLSemanticPlaceView before, before_child;
            const NLSemanticContext *w = nl_checked_context(arms[a]);
            CHECK(nl_semantic_place_view(w, field.parent, &before));
            CHECK(nl_semantic_place_view(w, field.child, &before_child));
            CHECK(before.live);
            CHECK(before_child.live);
            CHECK(before.incarnation == field.parent_incarnation);
            CHECK(before_child.incarnation == field.child_incarnation);
            CHECK(before.current_fact == field.parent_fact);
            CHECK(before_child.current_fact == field.child_fact);
        }
        NLSemanticScopeView loan_scope;
        CHECK(loans[i]->loan.scope != 0);
        CHECK(nl_semantic_scope_view(main_world, loans[i]->loan.scope,
                                     &loan_scope));
        CHECK(!loan_scope.active);
        CHECK(loans[i]->loan.body_nonescape_proved);
        CHECK(loans[i]->loan.normal_result_forwarded);
    }
    CHECK(writes[0]->field.parent != writes[1]->field.parent &&
          writes[0]->field.child != writes[1]->field.child);
    CHECK(nl_checked_c_node(n.entry, &generated, &length) == NL_NODE_C_OK &&
          generated != NULL && length > 0);
    free(generated);
    generated = NULL;
    NLCheckedNodeView *mut = (NLCheckedNodeView *)(uintptr_t)writes[0];
    const NLCheckedNodeView clean = *mut;
    size_t rejected = 0;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        const bool refused = reject_mutation(n.entry);                         \
        restore;                                                               \
        CHECK(refused);                                                        \
        ++rejected;                                                            \
    } while (0)
    REFUSE(mut->field.parent = 0, *mut = clean);
    REFUSE(mut->field.parent = SIZE_MAX, *mut = clean);
    REFUSE(mut->field.child = 0, *mut = clean);
    REFUSE(mut->field.child = SIZE_MAX, *mut = clean);
    REFUSE(mut->field.child = writes[1]->field.child, *mut = clean);
    REFUSE(mut->field.index = 1, *mut = clean);
    REFUSE(mut->field.old_value = 0, *mut = clean);
    REFUSE(mut->field.old_value = SIZE_MAX, *mut = clean);
    REFUSE(mut->field.parent_fact = writes[1]->field.parent_fact, *mut = clean);
    REFUSE(mut->field.child_fact = 0, *mut = clean);
    REFUSE(mut->field.parent_post_fact = 0, *mut = clean);
    REFUSE(mut->field.parent_post_fact = SIZE_MAX, *mut = clean);
    REFUSE(mut->field.new_value = 0, *mut = clean);
    REFUSE(mut->field.new_value = SIZE_MAX, *mut = clean);
    NLCheckedNodeView *if_mut = (NLCheckedNodeView *)(uintptr_t)join;
    const NLCheckedNodeView good_if = *if_mut;
    REFUSE(if_mut->normal_arms = 0, *if_mut = good_if);
    generated = NULL;
    length = 0;
    CHECK(nl_checked_c_node(n.entry, &generated, &length) == NL_NODE_C_OK &&
          generated != NULL && length > 0);
    /* The final checked C artifact is freed at cleanup on every exit. */
#undef REFUSE
    printf("R653_OWNED_OK: genuine=2 originals, two arm worlds, "
           "two closed writes; mutants_rejected=%zu\n",
           rejected);
    success = true;
cleanup:
    free(generated);
    node_checked_destroy(&n);
    return success;
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
