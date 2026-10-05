#include "../support/raw_storage_check.h"
#include <stdlib.h>

/* Test-only Linux failure injection covers every production malloc/realloc. */
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t size)
{
    if (injecting && allocation_index++ == fail_at) {
        return NULL;
    }
    return __real_malloc(size);
}
void *__wrap_realloc(void *p, size_t size)
{
    if (injecting && allocation_index++ == fail_at) {
        return NULL;
    }
    return __real_realloc(p, size);
}

static bool claim_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    NLSymbolId a, s, b, t;
    NLBackingRegionId region, other;
    CHECK(raw_allocate(c, 8, 8, true, true, "a", "s", &a, &s, &region));
    CHECK(raw_allocate(c, 8, 8, true, true, "b", "t", &b, &t, &other));
    CHECK(region != other);
    NLSemanticBindingView binding;
    CHECK(nl_semantic_binding_view(c, a, &binding));
    NLSemanticTypeView type;
    CHECK(nl_semantic_type_view(c, binding.type, &type) &&
          type.kind == NL_TYPE_ALLOCATION && !type.is_copy &&
          !type.is_discardable);
    CHECK(nl_semantic_binding_view(c, s, &binding));
    CHECK(nl_semantic_type_view(c, binding.type, &type) &&
          type.kind == NL_TYPE_STORAGE && !type.is_copy &&
          !type.is_discardable);
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_DEALLOCATE,
                         .operands = {raw_binding(b), raw_binding(s)}},
        NL_CHECK_SEMANTIC_ERROR, "P4-ALLOCATION-MISMATCH"));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_MERGE,
                         .operands = {raw_binding(s), raw_binding(t)}},
        NL_CHECK_SEMANTIC_ERROR, "P4-REGION-MISMATCH"));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_MERGE,
                         .operands = {raw_binding(s), raw_binding(s)}},
        NL_CHECK_SEMANTIC_ERROR, "P3-USE-AFTER-CONSUME"));
    CHECK(nl_semantic_set_layout(c, f.copy, 4, 4) == NL_CHECK_OK);
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                                        .operands = {raw_binding(s)},
                                        .data.slot_target = f.copy},
                       NL_CHECK_SEMANTIC_ERROR, "P4-SLOT-SIZE"));
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                                        .operands = {raw_binding(s)},
                                        .data.slot_target = f.linear},
                       NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                       "P4-LAYOUT-PRECISION"));
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_SPLIT,
                                        .operands = {raw_binding(s)},
                                        .data.split_at = {true, 0}},
                       NL_CHECK_SEMANTIC_ERROR, "P4-SPLIT-ENDPOINT"));
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_SPLIT,
                                        .operands = {raw_binding(s)},
                                        .data.split_at = {true, 8}},
                       NL_CHECK_SEMANTIC_ERROR, "P4-SPLIT-ENDPOINT"));
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_SPLIT,
                                        .operands = {raw_binding(s)},
                                        .data.split_at = {true, 9}},
                       NL_CHECK_SEMANTIC_ERROR, "P4-SPLIT-BOUNDS"));
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_SPLIT,
                                        .operands = {raw_binding(s)},
                                        .data.split_at = {false, 0}},
                       NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                       "P4-DYNAMIC-PRECISION"));
    NLCheckedNodeView parts, rest;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_SPLIT,
                                   .operands = {raw_binding(s)},
                                   .data.split_at = {true, 1}},
                  &parts));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_MERGE,
                         .operands = {raw_loose(parts.results[0].value),
                                      raw_loose(parts.results[0].value)}},
        NL_CHECK_SEMANTIC_ERROR, "P4-LOOSE-RESPONSIBILITY"));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){
            .kind = NL_RAW_DEALLOCATE,
            .operands = {raw_binding(a), raw_loose(parts.results[0].value)}},
        NL_CHECK_SEMANTIC_ERROR, "P4-FULL-RANGE-REQUIRED"));
    CHECK(raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_SPLIT,
                         .operands = {raw_loose(parts.results[1].value)},
                         .data.split_at = {true, 4}},
        &rest));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                         .operands = {raw_loose(rest.results[0].value)},
                         .data.slot_target = f.copy},
        NL_CHECK_SEMANTIC_ERROR, "P4-SLOT-ALIGNMENT"));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_MERGE,
                         .operands = {raw_loose(parts.results[0].value),
                                      raw_loose(rest.results[1].value)}},
        NL_CHECK_SEMANTIC_ERROR, "P4-NONADJACENT-MERGE"));
    NLCheckedNodeView merged;
    CHECK(raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_MERGE,
                         .operands = {raw_loose(rest.results[1].value),
                                      raw_loose(rest.results[0].value)}},
        &merged));
    CHECK(raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_MERGE,
                         .operands = {raw_loose(merged.results[0].value),
                                      raw_loose(parts.results[0].value)}},
        &merged));
    TestChecked moved = {0};
    CHECK(
        test_run(c, "let moved = a", TEST_BINDING, NL_CHECK_OK, NULL, &moved));
    a = test_root(&moved)->symbol;
    test_checked_destroy(&moved);
    NLSemanticValueView av;
    CHECK(nl_semantic_binding_view(c, a, &binding));
    CHECK(nl_semantic_value_view(c, binding.value, &av) &&
          av.allocation_region == region);
    CHECK(raw_run(
        c,
        (NLRawOperation){
            .kind = NL_RAW_DEALLOCATE,
            .operands = {raw_binding(a), raw_loose(merged.results[0].value)}},
        NULL));
    CHECK(
        raw_run(c,
                (NLRawOperation){.kind = NL_RAW_DEALLOCATE,
                                 .operands = {raw_binding(b), raw_binding(t)}},
                NULL));
    NLSemanticBackingView backing;
    CHECK(nl_semantic_backing_view(c, region, &backing) && !backing.live);
    nl_semantic_destroy(c);
    return true;
}

