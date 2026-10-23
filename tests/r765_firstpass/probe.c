#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include "newlang/copy_body.h"
#include <stdio.h>
#include <stdlib.h>

/* Independently selected R765 original calls, not producer oracle IDs.
 * This probe only accepts authentic checked source from node_checked_load.
 * No host-minted copy receipt can satisfy the positive assertions.
 */
static bool verify(const char *source, const size_t expected[8])
{
    TestNode state = {0};
    CHECK(node_checked_load(source, &state));
    const NLCheckedFragment *body =
        nl_checked_call_body(state.entry, nl_checked_root(state.entry));
    CHECK(body);
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(nl_checked_context(body), &snapshot));
    size_t h_count = 0;
    NLTypeId h_type = 0;
    for (size_t t = 1; t <= snapshot.types; ++t) {
        if (nl_semantic_recursive_local_type(nl_checked_context(body), t)) {
            h_type = t;
            ++h_count;
        }
    }
    CHECK(h_count == 1 && h_type != 0);
    size_t proofs = 0;
    NLPlaceId first_parent = 0;
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
        NLCopyBodyParameters params;
        if (!nl_checked_copy_body_parameters(body, i, &params))
            continue;
        NLCopyBodyView view;
        CHECK(proofs < 2 && nl_checked_copy_body_view(body, i, &view));
        CHECK(nl_checked_copy_body_validate(body, i, &view) == NL_CHECK_OK);
        CHECK(nl_checked_copy_body_parameters_validate(body, i, &params) ==
              NL_CHECK_OK);
        CHECK(nl_copy_same_types(nl_checked_context(body), view.live_exit));
        NLSemanticTypeView prior, current;
        CHECK(nl_semantic_type_view(view.live_exit, h_type, &prior));
        CHECK(
            nl_semantic_type_view(nl_checked_context(body), h_type, &current));
        CHECK(prior.layout_known && current.layout_known &&
              prior.size == current.size &&
              prior.alignment == current.alignment && current.size > 0 &&
              current.alignment > 0);
        CHECK(params.field.nominal != h_type);
        NLSemanticPlaceView parent_before, parent_after, child_after,
            sibling_before, sibling_after;
        CHECK(nl_semantic_place_view(params.entry, params.field.parent,
                                     &parent_before));
        CHECK(nl_semantic_place_view(params.handoff, params.field.parent,
                                     &parent_after));
        CHECK(nl_semantic_place_view(params.handoff, params.field.child,
                                     &child_after));
        CHECK(parent_before.incarnation == parent_after.incarnation);
        CHECK(parent_before.current_fact != parent_after.current_fact);
        CHECK(params.field.index < 2);
        NLPlaceId sibling_id =
            parent_before.fixed_fields[1 - params.field.index];
        CHECK(
            nl_semantic_place_view(params.entry, sibling_id, &sibling_before) &&
            nl_semantic_place_view(params.handoff, sibling_id, &sibling_after));
        CHECK(sibling_before.current_fact == sibling_after.current_fact &&
              sibling_before.current_value == sibling_after.current_value);
        NLSemanticValueView old, updated, sibling;
        CHECK(nl_semantic_value_view(view.live_exit, view.result.value, &old) &&
              nl_semantic_value_view(params.handoff, child_after.current_value,
                                     &updated) &&
              nl_semantic_value_view(params.handoff,
                                     sibling_after.current_value, &sibling));
        CHECK(old.scalar_known && updated.scalar_known && sibling.scalar_known);
        CHECK(params.field.index == expected[4 * proofs] &&
              old.scalar_value == expected[4 * proofs + 1] &&
              updated.scalar_value == expected[4 * proofs + 2] &&
              sibling.scalar_value == expected[4 * proofs + 3]);
        if (proofs == 0)
            first_parent = params.field.parent;
        else
            CHECK(params.field.parent != first_parent);
        /* Adversarial public certificate and body parameter denial. */
        NLCopyBodyView forged_view = view;
        forged_view.function++;
        CHECK(nl_checked_copy_body_validate(body, i, &forged_view) !=
              NL_CHECK_OK);
        forged_view = view;
        forged_view.result.value++;
        CHECK(nl_checked_copy_body_validate(body, i, &forged_view) !=
              NL_CHECK_OK);
        NLCopyBodyParameters forged_params = params;
        forged_params.field.parent_incarnation++;
        CHECK(nl_checked_copy_body_parameters_validate(
                  body, i, &forged_params) != NL_CHECK_OK);
        forged_params = params;
        forged_params.field.index = 1 - params.field.index;
        CHECK(nl_checked_copy_body_parameters_validate(
                  body, i, &forged_params) != NL_CHECK_OK);
        ++proofs;
        printf("R765_COPY call=%zu field=%zu old=%zu new=%zu sibling=%zu "
               "Hlayout=%zu/%zu checked_body=PASS forged=DENIED\n",
               i, params.field.index, old.scalar_value, updated.scalar_value,
               sibling.scalar_value, current.size, current.alignment);
    }
    CHECK(proofs == 2);
    puts("R765 independent two-original source-issued public Copy validation "
         "PASS");
    node_checked_destroy(&state);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 10)
        return 2;
    size_t expected[8];
    for (size_t i = 0; i < 8; ++i) {
        char *end = NULL;
        expected[i] = (size_t)strtoul(argv[i + 2], &end, 10);
        if (!end || *end != '\0')
            return 2;
    }
    return verify(argv[1], expected) ? 0 : 1;
}
