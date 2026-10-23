#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include "newlang/copy_body.h"
#include <stdlib.h>
static size_t copies, producers, owners, initializations;
static bool walk(const NLCheckedFragment *f, size_t depth)
{
    if (depth > 20)
        return false;
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        NLCopyBodyView copy;
        NLCopyBodyParameters parameters;
        if (nl_checked_copy_body_view(f, id, &copy) &&
            nl_checked_copy_body_parameters(f, id, &parameters)) {
            NLSemanticPlaceView child, sibling;
            NLSemanticValueView old, next, other;
            CHECK(nl_semantic_place_view(parameters.handoff,
                                         parameters.field.child, &child));
            NLSemanticPlaceView parent;
            CHECK(nl_semantic_place_view(parameters.handoff,
                                         parameters.field.parent, &parent));
            CHECK(nl_semantic_place_view(
                parameters.handoff,
                parent.fixed_fields[1 - parameters.field.index], &sibling));
            CHECK(nl_semantic_value_view(copy.live_exit, copy.result.value,
                                         &old));
            CHECK(nl_semantic_value_view(parameters.handoff,
                                         child.current_value, &next));
            CHECK(nl_semantic_value_view(parameters.handoff,
                                         sibling.current_value, &other));
            printf(
                "COPY depth=%zu call=%zu span=%zu:%zu nominal=%zu root=%zu "
                "child=%zu index=%zu incarnation=%zu old=%llu new=%llu "
                "sibling=%llu body_status=%d parameters_status=%d\n",
                depth, id, n->span.start_byte, n->span.end_byte,
                parameters.field.nominal, parameters.field.parent,
                parameters.field.child, parameters.field.index,
                parameters.field.parent_incarnation,
                (unsigned long long)old.scalar_value,
                (unsigned long long)next.scalar_value,
                (unsigned long long)other.scalar_value,
                nl_checked_copy_body_validate(f, id, &copy),
                nl_checked_copy_body_parameters_validate(f, id, &parameters));
            ++copies;
        }
        if (n->kind == NL_CHECKED_INITIALIZE) {
            printf("INITIALIZE depth=%zu node=%zu root=%zu incarnation=%zu "
                   "backing=%zu domain=%zu span=%zu:%zu\n",
                   depth, id, n->lifetime_place, n->lifetime_incarnation,
                   n->lifetime_range.region, n->lifetime_domain,
                   n->span.start_byte, n->span.end_byte);
            ++initializations;
        }
        if (n->producer.entry_proved) {
            NLProducerLocalReturnView local = {0};
            bool view = nl_checked_producer_local_return_view(f, id, &local);
            NLSemanticBindingView binding = {0};
            NLSemanticValueView packet = {0}, allocation = {0}, domain = {0},
                                pointer = {0};
            bool members = view &&
                           nl_semantic_binding_view(local.returned, local.local,
                                                    &binding) &&
                           nl_semantic_value_view(
                               local.returned, local.result.value, &packet) &&
                           packet.field_count == 3 &&
                           nl_semantic_value_view(local.returned,
                                                  packet.fields[0], &pointer) &&
                           nl_semantic_value_view(
                               local.returned, packet.fields[1], &allocation) &&
                           nl_semantic_value_view(local.returned,
                                                  packet.fields[2], &domain);
            printf(
                "PRODUCER depth=%zu call=%zu root=%zu incarnation=%zu "
                "backing=%zu domain=%zu original_valid=%u local_view=%u "
                "local_valid=%u body_same_caller_world=%u "
                "entry_distinct_return=%u construction=%zu consumer=%zu "
                "return_node=%zu local=%zu availability=%d packet=%zu "
                "same_ptr_root=%u same_allocation=%u same_domain=%u\n",
                depth, id, n->producer.root, n->producer.incarnation,
                n->producer.range.region, n->producer.domain,
                nl_checked_producer_valid(f, id), view,
                view && nl_checked_producer_local_return_valid(f, id, &local),
                view && nl_checked_context(local.body) == nl_checked_context(f),
                view && local.entry != local.returned, local.construction,
                local.consumer, local.return_node, local.local,
                (int)binding.availability, local.result.value,
                members && pointer.reference.place == n->producer.root,
                members &&
                    allocation.allocation_region == n->producer.range.region,
                members && domain.domain == n->producer.domain);
            ++producers;
        }
        if (n->owner_call.entry_proved) {
            const NLSemanticContext *entry = nl_checked_owner_entry(f, id);
            NLSemanticPlaceView original = {0};
            NLSemanticValueView a = {0}, d = {0};
            bool exact =
                entry &&
                nl_semantic_place_view(entry, n->owner_call.root, &original) &&
                nl_semantic_value_view(entry, n->owner_call.inputs[1], &a) &&
                nl_semantic_value_view(entry, n->owner_call.inputs[2], &d) &&
                a.allocation_region == original.placement.region &&
                d.domain == original.governing_domain;
            printf("RECEIVER depth=%zu call=%zu root=%zu incarnation=%zu "
                   "backing=%zu domain=%zu post_proved=%u "
                   "entry_original_exact=%u\n",
                   depth, id, n->owner_call.root, n->owner_call.incarnation,
                   n->owner_call.range.region, n->owner_call.domain,
                   n->owner_call.post_proved, exact);
            ++owners;
        }
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t arm = 0; arm < n->item_count; ++arm) {
                const NLCheckedFragment *child =
                    nl_checked_match_arm(f, id, arm);
                NLSemanticSnapshot snapshot;
                CHECK(child && nl_semantic_snapshot(nl_checked_context(child),
                                                    &snapshot));
                bool ended = true;
                for (size_t d = 1; d <= snapshot.domains; ++d) {
                    NLSemanticDomainView domain;
                    CHECK(nl_semantic_domain_view(nl_checked_context(child), d,
                                                  &domain));
                    ended = ended && !domain.live;
                }
                for (size_t r = 1; r <= snapshot.backing_regions; ++r) {
                    NLSemanticBackingView region;
                    CHECK(nl_semantic_backing_view(nl_checked_context(child), r,
                                                   &region));
                    ended = ended && !region.live;
                }
                printf("ARM parent_depth=%zu match=%zu arm=%zu "
                       "backing_regions=%zu domains=%zu all_closed=%u "
                       "captured_closed=%u\n",
                       depth, id, arm, snapshot.backing_regions,
                       snapshot.domains, ended, n->captured_frame_closed);
                CHECK(walk(child, depth + 1));
            }
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body)
            CHECK(walk(body, depth + 1));
    }
    return true;
}
int main(int argc, char **argv)
{
    TestNode source = {0};
    if (argc != 2 || !node_checked_load(argv[1], &source) ||
        !walk(source.entry, 0))
        return 1;
    char *c = NULL;
    size_t bytes = 191;
    NLNodeCStatus backend = nl_checked_c_node(source.entry, &c, &bytes);
    printf("NODE_BACKEND status=%d output_null=%u output_length_unchanged=%u\n",
           backend, c == NULL, bytes == 191);
    free(c);
    printf("SUMMARY Copy_calls=%zu H_producers=%zu owner_receivers=%zu "
           "H_initializations=%zu\n",
           copies, producers, owners, initializations);
    node_checked_destroy(&source);
    return 0;
}
