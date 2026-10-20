/* P671 read-only public views of actual registered source after AST teardown.
 */
#include "../support/node_checked.h"
static bool observe(const char *file)
{
    TestNode n = {0};
    CHECK(node_checked_load(file, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *write = NULL, *loan = NULL, *reads[2] = {0};
    size_t old_symbol = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(!write);
            write = v;
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(!loan);
            loan = v;
        }
        if (v->kind == NL_CHECKED_FIELD_READ) {
            CHECK(v->field.index < 2 && !reads[v->field.index]);
            reads[v->field.index] = v;
        }
    }
    CHECK(write && loan && reads[0] && reads[1]);
    const NLCheckedField e = write->field;
    NLSemanticPlaceView root, child, sibling;
    NLSemanticScopeView scope;
    NLSemanticValueView old, next, untouched;
    CHECK(nl_semantic_place_view(c, e.parent, &root) &&
          nl_semantic_place_view(c, e.child, &child) &&
          nl_semantic_place_view(c, reads[1]->field.child, &sibling) &&
          nl_semantic_scope_view(c, loan->loan.scope, &scope) &&
          nl_semantic_value_view(c, e.old_value, &old) &&
          nl_semantic_value_view(c, e.new_value, &next) &&
          nl_semantic_value_view(c, reads[1]->results[0].value, &untouched));
    CHECK(root.incarnation == e.parent_incarnation &&
          child.incarnation == e.child_incarnation &&
          child.parent_aggregate == e.parent && child.parent_field_index == 0 &&
          sibling.parent_aggregate == e.parent &&
          sibling.parent_field_index == 1 && !scope.active && !root.live &&
          reads[0]->field.parent_fact == e.parent_post_fact &&
          reads[1]->field.parent_fact == e.parent_post_fact &&
          reads[0]->field.child_fact == e.child_post_fact && old.scalar_known &&
          next.scalar_known && untouched.scalar_known);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_BINDING &&
            nl_checked_node_view(f, v->initializer) == loan)
            old_symbol = v->symbol;
    }
    CHECK(old_symbol);
    CHECK(loan->field.present && loan->field.nominal == e.nominal &&
          loan->field.base == e.base && loan->field.parent == e.parent &&
          loan->field.child == e.child &&
          loan->field.parent_fact == e.parent_fact &&
          loan->field.child_fact == e.child_fact &&
          loan->field.parent_incarnation == e.parent_incarnation &&
          loan->field.child_incarnation == e.child_incarnation);
    NLSemanticBindingView original;
    NLSemanticTypeView nominal;
    NLAggregateField fields[2];
    CHECK(nl_semantic_binding_view(c, e.base, &original) &&
          original.type == e.nominal && original.place == e.parent &&
          root.type == e.nominal && root.independent_root &&
          root.implicit_local && !child.independent_root &&
          !sibling.independent_root &&
          child.parent_incarnation == e.parent_incarnation &&
          sibling.parent_incarnation == e.parent_incarnation &&
          root.fixed_field_count == 0 && e.parent_fact != e.parent_post_fact &&
          e.child_fact != e.child_post_fact && root.current_fact == 0 &&
          child.current_fact == 0 && sibling.current_fact == 0 && !child.live &&
          !sibling.live && loan->loan.place == e.child &&
          loan->loan.incarnation == e.child_incarnation &&
          loan->loan.access == NL_ACCESS_WRITE && loan->loan.implicit_local &&
          loan->loan.body_nonescape_proved && loan->loan.domain == 0 &&
          nl_semantic_type_view(c, e.nominal, &nominal) &&
          nominal.kind == NL_TYPE_NOMINAL && nominal.field_count == 2 &&
          nominal.is_copy && nominal.is_discardable);
    for (size_t i = 0; i < 2; ++i)
        CHECK(nl_semantic_aggregate_field_view(c, e.nominal, i, &fields[i]) &&
              fields[i].type == nl_semantic_core_type(c, NL_TYPE_U8));
    /* Original construction is issued by the same retained checked body.
     * Compare actual old/sibling values, not host-seeded semantic objects. */
    const NLCheckedNodeView *creation = NULL;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_BINDING && v->symbol == e.base)
            creation = nl_checked_node_view(f, v->initializer);
    }
    NLSemanticValueView aggregate, first, second;
    CHECK(creation && creation->result_count == 1 &&
          creation->type == e.nominal &&
          nl_semantic_value_view(c, creation->results[0].value, &aggregate) &&
          aggregate.field_count == 2 &&
          nl_semantic_value_view(c, aggregate.fields[0], &first) &&
          nl_semantic_value_view(c, aggregate.fields[1], &second) &&
          first.scalar_known && first.scalar_value == old.scalar_value &&
          second.scalar_known && second.scalar_value == untouched.scalar_value);
    printf("{\"labels\":[\"%s\",\"%s\"],\"nominal\":%zu,\"root_symbol\":%zu,"
           "\"root\":%zu,"
           "\"incarnation\":%zu,\"child\":%zu,\"sibling\":%zu,"
           "\"field_index\":%zu,\"parent_facts\":[%zu,%zu],"
           "\"child_facts\":[%zu,%zu],\"sibling_fact\":%zu,"
           "\"sibling_incarnation\":%zu,\"scope\":%zu,\"scope_closed\":true,"
           "\"ref_symbol\":%zu,\"old_symbol\":%zu,"
           "\"values\":[%zu,%zu,%zu]}\n",
           fields[0].name, fields[1].name, e.nominal, e.base, e.parent,
           e.parent_incarnation, e.child, reads[1]->field.child, e.index,
           e.parent_fact, e.parent_post_fact, e.child_fact, e.child_post_fact,
           reads[1]->field.child_fact, sibling.incarnation, loan->loan.scope,
           loan->loan.ref_symbol, old_symbol, old.scalar_value,
           next.scalar_value, untouched.scalar_value);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
