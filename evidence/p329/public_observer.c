/* P329 public-only evidence, derived from P319 public observer.
 * Same owned artifact: four Changes AND actual original pair return/terminal.
 * No private headers or seeded root/Allocation/Domain state. */
#include "../../tests/support/test.h"
#include "newlang/captured_closure.h"
#include "newlang/checked_c_node.h"
#include "newlang/parser.h"
#include "newlang/raw_storage.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocations, failure = SIZE_MAX;
static bool refuse(void)
{
    return injecting && allocations++ == failure;
}
void *__wrap_malloc(size_t n)
{
    return refuse() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return refuse() ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return refuse() ? NULL : __real_realloc(p, n);
}

static NLCheckedNodeId allocation_match(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        const NLCheckedNodeView *a = nl_checked_node_view(f, n->initializer);
        if (n->kind == NL_CHECKED_MATCH && a &&
            a->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
            return i;
    }
    return 0;
}
static bool inventory(const NLSemanticContext *c, NLValueId as[5],
                      NLValueId ds[5])
{
    NLSemanticSnapshot snap;
    CHECK(nl_semantic_snapshot(c, &snap) && snap.backing_regions == 5 &&
          snap.domains == 5);
    memset(as, 0, 5 * sizeof(*as));
    memset(ds, 0, 5 * sizeof(*ds));
    for (size_t i = 1; i <= snap.values; ++i) {
        NLSemanticValueView v;
        NLSemanticTypeView t;
        CHECK(nl_semantic_value_view(c, i, &v) &&
              nl_semantic_type_view(c, v.type, &t));
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        if (t.kind == NL_TYPE_ALLOCATION) {
            CHECK(v.allocation_region >= 1 && v.allocation_region <= 5 &&
                  !as[v.allocation_region - 1]);
            as[v.allocation_region - 1] = i;
        }
        if (v.type == nl_semantic_domain_type(c)) {
            CHECK(v.domain >= 1 && v.domain <= 5 && !ds[v.domain - 1]);
            ds[v.domain - 1] = i;
        }
    }
    for (size_t i = 0; i < 5; ++i) {
        NLSemanticBackingView r;
        NLSemanticDomainView d;
        CHECK(as[i] && ds[i] && nl_semantic_backing_view(c, i + 1, &r) &&
              r.live && nl_semantic_domain_view(c, i + 1, &d) && d.live &&
              d.value == ds[i]);
    }
    return true;
}
static const NLSource *evidence_source;
static const NLCheckedFragment *evidence_fragment;
static bool carrier(const NLSemanticContext *c, const char *name,
                    NLAvailability availability, NLValueId value,
                    NLSemanticBindingView *out)
{
    NLSymbolId symbol = nl_semantic_find_binding(c, name);
    if (!symbol) {
        for (size_t i = 1; i <= nl_checked_node_count(evidence_fragment); ++i) {
            const NLCheckedNodeView *n =
                nl_checked_node_view(evidence_fragment, i);
            NLSourceView source;
            if (n->symbol &&
                nl_source_view(evidence_source, n->name, &source) &&
                source.length == strlen(name) &&
                !memcmp(source.bytes, name, strlen(name)))
                symbol = n->symbol;
        }
    }
    CHECK(symbol && nl_semantic_binding_view(c, symbol, out) &&
          out->availability == availability && (!value || out->value == value));
    return true;
}
static bool direct_third(const NLCheckedFragment *f,
                         const NLScopedFieldCallView *second)
{
    NLCapturedChangeView change, initial;
    CHECK(nl_checked_captured_change_count(f) == 8 &&
          nl_checked_captured_change_view(f, 6, &change) &&
          nl_checked_captured_change_view(f, 2, &initial));
    const NLCheckedNodeView *write = nl_checked_node_view(f, change.operation);
    const NLCheckedNodeView *setup = nl_checked_node_view(f, initial.operation);
    const NLCheckedNodeView *projection =
        write ? nl_checked_node_view(f, write->first_argument) : NULL;
    CHECK(write && setup && projection && write->kind == NL_CHECKED_REPLACE &&
          projection->kind == NL_CHECKED_FIELD_REF && write->field.index == 0 &&
          write->field.parent == setup->field.parent &&
          write->field.child == setup->field.child &&
          write->field.old_value == setup->field.new_value);
    const NLSemanticContext *worlds[] = {change.before, change.after};
    NLSemanticPlaceView roots[2], fields[2], target[2];
    NLSemanticValueView value[2], ptr[2];
    NLValueId av[2][5], dv[2][5];
    for (size_t k = 0; k < 2; ++k) {
        CHECK(
            inventory(worlds[k], av[k], dv[k]) &&
            nl_semantic_place_view(worlds[k], write->field.parent, &roots[k]) &&
            nl_semantic_place_view(worlds[k], write->field.child, &fields[k]) &&
            roots[k].live && roots[k].placement.region == 2 &&
            roots[k].governing_domain == 2 &&
            roots[k].fixed_fields[0] == write->field.child &&
            nl_semantic_value_view(worlds[k], fields[k].current_value,
                                   &value[k]) &&
            value[k].variant == 2 && value[k].sum_payload &&
            nl_semantic_value_view(worlds[k], value[k].sum_payload, &ptr[k]) &&
            ptr[k].reference.provenance == NL_PROVENANCE_VALID &&
            nl_semantic_place_view(worlds[k], ptr[k].reference.place,
                                   &target[k]) &&
            target[k].live);
    }
    CHECK(!memcmp(av[0], av[1], sizeof(av[0])) &&
          !memcmp(dv[0], dv[1], sizeof(dv[0])) &&
          target[0].placement.region == 3 && target[0].governing_domain == 3 &&
          target[1].placement.region == 4 && target[1].governing_domain == 4 &&
          roots[0].incarnation == roots[1].incarnation &&
          !memcmp(&roots[0].placement, &roots[1].placement,
                  sizeof(roots[0].placement)) &&
          fields[0].incarnation == fields[1].incarnation &&
          fields[0].current_fact != fields[1].current_fact &&
          roots[0].current_fact != roots[1].current_fact &&
          fields[0].current_fact == write->field.child_fact &&
          fields[1].current_fact == write->field.child_post_fact &&
          roots[0].current_fact == write->field.parent_fact &&
          roots[1].current_fact == write->field.parent_post_fact &&
          fields[0].payload_occurrence && fields[1].payload_occurrence &&
          fields[0].payload_occurrence != fields[1].payload_occurrence);
    NLSemanticOccurrenceView old, ended, fresh;
    NLSemanticPlaceView payload;
    CHECK(nl_semantic_occurrence_view(worlds[0], fields[0].payload_occurrence,
                                      &old) &&
          old.live &&
          nl_semantic_occurrence_view(worlds[1], fields[0].payload_occurrence,
                                      &ended) &&
          !ended.live &&
          nl_semantic_occurrence_view(worlds[1], fields[1].payload_occurrence,
                                      &fresh) &&
          fresh.live &&
          nl_semantic_place_view(worlds[1], old.payload_place, &payload) &&
          !payload.live && !payload.current_fact && !payload.current_value);
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(second->change.after, &snapshot));
    size_t count = 0;
    for (size_t i = 1; i <= snapshot.places; ++i) {
        NLSemanticPlaceView p, b, a;
        CHECK(nl_semantic_place_view(second->change.after, i, &p));
        if (!p.live || !p.placement.region || p.fixed_field_count != 4)
            continue;
        CHECK(nl_semantic_place_view(worlds[0], i, &b) &&
              !memcmp(&p, &b, sizeof(p)) &&
              nl_semantic_place_view(worlds[1], i, &a) && a.live &&
              a.incarnation == b.incarnation &&
              a.governing_domain == b.governing_domain &&
              !memcmp(&a.placement, &b.placement, sizeof(a.placement)));
        if (i != write->field.parent)
            CHECK(!memcmp(&a, &b, sizeof(a)));
        ++count;
        for (size_t j = 0; j < 4; ++j) {
            NLSemanticPlaceView prior, before, after;
            CHECK(
                nl_semantic_place_view(second->change.after, p.fixed_fields[j],
                                       &prior) &&
                nl_semantic_place_view(worlds[0], p.fixed_fields[j], &before) &&
                !memcmp(&prior, &before, sizeof(prior)) &&
                nl_semantic_place_view(worlds[1], p.fixed_fields[j], &after));
            if (p.fixed_fields[j] != write->field.child)
                CHECK(!memcmp(&before, &after, sizeof(before)));
            NLSemanticValueView v, point;
            NLSemanticPlaceView destination;
            CHECK(nl_semantic_value_view(worlds[1], after.current_value, &v));
            if (j == 3)
                continue;
            size_t expected = 0;
            if (p.placement.region == 1 && j == 2)
                expected = 2;
            if (p.placement.region == 2 && (j == 0 || j == 1))
                expected = 4;
            if (p.placement.region == 4 && j == 1)
                expected = 3;
            CHECK(v.variant == (expected ? 2u : 1u));
            if (expected)
                CHECK(
                    nl_semantic_value_view(worlds[1], v.sum_payload, &point) &&
                    nl_semantic_place_view(worlds[1], point.reference.place,
                                           &destination) &&
                    destination.live &&
                    destination.placement.region == expected);
        }
    }
    CHECK(count == 5);
    NLSemanticBindingView carrier_A, carrier_B;
    CHECK(carrier(worlds[0], "life_A", NL_AVAILABLE, dv[0][1], &carrier_A) &&
          carrier(worlds[0], "moved_d_B", NL_AVAILABLE, dv[0][2], &carrier_B) &&
          carrier(worlds[1], "moved_d_B", NL_AVAILABLE, dv[1][2], &carrier_B) &&
          carrier(worlds[1], "moved_a_B", NL_AVAILABLE, av[1][2], &carrier_B));
    const NLScopeId scope = projection->reference_result.scope;
    NLSemanticScopeView loan;
    CHECK(scope && nl_semantic_scope_view(worlds[0], scope, &loan) &&
          loan.active &&
          nl_semantic_scope_view(nl_checked_context(f), scope, &loan) &&
          !loan.active);
    size_t found = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_LOAN_HEADER && n->loan.scope == scope) {
            CHECK(n->loan.domain == 2 &&
                  n->loan.source ==
                      nl_semantic_find_binding(worlds[0], "life_A") &&
                  n->loan.place == carrier_A.place && !n->loan.is_exclusive);
            ++found;
        }
    }
    CHECK(found == 1);
    printf("{\"direct_cross_root_change\":true,\"ordinal\":3,\"operation\":%zu,"
           "\"scope\":%zu,"
           "\"root\":%zu,\"incarnation\":%zu,\"region\":2,\"DomainId\":2,"
           "\"Allocation_value\":%zu,\"Domain_value\":%zu,\"field\":%zu,"
           "\"ProjectionId_index\":0,\"fact_before\":%zu,\"fact_after\":%zu,"
           "\"root_fact_before\":%zu,\"root_fact_after\":%zu,\"old_"
           "occurrence\":%zu,\"new_occurrence\":%zu,\"old_payload_place\":%zu,"
           "\"old_ptr_region\":3,\"new_ptr_region\":4,\"original_B_still_live_"
           "and_owned\":true,\"second_after_equals_third_before\":true,\"all_"
           "siblings_preserved\":true,\"old_occurrence_ended\":true,\"new_"
           "occurrence_live\":true,\"direct_change_validated_in_five_captured_"
           "closures\":true}\n",
           change.operation, scope, write->field.parent, roots[0].incarnation,
           av[0][1], dv[0][1], write->field.child, fields[0].current_fact,
           fields[1].current_fact, roots[0].current_fact, roots[1].current_fact,
           fields[0].payload_occurrence, fields[1].payload_occurrence,
           old.payload_place);
    return true;
}
static bool direct_fourth(const NLCheckedFragment *f)
{
    NLCapturedChangeView change, initial, third;
    CHECK(nl_checked_captured_change_count(f) == 8 &&
          nl_checked_captured_change_view(f, 7, &change) &&
          nl_checked_captured_change_view(f, 5, &initial) &&
          nl_checked_captured_change_view(f, 6, &third));
    const NLCheckedNodeView *write = nl_checked_node_view(f, change.operation);
    const NLCheckedNodeView *setup = nl_checked_node_view(f, initial.operation);
    const NLCheckedNodeView *projection =
        write ? nl_checked_node_view(f, write->first_argument) : NULL;
    CHECK(write && setup && projection && write->kind == NL_CHECKED_REPLACE &&
          projection->kind == NL_CHECKED_FIELD_REF && write->field.index == 1 &&
          write->field.parent == setup->field.parent &&
          write->field.child == setup->field.child &&
          write->field.old_value == setup->field.new_value);
    const NLSemanticContext *worlds[] = {change.before, change.after};
    NLSemanticPlaceView roots[2], fields[2], target[2];
    NLSemanticValueView value[2], ptr[2];
    NLValueId av[2][5], dv[2][5];
    for (size_t k = 0; k < 2; ++k) {
        CHECK(
            inventory(worlds[k], av[k], dv[k]) &&
            nl_semantic_place_view(worlds[k], write->field.parent, &roots[k]) &&
            nl_semantic_place_view(worlds[k], write->field.child, &fields[k]) &&
            roots[k].live && roots[k].placement.region == 4 &&
            roots[k].governing_domain == 4 &&
            roots[k].fixed_fields[1] == write->field.child &&
            nl_semantic_value_view(worlds[k], fields[k].current_value,
                                   &value[k]) &&
            value[k].variant == 2 && value[k].sum_payload &&
            nl_semantic_value_view(worlds[k], value[k].sum_payload, &ptr[k]) &&
            ptr[k].reference.provenance == NL_PROVENANCE_VALID &&
            nl_semantic_place_view(worlds[k], ptr[k].reference.place,
                                   &target[k]) &&
            target[k].live);
    }
    CHECK(!memcmp(av[0], av[1], sizeof(av[0])) &&
          !memcmp(dv[0], dv[1], sizeof(dv[0])) &&
          target[0].placement.region == 3 && target[0].governing_domain == 3 &&
          target[1].placement.region == 2 && target[1].governing_domain == 2 &&
          roots[0].incarnation == roots[1].incarnation &&
          !memcmp(&roots[0].placement, &roots[1].placement,
                  sizeof(roots[0].placement)) &&
          fields[0].incarnation == fields[1].incarnation &&
          fields[0].current_fact != fields[1].current_fact &&
          roots[0].current_fact != roots[1].current_fact &&
          fields[0].current_fact == write->field.child_fact &&
          fields[1].current_fact == write->field.child_post_fact &&
          roots[0].current_fact == write->field.parent_fact &&
          roots[1].current_fact == write->field.parent_post_fact &&
          fields[0].payload_occurrence && fields[1].payload_occurrence &&
          fields[0].payload_occurrence != fields[1].payload_occurrence);
    NLSemanticOccurrenceView old, ended, fresh;
    NLSemanticPlaceView payload;
    CHECK(nl_semantic_occurrence_view(worlds[0], fields[0].payload_occurrence,
                                      &old) &&
          old.live &&
          nl_semantic_occurrence_view(worlds[1], fields[0].payload_occurrence,
                                      &ended) &&
          !ended.live &&
          nl_semantic_occurrence_view(worlds[1], fields[1].payload_occurrence,
                                      &fresh) &&
          fresh.live &&
          nl_semantic_place_view(worlds[1], old.payload_place, &payload) &&
          !payload.live && !payload.current_fact && !payload.current_value);
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(third.after, &snapshot));
    size_t count = 0;
    for (size_t i = 1; i <= snapshot.places; ++i) {
        NLSemanticPlaceView p, b, a;
        CHECK(nl_semantic_place_view(third.after, i, &p));
        if (!p.live || !p.placement.region || p.fixed_field_count != 4)
            continue;
        CHECK(nl_semantic_place_view(worlds[0], i, &b) &&
              !memcmp(&p, &b, sizeof(p)) &&
              nl_semantic_place_view(worlds[1], i, &a) && a.live &&
              a.incarnation == b.incarnation &&
              a.governing_domain == b.governing_domain &&
              !memcmp(&a.placement, &b.placement, sizeof(a.placement)));
        if (i != write->field.parent)
            CHECK(!memcmp(&a, &b, sizeof(a)));
        ++count;
        for (size_t j = 0; j < 4; ++j) {
            NLSemanticPlaceView prior, before, after;
            CHECK(
                nl_semantic_place_view(third.after, p.fixed_fields[j],
                                       &prior) &&
                nl_semantic_place_view(worlds[0], p.fixed_fields[j], &before) &&
                !memcmp(&prior, &before, sizeof(prior)) &&
                nl_semantic_place_view(worlds[1], p.fixed_fields[j], &after));
            if (p.fixed_fields[j] != write->field.child)
                CHECK(!memcmp(&before, &after, sizeof(before)));
            NLSemanticValueView v, point;
            NLSemanticPlaceView destination;
            CHECK(nl_semantic_value_view(worlds[1], after.current_value, &v));
            if (j == 3)
                continue;
            size_t expected = 0;
            if (p.placement.region == 1 && j == 2)
                expected = 2;
            if (p.placement.region == 2 && (j == 0 || j == 1))
                expected = 4;
            if (p.placement.region == 4 && j == 1)
                expected = 2;
            CHECK(v.variant == (expected ? 2u : 1u));
            if (expected)
                CHECK(
                    nl_semantic_value_view(worlds[1], v.sum_payload, &point) &&
                    nl_semantic_place_view(worlds[1], point.reference.place,
                                           &destination) &&
                    destination.live &&
                    destination.placement.region == expected);
        }
    }
    CHECK(count == 5);
    NLSemanticBindingView carrier_A, carrier_B;
    CHECK(carrier(worlds[0], "life_C", NL_AVAILABLE, dv[0][3], &carrier_A) &&
          carrier(worlds[0], "moved_d_B", NL_AVAILABLE, dv[0][2], &carrier_B) &&
          carrier(worlds[1], "moved_d_B", NL_AVAILABLE, dv[1][2], &carrier_B) &&
          carrier(worlds[1], "moved_a_B", NL_AVAILABLE, av[1][2], &carrier_B));
    const NLScopeId scope = projection->reference_result.scope;
    NLSemanticScopeView loan;
    CHECK(scope && nl_semantic_scope_view(worlds[0], scope, &loan) &&
          loan.active &&
          nl_semantic_scope_view(nl_checked_context(f), scope, &loan) &&
          !loan.active);
    size_t found = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_LOAN_HEADER && n->loan.scope == scope) {
            CHECK(n->loan.domain == 4 &&
                  n->loan.source ==
                      nl_semantic_find_binding(worlds[0], "life_C") &&
                  n->loan.place == carrier_A.place && !n->loan.is_exclusive);
            ++found;
        }
    }
    CHECK(found == 1);
    printf("{\"direct_cross_root_change\":true,\"ordinal\":4,\"operation\":%zu,"
           "\"scope\":%zu,"
           "\"root\":%zu,\"incarnation\":%zu,\"region\":4,\"DomainId\":4,"
           "\"Allocation_value\":%zu,\"Domain_value\":%zu,\"field\":%zu,"
           "\"ProjectionId_index\":1,\"fact_before\":%zu,\"fact_after\":%zu,"
           "\"root_fact_before\":%zu,\"root_fact_after\":%zu,\"old_"
           "occurrence\":%zu,\"new_occurrence\":%zu,\"old_payload_place\":%zu,"
           "\"old_ptr_region\":3,\"new_ptr_region\":2,\"original_B_still_live_"
           "and_owned\":true,\"third_after_equals_fourth_before\":true,\"all_"
           "siblings_preserved\":true,\"old_occurrence_ended\":true,\"new_"
           "occurrence_live\":true,\"direct_change_validated_in_five_captured_"
           "closures\":true}\n",
           change.operation, scope, write->field.parent, roots[0].incarnation,
           av[0][3], dv[0][3], write->field.child, fields[0].current_fact,
           fields[1].current_fact, roots[0].current_fact, roots[1].current_fact,
           fields[0].payload_occurrence, fields[1].payload_occurrence,
           old.payload_place);
    return true;
}
static bool owner_composition(const NLCheckedFragment *entry,
                              const NLCheckedFragment *f);