static bool byte_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    const NLTypeId byte = nl_semantic_core_type(c, NL_TYPE_BYTE),
                   u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    CHECK(byte != u8);
    NLSemanticTypeView type;
    CHECK(nl_semantic_type_view(c, byte, &type) && type.is_copy &&
          type.is_discardable && type.layout_known && type.size == 1 &&
          type.alignment == 1);
    for (size_t value = 0; value < 256; ++value) {
        NLCheckedNodeView r;
        CHECK(raw_run(c,
                      (NLRawOperation){.kind = NL_RAW_U8_TO_BYTE,
                                       .data.conversion = {u8, true, value}},
                      &r));
        CHECK(r.has_scalar_result && r.result_count == 0 &&
              r.scalar_result.type == byte && r.scalar_result.known &&
              r.scalar_result.value == value);
        CHECK(raw_run(c,
                      (NLRawOperation){.kind = NL_RAW_BYTE_TO_U8,
                                       .data.conversion = r.scalar_result},
                      &r));
        CHECK(r.scalar_result.type == u8 && r.scalar_result.known &&
              r.scalar_result.value == value);
    }
    NLCheckedNodeView r;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_BYTE_TO_U8,
                                   .data.conversion = {byte, false, 255}},
                  &r));
    CHECK(!r.scalar_result.known && r.scalar_result.value == 0);
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_U8_TO_BYTE,
                                        .data.conversion = {u8, true, 256}},
                       NL_CHECK_SEMANTIC_ERROR, "P4-OCTET-DOMAIN"));
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_U8_TO_BYTE,
                                        .data.conversion = {byte, true, 0}},
                       NL_CHECK_SEMANTIC_ERROR, "P4-SCALAR-TYPE"));
    NLSymbolId a, s, ref;
    NLBackingRegionId region;
    CHECK(raw_allocate(c, 4, 4, true, true, "a", "s", &a, &s, &region));
    CHECK(raw_ref(c, "r", s, &ref, NULL));
    CHECK(raw_unspecified(c, ref, 0));
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(c, &before));
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(c, s, &b));
    NLSemanticPlaceView p;
    CHECK(nl_semantic_place_view(c, b.place, &p));
    CHECK(raw_write(c, ref, 0, true, 255));
    CHECK(raw_read(c, ref, 0, true, 255));
    CHECK(raw_unspecified(c, ref, 1));
    CHECK(raw_write(c, ref, 1, false, 0));
    CHECK(raw_read(c, ref, 1, false, 0));
    CHECK(raw_write(c, ref, 2, true, 0));
    CHECK(raw_write(c, ref, 3, true, 0));
    CHECK(nl_semantic_snapshot(c, &after) && before.places == after.places &&
          before.domains == after.domains &&
          before.last_incarnation == after.last_incarnation &&
          before.last_value_fact == after.last_value_fact);
    NLSemanticPlaceView current;
    CHECK(nl_semantic_place_view(c, b.place, &current) &&
          test_place_equal(p, current));
    /* 255 is a valid byte representation on the native octet profile, yet
     * the raw write did not start a typed byte lifetime or install a package.
     */
    for (size_t i = before.values; i < after.values; ++i) {
        NLSemanticValueView temporary;
        NLSemanticTypeView temporary_type;
        CHECK(nl_semantic_value_view(c, i + 1, &temporary) &&
              nl_semantic_type_view(c, temporary.type, &temporary_type));
        CHECK(temporary_type.kind == NL_TYPE_REF &&
              temporary.carrier == NL_CARRIER_ENDED);
    }
    for (size_t i = 0; i < after.places; ++i) {
        CHECK(nl_semantic_place_view(c, i + 1, &current) &&
              current.placement.region == 0);
    }
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_STORAGE_READ_BYTE,
                                        .operands = {raw_binding(ref)},
                                        .data.byte_offset = {true, 4}},
                       NL_CHECK_SEMANTIC_ERROR, "P4-RAW-BOUNDS"));
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_STORAGE_READ_BYTE,
                                        .operands = {raw_binding(ref)},
                                        .data.byte_offset = {true, SIZE_MAX}},
                       NL_CHECK_SEMANTIC_ERROR, "P4-RAW-BOUNDS"));
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_STORAGE_READ_BYTE,
                                        .operands = {raw_binding(ref)},
                                        .data.byte_offset = {false, 0}},
                       NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                       "P4-DYNAMIC-PRECISION"));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_STORAGE_WRITE_BYTE,
                         .operands = {raw_binding(ref)},
                         .data.write = {{true, 0}, {u8, true, 42}}},
        NL_CHECK_SEMANTIC_ERROR, "P4-SCALAR-TYPE"));
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_STORAGE_LEN,
                                   .operands = {raw_binding(ref)}},
                  &r));
    CHECK(r.scalar_result.known && r.scalar_result.value == 4);
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_STORAGE_ADDR,
                                   .operands = {raw_binding(ref)}},
                  &r));
    CHECK(r.scalar_result.known && r.scalar_result.value == 4096 &&
          r.scalar_result.type == nl_semantic_core_type(c, NL_TYPE_ADDR));
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(c, b.value, &v) &&
          v.occupancy.region == region && v.occupancy.length == 4);
    nl_semantic_destroy(c);
    return true;
}

