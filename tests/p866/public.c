#include "../support/node_checked.h"
#include "newlang/copy_body.h"
#include "newlang/if_field.h"
static bool observe(const char *file)
{
    TestNode n = {0};
    CHECK(node_checked_load(file, &n));
    const NLCheckedFragment *caller =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    size_t count = 0;
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        NLCopyUnitView v;
        if (!nl_checked_copy_unit_view(caller, id, &v))
            continue;
        NLIfReturnView r;
        CHECK(nl_checked_if_return_view(v.body, v.conditional, &r));
        CHECK(nl_checked_if_return_validate(v.body, v.conditional, &r) ==
                  NL_CHECK_OK &&
              nl_checked_copy_unit_validate(caller, id, &v) == NL_CHECK_OK &&
              nl_checked_copy_body_parameters_validate(
                  caller, id, &v.parameters) == NL_CHECK_OK);
        const NLCheckedNodeView *loan =
            nl_checked_node_view(caller, v.parameters.loan);
        const NLCheckedNodeView *arg =
            nl_checked_node_view(caller, v.parameters.arguments[0]);
        NLSemanticPlaceView before, post, child, sibling, prior_sibling,
            terminal;
        NLSemanticValueView unknown;
        NLSemanticScopeView scope;
        CHECK(
            nl_semantic_place_view(v.parameters.entry, r.field.root, &before) &&
            nl_semantic_place_view(v.postcall, r.field.root, &post) &&
            nl_semantic_place_view(v.postcall, r.field.child, &child) &&
            nl_semantic_place_view(v.postcall, r.field.sibling, &sibling) &&
            nl_semantic_place_view(v.parameters.entry, r.field.sibling,
                                   &prior_sibling) &&
            nl_semantic_place_view(r.return_ended, r.field.root, &terminal) &&
            nl_semantic_value_view(v.postcall, child.current_value, &unknown));
        CHECK(before.live && post.live && terminal.live &&
              before.incarnation == post.incarnation &&
              post.incarnation == terminal.incarnation &&
              before.fixed_fields[r.field.index] == r.field.child &&
              child.parent_aggregate == r.field.root &&
              before.current_fact != post.current_fact &&
              !unknown.scalar_known &&
              sibling.current_fact == prior_sibling.current_fact &&
              sibling.current_value == prior_sibling.current_value &&
              nl_semantic_scope_view(v.postcall, loan->loan.scope, &scope) &&
              scope.active &&
              nl_semantic_scope_view(n.context, loan->loan.scope, &scope) &&
              !scope.active);
        printf("{\"unit\":0,\"parameters\":0,\"postcall_unknown\":true,"
               "\"call\":%zu,\"function\":%zu,\"base\":%zu,\"root\":%zu,"
               "\"child\":%zu,\"index\":%zu,\"scope\":%zu,\"caller_ref\":%zu,"
               "\"ref_formal\":%zu,\"value_formal\":%zu,\"if\":%zu,\"normal\":%"
               "zu,\"parent_before\":%zu,\"parent_post\":%zu,\"child_post\":%"
               "zu,\"streams\":[",
               id, v.function, r.field.base, r.field.root, r.field.child,
               r.field.index, loan->loan.scope, arg->symbol,
               v.parameters.parameters[0], v.parameters.parameters[1],
               v.conditional, r.normal_arm, before.current_fact,
               post.current_fact, child.current_fact);
        for (size_t a = 0; a < 3; ++a) {
            size_t events =
                a == 2 ? r.replay_events : r.field.arms[a].event_count;
            printf("%s[", a ? "," : "");
            for (size_t i = 0; i < events; ++i) {
                NLIfFieldEvent e;
                NLSemanticValueView old, incoming;
                CHECK(nl_checked_if_return_event(v.body, v.conditional, a, i,
                                                 &e) &&
                      e.change &&
                      nl_checked_if_return_event_validate(
                          v.body, v.conditional, a, i, &e) == NL_CHECK_OK &&
                      nl_semantic_value_view(e.before, e.result, &old) &&
                      old.scalar_known &&
                      nl_semantic_value_view(e.before, e.incoming, &incoming) &&
                      incoming.scalar_known);
                printf("%s{\"node\":%zu,\"old_id\":%zu,\"incoming_id\":%zu,"
                       "\"old\":%zu,\"incoming\":%zu}",
                       i ? "," : "", e.node, e.result, e.incoming,
                       old.scalar_value, incoming.scalar_value);
            }
            fputs("]", stdout);
        }
        fputs("],\"reads\":[", stdout);
        size_t reads = 0;
        for (size_t i = 1; i <= nl_checked_node_count(caller); ++i) {
            const NLCheckedNodeView *b = nl_checked_node_view(caller, i);
            const NLCheckedNodeView *read =
                b->kind == NL_CHECKED_BINDING
                    ? nl_checked_node_view(caller, b->initializer)
                    : NULL;
            if (read && read->kind == NL_CHECKED_FIELD_READ) {
                printf("%s{\"symbol\":%zu,\"base\":%zu,\"index\":%zu}",
                       reads ? "," : "", b->symbol, read->field.base,
                       read->field.index);
                ++reads;
            }
        }
        fputs("],\"direct\":[", stdout);
        size_t changes = 0;
        for (size_t i = 1; i <= nl_checked_node_count(caller); ++i) {
            const NLCheckedNodeView *op = nl_checked_node_view(caller, i);
            if (op->kind != NL_CHECKED_REPLACE)
                continue;
            const NLCheckedNodeView *ref =
                nl_checked_node_view(caller, op->first_argument);
            printf("%s{\"node\":%zu,\"base\":%zu,\"index\":%zu,\"ref\":%zu}",
                   changes ? "," : "", i, op->field.base, op->field.index,
                   ref->symbol);
            ++changes;
        }
        puts("]}");
        CHECK(reads == 4 && changes == 1);
        ++count;
    }
    CHECK(count == 1);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
