#include "../../src/copy_body_internal.h" /* read-only baseline diagnosis */
#include "../support/node_checked.h"
#include "newlang/copy_body.h"
static bool expect_valid;
static size_t calls, initialized, closed;
static NLPlaceId roots[2];
static NLDomainId domains[2];
static NLBackingRegionId regions[2];
static NLIncarnationId incarnations[2];
static bool walk(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        NLCopyBodyParameters p;
        NLCopyBodyView b;
        if (nl_checked_copy_body_parameters(f, i, &p)) {
            CHECK(nl_checked_copy_body_view(f, i, &b));
            NLCheckStatus bs = nl_checked_copy_body_validate(f, i, &b),
                          ps = nl_checked_copy_body_parameters_validate(f, i,
                                                                        &p);
            CHECK(bs == (expect_valid ? NL_CHECK_OK
                                      : NL_CHECK_ANALYSIS_PRECISION_LIMIT) &&
                  ps == bs);
            const NLCopyBodyProof *anchor = b.body->copy_body;
            CHECK(nl_sem_validate(nl_checked_context(f)) == NL_CHECK_OK &&
                  nl_sem_validate(anchor->entry) == NL_CHECK_OK &&
                  nl_sem_validate(anchor->exit) == NL_CHECK_OK);
            bool types =
                nl_copy_same_types(nl_checked_context(f), anchor->exit);
            CHECK(types == expect_valid);
            NLSemanticSnapshot snapshot;
            CHECK(nl_semantic_snapshot(nl_checked_context(f), &snapshot));
            size_t hs = 0;
            for (NLTypeId h = 1; h <= snapshot.types; ++h) {
                if (!nl_semantic_recursive_local_type(nl_checked_context(f), h))
                    continue;
                NLSemanticTypeView past, now, ptr;
                NLTypeId pointer;
                CHECK(h != p.field.nominal &&
                      nl_semantic_type_view(b.live_exit, h, &past) &&
                      nl_semantic_type_view(nl_checked_context(f), h, &now) &&
                      nl_semantic_recursive_link_types(nl_checked_context(f), h,
                                                       &pointer) &&
                      nl_semantic_type_view(nl_checked_context(f), pointer,
                                            &ptr) &&
                      ptr.target == h && past.layout_known == expect_valid &&
                      now.layout_known && now.size == 24 && now.alignment == 8);
                printf("COPY call=%zu H=%zu T=%zu type_prefix=%u layout=%u/%u "
                       "size=%zu/%zu align=%zu/%zu body=%d parameters=%d\n",
                       i, h, p.field.nominal, types, past.layout_known,
                       now.layout_known, past.size, now.size, past.alignment,
                       now.alignment, bs, ps);
                ++hs;
            }
            CHECK(hs == 1);
            NLSemanticPlaceView before, after, child, sib0, sib1;
            CHECK(nl_semantic_place_view(p.entry, p.field.parent, &before) &&
                  nl_semantic_place_view(p.handoff, p.field.parent, &after) &&
                  before.incarnation == after.incarnation &&
                  before.current_fact != after.current_fact &&
                  nl_semantic_place_view(p.handoff, p.field.child, &child) &&
                  child.incarnation == p.field.child_incarnation &&
                  nl_semantic_place_view(
                      p.entry, before.fixed_fields[1 - p.field.index], &sib0) &&
                  nl_semantic_place_view(p.handoff,
                                         before.fixed_fields[1 - p.field.index],
                                         &sib1) &&
                  sib0.current_fact == sib1.current_fact &&
                  sib0.current_value == sib1.current_value);
            NLSemanticValueView old, new_value, sibling;
            CHECK(nl_semantic_value_view(b.live_exit, b.result.value, &old) &&
                  old.scalar_known &&
                  nl_semantic_value_view(p.handoff, child.current_value,
                                         &new_value) &&
                  new_value.scalar_known &&
                  nl_semantic_value_view(p.handoff, sib1.current_value,
                                         &sibling) &&
                  sibling.scalar_known);
            const NLCheckedNodeView *loan = nl_checked_node_view(f, p.loan);
            NLSemanticScopeView active, ended;
            CHECK(
                loan &&
                nl_semantic_scope_view(p.handoff, loan->loan.scope, &active) &&
                active.active &&
                nl_semantic_scope_view(nl_checked_context(f), loan->loan.scope,
                                       &ended) &&
                !ended.active);
            printf("ORIGINAL root=%zu child=%zu index=%zu incarnation=%zu "
                   "old=%zu new=%zu sibling=%zu scope_closed=PASS\n",
                   p.field.parent, p.field.child, p.field.index,
                   before.incarnation, old.scalar_value, new_value.scalar_value,
                   sibling.scalar_value);
            if (calls == 0) {
                NLSymbolId saved = 0;
                for (size_t k = 1; k <= nl_checked_node_count(f); ++k) {
                    const NLCheckedNodeView *binding =
                        nl_checked_node_view(f, k);
                    if (binding->kind == NL_CHECKED_BINDING &&
                        binding->initializer == p.loan)
                        saved = binding->symbol;
                }
                CHECK(saved);
                size_t consumers = 0;
                for (size_t k = 1; k <= nl_checked_node_count(f); ++k) {
                    const NLCheckedNodeView *op = nl_checked_node_view(f, k);
                    if (op->argument_count != 2)
                        continue;
                    const NLCheckedNodeView *ref =
                        nl_checked_node_view(f, op->first_argument);
                    const NLCheckedNodeView *arg =
                        ref ? nl_checked_node_view(f, ref->next_argument)
                            : NULL;
                    if (!arg || arg->kind != NL_CHECKED_IDENTIFIER ||
                        arg->symbol != saved)
                        continue;
                    NLPlaceId destination = 0;
                    NLCopyBodyParameters q;
                    if (op->kind == NL_CHECKED_REPLACE)
                        destination = op->field.parent;
                    else if (nl_checked_copy_body_parameters(f, k, &q))
                        destination = q.field.parent;
                    if (destination) {
                        CHECK(destination != p.field.parent);
                        ++consumers;
                    }
                }
                CHECK(consumers == 1);
                printf("CALLER returned_Copy_binding=%zu "
                       "distinct_original_consumers=%zu source_graph=PASS\n",
                       saved, consumers);
            }
            ++calls;
        }
        if (n->kind == NL_CHECKED_INITIALIZE) {
            CHECK(initialized < 2);
            roots[initialized] = n->lifetime_place;
            domains[initialized] = n->lifetime_domain;
            regions[initialized] = n->backing;
            incarnations[initialized++] = n->lifetime_incarnation;
        }
        if (n->kind == NL_CHECKED_MATCH && n->captured_frame_closed) {
            CHECK(nl_checked_captured_post(f, i));
            for (size_t a = 0; a < n->item_count; ++a) {
                const NLSemanticContext *world =
                    nl_checked_context(nl_checked_match_arm(f, i, a));
                NLSemanticSnapshot snapshot;
                CHECK(nl_semantic_snapshot(world, &snapshot));
                for (size_t region = 1; region <= snapshot.backing_regions;
                     ++region) {
                    NLSemanticBackingView v;
                    CHECK(nl_semantic_backing_view(world, region, &v) &&
                          !v.live);
                }
                for (size_t d = 1; d <= snapshot.domains; ++d) {
                    NLSemanticDomainView v;
                    CHECK(nl_semantic_domain_view(world, d, &v) && !v.live);
                }
                printf("SEMANTIC cleanup_arm=%zu original_regions=%zu "
                       "all_closed=PASS\n",
                       a, snapshot.backing_regions);
            }
            ++closed;
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body)
            CHECK(walk(body));
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < n->item_count; ++a)
                CHECK(walk(nl_checked_match_arm(f, i, a)));
    }
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 1;
    expect_valid = strcmp(argv[1], "valid") == 0;
    TestNode n = {0};
    if (!node_checked_load(argv[2], &n) || !walk(n.entry))
        return 1;
    if (initialized != 2 || !closed || !calls || roots[0] == roots[1] ||
        domains[0] == domains[1] || regions[0] == regions[1] ||
        incarnations[0] == incarnations[1])
        return 1;
    printf("SOURCE_OWNED Copy_calls=%zu H_originals=%zu legacy_closed=%zu "
           "T_certificate_only NO_MIXED_NATIVE\n",
           calls, initialized, closed);
    node_checked_destroy(&n);
    return 0;
}
