#include "support.h"

static bool current_refuses(TestNode *node, const P1058Call *c)
{
    CHECK(nl_checked_copy_body_validate(c->caller, c->call, NULL) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          nl_checked_copy_body_parameters_validate(c->caller, c->call, NULL) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          p1058_unpublished(node));
    return true;
}
typedef struct {
    NLCheckedFragment *fragment;
    NLCheckedNodeId id;
} Producer;
static bool find_producer(NLCheckedFragment *f, Producer *out)
{
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n->producer.entry_proved) {
            *out = (Producer){f, id};
            return true;
        }
        NLCheckedFragment *body =
            (NLCheckedFragment *)nl_checked_call_body(f, id);
        if (body && find_producer(body, out))
            return true;
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t arm = 0; arm < n->item_count; ++arm) {
                body = (NLCheckedFragment *)nl_checked_match_arm(f, id, arm);
                if (body && find_producer(body, out))
                    return true;
            }
    }
    return false;
}
static bool run(const char *path)
{
    NLSemanticContext *base_a = NULL, *base_b = NULL;
    TestNode a = {0}, b = {0};
    P1058Call x[2], y[2];
    CHECK(registry(path, &base_a) && checked(base_a, &a) &&
          p1058_calls(&a, x) && p1058_valid(x) && registry(path, &base_b) &&
          checked(base_b, &b) && p1058_calls(&b, y) && p1058_valid(y));
    CHECK(x[0].body != y[0].body &&
          x[0].body->body_owner != y[0].body->body_owner);
    char *expected = NULL;
    size_t length = 777;
    CHECK(nl_checked_c_node(a.entry, &expected, &length) == NL_NODE_C_OK &&
          expected && length);
    for (size_t i = 0; i < 2; ++i) {
        P1058Call *c = &x[i];
        NLCheckedNodeView *call = &c->caller->nodes[c->call - 1];
        NLCheckedNodeView *loan = &c->caller->nodes[c->parameters.loan - 1];
        NLCheckedNodeView saved_call = *call, saved_loan = *loan;
        const NLSemanticContext *saved_context = c->caller->context;
        const NLSource *saved_source = c->caller->source;
        NLSemanticContext *context = (NLSemanticContext *)saved_context;
        NLFunctionEntry *function = &context->functions[c->view.function - 1];
        NLFunctionEntry saved_function = *function;
        for (size_t fault = 0; fault < 7; ++fault) {
            if (fault == 0)
                call->function = x[1 - i].view.function;
            else if (fault == 1)
                function->body =
                    context->functions[x[1 - i].view.function - 1].body;
            else if (fault == 2)
                c->caller->context = y[i].caller->context;
            else if (fault == 3)
                loan->loan.body_nonescape_proved = false;
            else if (fault == 4) {
                const NLCheckedNodeView *first =
                    nl_checked_node_view(c->caller, call->first_argument);
                CHECK(first && first->next_argument);
                call->first_argument = first->next_argument;
            } else if (fault == 5)
                loan->field.index ^= 1U;
            else
                c->caller->source = y[i].caller->source;
            CHECK(current_refuses(&a, c));
            *call = saved_call;
            *loan = saved_loan;
            *function = saved_function;
            c->caller->context = saved_context;
            c->caller->source = saved_source;
            CHECK(p1058_valid(x) && p1058_exact(&a, expected, length));
            printf("P1058 ACTUAL REJECT call=%zu fault=%zu unpublished=YES "
                   "exact_retry=YES\n",
                   i, fault);
        }
        NLCopyBodyParameters wrong = c->parameters;
        wrong.field.index ^= 1U;
        CHECK(nl_checked_copy_body_parameters_validate(c->caller, c->call,
                                                       &wrong) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
              nl_checked_copy_body_validate(c->caller, c->call, &y[i].view) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
              nl_checked_copy_body_parameters_validate(c->caller, c->call,
                                                       &y[i].parameters) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
              p1058_exact(&a, expected, length));
        printf("P1058 VIEW REJECT call=%zu supplied=3 actual_current=OK\n", i);
    }
    Producer h;
    CHECK(find_producer(a.entry, &h));
    NLCheckedNodeView *n = &h.fragment->nodes[h.id - 1], saved = *n;
    CHECK(nl_checked_producer_valid(h.fragment, h.id));
    printf("P1058 H CURRENT original_root=%zu backing=%zu domain=%zu "
           "producer=VALID\n",
           n->producer.root, n->producer.range.region, n->producer.domain);
    const NLSemanticContext *entry = n->producer.entry_world;
    NLValueId other_allocation = 0;
    for (size_t i = 1; i <= entry->value_count; ++i) {
        NLSemanticValueView value;
        NLSemanticTypeView type;
        if (nl_semantic_value_view(entry, i, &value) &&
            nl_semantic_type_view(entry, value.type, &type) &&
            type.kind == NL_TYPE_ALLOCATION && value.allocation_region &&
            value.allocation_region != n->producer.range.region) {
            other_allocation = i;
            break;
        }
    }
    CHECK(other_allocation && other_allocation != n->producer.inputs[2]);
    for (size_t fault = 0; fault < 4; ++fault) {
        if (!fault)
            n->producer.domain = n->producer.head_domain;
        else if (fault == 1)
            n->producer.inputs[2] = other_allocation;
        else if (fault == 2)
            n->producer.root = n->producer.head.parent;
        else
            n->producer.return_proved = false;
        CHECK(!nl_checked_producer_valid(h.fragment, h.id) &&
              p1058_unpublished(&a));
        *n = saved;
        CHECK(p1058_valid(x) && p1058_exact(&a, expected, length));
        printf("P1058 H REJECT fault=%zu original_owner=REQUIRED "
               "exact_retry=YES\n",
               fault);
    }
    size_t slot = 0;
    while (slot < x[1].caller->body_count &&
           x[1].caller->body_calls[slot] != x[1].call)
        ++slot;
    CHECK(slot < x[1].caller->body_count);
    x[1].caller->bodies[slot] = y[1].body;
    CHECK(current_refuses(&a, &x[1]));
    node_checked_destroy(
        &a); /* mutable presentation is intentionally not restored */
    CHECK(p1058_valid(y) && p1058_exact(&b, expected, length));
    node_checked_destroy(&b);
    nl_semantic_destroy(base_a);
    nl_semantic_destroy(base_b);
    free(expected);
    printf("P1058 OWNER A_destroyed_unrestored=YES "
           "B_current_exact_C_destroy=YES\n");
    return true;
}
static bool find_kind(NLCheckedFragment *f, NLCheckedKind kind,
                      NLCheckedFragment **owner, NLCheckedNodeId *node)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == kind) {
            *owner = f;
            *node = i;
            return true;
        }
        NLCheckedFragment *body =
            (NLCheckedFragment *)nl_checked_call_body(f, i);
        if (body && find_kind(body, kind, owner, node))
            return true;
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < n->item_count; ++a) {
                body = (NLCheckedFragment *)nl_checked_match_arm(f, i, a);
                if (body && find_kind(body, kind, owner, node))
                    return true;
            }
    }
    return false;
}
static bool worlds(const char *path)
{
    NLSemanticContext *base_a = NULL, *base_b = NULL;
    TestNode a = {0}, b = {0};
    P1058Call x[2], y[2];
    CHECK(registry(path, &base_a) && checked(base_a, &a) &&
          p1058_calls(&a, x) && p1058_valid(x) && registry(path, &base_b) &&
          checked(base_b, &b) && p1058_calls(&b, y) && p1058_valid(y));
    char *expected = NULL;
    size_t bytes = 777;
    CHECK(nl_checked_c_node(a.entry, &expected, &bytes) == NL_NODE_C_OK &&
          expected);
    NLCheckedFragment *main = x[0].caller, *owner = NULL;
    NLCheckedNodeId id = 0;
    CHECK(find_kind(main, NL_CHECKED_MATCH, &owner, &id) && owner == main);
    NLCheckedNodeView *match = &main->nodes[id - 1], saved = *match;
    for (size_t fault = 0; fault < 6; ++fault) {
        if (fault == 0)
            match->normal_arms = 1;
        else if (fault == 1)
            match->normal_frame_unchanged = false;
        else if (fault == 2)
            match->captured_frame_closed = true;
        else if (fault == 3)
            match->item_count = 1;
        else if (fault == 4)
            match->terminates = true;
        else
            match->span.end_byte = main->nodes[x[0].call - 1].span.end_byte;
        CHECK(p1058_unpublished(&a));
        printf("P1058 WORLD REJECT outer_match=%zu fault=%zu H_origin=%d "
               "unpublished=YES\n",
               id, fault, nl_h_origin_validate(a.entry));
        *match = saved;
        CHECK(p1058_valid(x) && p1058_exact(&a, expected, bytes));
    }
    CHECK(find_kind(main, NL_CHECKED_DEALLOCATE, &owner, &id));
    NLCheckedNodeView *release = &owner->nodes[id - 1],
                      saved_release = *release;
    release->kind = NL_CHECKED_UNIT;
    CHECK(p1058_unpublished(&a));
    *release = saved_release;
    CHECK(p1058_valid(x) && p1058_exact(&a, expected, bytes));
    puts("P1058 WORLD REJECT missing_actual_release unpublished=YES "
         "exact_retry=YES");
    CHECK(main->arm_count == 2 && y[0].caller->arm_count == 2 &&
          main->arms[0].artifact != y[0].caller->arms[0].artifact);
    main->arms[0] = y[0].caller->arms[0];
    CHECK(!nl_h_origin_validate(a.entry) && p1058_unpublished(&a));
    node_checked_destroy(&a); /* genuinely foreign B arm remains unrestored */
    CHECK(nl_h_origin_validate(b.entry) && p1058_valid(y) &&
          p1058_exact(&b, expected, bytes));
    node_checked_destroy(&b);
    nl_semantic_destroy(base_a);
    nl_semantic_destroy(base_b);
    free(expected);
    puts("P1058 H_OWNER foreign_arm_A_destroyed_unrestored=YES "
         "B_exact_C_destroy=YES");
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    return argc == 2 && run(argv[1]) && worlds(argv[1]) ? 0 : 1;
}