static bool inspect(const NLCheckedFragment *entry)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    CHECK(f);
    NLCapturedOriginal original_B = {0};
    for (size_t i = 0; i < 5; ++i) {
        NLCheckedNodeId m = allocation_match(f);
        CHECK(m && nl_checked_captured_closure_validate(f, m) == NL_CHECK_OK);
        NLCapturedClosureView closure;
        CHECK(nl_checked_captured_closure_view(f, m, &closure));
        if (i == 0) {
            for (size_t j = 0; j < 6; ++j)
                CHECK(closure.release_worlds[j] == 1);
            printf("{\"static_release_worlds_0_to_5\":[1,1,1,1,1,1],\"native_"
                   "failure_injection\":false}\n");
        }
        for (size_t j = 0; j < closure.count; ++j)
            if (closure.originals[j].domain == 3)
                original_B = closure.originals[j];
        f = nl_checked_match_arm(f, m, 1);
        CHECK(f);
    }
    evidence_fragment = f;
    evidence_source = nl_checked_source(f);
    CHECK(original_B.root && nl_checked_captured_change_count(f) == 8);
    NLScopedFieldCallView calls[2];
    NLCheckedNodeId ids[2];
    size_t count = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        NLScopedFieldCallView item;
        if (nl_checked_scoped_field_call_view(f, i, &item)) {
            CHECK(count < 2);
            calls[count] = item;
            ids[count++] = i;
        }
    }
    CHECK(count == 2 && ids[0] < ids[1] &&
          nl_checked_node_view(f, ids[0])->function ==
              nl_checked_node_view(f, ids[1])->function &&
          calls[0].body != calls[1].body &&
          calls[0].change.before != calls[1].change.before &&
          calls[0].change.after != calls[1].change.after);
    NLSemanticPlaceView first_root_after = {0}, first_prev_after = {0},
                        first_child_after = {0};
    NLScopeId first_scope = 0;
    for (size_t ordinal = 0; ordinal < 2; ++ordinal) {
        const NLScopedFieldCallView w = calls[ordinal];
        CHECK(nl_checked_scoped_field_call_validate(f, ids[ordinal]) ==
                  NL_CHECK_OK &&
              nl_checked_captured_change_count(w.body) == 1);
        const NLCheckedNodeView *write =
            nl_checked_node_view(w.body, w.change.operation);
        const NLCheckedNodeView *projection =
            nl_checked_node_view(f, w.projection);
        const size_t index = projection ? projection->field.index : 0;
        CHECK(write && projection && index == 1 - ordinal &&
              write->kind == NL_CHECKED_REPLACE &&
              projection->kind == NL_CHECKED_FIELD_REF &&
              write->field.index == index &&
              write->field.nominal == projection->field.nominal &&
              write->field.index == projection->field.index &&
              write->field.child == projection->field.child);
        NLSemanticPlaceView root[2], field[2], prev[2], child[2];
        NLSemanticValueView value[2], payload[2];
        NLSemanticOccurrenceView old_occ, ended_occ, new_occ;
        const NLSemanticContext *worlds[] = {w.change.before, w.change.after};
        NLValueId as[2][5], ds[2][5];
        for (size_t i = 0; i < 2; ++i) {
            CHECK(inventory(worlds[i], as[i], ds[i]) &&
                  nl_semantic_place_view(worlds[i], write->field.parent,
                                         &root[i]) &&
                  root[i].live &&
                  nl_semantic_place_view(worlds[i], write->field.child,
                                         &field[i]) &&
                  field[i].live && root[i].fixed_field_count == 4 &&
                  root[i].fixed_fields[index] == write->field.child &&
                  root[i].fixed_fields[0] != root[i].fixed_fields[1] &&
                  nl_semantic_place_view(
                      worlds[i], root[i].fixed_fields[1 - index], &prev[i]) &&
                  prev[i].live &&
                  nl_semantic_place_view(worlds[i], root[i].fixed_fields[2],
                                         &child[i]) &&
                  child[i].live &&
                  nl_semantic_value_view(worlds[i], field[i].current_value,
                                         &value[i]));
            if (value[i].variant == 2)
                CHECK(nl_semantic_value_view(worlds[i], value[i].sum_payload,
                                             &payload[i]));
        }
        CHECK(!memcmp(as[0], as[1], sizeof(as[0])) &&
              !memcmp(ds[0], ds[1], sizeof(ds[0])) &&
              root[0].placement.region == 3 && root[0].governing_domain == 3 &&
              root[0].incarnation == root[1].incarnation &&
              root[0].governing_domain == root[1].governing_domain &&
              !memcmp(&root[0].placement, &root[1].placement,
                      sizeof(root[0].placement)) &&
              field[0].incarnation == field[1].incarnation &&
              field[0].current_fact != field[1].current_fact &&
              root[0].current_fact != root[1].current_fact &&
              value[0].variant == 2 &&
              field[0].current_value == write->field.old_value &&
              field[1].current_value == write->field.new_value &&
              field[0].current_value != field[1].current_value &&
              field[0].payload_occurrence &&
              nl_semantic_occurrence_view(
                  worlds[0], field[0].payload_occurrence, &old_occ) &&
              old_occ.live &&
              nl_semantic_occurrence_view(
                  worlds[1], field[0].payload_occurrence, &ended_occ) &&
              !ended_occ.live && !memcmp(&prev[0], &prev[1], sizeof(prev[0])) &&
              !memcmp(&child[0], &child[1], sizeof(child[0])) &&
              (index ? prev[0].payload_occurrence != 0
                     : prev[0].payload_occurrence == 0));
        CHECK(value[1].variant == 1);
        if (value[1].variant == 2)
            CHECK(field[1].payload_occurrence &&
                  field[0].payload_occurrence != field[1].payload_occurrence &&
                  nl_semantic_occurrence_view(
                      worlds[1], field[1].payload_occurrence, &new_occ) &&
                  new_occ.live);
        else
            CHECK(value[1].variant == 1 && !field[1].payload_occurrence);
        NLSemanticPlaceView old_payload_before, old_payload_after;
        CHECK(nl_semantic_place_view(worlds[0], old_occ.payload_place,
                                     &old_payload_before) &&
              old_payload_before.live && old_payload_before.current_fact &&
              nl_semantic_place_view(worlds[1], old_occ.payload_place,
                                     &old_payload_after) &&
              !old_payload_after.live && !old_payload_after.current_fact &&
              !old_payload_after.current_value);
        NLSemanticPlaceView old_target, new_target = {0};
        CHECK(nl_semantic_place_view(worlds[0], payload[0].reference.place,
                                     &old_target) &&
              old_target.placement.region == (index ? 2 : 4));
        if (value[1].variant == 2)
            CHECK(nl_semantic_place_view(worlds[1], payload[1].reference.place,
                                         &new_target) &&
                  (new_target.placement.region == 3 ||
                   new_target.placement.region == 4));
        NLSemanticScopeView scope;
        CHECK(nl_semantic_scope_view(
                  worlds[0], projection->reference_result.scope, &scope) &&
              scope.active &&
              nl_semantic_scope_view(nl_checked_context(f),
                                     projection->reference_result.scope,
                                     &scope) &&
              !scope.active);
        NLSemanticBindingView old_d, new_d, new_a, donor, final_d, final_a;
        NLSemanticBindingView original_domain, original_allocation;
        NLSemanticPlaceView origin_root, old_local, new_local;
        CHECK(nl_semantic_binding_view(original_B.origin,
                                       original_B.domain_binding,
                                       &original_domain) &&
              nl_semantic_binding_view(original_B.origin,
                                       original_B.allocation_binding,
                                       &original_allocation) &&
              original_domain.availability == NL_AVAILABLE &&
              original_allocation.availability == NL_AVAILABLE &&
              original_domain.value == ds[0][2] &&
              original_allocation.value == as[0][2] &&
              nl_semantic_place_view(original_B.origin, original_B.root,
                                     &origin_root) &&
              origin_root.live &&
              origin_root.incarnation == root[0].incarnation &&
              original_B.root == write->field.parent &&
              original_B.domain == root[0].governing_domain &&
              !memcmp(&origin_root.placement, &root[0].placement,
                      sizeof(origin_root.placement)));
        CHECK(
            carrier(worlds[0], "life_B", NL_CONSUMED, ds[0][2], &old_d) &&
            carrier(worlds[0], "allocation_B", NL_CONSUMED, as[0][2], &donor) &&
            carrier(worlds[0], "moved_d_B", NL_AVAILABLE, ds[0][2], &new_d) &&
            carrier(worlds[0], "moved_a_B", NL_AVAILABLE, as[0][2], &new_a) &&
            old_d.place != new_d.place &&
            original_domain.place == old_d.place &&
            nl_semantic_place_view(worlds[0], old_d.place, &old_local) &&
            nl_semantic_place_view(worlds[0], new_d.place, &new_local) &&
            !old_local.live && !old_local.current_value && new_local.live &&
            new_local.current_value == ds[0][2]);
        const char *consumed[] = {"owner_B", "staged", "transferred", "held"};
        for (size_t i = 0; i < 4; ++i)
            CHECK(carrier(worlds[0], consumed[i], NL_CONSUMED, 0, &donor));
        CHECK(carrier(worlds[1], "moved_d_B", NL_AVAILABLE, ds[0][2], &donor) &&
              donor.place == new_d.place &&
              carrier(worlds[1], "moved_a_B", NL_AVAILABLE, as[0][2], &donor));
        size_t reloan = 0;
        for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
            const NLCheckedNodeView *n = nl_checked_node_view(f, i);
            if (n->kind == NL_CHECKED_LOAN_HEADER &&
                n->loan.scope == projection->reference_result.scope) {
                CHECK(!reloan && n->loan.domain == 3 &&
                      n->loan.place == new_d.place &&
                      n->loan.incarnation == new_local.incarnation &&
                      !n->loan.is_exclusive);
                reloan = i;
            }
        }
        CHECK(reloan);
        size_t previous_B_loans = 0;
        for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
            const NLCheckedNodeView *n = nl_checked_node_view(f, i);
            if (n->kind != NL_CHECKED_LOAN_HEADER || n->loan.domain != 3 ||
                n->loan.scope == projection->reference_result.scope ||
                i > reloan)
                continue;
            CHECK(n->loan.scope < projection->reference_result.scope &&
                  nl_semantic_scope_view(worlds[0], n->loan.scope, &scope) &&
                  !scope.active);
            ++previous_B_loans;
        }
        CHECK(previous_B_loans == 2 + ordinal);
        if (!ordinal) {
            first_root_after = root[1];
            first_prev_after = field[1];
            first_child_after = child[1];
            first_scope = projection->reference_result.scope;
        } else {
            CHECK(!memcmp(&first_root_after, &root[0], sizeof(root[0])) &&
                  !memcmp(&first_prev_after, &prev[0], sizeof(prev[0])) &&
                  !memcmp(&first_prev_after, &prev[1], sizeof(prev[1])) &&
                  !memcmp(&first_child_after, &child[0], sizeof(child[0])) &&
                  first_scope != projection->reference_result.scope &&
                  nl_semantic_scope_view(worlds[0], first_scope, &scope) &&
                  !scope.active);
        }
        printf(
            "{\"call_ordinal\":%zu,\"call_id\":%zu,\"scope\":%zu,"
            "\"prior_B_loans_ended\":%zu,\"chronological_connection\":true}\n",
            ordinal, ids[ordinal], projection->reference_result.scope,
            previous_B_loans);

        const NLSemanticContext *final = nl_checked_context(f);
        CHECK(carrier(final, "moved_d_B", NL_CONSUMED, ds[0][2], &donor) &&
              carrier(final, "moved_a_B", NL_CONSUMED, as[0][2], &donor) &&
              carrier(final, "rebuilt", NL_CONSUMED, 0, &donor) &&
              carrier(final, "terminal_owner", NL_CONSUMED, 0, &donor) &&
              carrier(final, "final_d_B", NL_CONSUMED, ds[0][2], &final_d) &&
              carrier(final, "final_a_B", NL_CONSUMED, as[0][2], &final_a));
        NLSemanticDomainView ended_domain;
        NLSemanticBackingView ended_region;
        NLSemanticPlaceView ended_root;
        CHECK(nl_semantic_domain_view(final, 3, &ended_domain) &&
              !ended_domain.live &&
              nl_semantic_backing_view(final, 3, &ended_region) &&
              !ended_region.live &&
              nl_semantic_place_view(final, original_B.root, &ended_root) &&
              !ended_root.live);
        printf("{\"original_B_domain\":3,\"original_B_allocation_value\":%zu,"
               "\"original_B_domain_value\":%zu,\"old_domain_place\":%zu,"
               "\"new_domain_place\":%zu,\"old_binding_consumed\":true,"
               "\"new_binding_sole_current\":true,\"fresh_loan_on_new_place\":"
               "true,"
               "\"repacked_and_exactly_released\":true}\n",
               as[0][2], ds[0][2], old_d.place, new_d.place);
        printf(
            "{\"owned_source_artifact\":true,\"closure_validators\":5,\"scoped_"
            "call_validator\":true,"
            "\"caller_changes\":8,\"callee_changes\":1,\"ProjectionId_"
            "nominal\":"
            "%zu,\"ProjectionId_index\":%zu,"
            "\"root\":%zu,\"root_incarnation\":%zu,\"field\":%zu,\"field_"
            "incarnation\":%zu,"
            "\"fact_before\":%zu,\"fact_after\":%zu,\"old_occurrence\":%zu,"
            "\"new_occurrence\":%zu,"
            "\"old_ptr_region\":%zu,\"new_ptr_region\":%zu,"
            "\"root_fact_before\":%zu,\"root_fact_after\":%zu,"
            "\"Allocation_values\":[%zu,%zu,%zu,%zu,%zu],\"Domain_values\":[%"
            "zu,"
            "%zu,%zu,%zu,%zu],"
            "\"old_payload_place\":%zu,\"old_payload_current_cleared\":true,"
            "\"sibling_and_child_current_preserved\":true,\"original_A_D_"
            "preserved_"
            "unique\":"
            "true,"
            "\"scoped_ref_ended\":true,\"matched_frees\":5,\"native_"
            "execution\":"
            "false}\n",
            write->field.nominal, write->field.index, write->field.parent,
            root[0].incarnation, write->field.child, field[0].incarnation,
            field[0].current_fact, field[1].current_fact,
            field[0].payload_occurrence, field[1].payload_occurrence,
            old_target.placement.region, new_target.placement.region,
            root[0].current_fact, root[1].current_fact, as[0][0], as[0][1],
            as[0][2], as[0][3], as[0][4], ds[0][0], ds[0][1], ds[0][2],
            ds[0][3], ds[0][4], old_occ.payload_place);
    }
    return direct_third(f, &calls[1]) && direct_fourth(f) &&
           owner_composition(entry, f);
}
static bool current_with_result(const NLSemanticContext *c, NLValueId id,
                                NLValueId result)
{
    for (size_t depth = 0; depth < 18; ++depth) {
        NLSemanticValueView v;
        CHECK(nl_semantic_value_view(c, id, &v));
        if (v.carrier == NL_CARRIER_LOOSE)
            return result && id == result;
        if (v.carrier == NL_CARRIER_PLACE) {
            NLSemanticPlaceView p;
            NLSemanticSnapshot snap;
            CHECK(nl_semantic_place_view(c, v.owner_place, &p) && p.live &&
                  p.current_value == id && nl_semantic_snapshot(c, &snap));
            size_t count = 0;
            for (size_t i = 1; i <= snap.bindings; ++i) {
                NLSemanticBindingView b;
                CHECK(nl_semantic_binding_view(c, i, &b));
                count += b.availability == NL_AVAILABLE && b.value == id &&
                         b.place == v.owner_place;
            }
            CHECK(count == 1);
            return true;
        }
        CHECK(v.carrier == NL_CARRIER_AGGREGATE && v.aggregate_owner);
        NLSemanticValueView parent;
        CHECK(nl_semantic_value_view(c, v.aggregate_owner, &parent));
        size_t count = 0;
        for (size_t i = 0; i < parent.field_count; ++i)
            count += parent.fields[i] == id;
        CHECK(count == 1);
        id = v.aggregate_owner;
    }
    return false;
}

