/* Read-only public evidence from the frozen source. No emitter or private
 * state. */
#include "../support/node_checked.h"
#include "newlang/node_join.h"

static bool inspect(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f);
    const NLSemanticContext *c = nl_checked_context(f);
    NLSymbolId symbols[2] = {0};
    NLPlaceId places[2] = {0};
    size_t roots = 0, certificates = 0, reloans = 0, initial_variant = 0;
    NLNodeJoinView p = {0};
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *r =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, v->initializer);
            CHECK(r && init);
            if (nl_semantic_recursive_local_type(c, r->type)) {
                CHECK(roots < 2 && init->kind == NL_CHECKED_AGGREGATE);
                NLSemanticBindingView b;
                NLSemanticPlaceView place;
                CHECK(nl_semantic_binding_view(c, r->symbol, &b) &&
                      nl_semantic_place_view(c, b.place, &place));
                CHECK(place.independent_root && place.implicit_local &&
                      place.governing_domain == 0 &&
                      place.placement.region == 0);
                symbols[roots] = r->symbol;
                places[roots] = b.place;
                size_t variant = 0, payload = 0;
                for (size_t field = init->first_argument; field;) {
                    const NLCheckedNodeView *a = nl_checked_node_view(f, field);
                    CHECK(a);
                    const NLCheckedNodeView *x =
                        nl_checked_node_view(f, a->initializer);
                    CHECK(x);
                    if (a->field_index == 0) {
                        CHECK(x->kind == NL_CHECKED_SUM_CONSTRUCTOR);
                        variant = x->variant;
                        if (variant == 2) {
                            const NLCheckedNodeView *arg =
                                nl_checked_node_view(f, x->initializer);
                            CHECK(roots == 1 && arg &&
                                  arg->has_reference_result &&
                                  arg->reference_result.place == places[0] &&
                                  arg->reference_result.scope == 0 &&
                                  arg->reference_result.provenance ==
                                      NL_PROVENANCE_VALID);
                        }
                    } else {
                        CHECK(a->field_index == 1 && x->has_scalar_result &&
                              x->scalar_result.known);
                        payload = x->scalar_result.value;
                    }
                    field = a->next_argument;
                }
                CHECK((roots == 0 && variant == 1 && payload == 29) ||
                      (roots == 1 && (variant == 1 || variant == 2) &&
                       payload == 61));
                if (roots == 1)
                    initial_variant = variant;
                printf("ROOT symbol=%zu place=%zu incarnation=%zu variant=%zu "
                       "payload=%zu\n",
                       r->symbol, b.place, place.incarnation, variant, payload);
                ++roots;
            } else if (init->kind == NL_CHECKED_FIELD_READ) {
                CHECK(roots == 2 && init->field.base == symbols[1] &&
                      init->field.present &&
                      init->field.dependency_compatible &&
                      init->field.index == 0);
                printf("READ symbol=%zu field=%zu fact=%zu\n", r->symbol,
                       init->field.child, init->field.child_fact);
            } else if (init->kind == NL_CHECKED_LOAN_HEADER) {
                CHECK(roots == 1 && init->loan.implicit_local &&
                      !init->loan.from_ptr && init->loan.source == symbols[0] &&
                      init->loan.place == places[0] &&
                      init->loan.body_nonescape_proved &&
                      init->loan.normal_result_forwarded);
                printf("POINTER symbol=%zu original=%zu incarnation=%zu\n",
                       r->symbol, init->loan.place, init->loan.incarnation);
            }
        }
        if (v->kind == NL_CHECKED_MATCH) {
            CHECK(nl_checked_node_join_view(f, i, &p));
            CHECK(nl_checked_node_join_validate(f, i, NULL) == NL_CHECK_OK &&
                  nl_checked_node_join_validate(f, i, &p) == NL_CHECK_OK);
            CHECK(v->normal_arms == 2 && !v->normal_frame_unchanged);
            ++certificates;
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER && v->loan.from_ptr) {
            CHECK(v->loan.implicit_local && v->loan.place == places[0] &&
                  v->loan.body_nonescape_proved);
            ++reloans;
        }
    }
    CHECK(roots == 2 && places[0] != places[1] && certificates == 1 &&
          reloans == 1);
    NLSemanticPlaceView original[2], joined[2], field, post, sibling,
        sibling_post;
    for (size_t i = 0; i < 2; ++i) {
        CHECK(p.originals[i] == places[i]);
        CHECK(nl_semantic_place_view(p.entry, places[i], &original[i]) &&
              nl_semantic_place_view(p.post, places[i], &joined[i]));
        CHECK(original[i].live && joined[i].live &&
              original[i].incarnation == joined[i].incarnation &&
              original[i].governing_domain == joined[i].governing_domain &&
              original[i].placement.region == joined[i].placement.region);
    }
    CHECK(original[0].current_fact == joined[0].current_fact &&
          original[1].current_fact != joined[1].current_fact);
    CHECK(nl_semantic_place_view(p.entry, p.changed, &field) &&
          nl_semantic_place_view(p.post, p.changed, &post) &&
          nl_semantic_place_view(p.entry, p.sibling, &sibling) &&
          nl_semantic_place_view(p.post, p.sibling, &sibling_post));
    CHECK(field.parent_aggregate == places[1] &&
          field.parent_field_index == 0 &&
          sibling.parent_aggregate == places[1] &&
          sibling.parent_field_index == 1);
    CHECK(sibling.current_fact == sibling_post.current_fact &&
          sibling.current_value == sibling_post.current_value);
    NLSemanticValueView input, unknown;
    CHECK(nl_semantic_value_view(p.entry, field.current_value, &input) &&
          input.variant == initial_variant);
    CHECK(nl_semantic_value_view(p.post, post.current_value, &unknown) &&
          unknown.variant == 0 && unknown.sum_payload == 0 &&
          unknown.dependencies == NL_DEPENDENCY_FREE &&
          unknown.value_dependency_count == 0 && post.payload_occurrence == 0);
    if (initial_variant == 2) {
        NLSemanticValueView ptr;
        NLSemanticOccurrenceView old;
        CHECK(input.sum_payload && field.payload_occurrence &&
              nl_semantic_value_view(p.entry, input.sum_payload, &ptr));
        CHECK(ptr.reference.place == places[0] &&
              ptr.reference.incarnation == original[0].incarnation &&
              ptr.reference.provenance == NL_PROVENANCE_VALID);
        CHECK(nl_semantic_occurrence_view(p.post, field.payload_occurrence,
                                          &old) &&
              !old.live);
    }
    printf("JOIN match=%zu original=%zu/%zu changed=%zu sibling=%zu F=%zu->%zu "
           "RF=%zu->%zu Unknown=1\n",
           p.match, places[0], places[1], p.changed, p.sibling,
           field.current_fact, post.current_fact, original[1].current_fact,
           joined[1].current_fact);
    for (size_t k = 0; k < 2; ++k) {
        const NLCheckedFragment *arm = p.arms[k].artifact;
        size_t old_bindings = 0;
        for (size_t j = 1; j <= nl_checked_node_count(arm); ++j) {
            const NLCheckedNodeView *v = nl_checked_node_view(arm, j);
            if (v->kind == NL_CHECKED_BINDING) {
                const NLCheckedNodeView *r =
                    nl_checked_node_view(arm, v->first_argument);
                const NLCheckedNodeView *init =
                    nl_checked_node_view(arm, v->initializer);
                CHECK(r && init && init->kind == NL_CHECKED_LOAN_HEADER &&
                      init->loan.access == NL_ACCESS_WRITE &&
                      init->loan.body_nonescape_proved &&
                      init->loan.normal_result_forwarded);
                printf("OLD variant=%zu symbol=%zu\n", p.arms[k].variant,
                       r->symbol);
                ++old_bindings;
            }
        }
        CHECK(old_bindings == 1);
        NLCheckedField e = p.arms[k].change;
        CHECK(p.arms[k].operation && e.present && e.dependency_compatible &&
              e.parent == places[1] && e.child == p.changed && e.index == 0 &&
              e.parent_fact != e.parent_post_fact &&
              e.child_fact != e.child_post_fact);
        NLSemanticPlaceView before, after, sb, sa;
        NLSemanticValueView old, next;
        CHECK(
            nl_semantic_place_view(p.arms[k].before_change, e.child, &before) &&
            nl_semantic_place_view(p.arms[k].after_change, e.child, &after) &&
            nl_semantic_value_view(p.arms[k].before_change, e.old_value,
                                   &old) &&
            nl_semantic_value_view(p.arms[k].after_change, e.new_value, &next));
        CHECK(old.variant == initial_variant &&
              next.variant == (p.arms[k].variant == 1 ? 2u : 1u));
        CHECK(before.incarnation == after.incarnation &&
              before.current_fact == e.child_fact &&
              after.current_fact == e.child_post_fact);
        CHECK(nl_semantic_place_view(p.arms[k].before_change, p.sibling, &sb) &&
              nl_semantic_place_view(p.arms[k].after_change, p.sibling, &sa) &&
              sb.current_fact == sa.current_fact &&
              sb.current_value == sa.current_value);
        if (e.payload_occurrence) {
            NLSemanticOccurrenceView occurrence;
            CHECK(nl_semantic_occurrence_view(p.arms[k].after_change,
                                              e.payload_occurrence,
                                              &occurrence) &&
                  !occurrence.live);
        }
        if (next.variant == 2) {
            NLSemanticValueView ptr;
            NLSemanticOccurrenceView occurrence;
            CHECK(e.post_payload_occurrence &&
                  e.post_payload_occurrence != e.payload_occurrence &&
                  nl_semantic_occurrence_view(p.arms[k].after_change,
                                              e.post_payload_occurrence,
                                              &occurrence) &&
                  occurrence.live &&
                  nl_semantic_value_view(p.arms[k].after_change,
                                         next.sum_payload, &ptr) &&
                  ptr.reference.place == places[0] &&
                  ptr.reference.incarnation == original[0].incarnation);
        } else
            CHECK(e.post_payload_occurrence == 0);
        printf("ARM variant=%zu operation=%zu F=%zu->%zu RF=%zu->%zu "
               "occurrence=%zu->%zu old_variant=%zu new_variant=%zu\n",
               p.arms[k].variant, p.arms[k].operation, e.child_fact,
               e.child_post_fact, e.parent_fact, e.parent_post_fact,
               e.payload_occurrence, e.post_payload_occurrence, old.variant,
               next.variant);
    }
    NLNodeJoinView bad = p;
    bad.arms[0].change.parent_post_fact = bad.arms[0].change.parent_fact;
    CHECK(nl_checked_node_join_validate(f, p.match, &bad) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    bad = p;
    bad.arms[1].change.payload_occurrence += 100;
    CHECK(nl_checked_node_join_validate(f, p.match, &bad) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(nl_checked_node_join_validate(f, p.match, &p) == NL_CHECK_OK);
    puts("PUBLIC VALID; copied stale RF and wrong occurrence rejected; one "
         "original anchor reloan");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
