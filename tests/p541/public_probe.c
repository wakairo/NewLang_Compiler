/* P541 CALIBRATED: actual source only; all positive observations use public
 * owned APIs after original input source/AST teardown. No semantic seed. */
#include "../support/node_checked.h"
#include "newlang/array4.h"
#include <stdint.h>
static bool probe(const char *path)
{
    TestNode n = {0}, foreign = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(nl_checked_array4_count(f) == 2);
    NLArray4Construction p[2], bad;
    for (size_t i = 0; i < 2; ++i)
        CHECK(nl_checked_array4_view_at(f, i, &p[i]) &&
              nl_checked_array4_valid_at(f, i, &p[i]));
    CHECK(p[0].root != p[1].root && p[0].incarnation != p[1].incarnation &&
          p[0].fact != p[1].fact && p[0].value != p[1].value &&
          p[0].symbol != p[1].symbol && p[0].binding != p[1].binding &&
          p[0].constructor != p[1].constructor &&
          p[0].array_type == p[1].array_type &&
          p[0].element_type == p[1].element_type &&
          nl_checked_array4_world_at(f, 0) != nl_checked_array4_world_at(f, 1));
    bad = p[0];
    bad.root = SIZE_MAX;
    CHECK(!nl_checked_array4_view(f, &bad) && bad.root == SIZE_MAX &&
          nl_checked_array4_world(f) == NULL &&
          !nl_checked_array4_valid(f, &p[0]) &&
          !nl_checked_array4_view_at(f, 2, &bad) && bad.root == SIZE_MAX &&
          nl_checked_array4_world_at(f, 2) == NULL &&
          !nl_checked_array4_valid_at(f, 2, &p[0]));
    CHECK(node_checked_load(path, &foreign));
    const NLCheckedFragment *other =
        nl_checked_call_body(foreign.entry, nl_checked_root(foreign.entry));
    size_t rejected = 0;
    for (size_t i = 0; i < 2; ++i) {
        const NLSemanticContext *world = nl_checked_array4_world_at(f, i);
        NLSemanticSnapshot snapshot;
        NLSemanticTypeView array, option;
        CHECK(nl_semantic_snapshot(world, &snapshot) &&
              snapshot.places == 5 * (i + 1) &&
              snapshot.values == 5 * (i + 1) && snapshot.bindings == i + 1 &&
              !snapshot.domains && !snapshot.backing_regions &&
              !snapshot.scopes && !snapshot.occurrences);
        CHECK(nl_semantic_type_view(world, p[i].array_type, &array) &&
              array.kind == NL_TYPE_ARRAY && array.element_count == 4 &&
              array.target == p[i].element_type && array.is_copy &&
              array.is_discardable &&
              nl_semantic_type_view(world, p[i].element_type, &option) &&
              option.kind == NL_TYPE_SUM && option.variant_count == 2 &&
              option.is_copy && option.is_discardable);
        size_t roots = 0, live = 0;
        for (size_t id = 1; id <= snapshot.places; ++id) {
            NLSemanticPlaceView v;
            CHECK(nl_semantic_place_view(world, id, &v));
            roots += v.independent_root;
            live += v.live;
        }
        CHECK(roots == i + 1 && live == snapshot.places);
        for (size_t j = 0; j <= i; ++j) {
            NLSemanticPlaceView root;
            CHECK(nl_semantic_place_view(world, p[j].root, &root) &&
                  root.live && root.independent_root && root.implicit_local &&
                  root.incarnation == p[j].incarnation &&
                  root.current_fact == p[j].fact &&
                  root.current_value == p[j].value && !root.parent_aggregate &&
                  !root.parent_sum && !root.governing_domain &&
                  !root.placement.region);
            for (size_t e = 0; e < 4; ++e) {
                NLSemanticPlaceView child;
                NLSemanticValueView value;
                CHECK(nl_semantic_place_view(world, p[j].elements[e].place,
                                             &child) &&
                      nl_semantic_value_view(world, p[j].elements[e].value,
                                             &value));
                CHECK(child.live && !child.independent_root &&
                      child.parent_aggregate == p[j].root &&
                      child.parent_incarnation == p[j].incarnation &&
                      child.parent_field_index == e &&
                      child.incarnation == p[j].elements[e].incarnation &&
                      child.current_fact == p[j].elements[e].fact &&
                      child.current_value == p[j].elements[e].value &&
                      !child.payload_occurrence &&
                      value.type == p[j].element_type && value.variant == 1 &&
                      !value.sum_payload &&
                      value.carrier == NL_CARRIER_AGGREGATE &&
                      value.aggregate_owner == p[j].value);
            }
        }
#define REFUSE(change)                                                         \
    do {                                                                       \
        bad = p[i];                                                            \
        change;                                                                \
        CHECK(!nl_checked_array4_valid_at(f, i, &bad) &&                       \
              nl_checked_array4_valid_at(f, i, &p[i]));                        \
        ++rejected;                                                            \
    } while (0)
        const NLArray4Construction peer = p[1 - i];
        REFUSE(bad.root = 0);
        REFUSE(bad.root = peer.root);
        REFUSE(bad.symbol = peer.symbol);
        REFUSE(bad.binding = peer.binding);
        REFUSE(bad.constructor = peer.constructor);
        REFUSE(bad.receiver = peer.receiver);
        REFUSE(bad.value = peer.value);
        REFUSE(bad.incarnation = peer.incarnation);
        REFUSE(bad.fact = peer.fact);
        REFUSE(bad.array_type = bad.element_type);
        REFUSE(bad.element_type = bad.array_type);
        REFUSE(bad.origin = NULL);
        REFUSE(bad.origin = other);
        REFUSE(bad = peer);
        for (size_t e = 0; e < 4; ++e) {
            REFUSE(bad.elements[e].place = p[i].root);
            REFUSE(bad.elements[e].place = p[i].elements[(e + 1) % 4].place);
            REFUSE(bad.elements[e].place = peer.elements[e].place);
            REFUSE(bad.elements[e].fact = peer.elements[e].fact);
            REFUSE(bad.elements[e].value = peer.elements[e].value);
            REFUSE(bad.elements[e].incarnation = peer.elements[e].incarnation);
            REFUSE(bad.elements[e].initializer = peer.elements[e].initializer);
            REFUSE(bad.elements[e].index = (e + 1) % 4);
            REFUSE(bad.elements[e] = peer.elements[e]);
        }
        CHECK(!nl_checked_array4_valid_at(other, i, &p[i]));
#undef REFUSE
        NLSemanticPlaceView ended;
        NLSemanticBindingView binding;
        CHECK(nl_semantic_place_view(n.context, p[i].root, &ended) &&
              !ended.live &&
              nl_semantic_binding_view(n.context, p[i].symbol, &binding) &&
              binding.availability == NL_CONSUMED);
    }
    const NLCheckedNodeView *entry =
        nl_checked_node_view(n.entry, nl_checked_root(n.entry));
    printf("{\"record_count\":2,\"source_function\":%zu,"
           "\"public_validator\":true,\"legacy_singleton_refused\":true,"
           "\"worlds_distinct\":true,\"final_roots_ended\":true,"
           "\"view_mutants_rejected\":%zu,\"records\":[",
           entry->function, rejected);
    for (size_t i = 0; i < 2; ++i) {
        printf("%s{\"index\":%zu,\"array_type\":%zu,\"element_type\":%zu,"
               "\"symbol\":%zu,\"binding\":%zu,\"constructor\":%zu,"
               "\"root\":%zu,\"incarnation\":%zu,\"fact\":%zu,"
               "\"value\":%zu,\"checkpoint_places\":%zu,"
               "\"checkpoint_roots\":%zu,\"elements\":[",
               i ? "," : "", i, p[i].array_type, p[i].element_type, p[i].symbol,
               p[i].binding, p[i].constructor, p[i].root, p[i].incarnation,
               p[i].fact, p[i].value, 5 * (i + 1), i + 1);
        for (size_t e = 0; e < 4; ++e) {
            const NLArray4Element x = p[i].elements[e];
            printf("%s{\"index\":%zu,\"place\":%zu,\"incarnation\":%zu,"
                   "\"fact\":%zu,\"value\":%zu,\"source_node\":%zu}",
                   e ? "," : "", e, x.place, x.incarnation, x.fact, x.value,
                   x.initializer);
        }
        printf("]}");
    }
    printf("]}\n");
    node_checked_destroy(&foreign);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
