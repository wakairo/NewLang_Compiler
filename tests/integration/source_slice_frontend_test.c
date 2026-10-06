#include "../support/raw_storage_check.h"

/* Fixture supplies target layout/region/domain facts only. All selected typed
 * lifetime transitions and receiving below run through lexed/parsed source. */
static bool source_cycle(bool readable)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *c = f.context;
    CHECK(nl_semantic_set_layout(c, f.copy, 4, 4) == NL_CHECK_OK);
    NLSymbolId allocation, storage, slot, stable, ending;
    NLBackingRegionId region;
    CHECK(raw_allocate(c, 4, 4, readable, true, "allocation", "storage",
                       &allocation, &storage, &region));
    NLCheckedNodeView slot_op;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                                   .operands = {raw_binding(storage)},
                                   .data.slot_target = f.copy},
                  &slot_op));
    CHECK(raw_bind(c, "empty", slot_op.results[0].value, &slot));
    NLSemanticValueView slot_value;
    CHECK(nl_semantic_value_view(c, slot_op.results[0].value, &slot_value));
    const NLPlaceId place = slot_value.slot_place;
    NLSymbolId incoming;
    CHECK(nl_semantic_seed_value(c, "incoming", f.copy, NL_DEPENDENCY_FREE,
                                 &incoming) == NL_CHECK_OK);
    NLScopeId stable_scope, ending_scope;
    CHECK(test_domain_ref(&f, "stable", NL_ACCESS_READ, false, &stable,
                          &stable_scope));
    TestChecked a = {0};
    CHECK(test_run(c, "let p=initialize(empty,incoming,stable)", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &a));
    test_checked_destroy(&a);
    CHECK(nl_semantic_end_scope(c, stable_scope) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending,
                          &ending_scope));
    NLSemanticPlaceView before, after;
    CHECK(nl_semantic_place_view(c, place, &before) && before.live);
    if (readable) {
        /* Ordinary stability and exclusive ending scopes are explicitly
         * alternated by the host. Source calls do not synthesize ordinary
         * children from ending; all receiving/transitions remain parsed. */
        const char *takes[] = {"let(v1,s1)=take(p,ending)",
                               "let(v2,s2)=take(q1,ending2)"};
        const char *initializations[] = {"let q1=initialize(s1,v1,stable2)",
                                         "let fresh=initialize(s2,v2,stable3)"};
        const char *stability_names[] = {"stable2", "stable3"};
        const char *ending_names[] = {"ending2", "ending3"};
        size_t children = 0;
        for (size_t n = 0; n < 2; ++n) {
            CHECK(test_run(c, takes[n], TEST_SOURCE, NL_CHECK_OK, NULL, &a));
            for (size_t i = 1; i <= nl_checked_node_count(a.artifact); ++i) {
                const NLCheckedNodeView *node =
                    nl_checked_node_view(a.artifact, i);
                if (node->reborrow_scope != 0) {
                    NLSemanticScopeView child;
                    CHECK(nl_semantic_scope_view(c, node->reborrow_scope,
                                                 &child) &&
                          !child.active);
                    CHECK(node->value_use == NL_VALUE_REBORROWED);
                    ++children;
                }
            }
            test_checked_destroy(&a);
            CHECK(nl_semantic_end_scope(c, ending_scope) == NL_CHECK_OK);
            CHECK(test_domain_ref(&f, stability_names[n], NL_ACCESS_READ, false,
                                  &stable, &stable_scope));
            CHECK(test_run(c, initializations[n], TEST_SOURCE, NL_CHECK_OK,
                           NULL, &a));
            for (size_t i = 1; i <= nl_checked_node_count(a.artifact); ++i)
                CHECK(nl_checked_node_view(a.artifact, i)->reborrow_scope == 0);
            test_checked_destroy(&a);
            CHECK(nl_semantic_end_scope(c, stable_scope) == NL_CHECK_OK);
            CHECK(test_domain_ref(&f, ending_names[n], NL_ACCESS_READ, true,
                                  &ending, &ending_scope));
        }
        CHECK(children == 2); /* take only; initialize uses ordinary evidence */
        CHECK(nl_semantic_place_view(c, place, &after) && after.live &&
              after.incarnation != before.incarnation &&
              after.placement.region == region);
        CHECK(test_rejected(c, "let(v,s)=take(p,ending3)", TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
        CHECK(test_run(c, "let final_empty=destroy(fresh,ending3)", TEST_SOURCE,
                       NL_CHECK_OK, NULL, &a));
    } else {
        CHECK(test_rejected(c, "let(v,s)=take(p,ending)", TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, "P4-TAKE-BACKING-READ"));
        CHECK(test_run(c, "let final_empty=destroy(p,ending)", TEST_SOURCE,
                       NL_CHECK_OK, NULL, &a));
    }
    test_checked_destroy(&a);
    NLSemanticBindingView outer;
    CHECK(nl_semantic_binding_view(c, ending, &outer) &&
          outer.availability == NL_AVAILABLE);
    CHECK(nl_semantic_place_view(c, place, &after) && !after.live);
    NLSymbolId empty = nl_semantic_find_binding(c, "final_empty");
    NLSemanticBindingView claim;
    NLSemanticValueView value;
    CHECK(nl_semantic_binding_view(c, empty, &claim) &&
          nl_semantic_value_view(c, claim.value, &value));
    CHECK(value.occupancy.region == region && value.occupancy.length == 4);
    NLCheckedNodeView raw;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_ERASE_SLOT,
                                   .operands = {raw_binding(empty)}},
                  &raw));
    CHECK(
        raw_run(c,
                (NLRawOperation){.kind = NL_RAW_DEALLOCATE,
                                 .operands = {raw_binding(allocation),
                                              raw_loose(raw.results[0].value)}},
                NULL));
    NLSemanticBackingView backing;
    CHECK(nl_semantic_backing_view(c, region, &backing) && !backing.live);
    CHECK(nl_semantic_end_scope(c, ending_scope) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "stable_again", NL_ACCESS_READ, false, &stable,
                          NULL));
    CHECK(test_rejected(c, "loan read p using stable_again as dangling {}",
                        TEST_LOAN, NL_CHECK_SEMANTIC_ERROR,
                        "P3-STALE-POINTER"));
    nl_semantic_destroy(c);
    return true;
}