static bool copy_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    NLSymbolId a, s, b, t, r, q;
    CHECK(raw_allocate(c, 8, 8, true, true, "a", "s", &a, &s, NULL));
    CHECK(raw_allocate(c, 8, 8, true, true, "b", "t", &b, &t, NULL));
    CHECK(raw_ref(c, "r", s, &r, NULL));
    CHECK(raw_ref(c, "q", t, &q, NULL));
    CHECK(raw_write(c, r, 0, true, 10));
    CHECK(raw_write(c, r, 2, true, 20));
    CHECK(raw_write(c, r, 3, false, 0));
    for (size_t i = 0; i < 8; ++i) {
        CHECK(raw_write(c, q, i, true, 99));
    }
    CHECK(raw_copy(c, q, 0, r, 0, 8));
    CHECK(raw_read(c, q, 0, true, 10));
    CHECK(raw_unspecified(c, q, 1));
    CHECK(raw_read(c, q, 2, true, 20));
    CHECK(raw_read(c, q, 3, false, 0));
    CHECK(raw_unspecified(c, q, 7));
    /* Scalar loop fails on an Unspecified source, whereas raw copy succeeded.
     */
    CHECK(raw_unspecified(c, r, 1));
    CHECK(raw_copy(c, r, 1, r, 0,
                   4)); /* right overlap, pre-state [10,U,20,unknown] */
    CHECK(raw_read(c, r, 0, true, 10));
    CHECK(raw_read(c, r, 1, true, 10));
    CHECK(raw_unspecified(c, r, 2));
    CHECK(raw_read(c, r, 3, true, 20));
    CHECK(raw_read(c, r, 4, false, 0));
    CHECK(raw_unspecified(c, r, 5));
    CHECK(raw_copy(c, r, 0, r, 1, 4)); /* left overlap */
    CHECK(raw_read(c, r, 0, true, 10));
    CHECK(raw_unspecified(c, r, 1));
    CHECK(raw_read(c, r, 2, true, 20));
    CHECK(raw_read(c, r, 3, false, 0));
    CHECK(raw_copy(c, r, 0, r, 0, 8));
    CHECK(raw_copy(c, r, 8, q, 8, 0));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_COPY_BYTES,
                         .operands = {raw_binding(r), raw_binding(q)},
                         .data.copy = {{true, 9}, {true, 8}, {true, 0}}},
        NL_CHECK_SEMANTIC_ERROR, "P4-RAW-BOUNDS"));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_COPY_BYTES,
                         .operands = {raw_binding(r), raw_binding(q)},
                         .data.copy = {{true, 7}, {true, 0}, {true, SIZE_MAX}}},
        NL_CHECK_SEMANTIC_ERROR, "P4-RAW-BOUNDS"));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_COPY_BYTES,
                         .operands = {raw_binding(r), raw_binding(q)},
                         .data.copy = {{true, 0}, {false, 0}, {true, 1}}},
        NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P4-DYNAMIC-PRECISION"));
    /* Independent eight-byte sequence model: all 285 legal constant
     * source/destination/count selections, including both overlap directions.
     */
    NLSemanticBindingView holder;
    NLSemanticValueView claim;
    CHECK(nl_semantic_binding_view(c, s, &holder) &&
          nl_semantic_value_view(c, holder.value, &claim));
    NLRawRepView model[8];
    for (size_t i = 0; i < 8; ++i) {
        CHECK(
            nl_semantic_raw_rep_view(c, claim.occupancy.region, i, &model[i]));
    }
    for (size_t count = 0; count <= 8; ++count) {
        for (size_t src = 0; src <= 8 - count; ++src) {
            for (size_t dst = 0; dst <= 8 - count; ++dst) {
                NLRawRepView snapshot[8];
                for (size_t i = 0; i < count; ++i) {
                    snapshot[i] = model[src + i];
                }
                for (size_t i = 0; i < count; ++i) {
                    model[dst + i] = snapshot[i];
                }
                CHECK(raw_copy(c, r, dst, r, src, count));
                for (size_t i = 0; i < 8; ++i) {
                    NLRawRepView actual;
                    CHECK(nl_semantic_raw_rep_view(c, claim.occupancy.region, i,
                                                   &actual));
                    CHECK(actual.validity == model[i].validity &&
                          actual.value_known == model[i].value_known &&
                          actual.value == model[i].value);
                }
            }
        }
    }
    nl_semantic_destroy(c);
    return true;
}

