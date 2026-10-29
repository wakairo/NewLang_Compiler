/* Reuse the frozen public observation engine; add P-owned consumer negatives.
 */
#define main p1013_stage0_main
#include "../stage0/probe.c"
#undef main
#include "../../../src/copy_body_internal.h"
#define MUST(x)                                                                \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P1013 consumer:%d: %s\n", __LINE__, #x);          \
            abort();                                                           \
        }                                                                      \
    } while (0)
typedef struct {
    NLCheckedFragment *fragment;
    NLCheckedNodeId id;
} Location;
static bool producer(NLCheckedFragment *f, Location *out)
{
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n->producer.entry_proved) {
            *out = (Location){f, id};
            return true;
        }
        NLCheckedFragment *child =
            (NLCheckedFragment *)nl_checked_call_body(f, id);
        if (child && producer(child, out))
            return true;
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t i = 0; i < n->item_count; ++i) {
                child = (NLCheckedFragment *)nl_checked_match_arm(f, id, i);
                if (child && producer(child, out))
                    return true;
            }
    }
    return false;
}
static void exact_emit(const Run *r, const char *expected, size_t length)
{
    char *text = NULL;
    size_t bytes = 0;
    MUST(nl_checked_c_node(r->entry, &text, &bytes) == NL_NODE_C_OK);
    MUST(text && bytes == length && !memcmp(text, expected, bytes));
    free(text);
}
static void no_publication(const Run *r)
{
    char *text = NULL;
    size_t bytes = 777;
    NLNodeCStatus status = nl_checked_c_node(r->entry, &text, &bytes);
    MUST(status == NL_NODE_C_UNSUPPORTED && !text && bytes == 777);
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    char *public_args[] = {argv[0], argv[1], "observe", NULL};
    MUST(p1013_stage0_main(3, public_args) == 0);
    Run a = {0}, b = {0};
    Observation x[2], y[2];
    MUST(load(argv[1], &a) == 0 && load(argv[1], &b) == 0);
    MUST(observe(&a, x, true) && observe(&b, y, false));
    char *expected = NULL;
    size_t length = 0;
    MUST(nl_checked_c_node(a.entry, &expected, &length) == NL_NODE_C_OK &&
         expected && length);
    for (size_t ordinal = 0; ordinal < 2; ++ordinal) {
        NLCheckedFragment *caller = (NLCheckedFragment *)x[ordinal].caller;
        NLCopyBodyParameters p = x[ordinal].parameters;
        size_t function = x[ordinal].body.function - 1;
        NLSemanticContext *context = (NLSemanticContext *)caller->context;
        MUST(function < context->function_count && context->functions &&
             context->functions[function].body);
        size_t other_function = x[1 - ordinal].body.function - 1;
        MUST(other_function < context->function_count);
        if (other_function == function) {
            for (size_t i = 0; i < context->function_count; ++i) {
                const NLFunctionEntry *other = &context->functions[i];
                const NLFunctionEntry *actual = &context->functions[function];
                if (i != function && other->body && other->count == 2 &&
                    actual->count == 2 && other->result == actual->result &&
                    other->parameters[0] == actual->parameters[0] &&
                    other->parameters[1] == actual->parameters[1]) {
                    other_function = i;
                    break;
                }
            }
        }
        MUST(other_function != function);
        size_t slot = SIZE_MAX;
        for (size_t i = 0; i < caller->body_count; ++i)
            if (caller->body_calls[i] == x[ordinal].call)
                slot = i;
        MUST(slot != SIZE_MAX);
        for (size_t fault = 0; fault < 9; ++fault) {
            NLCheckedNodeView call = caller->nodes[x[ordinal].call - 1];
            NLCheckedNodeView loan = caller->nodes[p.loan - 1];
            NLFunctionEntry signature = context->functions[function];
            const NLSemanticContext *saved_context = caller->context;
            const NLSource *saved_source = caller->source;
            NLCheckedFragment *child = caller->bodies[slot];
            switch (fault) {
            case 0:
                caller->nodes[x[ordinal].call - 1].function =
                    other_function + 1;
                break;
            case 1:
                caller->nodes[p.loan - 1].field.index ^= 1u;
                break;
            case 2:
                caller->nodes[p.loan - 1].field.child = p.field.child + 1;
                break;
            case 3:
                caller->nodes[p.loan - 1].loan.body_nonescape_proved = false;
                break;
            case 4:
                caller->context = y[ordinal].caller->context;
                break;
            case 5:
                caller->source = nl_checked_source(y[ordinal].caller);
                break;
            case 6:
                caller->bodies[slot] =
                    (NLCheckedFragment *)y[ordinal].body.body;
                break;
            case 7:
                /* Same-ABI other callable plan. For same-helper controls,
                 * use the separately CHECK-owned foreign plan instead. */
                context->functions[function].body =
                    other_function != function
                        ? context->functions[other_function].body
                        : ((NLSemanticContext *)y[ordinal].caller->context)
                              ->functions[function]
                              .body;
                break;
            case 8:
                caller->nodes[p.loan - 1].field.child_fact = 0;
                break;
            }
            bool historical = nl_checked_copy_body_view(caller, x[ordinal].call,
                                                        &(NLCopyBodyView){0});
            NLCheckStatus bs = nl_checked_copy_body_validate(
                caller, x[ordinal].call, &x[ordinal].body);
            NLCheckStatus ps = nl_checked_copy_body_parameters_validate(
                caller, x[ordinal].call, &x[ordinal].parameters);
            MUST(bs != NL_CHECK_OK && ps != NL_CHECK_OK);
            no_publication(&a);
            caller->nodes[x[ordinal].call - 1] = call;
            caller->nodes[p.loan - 1] = loan;
            context->functions[function] = signature;
            caller->context = saved_context;
            caller->source = saved_source;
            caller->bodies[slot] = child;
            MUST(observe(&a, x, false));
            exact_emit(&a, expected, length);
            printf("P1013 metadata call=%zu fault=%zu historical_GET=%d "
                   "body=%d parameters=%d unpublished=YES exact_retry=YES\n",
                   ordinal, fault, historical, (int)bs, (int)ps);
        }
    }
    Location h;
    MUST(producer(a.entry, &h));
    NLCheckedNodeView original_node = h.fragment->nodes[h.id - 1];
    NLValueId head_allocation = 0;
    const NLSemanticContext *entry = original_node.producer.entry_world;
    for (size_t i = 1; i <= entry->value_count; ++i) {
        NLSemanticValueView value;
        NLSemanticTypeView type;
        if (nl_semantic_value_view(entry, i, &value) &&
            nl_semantic_type_view(entry, value.type, &type) &&
            type.kind == NL_TYPE_ALLOCATION && value.allocation_region &&
            value.allocation_region != original_node.producer.range.region) {
            head_allocation = i;
            break;
        }
    }
    MUST(head_allocation &&
         head_allocation != original_node.producer.inputs[2]);
    for (size_t fault = 0; fault < 4; ++fault) {
        NLCheckedNodeView *n = &h.fragment->nodes[h.id - 1];
        switch (fault) {
        case 0:
            n->producer.domain = n->producer.head_domain;
            break;
        case 1:
            n->producer.inputs[2] = head_allocation;
            break;
        case 2:
            n->producer.root = n->producer.head.parent;
            break;
        case 3:
            n->producer.return_proved = false;
            break;
        }
        MUST(!nl_checked_producer_valid(h.fragment, h.id));
        no_publication(&a);
        *n = original_node;
        exact_emit(&a, expected, length);
        printf("P1013 H owner fault=%zu unpublished=YES exact_retry=YES\n",
               fault);
    }
    /* A's mutable presentation must not adopt B's genuine child on destroy. */
    NLCheckedFragment *caller = (NLCheckedFragment *)x[0].caller;
    size_t slot = SIZE_MAX;
    for (size_t i = 0; i < caller->body_count; ++i)
        if (caller->body_calls[i] == x[0].call)
            slot = i;
    MUST(slot != SIZE_MAX);
    caller->bodies[slot] = (NLCheckedFragment *)y[0].body.body;
    no_publication(&a);
    destroy(&a);
    MUST(observe(&b, y, false));
    exact_emit(&b, expected, length);
    destroy(&b);
    free(expected);
    printf("P1013 negative metadata=18 H=4 foreign_child_unrestored=1 "
           "B_survives_A=YES PASS\n");
    return 0;
}
