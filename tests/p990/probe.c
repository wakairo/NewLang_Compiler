#include "../p974/support.h"
#include <stdio.h>

static bool run(const char *path, bool direct, bool mixed, size_t expected_old)
{
    NLSemanticContext *base = NULL, *foreign_base = NULL;
    TestNode a = {0}, b = {0}, foreign = {0};
    P974Observation x, y, z;
    CHECK(registry(path, &base) && checked(base, &a) && checked(base, &b));
    CHECK(registry(path, &foreign_base) && checked(foreign_base, &foreign));
    CHECK(p974_find(a.entry, false, &x) && p974_find(b.entry, false, &y) &&
          p974_find(foreign.entry, false, &z));
    CHECK(p974_get(&x, false) && p974_validate(&x, false, true));
    CHECK(x.caller != y.caller && x.caller->context != y.caller->context);
    if (mixed)
        CHECK(nl_h_origin_validate(a.entry));
    const NLCheckedNodeView *root =
        nl_checked_node_view(x.body, nl_checked_root(x.body));
    CHECK(root && root->kind == NL_CHECKED_BLOCK && !root->tail &&
          root->item_count == (direct ? 1u : 2u));
    const NLCheckedNodeView *first =
        nl_checked_node_view(x.body, root->first_item);
    CHECK(first);
    NLCheckedNodeId ret_id = direct ? root->first_item : first->next_item;
    const NLCheckedNodeView *ret = nl_checked_node_view(x.body, ret_id);
    CHECK(ret && ret->kind == NL_CHECKED_RETURN);
    NLCheckedNodeId replace_id = direct ? ret->initializer : first->initializer;
    const NLCheckedNodeView *replace = nl_checked_node_view(x.body, replace_id);
    CHECK(ret && ret->kind == NL_CHECKED_RETURN && replace &&
          replace->kind == NL_CHECKED_REPLACE);
    NLCopyBodyEventView event, foreign_event;
    CHECK(nl_checked_copy_body_event(x.caller, x.call, 0, &event) &&
          nl_checked_copy_body_event(y.caller, y.call, 0, &foreign_event));
    CHECK(x.view.event_count == 1 && event.kind == NL_COPY_BODY_CHANGE &&
          event.node == replace_id);
    NLSemanticValueView old;
    CHECK(nl_semantic_value_view(x.parameters.handoff, x.view.result.value,
                                 &old) &&
          old.scalar_known && old.scalar_value == expected_old);
    CHECK(nl_checked_copy_body_validate(x.caller, x.call, &y.view) !=
          NL_CHECK_OK);
    CHECK(nl_checked_copy_body_parameters_validate(
              x.caller, x.call, &y.parameters) != NL_CHECK_OK);
    CHECK(nl_checked_copy_body_event_validate(x.caller, x.call, 0,
                                              &foreign_event) != NL_CHECK_OK);
    foreign_event = event;
    foreign_event.kind = NL_COPY_BODY_INITIALIZE;
    CHECK(nl_checked_copy_body_event_validate(x.caller, x.call, 0,
                                              &foreign_event) != NL_CHECK_OK);
    if (mixed)
        CHECK(emit(a.entry, true, NULL, 0));
    NLCheckedFragment *f = x.caller, *body = x.body;
    NLSemanticContext *current = (NLSemanticContext *)f->context;
    size_t function = f->nodes[x.call - 1].function - 1;
    NLFunctionEntry saved_function = current->functions[function];
    CHECK(saved_function.count == 2 && saved_function.parameters);
    NLTypeId parameter_types[2] = {saved_function.parameters[0],
                                   saved_function.parameters[1]};
    NLPlaceId other = 0;
    for (size_t i = 1; i <= x.parameters.entry->place_count; ++i) {
        NLSemanticPlaceView p;
        CHECK(nl_semantic_place_view(x.parameters.entry, i, &p));
        if (p.type == x.parameters.field.nominal && p.independent_root &&
            i != x.parameters.field.parent)
            other = i;
    }
    CHECK(other);
    NLCheckedNodeId formal_ref = replace->first_argument;
    const NLSource *saved_source = f->source, *body_source = body->source;
    NLFunctionBody *saved_plan = body->body_owner;
    for (size_t fault = 0; fault < 26; ++fault) {
        NLCheckedNodeView call = f->nodes[x.call - 1],
                          loan = f->nodes[x.parameters.loan - 1],
                          arg = f->nodes[x.parameters.arguments[0] - 1],
                          saved_root = body->nodes[body->root - 1],
                          saved_ret = body->nodes[ret_id - 1],
                          saved_ref = body->nodes[formal_ref - 1];
        switch (fault) {
        case 0:
            f->context = y.caller->context;
            break;
        case 1:
            f->source = z.caller->source;
            break;
        case 2:
            f->nodes[x.parameters.loan - 1].field.parent = other;
            break;
        case 3:
            f->nodes[x.parameters.loan - 1].field.index ^= 1;
            break;
        case 4:
            f->nodes[x.call - 1].results[0].value =
                f->nodes[x.parameters.arguments[1] - 1].results[0].value;
            break;
        case 5:
            f->nodes[x.call - 1].function =
                a.entry->nodes[a.entry->root - 1].function;
            break;
        case 6:
            f->nodes[x.call - 1].first_argument = x.parameters.arguments[1];
            break;
        case 7:
            f->nodes[x.parameters.arguments[0] - 1].parameter_type =
                f->nodes[x.parameters.arguments[1] - 1].parameter_type;
            break;
        case 8:
            f->nodes[x.parameters.arguments[0] - 1].symbol =
                x.parameters.parameters[1];
            break;
        case 9:
            f->nodes[x.parameters.loan - 1].loan.body_nonescape_proved = false;
            break;
        case 10:
            body->nodes[ret_id - 1].returned.value =
                f->nodes[x.parameters.arguments[1] - 1].results[0].value;
            break;
        case 11:
            body->nodes[ret_id - 1].initializer = 0;
            break;
        case 12:
            body->nodes[body->root - 1].item_count = 3;
            break;
        case 13:
            body->nodes[body->root - 1].tail = ret_id;
            break;
        case 14:
            body->nodes[formal_ref - 1].next_argument = 0;
            break;
        case 15:
            current->functions[function].body = z.body->body_owner;
            break;
        case 16:
            current->functions[function].kind = NL_CHECKED_UNIT;
            break;
        case 17:
            current->functions[function].result =
                nl_semantic_unit_type(current);
            break;
        case 18:
            current->functions[function].count = 1;
            break;
        case 19:
            current->functions[function].caller_effects = true;
            break;
        case 20:
            current->functions[function].owner_receiver = true;
            break;
        case 21:
            current->functions[function].hidden_dependencies = true;
            break;
        case 22:
            body->source = z.body->source;
            break;
        case 23:
            body->body_owner = z.body->body_owner;
            break;
        case 24:
            f->nodes[x.parameters.loan - 1].loan.from_ptr = true;
            break;
        case 25:
            saved_function.parameters[0] = parameter_types[1];
            break;
        }
        NLCopyBodyView observed;
        bool got = nl_checked_copy_body_view(f, x.call, &observed);
        NLCheckStatus valid = nl_checked_copy_body_validate(f, x.call, &x.view);
        char *output = NULL;
        size_t bytes = 777;
        NLNodeCStatus emitted =
            mixed ? nl_checked_c_node(a.entry, &output, &bytes)
                  : NL_NODE_C_UNSUPPORTED;
        /* All genuine objects/retained plans remain alive. Restore before
         * assertions, any retry, or either canonical owner teardown. */
        f->context = current;
        f->source = saved_source;
        f->nodes[x.call - 1] = call;
        f->nodes[x.parameters.loan - 1] = loan;
        f->nodes[x.parameters.arguments[0] - 1] = arg;
        body->nodes[body->root - 1] = saved_root;
        body->nodes[ret_id - 1] = saved_ret;
        body->nodes[formal_ref - 1] = saved_ref;
        body->source = body_source;
        body->body_owner = saved_plan;
        saved_function.parameters[0] = parameter_types[0];
        saved_function.parameters[1] = parameter_types[1];
        current->functions[function] = saved_function;
        CHECK(valid != NL_CHECK_OK);
        CHECK(!mixed ||
              (emitted == NL_NODE_C_UNSUPPORTED && !output && bytes == 777));
        CHECK(p974_validate(&x, false, true));
        printf("P990 metadata fault=%zu historical_GET=%d current_status=%d "
               "backend_checked=%d restored=YES\n",
               fault, got, (int)valid, mixed);
    }
    node_checked_destroy(&a);
    CHECK(p974_get(&y, false) && p974_validate(&y, false, true));
    if (mixed)
        CHECK(nl_h_origin_validate(b.entry) && emit(b.entry, true, NULL, 0));
    node_checked_destroy(&b);
    node_checked_destroy(&foreign);
    nl_semantic_destroy(base);
    nl_semantic_destroy(foreign_base);
    printf("P990 direct=%d mixed=%d old=%zu metadata=26 foreign_candidates=4 "
           "B_survives_A=YES PASS\n",
           direct, mixed, expected_old);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 5 && run(argv[1], atoi(argv[2]) != 0, atoi(argv[3]) != 0,
                            (size_t)strtoul(argv[4], NULL, 10))
               ? 0
               : 1;
}
