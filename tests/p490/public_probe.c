/* P490 source-issued post-join read evidence, based on adopted P471 probe.
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
    size_t reads = 0;
    NLSemanticPlaceView post_field, post_parent;
    CHECK(
        nl_semantic_place_view(p.post, p.changed, &post_field) &&
        nl_semantic_place_view(p.post, field.parent_aggregate, &post_parent) &&
        post_field.live && post_parent.live &&
        post_field.incarnation == field.incarnation &&
        post_parent.incarnation == roots[0].incarnation);
    NLSemanticValueView joined;
    CHECK(nl_semantic_value_view(p.post, post_field.current_value, &joined) &&
          joined.variant == 0 && joined.sum_payload == 0 &&
          joined.reference_count == 0 && joined.reference.place == 0);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind != NL_CHECKED_FIELD_READ || !v->field.present ||
            v->field.child != p.changed)
            continue;
        CHECK(v->span.start_byte > match->span.end_byte &&
              v->value_use == NL_VALUE_COPIED && v->result_count == 1 &&
              v->field.dependency_compatible &&
              v->field.parent == field.parent_aggregate &&
              v->field.parent_incarnation == post_parent.incarnation &&
              v->field.child_incarnation == post_field.incarnation &&
              v->field.parent_fact == post_parent.current_fact &&
              v->field.child_fact == post_field.current_fact &&
              v->field.old_value == post_field.current_value &&
              v->field.payload_occurrence == post_field.payload_occurrence);
        NLSemanticValueView copy;
        CHECK(nl_semantic_value_view(nl_checked_context(f), v->results[0].value,
                                     &copy) &&
              copy.type == joined.type && copy.variant == 0 &&
              copy.sum_payload == 0 && copy.reference.place == 0 &&
              copy.reference_count == 0 && copy.domain == 0 &&
              copy.allocation_region == 0);
        printf("POST_READ node=%zu original=%zu/%zu field=%zu/%zu "
               "RF=%zu F=%zu value=%zu copy=%zu variant=UNKNOWN "
               "payload=0 provenance=NOT_INVENTED\n",
               i, v->field.parent, v->field.parent_incarnation, v->field.child,
               v->field.child_incarnation, v->field.parent_fact,
               v->field.child_fact, v->field.old_value, v->results[0].value);
        ++reads;
    }
    CHECK(reads == 1);
    puts("PUBLIC VALID q/R/O/Change/Reset + post-read Copy Unknown; "
         "wrong projection rejected; concrete post pointer NOT PROVED");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