static bool access_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    NLSymbolId a, s, b, t, r, q;
    CHECK(raw_allocate(c, 4, 4, true, false, "a", "s", &a, &s, NULL));
    CHECK(raw_allocate(c, 4, 4, false, true, "b", "t", &b, &t, NULL));
    CHECK(raw_ref(c, "r", s, &r, NULL));
    CHECK(raw_ref(c, "q", t, &q, NULL));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){
            .kind = NL_RAW_STORAGE_WRITE_BYTE,
            .operands = {raw_binding(r)},
            .data.write = {{true, 0},
                           {nl_semantic_core_type(c, NL_TYPE_BYTE), true, 1}}},
        NL_CHECK_SEMANTIC_ERROR, "P4-BACKING-ACCESS"));
    CHECK(raw_write(c, q, 0, true, 1));
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_STORAGE_READ_BYTE,
                                        .operands = {raw_binding(q)},
                                        .data.byte_offset = {true, 0}},
                       NL_CHECK_SEMANTIC_ERROR, "P4-BACKING-ACCESS"));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_COPY_BYTES,
                         .operands = {raw_binding(r), raw_binding(q)},
                         .data.copy = {{true, 0}, {true, 0}, {true, 1}}},
        NL_CHECK_SEMANTIC_ERROR, "P4-BACKING-ACCESS"));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_COPY_BYTES,
                         .operands = {raw_binding(q), raw_binding(q)},
                         .data.copy = {{true, 0}, {true, 0}, {true, 1}}},
        NL_CHECK_SEMANTIC_ERROR, "P4-BACKING-ACCESS"));
    NLSemanticBindingView rb;
    NLSemanticValueView rv;
    CHECK(nl_semantic_binding_view(c, r, &rb) &&
          nl_semantic_value_view(c, rb.value, &rv));
    CHECK(nl_semantic_end_scope(c, rv.reference.scope) == NL_CHECK_OK);
    /* Observation takes Storage refs, never implicit T -> ref<T>. */
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_STORAGE_LEN,
                                        .operands = {raw_binding(s)}},
                       NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));

    /* Draft 17.7: typed initialize needs destination write access, and the
     * persistent ptr inherits backing read/write access rather than gaining it.
     */
    CHECK(nl_semantic_set_layout(c, f.copy, 4, 4) == NL_CHECK_OK);
    NLSymbolId ro_a, ro_s, ro_empty, ro_incoming, ro_stable;
    NLScopeId ro_stable_scope;
    CHECK(
        raw_allocate(c, 4, 4, true, false, "ro_a", "ro_s", &ro_a, &ro_s, NULL));
    NLCheckedNodeView ro_slot;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                                   .operands = {raw_binding(ro_s)},
                                   .data.slot_target = f.copy},
                  &ro_slot));
    CHECK(raw_bind(c, "ro_empty", ro_slot.results[0].value, &ro_empty));
    CHECK(nl_semantic_seed_value(c, "ro_incoming", f.copy, NL_DEPENDENCY_FREE,
                                 &ro_incoming) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "ro_stable", NL_ACCESS_READ, false, &ro_stable,
                          &ro_stable_scope));
    CHECK(test_rejected(c, "initialize(ro_empty,ro_incoming,ro_stable)",
                        TEST_EXPRESSION, NL_CHECK_SEMANTIC_ERROR,
                        "P4-INITIALIZE-BACKING-WRITE"));
    CHECK(nl_semantic_end_scope(c, ro_stable_scope) == NL_CHECK_OK);

    NLSymbolId wo_a, wo_s, wo_empty, wo_incoming, wo_stable, wo_p, wo_ending;
    NLScopeId wo_stable_scope, wo_ending_scope;
    CHECK(
        raw_allocate(c, 4, 4, false, true, "wo_a", "wo_s", &wo_a, &wo_s, NULL));
    NLCheckedNodeView wo_slot;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                                   .operands = {raw_binding(wo_s)},
                                   .data.slot_target = f.copy},
                  &wo_slot));
    CHECK(raw_bind(c, "wo_empty", wo_slot.results[0].value, &wo_empty));
    CHECK(nl_semantic_seed_value(c, "wo_incoming", f.copy, NL_DEPENDENCY_FREE,
                                 &wo_incoming) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "wo_stable", NL_ACCESS_READ, false, &wo_stable,
                          &wo_stable_scope));
    TestChecked initialized = {0};
    CHECK(test_run(c, "initialize(wo_empty,wo_incoming,wo_stable)",
                   TEST_EXPRESSION, NL_CHECK_OK, NULL, &initialized));
    const NLValueId ptr = test_root(&initialized)->results[0].value;
    NLSemanticValueView ptr_view;
    CHECK(nl_semantic_value_view(c, ptr, &ptr_view) &&
          !ptr_view.reference.readable && ptr_view.reference.writable);
    CHECK(raw_bind(c, "wo_p", ptr, &wo_p));
    test_checked_destroy(&initialized);
    CHECK(nl_semantic_end_scope(c, wo_stable_scope) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "wo_ending", NL_ACCESS_READ, true, &wo_ending,
                          &wo_ending_scope));
    CHECK(test_rejected(c, "take(wo_p,wo_ending)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P4-TAKE-BACKING-READ"));
    TestChecked destroyed = {0};
    CHECK(test_run(c, "destroy(wo_p,wo_ending)", TEST_EXPRESSION, NL_CHECK_OK,
                   NULL, &destroyed));
    CHECK(test_root(&destroyed)->result_count == 1);
    test_checked_destroy(&destroyed);
    CHECK(nl_semantic_end_scope(c, wo_ending_scope) == NL_CHECK_OK);

    nl_semantic_destroy(c);
    return true;
}

