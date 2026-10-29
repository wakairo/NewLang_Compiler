/* C137 P-owned source-issued observations. No R source or fabricated proof. */
#include "../p965/shared.h"
#include "newlang/copy_body.h"
#include <inttypes.h>

static uintptr_t watched_reject;
static size_t rejected;
void __cyg_profile_func_enter(void *function, void *site)
{
    if ((uintptr_t)function == watched_reject && !rejected++)
        printf("P1045 FIRST_PRIVATE_REJECT return_address=0x%" PRIxPTR "\n",
               (uintptr_t)site);
}
void __cyg_profile_func_exit(void *function, void *site)
{
    (void)function;
    (void)site;
}
typedef struct Observation {
    const NLCheckedFragment *issuer;
    NLPlaceId root, child;
    NLIncarnationId incarnation;
    NLCheckedNodeId loan;
    size_t function, count;
} Observation;
static bool scalar(const NLSemanticContext *c, NLValueId id, size_t *out)
{
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(c, id, &v) && v.scalar_known &&
          v.scalar_value <= 255 && v.dependencies == NL_DEPENDENCY_FREE);
    *out = v.scalar_value;
    return true;
}
static bool inspect(const NLCheckedFragment *f, size_t depth, Observation *o)
{
    CHECK(f && depth <= 2);
    const size_t olds[2] = {19, 67}, nexts[2] = {88, 19};
    const size_t siblings[2] = {149, 222};
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(f, id);
        CHECK(node);
        if (node->kind == NL_CHECKED_MATCH && node->item_count == 2) {
            const NLCheckedNodeView *trial =
                nl_checked_node_view(f, node->initializer);
            if (trial && trial->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
                for (size_t arm = 0; arm < 2; ++arm)
                    CHECK(inspect(nl_checked_match_arm(f, id, arm), depth + 1,
                                  o));
        }
        NLCopyBodyParameters p;
        if (!nl_checked_copy_body_parameters(f, id, &p))
            continue;
        NLCopyBodyView body;
        NLCopyBodyEventView event;
        CHECK(o->count < 2 && node->kind == NL_CHECKED_REGISTERED_CALL &&
              nl_checked_copy_body_view(f, id, &body) &&
              body.event_count == 1 &&
              nl_checked_copy_body_event(f, id, 0, &event));
        const NLCheckStatus bs = nl_checked_copy_body_validate(f, id, &body),
                            ps = nl_checked_copy_body_parameters_validate(f, id,
                                                                          &p),
                            es = nl_checked_copy_body_event_validate(f, id, 0,
                                                                     &event);
        printf("P1045 CURRENT ordinal=%zu depth=%zu function=%zu call=%zu "
               "body=%d parameters=%d event=%d GET=YES\n",
               o->count, depth, body.function, id, (int)bs, (int)ps, (int)es);
        CHECK(bs == NL_CHECK_OK && ps == NL_CHECK_OK && es == NL_CHECK_OK &&
              event.kind == NL_COPY_BODY_CHANGE && p.field.index == o->count);
        NLSemanticPlaceView root, before, after, sibling0, sibling1;
        size_t old, next, returned, sibling;
        CHECK(nl_semantic_place_view(p.entry, p.field.parent, &root) &&
              root.live && root.independent_root && root.implicit_local &&
              root.fixed_field_count == 2 &&
              root.incarnation == p.field.parent_incarnation &&
              root.fixed_fields[p.field.index] == p.field.child &&
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
        CHECK(before.live && after.live && returned == old &&
              old == olds[o->count] && next == nexts[o->count] &&
              sibling == siblings[o->count] &&
              before.incarnation == after.incarnation &&
              before.current_fact != after.current_fact &&
              sibling0.current_fact == sibling1.current_fact &&
              sibling0.current_value == sibling1.current_value &&
              sibling0.incarnation == sibling1.incarnation);
        if (!o->count) {
            o->issuer = f;
            o->root = p.field.parent;
            o->child = p.field.child;
            o->incarnation = p.field.parent_incarnation;
            o->loan = p.loan;
            o->function = body.function;
        } else {
            NLSemanticPlaceView previous;
            CHECK(o->issuer == f && o->root != p.field.parent &&
                  o->child != p.field.child && o->loan != p.loan &&
                  o->function != body.function &&
                  nl_semantic_place_view(p.entry, o->root, &previous) &&
                  previous.live && previous.incarnation == o->incarnation);
        }
        printf("P1045 VALUES ordinal=%zu root=%zu child=%zu incarnation=%zu "
               "loan=%zu index=%zu old=%zu next=%zu sibling=%zu "
               "separate_originals=YES sibling_current=UNCHANGED\n",
               o->count++, p.field.parent, p.field.child,
               p.field.parent_incarnation, p.loan, p.field.index, old, next,
               sibling);
    }
    return true;
}
static bool run(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode node = {0};
    CHECK(registry(path, &base) && checked(base, &node));
    const NLCheckedFragment *body =
        nl_checked_call_body(node.entry, nl_checked_root(node.entry));
    Observation o = {0};
    CHECK(nl_h_custody_validate(node.entry) && inspect(body, 0, &o) &&
          o.count == 2);
    char *text = NULL;
    size_t length = 777;
    const NLNodeCStatus status = nl_checked_c_node(node.entry, &text, &length);
    printf("P1045 C17 status=%d published=%s length=%zu\n", (int)status,
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
