#include "../p1280/support.h"

static size_t scalar(const NLSemanticContext *c, NLValueId id)
{
    NLSemanticValueView v;
    assert(nl_semantic_value_view(c, id, &v) && v.scalar_known &&
           v.type == nl_semantic_core_type(c, NL_TYPE_U8) &&
           v.dependencies == NL_DEPENDENCY_FREE && !v.reference_count &&
           !v.value_dependency_count);
    return v.scalar_value;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    PInput p = {0};
    p_load(&p, argv[1]);
    assert(p_check(&p) == NL_CHECK_OK && nl_h_origin_validate(p.entry) &&
           nl_h_custody_validate(p.entry) &&
           nl_h_custody_result_valid(p.entry, nl_checked_root(p.entry)));
    const NLCheckedFragment *body =
        nl_checked_call_body(p.entry, nl_checked_root(p.entry));
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    const NLCheckedNodeView *binding =
        nl_checked_node_view(body, root->first_item);
    const NLCheckedNodeView *result =
        nl_checked_node_view(body, binding->next_item);
    const NLCheckedNodeView *match =
        nl_checked_node_view(body, result->initializer);
    assert(binding->kind == NL_CHECKED_BINDING &&
           match->kind == NL_CHECKED_MATCH && match->normal_frame_unchanged &&
           match->match_binding_prefix >= binding->symbol);
    const NLCheckedFragment *none = NULL;
    for (size_t arm = 0; arm < 2; ++arm) {
        const NLCheckedFragment *a =
            nl_checked_match_arm(body, result->initializer, arm);
        const NLCheckedNodeView *r =
            nl_checked_node_view(a, nl_checked_root(a));
        assert(nl_h_origin_edge(p.entry, body, result->initializer, arm, a) &&
               r->normal_frame_unchanged);
        if (r->variant == 1)
            none = a;
    }
    assert(none);
    const NLSemanticContext *entry = nl_checked_context(none),
                            *current = nl_checked_context(body);
    NLSemanticBindingView inherited, final;
    NLSemanticPlaceView initial, end, fields[2];
    assert(nl_semantic_binding_view(entry, binding->symbol, &inherited) &&
           nl_semantic_binding_view(current, binding->symbol, &final) &&
           inherited.place == final.place &&
           nl_semantic_place_view(entry, inherited.place, &initial) &&
           nl_semantic_place_view(current, final.place, &end) && initial.live &&
           initial.independent_root && initial.implicit_local &&
           !initial.governing_domain && initial.fixed_field_count == 2 &&
           initial.incarnation == end.incarnation);
    for (size_t i = 0; i < 2; ++i)
        assert(nl_semantic_place_view(entry, initial.fixed_fields[i],
                                      &fields[i]) && fields[i].live &&
               fields[i].parent_aggregate == inherited.place);
    assert(scalar(entry, fields[0].current_value) == 213 &&
           scalar(entry, fields[1].current_value) == 101);
    size_t loans = 0, reads = 0;
    const NLCheckedNodeView *change = NULL;
    for (size_t id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, id);
        if (n->kind == NL_CHECKED_LOAN_HEADER && n->loan.implicit_local) {
            const NLCheckedNodeView *block =
                nl_checked_node_view(body, n->initializer);
            change = nl_checked_node_view(body, block->tail);
            assert(++loans == 1 && n->field.base == binding->symbol &&
                   n->field.parent == inherited.place && n->field.index == 1 &&
                   n->field.parent_fact == initial.current_fact &&
                   n->field.child == initial.fixed_fields[1] &&
                   n->field.child_fact == fields[1].current_fact &&
                   n->loan.place == n->field.child &&
                   n->loan.incarnation == fields[1].incarnation &&
                   n->loan.body_nonescape_proved &&
                   n->loan.normal_result_forwarded &&
                   change->kind == NL_CHECKED_REPLACE &&
                   change->field.old_value == fields[1].current_value &&
                   change->field.parent_post_fact != initial.current_fact &&
                   change->field.child_post_fact != fields[1].current_fact &&
                   scalar(current, change->results[0].value) == 101);
            NLSemanticValueView incoming;
            assert(nl_semantic_value_view(current, change->field.new_value,
                                          &incoming) &&
                   incoming.type == nl_semantic_core_type(current, NL_TYPE_U8) &&
                   !incoming.scalar_known &&
                   incoming.dependencies == NL_DEPENDENCY_FREE &&
                   !incoming.reference_count && !incoming.value_dependency_count);
        }
        if (n->kind == NL_CHECKED_FIELD_READ &&
            n->field.base == binding->symbol) {
            assert(change && n->field.parent == inherited.place &&
                   n->field.parent_fact == change->field.parent_post_fact &&
                   n->field.child == initial.fixed_fields[n->field.index] &&
                   n->field.child_fact ==
                       (n->field.index == 1 ? change->field.child_post_fact
                                            : fields[0].current_fact));
            ++reads;
        }
    }
    assert(loans == 1 && reads == 2);
    printf("PREFIX-PASS original=%zu incarnation=%zu unchanged-all-H=1 "
           "old=101 runtime-current=unknown sibling=213 closed-loan=1 "
           "reads=2 final-live=%u\n", inherited.place, initial.incarnation,
           end.live);
    p_dispose(&p);
    return 0;
}