static bool current(const NLSemanticContext *c, NLValueId id)
{
    return current_with_result(c, id, 0);
}

static bool packet(const NLSemanticContext *c, NLValueId id, size_t *r,
                   size_t *a, size_t *d, NLPlaceId *root, NLIncarnationId *inc)
{
    NLSemanticValueView v, p, av, dv;
    NLSemanticPlaceView place;
    CHECK(nl_semantic_value_view(c, id, &v) && v.field_count == 3 &&
          nl_semantic_value_view(c, v.fields[0], &p) &&
          nl_semantic_value_view(c, v.fields[1], &av) &&
          nl_semantic_value_view(c, v.fields[2], &dv));
    CHECK(p.reference.provenance == NL_PROVENANCE_VALID &&
          p.reference.scope == 0 &&
          nl_semantic_place_view(c, p.reference.place, &place) && place.live &&
          place.independent_root &&
          place.incarnation == p.reference.incarnation);
    CHECK(place.governing_domain == dv.domain && av.allocation_region &&
          current(c, v.fields[1]) && current(c, v.fields[2]));
    *r = place.placement.region;
    *a = av.allocation_region;
    *d = dv.domain;
    *root = p.reference.place;
    *inc = p.reference.incarnation;
    return true;
}

static bool releases(const NLCheckedFragment *f, size_t order[5], size_t *count,
                     size_t *terminals, size_t depth)
{
    CHECK(depth < 4);
    const NLSemanticContext *c = nl_checked_context(f);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_DEALLOCATE) {
            const NLCheckedNodeView *a =
                nl_checked_node_view(f, n->first_argument);
            const NLCheckedNodeView *raw =
                a ? nl_checked_node_view(f, a->next_argument) : NULL;
            NLSemanticValueView av, rv;
            CHECK(*count < 5 && a && raw && a->result_count == 1 &&
                  raw->result_count == 1 &&
                  nl_semantic_value_view(c, a->results[0].value, &av) &&
                  nl_semantic_value_view(c, raw->results[0].value, &rv));
            CHECK(av.allocation_region == rv.occupancy.region &&
                  rv.occupancy.start == 0 && rv.occupancy.length);
            order[(*count)++] = av.allocation_region;
        }
        if (n->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(f, i);
            CHECK(body);
            NLScopedFieldCallView scoped;
            if (n->type == nl_semantic_unit_type(c) &&
                !nl_checked_scoped_field_call_view(f, i, &scoped)) {
                NLTwoRootDefinition nested;
                if (!nl_semantic_two_root_definition(c, n->function, &nested)) {
                    NLTypedOwnerDefinition definition;
                    CHECK(nl_semantic_function_applicability(c, n->function,
                                                             &definition) &&
                          definition.definition_checked &&
                          definition.requirements ==
                              NL_OWNER_ALL_REQUIREMENTS &&
                          definition.step_count == 4);
                    for (size_t s = 0; s < 4; ++s)
                        CHECK(definition.steps[s] == (NLTypedOwnerStep)(s + 1));
                    ++*terminals;
                } else
                    CHECK(nested.definition_checked && nested.count == 2);
            }
            CHECK(releases(body, order, count, terminals, depth + 1));
        }
    }
    return true;
}

