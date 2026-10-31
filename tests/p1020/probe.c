#include "support.h"

static bool p1020_loaded(const char *path, NLSemanticContext **base,
                         TestNode *node, P974Observation *o)
{
    CHECK(registry(path, base) && checked(*base, node) &&
          p974_find(node->entry, true, o) && p974_get(o, true) &&
          p974_validate(o, true, true));
    return true;
}
static bool p1020_failed(TestNode *node, const P974Observation *o,
                         const char *label)
{
    CHECK(nl_checked_copy_unit_validate(o->caller, o->call, NULL) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(nl_checked_copy_body_parameters_validate(o->caller, o->call, NULL) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    char *out = NULL;
    size_t bytes = 777;
    CHECK(p1020_text(node->entry, &out, &bytes) == NL_NODE_C_UNSUPPORTED &&
          !out && bytes == 777);
    printf("P1020 METADATA REJECT %s unit=3 parameters=3 unpublished=YES\n",
           label);
    return true;
}
static bool p1020_positive(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode node = {0};
    P974Observation o;
    CHECK(p1020_loaded(path, &base, &node, &o));
    const NLCopyBodyParameters p = o.parameters;
    NLSemanticPlaceView entry, post, parent, sibling_entry, sibling_post;
    NLSemanticValueView old, unknown, sibling0, sibling1;
    CHECK(
        nl_semantic_place_view(p.entry, p.field.child, &entry) &&
        nl_semantic_place_view(p.handoff, p.field.child, &post) &&
        nl_semantic_place_view(p.entry, p.field.parent, &parent) &&
        parent.fixed_field_count == 2 && p.field.index < 2 &&
        nl_semantic_place_view(p.entry, parent.fixed_fields[1 - p.field.index],
                               &sibling_entry) &&
        nl_semantic_place_view(
            p.handoff, parent.fixed_fields[1 - p.field.index], &sibling_post) &&
        nl_semantic_value_view(p.entry, entry.current_value, &old) &&
        nl_semantic_value_view(p.handoff, post.current_value, &unknown) &&
        nl_semantic_value_view(p.entry, sibling_entry.current_value,
                               &sibling0) &&
        nl_semantic_value_view(p.handoff, sibling_post.current_value,
                               &sibling1));
    CHECK(entry.live && post.live && entry.incarnation == post.incarnation &&
          entry.current_fact != post.current_fact && old.scalar_known &&
          !unknown.scalar_known && sibling0.scalar_known &&
          sibling1.scalar_known &&
          sibling0.scalar_value == sibling1.scalar_value &&
          sibling_entry.current_fact == sibling_post.current_fact);
    printf("P1020 CURRENT function=%zu call=%zu root=%zu child=%zu index=%zu "
           "incarnation=%zu old=%zu sibling=%zu post=UNKNOWN unit=0 "
           "parameters=0\n",
           o.unit.function, o.call, p.field.parent, p.field.child,
           p.field.index, post.incarnation, old.scalar_value,
           sibling0.scalar_value);
    NLIfFieldView field;
    NLIfReturnView returned;
    if (nl_checked_if_field_view(o.body, o.unit.conditional, &field)) {
        CHECK(nl_checked_if_field_validate(o.body, o.unit.conditional,
                                           &field) == NL_CHECK_OK);
        for (size_t arm = 0; arm < 2; ++arm) {
            NLIfFieldEvent event;
            NLSemanticValueView displaced, incoming;
            CHECK(
                field.arms[arm].event_count == 1 &&
                field.arms[arm].change_count == 1 &&
                nl_checked_if_field_event(o.body, o.unit.conditional, arm, 0,
                                          &event) &&
                event.change &&
                nl_checked_if_field_event_validate(o.body, o.unit.conditional,
                                                   arm, 0,
                                                   &event) == NL_CHECK_OK &&
                nl_semantic_value_view(event.after, event.result, &displaced) &&
                nl_semantic_value_view(event.before, event.incoming,
                                       &incoming) &&
                displaced.scalar_known && incoming.scalar_known &&
                displaced.scalar_value == old.scalar_value);
            printf("P1020 ARM index=%zu old=%zu new=%zu current=0\n", arm,
                   displaced.scalar_value, incoming.scalar_value);
        }
    } else {
        CHECK(
            nl_checked_if_return_view(o.body, o.unit.conditional, &returned) &&
            nl_checked_if_return_validate(o.body, o.unit.conditional,
                                          &returned) == NL_CHECK_OK);
        printf("P1020 OLD IF-RETURN current=0\n");
    }
    char *text = NULL;
    size_t length = 0;
    CHECK(p1020_text(node.entry, &text, &length) == NL_NODE_C_OK && text &&
          length);
    CHECK(p1020_exact(&node, text, length));
    free(text);
    node_checked_destroy(&node);
    nl_semantic_destroy(base);
    return true;
}
static bool p1020_negative(const char *path)
{
    NLSemanticContext *base_a = NULL, *base_b = NULL;
    TestNode a = {0}, b = {0};
    P974Observation x, y;
    CHECK(p1020_loaded(path, &base_a, &a, &x) &&
          p1020_loaded(path, &base_b, &b, &y));
    CHECK(x.body != y.body && x.body->body_owner != y.body->body_owner &&
          x.body->source != y.body->source &&
          x.unit.postcall != y.unit.postcall);
    char *expected = NULL;
    size_t bytes = 0;
    CHECK(p1020_text(a.entry, &expected, &bytes) == NL_NODE_C_OK);
    NLCheckedNodeView *call = &x.caller->nodes[x.call - 1];
    NLFunctionEntry *fn = &a.context->functions[x.unit.function - 1];
    size_t reserved = 0;
    for (size_t i = 0; i < a.context->function_count; ++i)
        if (!strcmp(a.context->functions[i].name, "reserved"))
            reserved = i + 1;
    CHECK(reserved && reserved != call->function &&
          a.context->functions[reserved - 1].count == fn->count &&
          a.context->functions[reserved - 1].result == fn->result);
#define RESTORED()                                                             \
    CHECK(p974_get(&x, true) && p974_validate(&x, true, true) &&               \
          p1020_exact(&a, expected, bytes))
    size_t function = call->function;
    call->function = reserved;
    CHECK(p1020_failed(&a, &x, "wrong-genuine-function"));
    call->function = function;
    RESTORED();
    NLFunctionBody *plan = fn->body;
    fn->body = b.context->functions[y.unit.function - 1].body;
    CHECK(p1020_failed(&a, &x, "foreign-same-ABI-current-plan"));
    fn->body = plan;
    RESTORED();
    NLTypeId result = fn->result;
    fn->result = nl_semantic_core_type(a.context, NL_TYPE_U8);
    CHECK(p1020_failed(&a, &x, "current-result"));
    fn->result = result;
    RESTORED();
    fn->owner_receiver = true;
    CHECK(p1020_failed(&a, &x, "current-owner-kind"));
    fn->owner_receiver = false;
    RESTORED();
    const NLSemanticContext *caller_world = x.caller->context;
    x.caller->context = b.context;
    CHECK(p1020_failed(&a, &x, "foreign-current-caller-world"));
    x.caller->context = caller_world;
    RESTORED();
    const NLSource *source = x.body->source;
    x.body->source = y.body->source;
    CHECK(p1020_failed(&a, &x, "foreign-callee-source"));
    x.body->source = source;
    RESTORED();
    x.body->body_owner = y.body->body_owner;
    CHECK(p1020_failed(&a, &x, "foreign-callee-plan"));
    x.body->body_owner = plan;
    RESTORED();
    NLCheckedNodeId argument = call->first_argument;
    const NLCheckedNodeView *first = nl_checked_node_view(x.caller, argument);
    CHECK(first && first->next_argument && first->next_argument != argument);
    call->first_argument = first->next_argument;
    CHECK(p1020_failed(&a, &x, "wrong-actual-order"));
    call->first_argument = argument;
    RESTORED();
    NLCheckedNodeView *loan = &x.caller->nodes[x.parameters.loan - 1];
    loan->loan.body_nonescape_proved = false;
    CHECK(p1020_failed(&a, &x, "loan-escape"));
    loan->loan.body_nonescape_proved = true;
    RESTORED();
    NLScopeId scope = loan->loan.scope;
    CHECK(scope);
    loan->loan.scope = 0;
    CHECK(p1020_failed(&a, &x, "closed-loan"));
    loan->loan.scope = scope;
    RESTORED();
    NLCheckedFragment *arm = x.body->arms[0].artifact;
    NLCheckedNodeView *root = &arm->nodes[arm->root - 1];
    CHECK(!root->terminates);
    root->terminates = true;
    CHECK(p1020_failed(&a, &x, "one-normal-arm-graph-altered"));
    root->terminates = false;
    RESTORED();
    const NLPlaceId child = x.parameters.field.child;
    const NLSemanticContext *entry = x.parameters.entry;
    NLValueFactId fact = arm->context->places[child - 1].current_fact;
    CHECK(fact != entry->places[child - 1].current_fact);
    arm->context->places[child - 1].current_fact =
        entry->places[child - 1].current_fact;
    CHECK(p1020_failed(&a, &x, "one-arm-world-stale"));
    arm->context->places[child - 1].current_fact = fact;
    RESTORED();
    NLSemanticContext *handoff = (NLSemanticContext *)x.unit.postcall;
    fact = handoff->places[child - 1].current_fact;
    CHECK(fact != entry->places[child - 1].current_fact);
    handoff->places[child - 1].current_fact =
        entry->places[child - 1].current_fact;
    CHECK(p1020_failed(&a, &x, "stale-postcall-child"));
    handoff->places[child - 1].current_fact = fact;
    RESTORED();
    const NLPlaceId sibling = entry->places[x.parameters.field.parent - 1]
                                  .fixed_fields[1 - x.parameters.field.index];
    fact = handoff->places[sibling - 1].current_fact;
    CHECK(fact != entry->places[child - 1].current_fact);
    handoff->places[sibling - 1].current_fact =
        entry->places[child - 1].current_fact;
    CHECK(p1020_failed(&a, &x, "stale-postcall-sibling"));
    handoff->places[sibling - 1].current_fact = fact;
    RESTORED();
    NLSemanticContext *normal = (NLSemanticContext *)x.unit.normal_end;
    fact = normal->places[child - 1].current_fact;
    CHECK(fact != entry->places[child - 1].current_fact);
    normal->places[child - 1].current_fact =
        entry->places[child - 1].current_fact;
    CHECK(p1020_failed(&a, &x, "stale-normal-end"));
    normal->places[child - 1].current_fact = fact;
    RESTORED();

    /* Candidate-only mismatches reject that candidate, while the unmodified
     * actual call remains current and may still emit its original C. */
    for (size_t fault = 0; fault < 5; ++fault) {
        NLCopyUnitView bad = x.unit;
        if (fault == 0)
            bad.postcall = y.unit.postcall;
        else if (fault == 1)
            bad.normal_end = y.unit.normal_end;
        else if (fault == 2) {
            CHECK(bad.parameters.parameters[0] != bad.parameters.parameters[1]);
            NLSymbolId saved = bad.parameters.parameters[0];
            bad.parameters.parameters[0] = bad.parameters.parameters[1];
            bad.parameters.parameters[1] = saved;
        } else if (fault == 3) {
            CHECK(bad.parameters.arguments[0] != bad.parameters.arguments[1]);
            NLCheckedNodeId saved = bad.parameters.arguments[0];
            bad.parameters.arguments[0] = bad.parameters.arguments[1];
            bad.parameters.arguments[1] = saved;
        } else {
            bad.parameters.field.child = sibling;
            bad.parameters.field.index = 1 - bad.parameters.field.index;
        }
        CHECK(nl_checked_copy_unit_validate(x.caller, x.call, &bad) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);
        if (fault >= 2)
            CHECK(nl_checked_copy_body_parameters_validate(x.caller, x.call,
                                                           &bad.parameters) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT);
        RESTORED();
        printf("P1020 VIEW REJECT index=%zu actual_current=0 exact_C=YES\n",
               fault);
    }
    size_t index = 0;
    while (index < x.caller->body_count &&
           x.caller->body_calls[index] != x.call)
        ++index;
    CHECK(index < x.caller->body_count);
    x.caller->bodies[index] = y.body;
    CHECK(p1020_failed(&a, &x, "foreign-child-unrestored"));
    /* Canonical ownership, not this presentation array, must destroy A. */
    node_checked_destroy(&a);
    CHECK(p974_get(&y, true) && p974_validate(&y, true, true) &&
          p1020_exact(&b, expected, bytes));
    printf(
        "P1020 OWNER A_destroyed_unrestored=YES B_current_and_destroy=YES\n");
    node_checked_destroy(&b);
    nl_semantic_destroy(base_a);
    nl_semantic_destroy(base_b);
    free(expected);
    return true;
}

int main(int argc, char **argv)
{
    (void)node_backend;
    if (argc != 3)
        return 64;
    if (!strcmp(argv[1], "positive"))
        return p1020_positive(argv[2]) ? 0 : 1;
    if (!strcmp(argv[1], "negative"))
        return p1020_negative(argv[2]) ? 0 : 1;
    return 64;
}
