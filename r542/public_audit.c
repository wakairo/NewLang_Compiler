/* R542 independently frozen public issuer consumer, NON-BLIND calibrated.
 * This source contains no seeded rights, private internal header or custom world.
 * All tamper probes clone the genuinely issued public view, never mutate f/world.
 */
#include "node_checked.h"
#include "newlang/array4.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define REQUIRE(expr, reason) do { \
    if (!(expr)) { fprintf(stderr, "R542 PUBLIC FAIL: %s at %d\n", reason, __LINE__); \
                   return false; } \
} while (0)

static bool audit(const char *path, const char *emitted_file)
{
    TestNode n = {0}, another = {0};
    REQUIRE(node_checked_load(path, &n), "source -> registration -> original checked call");
    const NLCheckedFragment *body = nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    REQUIRE(body && nl_checked_source(body), "real retained source-backed body");
    NLArray4Construction original = {0};
    REQUIRE(nl_checked_array4_view(body, &original), "captured public issuance");
    REQUIRE(original.origin == body && nl_checked_array4_valid(body, &original),
            "original issued view independently validated");
    REQUIRE(!nl_checked_array4_valid(body, NULL), "null public view rejected");
    REQUIRE(!nl_checked_array4_view(body, NULL), "null capture rejected");

    const NLSemanticContext *world = nl_checked_array4_world(body);
    NLSemanticSnapshot snapshot = {0};
    REQUIRE(world && nl_semantic_snapshot(world, &snapshot), "borrowed captured world");
    REQUIRE(snapshot.places == 5 && snapshot.bindings == 1 &&
            snapshot.occurrences == 0 && snapshot.domains == 0 &&
            snapshot.backing_regions == 0, "one original root, four children, no extra authority");
    NLSemanticTypeView at = {0}, ot = {0};
    REQUIRE(nl_semantic_type_view(world, original.array_type, &at) &&
            at.kind == NL_TYPE_ARRAY && at.target == original.element_type &&
            at.element_count == 4 && at.field_count == 4, "original structural Array4 type");
    REQUIRE(nl_semantic_type_view(world, original.element_type, &ot) &&
            ot.kind == NL_TYPE_SUM && ot.variant_count == 2,
            "original Option envelope type");

    const NLCheckedNodeView *binding =
        nl_checked_node_view(body, original.binding);
    const NLCheckedNodeView *constructor =
        nl_checked_node_view(body, original.constructor);
    const NLCheckedNodeView *receiver =
        nl_checked_node_view(body, original.receiver);
    REQUIRE(binding && constructor && receiver &&
            binding->kind == NL_CHECKED_BINDING &&
            constructor->kind == NL_CHECKED_ARRAY4_CONSTRUCTOR &&
            receiver->kind == NL_CHECKED_RECEIVER &&
            binding->initializer == original.constructor &&
            binding->first_argument == original.receiver &&
            receiver->symbol == original.symbol &&
            constructor->argument_count == 4, "lexical origin/receiver/constructor");
    NLSemanticBindingView bv = {0};
    REQUIRE(nl_semantic_binding_view(world, original.symbol, &bv) &&
            bv.place == original.root && bv.value == original.value &&
            bv.type == original.array_type && bv.availability == NL_AVAILABLE,
            "actual source binding consumes aggregate only once");
    NLSemanticPlaceView root = {0};
    NLSemanticValueView root_value = {0};
    REQUIRE(nl_semantic_place_view(world, original.root, &root) &&
            nl_semantic_value_view(world, original.value, &root_value),
            "root public semantic views");
    REQUIRE(root.live && root.independent_root && root.implicit_local &&
            root.parent_aggregate == 0 && root.parent_sum == 0 &&
            root.fixed_field_count == 4 &&
            root.incarnation == original.incarnation &&
            root.current_fact == original.fact &&
            root.current_value == original.value &&
            root_value.carrier == NL_CARRIER_PLACE &&
            root_value.owner_place == original.root &&
            root_value.field_count == 4 &&
            root_value.dependencies == NL_DEPENDENCY_FREE,
            "one actual governed root, one aggregate value");

    size_t independent_roots = 0, live = 0;
    for (size_t id = 1; id <= snapshot.places; ++id) {
        NLSemanticPlaceView v = {0};
        REQUIRE(nl_semantic_place_view(world, id, &v), "all public places exposed");
        independent_roots += v.independent_root ? 1u : 0u;
        live += v.live ? 1u : 0u;
    }
    REQUIRE(independent_roots == 1 && live == 5, "no independently minted child roots");
    for (size_t i = 0; i < 4; ++i) {
        const NLArray4Element e = original.elements[i];
        NLSemanticPlaceView child = {0};
        NLSemanticValueView value = {0};
        const NLCheckedNodeView *leaf = nl_checked_node_view(body, e.initializer);
        REQUIRE(nl_semantic_place_view(world, e.place, &child) &&
                nl_semantic_value_view(world, e.value, &value) && leaf,
                "source-built child views");
        REQUIRE(e.index == i && child.live && !child.independent_root &&
                child.type == original.element_type &&
                child.parent_aggregate == original.root &&
                child.parent_incarnation == original.incarnation &&
                child.parent_field_index == i &&
                root.fixed_fields[i] == e.place &&
                child.incarnation == e.incarnation &&
                child.current_fact == e.fact && child.current_value == e.value,
                "actual qualified indexed dependent projection");
        REQUIRE(value.type == original.element_type &&
                value.carrier == NL_CARRIER_AGGREGATE &&
                value.aggregate_owner == original.value &&
                value.owner_place == 0 && root_value.fields[i] == e.value &&
                value.variant == 1 && value.sum_payload == 0 &&
                child.payload_occurrence == 0 &&
                leaf->kind == NL_CHECKED_SUM_CONSTRUCTOR &&
                leaf->variant == 1 && leaf->type == original.element_type &&
                leaf->argument_count == 0 && leaf->initializer == 0,
                "none value package is exclusively dependent, payload-free");
        for (size_t k = 0; k < i; ++k)
            REQUIRE(e.place != original.elements[k].place &&
                    e.incarnation != original.elements[k].incarnation &&
                    e.fact != original.elements[k].fact &&
                    e.value != original.elements[k].value &&
                    e.initializer != original.elements[k].initializer,
                    "all four independently authored leaf projections");
        printf("PUBLIC_CHILD index=%zu place=%zu inc=%zu fact=%zu value=%zu "
               "source_node=%zu parent=%zu parent_inc=%zu variant=%zu\n",
               i, e.place, e.incarnation, e.fact, e.value, e.initializer,
               child.parent_aggregate, child.parent_incarnation, value.variant);
    }
    NLSemanticPlaceView after = {0};
    REQUIRE(nl_semantic_place_view(n.context, original.root, &after) &&
            !after.live && after.incarnation == original.incarnation,
            "captured historical issuance != still-live final authority");

    size_t mutations_refused = 0;
    NLArray4Construction altered;
#define REJECT(mutation) do { \
    altered = original; mutation; \
    REQUIRE(!nl_checked_array4_valid(body, &altered), "altered public evidence must reject"); \
    REQUIRE(nl_checked_array4_valid(body, &original), "immutable original must remain valid"); \
    ++mutations_refused; \
} while (0)
    REJECT(altered.origin = NULL);
    REJECT(altered.binding = original.constructor);
    REJECT(altered.constructor = original.binding);
    REJECT(altered.receiver = 0);
    REJECT(altered.symbol = 0);
    REJECT(altered.array_type = original.element_type);
    REJECT(altered.element_type = original.array_type);
    REJECT(altered.root = 0);
    REJECT(altered.root = original.elements[0].place);
    REJECT(++altered.incarnation);
    REJECT(++altered.fact);
    REJECT(altered.value = original.elements[0].value);
    for (size_t i = 0; i < 4; ++i) {
        const size_t other = (i + 1u) % 4u;
        REJECT(altered.elements[i].index = other);
        REJECT(altered.elements[i].initializer = original.elements[other].initializer);
        REJECT(altered.elements[i].place = original.root);
        REJECT(altered.elements[i].place = original.elements[other].place);
        REJECT(++altered.elements[i].incarnation);
        REJECT(++altered.elements[i].fact);
        REJECT(altered.elements[i].value = original.elements[other].value);
        REJECT(altered.elements[i] = original.elements[other]);
    }
    REQUIRE(node_checked_load(path, &another), "independent separate source proof");
    const NLCheckedFragment *foreign =
        nl_checked_call_body(another.entry, nl_checked_root(another.entry));
    REQUIRE(foreign && foreign != body, "nonidentical fragment identity");
    REJECT(altered.origin = foreign);
    REQUIRE(!nl_checked_array4_valid(foreign, &original),
            "same numeric IDs in different source world do not grant authority");
#undef REJECT

    char *issued_c = NULL;
    size_t issued_n = 0;
    REQUIRE(nl_checked_array4_c17(n.entry, &issued_c, &issued_n) == NL_CHECK_OK &&
            issued_c != NULL && issued_n > 0, "authenticated public source->C17 export");
    FILE *out = fopen(emitted_file, "wb");
    REQUIRE(out != NULL, "open public-owned C17 comparison output");
    REQUIRE(fwrite(issued_c, 1, issued_n, out) == issued_n &&
            fclose(out) == 0, "exact published C17 buffer");
    free(issued_c);

    printf("PUBLIC_OK root=%zu incarnation=%zu fact=%zu value=%zu "
           "array_type=%zu option_type=%zu symbol=%zu "
           "independent_roots=%zu live_places=%zu children=4 "
           "mutants_rejected=%zu source_AST_torn_down=1 final_root_ended=1\n",
           original.root, original.incarnation, original.fact, original.value,
           original.array_type, original.element_type, original.symbol,
           independent_roots, live, mutations_refused);
    node_checked_destroy(&another);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && audit(argv[1], argv[2]) ? 0 : 1;
}
