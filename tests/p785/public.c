#include "arm.h"
static bool run(const char *path, size_t expected)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    ArmCall found[8];
    size_t count = 0;
    CHECK(collect(n.entry, NULL, 0, 0, found, &count, 0) && count == expected);
    const unsigned old[2] = {expected == 1 ? 184u : 237u, 14},
                   next[2] = {expected == 1 ? 27u : 97u, 237},
                   other[2] = {expected == 1 ? 7u : 57u, 203};
    for (size_t i = 0; i < count; ++i) {
        ArmCall a = found[i];
        NLCopyBodyProof *issued = a.body->copy_body;
        NLCopyBodyView v;
        NLCopyBodyParameters p;
        CHECK(valid(a) && nl_checked_copy_body_view(a.caller, a.call, &v) &&
              nl_checked_copy_body_parameters(a.caller, a.call, &p));
        const NLCheckedNodeView *r =
            nl_checked_node_view(a.caller, a.caller->root);
        CHECK(r && r->kind == NL_CHECKED_MATCH_ARM && r->variant == 2 &&
              a.index == 1 &&
              nl_checked_match_arm(a.parent, a.match, a.index) == a.caller &&
              issued->caller == a.caller && issued->owner == a.body &&
              nl_checked_context(a.body) == nl_checked_context(a.caller) &&
              issued->caller_nodes &&
              issued->caller_nodes_origin == a.caller->nodes &&
              issued->caller_root == a.caller->root &&
              issued->caller_count == a.caller->count &&
              issued->caller_source == a.caller->source &&
              issued->caller_body == a.caller->body_owner &&
              !memcmp(issued->caller_nodes, a.caller->nodes,
                      a.caller->count * sizeof(*a.caller->nodes)));
        CHECK(p.field.index == (i ? 0u : 1u) && p.field.dependency_compatible &&
              scalar(v.live_exit, v.result.value, old[i]));
        NLSemanticPlaceView child, parent, sibling, before;
        CHECK(nl_semantic_place_view(p.entry, p.field.child, &before) &&
              before.current_fact == p.field.child_fact &&
              scalar(p.entry, before.current_value, old[i]) &&
              nl_semantic_place_view(p.handoff, p.field.child, &child) &&
              child.incarnation == before.incarnation &&
              child.current_fact != before.current_fact &&
              scalar(p.handoff, child.current_value, next[i]) &&
              nl_semantic_place_view(p.handoff, p.field.parent, &parent) &&
              parent.incarnation == p.field.parent_incarnation &&
              parent.current_fact != p.field.parent_fact &&
              nl_semantic_place_view(p.handoff,
                                     parent.fixed_fields[1 - p.field.index],
                                     &sibling) &&
              scalar(p.handoff, sibling.current_value, other[i]));
        NLSemanticPlaceView old_sibling;
        CHECK(nl_semantic_place_view(p.entry,
                                     parent.fixed_fields[1 - p.field.index],
                                     &old_sibling) &&
              sibling.current_fact == old_sibling.current_fact &&
              sibling.current_value == old_sibling.current_value);
        NLCopyBodyEventView event;
        CHECK(v.event_count == 1 &&
              nl_checked_copy_body_event(a.caller, a.call, 0, &event) &&
              event.kind == NL_COPY_BODY_CHANGE);
        printf(
            "PUBLIC call=%zu arm_root=%zu count=%zu parent_match=%zu index=%zu "
            "field=%zu root=%zu child=%zu old=%u new=%u sibling=%u "
            "body=0 parameters=0 seal=exact source/world=exact\n",
            a.call, a.caller->root, a.caller->count, a.match, a.index,
            p.field.index, p.field.parent, p.field.child, old[i], next[i],
            other[i]);
    }
    char *text = NULL;
    size_t length = 923;
    CHECK(nl_checked_c_node(n.entry, &text, &length) == NL_NODE_C_OK && text &&
          length > 0);
    free(text);
    printf("P790 migration: issued public proof and C17 OK; native separate\n");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && run(argv[1], (size_t)strtoul(argv[2], NULL, 10)) ? 0
                                                                         : 1;
}
