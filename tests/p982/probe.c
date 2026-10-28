#include "../p974/support.h"
#include <stdio.h>

static bool run(const char *path)
{
    NLSemanticContext *base = NULL, *foreign_base = NULL;
    TestNode a = {0}, b = {0}, foreign = {0};
    P974Observation x, y, z;
    CHECK(registry(path, &base) && checked(base, &a) && checked(base, &b));
    CHECK(registry(path, &foreign_base) && checked(foreign_base, &foreign));
    CHECK(p974_find(a.entry, false, &x) && p974_find(b.entry, false, &y) &&
          p974_find(foreign.entry, false, &z));
    CHECK(x.caller != y.caller && x.caller->context != y.caller->context);
    CHECK(p974_get(&x, false) && p974_validate(&x, false, true));
    CHECK(nl_h_origin_validate(a.entry) && nl_h_origin_validate(b.entry));
    NLPlaceId originals[2] = {0};
    size_t roots = 0, ended_h = 0;
    const NLSemanticContext *entry = x.parameters.entry;
    for (size_t i = 1; i <= entry->place_count; ++i) {
        const NLSemanticPlaceView *p = &entry->places[i - 1];
        if (p->type == x.parameters.field.nominal && p->independent_root &&
            p->live) {
            CHECK(roots < 2 && p->implicit_local && p->fixed_field_count == 2);
            originals[roots++] = i;
        }
        if (nl_semantic_recursive_local_type(entry, p->type)) {
            CHECK(!p->live && !p->current_fact && !p->placement.region);
            ++ended_h;
        }
    }
    CHECK(roots == 2 && originals[0] != originals[1] && ended_h == 2);
    char *expected = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(a.entry, &expected, &length) == NL_NODE_C_OK &&
          expected && length);
    NLCheckedFragment *f = x.caller;
    NLCheckedNodeId producer = 0, terminal = 0, conditional = 0;
    for (size_t i = 1; i <= f->count; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->producer.entry_proved)
            producer = i;
        if (n->owner_call.entry_proved)
            terminal = i;
        if (n->kind == NL_CHECKED_IF)
            conditional = i;
    }
    CHECK(producer && terminal && conditional);
    const NLDomainId head_domain = f->nodes[producer - 1].producer.head_domain;
    NLSemanticPlaceView head;
    CHECK(nl_semantic_place_view(nl_checked_producer_entry(f, producer),
                                 f->nodes[producer - 1].producer.head.parent,
                                 &head));
    CHECK(head_domain &&
          head_domain != f->nodes[terminal - 1].owner_call.domain &&
          head.placement.region &&
          head.placement.region !=
              f->nodes[terminal - 1].owner_call.range.region);
    NLCheckedFragment *arm =
        (NLCheckedFragment *)nl_checked_if_arm(f, conditional, 0);
    NLCheckedFragment *other_arm =
        (NLCheckedFragment *)nl_checked_if_arm(y.caller, conditional, 0);
    CHECK(arm && other_arm && arm != other_arm);
    size_t edge = SIZE_MAX;
    for (size_t i = 0; i < f->arm_count; ++i)
        if (f->arms[i].artifact == arm)
            edge = i;
    CHECK(edge != SIZE_MAX && arm->count == 3 &&
          arm->nodes[0].kind == NL_CHECKED_U8_LITERAL &&
          arm->context->place_count >= originals[1]);
    NLSemanticContext *current = f->context;
    const size_t copy_function = f->nodes[x.call - 1].function - 1,
                 producer_function = f->nodes[producer - 1].function - 1,
                 terminal_function = f->nodes[terminal - 1].function - 1;
    for (size_t fault = 0; fault < 20; ++fault) {
        NLCheckedNodeView call = f->nodes[x.call - 1],
                          loan = f->nodes[x.parameters.loan - 1],
                          p = f->nodes[producer - 1],
                          t = f->nodes[terminal - 1];
        NLFunctionEntry copy_fn = current->functions[copy_function],
                        producer_fn = current->functions[producer_function],
                        terminal_fn = current->functions[terminal_function];
        const NLSource *source = f->source;
        NLSemanticContext *arm_context = arm->context;
        NLCheckedNodeView literal = arm->nodes[0];
        const NLValueFactId fact =
            arm_context->places[originals[0] - 1].current_fact;
        switch (fault) {
        case 0:
            f->context = y.caller->context;
            break;
        case 1:
            f->source = z.caller->source;
            break;
        case 2:
            f->nodes[x.parameters.loan - 1].field.parent =
                x.parameters.field.parent == originals[0] ? originals[1]
                                                          : originals[0];
            break;
        case 3:
            f->nodes[x.parameters.loan - 1].field.index =
                1 - x.parameters.field.index;
            break;
        case 4:
            f->nodes[x.call - 1].results[0].value =
                f->nodes[x.parameters.arguments[1] - 1].results[0].value;
            break;
        case 5:
            f->nodes[producer - 1].producer.entry_proved = false;
            break;
        case 6:
            f->nodes[terminal - 1].owner_call.entry_proved = false;
            break;
        case 7:
            f->nodes[terminal - 1].owner_call.domain = head_domain;
            break;
        case 8:
            f->nodes[terminal - 1].owner_call.range.region =
                head.placement.region;
            break;
        case 9:
            f->nodes[producer - 1].producer.return_proved = false;
            break;
        case 10:
            current->functions[copy_function].body = z.body->body_owner;
            break;
        case 11:
            current->functions[copy_function].result =
                nl_semantic_unit_type(current);
            break;
        case 12:
            current->functions[copy_function].kind = NL_CHECKED_UNIT;
            break;
        case 13:
            current->functions[copy_function].caller_effects =
                !copy_fn.caller_effects;
            break;
        case 14:
            current->functions[producer_function].body =
                foreign.context->functions[producer_function].body;
            break;
        case 15:
            current->functions[terminal_function].body =
                foreign.context->functions[terminal_function].body;
            break;
        case 16:
            arm->context = other_arm->context;
            break;
        case 17:
            arm->nodes[0].scalar_result.value ^= 1;
            break;
        case 18:
            arm_context->places[originals[0] - 1].current_fact ^= 1;
            break;
        case 19:
            f->arms[edge].artifact = other_arm;
            break;
        }
        const bool observed = p974_get(&x, false);
        const NLCheckStatus valid =
            nl_checked_copy_body_validate(f, x.call, &x.view);
        char *out = NULL;
        size_t bytes = 777;
        const NLNodeCStatus status = nl_checked_c_node(a.entry, &out, &bytes);
        /* Restore every temporary view before any assertion or teardown. */
        f->context = current;
        f->source = source;
        f->nodes[x.call - 1] = call;
        f->nodes[x.parameters.loan - 1] = loan;
        f->nodes[producer - 1] = p;
        f->nodes[terminal - 1] = t;
        current->functions[copy_function] = copy_fn;
        current->functions[producer_function] = producer_fn;
        current->functions[terminal_function] = terminal_fn;
        arm->context = arm_context;
        arm->nodes[0] = literal;
        arm_context->places[originals[0] - 1].current_fact = fact;
        f->arms[edge].artifact = arm;
        const bool refused =
            status == NL_NODE_C_UNSUPPORTED && !out && bytes == 777;
        free(out);
        CHECK(observed && refused);
        if (fault < 14)
            CHECK(valid == NL_CHECK_ANALYSIS_PRECISION_LIMIT);
        CHECK(p974_validate(&x, false, true) &&
              emit(a.entry, true, expected, length));
        printf("P982 negative=%zu current_Copy_status=%d emitter=UNSUPPORTED "
               "restored=YES\n",
               fault, (int)valid);
    }
    node_checked_destroy(&a);
    CHECK(p974_validate(&y, false, true) &&
          emit(b.entry, true, expected, length));
    node_checked_destroy(&b);
    node_checked_destroy(&foreign);
    nl_semantic_destroy(base);
    nl_semantic_destroy(foreign_base);
    free(expected);
    puts("P982 authentic post-H T originals/current proof/20 negatives/B "
         "survival PASS");
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && run(argv[1]) ? 0 : 1;
}
