/* Track R #708: independent registered-source historical witness.
 * Uses public views as read-only observations; modified copies are NOT private
 * issuer records. No authority is constructed by this observer.
 */
#include "../support/node_checked.h"
#include "newlang/copy_body.h"
#include <stdio.h>

static bool bad_parameters(const NLCheckedFragment *f, size_t call,
                           NLCopyBodyParameters p)
{
    return nl_checked_copy_body_parameters_validate(f, call, &p) ==
           NL_CHECK_ANALYSIS_PRECISION_LIMIT;
}
static bool bad_body(const NLCheckedFragment *f, size_t call,
                     NLCopyBodyView p)
{
    return nl_checked_copy_body_validate(f, call, &p) ==
           NL_CHECK_ANALYSIS_PRECISION_LIMIT;
}
static bool bad_event(const NLCheckedFragment *f, size_t call, size_t index,
                      NLCopyBodyEventView p)
{
    return nl_checked_copy_body_event_validate(f, call, index, &p) ==
           NL_CHECK_ANALYSIS_PRECISION_LIMIT;
}
static bool examine(const char *path, size_t required_calls)
{
    TestNode n = {0}, other = {0};
    CHECK(node_checked_load(path, &n));
    CHECK(node_checked_load(path, &other));
    const NLCheckedFragment *caller =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    const NLCheckedFragment *different_world =
        nl_checked_call_body(other.entry, nl_checked_root(other.entry));
    CHECK(caller && different_world && caller != different_world);
    size_t calls = 0, rejects = 0, prior_root = 0;
    fputs("{\"calls\":[", stdout);
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        NLCopyBodyParameters p;
        if (!nl_checked_copy_body_parameters(caller, id, &p))
            continue;
        NLCopyBodyView b;
        CHECK(nl_checked_copy_body_view(caller, id, &b));
        CHECK(nl_checked_copy_body_parameters_validate(caller, id, &p) ==
              NL_CHECK_OK);
        CHECK(nl_checked_copy_body_validate(caller, id, NULL) == NL_CHECK_OK);
        CHECK(nl_checked_copy_body_validate(caller, id, &b) == NL_CHECK_OK);
        CHECK(p.field.present && p.field.index < 2 &&
              p.field.child != p.field.parent && p.field.base &&
              p.field.parent && p.field.child && p.entry && p.handoff);
        const NLCheckedNodeView *loan = nl_checked_node_view(caller, p.loan);
        const NLCheckedNodeView *ref =
            nl_checked_node_view(caller, p.arguments[0]);
        const NLCheckedNodeView *input =
            nl_checked_node_view(caller, p.arguments[1]);
        CHECK(loan && ref && input && loan->loan.scope && ref->symbol);
        NLSemanticScopeView active, after_call, ended;
        CHECK(nl_semantic_scope_view(p.entry, loan->loan.scope, &active) &&
              active.active &&
              nl_semantic_scope_view(p.handoff, loan->loan.scope, &after_call) &&
              after_call.active &&
              nl_semantic_scope_view(n.context, loan->loan.scope, &ended) &&
              !ended.active);
        NLSemanticPlaceView parent_before, parent_after, child_before,
            child_after, sibling_before, sibling_after;
        CHECK(nl_semantic_place_view(p.entry, p.field.parent, &parent_before) &&
              nl_semantic_place_view(p.handoff, p.field.parent, &parent_after));
        CHECK(parent_before.incarnation == parent_after.incarnation &&
              parent_before.current_fact != parent_after.current_fact &&
              parent_before.fixed_fields[0] == parent_after.fixed_fields[0] &&
              parent_before.fixed_fields[1] == parent_after.fixed_fields[1] &&
              parent_before.fixed_fields[p.field.index] == p.field.child);
        CHECK(nl_semantic_place_view(p.entry, p.field.child, &child_before) &&
              nl_semantic_place_view(p.handoff, p.field.child, &child_after) &&
              child_before.incarnation == child_after.incarnation &&
              child_before.current_fact != child_after.current_fact);
        const size_t sibling =
            parent_before.fixed_fields[1 - p.field.index];
        CHECK(sibling && sibling != p.field.child &&
              nl_semantic_place_view(p.entry, sibling, &sibling_before) &&
              nl_semantic_place_view(p.handoff, sibling, &sibling_after) &&
              sibling_before.incarnation == sibling_after.incarnation &&
              sibling_before.current_fact == sibling_after.current_fact &&
              sibling_before.current_value == sibling_after.current_value);
        if (prior_root)
            CHECK(prior_root != p.field.parent);
        prior_root = p.field.parent;
        size_t count_change = 0, old_id = 0, incoming_id = 0, change_node = 0;
        for (size_t j = 0; j < b.event_count; ++j) {
            NLCopyBodyEventView e;
            CHECK(nl_checked_copy_body_event(caller, id, j, &e) &&
                  nl_checked_copy_body_event_validate(caller, id, j, &e) ==
                      NL_CHECK_OK);
            if (e.kind == NL_COPY_BODY_CHANGE) {
                CHECK(e.field.parent == p.field.parent &&
                      e.field.child == p.field.child &&
                      e.field.index == p.field.index &&
                      e.result == e.field.old_value &&
                      e.result == b.result.value);
                ++count_change;
                old_id = e.result;
                incoming_id = e.incoming;
                change_node = e.node;
                NLCopyBodyEventView wrong = e;
                wrong.field.index = 1 - wrong.field.index;
                CHECK(bad_event(caller, id, j, wrong));
                ++rejects;
                wrong = e;
                ++wrong.field.parent_post_fact;
                CHECK(bad_event(caller, id, j, wrong));
                ++rejects;
                wrong = e;
                wrong.result = e.incoming;
                CHECK(bad_event(caller, id, j, wrong));
                ++rejects;
                wrong = e;
                wrong.before = nl_checked_context(other.entry);
                CHECK(bad_event(caller, id, j, wrong));
                ++rejects;
            }
            NLCopyBodyEventView wrong = e;
            wrong.kind = (NLCopyBodyEventKind)((e.kind + 1) % 3);
            CHECK(bad_event(caller, id, j, wrong));
            ++rejects;
        }
        CHECK(count_change == 1 && old_id && incoming_id && change_node);
        NLCopyBodyParameters wrong = p;
        wrong.field.index = 1 - p.field.index;
        CHECK(bad_parameters(caller, id, wrong));
        ++rejects;
        wrong = p;
        wrong.field.parent = sibling;
        CHECK(bad_parameters(caller, id, wrong));
        ++rejects;
        wrong = p;
        wrong.field.child = sibling;
        CHECK(bad_parameters(caller, id, wrong));
        ++rejects;
        wrong = p;
        wrong.field.child_fact++;
        CHECK(bad_parameters(caller, id, wrong));
        ++rejects;
        wrong = p;
        wrong.field.parent_incarnation++;
        CHECK(bad_parameters(caller, id, wrong));
        ++rejects;
        wrong = p;
        wrong.arguments[0] = p.arguments[1];
        CHECK(bad_parameters(caller, id, wrong));
        ++rejects;
        wrong = p;
        wrong.parameters[0] = p.parameters[1];
        CHECK(bad_parameters(caller, id, wrong));
        ++rejects;
        wrong = p;
        wrong.entry = nl_checked_context(other.entry);
        CHECK(bad_parameters(caller, id, wrong));
        ++rejects;
        wrong = p;
        wrong.handoff = nl_checked_context(other.entry);
        CHECK(bad_parameters(caller, id, wrong));
        ++rejects;
        NLCopyBodyView tampered = b;
        tampered.event_count = b.event_count + 1;
        CHECK(bad_body(caller, id, tampered));
        ++rejects;
        tampered = b;
        tampered.function = 0;
        CHECK(bad_body(caller, id, tampered));
        ++rejects;
        tampered = b;
        tampered.result.value = 0;
        CHECK(bad_body(caller, id, tampered));
        ++rejects;
        tampered = b;
        tampered.live_exit = nl_checked_context(other.entry);
        CHECK(bad_body(caller, id, tampered));
        ++rejects;
        /* Revalidation after each borrowed-view edit must retain real origin. */
        CHECK(nl_checked_copy_body_validate(caller, id, NULL) == NL_CHECK_OK);
        printf("%s{\"call\":%zu,\"function\":%zu,\"root\":%zu,"
               "\"symbol\":%zu,\"index\":%zu,\"ref\":%zu,"
               "\"value_formal\":%zu,\"value_actual\":%zu,"
               "\"old_id\":%zu,\"incoming_id\":%zu,"
               "\"change\":%zu,\"parent_before\":%zu,\"parent_after\":%zu,"
               "\"child_before\":%zu,\"child_after\":%zu}",
               calls ? "," : "", id, b.function, p.field.parent,
               p.field.base, p.field.index, ref->symbol, p.parameters[1],
               input->symbol, old_id, incoming_id, change_node,
               parent_before.current_fact, parent_after.current_fact,
               child_before.current_fact, child_after.current_fact);
        ++calls;
    }
    CHECK(calls == required_calls && rejects >= required_calls * 13);
    fputs("],\"direct\":[", stdout);
    size_t direct = 0;
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(caller, id);
        CHECK(v);
        if (v->kind != NL_CHECKED_REPLACE || !v->field.present)
            continue;
        const NLCheckedNodeView *ref =
            nl_checked_node_view(caller, v->first_argument);
        CHECK(ref);
        printf("%s{\"node\":%zu,\"root\":%zu,\"symbol\":%zu,"
               "\"index\":%zu,\"ref\":%zu}",
               direct ? "," : "", id, v->field.parent, v->field.base,
               v->field.index, ref->symbol);
        ++direct;
    }
    printf("],\"rejections\":%zu}\n", rejects);
    node_checked_destroy(&other);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && examine(argv[1], (size_t)(argv[2][0] - '0')) ? 0
                                                                      : 1;
}
