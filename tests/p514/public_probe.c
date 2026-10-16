/* Public accessors only; observations of the actual issued artifact. This is
 * not a new public Pair/IF validator and does not create permission. */
#include "../support/node_checked.h"
static bool observe(const char *file)
{
    TestNode n = {0};
    CHECK(node_checked_load(file, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    NLCheckedNodeId id = 0, write_id = 0;
    const NLCheckedNodeView *loan = NULL, *write = NULL, *read[2] = {0},
                            *eq = NULL;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_IF) {
            CHECK(id == 0);
            id = i;
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(write == NULL);
            write = v;
            write_id = i;
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(loan == NULL);
            loan = v;
        }
        if (v->kind == NL_CHECKED_FIELD_READ) {
            CHECK(v->field.index < 2);
            read[v->field.index] = v;
        }
        if (v->kind == NL_CHECKED_USIZE_EQUALITY)
            eq = v;
    }
    CHECK(id && write && loan && read[0] && read[1] && eq);
    const NLCheckedNodeView *join = nl_checked_node_view(f, id),
                            *cond = nl_checked_node_view(f, join->initializer),
                            *ref =
                                nl_checked_node_view(f, write->first_argument),
                            *input =
                                nl_checked_node_view(f, ref->next_argument);
    CHECK(join->normal_arms == 2 && join->result_count == 1 &&
          join->type == u8);
    NLSemanticValueView result, condition, input_value, old, sibling;
    CHECK(nl_semantic_value_view(c, join->results[0].value, &result) &&
          nl_semantic_value_view(c, cond->results[0].value, &condition) &&
          nl_semantic_value_view(c, input->results[0].value, &input_value) &&
          nl_semantic_value_view(c, write->field.old_value, &old) &&
          nl_semantic_value_view(c, read[1]->results[0].value, &sibling));
    CHECK(!result.scalar_known && !input_value.scalar_known &&
          result.dependencies == NL_DEPENDENCY_FREE &&
          result.value_dependency_count == 0 && result.domain == 0 &&
          result.reference.place == 0 && result.allocation_region == 0 &&
          old.scalar_known && old.scalar_value == 0 && sibling.scalar_known &&
          sibling.scalar_value == 9);
    NLSemanticPlaceView original[3];
    size_t arm_values[2], arm_symbols[2];
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, a);
        CHECK(arm && nl_checked_source(arm) == nl_checked_source(f));
        const NLSemanticContext *w = nl_checked_context(arm);
        NLSemanticSnapshot snapshot;
        CHECK(nl_semantic_snapshot(w, &snapshot) && snapshot.scopes == 0 &&
              snapshot.domains == 0 && snapshot.backing_regions == 0 &&
              snapshot.occurrences == 0);
        for (size_t i = 0; i < 3; ++i) {
            NLSemanticPlaceView place;
            CHECK(nl_semantic_place_view(w, i + 1, &place) && place.live);
            if (a == 0)
                original[i] = place;
            CHECK(place.incarnation == original[i].incarnation &&
                  place.current_fact == original[i].current_fact &&
                  place.current_value == original[i].current_value &&
                  place.governing_domain == 0);
        }
        const NLCheckedNodeView *ar = nl_checked_node_view(
                                    arm, nl_checked_root(arm)),
                                *block =
                                    nl_checked_node_view(arm, ar->initializer),
                                *tail = nl_checked_node_view(arm, block->tail);
        NLSemanticValueView value;
        CHECK(nl_semantic_value_view(w, ar->results[0].value, &value) &&
              value.scalar_known);
        arm_values[a] = value.scalar_value;
        arm_symbols[a] = tail->symbol;
    }
    const NLCheckedField e = write->field;
    CHECK(e.parent == 1 && e.child == 2 &&
          e.parent_incarnation == original[0].incarnation &&
          e.child_incarnation == original[1].incarnation &&
          e.parent_fact == original[0].current_fact &&
          e.child_fact == original[1].current_fact &&
          e.old_value == original[1].current_value &&
          e.new_value == input->results[0].value &&
          e.parent_post_fact != e.parent_fact &&
          e.child_post_fact != e.child_fact &&
          read[0]->field.parent_fact == e.parent_post_fact &&
          read[0]->field.child_fact == e.child_post_fact &&
          read[1]->field.parent_fact == e.parent_post_fact &&
          read[1]->field.child_fact == original[2].current_fact &&
          read[1]->field.old_value == original[2].current_value);
    NLSemanticScopeView scope;
    CHECK(nl_semantic_scope_view(c, loan->loan.scope, &scope) &&
          !scope.active && loan->loan.body_nonescape_proved &&
          loan->loan.normal_result_forwarded);
    NLSymbolId selected = 0, old_symbol = 0, after_symbol = 0,
               sibling_symbol = 0, equal = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind != NL_CHECKED_BINDING)
            continue;
        const NLCheckedNodeView *init = nl_checked_node_view(f, v->initializer);
        if (init == join)
            selected = v->symbol;
        if (init == loan)
            old_symbol = v->symbol;
        if (init == read[0])
            after_symbol = v->symbol;
        if (init == read[1])
            sibling_symbol = v->symbol;
        if (init == eq)
            equal = v->symbol;
    }
    CHECK(selected && old_symbol && after_symbol && sibling_symbol && equal);
    const NLCheckedNodeView *lhs = nl_checked_node_view(f, eq->first_argument),
                            *rhs = nl_checked_node_view(f, lhs->next_argument);
    printf(
        "{\"if\":%zu,\"bool\":%zu,\"condition_symbol\":%zu,\"equal_symbol\":%"
        "zu,"
        "\"operand_symbols\":[%zu,%zu],\"arms\":[%zu,%zu],\"arm_symbols\":[%zu,"
        "%zu],"
        "\"selected_symbol\":%zu,\"input_symbol\":%zu,\"original_symbol\":%zu,"
        "\"old_symbol\":%zu,\"after_symbol\":%zu,\"sibling_symbol\":%zu,"
        "\"ref_symbol\":%zu,\"replace_node\":%zu,\"scope\":%zu,"
        "\"root\":%zu,\"child\":%zu,\"incarnations\":[%zu,%zu,%zu],"
        "\"root_facts\":[%zu,%zu],\"left_facts\":[%zu,%zu],\"right_fact\":%zu,"
        "\"old\":%zu,\"sibling\":%zu,\"joined_unknown\":true,\"scope_ended\":"
        "true}\n",
        id, condition.scalar_value, cond->symbol, equal, lhs->symbol,
        rhs->symbol, arm_values[0], arm_values[1], arm_symbols[0],
        arm_symbols[1], selected, input->symbol, e.base, old_symbol,
        after_symbol, sibling_symbol, loan->loan.ref_symbol, write_id,
        loan->loan.scope, e.parent, e.child, original[0].incarnation,
        original[1].incarnation, original[2].incarnation, e.parent_fact,
        e.parent_post_fact, e.child_fact, e.child_post_fact,
        original[2].current_fact, old.scalar_value, sibling.scalar_value);
    /* Ended final roots are not represented as current live poststate. */
    NLSemanticPlaceView ended;
    CHECK(nl_semantic_place_view(c, e.parent, &ended) && !ended.live &&
          ended.current_fact == 0);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
