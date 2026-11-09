#include "support.h"

static const NLCheckedFragment *h_owner, *t_owner, *linked_arm;
static NLCheckedNodeId h_read, t_call;
static size_t h_count, t_count;
static const NLCheckedFragment *visited[256];
static size_t visited_count;
static void visit(const NLCheckedFragment *f)
{
    if (!f)
        return;
    for (size_t i = 0; i < visited_count; ++i)
        if (visited[i] == f)
            return;
    assert(visited_count < 256);
    visited[visited_count++] = f;
    const NLTypeId u8 =
        nl_semantic_core_type(nl_checked_context(f), NL_TYPE_U8);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        assert(n);
        if (n->kind == NL_CHECKED_BINDING) {
            NLSemanticBindingView binding;
            NLSourceView name;
            if (nl_semantic_binding_view(nl_checked_context(f), n->symbol,
                                         &binding) &&
                binding.type == u8 && nl_source_view(f->source, n->name, &name))
                printf("BIND %.*s %zu\n", (int)name.length,
                       (const char *)name.bytes, n->symbol);
        }
        if (n->kind == NL_CHECKED_LINK_READ && n->type == u8) {
            ++h_count;
            h_owner = f;
            h_read = i;
        }
        NLCopyBodyParameters p;
        if (nl_checked_copy_body_parameters(f, i, &p)) {
            ++t_count;
            t_owner = f;
            t_call = i;
        }
        visit(nl_checked_call_body(f, i));
        if (n->kind == NL_CHECKED_MATCH) {
            const NLCheckedNodeView *s =
                nl_checked_node_view(f, n->initializer);
            for (size_t a = 0; a < n->item_count; ++a) {
                const NLCheckedFragment *child = nl_checked_match_arm(f, i, a);
                const NLCheckedNodeView *arm =
                    child ? nl_checked_node_view(child, nl_checked_root(child))
                          : NULL;
                assert(arm);
                if (s && s->kind != NL_CHECKED_TRY_ALLOCATE_ONE &&
                    arm->variant == 2)
                    linked_arm = child;
                visit(child);
            }
        }
    }
}
static size_t byte(const NLSemanticContext *c, NLValueId value)
{
    NLSemanticValueView v;
    assert(nl_semantic_value_view(c, value, &v) && v.scalar_known &&
           v.type == nl_semantic_core_type(c, NL_TYPE_U8) &&
           v.dependencies == NL_DEPENDENCY_FREE && !v.reference_count &&
           !v.value_dependency_count && v.scalar_value <= 255);
    return v.scalar_value;
}
int main(int argc, char **argv)
{
    assert(argc == 4);
    setvbuf(stdout, NULL, _IONBF, 0);
    const bool linked = strtoul(argv[2], NULL, 10) != 0;
    const size_t expected = strtoul(argv[3], NULL, 10);
    PInput input = {0};
    p_load(&input, argv[1]);
    const NLCheckStatus status = p_check(&input);
    NLCheckedFragment *entry = input.entry;
    printf("CHECK=%d owned=%u\n", status, entry != NULL);
    assert(status == NL_CHECK_OK && entry && nl_h_origin_has_sites(entry) &&
           nl_h_origin_validate(entry) && nl_h_custody_validate(entry) &&
           nl_h_custody_result_valid(entry, nl_checked_root(entry)));
    visit(entry);
    printf("OBSERVE H-reads=%zu T-calls=%zu same-owner=%u linked-arm=%u\n",
           h_count, t_count, h_owner == t_owner, linked_arm != NULL);
    assert(h_count == 1 && t_count == 1 && h_owner == t_owner &&
           (linked ? linked_arm == h_owner : linked_arm == NULL));
    const NLSemanticContext *c = nl_checked_context(h_owner);
    const NLCheckedNodeView *read = nl_checked_node_view(h_owner, h_read);
    const NLCheckedNodeView *project =
        nl_checked_node_view(h_owner, read->first_argument);
    NLSemanticPlaceView root, child;
    assert(
        read->field.present && read->field.index == 1 &&
        read->field.access == NL_ACCESS_READ && read->field.parent_fact &&
        read->field.child_fact &&
        read->field.old_value != read->results[0].value && project &&
        project->has_reference_result && !project->reference_result.writable &&
        project->reference_result.readable && project->reference_result.scope &&
        nl_semantic_place_view(c, read->field.parent, &root) &&
        nl_semantic_place_view(c, read->field.child, &child) &&
        root.incarnation == read->field.parent_incarnation &&
        child.incarnation == read->field.child_incarnation &&
        byte(c, read->field.old_value) == expected &&
        byte(c, read->results[0].value) == expected);
    if (linked) {
        const NLCheckedNodeView *a =
            nl_checked_node_view(linked_arm, nl_checked_root(linked_arm));
        NLSemanticBindingView binding;
        NLSemanticValueView pointer;
        assert(a->variant == 2 && a->symbol &&
               nl_semantic_binding_view(c, a->symbol, &binding) &&
               nl_semantic_value_view(c, binding.value, &pointer) &&
               !pointer.reference_count &&
               pointer.reference.place == read->field.parent &&
               pointer.reference.incarnation ==
                   read->field.parent_incarnation &&
               pointer.reference.provenance == NL_PROVENANCE_VALID);
    }
    NLCopyBodyView body;
    NLCopyBodyParameters p;
    NLCopyBodyEventView event;
    assert(nl_checked_copy_body_view(t_owner, t_call, &body) &&
           nl_checked_copy_body_parameters(t_owner, t_call, &p) &&
           nl_checked_copy_body_event(t_owner, t_call, 0, &event));
    const NLCheckStatus bv = nl_checked_copy_body_validate(t_owner, t_call,
                                                           &body),
                        pv = nl_checked_copy_body_parameters_validate(
                            t_owner, t_call, &p),
                        ev = nl_checked_copy_body_event_validate(
                            t_owner, t_call, 0, &event);
    printf("PUBLIC current/body=%d parameters=%d event=%d\n", bv, pv, ev);
    assert(bv == NL_CHECK_OK && pv == NL_CHECK_OK && ev == NL_CHECK_OK &&
           body.event_count == 1 && event.kind == NL_COPY_BODY_CHANGE &&
           p.field.index == 0);
    NLSemanticPlaceView h_point, child_point;
    assert(nl_semantic_place_view(p.entry, read->field.parent, &h_point) &&
           nl_semantic_place_view(p.entry, read->field.child, &child_point) &&
           h_point.live && child_point.live &&
           h_point.incarnation == read->field.parent_incarnation &&
           child_point.incarnation == read->field.child_incarnation &&
           h_point.governing_domain == project->lifetime_domain &&
           child_point.governing_domain == project->lifetime_domain &&
           h_point.fixed_field_count == 2 &&
           h_point.fixed_fields[1] == read->field.child &&
           byte(p.entry, child_point.current_value) == expected);
    NLSemanticPlaceView pre, post, old_child, new_child, sibling_before,
        sibling_after;
    assert(
        nl_semantic_place_view(p.entry, p.field.parent, &pre) &&
        nl_semantic_place_view(p.handoff, p.field.parent, &post) &&
        pre.independent_root && pre.implicit_local && pre.live && post.live &&
        pre.incarnation == post.incarnation &&
        pre.incarnation == p.field.parent_incarnation &&
        pre.current_fact != post.current_fact && pre.fixed_field_count == 2 &&
        post.fixed_field_count == 2 && pre.fixed_fields[0] == p.field.child &&
        post.fixed_fields[0] == p.field.child &&
        nl_semantic_place_view(p.entry, p.field.child, &old_child) &&
        nl_semantic_place_view(p.handoff, p.field.child, &new_child) &&
        old_child.incarnation == new_child.incarnation &&
        old_child.current_fact != new_child.current_fact &&
        byte(p.entry, old_child.current_value) == 31 &&
        byte(p.handoff, new_child.current_value) == expected &&
        nl_semantic_place_view(p.entry, pre.fixed_fields[1], &sibling_before) &&
        nl_semantic_place_view(p.handoff, post.fixed_fields[1],
                               &sibling_after) &&
        sibling_before.incarnation == sibling_after.incarnation &&
        sibling_before.current_fact == sibling_after.current_fact &&
        sibling_before.current_value == sibling_after.current_value &&
        byte(p.handoff, sibling_after.current_value) == 127);
    const NLCheckedNodeView *call = nl_checked_node_view(t_owner, t_call),
                            *loan = nl_checked_node_view(t_owner, p.loan);
    NLSemanticScopeView scope;
    assert(call->result_count == 1 && byte(c, call->results[0].value) == 31 &&
           loan->loan.body_nonescape_proved &&
           loan->loan.normal_result_forwarded &&
           nl_semantic_scope_view(c, loan->loan.scope, &scope) &&
           !scope.active);
    printf("OWNED-PASS actual-H-root=%zu H-field=%zu Domain=%zu read-Copy=%zu "
           "byte=%zu T-root=%zu T-inc=%zu old=31 new=%zu sibling=127 "
           "closed-loan=1 same-Some-arm=%u\n",
           read->field.parent, read->field.child, project->lifetime_domain,
           read->results[0].value, expected, p.field.parent,
           p.field.parent_incarnation, expected, linked);
    char *baseline = NULL;
    size_t baseline_length = 0;
    assert(nl_checked_c_node(entry, &baseline, &baseline_length) ==
               NL_NODE_C_OK &&
           baseline);
    NLCopyBodyParameters bad_parameters = p;
    bad_parameters.field.index ^= 1u;
    assert(nl_checked_copy_body_parameters_validate(
               t_owner, t_call, &bad_parameters) != NL_CHECK_OK);
    bad_parameters = p;
    ++bad_parameters.field.parent_incarnation;
    assert(nl_checked_copy_body_parameters_validate(
               t_owner, t_call, &bad_parameters) != NL_CHECK_OK);
    bad_parameters = p;
    bad_parameters.entry = p.handoff;
    assert(nl_checked_copy_body_parameters_validate(
               t_owner, t_call, &bad_parameters) != NL_CHECK_OK);
    NLCopyBodyEventView bad_event = event;
    bad_event.result = bad_event.incoming;
    assert(nl_checked_copy_body_event_validate(t_owner, t_call, 0,
                                               &bad_event) != NL_CHECK_OK);
    NLCheckedFragment *mutable_caller = (NLCheckedFragment *)t_owner;
    NLCheckedNodeView *mutable_read = &mutable_caller->nodes[h_read - 1];
    const NLCheckedNodeView saved_read = *mutable_read;
    NLCheckedNodeView *mutable_loan = &mutable_caller->nodes[p.loan - 1];
    const NLCheckedNodeView saved_loan = *mutable_loan;
    size_t rejected = 4;
    for (size_t fault = 0; fault < 7; ++fault) {
        if (fault == 0)
            mutable_read->field.parent = mutable_read->field.child;
        else if (fault == 1)
            ++mutable_read->field.parent_incarnation;
        else if (fault == 2)
            mutable_read->field.index = 0;
        else if (fault == 3)
            mutable_read->results[0].value = mutable_read->field.old_value;
        else if (fault == 4)
            mutable_loan->field.index ^= 1u;
        else if (fault == 5)
            ++mutable_loan->field.child_incarnation;
        else
            mutable_loan->loan.body_nonescape_proved = false;
        char *text = NULL;
        size_t length = 98765;
        assert(!nl_h_custody_validate(entry) &&
               nl_checked_c_node(entry, &text, &length) ==
                   NL_NODE_C_UNSUPPORTED &&
               !text && length == 98765);
        if (fault >= 4)
            assert(nl_checked_copy_body_validate(t_owner, t_call, NULL) !=
                   NL_CHECK_OK);
        NLCopyBodyView history;
        assert(nl_checked_copy_body_view(t_owner, t_call, &history));
        *mutable_read = saved_read;
        *mutable_loan = saved_loan;
        assert(nl_checked_copy_body_validate(t_owner, t_call, &body) ==
                   NL_CHECK_OK &&
               nl_checked_copy_body_parameters_validate(t_owner, t_call, &p) ==
                   NL_CHECK_OK &&
               nl_checked_c_node(entry, &text, &length) == NL_NODE_C_OK &&
               length == baseline_length && !memcmp(text, baseline, length));
        free(text);
        ++rejected;
    }
    NLSemanticContext *clone = NULL;
    assert(nl_sem_clone(c, &clone) == NL_CHECK_OK);
    mutable_caller->context = clone;
    char *text = NULL;
    size_t length = 98765;
    assert(nl_checked_c_node(entry, &text, &length) == NL_NODE_C_UNSUPPORTED &&
           !text && length == 98765);
    mutable_caller->context = c;
    nl_semantic_destroy(clone);
    ++rejected;
    PInput other = {0};
    p_load(&other, argv[1]);
    assert(p_check(&other) == NL_CHECK_OK);
    NLCheckedFragment *a = (NLCheckedFragment *)nl_checked_call_body(
                          entry, nl_checked_root(entry)),
                      *b = (NLCheckedFragment *)nl_checked_call_body(
                          other.entry, nl_checked_root(other.entry));
    assert(a && b && a->arm_count >= 2 && b->arm_count == a->arm_count);
    a->arms[1].artifact = b->arms[1].artifact;
    assert(nl_checked_c_node(entry, &text, &length) == NL_NODE_C_UNSUPPORTED &&
           !text);
    nl_checked_destroy(input.entry);
    input.entry = NULL;
    assert(nl_checked_c_node(other.entry, &text, &length) == NL_NODE_C_OK &&
           text && length == baseline_length &&
           !memcmp(text, baseline, length));
    free(text);
    p_dispose(&other);
    ++rejected;
    printf("CURRENT-NEGATIVE-PASS refusals=%zu foreign-B-after-A-destroy=1\n",
           rejected);
    free(baseline);
    p_dispose(&input);
    return 0;
}