static bool owner_composition(const NLCheckedFragment *entry,
                              const NLCheckedFragment *f)
{
    NLWholeValueCallView w = {0};
    NLTwoRootCallView two = {0};
    NLCheckedNodeId pure_id = 0, terminal_id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        NLWholeValueCallView candidate;
        NLTwoRootCallView late;
        if (nl_checked_whole_value_call_view(f, i, &candidate)) {
            CHECK(!pure_id);
            pure_id = i;
            w = candidate;
        }
        if (nl_checked_two_root_call_view(f, i, &late)) {
            CHECK(!terminal_id);
            terminal_id = i;
            two = late;
        }
    }
    CHECK(pure_id && terminal_id > pure_id && w.count == 3 &&
          nl_checked_two_root_call_validate(f, terminal_id) == NL_CHECK_OK &&
          two.input == w.result && two.donor == w.receiver);
    const NLCheckedFragment *body = nl_checked_call_body(f, pure_id);
    CHECK(body);
    size_t returns = 0, aggregates = 0;
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, i);
        CHECK(
            n->kind == NL_CHECKED_BLOCK || n->kind == NL_CHECKED_BINDING ||
            n->kind == NL_CHECKED_AGGREGATE ||
            n->kind == NL_CHECKED_IDENTIFIER || n->kind == NL_CHECKED_RETURN ||
            n->kind == NL_CHECKED_AGGREGATE_FIELD ||
            n->kind == NL_CHECKED_STATEMENT || n->kind == NL_CHECKED_RECEIVER);
        returns += n->kind == NL_CHECKED_RETURN;
        aggregates += n->kind == NL_CHECKED_AGGREGATE;
    }
    CHECK(returns == 1 && aggregates == 1);
    NLSemanticValueView whole[3];
    const NLSemanticContext *worlds[] = {w.entry, w.returned, w.received};
    CHECK(nl_semantic_value_view(w.returned, w.result, &whole[1]) &&
          nl_semantic_value_view(w.received, w.result, &whole[2]) &&
          whole[1].field_count == 2 && whole[1].carrier == NL_CARRIER_LOOSE &&
          whole[2].carrier == NL_CARRIER_PLACE &&
          !memcmp(whole[1].fields, whole[2].fields, sizeof(whole[1].fields)) &&
          whole[2].fields[0] == w.inputs[0] &&
          whole[2].fields[1] == w.inputs[1]);
    NLValueId allocations[3][5], domains[3][5];
    for (size_t j = 0; j < 3; ++j) {
        CHECK(inventory(worlds[j], allocations[j], domains[j]));
        NLSemanticSnapshot snap;
        CHECK(nl_semantic_snapshot(worlds[j], &snap));
        for (size_t i = 1; i <= snap.values; ++i) {
            NLSemanticValueView v;
            NLSemanticTypeView t;
            CHECK(nl_semantic_value_view(worlds[j], i, &v) &&
                  nl_semantic_type_view(worlds[j], v.type, &t));
            if (v.carrier != NL_CARRIER_ENDED &&
                (t.kind == NL_TYPE_ALLOCATION ||
                 v.type == nl_semantic_domain_type(worlds[j])))
                CHECK(current_with_result(worlds[j], i, j == 1 ? w.result : 0));
        }
        for (size_t i = 1; i <= snap.scopes; ++i) {
            NLSemanticScopeView scope;
            CHECK(nl_semantic_scope_view(worlds[j], i, &scope) &&
                  !scope.active);
        }
        CHECK(!memcmp(allocations[0], allocations[j], sizeof(allocations[0])) &&
              !memcmp(domains[0], domains[j], sizeof(domains[0])));
    }
    NLCapturedChangeView last_change;
    CHECK(nl_checked_captured_change_view(f, 7, &last_change));
    for (size_t n = 1; n <= nl_checked_node_count(f); ++n) {
        const NLCheckedNodeView *initial = nl_checked_node_view(f, n);
        if (initial->kind != NL_CHECKED_INITIALIZE)
            continue;
        CHECK(initial->lifetime_range.region >= 1 &&
              initial->lifetime_range.region <= 5);
        NLSemanticPlaceView original;
        CHECK(nl_semantic_place_view(last_change.after, initial->lifetime_place,
                                     &original) &&
              original.live &&
              original.incarnation == initial->lifetime_incarnation &&
              original.governing_domain == initial->lifetime_domain &&
              original.placement.region == initial->lifetime_range.region);
        for (size_t j = 0; j < 3; ++j) {
            NLSemanticPlaceView now;
            CHECK(nl_semantic_place_view(worlds[j], initial->lifetime_place,
                                         &now) &&
                  !memcmp(&original, &now, sizeof(now)));
            for (size_t k = 0; k < 4; ++k) {
                NLSemanticPlaceView field_before, field_now;
                CHECK(nl_semantic_place_view(last_change.after,
                                             original.fixed_fields[k],
                                             &field_before) &&
                      nl_semantic_place_view(
                          worlds[j], original.fixed_fields[k], &field_now) &&
                      !memcmp(&field_before, &field_now, sizeof(field_now)));
            }
        }
    }
    NLPlaceId roots[2];
    NLIncarnationId inc[2];
    size_t rr[2], aa[2], dd[2];
    NLSemanticBindingView received;
    CHECK(nl_semantic_binding_view(w.received, w.receiver, &received) &&
          received.availability == NL_AVAILABLE && received.value == w.result);
    for (size_t i = 0; i < 2; ++i) {
        const size_t region = i == 0 ? 5 : 3;
        NLSemanticBindingView donor, consumed, formal;
        CHECK(
            nl_semantic_binding_view(w.entry, w.donors[i], &donor) &&
            donor.availability == NL_AVAILABLE && donor.value == w.inputs[i] &&
            nl_semantic_binding_view(w.returned, w.donors[i], &consumed) &&
            consumed.availability == NL_CONSUMED &&
            nl_semantic_binding_view(w.returned, w.parameters[i], &formal) &&
            formal.availability == NL_CONSUMED && formal.value == w.inputs[i] &&
            donor.place != formal.place && formal.place != received.place);
        for (size_t j = 0; j < 3; ++j) {
            NLSemanticValueView p;
            CHECK(nl_semantic_value_view(worlds[j], w.inputs[i], &p) &&
                  p.field_count == 3 &&
                  p.fields[1] == allocations[j][region - 1] &&
                  p.fields[2] == domains[j][region - 1]);
            for (size_t k = 0; k < 3; ++k) {
                NLSemanticValueView before, now;
                CHECK(nl_semantic_value_view(worlds[0], p.fields[k], &before) &&
                      nl_semantic_value_view(worlds[j], p.fields[k], &now) &&
                      before.allocation_region == now.allocation_region &&
                      before.domain == now.domain &&
                      !memcmp(&before.reference, &now.reference,
                              sizeof(now.reference)));
            }
        }
        CHECK(packet(w.received, w.inputs[i], &rr[i], &aa[i], &dd[i], &roots[i],
                     &inc[i]) &&
              rr[i] == region && aa[i] == region && dd[i] == region);
    }
    NLSemanticBindingView token, token_after, formal;
    NLSemanticValueView token_value;
    NLSemanticTypeView token_type;
    CHECK(nl_semantic_binding_view(w.entry, w.donors[2], &token) &&
          nl_semantic_binding_view(w.received, w.donors[2], &token_after) &&
          token.availability == NL_AVAILABLE &&
          token_after.availability == NL_AVAILABLE &&
          token.value == token_after.value &&
          nl_semantic_binding_view(w.returned, w.parameters[2], &formal) &&
          formal.availability == NL_CONSUMED && formal.value == w.inputs[2] &&
          nl_semantic_value_view(w.returned, formal.value, &token_value) &&
          nl_semantic_type_view(w.returned, token_value.type, &token_type) &&
          token_type.kind == NL_TYPE_PTR && token_type.is_copy &&
          !token_value.domain && !token_value.allocation_region &&
          !token_value.reference.scope);
    NLSemanticBindingView old;
    const char *old_b[] = {
        "owner_B",        "staged",    "transferred", "held",      "rebuilt",
        "terminal_owner", "moved_a_B", "moved_d_B",   "final_a_B", "final_d_B"};
    for (size_t i = 0; i < sizeof(old_b) / sizeof(old_b[0]); ++i)
        CHECK(carrier(w.received, old_b[i], NL_CONSUMED, 0, &old));
    CHECK(carrier(w.received, "allocation_dst", NL_CONSUMED, allocations[0][4],
                  &old) &&
          carrier(w.received, "life_dst", NL_CONSUMED, domains[0][4], &old));
    NLSemanticValueView terminal_whole;
    CHECK(nl_semantic_value_view(two.entry, two.input, &terminal_whole) &&
          !memcmp(terminal_whole.fields, whole[2].fields,
                  sizeof(terminal_whole.fields)) &&
          current(two.entry, two.input));
    NLSemanticSnapshot snap;
    CHECK(nl_semantic_snapshot(two.entry, &snap));
    size_t live_as = 0, live_ds = 0;
    for (size_t i = 1; i <= snap.values; ++i) {
        NLSemanticValueView v;
        NLSemanticTypeView t;
        CHECK(nl_semantic_value_view(two.entry, i, &v) &&
              nl_semantic_type_view(two.entry, v.type, &t));
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        if (t.kind == NL_TYPE_ALLOCATION ||
            v.type == nl_semantic_domain_type(two.entry)) {
            CHECK(current(two.entry, i));
            live_as += t.kind == NL_TYPE_ALLOCATION;
            live_ds += v.type == nl_semantic_domain_type(two.entry);
        }
    }
    CHECK(live_as == 2 && live_ds == 2);
    for (size_t i = 1; i <= snap.scopes; ++i) {
        NLSemanticScopeView scope;
        CHECK(nl_semantic_scope_view(two.entry, i, &scope) && !scope.active);
    }
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticPlaceView before, after;
        CHECK(nl_semantic_place_view(w.received, roots[i], &before) &&
              nl_semantic_place_view(two.entry, roots[i], &after) &&
              !memcmp(&before, &after, sizeof(after)));
        for (size_t j = 0; j < 4; ++j) {
            CHECK(nl_semantic_place_view(w.received, before.fixed_fields[j],
                                         &before) &&
                  nl_semantic_place_view(two.entry, after.fixed_fields[j],
                                         &after) &&
                  !memcmp(&before, &after, sizeof(after)));
            CHECK(nl_semantic_place_view(w.received, roots[i], &before) &&
                  nl_semantic_place_view(two.entry, roots[i], &after));
        }
    }
    for (size_t r = 1; r <= 5; ++r) {
        NLSemanticBackingView backing;
        NLSemanticDomainView domain;
        CHECK(nl_semantic_backing_view(two.entry, r, &backing) &&
              nl_semantic_domain_view(two.entry, r, &domain) &&
              backing.live == (r == 3 || r == 5) &&
              domain.live == backing.live);
    }
    NLTwoRootDefinition summary;
    CHECK(nl_semantic_two_root_definition(
              two.entry, nl_checked_node_view(f, terminal_id)->function,
              &summary) &&
          summary.definition_checked && summary.count == 2 &&
          summary.calls[0].member != summary.calls[1].member);
    for (size_t i = 0; i < 2; ++i) {
        size_t r, a, d;
        NLPlaceId root;
        NLIncarnationId incarnation;
        NLSemanticPlaceView ended;
        CHECK(packet(two.entry, terminal_whole.fields[i], &r, &a, &d, &root,
                     &incarnation) &&
              r == rr[i] && a == aa[i] && d == dd[i] && root == roots[i] &&
              incarnation == inc[i] &&
              nl_semantic_place_view(two.returned, root, &ended) &&
              !ended.live &&
              summary.calls[i].definition.requirements ==
                  NL_OWNER_ALL_REQUIREMENTS);
    }
    NLSemanticBindingView after;
    CHECK(nl_semantic_binding_view(two.returned, two.donor, &after) &&
          after.availability == NL_CONSUMED &&
          nl_semantic_binding_view(two.returned, two.parameter, &after) &&
          after.availability == NL_CONSUMED);
    size_t order[5] = {0}, freed = 0, terminals = 0;
    CHECK(releases(f, order, &freed, &terminals, 0) && freed == 5 &&
          order[0] == 2 && order[1] == 4 && order[2] == 1 && order[3] == 3 &&
          order[4] == 5);
    char *out = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(entry, &out, &length) == NL_NODE_C_UNSUPPORTED &&
          !out && length == 777);
    printf(
        "{\"pure_call\":%zu,\"terminal_call\":%zu,\"owner_inputs\":[%zu,%zu],"
        "\"result\":%zu,"
        "\"donors\":[%zu,%zu],\"formals\":[%zu,%zu],\"receiver\":%zu,"
        "\"roots\":[%zu,%zu],\"incarnations\":[%zu,%zu],\"regions\":[%zu,%zu],"
        "\"Allocation_values\":[%zu,%zu],\"Domain_values\":[%zu,%zu],"
        "\"inert_Copy_token_no_authority\":true,\"three_donors_dead_before_"
        "terminal\":true,"
        "\"sole_returned_owner_live_A_D\":[2,2],\"exact_result_is_only_"
        "terminal_argument\":true,"
        "\"all_5_roots_and_20_siblings_preserved_through_pure_call\":true,\"B_"
        "dst_current_unchanged_during_donor_frees\":true,\"two_root_validator_"
        "status\":0,\"release_order\":[2,4,1,3,5],\"backend\":\"unsupported\"}"
        "\n",
        pure_id, terminal_id, w.inputs[0], w.inputs[1], w.result, w.donors[0],
        w.donors[1], w.parameters[0], w.parameters[1], w.receiver, roots[0],
        roots[1], inc[0], inc[1], rr[0], rr[1], allocations[0][4],
        allocations[0][2], domains[0][4], domains[0][2]);
    return true;
}

