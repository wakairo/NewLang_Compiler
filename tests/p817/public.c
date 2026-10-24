#include "../p785/arm.h"
static bool all_calls(const NLCheckedFragment *f, size_t *count, size_t depth)
{
    CHECK(f && depth < 20);
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        NLCopyBodyView v;
        NLCopyBodyParameters p;
        if (nl_checked_copy_body_view(f, id, &v)) {
            CHECK(nl_checked_copy_body_parameters(f, id, &p) &&
                  nl_checked_copy_body_validate(f, id, &v) == NL_CHECK_OK &&
                  nl_checked_copy_body_parameters_validate(f, id, &p) ==
                      NL_CHECK_OK);
            ++*count;
            printf("ALL depth=%zu call=%zu body=0 parameters=0\n", depth, id);
        }
        const NLCheckedNodeView *node = nl_checked_node_view(f, id);
        if (node->kind == NL_CHECKED_IF)
            for (size_t arm = 0; arm < 2; ++arm)
                CHECK(
                    all_calls(nl_checked_if_arm(f, id, arm), count, depth + 1));
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body)
            CHECK(all_calls(body, count, depth + 1));
    }
    return true;
}
static bool run(const char *path, bool charge)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    ArmCall found[8];
    size_t count = 0, total = 0;
    CHECK(collect(n.entry, NULL, 0, 0, found, &count, 0) &&
          count == (charge ? 4u : 2u) && all_calls(n.entry, &total, 0) &&
          total == (charge ? 7u : 4u));
    for (size_t i = 0; i < count; ++i) {
        ArmCall a = found[i];
        NLCopyBodyProof *issued = a.body->copy_body;
        NLCopyBodyView v;
        NLCopyBodyParameters p;
        const NLCheckedNodeView *r =
            nl_checked_node_view(a.caller, a.caller->root);
        const NLCheckedNodeView *conditional =
            nl_checked_node_view(a.parent, a.match);
        CHECK(valid(a) && nl_checked_copy_body_view(a.caller, a.call, &v) &&
              nl_checked_copy_body_parameters(a.caller, a.call, &p) && r &&
              r->kind == NL_CHECKED_IF_ARM && conditional &&
              conditional->kind == NL_CHECKED_IF &&
              conditional->normal_arms == 1 && !conditional->terminates &&
              r->terminates == (a.index == 0) &&
              nl_checked_if_arm(a.parent, a.match, a.index) == a.caller &&
              a.caller->if_parent == a.parent &&
              a.caller->if_parent_node == a.match &&
              a.caller->if_arm_index == a.index && issued->caller == a.caller &&
              issued->owner == a.body &&
              nl_checked_context(a.body) == nl_checked_context(a.caller) &&
              issued->caller_nodes &&
              issued->caller_nodes_origin == a.caller->nodes &&
              issued->caller_root == a.caller->root &&
              issued->caller_count == a.caller->count &&
              issued->caller_source == a.caller->source &&
              issued->caller_body == a.caller->body_owner &&
              !memcmp(issued->caller_nodes, a.caller->nodes,
                      a.caller->count * sizeof(*a.caller->nodes)));
        unsigned old = charge ? (i % 2 ? 26 : 54) : 43;
        unsigned next =
            charge ? (i % 2 ? 54 : (a.index ? 148 : 97)) : (a.index ? 126 : 73);
        unsigned other = charge ? (i % 2 ? 220 : 181) : 198;
        size_t field = charge ? (i % 2 ? 0 : 1) : 0;
        NLSemanticPlaceView before, after, root, prior_root, sibling,
            prior_sibling;
        CHECK(p.field.index == field && p.field.dependency_compatible &&
              scalar(v.live_exit, v.result.value, old) &&
              nl_semantic_place_view(p.entry, p.field.child, &before) &&
              before.current_fact == p.field.child_fact &&
              scalar(p.entry, before.current_value, old) &&
              nl_semantic_place_view(p.handoff, p.field.child, &after) &&
              after.incarnation == before.incarnation &&
              after.current_fact != before.current_fact &&
              scalar(p.handoff, after.current_value, next) &&
              nl_semantic_place_view(p.entry, p.field.parent, &prior_root) &&
              nl_semantic_place_view(p.handoff, p.field.parent, &root) &&
              root.incarnation == prior_root.incarnation &&
              root.incarnation == p.field.parent_incarnation &&
              root.current_fact != prior_root.current_fact &&
              root.fixed_fields[field] == p.field.child &&
              nl_semantic_place_view(p.handoff, root.fixed_fields[1 - field],
                                     &sibling) &&
              nl_semantic_place_view(p.entry, root.fixed_fields[1 - field],
                                     &prior_sibling) &&
              sibling.current_fact == prior_sibling.current_fact &&
              sibling.current_value == prior_sibling.current_value &&
              scalar(p.handoff, sibling.current_value, other));
        NLCopyBodyEventView event;
        CHECK(v.event_count == 1 &&
              nl_checked_copy_body_event(a.caller, a.call, 0, &event) &&
              event.kind == NL_COPY_BODY_CHANGE);
        printf("ORIGINAL arm=%zu call=%zu terminal=%d field=%zu "
               "old=%u new=%u sibling=%u exact_snapshot=1 body=0 params=0\n",
               a.index, a.call, r->terminates, field, old, next, other);
    }
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 4 && run(argv[1], atoi(argv[3]) != 0) &&
                   run(argv[2], atoi(argv[3]) != 0)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
