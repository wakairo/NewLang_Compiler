/* New forward epoch for the immutable P1013 formerly unsupported sibling case.
 */
#include "support.h"
static bool value(const NLSemanticContext *world, NLValueId id, size_t expected)
{
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(world, id, &v) && v.scalar_known &&
          v.scalar_value == expected);
    return true;
}
static bool run(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode node = {0};
    P1030Call calls[2];
    CHECK(registry(path, &base) && checked(base, &node) &&
          p1030_calls(&node, calls) && p1030_valid(calls));
    const size_t old[2] = {41, 173}, next[2] = {67, 109},
                 sibling[2] = {173, 67};
    for (size_t i = 0; i < 2; ++i) {
        const P1030Call *c = &calls[i];
        const NLCopyBodyParameters *p = &c->parameters;
        NLSemanticPlaceView root, before, after, s0, s1;
        CHECK(
            p->field.index == i &&
            nl_semantic_place_view(p->entry, p->field.parent, &root) &&
            root.fixed_field_count == 2 &&
            nl_semantic_place_view(p->entry, p->field.child, &before) &&
            nl_semantic_place_view(p->handoff, p->field.child, &after) &&
            nl_semantic_place_view(p->entry, root.fixed_fields[1 - i], &s0) &&
            nl_semantic_place_view(p->handoff, root.fixed_fields[1 - i], &s1) &&
            before.incarnation == after.incarnation &&
            s0.current_fact == s1.current_fact &&
            value(p->entry, before.current_value, old[i]) &&
            value(p->handoff, c->view.result.value, old[i]) &&
            value(p->handoff, after.current_value, next[i]) &&
            value(p->handoff, s1.current_value, sibling[i]));
        NLCopyBodyEventView event;
        CHECK(c->view.event_count == 1 &&
              nl_checked_copy_body_event(c->caller, c->call, 0, &event) &&
              nl_checked_copy_body_event_validate(c->caller, c->call, 0,
                                                  &event) == NL_CHECK_OK);
        printf("P1030 CURRENT ordinal=%zu function=%zu call=%zu body=0 "
               "parameters=0 event=0\n",
               i, c->view.function, c->call);
        printf("P1030 FORWARD VALUES index=%zu old=%zu new=%zu sibling=%zu "
               "original=YES\n",
               i, old[i], next[i], sibling[i]);
    }
    char *text = NULL;
    size_t bytes = 777;
    CHECK(nl_checked_c_node(node.entry, &text, &bytes) == NL_NODE_C_OK &&
          text && bytes);
    printf("P1030 C17 status=0 published=YES length=%zu\n", bytes);
    free(text);
    node_checked_destroy(&node);
    nl_semantic_destroy(base);
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    return argc == 2 && run(argv[1]) ? 0 : 1;
}