static bool sample_validator_oom(const NLCheckedFragment *entry)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    const NLCheckedFragment *parent = NULL;
    NLCheckedNodeId last_match = 0;
    for (size_t i = 0; i < 5; ++i) {
        parent = f;
        last_match = allocation_match(f);
        f = nl_checked_match_arm(f, last_match, 1);
    }
    NLCheckedNodeId ids[2] = {0};
    size_t count = 0;
    NLScopedFieldCallView view;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_scoped_field_call_view(f, i, &view)) {
            CHECK(count < 2);
            ids[count++] = i;
        }
    CHECK(count == 2);
    NLCheckedNodeId terminal = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        NLTwoRootCallView two;
        if (nl_checked_two_root_call_view(f, i, &two))
            terminal = i;
    }
    CHECK(terminal);
    for (size_t ordinal = 0; ordinal < 4; ++ordinal) {
        const NLCheckedNodeId call = ordinal < 2 ? ids[ordinal] : last_match;
        size_t total = 0;
        for (size_t test = 0; test < 8; ++test) {
            const size_t points[] = {0,
                                     1,
                                     2,
                                     total / 4,
                                     total / 2,
                                     total * 3 / 4,
                                     total ? total - 1 : 0};
            NLSemanticSnapshot before, after;
            CHECK(nl_semantic_snapshot(nl_checked_context(f), &before));
            allocations = 0;
            failure =
                test ? (total && points[test - 1] >= total ? total - 1
                                                           : points[test - 1])
                     : SIZE_MAX;
            injecting = true;
            const NLCheckStatus status =
                ordinal < 2 ? nl_checked_scoped_field_call_validate(f, call)
                : ordinal == 2
                    ? nl_checked_captured_closure_validate(parent, last_match)
                    : nl_checked_two_root_call_validate(f, terminal);
            injecting = false;
            if (!test) {
                total = allocations;
                CHECK(status == NL_CHECK_OK);
                if (ordinal == 3 && total == 0) {
                    printf("{\"two_root_validator_allocation_count\":0,\"OOM_"
                           "injection_applicable\":false}\n");
                    break;
                }
                CHECK(total > 0);
            } else {
                CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
                      nl_semantic_snapshot(nl_checked_context(f), &after) &&
                      !memcmp(&before, &after, sizeof(before)) &&
                      (ordinal < 2
                           ? nl_checked_scoped_field_call_validate(f, call)
                       : ordinal == 2
                           ? nl_checked_captured_closure_validate(parent,
                                                                  last_match)
                           : nl_checked_two_root_call_validate(f, terminal)) ==
                          NL_CHECK_OK);
            }
        }
        if (ordinal == 3 && total == 0)
            continue;
        printf("{\"oom_stage\":2,\"call_ordinal\":%zu,\"baseline_allocations\":"
               "%zu,\"injected_"
               "failures\":7,\"rollback_retry\":true}\n",
               ordinal, total);
    }
    return true;
}

