#include "../support/raw_storage_check.h"

static bool cycle(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    CHECK(nl_semantic_set_layout(c, f.copy, 4, 4) == NL_CHECK_OK);
    NLSymbolId a, s, empty, tail, stable, ending, incoming, p;
    NLBackingRegionId region;
    NLScopeId stability_scope, ending_scope;
    CHECK(raw_allocate(c, 8, 8, true, true, "a", "s", &a, &s, &region));
    NLCheckedNodeView pieces, slot;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_SPLIT,
                                   .operands = {raw_binding(s)},
                                   .data.split_at = {true, 4}},
                  &pieces));
    CHECK(raw_bind(c, "tail", pieces.results[1].value, &tail));
    CHECK(raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                         .operands = {raw_loose(pieces.results[0].value)},
                         .data.slot_target = f.copy},
        &slot));
    CHECK(raw_bind(c, "empty", slot.results[0].value, &empty));
    NLSemanticValueView slot_value;
    CHECK(nl_semantic_value_view(c, slot.results[0].value, &slot_value));
    const NLPlaceId place = slot_value.slot_place;
    CHECK(slot_value.occupancy.region == region &&
          slot_value.occupancy.start == 0 && slot_value.occupancy.length == 4);
    NLSemanticTypeView slot_type;
    CHECK(nl_semantic_type_view(c, slot.type, &slot_type) &&
          !slot_type.is_copy && !slot_type.is_discardable);
    CHECK(test_domain_ref(&f, "stable", NL_ACCESS_READ, false, &stable,
                          &stability_scope));
    CHECK(nl_semantic_seed_value(c, "incoming", f.copy, NL_DEPENDENCY_FREE,
                                 &incoming) == NL_CHECK_OK);
    TestChecked checked = {0};
    CHECK(test_run(c, "initialize(empty,incoming,stable)", TEST_EXPRESSION,
                   NL_CHECK_OK, NULL, &checked));
    CHECK(raw_bind(c, "p", test_root(&checked)->results[0].value, &p));
    test_checked_destroy(&checked);
    NLSemanticPlaceView root;
    CHECK(nl_semantic_place_view(c, place, &root) && root.live &&
          root.placement.region == region);
    const NLIncarnationId first = root.incarnation;
    CHECK(nl_semantic_end_scope(c, stability_scope) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending,
                          &ending_scope));
    CHECK(test_run(c, "take(p,ending)", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &checked));
    CHECK(test_root(&checked)->result_count == 2);
    NLSymbolId returned;
    CHECK(raw_bind(c, "returned", test_root(&checked)->results[0].value,
                   &returned));
    CHECK(raw_bind(c, "empty_again", test_root(&checked)->results[1].value,
                   &empty));
    test_checked_destroy(&checked);
    CHECK(nl_semantic_place_view(c, place, &root) && !root.live &&
          root.placement.region == 0);
    NLSemanticBindingView eb;
    CHECK(nl_semantic_binding_view(c, ending, &eb) &&
          eb.availability == NL_AVAILABLE);
    /* Reinitialize the same range without reviving the old token. */
    CHECK(nl_semantic_end_scope(c, ending_scope) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "stable2", NL_ACCESS_READ, false, &stable,
                          &stability_scope));
    CHECK(test_run(c, "initialize(empty_again,returned,stable2)",
                   TEST_EXPRESSION, NL_CHECK_OK, NULL, &checked));
    CHECK(raw_bind(c, "fresh", test_root(&checked)->results[0].value, &p));
    test_checked_destroy(&checked);
    CHECK(nl_semantic_place_view(c, place, &root) && root.live &&
          root.incarnation != first && root.placement.region == region);
    CHECK(test_rejected(c, "loan read p using stable2 as invalid {}", TEST_LOAN,
                        NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
    CHECK(nl_semantic_end_scope(c, stability_scope) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "ending2", NL_ACCESS_READ, true, &ending,
                          &ending_scope));
    CHECK(test_run(c, "destroy(fresh,ending2)", TEST_EXPRESSION, NL_CHECK_OK,
                   NULL, &checked));
    CHECK(raw_bind(c, "final_empty", test_root(&checked)->results[0].value,
                   &empty));
    test_checked_destroy(&checked);
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(c, empty, &b));
    CHECK(nl_semantic_value_view(c, b.value, &slot_value) &&
          slot_value.occupancy.region == region &&
          slot_value.occupancy.length == 4);
    NLCheckedNodeView storage, merged;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_ERASE_SLOT,
                                   .operands = {raw_binding(empty)}},
                  &storage));
    CHECK(raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_MERGE,
                         .operands = {raw_binding(tail),
                                      raw_loose(storage.results[0].value)}},
        &merged));
    NLSymbolId full, ref;
    NLScopeId read_scope;
    CHECK(raw_bind(c, "full", merged.results[0].value, &full));
    CHECK(raw_ref(c, "raw", full, &ref, &read_scope));
    CHECK(raw_unspecified(c, ref, 0)); /* typed end grants no Definedness */
    CHECK(nl_semantic_end_scope(c, read_scope) == NL_CHECK_OK);
    CHECK(raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_DEALLOCATE,
                         .operands = {raw_binding(a), raw_binding(full)}},
        NULL));
    NLSemanticBackingView backing;
    CHECK(nl_semantic_backing_view(c, region, &backing) && !backing.live);
    CHECK(nl_semantic_end_scope(c, ending_scope) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "stable3", NL_ACCESS_READ, false, &stable, NULL));
    CHECK(test_rejected(c, "loan read fresh using stable3 as dangling {}",
                        TEST_LOAN, NL_CHECK_SEMANTIC_ERROR,
                        "P3-STALE-POINTER"));
    /* Numeric address reuse by a fresh allocation also does not revive p. */
    CHECK(
        raw_allocate(c, 8, 8, true, true, "next_a", "next_s", &a, &s, &region));
    CHECK(test_rejected(c, "loan read p using stable3 as reused {}", TEST_LOAN,
                        NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
    nl_semantic_destroy(c);
    return true;
}

/* Value-owned nested Storage range differs from the root's place-owned range.
 * Taking Storage forwards its claim while releasing only root placement. */
static bool nested_storage(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    const NLTypeId storage_type = nl_semantic_core_type(c, NL_TYPE_STORAGE);
    CHECK(nl_semantic_set_layout(c, storage_type, 8, 8) == NL_CHECK_OK);
    NLSymbolId a, s, b, t, empty, stable, ending, p;
    NLBackingRegionId outer, inner;
    CHECK(raw_allocate(c, 8, 8, true, true, "a", "s", &a, &s, &outer));
    CHECK(raw_allocate(c, 4, 4, true, true, "b", "t", &b, &t, &inner));
    NLCheckedNodeView slot;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                                   .operands = {raw_binding(s)},
                                   .data.slot_target = storage_type},
                  &slot));
    CHECK(raw_bind(c, "empty", slot.results[0].value, &empty));
    NLScopeId scope;
    CHECK(
        test_domain_ref(&f, "stable", NL_ACCESS_READ, false, &stable, &scope));
    TestChecked checked = {0};
    CHECK(test_run(c, "initialize(empty,t,stable)", TEST_EXPRESSION,
                   NL_CHECK_OK, NULL, &checked));
    CHECK(raw_bind(c, "p", test_root(&checked)->results[0].value, &p));
    test_checked_destroy(&checked);
    NLSemanticBindingView token;
    NLSemanticValueView ptr, payload;
    NLSemanticPlaceView root;
    CHECK(nl_semantic_binding_view(c, p, &token) &&
          nl_semantic_value_view(c, token.value, &ptr));
    CHECK(nl_semantic_place_view(c, ptr.reference.place, &root) &&
          root.placement.region == outer);
    CHECK(nl_semantic_value_view(c, root.current_value, &payload) &&
          payload.occupancy.region == inner);
    CHECK(nl_semantic_end_scope(c, scope) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending, &scope));
    CHECK(test_rejected(c, "destroy(p,ending)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-DISCARDABLE-REQUIRED"));
    CHECK(test_run(c, "take(p,ending)", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &checked));
    const NLCheckedNodeView result = *test_root(&checked);
    test_checked_destroy(&checked);
    CHECK(nl_semantic_value_view(c, result.results[0].value, &payload) &&
          payload.occupancy.region == inner);
    CHECK(nl_semantic_value_view(c, result.results[1].value, &payload) &&
          payload.occupancy.region == outer);
    CHECK(raw_run(
        c,
        (NLRawOperation){
            .kind = NL_RAW_DEALLOCATE,
            .operands = {raw_binding(b), raw_loose(result.results[0].value)}},
        NULL));
    NLCheckedNodeView raw;
    CHECK(raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_ERASE_SLOT,
                         .operands = {raw_loose(result.results[1].value)}},
        &raw));
    CHECK(
        raw_run(c,
                (NLRawOperation){.kind = NL_RAW_DEALLOCATE,
                                 .operands = {raw_binding(a),
                                              raw_loose(raw.results[0].value)}},
                NULL));
    nl_semantic_destroy(c);
    return true;
}
int main(void)
{
    return cycle() && nested_storage() ? 0 : 1;
}