static bool current_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    NLSymbolId a, s, b, t, r, w;
    NLScopeId scope;
    CHECK(raw_allocate(c, 8, 8, true, true, "a", "s", &a, &s, NULL));
    CHECK(raw_allocate(c, 4, 4, true, true, "b", "t", &b, &t, NULL));
    CHECK(raw_ref(c, "r", s, &r, &scope));
    NLSemanticBindingView holder;
    CHECK(nl_semantic_binding_view(c, s, &holder));
    CHECK(test_reference(c, "w", holder.place, NL_TYPE_REF, NL_ACCESS_WRITE,
                         false, &w, NULL));
    NLCheckedNodeView result;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_STORAGE_LEN,
                                   .operands = {raw_binding(r)}},
                  &result));
    CHECK(result.scalar_result.value == 8);
    TestChecked replaced = {0};
    CHECK(test_run(c, "replace(w,t)", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &replaced));
    NLSymbolId old;
    CHECK(raw_bind(c, "old", test_root(&replaced)->results[0].value, &old));
    test_checked_destroy(&replaced);
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_STORAGE_LEN,
                                   .operands = {raw_binding(r)}},
                  &result));
    CHECK(result.scalar_result.value == 4);
    CHECK(raw_rejected(
        c,
        (NLRawOperation){
            .kind = NL_RAW_STORAGE_WRITE_BYTE,
            .operands = {raw_binding(r)},
            .data.write = {{true, 6},
                           {nl_semantic_core_type(c, NL_TYPE_BYTE), true, 9}}},
        NL_CHECK_SEMANTIC_ERROR, "P4-RAW-BOUNDS"));
    CHECK(raw_write(c, r, 3, true, 9));
    CHECK(raw_read(c, r, 3, true, 9));
    CHECK(test_rejected(c, "store(w,old)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-DISCARDABLE-REQUIRED"));
    NLSymbolId old_ref;
    NLScopeId old_scope;
    CHECK(raw_ref(c, "old_ref", old, &old_ref, &old_scope));
    NLCheckedNodeView addr1, addr2;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_STORAGE_ADDR,
                                   .operands = {raw_binding(r)}},
                  &addr1));
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_STORAGE_ADDR,
                                   .operands = {raw_binding(old_ref)}},
                  &addr2));
    CHECK(addr1.scalar_result.known && addr2.scalar_result.known &&
          addr1.scalar_result.value == addr2.scalar_result.value);
    NLSemanticBindingView old_holder;
    NLSymbolId old_write;
    NLScopeId write_scope;
    CHECK(nl_semantic_binding_view(c, old, &old_holder));
    CHECK(test_reference(c, "old_write", old_holder.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &old_write, &write_scope));
    CHECK(test_run(c, "swap(w,old_write)", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &replaced));
    test_checked_destroy(&replaced);
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_STORAGE_LEN,
                                   .operands = {raw_binding(r)}},
                  &result) &&
          result.scalar_result.value == 8);
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_STORAGE_LEN,
                                   .operands = {raw_binding(old_ref)}},
                  &result) &&
          result.scalar_result.value == 4);
    CHECK(raw_read(c, old_ref, 3, true, 9));
    CHECK(nl_semantic_binding_view(c, s, &holder));
    NLSemanticValueView current_claim;
    CHECK(nl_semantic_value_view(c, holder.value, &current_claim) &&
          current_claim.occupancy.length == 8);
    CHECK(test_run(c, "swap(w,old_write)", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &replaced));
    test_checked_destroy(&replaced);
    CHECK(nl_semantic_end_scope(c, write_scope) == NL_CHECK_OK);
    CHECK(nl_semantic_end_scope(c, old_scope) == NL_CHECK_OK);
    CHECK(raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_DEALLOCATE,
                         .operands = {raw_binding(a), raw_binding(old)}},
        NULL));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_DEALLOCATE,
                         .operands = {raw_binding(b), raw_binding(s)}},
        NL_CHECK_SEMANTIC_ERROR, "P3-REF-CONFLICT"));
    NLSemanticBindingView wb;
    NLSemanticValueView wv;
    CHECK(nl_semantic_binding_view(c, w, &wb) &&
          nl_semantic_value_view(c, wb.value, &wv));
    CHECK(nl_semantic_end_scope(c, wv.reference.scope) == NL_CHECK_OK &&
          nl_semantic_end_scope(c, scope) == NL_CHECK_OK);
    CHECK(
        raw_run(c,
                (NLRawOperation){.kind = NL_RAW_DEALLOCATE,
                                 .operands = {raw_binding(b), raw_binding(s)}},
                NULL));
    nl_semantic_destroy(c);
    return true;
}

static bool raw_oom_case(NLRawOperationKind kind)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    NLSymbolId a, s, r = 0;
    CHECK(raw_allocate(c, 4, 4, true, true, "a", "s", &a, &s, NULL));
    CHECK(nl_semantic_set_layout(c, f.copy, 4, 4) == NL_CHECK_OK);
    NLRawOperation op = {.kind = kind, .operands = {raw_binding(s)}};
    NLCheckedNodeView prepared;
    switch (kind) {
    case NL_RAW_ALLOCATE:
        op = (NLRawOperation){
            .kind = kind,
            .data.allocate = {{true, 4}, {true, 4}, true, true, false, 0}};
        break;
    case NL_RAW_DEALLOCATE:
        op.operands[0] = raw_binding(a);
        op.operands[1] = raw_binding(s);
        break;
    case NL_RAW_SPLIT:
        op.data.split_at = raw_known(2);
        break;
    case NL_RAW_MERGE:
        CHECK(raw_run(c,
                      (NLRawOperation){.kind = NL_RAW_SPLIT,
                                       .operands = {raw_binding(s)},
                                       .data.split_at = {true, 2}},
                      &prepared));
        op.operands[0] = raw_loose(prepared.results[0].value);
        op.operands[1] = raw_loose(prepared.results[1].value);
        break;
    case NL_RAW_INTO_SLOT:
        op.data.slot_target = f.copy;
        break;
    case NL_RAW_ERASE_SLOT:
        CHECK(raw_run(c,
                      (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                                       .operands = {raw_binding(s)},
                                       .data.slot_target = f.copy},
                      &prepared));
        op.operands[0] = raw_loose(prepared.results[0].value);
        break;
    case NL_RAW_BYTE_TO_U8:
    case NL_RAW_U8_TO_BYTE:
        op.data.conversion = (NLScalarValue){
            nl_semantic_core_type(c, kind == NL_RAW_BYTE_TO_U8 ? NL_TYPE_BYTE
                                                               : NL_TYPE_U8),
            true, 255};
        break;
    default:
        CHECK(raw_ref(c, "r", s, &r, NULL));
        CHECK(raw_write(c, r, 0, true, 42));
        CHECK(raw_write(c, r, 2, false, 0));
        op.operands[0] = raw_binding(r);
        if (kind == NL_RAW_STORAGE_WRITE_BYTE) {
            op.data.write.offset = raw_known(1);
            op.data.write.value = (NLScalarValue){
                nl_semantic_core_type(c, NL_TYPE_BYTE), true, 7};
        } else if (kind == NL_RAW_COPY_BYTES) {
            op.operands[1] = raw_binding(r);
            op.data.copy.dst_offset = raw_known(1);
            op.data.copy.src_offset = raw_known(0);
            op.data.copy.count = raw_known(3);
        } else {
            op.data.byte_offset = raw_known(0);
        }
        break;
    }
    TestState before;
    CHECK(test_state(c, &before));
    bool succeeded = false;
    for (size_t index = 0; index < 160; ++index) {
        fail_at = index;
        allocation_index = 0;
        injecting = true;
        NLCheckedFragment *out = NULL;
        NLCheckDiagnostic diagnostic = {.diagnostic = {.code = "untouched"}};
        NLCheckStatus status =
            nl_semantic_check_raw_operation(c, &op, &out, &diagnostic);
        injecting = false;
        if (status == NL_CHECK_OK) {
            CHECK(out != NULL &&
                  strcmp(diagnostic.diagnostic.code, "untouched") == 0);
            nl_checked_destroy(out);
            succeeded = true;
            printf("raw operation %d: %zu allocation failures rolled back\n",
                   (int)kind, index);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && out == NULL &&
              diagnostic.diagnostic.code != NULL &&
              diagnostic.diagnostic.severity == NL_DIAG_ERROR);
        CHECK(test_unchanged(c, &before));
    }
    CHECK(succeeded);
    nl_semantic_destroy(c);
    return true;
}

