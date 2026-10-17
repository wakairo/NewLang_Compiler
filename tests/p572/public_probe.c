#include "../support/node_checked.h"
#include "newlang/node_links.h"
#include <stdlib.h>

static bool inspect(const char *path)
{
    TestNode n = {0}, other = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    NLNodeLinksView v;
    CHECK(nl_checked_node_links_view(f, &v) &&
          nl_checked_node_links_validate(f, NULL) == NL_CHECK_OK &&
          nl_checked_node_links_validate(f, &v) == NL_CHECK_OK);
    CHECK(node_checked_load(path, &other));
    const NLCheckedFragment *foreign =
        nl_checked_call_body(other.entry, nl_checked_root(other.entry));
    NLNodeLinksView bad;
    size_t refused = 0;
#define REFUSE(change)                                                         \
    do {                                                                       \
        bad = v;                                                               \
        change;                                                                \
        CHECK(nl_checked_node_links_validate(f, &bad) ==                       \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        ++refused;                                                             \
    } while (0)
    REFUSE(bad.origin = foreign);
    for (size_t i = 0; i < 3; ++i) {
        REFUSE(bad.originals[i] = 0);
        REFUSE(bad.originals[i] = v.originals[(i + 1) % 3]);
    }
    for (size_t i = 0; i < 2; ++i) {
        REFUSE(bad.changes[i].operation = 0);
        REFUSE(bad.changes[i].operation = v.changes[1 - i].operation);
        REFUSE(bad.changes[i].world = nl_checked_context(foreign));
        REFUSE(bad.changes[i].before = v.changes[i].after);
        REFUSE(bad.changes[i].after = v.changes[i].before);
    }
#undef REFUSE
    CHECK(nl_checked_node_links_validate(foreign, &v) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    const size_t payloads[3] = {22, 11, 33};
    for (size_t step = 0; step < 2; ++step) {
        const NLSemanticContext *b = v.changes[step].before,
                                *a = v.changes[step].after;
        const NLCheckedNodeView *op =
            nl_checked_node_view(f, v.changes[step].operation);
        CHECK(op && op->kind == NL_CHECKED_REPLACE &&
              op->field.parent == v.originals[step + 1]);
        for (size_t i = 0; i < 3; ++i) {
            NLSemanticPlaceView x, y, sibling, post_sibling;
            NLSemanticValueView value;
            CHECK(nl_semantic_place_view(b, v.originals[i], &x) &&
                  nl_semantic_place_view(a, v.originals[i], &y));
            CHECK(x.live && y.live && x.implicit_local && x.independent_root &&
                  x.incarnation == y.incarnation && !x.governing_domain &&
                  !x.placement.region && x.fixed_field_count == 2);
            CHECK(nl_semantic_place_view(b, x.fixed_fields[1], &sibling) &&
                  nl_semantic_place_view(a, y.fixed_fields[1], &post_sibling));
            CHECK(
                sibling.current_fact == post_sibling.current_fact &&
                sibling.current_value == post_sibling.current_value &&
                nl_semantic_value_view(a, post_sibling.current_value, &value) &&
                value.scalar_known && value.scalar_value == payloads[i]);
            CHECK((i == step + 1) ? x.current_fact != y.current_fact
                                  : x.current_fact == y.current_fact);
            CHECK(x.fixed_fields[0] == y.fixed_fields[0]);
        }
        NLSemanticPlaceView before, after;
        NLSemanticValueView old, next, pointer;
        CHECK(nl_semantic_place_view(b, op->field.child, &before) &&
              nl_semantic_place_view(a, op->field.child, &after));
        CHECK(before.current_fact == op->field.child_fact &&
              after.current_fact == op->field.child_post_fact &&
              before.current_fact != after.current_fact &&
              before.payload_occurrence == op->field.payload_occurrence &&
              after.payload_occurrence == op->field.post_payload_occurrence);
        CHECK(nl_semantic_value_view(b, op->field.old_value, &old) &&
              old.variant == (step ? 1u : 2u));
        if (!step) {
            CHECK(nl_semantic_value_view(b, old.sum_payload, &pointer) &&
                  pointer.reference.place == v.originals[0]);
            NLSemanticOccurrenceView occurrence;
            CHECK(nl_semantic_occurrence_view(b, before.payload_occurrence,
                                              &occurrence) &&
                  occurrence.live &&
                  nl_semantic_occurrence_view(a, before.payload_occurrence,
                                              &occurrence) &&
                  !occurrence.live);
        }
        CHECK(nl_semantic_value_view(a, after.current_value, &next) &&
              next.variant == 2 &&
              nl_semantic_value_view(a, next.sum_payload, &pointer) &&
              pointer.reference.place == v.originals[step ? 1 : 2] &&
              pointer.reference.provenance == NL_PROVENANCE_VALID &&
              pointer.reference.scope == 0 && !pointer.reference_count &&
              !pointer.domain && !pointer.allocation_region);
        printf("CHANGE step=%zu parent=%zu child=%zu RF=%zu->%zu F=%zu->%zu "
               "occurrence=%zu->%zu target=%zu\n",
               step, op->field.parent, op->field.child, op->field.parent_fact,
               op->field.parent_post_fact, op->field.child_fact,
               op->field.child_post_fact, before.payload_occurrence,
               after.payload_occurrence, pointer.reference.place);
    }
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *p = nl_checked_node_view(f, id);
        if (p->kind == NL_CHECKED_BINDING) {
            NLSourceView name;
            CHECK(nl_source_view(nl_checked_source(f), p->name, &name));
            printf("B %.*s %zu\n", (int)name.length, name.bytes, p->symbol);
        }
    }
    for (size_t i = 0; i < 3; ++i) {
        NLSemanticPlaceView ended;
        CHECK(nl_semantic_place_view(n.context, v.originals[i], &ended) &&
              !ended.live && ended.current_fact == 0);
        printf("ROOT index=%zu place=%zu incarnation=%zu final_ended=1\n", i,
               v.originals[i], ended.incarnation);
    }
    printf("PUBLIC NODE_LINKS VALID roots=3 changes=2 copied_view_refusals=%zu "
           "input_AST_destroyed=1\n",
           refused);
    node_checked_destroy(&other);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
