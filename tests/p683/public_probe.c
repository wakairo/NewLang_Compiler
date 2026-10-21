/* Read-only actual-source owned observations; no semantic seeds or mutation. */
#include "node_checked.h"
#include <stdlib.h>
static bool scalar(const NLSemanticContext *c, NLValueId id, size_t expected)
{
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(c, id, &v) &&
          v.type == nl_semantic_core_type(c, NL_TYPE_U8) && v.scalar_known &&
          v.scalar_value == expected && v.dependencies == NL_DEPENDENCY_FREE);
    return true;
}
static bool observe(const char *file, size_t expected)
{
    TestNode n = {0};
    CHECK(node_checked_load(file, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *writes[2] = {0}, *loans[2] = {0}, *reads[3] = {0};
    size_t write_ids[2] = {0}, loan_ids[2] = {0}, read_ids[3] = {0};
    size_t wc = 0, lc = 0, rc = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(wc < 2);
            write_ids[wc] = i;
            writes[wc++] = v;
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(lc < 2);
            loan_ids[lc] = i;
            loans[lc++] = v;
        }
        if (v->kind == NL_CHECKED_FIELD_READ) {
            CHECK(rc < 3);
            read_ids[rc] = i;
            reads[rc++] = v;
        }
    }
    CHECK(wc == 2 && lc == 2 && rc == 3 && write_ids[0] < write_ids[1]);
    const NLCheckedField a = writes[0]->field, b = writes[1]->field;
    CHECK(a.present && b.present && a.dependency_compatible &&
          b.dependency_compatible && a.index == 1 && b.index == 0 &&
          a.nominal == b.nominal && a.base == b.base && a.parent == b.parent &&
          a.parent_incarnation == b.parent_incarnation && a.child != b.child &&
          a.parent_post_fact == b.parent_fact &&
          a.parent_fact != a.parent_post_fact &&
          b.parent_fact != b.parent_post_fact &&
          a.child_fact != a.child_post_fact &&
          b.child_fact != b.child_post_fact && a.access == NL_ACCESS_WRITE &&
          b.access == NL_ACCESS_WRITE);
    NLSemanticPlaceView parent, children[2];
    NLSemanticTypeView type;
    NLAggregateField fields[2];
    CHECK(nl_semantic_place_view(c, a.parent, &parent) &&
          parent.type == a.nominal && parent.independent_root &&
          parent.implicit_local && parent.incarnation == a.parent_incarnation &&
          !parent.live && parent.current_fact == 0 &&
          parent.fixed_field_count == 0 &&
          nl_semantic_type_view(c, a.nominal, &type) &&
          type.kind == NL_TYPE_NOMINAL && type.field_count == 2 &&
          type.is_copy && type.is_discardable);
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedField e = i ? a : b;
        CHECK(nl_semantic_place_view(c, e.child, &children[i]) &&
              !children[i].independent_root &&
              children[i].parent_aggregate == e.parent &&
              children[i].parent_incarnation == e.parent_incarnation &&
              children[i].parent_field_index == i &&
              children[i].incarnation == e.child_incarnation &&
              !children[i].live && children[i].current_fact == 0 &&
              nl_semantic_aggregate_field_view(c, a.nominal, i, &fields[i]) &&
              fields[i].type == nl_semantic_core_type(c, NL_TYPE_U8));
    }
    CHECK(strcmp(fields[0].name, "port") == 0 &&
          strcmp(fields[1].name, "mode") == 0);
    CHECK(reads[0]->field.index == 0 && reads[0]->field.child == b.child &&
          reads[0]->field.child_fact == b.child_fact &&
          reads[0]->field.parent_fact == a.parent_fact &&
          reads[1]->field.index == 0 && reads[1]->field.child == b.child &&
          reads[1]->field.child_fact == b.child_post_fact &&
          reads[2]->field.index == 1 && reads[2]->field.child == a.child &&
          reads[2]->field.child_fact == a.child_post_fact &&
          reads[1]->field.parent_fact == b.parent_post_fact &&
          reads[2]->field.parent_fact == b.parent_post_fact);
    NLSymbolId old_symbols[2] = {0}, read_symbols[3] = {0};
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind != NL_CHECKED_BINDING)
            continue;
        for (size_t k = 0; k < 2; ++k)
            if (v->initializer == loan_ids[k])
                old_symbols[k] = v->symbol;
        for (size_t k = 0; k < 3; ++k)
            if (v->initializer == read_ids[k])
                read_symbols[k] = v->symbol;
    }
    CHECK(old_symbols[0] && old_symbols[1] && read_symbols[0] &&
          read_symbols[1] && read_symbols[2]);
    const size_t old_expected[2] = {expected, 7},
                 new_expected[2] = {7, expected};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedField e = writes[i]->field;
        const NLCheckedNodeView *ref =
            nl_checked_node_view(f, writes[i]->first_argument);
        const NLCheckedNodeView *input =
            ref ? nl_checked_node_view(f, ref->next_argument) : NULL;
        NLSemanticScopeView scope;
        NLSemanticBindingView result;
        CHECK(ref && input && ref->kind == NL_CHECKED_IDENTIFIER &&
              ref->symbol == loans[i]->loan.ref_symbol &&
              input->kind == NL_CHECKED_IDENTIFIER &&
              input->value_use == NL_VALUE_COPIED &&
              input->symbol == (i ? old_symbols[0] : read_symbols[0]) &&
              input->results[0].value == e.new_value &&
              loans[i]->loan.place == e.child &&
              loans[i]->loan.incarnation == e.child_incarnation &&
              loans[i]->loan.access == NL_ACCESS_WRITE &&
              loans[i]->loan.implicit_local &&
              loans[i]->loan.body_nonescape_proved &&
              loans[i]->loan.domain == 0 &&
              loans[i]->results[0].value == e.old_value &&
              nl_semantic_scope_view(c, loans[i]->loan.scope, &scope) &&
              !scope.active && scope.parent == 0 &&
              nl_semantic_binding_view(c, old_symbols[i], &result) &&
              result.value == e.old_value &&
              scalar(c, e.old_value, old_expected[i]) &&
              scalar(c, e.new_value, new_expected[i]));
    }
    CHECK(loans[0]->loan.scope != loans[1]->loan.scope &&
          scalar(c, reads[0]->results[0].value, 7) &&
          scalar(c, reads[1]->results[0].value, expected) &&
          scalar(c, reads[2]->results[0].value, 7));
    const NLCheckedNodeView *call =
        nl_checked_node_view(n.entry, nl_checked_root(n.entry));
    const NLCheckedNodeView *body = nl_checked_node_view(f, nl_checked_root(f));
    const NLCheckedNodeView *tail = nl_checked_node_view(f, body->tail);
    CHECK(call && body && tail && call->result_count == 1 &&
          body->result_count == 1 && tail->kind == NL_CHECKED_IDENTIFIER &&
          tail->symbol == read_symbols[1] &&
          call->results[0].value == body->results[0].value &&
          body->results[0].value == tail->results[0].value &&
          tail->value_use == NL_VALUE_COPIED &&
          scalar(c, call->results[0].value, expected));
    printf(
        "{\"root\":%zu,\"children\":[%zu,%zu],\"scopes\":[%zu,%zu],"
        "\"writes\":[%zu,%zu],\"read_symbols\":[%zu,%zu,%zu],"
        "\"call_result\":%zu,\"tail_result\":%zu,\"old_copy_forwarded\":true,"
        "\"value\":%zu,\"sibling\":7,\"closed_and_ended\":true}\n",
        a.parent, b.child, a.child, loans[0]->loan.scope, loans[1]->loan.scope,
        write_ids[0], write_ids[1], read_symbols[0], read_symbols[1],
        read_symbols[2], call->results[0].value, tail->results[0].value,
        expected);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && observe(argv[1], (size_t)strtoul(argv[2], NULL, 10))
               ? 0
               : 1;
}
