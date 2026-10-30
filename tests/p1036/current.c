/* P1036 actual source observation; no fabricated node/world or R fixture. */
#include "../p965/shared.h"
#include "newlang/copy_body.h"
#include <inttypes.h>

static uintptr_t watched_reject;
static size_t rejected;
void __cyg_profile_func_enter(void *function, void *site)
{
    if ((uintptr_t)function == watched_reject && !rejected++)
        printf("P1036 FIRST_PRIVATE_REJECT return_address=0x%" PRIxPTR "\n",
               (uintptr_t)site);
}
void __cyg_profile_func_exit(void *function, void *site)
{
    (void)function;
    (void)site;
}
static bool scalar(const NLSemanticContext *c, NLValueId id, size_t *out)
{
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(c, id, &v) && v.scalar_known &&
          v.scalar_value <= 255 && v.dependencies == NL_DEPENDENCY_FREE &&
          !v.reference_count && !v.value_dependency_count);
    *out = v.scalar_value;
    return true;
}
static bool observe(const char *source, size_t first)
{
    NLSemanticContext *base = NULL;
    TestNode node = {0};
    CHECK(first < 3 && registry(source, &base) && checked(base, &node));
    const NLCheckedFragment *caller =
        nl_checked_call_body(node.entry, nl_checked_root(node.entry));
    CHECK(caller && nl_h_custody_validate(node.entry));
    const bool compatibility = first == 2;
    if (compatibility)
        first = 0;
    const size_t initial[2] = {compatibility ? 41 : 37, 173};
    const size_t indices[3] = {first, compatibility ? first : 1 - first, first};
    const size_t olds[3] = {initial[first],
                            compatibility ? 67 : initial[1 - first],
                            compatibility ? 109 : 94};
    const size_t nexts[3] = {compatibility ? 67 : 94,
                             compatibility ? 109 : initial[first],
                             compatibility ? 151 : initial[1 - first]};
    const size_t siblings[3] = {initial[1 - first], compatibility ? 173 : 94,
                                compatibility ? 173 : initial[first]};
    NLPlaceId original = 0, children[2] = {0};
    NLIncarnationId incarnation = 0, child_incarnations[2] = {0};
    NLCheckedNodeId loans[3] = {0};
    size_t functions[3] = {0}, count = 0;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(caller); ++id) {
        NLCopyBodyParameters p;
        if (!nl_checked_copy_body_parameters(caller, id, &p))
            continue;
        NLCopyBodyView body;
        NLCopyBodyEventView event;
        CHECK(count < 3 && nl_checked_copy_body_view(caller, id, &body) &&
              body.event_count == 1 &&
              nl_checked_copy_body_event(caller, id, 0, &event));
        NLCheckStatus bs = nl_checked_copy_body_validate(caller, id, &body),
                      ps = nl_checked_copy_body_parameters_validate(caller, id,
                                                                    &p),
                      es = nl_checked_copy_body_event_validate(caller, id, 0,
                                                               &event);
        printf("P1036 CURRENT ordinal=%zu function=%zu call=%zu "
               "body=%d parameters=%d event=%d GET=YES\n",
               count, body.function, id, (int)bs, (int)ps, (int)es);
        CHECK(bs == NL_CHECK_OK && ps == NL_CHECK_OK && es == NL_CHECK_OK &&
              event.kind == NL_COPY_BODY_CHANGE &&
              p.field.index == indices[count]);
        NLSemanticPlaceView root, root_after, before, after, sibling0, sibling1;
        size_t old, next, returned, sibling;
        CHECK(nl_semantic_place_view(p.entry, p.field.parent, &root) &&
              root.live && root.independent_root && root.implicit_local &&
              root.fixed_field_count == 2 &&
              nl_semantic_place_view(p.handoff, p.field.parent, &root_after) &&
              nl_semantic_place_view(p.entry, p.field.child, &before) &&
              nl_semantic_place_view(p.handoff, p.field.child, &after) &&
              nl_semantic_place_view(
                  p.entry, root.fixed_fields[1 - p.field.index], &sibling0) &&
              nl_semantic_place_view(
                  p.handoff, root.fixed_fields[1 - p.field.index], &sibling1) &&
              scalar(p.entry, before.current_value, &old) &&
              scalar(p.handoff, after.current_value, &next) &&
              scalar(p.handoff, body.result.value, &returned) &&
              scalar(p.handoff, sibling1.current_value, &sibling));
        CHECK(old == olds[count] && next == nexts[count] && returned == old &&
              sibling == siblings[count] && before.live && after.live &&
              before.incarnation == after.incarnation &&
              before.current_fact != after.current_fact &&
              root.current_fact != root_after.current_fact &&
              root.incarnation == root_after.incarnation &&
              sibling0.current_fact == sibling1.current_fact &&
              sibling0.current_value == sibling1.current_value &&
              sibling0.incarnation == sibling1.incarnation);
        if (!count) {
            original = p.field.parent;
            incarnation = p.field.parent_incarnation;
            for (size_t i = 0; i < 2; ++i) {
                NLSemanticPlaceView child;
                children[i] = root.fixed_fields[i];
                CHECK(nl_semantic_place_view(p.entry, children[i], &child));
                child_incarnations[i] = child.incarnation;
            }
        }
        CHECK(original == p.field.parent &&
              incarnation == p.field.parent_incarnation &&
              p.field.child == children[p.field.index] &&
              p.field.child_incarnation == child_incarnations[p.field.index]);
        for (size_t i = 0; i < count; ++i)
            CHECK(functions[i] != body.function && loans[i] != p.loan);
        functions[count] = body.function;
        loans[count] = p.loan;
        NLSymbolId binding = 0;
        for (NLCheckedNodeId j = 1; j <= nl_checked_node_count(caller); ++j) {
            const NLCheckedNodeView *b = nl_checked_node_view(caller, j);
            if (b->kind == NL_CHECKED_BINDING && b->initializer == p.loan) {
                CHECK(!binding && b->symbol);
                binding = b->symbol;
            }
        }
        CHECK(binding);
        printf("P1036 RESULT ordinal=%zu binding=%zu\n", count, binding);
        printf(
            "P1036 VALUES ordinal=%zu index=%zu old=%zu next=%zu sibling=%zu "
            "root=%zu root_incarnation=%zu child=%zu child_incarnation=%zu "
            "entry_parent_fact=%zu handoff_parent_fact=%zu "
            "sibling_fact=UNCHANGED\n",
            count++, p.field.index, old, next, sibling, original, incarnation,
            p.field.child, p.field.child_incarnation, root.current_fact,
            root_after.current_fact);
    }
    CHECK(count == 3);
    char *text = NULL;
    size_t length = 777;
    NLNodeCStatus status = nl_checked_c_node(node.entry, &text, &length);
    printf("P1036 C17 status=%d published=%s length=%zu\n", (int)status,
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
    if (argc == 4)
        watched_reject = (uintptr_t)strtoull(argv[3], NULL, 16);
    return (argc == 3 || argc == 4) &&
                   observe(argv[1], strtoull(argv[2], NULL, 10))
               ? 0
               : 1;
}
