#include "../support/node_checked.h"
#include "newlang/copy_body.h"
static bool probe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *caller =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(caller);
    size_t calls = 0;
    fputs("{\"calls\":[", stdout);
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        NLCopyBodyParameters p;
        if (!nl_checked_copy_body_parameters(caller, id, &p))
            continue;
        NLCopyBodyView b;
        CHECK(nl_checked_copy_body_view(caller, id, &b) &&
              nl_checked_copy_body_validate(caller, id, &b) == NL_CHECK_OK &&
              nl_checked_copy_body_parameters_validate(caller, id, &p) ==
                  NL_CHECK_OK);
        NLSemanticPlaceView before, after, child, sibling;
        CHECK(nl_semantic_place_view(p.entry, p.field.parent, &before) &&
              nl_semantic_place_view(p.handoff, p.field.parent, &after) &&
              before.incarnation == after.incarnation &&
              before.fixed_fields[0] == after.fixed_fields[0] &&
              before.fixed_fields[1] == after.fixed_fields[1] &&
              before.current_fact != after.current_fact &&
              nl_semantic_place_view(p.handoff, p.field.child, &child) &&
              child.incarnation == p.field.child_incarnation &&
              nl_semantic_place_view(
                  p.handoff, before.fixed_fields[1 - p.field.index], &sibling));
        NLSemanticPlaceView entry_sibling;
        CHECK(nl_semantic_place_view(p.entry,
                                     before.fixed_fields[1 - p.field.index],
                                     &entry_sibling) &&
              entry_sibling.current_fact == sibling.current_fact &&
              entry_sibling.current_value == sibling.current_value);
        const NLCheckedNodeView *loan = nl_checked_node_view(caller, p.loan);
        const NLCheckedNodeView *arg =
            nl_checked_node_view(caller, p.arguments[0]);
        const NLCheckedNodeView *value =
            nl_checked_node_view(caller, p.arguments[1]);
        NLSemanticScopeView in_scope, out_scope;
        CHECK(loan && arg && value &&
              nl_semantic_scope_view(p.entry, loan->loan.scope, &in_scope) &&
              in_scope.active &&
              nl_semantic_scope_view(p.handoff, loan->loan.scope, &out_scope) &&
              out_scope.active &&
              nl_semantic_scope_view(n.context, loan->loan.scope, &out_scope) &&
              !out_scope.active);
        printf(
            "%s{\"call\":%zu,\"function\":%zu,\"loan\":%zu,\"scope\":%zu,"
            "\"ref_formal\":%zu,\"value_formal\":%zu,\"source_root\":%zu,"
            "\"source_symbol\":%zu,\"caller_ref\":%zu,\"index\":%zu,"
            "\"incoming_symbol\":%zu,\"parent_fact\":%zu,\"parent_post\":%zu,"
            "\"child_fact\":%zu,\"child_post\":%zu,\"returned_id\":%zu,"
            "\"changes\":[",
            calls ? "," : "", id, b.function, p.loan, loan->loan.scope,
            p.parameters[0], p.parameters[1], p.field.parent, p.field.base,
            arg->symbol, p.field.index, value->symbol, before.current_fact,
            after.current_fact, p.field.child_fact, child.current_fact,
            b.result.value);
        size_t changes = 0;
        for (size_t j = 0; j < b.event_count; ++j) {
            NLCopyBodyEventView e;
            CHECK(nl_checked_copy_body_event(caller, id, j, &e) &&
                  nl_checked_copy_body_event_validate(caller, id, j, &e) ==
                      NL_CHECK_OK);
            if (e.kind != NL_COPY_BODY_CHANGE)
                continue;
            CHECK(e.field.parent == p.field.parent &&
                  e.field.child == p.field.child &&
                  e.result == e.field.old_value);
            printf("%s{\"node\":%zu,\"old_id\":%zu,\"incoming_id\":%zu}",
                   changes ? "," : "", e.node, e.result, e.incoming);
            ++changes;
        }
        CHECK(changes);
        fputs("]}", stdout);
        ++calls;
    }
    CHECK(calls);
    fputs("],\"direct\":[", stdout);
    size_t direct = 0;
    for (size_t i = 1; i <= nl_checked_node_count(caller); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(caller, i);
        if (v->kind != NL_CHECKED_REPLACE)
            continue;
        const NLCheckedNodeView *ref =
            nl_checked_node_view(caller, v->first_argument);
        const NLCheckedNodeView *value =
            nl_checked_node_view(caller, ref->next_argument);
        printf("%s{\"node\":%zu,\"symbol\":%zu,\"root\":%zu,\"index\":%zu,"
               "\"ref\":%zu,\"incoming_symbol\":%zu}",
               direct ? "," : "", i, v->field.base, v->field.parent,
               v->field.index, ref->symbol, value->symbol);
        ++direct;
    }
    fputs("]}\n", stdout);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