static bool root_oom_case(size_t which)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    CHECK(nl_semantic_set_layout(c, f.copy, 4, 4) == NL_CHECK_OK);
    NLSymbolId a, s, empty, stable, incoming, ending, p;
    NLScopeId scope;
    CHECK(raw_allocate(c, 4, 4, true, true, "a", "s", &a, &s, NULL));
    NLCheckedNodeView slot;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                                   .operands = {raw_binding(s)},
                                   .data.slot_target = f.copy},
                  &slot));
    CHECK(raw_bind(c, "empty", slot.results[0].value, &empty));
    CHECK(nl_semantic_seed_value(c, "incoming", f.copy, NL_DEPENDENCY_FREE,
                                 &incoming) == NL_CHECK_OK);
    CHECK(
        test_domain_ref(&f, "stable", NL_ACCESS_READ, false, &stable, &scope));
    const char *const texts[] = {"initialize(empty,incoming,stable)",
                                 "take(p,ending)", "destroy(p,ending)"};
    if (which != 0) {
        TestChecked checked = {0};
        CHECK(test_run(c, texts[0], TEST_EXPRESSION, NL_CHECK_OK, NULL,
                       &checked));
        CHECK(raw_bind(c, "p", test_root(&checked)->results[0].value, &p));
        test_checked_destroy(&checked);
        CHECK(nl_semantic_end_scope(c, scope) == NL_CHECK_OK);
        CHECK(
            test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending, NULL));
    }
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_create(texts[which], strlen(texts[which]), "raw-root-oom",
                           &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_expression_fragment(parser, &tree, NULL) ==
              NL_PARSE_OK);
    TestState before;
    CHECK(test_state(c, &before));
    bool succeeded = false;
    for (size_t index = 0; index < 160; ++index) {
        fail_at = index;
        allocation_index = 0;
        injecting = true;
        NLCheckedFragment *out = NULL;
        NLCheckDiagnostic diagnostic = {0};
        NLCheckStatus status =
            nl_semantic_check_expression(c, tree, &out, &diagnostic);
        injecting = false;
        if (status == NL_CHECK_OK) {
            CHECK(out != NULL);
            nl_checked_destroy(out);
            succeeded = true;
            printf("%s: %zu allocation failures rolled back\n", texts[which],
                   index);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && out == NULL &&
              diagnostic.diagnostic.code != NULL);
        CHECK(test_unchanged(c, &before));
    }
    CHECK(succeeded);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}

static bool registration_oom_case(bool scalar)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    NLSymbolId a, s;
    CHECK(raw_allocate(c, 4, 4, true, true, "a", "s", &a, &s, NULL));
    TestState before;
    CHECK(test_state(c, &before));
    bool succeeded = false;
    for (size_t index = 0; index < 160; ++index) {
        NLSymbolId out = SIZE_MAX;
        fail_at = index;
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status =
            scalar ? nl_semantic_seed_scalar(
                         c, "octet",
                         (NLScalarValue){nl_semantic_core_type(c, NL_TYPE_BYTE),
                                         true, 255},
                         &out)
                   : nl_semantic_set_layout(c, f.copy, 4, 4);
        injecting = false;
        if (status == NL_CHECK_OK) {
            CHECK(!scalar || out != SIZE_MAX);
            succeeded = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && out == SIZE_MAX);
        CHECK(test_unchanged(c, &before));
    }
    CHECK(succeeded);
    nl_semantic_destroy(c);
    return true;
}

static bool failure_tests(void)
{
    for (size_t i = 0; i <= NL_RAW_U8_TO_BYTE; ++i) {
        CHECK(raw_oom_case((NLRawOperationKind)i));
    }
    for (size_t i = 0; i < 3; ++i) {
        CHECK(root_oom_case(i));
    }
    CHECK(registration_oom_case(false));
    CHECK(registration_oom_case(true));
    return true;
}

static bool limit_tests(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    NLSymbolId a, s, r;
    CHECK(raw_allocate(c, 4097, 1, true, true, "a", "s", &a, &s, NULL));
    CHECK(raw_ref(c, "r", s, &r, NULL));
    /* 2047 isolated writes create 4095 intervals. The next would require
     * 4097, while value/table counts still remain below their budgets. */
    for (size_t i = 0; i < 2047; ++i) {
        CHECK(raw_write(c, r, 2 * i + 1, true, 42));
    }
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(c, &before) && before.raw_intervals == 4095);
    NLCheckedFragment *out = NULL;
    NLCheckDiagnostic d = {0};
    NLRawOperation op = {
        .kind = NL_RAW_STORAGE_WRITE_BYTE,
        .operands = {raw_binding(r)},
        .data.write = {{true, 4095},
                       {nl_semantic_core_type(c, NL_TYPE_BYTE), true, 42}}};
    CHECK(nl_semantic_check_raw_operation(c, &op, &out, &d) ==
              NL_CHECK_RESOURCE_LIMIT &&
          out == NULL && strcmp(d.diagnostic.code, "P4-RESOURCE-LIMIT") == 0);
    CHECK(nl_semantic_snapshot(c, &after) && after.values == before.values &&
          after.raw_intervals == before.raw_intervals &&
          after.last_value_fact == before.last_value_fact);
    NLRawRepView raw;
    CHECK(nl_semantic_raw_rep_view(c, 1, 4095, &raw) &&
          raw.validity == NL_RAW_UNSPECIFIED);
    CHECK(raw_read(c, r, 4093, true, 42));
    nl_semantic_destroy(c);
    /* Huge ranges remain summarized, not malloc(size) or host shadow bytes. */
    c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    NLCheckedNodeView huge;
    CHECK(raw_run(c,
                  (NLRawOperation){
                      .kind = NL_RAW_ALLOCATE,
                      .data.allocate =
                          {{true, SIZE_MAX}, {true, 1}, true, true, false, 0}},
                  &huge));
    CHECK(raw_bind(c, "huge_a", huge.results[0].value, &a));
    CHECK(raw_bind(c, "huge_s", huge.results[1].value, &s));
    CHECK(raw_ref(c, "huge_r", s, &r, NULL));
    CHECK(raw_write(c, r, SIZE_MAX - 1, true, 255));
    CHECK(raw_read(c, r, SIZE_MAX - 1, true, 255));
    CHECK(raw_copy(c, r, SIZE_MAX, r, SIZE_MAX, 0));
    CHECK(nl_semantic_snapshot(c, &after) && after.raw_intervals == 2);
    nl_semantic_destroy(c);
    return true;
}

