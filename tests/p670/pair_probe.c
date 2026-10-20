/* Read-only public views of the unchanged ordinary Pair control. */
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
    printf("{\"nominal\":%zu,\"root_symbol\":%zu,\"root\":%zu,"
           "\"incarnation\":%zu,\"child\":%zu,\"sibling\":%zu,"
           "\"field_index\":%zu,\"parent_facts\":[%zu,%zu],"
           "\"child_facts\":[%zu,%zu],\"scope_closed\":true,"
           "\"ref_symbol\":%zu,\"old_symbol\":%zu,"
           "\"values\":[%zu,%zu,%zu]}\n",
           e.nominal, e.base, e.parent, e.parent_incarnation, e.child,
           reads[1]->field.child, e.index, e.parent_fact, e.parent_post_fact,
           e.child_fact, e.child_post_fact, loan->loan.ref_symbol, old_symbol,
           old.scalar_value, next.scalar_value, untouched.scalar_value);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
