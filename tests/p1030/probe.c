/* New P-owned actual-source observation, not independent/BLIND R evidence. */
#include "../p965/shared.h"
#include "newlang/copy_body.h"
#include <inttypes.h>

static uintptr_t watched_reject;
static size_t rejected;
void __cyg_profile_func_enter(void *function, void *site)
{
    if ((uintptr_t)function == watched_reject && !rejected++)
        printf("P1030 FIRST_PRIVATE_REJECT return_address=0x%" PRIxPTR "\n",
               (uintptr_t)site);
}
void __cyg_profile_func_exit(void *function, void *site)
{
    (void)function;
    (void)site;
}
static bool known(const NLSemanticContext *c, NLValueId id, size_t *out)
{
    NLSemanticValueView value;
    CHECK(nl_semantic_value_view(c, id, &value) && value.scalar_known);
    *out = value.scalar_value;
    return true;
}
static bool run(const char *source)
{
    NLSemanticContext *base = NULL;
    TestNode node = {0};
    CHECK(registry(source, &base) && checked(base, &node));
    const NLCheckedFragment *caller =
        nl_checked_call_body(node.entry, nl_checked_root(node.entry));
    CHECK(caller && nl_h_custody_validate(node.entry));
    const size_t old_values[2] = {203, 58}, new_values[2] = {97, 203};
    const size_t siblings[2] = {58, 97}, indices[2] = {1, 0};
    NLPlaceId parent = 0, first_child = 0;
    NLIncarnationId parent_incarnation = 0;
    size_t count = 0, function = 0;
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        NLCopyBodyView body;
        NLCopyBodyParameters p;
        if (!nl_checked_copy_body_parameters(caller, id, &p))
            continue;
        CHECK(count < 2 && nl_checked_copy_body_view(caller, id, &body));
        NLCheckStatus bs = nl_checked_copy_body_validate(caller, id, &body);
        NLCheckStatus ps =
            nl_checked_copy_body_parameters_validate(caller, id, &p);
        NLCopyBodyEventView event;
        CHECK(body.event_count == 1 &&
              nl_checked_copy_body_event(caller, id, 0, &event));
        NLCheckStatus es =
            nl_checked_copy_body_event_validate(caller, id, 0, &event);
        printf("P1030 CURRENT ordinal=%zu function=%zu call=%zu body=%d "
               "parameters=%d event=%d\n",
               count, body.function, id, (int)bs, (int)ps, (int)es);
        CHECK(bs == NL_CHECK_OK && ps == NL_CHECK_OK && es == NL_CHECK_OK &&
              event.kind == NL_COPY_BODY_CHANGE &&
              p.field.index == indices[count]);
        NLSemanticPlaceView root, before, after, sibling0, sibling1;
        size_t old, returned, next, sibling;
        CHECK(nl_semantic_place_view(p.entry, p.field.parent, &root) &&
              root.live && root.fixed_field_count == 2 &&
              nl_semantic_place_view(p.entry, p.field.child, &before) &&
              nl_semantic_place_view(p.handoff, p.field.child, &after) &&
              nl_semantic_place_view(
                  p.entry, root.fixed_fields[1 - p.field.index], &sibling0) &&
              nl_semantic_place_view(
                  p.handoff, root.fixed_fields[1 - p.field.index], &sibling1) &&
              known(p.entry, before.current_value, &old) &&
              known(p.handoff, body.result.value, &returned) &&
              known(p.handoff, after.current_value, &next) &&
              known(p.handoff, sibling1.current_value, &sibling));
        CHECK(before.live && after.live &&
              before.incarnation == after.incarnation &&
              before.current_fact != after.current_fact &&
              old == old_values[count] && returned == old &&
              next == new_values[count] && sibling == siblings[count] &&
              sibling0.current_fact == sibling1.current_fact &&
              sibling0.incarnation == sibling1.incarnation);
        if (!count) {
            parent = p.field.parent;
            first_child = p.field.child;
            parent_incarnation = p.field.parent_incarnation;
            function = body.function;
        } else {
            CHECK(parent == p.field.parent && first_child != p.field.child &&
                  parent_incarnation == p.field.parent_incarnation &&
                  function != body.function);
        }
        printf("P1030 VALUES ordinal=%zu index=%zu old=%zu new=%zu sibling=%zu "
               "parent=%zu child=%zu root_incarnation=%zu loan=%zu "
               "sibling_fact=UNCHANGED\n",
               count++, p.field.index, old, next, sibling, p.field.parent,
               p.field.child, p.field.parent_incarnation, p.loan);
    }
    CHECK(count == 2);
    char *text = NULL;
    size_t length = 777;
    NLNodeCStatus status = nl_checked_c_node(node.entry, &text, &length);
    printf("P1030 C17 status=%d published=%s length=%zu\n", (int)status,
           text ? "YES" : "NO", length);
    CHECK((status == NL_NODE_C_OK && text && length) ||
          (status == NL_NODE_C_UNSUPPORTED && !text && length == 777));
    free(text);
    node_checked_destroy(&node);
    nl_semantic_destroy(base);
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    if (argc == 3)
        watched_reject = (uintptr_t)strtoull(argv[2], NULL, 16);
    return (argc == 2 || argc == 3) && run(argv[1]) ? 0 : 1;
}