static bool diagnostic_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    NLSymbolId a, s, b, t;
    CHECK(raw_allocate(c, 4, 4, true, true, "a", "s", &a, &s, NULL));
    CHECK(raw_allocate(c, 4, 4, true, true, "b", "t", &b, &t, NULL));
    NLSource *source = NULL;
    CHECK(nl_source_create("deallocate(a,t)", 15, "raw-roles", &source) ==
          NL_SOURCE_OK);
    const NLRawOperation op = {.kind = NL_RAW_DEALLOCATE,
                               .source = source,
                               .span = {0, 15},
                               .operands = {{.binding = a, .span = {11, 12}},
                                            {.binding = t, .span = {13, 14}}}};
    CHECK(
        raw_rejected(c, op, NL_CHECK_SEMANTIC_ERROR, "P4-ALLOCATION-MISMATCH"));
    NLCheckedFragment *out = NULL;
    NLCheckDiagnostic d = {0};
    CHECK(nl_semantic_check_raw_operation(c, &op, &out, &d) ==
              NL_CHECK_SEMANTIC_ERROR &&
          d.span.start_byte == 11 && d.span.end_byte == 12);
    TestState before;
    CHECK(test_state(c, &before));
    NLRawOperation invalid = op;
    invalid.operands[0].loose = 1;
    CHECK(nl_semantic_check_raw_operation(c, &invalid, &out, NULL) ==
              NL_CHECK_INTERNAL_ERROR &&
          out == NULL && test_unchanged(c, &before));
    invalid = op;
    invalid.span.end_byte = 16;
    CHECK(nl_semantic_check_raw_operation(c, &invalid, &out, NULL) ==
              NL_CHECK_INTERNAL_ERROR &&
          test_unchanged(c, &before));
    invalid = op;
    invalid.kind = (NLRawOperationKind)-1;
    CHECK(nl_semantic_check_raw_operation(c, &invalid, &out, NULL) ==
              NL_CHECK_INTERNAL_ERROR &&
          test_unchanged(c, &before));
    const char *const internal_names[] = {"BackingRegion", "RawRange",
                                          "DefinedStorage"};
    for (size_t i = 0; i < sizeof(internal_names) / sizeof(internal_names[0]);
         ++i) {
        CHECK(test_rejected(c, internal_names[i], TEST_TYPE,
                            NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-TYPE"));
    }
    CHECK(nl_semantic_set_layout(c, f.copy, 0, 1) == NL_CHECK_SEMANTIC_ERROR &&
          test_unchanged(c, &before));
    CHECK(nl_semantic_set_layout(c, f.copy, 4, 4) == NL_CHECK_OK);
    CHECK(test_state(c, &before));
    CHECK(nl_semantic_set_layout(c, f.copy, 8, 8) == NL_CHECK_SEMANTIC_ERROR &&
          test_unchanged(c, &before));
    NLSymbolId sentinel = SIZE_MAX;
    CHECK(
        nl_semantic_seed_scalar(
            c, "bad",
            (NLScalarValue){nl_semantic_core_type(c, NL_TYPE_BYTE), true, 256},
            &sentinel) == NL_CHECK_SEMANTIC_ERROR &&
        sentinel == SIZE_MAX && test_unchanged(c, &before));
    CHECK(nl_semantic_seed_value(
              c, "forged", nl_semantic_core_type(c, NL_TYPE_STORAGE),
              NL_DEPENDENCY_FREE, &sentinel) == NL_CHECK_SEMANTIC_UNSUPPORTED &&
          test_unchanged(c, &before));
    CHECK(raw_rejected(
        c,
        (NLRawOperation){
            .kind = NL_RAW_ALLOCATE,
            .data.allocate = {{true, 0}, {true, 1}, true, true, false, 0}},
        NL_CHECK_SEMANTIC_UNSUPPORTED, "P4-EMPTY-ALLOCATION-DEFERRED"));
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}

static bool reborrow_tests(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    NLSymbolId a, s, ending;
    CHECK(raw_allocate(c, 4, 4, true, true, "a", "s", &a, &s, NULL));
    NLSemanticBindingView holder;
    CHECK(nl_semantic_binding_view(c, s, &holder));
    NLScopeId scope;
    CHECK(test_reference(c, "ending", holder.place, NL_TYPE_REF, NL_ACCESS_READ,
                         true, &ending, &scope));
    for (size_t i = 0; i < 2; ++i) {
        NLRawOperation op = {.kind = NL_RAW_STORAGE_LEN,
                             .operands = {raw_binding(ending)}};
        NLCheckedFragment *artifact = NULL;
        CHECK(nl_semantic_check_raw_operation(c, &op, &artifact, NULL) ==
              NL_CHECK_OK);
        const NLCheckedNodeView *root =
            nl_checked_node_view(artifact, nl_checked_root(artifact));
        const NLCheckedNodeView *arg =
            nl_checked_node_view(artifact, root->first_argument);
        CHECK(root->scalar_result.known && root->scalar_result.value == 4 &&
              arg->value_use == NL_VALUE_REBORROWED &&
              arg->reborrow_scope != scope);
        NLSemanticScopeView child;
        CHECK(nl_semantic_scope_view(c, arg->reborrow_scope, &child) &&
              !child.active);
        NLSemanticBindingView parent;
        CHECK(nl_semantic_binding_view(c, ending, &parent) &&
              parent.availability == NL_AVAILABLE);
        nl_checked_destroy(artifact);
    }
    CHECK(raw_write(c, ending, 0, true, 11));
    CHECK(raw_read(c, ending, 0, true, 11));
    /* One exclusive parent cannot supply two simultaneously live children. */
    CHECK(raw_rejected(
        c,
        (NLRawOperation){.kind = NL_RAW_COPY_BYTES,
                         .operands = {raw_binding(ending), raw_binding(ending)},
                         .data.copy = {{true, 0}, {true, 0}, {true, 1}}},
        NL_CHECK_SEMANTIC_ERROR, "P3-SUSPENDED-AUTHORITY"));
    CHECK(nl_semantic_end_scope(c, scope) == NL_CHECK_OK);
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_STORAGE_LEN,
                                        .operands = {raw_binding(ending)}},
                       NL_CHECK_SEMANTIC_ERROR, "P3-DEAD-SCOPE"));
    nl_semantic_destroy(c);
    return true;
}

