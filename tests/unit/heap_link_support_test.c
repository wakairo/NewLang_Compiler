/* SUPPORTING programmatic claims, never the actual-source positive witness.
 * Exercise the exact read-only write predicate and existing mutation blockers
 * on a logical allocated root. No host address or runtime Node is involved. */
#include "../../src/semantic_internal.h"
#include "../support/semantic_check.h"

static bool run(void)
{
    NLSemanticContext *c = NULL;
    NLTypeId h, option, pointer, allocated_option;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(nl_recursive_header(c, "Node", &h) == NL_CHECK_OK);
    CHECK(nl_sem_compound(c, NL_TYPE_PTR, h, NL_ACCESS_READ, false, &pointer) ==
          NL_CHECK_OK);
    CHECK(nl_recursive_option(c, pointer, &option) == NL_CHECK_OK);
    const NLAggregateField fields[] = {
        {"next", option}, {"payload", nl_semantic_core_type(c, NL_TYPE_U8)}};
    CHECK(nl_recursive_complete(c, h, fields, 2) == NL_CHECK_OK);
    CHECK(nl_allocated_registry(c, h, &allocated_option) == NL_CHECK_OK);
    CHECK(nl_sem_compound(c, NL_TYPE_PTR, h, NL_ACCESS_READ, false, &pointer) ==
          NL_CHECK_OK);
    NLCheckedNodeView raw = {0}, slot = {0};
    const NLValueId zero[2] = {0};
    const NLRawOperation allocate = {.kind = NL_RAW_ALLOCATE,
                                     .data.allocate = {.size = {true, 24},
                                                       .alignment = {true, 8},
                                                       .ordinary_read = true,
                                                       .ordinary_write = true}};
    CHECK(nl_raw_apply(c, &allocate, zero, &raw, NULL) == NL_CHECK_OK);
    const NLValueId storage[] = {raw.results[1].value, 0};
    const NLRawOperation into = {.kind = NL_RAW_INTO_SLOT,
                                 .data.slot_target = h};
    CHECK(nl_raw_apply(c, &into, storage, &slot, NULL) == NL_CHECK_OK);
    NLValueId none, payload, aggregate, domain_value, ptr;
    NLDomainId domain;
    CHECK(nl_sem_new_value(c,
                           (NLSemanticValueView){.type = option, .variant = 1},
                           &none) == NL_CHECK_OK);
    CHECK(nl_sem_new_value(c,
                           (NLSemanticValueView){.type = fields[1].type,
                                                 .scalar_known = true,
                                                 .scalar_value = 9},
                           &payload) == NL_CHECK_OK);
    CHECK(nl_sem_new_value(c,
                           (NLSemanticValueView){.type = h,
                                                 .field_count = 2,
                                                 .fields = {none, payload}},
                           &aggregate) == NL_CHECK_OK);
    c->values[none - 1].carrier = c->values[payload - 1].carrier =
        NL_CARRIER_AGGREGATE;
    c->values[none - 1].aggregate_owner =
        c->values[payload - 1].aggregate_owner = aggregate;
    CHECK(nl_sem_new_domain(c, &domain, &domain_value) == NL_CHECK_OK);
    const NLValueId empty = slot.results[0].value;
    const NLPlaceId root = c->values[empty - 1].slot_place;
    CHECK(nl_sem_install(c, root, aggregate, domain) == NL_CHECK_OK);
    CHECK(nl_raw_start_root(c, empty, root) == NL_CHECK_OK);
    CHECK(nl_sem_new_value(
              c,
              (NLSemanticValueView){
                  .type = pointer,
                  .reference = {.place = root,
                                .incarnation = c->places[root - 1].incarnation,
                                .provenance = NL_PROVENANCE_VALID,
                                .readable = true,
                                .writable = true}},
              &ptr) == NL_CHECK_OK);
    CHECK(nl_sem_validate(c) == NL_CHECK_OK &&
          nl_raw_validate(c) == NL_CHECK_OK);
    CHECK(nl_allocated_write_access(c, ptr) == NL_CHECK_OK);
    const NLBackingRegionId region = c->places[root - 1].placement.region;
    c->values[ptr - 1].reference.writable = false;
    CHECK(nl_allocated_write_access(c, ptr) == NL_CHECK_SEMANTIC_ERROR);
    c->values[ptr - 1].reference.writable = true;
    c->regions[region - 1].view.ordinary_write = false;
    CHECK(nl_allocated_write_access(c, ptr) == NL_CHECK_SEMANTIC_ERROR);
    c->regions[region - 1].view.ordinary_write = true;
    c->values[ptr - 1].reference.provenance = NL_PROVENANCE_UNKNOWN;
    CHECK(nl_allocated_write_access(c, ptr) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    c->values[ptr - 1].reference.provenance = NL_PROVENANCE_INVALID;
    CHECK(nl_allocated_write_access(c, ptr) == NL_CHECK_SEMANTIC_ERROR);
    c->values[ptr - 1].reference.provenance = NL_PROVENANCE_VALID;
    ++c->values[ptr - 1].reference.incarnation;
    CHECK(nl_allocated_write_access(c, ptr) == NL_CHECK_SEMANTIC_ERROR);
    --c->values[ptr - 1].reference.incarnation;
    c->regions[region - 1].view.live = false;
    CHECK(nl_allocated_write_access(c, ptr) == NL_CHECK_SEMANTIC_ERROR);
    c->regions[region - 1].view.live = true;
    c->regions[region - 1].view.alignment = 1;
    CHECK(nl_allocated_write_access(c, ptr) == NL_CHECK_SEMANTIC_ERROR);
    c->regions[region - 1].view.alignment = 8;
    CHECK(nl_allocated_write_access(c, ptr) == NL_CHECK_OK);

    /* Existing §17 Change / §26 Reset rules, exercised with ref authority to
     * the same fixed site. Source borrowed-match is separately precision
     * fenced. */
    NLSymbolId p, w;
    CHECK(nl_sem_bind(c, "p", ptr, &p) == NL_CHECK_OK);
    const NLPlaceId link = c->places[root - 1].fixed_fields[0],
                    sibling = c->places[root - 1].fixed_fields[1];
    CHECK(test_reference(c, "w", link, NL_TYPE_REF, NL_ACCESS_WRITE, false, &w,
                         NULL));
    NLValueId ptr_copy, some;
    NLSymbolId some_binding;
    CHECK(nl_sem_copy_value(c, ptr, &ptr_copy) == NL_CHECK_OK);
    CHECK(nl_sem_new_value(c,
                           (NLSemanticValueView){.type = option,
                                                 .variant = 2,
                                                 .sum_payload = ptr_copy},
                           &some) == NL_CHECK_OK);
    c->values[ptr_copy - 1].carrier = NL_CARRIER_SUM;
    c->values[ptr_copy - 1].sum_owner = some;
    CHECK(nl_sem_bind(c, "some", some, &some_binding) == NL_CHECK_OK);
    TestChecked changed = {0};
    CHECK(test_run(c, "replace(w,some)", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &changed));
    test_checked_destroy(&changed);
    NLSemanticValueView blocked = {
        .type = fields[1].type,
        .dependencies = NL_EXACT_VALUE_DEPENDENCIES,
        .value_dependency_count = 1,
        .value_dependencies = {{link, c->places[link - 1].current_fact}}};
    NLValueId dependent;
    CHECK(nl_sem_new_value(c, blocked, &dependent) == NL_CHECK_OK);
    CHECK(test_run(c, "replace(w,some)", TEST_EXPRESSION,
                   NL_CHECK_SEMANTIC_ERROR, "FIELD-VALUE-DEPENDENCY",
                   &changed));
    test_checked_destroy(&changed);
    nl_sem_end_value(c, dependent);
    blocked.value_dependencies[0] =
        (NLValueDependency){sibling, c->places[sibling - 1].current_fact};
    CHECK(nl_sem_new_value(c, blocked, &dependent) == NL_CHECK_OK);
    const NLValueFactId sibling_fact = c->places[sibling - 1].current_fact;
    CHECK(test_run(c, "replace(w,some)", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &changed));
    test_checked_destroy(&changed);
    CHECK(c->places[sibling - 1].current_fact == sibling_fact &&
          nl_fixed_dependencies(c) == NL_CHECK_OK);
    nl_sem_end_value(c, dependent);
    const NLPlaceId conditional =
        c->occurrences[c->places[link - 1].payload_occurrence - 1]
            .payload_place;
    NLSymbolId payload_ref;
    CHECK(test_reference(c, "conditional", conditional, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &payload_ref, NULL));
    c->values[c->bindings[payload_ref - 1].view.value - 1]
        .reference.occurrence_dependency =
        c->places[link - 1].payload_occurrence;
    CHECK(test_run(c, "replace(w,some)", TEST_EXPRESSION,
                   NL_CHECK_SEMANTIC_ERROR, "P6-OCCURRENCE-CONFLICT",
                   &changed));
    test_checked_destroy(&changed);
    nl_semantic_destroy(c);
    return true;
}
int main(void)
{
    return run() ? 0 : 1;
}
