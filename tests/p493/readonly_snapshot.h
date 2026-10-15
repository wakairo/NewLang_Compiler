/* Existing public-view snapshot comparisons, const-correct local adapter.
 * No mutable access to artifact-owned worlds and no private layouts. */
#ifndef P493_READONLY_SNAPSHOT_H
#define P493_READONLY_SNAPSHOT_H
#include "../support/semantic_check.h"
static inline bool readonly_state(const NLSemanticContext *context,
                                  TestState *state)
{
    CHECK(nl_semantic_snapshot(context, &state->counts));
    const NLSemanticSnapshot c = state->counts;
    CHECK(c.types <= 256 && c.bindings <= 256 && c.values <= 256 &&
          c.places <= 256 && c.domains <= 256 && c.scopes <= 256);
    for (size_t i = 0; i < c.types; ++i) {
        CHECK(nl_semantic_type_view(context, i + 1, &state->types[i]));
    }
    for (size_t i = 0; i < c.bindings; ++i) {
        CHECK(nl_semantic_binding_view(context, i + 1, &state->bindings[i]));
    }
    for (size_t i = 0; i < c.values; ++i) {
        CHECK(nl_semantic_value_view(context, i + 1, &state->values[i]));
    }
    for (size_t i = 0; i < c.places; ++i) {
        CHECK(nl_semantic_place_view(context, i + 1, &state->places[i]));
    }
    for (size_t i = 0; i < c.domains; ++i) {
        CHECK(nl_semantic_domain_view(context, i + 1, &state->domains[i]));
    }
    for (size_t i = 0; i < c.scopes; ++i) {
        CHECK(nl_semantic_scope_view(context, i + 1, &state->scopes[i]));
    }
    CHECK(c.occurrences <= 256);
    for (size_t i = 0; i < c.occurrences; ++i)
        CHECK(nl_semantic_occurrence_view(context, i + 1,
                                          &state->occurrences[i]));
    CHECK(c.backing_regions <= 16);
    for (size_t i = 0; i < c.backing_regions; ++i) {
        CHECK(nl_semantic_backing_view(context, i + 1, &state->regions[i]));
        const NLSemanticBackingView r = state->regions[i];
        if (r.live) {
            CHECK(r.size <= 64);
            for (size_t j = 0; j < r.size; ++j) {
                CHECK(nl_semantic_raw_rep_view(context, i + 1, j,
                                               &state->raw[i][j]));
            }
        }
    }
    return true;
}
static inline bool readonly_unchanged(const NLSemanticContext *context,
                                      const TestState *before)
{
    TestState after;
    CHECK(readonly_state(context, &after));
    const NLSemanticSnapshot a = before->counts, b = after.counts;
    CHECK(a.types == b.types && a.bindings == b.bindings &&
          a.values == b.values && a.places == b.places &&
          a.domains == b.domains && a.scopes == b.scopes &&
          a.functions == b.functions &&
          a.last_incarnation == b.last_incarnation &&
          a.last_value_fact == b.last_value_fact &&
          a.backing_regions == b.backing_regions &&
          a.raw_intervals == b.raw_intervals && a.occurrences == b.occurrences);
    for (size_t i = 0; i < a.types; ++i) {
        CHECK(test_type_equal(before->types[i], after.types[i]));
    }
    for (size_t i = 0; i < a.bindings; ++i) {
        const NLSemanticBindingView x = before->bindings[i],
                                    y = after.bindings[i];
        CHECK(x.availability == y.availability && x.type == y.type &&
              x.value == y.value && x.place == y.place);
    }
    for (size_t i = 0; i < a.values; ++i) {
        const NLSemanticValueView x = before->values[i], y = after.values[i];
        CHECK(x.type == y.type && x.carrier == y.carrier &&
              x.owner_place == y.owner_place &&
              x.aggregate_owner == y.aggregate_owner &&
              x.field_count == y.field_count && x.variant == y.variant &&
              x.sum_payload == y.sum_payload && x.sum_owner == y.sum_owner &&
              memcmp(x.fields, y.fields, sizeof(x.fields)) == 0 &&
              x.dependencies == y.dependencies &&
              x.value_dependency_count == y.value_dependency_count &&
              memcmp(x.value_dependencies, y.value_dependencies,
                     sizeof(x.value_dependencies)) == 0 &&
              x.domain == y.domain && x.slot_place == y.slot_place &&
              x.allocation_region == y.allocation_region &&
              test_range_equal(x.occupancy, y.occupancy) &&
              x.scalar_known == y.scalar_known &&
              x.scalar_value == y.scalar_value &&
              test_reference_equal(x.reference, y.reference) &&
              x.reference_count == y.reference_count);
        for (size_t j = 0; j < x.reference_count; ++j)
            CHECK(test_reference_equal(x.references[j], y.references[j]));
    }
    for (size_t i = 0; i < a.places; ++i) {
        CHECK(test_place_equal(before->places[i], after.places[i]));
    }
    for (size_t i = 0; i < a.domains; ++i) {
        CHECK(before->domains[i].live == after.domains[i].live &&
              before->domains[i].value == after.domains[i].value);
    }
    for (size_t i = 0; i < a.scopes; ++i) {
        CHECK(before->scopes[i].active == after.scopes[i].active &&
              before->scopes[i].parent == after.scopes[i].parent &&
              before->scopes[i].parent_authority ==
                  after.scopes[i].parent_authority);
    }
    for (size_t i = 0; i < a.occurrences; ++i) {
        NLSemanticOccurrenceView x = before->occurrences[i],
                                 y = after.occurrences[i];
        CHECK(x.live == y.live && x.root == y.root &&
              x.payload_place == y.payload_place && x.variant == y.variant);
    }
    for (size_t i = 0; i < a.backing_regions; ++i) {
        const NLSemanticBackingView x = before->regions[i],
                                    y = after.regions[i];
        CHECK(x.live == y.live && x.size == y.size &&
              x.alignment == y.alignment &&
              x.ordinary_read == y.ordinary_read &&
              x.ordinary_write == y.ordinary_write &&
              x.address_known == y.address_known && x.address == y.address);
        if (x.live) {
            for (size_t j = 0; j < x.size; ++j) {
                const NLRawRepView u = before->raw[i][j], v = after.raw[i][j];
                CHECK(u.validity == v.validity &&
                      u.value_known == v.value_known && u.value == v.value);
            }
        }
    }
    return true;
}
#endif
