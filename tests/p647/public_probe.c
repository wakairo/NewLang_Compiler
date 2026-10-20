/* Public observations of actual registered source after syntax teardown.
 * No semantic seed, proof mutation, private API or guest/native injection. */
#include "../support/node_checked.h"
static bool observe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSource *owned_source = nl_checked_source(f);
    NLSourceView bytes;
    CHECK(nl_source_view(owned_source,
                         (NLSourceSpan){0, nl_source_length(owned_source)},
                         &bytes));
    fputs("{\"kind\":\"owned_source\",\"hex\":\"", stdout);
    for (size_t i = 0; i < bytes.length; ++i)
        printf("%02x", (unsigned char)bytes.bytes[i]);
    puts("\"}");
    const NLSemanticContext *c = nl_checked_context(f);
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(c, &snapshot) && snapshot.scopes == 2 &&
          snapshot.domains == 0 && snapshot.backing_regions == 0);
    NLPlaceId roots[2] = {0}, children[2][2] = {{0}};
    NLIncarnationId incarnation[2] = {0}, child_incarnation[2][2] = {{0}};
    NLValueFactId sibling_fact[2] = {0}, post_fact[2] = {0},
                  post_parent[2] = {0};
    NLCheckedNodeId first_if = 0;
    size_t aggregates = 0, choices = 0, writes = 0, loans = 0, reads = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        CHECK(v != NULL);
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, v->initializer);
            if (init && init->kind == NL_CHECKED_AGGREGATE) {
                NLSemanticBindingView binding;
                NLSemanticPlaceView place;
                CHECK(aggregates < 2 &&
                      nl_semantic_binding_view(c, v->symbol, &binding) &&
                      nl_semantic_place_view(c, binding.place, &place) &&
                      !place.live && place.current_fact == 0);
                roots[aggregates++] = binding.place;
            }
        }
        if (v->kind == NL_CHECKED_IF) {
            if (!first_if)
                first_if = i;
            if (first_if == i) {
                const NLSemanticContext *w =
                    nl_checked_context(nl_checked_if_arm(f, i, 0));
                for (size_t k = 0; k < 2; ++k) {
                    NLSemanticPlaceView r;
                    CHECK(nl_semantic_place_view(w, roots[k], &r));
                    incarnation[k] = r.incarnation;
                    for (size_t j = 0; j < 2; ++j) {
                        NLSemanticPlaceView ch;
                        children[k][j] = r.fixed_fields[j];
                        CHECK(nl_semantic_place_view(w, children[k][j], &ch));
                        child_incarnation[k][j] = ch.incarnation;
                        if (j == 1)
                            sibling_fact[k] = ch.current_fact;
                    }
                }
            }
            const NLCheckedNodeView *cond =
                nl_checked_node_view(f, v->initializer);
            NLSemanticValueView truth, result;
            CHECK(cond && v->normal_arms == 2 && v->result_count == 1 &&
                  nl_semantic_value_view(c, cond->results[0].value, &truth) &&
                  truth.scalar_known &&
                  truth.scalar_value == (choices == 0 ? 1 : 0) &&
                  nl_semantic_value_view(c, v->results[0].value, &result) &&
                  !result.scalar_known && result.domain == 0 &&
                  result.allocation_region == 0 && result.reference.place == 0);
            const NLSemanticContext *worlds[2] = {0};
            size_t values[2] = {0};
            for (size_t a = 0; a < 2; ++a) {
                const NLCheckedFragment *arm = nl_checked_if_arm(f, i, a);
                CHECK(arm && nl_checked_source(arm) == nl_checked_source(f));
                worlds[a] = nl_checked_context(arm);
                const NLCheckedNodeView *r =
                    nl_checked_node_view(arm, nl_checked_root(arm));
                NLSemanticValueView value;
                CHECK(r && r->result_count == 1 &&
                      nl_semantic_value_view(worlds[a], r->results[0].value,
                                             &value) &&
                      value.scalar_known && value.domain == 0 &&
                      value.allocation_region == 0 &&
                      value.reference.place == 0);
                values[a] = value.scalar_value;
            }
            CHECK(worlds[0] != worlds[1] && worlds[0] != c && worlds[1] != c);
            CHECK(values[0] == (choices == 0 ? 72 : 91) &&
                  values[1] == (choices == 0 ? 8 : 14));
            printf("{\"kind\":\"if\",\"node\":%zu,\"span\":[%zu,%zu],"
                   "\"condition\":%zu,\"truth\":%zu,\"arms\":[%zu,%zu],"
                   "\"joined_unknown\":true,\"distinct_worlds\":true}\n",
                   i, v->span.start_byte, v->span.end_byte, v->initializer,
                   truth.scalar_value, values[0], values[1]);
            ++choices;
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER) {
            NLSemanticScopeView scope;
            CHECK(v->loan.implicit_local && v->loan.body_nonescape_proved &&
                  v->loan.normal_result_forwarded &&
                  nl_semantic_scope_view(c, v->loan.scope, &scope) &&
                  !scope.active);
            ++loans;
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            const NLCheckedField *e = &v->field;
            const NLCheckedNodeView *ref =
                nl_checked_node_view(f, v->first_argument);
            NLSemanticValueView old, reference;
            CHECK(
                writes < 2 && e->present && e->index == 0 &&
                e->parent == roots[writes] && e->access == NL_ACCESS_WRITE &&
                e->parent_incarnation == incarnation[writes] &&
                e->child == children[writes][0] &&
                e->child_incarnation == child_incarnation[writes][0] &&
                e->parent_fact != e->parent_post_fact &&
                e->child_fact != e->child_post_fact &&
                nl_semantic_value_view(c, e->old_value, &old) &&
                old.scalar_known &&
                old.scalar_value == (writes == 0 ? 14 : 33) && ref &&
                nl_semantic_value_view(c, ref->results[0].value, &reference) &&
                reference.reference.place == e->child &&
                reference.reference.incarnation == e->child_incarnation &&
                reference.reference.provenance == NL_PROVENANCE_VALID &&
                reference.reference.writable && reference.reference.readable &&
                reference.reference.scope == writes + 1 &&
                reference.carrier == NL_CARRIER_ENDED && reference.domain == 0);
            post_fact[writes] = e->child_post_fact;
            post_parent[writes] = e->parent_post_fact;
            printf("{\"kind\":\"change\",\"node\":%zu,\"root\":%zu,"
                   "\"field\":%zu,\"incarnation\":%zu,\"old\":%zu,"
                   "\"facts\":[%zu,%zu,%zu,%zu],\"scope\":%zu}\n",
                   i, e->parent, e->child, e->parent_incarnation,
                   old.scalar_value, e->parent_fact, e->parent_post_fact,
                   e->child_fact, e->child_post_fact,
                   reference.reference.scope);
            ++writes;
        }
        if (v->kind == NL_CHECKED_FIELD_READ) {
            NLSemanticValueView value;
            CHECK(v->field.present && v->result_count == 1 &&
                  nl_semantic_value_view(c, v->results[0].value, &value));
            const size_t k = v->field.parent == roots[0] ? 0 : 1;
            CHECK(v->field.parent == roots[k] && v->field.index < 2 &&
                  v->field.parent_incarnation == incarnation[k] &&
                  v->field.child == children[k][v->field.index] &&
                  v->field.child_incarnation ==
                      child_incarnation[k][v->field.index] &&
                  v->field.parent_fact == post_parent[k] &&
                  v->field.child_fact ==
                      (v->field.index == 1 ? sibling_fact[k] : post_fact[k]));
            if (v->field.index == 1)
                CHECK(value.scalar_known &&
                      value.scalar_value ==
                          (v->field.parent == roots[0] ? 181 : 207));
            printf("{\"kind\":\"read\",\"node\":%zu,\"root\":%zu,"
                   "\"field\":%zu,\"index\":%zu,\"known\":%s,\"value\":%zu}\n",
                   i, v->field.parent, v->field.child, v->field.index,
                   value.scalar_known ? "true" : "false", value.scalar_value);
            ++reads;
        }
    }
    CHECK(aggregates == 2 && roots[0] != roots[1] && choices >= 1 &&
          choices <= 2 && writes == 2 && loans == 2 && reads == 4);
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, first_if, a);
        const NLSemanticContext *w = nl_checked_context(arm);
        for (size_t k = 0; k < 2; ++k) {
            NLSemanticPlaceView root;
            CHECK(nl_semantic_place_view(w, roots[k], &root) && root.live &&
                  root.independent_root && root.implicit_local &&
                  root.fixed_field_count == 2 && root.governing_domain == 0);
            for (size_t j = 0; j < 2; ++j) {
                NLSemanticPlaceView child;
                NLSemanticValueView value;
                const size_t expected[2][2] = {{14, 181}, {33, 207}};
                CHECK(nl_semantic_place_view(w, root.fixed_fields[j], &child) &&
                      child.live && !child.independent_root &&
                      child.parent_aggregate == roots[k] &&
                      child.parent_incarnation == root.incarnation &&
                      child.parent_field_index == j &&
                      nl_semantic_value_view(w, child.current_value, &value) &&
                      value.scalar_known &&
                      value.scalar_value == expected[k][j]);
            }
            printf("{\"kind\":\"original\",\"arm\":%zu,\"root\":%zu,"
                   "\"incarnation\":%zu,\"fields\":[%zu,%zu],\"fact\":%zu}\n",
                   a, roots[k], root.incarnation, root.fixed_fields[0],
                   root.fixed_fields[1], root.current_fact);
        }
    }
    printf("{\"kind\":\"summary\",\"roots\":2,\"ifs\":%zu,\"loans\":2,"
           "\"scopes\":%zu,\"all_scopes_closed\":true,\"allocation_regions\":0,"
           "\"domains\":0,\"public_readback_only\":true}\n",
           choices, snapshot.scopes);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
