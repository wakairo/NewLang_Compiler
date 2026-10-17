/* Actual-source construction, public AST/type/owned/checkpoint APIs only.
 * No seeded value/place, host constructed semantic world or new authority. */
#include "../support/node_checked.h"
#include "newlang/array4.h"
#include <stdlib.h>
static bool probe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    NLArray4Construction p;
    CHECK(nl_checked_array4_view(f, &p) && nl_checked_array4_valid(f, &p));
    const NLSemanticContext *world = nl_checked_array4_world(f);
    NLSemanticSnapshot snapshot;
    NLSemanticTypeView t, option;
    CHECK(nl_semantic_snapshot(world, &snapshot) && snapshot.places == 5 &&
          snapshot.bindings == 1 && snapshot.occurrences == 0 &&
          snapshot.domains == 0 && snapshot.backing_regions == 0);
    CHECK(nl_semantic_type_view(world, p.array_type, &t) &&
          t.kind == NL_TYPE_ARRAY && t.element_count == 4 &&
          t.field_count == 4 && t.target == p.element_type && t.is_copy &&
          t.is_discardable);
    CHECK(nl_semantic_type_view(world, p.element_type, &option) &&
          option.kind == NL_TYPE_SUM && option.is_copy &&
          option.is_discardable && option.variant_count == 2);
    size_t roots = 0, live = 0;
    for (size_t i = 1; i <= snapshot.places; ++i) {
        NLSemanticPlaceView v;
        CHECK(nl_semantic_place_view(world, i, &v));
        roots += v.independent_root;
        live += v.live;
    }
    CHECK(roots == 1 && live == 5);
    NLArray4Construction bad;
    size_t rejects = 0;
#define REFUSE(change)                                                         \
    do {                                                                       \
        bad = p;                                                               \
        change;                                                                \
        CHECK(!nl_checked_array4_valid(f, &bad) &&                             \
              nl_checked_array4_valid(f, &p));                                 \
        ++rejects;                                                             \
    } while (0)
    REFUSE(bad.root = 0);
    REFUSE(bad.root = p.elements[0].place);
    REFUSE(++bad.incarnation);
    REFUSE(++bad.fact);
    REFUSE(bad.array_type = p.element_type);
    REFUSE(bad.element_type = p.array_type);
    REFUSE(bad.value = p.elements[0].value);
    REFUSE(bad.symbol = 0);
    REFUSE(bad.constructor = bad.binding);
    REFUSE(bad.receiver = 0);
    REFUSE(bad.origin = NULL);
    for (size_t i = 0; i < 4; ++i) {
        REFUSE(bad.elements[i].place = p.root);
        REFUSE(bad.elements[i].place = p.elements[(i + 1) % 4].place);
        REFUSE(bad.elements[i].fact = p.elements[(i + 1) % 4].fact);
        REFUSE(++bad.elements[i].incarnation);
        REFUSE(bad.elements[i].value = p.elements[(i + 1) % 4].value);
        REFUSE(bad.elements[i].index = (i + 1) % 4);
        REFUSE(bad.elements[i].initializer =
                   p.elements[(i + 1) % 4].initializer);
    }
    TestNode other = {0};
    CHECK(node_checked_load(path, &other));
    const NLCheckedFragment *foreign =
        nl_checked_call_body(other.entry, nl_checked_root(other.entry));
    REFUSE(bad.origin = foreign);
    CHECK(!nl_checked_array4_valid(foreign, &p));
#undef REFUSE
    NLSemanticPlaceView ended;
    CHECK(nl_semantic_place_view(n.context, p.root, &ended) && !ended.live);
    printf("{\"array_type\":%zu,\"element_type\":%zu,\"symbol\":%zu,\"root\":%"
           "zu,\"incarnation\":%zu,\"fact\":%zu,\"value\":%zu,\"live_"
           "checkpoint_places\":%zu,\"independent_roots\":%zu,\"elements\":[",
           p.array_type, p.element_type, p.symbol, p.root, p.incarnation,
           p.fact, p.value, live, roots);
    for (size_t i = 0; i < 4; ++i) {
        NLSemanticPlaceView v;
        NLSemanticValueView x;
        CHECK(nl_semantic_place_view(world, p.elements[i].place, &v) &&
              nl_semantic_value_view(world, p.elements[i].value, &x));
        CHECK(v.parent_aggregate == p.root &&
              v.parent_incarnation == p.incarnation &&
              v.parent_field_index == i && v.live && !v.independent_root &&
              x.variant == 1 && !x.sum_payload);
        printf("%s{\"index\":%zu,\"place\":%zu,\"incarnation\":%zu,\"fact\":%"
               "zu,\"value\":%zu,\"source_node\":%zu,\"variant\":%zu}",
               i ? "," : "", i, p.elements[i].place, p.elements[i].incarnation,
               p.elements[i].fact, p.elements[i].value,
               p.elements[i].initializer, x.variant);
    }
    printf("],\"separate_view_mutants_rejected\":%zu,\"final_root_ended\":true,"
           "\"public_validator\":true}\n",
           rejects);
    node_checked_destroy(&other);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
