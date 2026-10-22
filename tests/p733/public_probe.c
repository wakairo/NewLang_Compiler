#include "../support/node_checked.h"
#include "newlang/captured_closure.h"
static size_t producers, closures, receivers, closed_legacy;
static bool walk(const NLCheckedFragment *f)
{
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n->producer.entry_proved) {
            NLProducerLocalReturnView v;
            CHECK(nl_checked_producer_local_return_view(f, id, &v) &&
                  nl_checked_producer_local_return_valid(f, id, &v));
            NLSemanticBindingView binding;
            NLSemanticValueView packet, allocation, domain, ptr;
            NLSemanticTypeView type;
            NLSemanticPlaceView before, after, local;
            CHECK(
                nl_semantic_binding_view(v.returned, v.local, &binding) &&
                binding.availability == NL_CONSUMED &&
                binding.value == v.result.value &&
                nl_semantic_type_view(v.returned, v.result.type, &type) &&
                !type.is_copy && !type.is_discardable &&
                nl_semantic_value_view(v.returned, v.result.value, &packet) &&
                packet.field_count == 3 &&
                nl_semantic_value_view(v.returned, packet.fields[0], &ptr) &&
                nl_semantic_value_view(v.returned, packet.fields[1],
                                       &allocation) &&
                nl_semantic_value_view(v.returned, packet.fields[2], &domain) &&
                allocation.allocation_region == n->producer.range.region &&
                domain.domain == n->producer.domain &&
                ptr.reference.place == n->producer.root &&
                ptr.reference.incarnation == n->producer.incarnation &&
                packet.fields[1] == n->producer.inputs[2] &&
                packet.fields[2] == n->producer.inputs[3] &&
                nl_semantic_place_view(v.entry, n->producer.root, &before) &&
                nl_semantic_place_view(v.returned, n->producer.root, &after) &&
                before.live && after.live &&
                before.incarnation == after.incarnation &&
                before.current_fact == after.current_fact &&
                before.current_value == after.current_value &&
                before.placement.region == after.placement.region &&
                nl_semantic_place_view(v.returned, binding.place, &local) &&
                !local.live);
            printf("LOCAL call=%zu binding=%zu construction=%zu consume=%zu "
                   "return=%zu package=%zu Allocation=%zu DomainValue=%zu "
                   "root=%zu incarnation=%zu region=%zu Domain=%zu "
                   "validators=PASS\n",
                   id, v.local, v.construction, v.consumer, v.return_node,
                   v.result.value, packet.fields[1], packet.fields[2],
                   n->producer.root, n->producer.incarnation,
                   n->producer.range.region, n->producer.domain);
            ++producers;
        }
        NLCapturedClosureView closure;
        if (nl_checked_captured_closure_view(f, id, &closure)) {
            CHECK(nl_checked_captured_closure_validate(f, id) == NL_CHECK_OK);
            printf("CLOSURE originals=%zu release_worlds0=%zu "
                   "release_worlds1=%zu release_worlds2=%zu public=PASS\n",
                   closure.count, closure.release_worlds[0],
                   closure.release_worlds[1], closure.release_worlds[2]);
            ++closures;
        }
        if (n->kind == NL_CHECKED_MATCH && n->captured_frame_closed) {
            CHECK(nl_checked_captured_post(f, id));
            for (size_t arm = 0; arm < n->item_count; ++arm) {
                const NLSemanticContext *c =
                    nl_checked_context(nl_checked_match_arm(f, id, arm));
                NLSemanticSnapshot snapshot;
                CHECK(nl_semantic_snapshot(c, &snapshot));
                for (size_t r = 1; r <= snapshot.backing_regions; ++r) {
                    NLSemanticBackingView backing;
                    CHECK(nl_semantic_backing_view(c, r, &backing) &&
                          !backing.live);
                }
                for (size_t d = 1; d <= snapshot.domains; ++d) {
                    NLSemanticDomainView domain;
                    CHECK(nl_semantic_domain_view(c, d, &domain) &&
                          !domain.live);
                }
                printf("CLEANUP legacy_arm=%zu regions=%zu all_closed=PASS\n",
                       arm, snapshot.backing_regions);
            }
            ++closed_legacy;
        }
        if (n->owner_call.entry_proved) {
            const NLSemanticContext *entry = nl_checked_owner_entry(f, id);
            NLSemanticValueView a, d, p;
            NLSemanticPlaceView root;
            CHECK(entry && n->owner_call.post_proved &&
                  nl_semantic_value_view(entry, n->owner_call.inputs[0], &p) &&
                  nl_semantic_value_view(entry, n->owner_call.inputs[1], &a) &&
                  nl_semantic_value_view(entry, n->owner_call.inputs[2], &d) &&
                  nl_semantic_place_view(entry, n->owner_call.root, &root) &&
                  root.live && p.reference.place == n->owner_call.root &&
                  p.reference.incarnation == n->owner_call.incarnation &&
                  a.allocation_region == root.placement.region &&
                  d.domain == root.governing_domain);
            ++receivers;
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body)
            CHECK(walk(body));
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t i = 0; i < n->item_count; ++i)
                CHECK(walk(nl_checked_match_arm(f, id, i)));
    }
    return true;
}
int main(int argc, char **argv)
{
    TestNode n = {0};
    if (argc != 2 || !node_checked_load(argv[1], &n) || !walk(n.entry) ||
        producers != 1 || receivers != 1 || closed_legacy != 1)
        return 1;
    printf("PUBLIC producer=%zu recipient=%zu legacy_closed=%zu "
           "generic_closure_views=%zu\n",
           producers, receivers, closed_legacy, closures);
    node_checked_destroy(&n);
    return 0;
}
