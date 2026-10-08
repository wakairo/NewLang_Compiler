#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
#include <stdlib.h>

static NLCheckedNodeId match(const NLCheckedFragment *f)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_MATCH)
            return i;
    return 0;
}
static size_t count(const NLCheckedFragment *f, NLCheckedKind kind)
{
    size_t n = 0;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i)
        n += nl_checked_node_view(f, i)->kind == kind;
    return n;
}
static const NLCheckedNodeView *operation(const NLCheckedFragment *f,
                                          NLCheckedKind kind)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == kind)
            return nl_checked_node_view(f, i);
    return NULL;
}
static bool evidence(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n)); /* destroys original AST/source */
    const NLCheckedFragment *body = nl_checked_call_body(n.entry, 1);
    CHECK(body != NULL);
    NLCheckedNodeId outer_id = match(body);
    CHECK(outer_id != 0);
    const NLCheckedNodeView *outer = nl_checked_node_view(body, outer_id);
    CHECK(outer->normal_frame_unchanged && !outer->captured_frame_closed);
    const NLCheckedFragment *first_none =
        nl_checked_match_arm(body, outer_id, 0);
    const NLCheckedFragment *first_some =
        nl_checked_match_arm(body, outer_id, 1);
    CHECK(first_none != NULL && first_some != NULL);
    CHECK(count(first_none, NL_CHECKED_INITIALIZE) == 0 &&
          count(first_none, NL_CHECKED_DEALLOCATE) == 0 &&
          match(first_none) == 0);
    const NLSemanticContext *zero = nl_checked_context(first_none);
    CHECK(zero->region_count == 0 && zero->domain_count == 0);
    NLCheckedNodeId inner_id = match(first_some);
    CHECK(inner_id != 0);
    const NLCheckedNodeView *inner = nl_checked_node_view(first_some, inner_id);
    CHECK(!inner->normal_frame_unchanged && inner->captured_frame_closed &&
          inner->normal_arms == 2);
    const NLSemanticContext *joined =
        nl_checked_captured_post(first_some, inner_id);
    CHECK(joined != NULL);
    CHECK(joined->region_count == 1 && !joined->regions[0].view.live &&
          joined->domain_count == 1 && !joined->domains[0].live &&
          !joined->places[inner->captured_root - 1].live &&
          joined->places[inner->captured_root - 1].incarnation ==
              inner->captured_incarnation);
    CHECK(joined->bindings[inner->captured_allocation - 1].view.availability ==
              NL_CONSUMED &&
          joined->bindings[inner->captured_domain_binding - 1]
                  .view.availability == NL_CONSUMED);
    const NLCheckedNodeView *head =
        operation(first_some, NL_CHECKED_INITIALIZE);
    CHECK(head != NULL && head->lifetime_place == inner->captured_root &&
          head->lifetime_incarnation == inner->captured_incarnation &&
          head->lifetime_domain == inner->captured_domain &&
          head->backing == inner->captured_backing);
    const NLCheckedFragment *arms[2] = {
        nl_checked_match_arm(first_some, inner_id, 0),
        nl_checked_match_arm(first_some, inner_id, 1)};
    for (size_t a = 0; a < 2; ++a) {
        CHECK(arms[a] != NULL);
        const NLSemanticContext *c = nl_checked_context(arms[a]);
        const NLCheckedNodeView *root =
            nl_checked_node_view(arms[a], nl_checked_root(arms[a]));
        CHECK(!root->normal_frame_unchanged);
        CHECK(c->region_count == a + 1 && c->domain_count == a + 1);
        CHECK(count(arms[a], NL_CHECKED_INITIALIZE) == a &&
              count(arms[a], NL_CHECKED_DESTROY) == a + 1 &&
              count(arms[a], NL_CHECKED_DEALLOCATE) == a + 1);
        CHECK(nl_allocated_post_matches(joined, c, root));
        for (size_t r = 0; r < c->region_count; ++r)
            CHECK(!c->regions[r].view.live && !c->domains[r].live);
        for (size_t b = 0; b < c->binding_count; ++b)
            CHECK(c->bindings[b].view.availability == NL_CONSUMED ||
                  c->types[c->bindings[b].view.type - 1].view.is_discardable);
        const NLCheckedNodeView *grant =
            nl_checked_node_view(arms[a], root->initializer);
        CHECK(grant != NULL && grant->allocation_success == (a == 1));
        if (a == 0)
            CHECK(grant->backing == 0 && grant->allocation_authority == 0 &&
                  grant->storage_authority == 0);
        else {
            const NLCheckedNodeView *tail =
                operation(arms[a], NL_CHECKED_INITIALIZE);
            CHECK(tail->type == head->type &&
                  tail->lifetime_place != head->lifetime_place &&
                  tail->lifetime_incarnation != head->lifetime_incarnation &&
                  tail->lifetime_domain != head->lifetime_domain &&
                  tail->backing != head->backing &&
                  grant->backing == tail->backing);
        }
        /* SUPPORTING corruption attacks on an owned clone, not source proof.
         * Coincident numeric suffix IDs cannot satisfy ancestor provenance. */
        for (size_t attack = 0; attack < 10; ++attack) {
            NLSemanticContext *bad = NULL;
            CHECK(nl_sem_clone(c, &bad) == NL_CHECK_OK);
            NLPlaceId p = inner->captured_root;
            NLValueId owner =
                c->bindings[inner->captured_allocation - 1].view.value;
            switch (attack) {
            case 0:
                ++bad->places[p - 1].incarnation;
                break;
            case 1:
                bad->bindings[inner->captured_allocation - 1]
                    .view.availability = NL_AVAILABLE;
                break;
            case 2:
                bad->values[owner - 1].dependencies = NL_DEPENDENCIES_UNKNOWN;
                break;
            case 3:
                ++bad->domains[inner->captured_domain - 1].value;
                break;
            case 4:
                bad->regions[inner->captured_backing - 1].view.address_known =
                    true;
                break;
            case 5:
                bad->places[p - 1].current_fact = 1;
                break;
            case 6:
                bad->places[p - 1].payload_occurrence = 1;
                break;
            case 7:
                bad->values[owner - 1].allocation_region = 2;
                break;
            case 8:
                bad->values[owner - 1].carrier = NL_CARRIER_LOOSE;
                break;
            case 9:
                bad->regions[c->region_count - 1].view.live = true;
                break;
            }
            CHECK(!nl_allocated_post_matches(joined, bad, root));
            nl_semantic_destroy(bad);
        }
        NLCheckedNodeView forged = *root;
        forged.result_count = 1;
        forged.results[0] = (NLCheckedResult){1, SIZE_MAX};
        CHECK(!nl_allocated_post_matches(joined, c, &forged));
        CHECK(nl_allocated_post_matches(joined, c, root));
    }
    /* Same numeric suffix ID, different origin and meaning. Neither suffix
     * appears in the ancestor-only proof/caller prefix. */
    const NLSemanticContext *left = nl_checked_context(arms[0]);
    const NLSemanticContext *right = nl_checked_context(arms[1]);
    CHECK(left != right && left->value_count > joined->value_count &&
          right->value_count > joined->value_count);
    CHECK(left->values[joined->value_count].type !=
          right->values[joined->value_count].type);
    CHECK(nl_checked_captured_post(first_some, inner_id + 1) == NULL);
    NLSemanticContext *not_published = NULL;
    NLCheckedNodeView untouched = {.captured_root = 777};
    CHECK(nl_allocated_closed_prefix(joined, &not_published, &untouched) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(not_published == NULL && untouched.captured_root == 777);
    char *out = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(n.entry, &out, &length) == NL_NODE_C_UNSUPPORTED &&
          out == NULL && length == 777);
    /* Malformed certificate is not a backend capability. New two-site
     * artifacts remain unsupported, with staged output untouched. */
    NLCheckedNodeView *malformed =
        &((NLCheckedFragment *)first_some)->nodes[inner_id - 1];
    const NLCheckedNodeView saved = *malformed;
    malformed->normal_frame_unchanged = true;
    malformed->captured_backing = 2;
    malformed->captured_root = SIZE_MAX;
    CHECK(nl_checked_c_node(n.entry, &out, &length) == NL_NODE_C_UNSUPPORTED &&
          out == NULL && length == 777);
    *malformed = saved;
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && evidence(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
