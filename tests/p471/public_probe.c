/* Source-owned public-only q transport and original-root currentness evidence.
 * Does not call emitter or use private checker layout/host-created authority.
 */
#include "../support/node_checked.h"
#include "newlang/node_join.h"

static bool inspect(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f);
    NLNodeJoinView p = {0};
    size_t count = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_join_view(f, i, &p))
            ++count;
    CHECK(count == 1 &&
          nl_checked_node_join_validate(f, p.match, NULL) == NL_CHECK_OK &&
          nl_checked_node_join_validate(f, p.match, &p) == NL_CHECK_OK);
    const NLCheckedNodeView *match = nl_checked_node_view(f, p.match);
    CHECK(match && match->normal_arms == 2 && !match->normal_frame_unchanged);
    NLSemanticPlaceView roots[2], post_roots[2], field, sibling, sibling_post;
    for (size_t i = 0; i < 2; ++i) {
        CHECK(nl_semantic_place_view(p.entry, p.originals[i], &roots[i]) &&
              nl_semantic_place_view(p.post, p.originals[i], &post_roots[i]));
        CHECK(roots[i].live && post_roots[i].live &&
              roots[i].independent_root && roots[i].implicit_local &&
              roots[i].governing_domain == 0 &&
              roots[i].placement.region == 0 &&
              roots[i].incarnation == post_roots[i].incarnation);
        printf("ORIGINAL index=%zu place=%zu incarnation=%zu RF=%zu->%zu\n", i,
               p.originals[i], roots[i].incarnation, roots[i].current_fact,
               post_roots[i].current_fact);
    }
    CHECK(p.originals[0] != p.originals[1]);
    CHECK(nl_semantic_place_view(p.entry, p.changed, &field) &&
          field.parent_aggregate == p.originals[0] &&
          field.parent_field_index == 0);
    CHECK(nl_semantic_place_view(p.entry, p.sibling, &sibling) &&
          nl_semantic_place_view(p.post, p.sibling, &sibling_post) &&
          sibling.current_fact == sibling_post.current_fact &&
          sibling.current_value == sibling_post.current_value);
    NLSemanticValueView input, ptr;
    CHECK(nl_semantic_value_view(p.entry, p.input, &input) &&
          input.variant == 2 && input.sum_payload &&
          nl_semantic_value_view(p.entry, input.sum_payload, &ptr) &&
          ptr.reference_count == 0 && ptr.reference.place == p.originals[0] &&
          ptr.reference.incarnation == roots[0].incarnation &&
          ptr.reference.provenance == NL_PROVENANCE_VALID &&
          ptr.reference.scope == 0);
    printf("JOIN match=%zu changed=%zu sibling=%zu input_variant=%zu "
           "input_target=%zu\n",
           p.match, p.changed, p.sibling, input.variant, ptr.reference.place);
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = p.arms[i].artifact;
        const NLCheckedNodeView *a =
            nl_checked_node_view(arm, nl_checked_root(arm));
        CHECK(a && a->kind == NL_CHECKED_MATCH_ARM && !a->terminates);
        if (!p.arms[i].operation) {
            CHECK(a->variant == 1);
            printf("ARM variant=1 unchanged=1\n");
            continue;
        }
        const NLCheckedNodeView *op =
            nl_checked_node_view(arm, p.arms[i].operation);
        CHECK(op && op->kind == NL_CHECKED_REPLACE &&
              op->field.parent == p.originals[0] &&
              op->field.child == p.changed && op->field.dependency_compatible &&
              op->field.parent_fact != op->field.parent_post_fact &&
              op->field.child_fact != op->field.child_post_fact);
        const NLCheckedNodeView *ref =
            nl_checked_node_view(arm, op->first_argument);
        const NLCheckedNodeView *sum =
            ref ? nl_checked_node_view(arm, ref->next_argument) : NULL;
        CHECK(sum && sum->kind == NL_CHECKED_SUM_CONSTRUCTOR &&
              sum->variant == a->variant);
        size_t loans = 0;
        for (size_t j = 1; j <= nl_checked_node_count(arm); ++j) {
            const NLCheckedNodeView *v = nl_checked_node_view(arm, j);
            if (v->kind == NL_CHECKED_LOAN_HEADER) {
                CHECK(v->loan.implicit_local &&
                      v->loan.access == NL_ACCESS_WRITE &&
                      v->loan.place == p.changed &&
                      v->loan.body_nonescape_proved &&
                      v->loan.normal_result_forwarded);
                ++loans;
            }
        }
        CHECK(loans == 1);
        if (a->variant == 2) {
            const NLCheckedNodeView *q =
                nl_checked_node_view(arm, sum->initializer);
            NLSemanticValueView incoming, value;
            CHECK(a->symbol > match->match_binding_prefix && q &&
                  q->kind == NL_CHECKED_IDENTIFIER && q->symbol == a->symbol &&
                  q->value_use == NL_VALUE_COPIED && q->has_reference_result &&
                  q->reference_result.place == p.originals[0] &&
                  q->reference_result.incarnation == roots[0].incarnation);
            CHECK(nl_semantic_value_view(p.arms[i].after_change,
                                         op->field.new_value, &incoming) &&
                  incoming.variant == 2 &&
                  nl_semantic_value_view(p.arms[i].after_change,
                                         incoming.sum_payload, &value) &&
                  value.reference_count == 0 &&
                  value.reference.place == ptr.reference.place &&
                  value.reference.incarnation == ptr.reference.incarnation &&
                  value.reference.provenance == NL_PROVENANCE_VALID &&
                  value.reference.scope == 0);
            printf("Q binder=%zu operand_symbol=%zu use=COPY target=%zu "
                   "incarnation=%zu\n",
                   a->symbol, q->symbol, q->reference_result.place,
                   q->reference_result.incarnation);
        }
        printf("ARM variant=%zu operation=%zu F=%zu->%zu RF=%zu->%zu "
               "occurrence=%zu->%zu\n",
               a->variant, p.arms[i].operation, op->field.child_fact,
               op->field.child_post_fact, op->field.parent_fact,
               op->field.parent_post_fact, op->field.payload_occurrence,
               op->field.post_payload_occurrence);
    }
    NLNodeJoinView bad = p;
    bad.changed = p.sibling;
    CHECK(nl_checked_node_join_validate(f, p.match, &bad) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(nl_checked_node_join_validate(f, p.match, &p) == NL_CHECK_OK);
    puts("PUBLIC VALID q/R/O/Change/Reset; wrong projection rejected; native "
         "untested");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