static bool partition_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSemanticContext *const c = f.context;
    NLSymbolId a, s, r;
    NLScopeId scope;
    CHECK(raw_allocate(c, 8, 8, true, true, "a", "s", &a, &s, NULL));
    CHECK(raw_ref(c, "r", s, &r, &scope));
    CHECK(raw_write(c, r, 3, true, 3));
    CHECK(raw_write(c, r, 4, true, 4));
    CHECK(nl_semantic_end_scope(c, scope) == NL_CHECK_OK);
    NLCheckedNodeView parts;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_SPLIT,
                                   .operands = {raw_binding(s)},
                                   .data.split_at = {true, 4}},
                  &parts));
    NLSymbolId left, right, l, q;
    NLScopeId lscope, qscope;
    CHECK(raw_bind(c, "left", parts.results[0].value, &left));
    CHECK(raw_bind(c, "right", parts.results[1].value, &right));
    CHECK(raw_ref(c, "l", left, &l, &lscope));
    CHECK(raw_ref(c, "q", right, &q, &qscope));
    CHECK(raw_read(c, l, 3, true, 3));
    CHECK(raw_read(c, q, 0, true, 4));
    CHECK(raw_copy(c, q, 1, l, 2, 2));
    CHECK(raw_unspecified(c, q, 1));
    CHECK(raw_read(c, q, 2, true, 3));
    CHECK(raw_rejected(c,
                       (NLRawOperation){.kind = NL_RAW_STORAGE_READ_BYTE,
                                        .operands = {raw_binding(l)},
                                        .data.byte_offset = {true, 4}},
                       NL_CHECK_SEMANTIC_ERROR, "P4-RAW-BOUNDS"));
    CHECK(nl_semantic_end_scope(c, lscope) == NL_CHECK_OK &&
          nl_semantic_end_scope(c, qscope) == NL_CHECK_OK);
    CHECK(nl_semantic_set_layout(c, f.copy, 4, 4) == NL_CHECK_OK);
    NLSemanticSnapshot pre_slot, post_slot;
    CHECK(nl_semantic_snapshot(c, &pre_slot));
    NLCheckedNodeView slot, erased;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_INTO_SLOT,
                                   .operands = {raw_binding(left)},
                                   .data.slot_target = f.copy},
                  &slot));
    CHECK(raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_ERASE_SLOT,
                         .operands = {raw_loose(slot.results[0].value)}},
        &erased));
    CHECK(nl_semantic_snapshot(c, &post_slot) &&
          pre_slot.last_incarnation == post_slot.last_incarnation);
    NLCheckedNodeView merged;
    CHECK(raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_MERGE,
                         .operands = {raw_binding(right),
                                      raw_loose(erased.results[0].value)}},
        &merged));
    NLSymbolId full;
    CHECK(raw_bind(c, "full", merged.results[0].value, &full));
    CHECK(raw_ref(c, "after", full, &r, NULL));
    CHECK(raw_read(c, r, 3, true, 3));
    CHECK(raw_read(c, r, 4, true, 4));
    CHECK(raw_unspecified(c, r, 5));
    CHECK(raw_read(c, r, 6, true, 3));
    nl_semantic_destroy(c);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    bool okay = false;
    if (strcmp(argv[1], "claim") == 0) {
        okay = claim_tests();
    } else if (strcmp(argv[1], "byte") == 0) {
        okay = byte_tests();
    } else if (strcmp(argv[1], "copy") == 0) {
        okay = copy_tests();
    } else if (strcmp(argv[1], "access") == 0) {
        okay = access_tests();
    } else if (strcmp(argv[1], "current") == 0) {
        okay = current_tests();
    } else if (strcmp(argv[1], "failure") == 0) {
        okay = failure_tests();
    } else if (strcmp(argv[1], "limit") == 0) {
        okay = limit_tests();
    } else if (strcmp(argv[1], "diagnostic") == 0) {
        okay = diagnostic_tests();
    } else if (strcmp(argv[1], "reborrow") == 0) {
        okay = reborrow_tests();
    } else if (strcmp(argv[1], "partition") == 0) {
        okay = partition_tests();
    }
    return okay ? 0 : 1;
}
