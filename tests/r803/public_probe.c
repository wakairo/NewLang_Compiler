/* R803: public observations of independently frozen real registered source. */
#include "../support/node_checked.h"
#include "newlang/array4.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool reject_modified(const NLCheckedFragment *f, size_t i,
                            const NLArray4Construction *good,
                            const NLCheckedFragment *foreign)
{
    NLArray4Construction bad = *good;
    bad.root = good->elements[0].place;
    CHECK(!nl_checked_array4_valid_at(f, i, &bad));
    bad = *good;
    ++bad.incarnation;
    CHECK(!nl_checked_array4_valid_at(f, i, &bad));
    bad = *good;
    ++bad.fact;
    CHECK(!nl_checked_array4_valid_at(f, i, &bad));
    bad = *good;
    ++bad.elements[0].fact;
    CHECK(!nl_checked_array4_valid_at(f, i, &bad));
    bad = *good;
    bad.elements[2].place = good->elements[0].place;
    CHECK(!nl_checked_array4_valid_at(f, i, &bad));
    bad = *good;
    bad.elements[3].index = 0;
    CHECK(!nl_checked_array4_valid_at(f, i, &bad));
    bad = *good;
    bad.origin = foreign;
    CHECK(!nl_checked_array4_valid_at(f, i, &bad));
    return nl_checked_array4_valid_at(f, i, good);
}

static bool inspect(const char *path, size_t expected)
{
    TestNode n = {0}, foreign = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL && nl_checked_array4_count(f) == expected);
    NLArray4Construction roots[2];
    for (size_t i = 0; i < expected; ++i)
        CHECK(nl_checked_array4_view_at(f, i, &roots[i]) &&
              nl_checked_array4_valid_at(f, i, &roots[i]));
    CHECK(node_checked_load(path, &foreign));
    const NLCheckedFragment *other =
        nl_checked_call_body(foreign.entry, nl_checked_root(foreign.entry));
    CHECK(other != NULL && other != f &&
          nl_checked_array4_count(other) == expected);
    size_t same_number = 0;
    for (size_t i = 0; i < expected; ++i) {
        NLArray4Construction peer;
        CHECK(nl_checked_array4_view_at(other, i, &peer));
        CHECK(!nl_checked_array4_valid_at(f, i, &peer) &&
              !nl_checked_array4_valid_at(other, i, &roots[i]));
        same_number += peer.root == roots[i].root &&
                       peer.incarnation == roots[i].incarnation;
        CHECK(reject_modified(f, i, &roots[i], other));
        const NLSemanticContext *world = nl_checked_array4_world_at(f, i);
        NLSemanticSnapshot snapshot;
        CHECK(world != NULL && nl_semantic_snapshot(world, &snapshot) &&
              snapshot.places == 5 * (i + 1) &&
              snapshot.values == 5 * (i + 1) && snapshot.bindings == i + 1 &&
              !snapshot.domains && !snapshot.backing_regions &&
              !snapshot.occurrences);
        size_t original_count = 0;
        for (size_t id = 1; id <= snapshot.places; ++id) {
            NLSemanticPlaceView place;
            CHECK(nl_semantic_place_view(world, id, &place));
            CHECK(place.live);
            original_count += place.independent_root;
        }
        CHECK(original_count == i + 1);
        for (size_t j = 0; j <= i; ++j) {
            const NLArray4Construction *p = &roots[j];
            NLSemanticPlaceView parent;
            CHECK(nl_semantic_place_view(world, p->root, &parent) &&
                  parent.live && parent.independent_root &&
                  parent.implicit_local && parent.type == p->array_type &&
                  parent.incarnation == p->incarnation &&
                  parent.current_fact == p->fact &&
                  parent.current_value == p->value &&
                  parent.fixed_field_count == 4);
            for (size_t e = 0; e < 4; ++e) {
                NLSemanticPlaceView child;
                NLSemanticValueView value;
                CHECK(nl_semantic_place_view(world, p->elements[e].place,
                                             &child) &&
                      nl_semantic_value_view(world, p->elements[e].value,
                                             &value));
                CHECK(child.live && !child.independent_root &&
                      child.type == p->element_type &&
                      child.parent_aggregate == p->root &&
                      child.parent_incarnation == p->incarnation &&
                      child.parent_field_index == e &&
                      child.incarnation == p->elements[e].incarnation &&
                      child.current_fact == p->elements[e].fact &&
                      child.current_value == p->elements[e].value &&
                      !child.payload_occurrence && value.variant == 1 &&
                      !value.sum_payload &&
                      value.carrier == NL_CARRIER_AGGREGATE &&
                      value.aggregate_owner == p->value);
            }
        }
    }
    if (expected == 2) {
        CHECK(roots[0].root != roots[1].root &&
              roots[0].incarnation != roots[1].incarnation &&
              roots[0].symbol != roots[1].symbol &&
              nl_checked_array4_world_at(f, 0) !=
                  nl_checked_array4_world_at(f, 1));
        for (size_t a = 0; a < 4; ++a)
            for (size_t b = 0; b < 4; ++b)
                CHECK(roots[0].elements[a].place != roots[1].elements[b].place);
    }
    for (size_t i = 0; i < expected; ++i) {
        NLSemanticPlaceView ended;
        CHECK(nl_semantic_place_view(n.context, roots[i].root, &ended) &&
              !ended.live);
    }
    printf("{\"count\":%zu,\"array_type\":%zu,\"element_type\":%zu,"
           "\"same_number_foreign_rejected\":%zu,\"roots\":[",
           expected, roots[0].array_type, roots[0].element_type, same_number);
    for (size_t i = 0; i < expected; ++i)
        printf("%s{\"symbol\":%zu,\"root\":%zu,\"incarnation\":%zu,"
               "\"child_count\":4}",
               i ? "," : "", roots[i].symbol, roots[i].root,
               roots[i].incarnation);
    printf("],\"mutations_rejected\":%zu}\n", expected * 9);
    node_checked_destroy(&foreign);
    node_checked_destroy(&n);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return EXIT_FAILURE;
    const size_t expected = (size_t)strtoul(argv[2], NULL, 10);
    return (expected == 1 || expected == 2) && inspect(argv[1], expected)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