static bool sample_oom(const NLSyntaxTree *syntax,
                       const NLSyntaxTree *entry_syntax)
{
    const NLSyntaxTree *units[] = {syntax};
    for (size_t stage = 0; stage < 2; ++stage) {
        size_t total = 0;
        for (size_t test = 0; test < 8; ++test) {
            NLSemanticContext *c = NULL;
            NLCheckedFragment *entry = NULL;
            NLFunctionUnitDiagnostic d = {0};
            NLCheckDiagnostic e = {0};
            CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
            if (stage)
                CHECK(nl_semantic_register_function_unit(c, units, 1, &d) ==
                      NL_CHECK_OK);
            NLSemanticSnapshot before, after;
            CHECK(nl_semantic_snapshot(c, &before));
            allocations = 0;
            const size_t points[] = {0,
                                     1,
                                     2,
                                     total / 4,
                                     total / 2,
                                     total * 3 / 4,
                                     total ? total - 1 : 0};
            failure = test ? points[test - 1] : SIZE_MAX;
            injecting = true;
            NLCheckStatus status =
                stage
                    ? nl_semantic_check_expression(c, entry_syntax, &entry, &e)
                    : nl_semantic_register_function_unit(c, units, 1, &d);
            injecting = false;
            if (!test) {
                total = allocations;
                CHECK(status == NL_CHECK_OK && total > 8);
            } else {
                CHECK(status == NL_CHECK_OUT_OF_MEMORY && !entry &&
                      nl_semantic_snapshot(c, &after) &&
                      !memcmp(&before, &after, sizeof(before)));
                if (stage)
                    CHECK(nl_semantic_check_expression(c, entry_syntax, &entry,
                                                       &e) == NL_CHECK_OK &&
                          entry);
                else
                    CHECK(nl_semantic_register_function_unit(c, units, 1, &d) ==
                          NL_CHECK_OK);
            }
            nl_checked_destroy(entry);
            nl_semantic_destroy(c);
        }
        printf("{\"oom_stage\":%zu,\"baseline_allocations\":%zu,\"injected_"
               "failures\":7,\"rollback_retry\":true}\n",
               stage, total);
    }
    return true;
}
/* Refused source must not poison a context reused for a valid source. */
static bool rejected_registration(NLSemanticContext *c, const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *syntax = NULL;
    NLSemanticSnapshot before, after;
    NLFunctionUnitDiagnostic diagnostic = {0};
    bool ok = false;
    if (nl_source_load(path, &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(parser, &syntax, NULL) != NL_PARSE_OK ||
        !nl_semantic_snapshot(c, &before))
        goto done;
    const NLSyntaxTree *units[] = {syntax};
    ok = nl_semantic_register_function_unit(c, units, 1, &diagnostic) !=
             NL_CHECK_OK &&
         nl_semantic_snapshot(c, &after) &&
         !memcmp(&before, &after, sizeof(before));
done:
    nl_syntax_tree_destroy(syntax);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok;
}
static bool run(const char *path, bool oom, const char *refused)
{
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *syntax = NULL, *entry_syntax = NULL;
    NLSemanticContext *c = NULL;
    NLCheckedFragment *entry = NULL;
    bool ok = false;
    if (nl_source_load(path, &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(parser, &syntax, NULL) != NL_PARSE_OK ||
        nl_semantic_create(&c) != NL_CHECK_OK)
        goto done;
    const NLSyntaxTree *units[] = {syntax};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckDiagnostic e = {0};
    if (refused && !rejected_registration(c, refused))
        goto done;
    if (nl_semantic_register_function_unit(c, units, 1, &d) != NL_CHECK_OK) {
        (void)nl_check_diagnostic_render(stderr, source, &d.diagnostic);
        goto done;
    }
    if (nl_source_create("main()", 6, "<P329-entry>", &entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(entry_parser, &entry_syntax,
                                            NULL) != NL_PARSE_OK ||
        nl_semantic_check_expression(c, entry_syntax, &entry, &e) !=
            NL_CHECK_OK ||
        !entry)
        goto done;
    printf("{\"parse_status\":0,\"registration_status\":0,\"main_check_"
           "status\":0,\"owned_entry_artifact\":true}\n");
    evidence_source = source;
    ok = inspect(entry) && (!oom || (sample_oom(syntax, entry_syntax) &&
                                     sample_validator_oom(entry)));
    if (ok && refused)
        printf("{\"registration_refusal_rollback_retry\":true}\n");
done:
    nl_checked_destroy(entry);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(entry_syntax);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_syntax_tree_destroy(syntax);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok;
}
int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[2], "after-refusal"))
        return run(argv[1], false, argv[3]) ? EXIT_SUCCESS : EXIT_FAILURE;
    return (argc == 2 || (argc == 3 && !strcmp(argv[2], "oom"))) &&
                   run(argv[1], argc == 3, NULL)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