static bool source_aggregate_transfer(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLAggregateField fields[] = {{"left", f.linear}, {"right", f.discardable}};
    NLTypeId pair;
    NLSymbolId x, y;
    CHECK(nl_semantic_register_aggregate(f.context, "OwnedPair", fields, 2,
                                         &pair) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f.context, "x", f.linear, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f.context, "y", f.discardable,
                                 NL_DEPENDENCY_FREE, &y) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "consume", &f.linear, 1, 1,
                                        false, false) == NL_CHECK_OK);
    const char *text = "{let pair=OwnedPair{right:y,left:x};"
                       "let moved=pair; let OwnedPair{left,right}=moved; "
                       "consume(left); right;}";
    TestChecked a = {0};
    CHECK(test_run(f.context, text, TEST_SOURCE, NL_CHECK_OK, NULL, &a));
    CHECK(test_root(&a)->type == 1 && test_root(&a)->result_count == 0);
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(f.context, x, &b) &&
          b.availability == NL_CONSUMED);
    CHECK(nl_semantic_binding_view(f.context, y, &b) &&
          b.availability == NL_CONSUMED);
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(f.context, &snapshot));
    /* No loose/aggregate member orphan remains after complete source cycle. */
    for (size_t i = 1; i <= snapshot.values; ++i) {
        NLSemanticValueView v;
        CHECK(nl_semantic_value_view(f.context, i, &v));
        CHECK(v.carrier != NL_CARRIER_LOOSE &&
              v.carrier != NL_CARRIER_AGGREGATE);
    }
    test_checked_destroy(&a);
    nl_semantic_destroy(f.context);
    return true;
}
int main(void)
{
    return source_cycle(true) && source_cycle(false) &&
                   source_aggregate_transfer()
               ? 0
               : 1;
}
