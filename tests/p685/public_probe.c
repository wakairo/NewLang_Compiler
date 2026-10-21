/* Observes genuine registered source only, after input AST destruction. */
#include "node_checked.h"
#include <stdlib.h>
static bool scalar(const NLSemanticContext *c, NLValueId id, size_t wanted)
{
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(c, id, &v) &&
          v.type == nl_semantic_core_type(c, NL_TYPE_U8) && v.scalar_known &&
          v.scalar_value == wanted && v.dependencies == NL_DEPENDENCY_FREE &&
          !v.value_dependency_count);
    return true;
}
static bool observe(const char *path, size_t expected, size_t old,
                    size_t sibling, size_t field)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *call = NULL, *write = NULL, *loan = NULL,
                            *reads[2] = {0};
    size_t call_id = 0, write_id = 0, loan_id = 0, read_id[2] = {0};
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            CHECK(!call);
            call = v;
            call_id = i;
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(!write);
            write = v;
            write_id = i;
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(!loan);
            loan = v;
            loan_id = i;
        }
        if (v->kind == NL_CHECKED_FIELD_READ) {
            size_t k = v->field.index == field ? 0 : 1;
            CHECK(!reads[k]);
            reads[k] = v;
            read_id[k] = i;
        }
    }
    CHECK(call && write && loan && reads[0] && reads[1] && call_id < write_id &&
          call->body_backed && call->argument_count == 0 &&
          call->first_argument == 0 && call->result_count == 1);
    const NLCheckedFragment *body = nl_checked_call_body(f, call_id);
    CHECK(body && nl_checked_context(body) == c);
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    CHECK(root);
    NLValueId result = 0;
    size_t exits = 0;
    if (root->terminates) {
        for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
            const NLCheckedNodeView *v = nl_checked_node_view(body, i);
            if (v->kind == NL_CHECKED_RETURN) {
                result = v->returned.value;
                ++exits;
            }
        }
        CHECK(exits == 1);
    } else {
        const NLCheckedNodeView *tail = nl_checked_node_view(body, root->tail);
        CHECK(tail && root->result_count == 1 && tail->result_count == 1 &&
              root->results[0].value == tail->results[0].value);
        result = tail->results[0].value;
    }
    CHECK(result == call->results[0].value && scalar(c, result, expected));
    NLSymbolId incoming = 0, old_binding = 0, read_symbols[2] = {0};
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind != NL_CHECKED_BINDING)
            continue;
        if (v->initializer == call_id)
            incoming = v->symbol;
        if (v->initializer == loan_id)
            old_binding = v->symbol;
        for (size_t k = 0; k < 2; ++k)
            if (v->initializer == read_id[k])
                read_symbols[k] = v->symbol;
    }
    const NLCheckedNodeView *ref =
        nl_checked_node_view(f, write->first_argument);
    const NLCheckedNodeView *input =
        ref ? nl_checked_node_view(f, ref->next_argument) : NULL;
    NLSemanticBindingView received;
    NLSemanticScopeView scope;
    NLSemanticPlaceView parent, child, other;
    const NLCheckedField e = write->field;
    CHECK(incoming && old_binding && read_symbols[0] && read_symbols[1] &&
          ref && input && input->kind == NL_CHECKED_IDENTIFIER &&
          input->symbol == incoming && input->value_use == NL_VALUE_COPIED &&
          nl_semantic_binding_view(c, incoming, &received) &&
          received.value == result && e.present && e.dependency_compatible &&
          e.index == field && e.access == NL_ACCESS_WRITE &&
          e.new_value == input->results[0].value &&
          e.old_value == write->results[0].value &&
          scalar(c, e.old_value, old) && scalar(c, e.new_value, expected) &&
          loan->loan.source == e.base && loan->loan.place == e.child &&
          loan->loan.incarnation == e.child_incarnation &&
          loan->loan.implicit_local && loan->loan.body_nonescape_proved &&
          loan->loan.normal_result_forwarded &&
          loan->results[0].value == e.old_value &&
          nl_semantic_scope_view(c, loan->loan.scope, &scope) &&
          !scope.active && reads[0]->field.parent == e.parent &&
          reads[0]->field.child == e.child &&
          reads[0]->field.child_fact == e.child_post_fact &&
          reads[0]->field.parent_fact == e.parent_post_fact &&
          reads[1]->field.parent == e.parent &&
          reads[1]->field.child != e.child &&
          reads[1]->field.parent_fact == e.parent_post_fact &&
          scalar(c, reads[0]->results[0].value, expected) &&
          scalar(c, reads[1]->results[0].value, sibling));
    CHECK(nl_semantic_place_view(c, e.parent, &parent) &&
          parent.independent_root && parent.implicit_local &&
          parent.incarnation == e.parent_incarnation && !parent.live &&
          !parent.current_fact && nl_semantic_place_view(c, e.child, &child) &&
          child.parent_aggregate == e.parent &&
          child.parent_field_index == field &&
          child.incarnation == e.child_incarnation && !child.live &&
          nl_semantic_place_view(c, reads[1]->field.child, &other) &&
          !other.live && other.parent_aggregate == e.parent);
    printf("{\"function\":%zu,\"call\":%zu,\"call_result\":%zu,\"body_result\":"
           "%zu,"
           "\"root\":%zu,\"root_symbol\":%zu,\"incarnation\":%zu,\"field_"
           "index\":%zu,"
           "\"child\":%zu,\"sibling_child\":%zu,\"incoming_symbol\":%zu,\"ref_"
           "symbol\":%zu,"
           "\"old_symbol\":%zu,\"read_symbols\":[%zu,%zu],\"value\":%zu,"
           "\"old\":%zu,\"sibling\":%zu,"
           "\"closed_and_ended\":true,\"actual_return_to_store\":true}\n",
           call->function, call_id, call->results[0].value, result, e.parent,
           e.base, e.parent_incarnation, field, e.child, reads[1]->field.child,
           incoming, ref->symbol, old_binding, read_symbols[0], read_symbols[1],
           expected, old, sibling);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 6 && observe(argv[1], strtoul(argv[2], NULL, 10),
                                strtoul(argv[3], NULL, 10),
                                strtoul(argv[4], NULL, 10),
                                strtoul(argv[5], NULL, 10))
               ? 0
               : 1;
}
