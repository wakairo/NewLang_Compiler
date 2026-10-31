/* Reuse frozen P-owned observation/real-CHECK helpers, never the old probe. */
#define main p965_c85_original_main
#include "observer_original.h"
#undef main
#include "../../../src/copy_body_internal.h"

static bool guard(const char *path, bool unit)
{
    NLSemanticContext *base = NULL, *other = NULL;
    TestNode a = {0}, b = {0}, f = {0};
    CHECK(registry(path, &base) && registry(path, &other));
    CHECK(checked(base, &a) && checked(base, &b) && checked(other, &f));
    Observation o, sibling, foreign;
    CHECK(find(a.entry, unit, &o) && find(b.entry, unit, &sibling) &&
          find(f.entry, unit, &foreign));
    printf("C89 old conjuncts caller=%d world=%d source=%d root_not1=%d "
           "count_gt1=%d root=%zu count=%zu call=%zu\n",
           o.caller != foreign.caller,
           o.caller->context != foreign.caller->context,
           o.caller->source != foreign.caller->source, o.caller->root != 1,
           o.caller->count > 1, (size_t)o.caller->root, o.caller->count,
           (size_t)o.call);
    CHECK(o.caller != sibling.caller &&
          o.caller->context != sibling.caller->context &&
          o.caller->source == sibling.caller->source &&
          o.caller->body_owner == sibling.caller->body_owner);
    CHECK(gets(&o, unit) && validates(&o, unit, true));
    CHECK(gets(&sibling, unit) && validates(&sibling, unit, true));
    CHECK(gets(&foreign, unit) && validates(&foreign, unit, true));
    NLSemanticContext *current = (NLSemanticContext *)o.caller->context;
    NLSemanticContext *entry = (NLSemanticContext *)o.parameters.entry;
    CHECK(current && entry && current != entry && current->functions &&
          entry->functions && current->function_count < SIZE_MAX);
    NLCheckedNodeView *call = &o.caller->nodes[o.call - 1];
    CHECK(call->function && call->function <= current->function_count);
    const NLCheckedNodeView saved_call = *call;
    const NLCheckedFragment saved_caller = *o.caller;
    const NLSemanticContext saved_current = *current, saved_entry = *entry;
    for (unsigned fault = 0; fault < 12; ++fault) {
        switch (fault) {
        case 0:
            current->functions = NULL;
            current->function_count = 0;
            break;
        case 1:
            current->functions = NULL; /* genuine original positive count */
            break;
        case 2:
            current->function_count = 0;
            break;
        case 3:
            current->function_count = call->function - 1;
            break;
        case 4:
            o.caller->context = NULL;
            break;
        case 5:
            o.caller->context = sibling.caller->context;
            break;
        case 6:
            call->function = 0;
            break;
        case 7:
            call->function = saved_current.function_count + 1;
            break;
        case 8:
            entry->functions = NULL;
            entry->function_count = 0;
            break;
        case 9:
            entry->functions = NULL;
            break;
        case 10:
            entry->function_count = 0;
            break;
        case 11:
            call->function = SIZE_MAX;
            break;
        }
        /* Restore even on a failed observation before any assertion/teardown.
         */
        bool observed = gets(&o, unit);
        bool refused = validates(&o, unit, false);
        NLCheckStatus direct = unit ? nl_copy_unit_entry_validate(o.body)
                                    : NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        *current = saved_current;
        *entry = saved_entry;
        *o.caller = saved_caller;
        *call = saved_call;
        CHECK(observed && refused &&
              direct == NL_CHECK_ANALYSIS_PRECISION_LIMIT);
        CHECK(gets(&o, unit) && validates(&o, unit, true));
        printf("C89 guard %s fault=%u GET=true VALIDATE=INVALID restore=OK\n",
               unit ? "unit" : "u8", fault);
    }
    NLCopyBodyView sentinel = {.function = SIZE_MAX};
    CHECK(!nl_checked_copy_body_view(o.caller, 0, &sentinel) &&
          sentinel.function == SIZE_MAX);
    CHECK(nl_checked_copy_body_parameters_validate(o.caller, o.call,
                                                   &sibling.parameters) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK((unit ? nl_checked_copy_unit_validate(o.caller, o.call, &sibling.unit)
                : nl_checked_copy_body_validate(o.caller, o.call,
                                                &sibling.view)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    node_checked_destroy(&a);
    CHECK(gets(&sibling, unit) && validates(&sibling, unit, true));
    node_checked_destroy(&b);
    node_checked_destroy(&f);
    nl_semantic_destroy(base);
    nl_semantic_destroy(other);
    puts("C89 genuine shared-plan CHECK B survives A destruction");
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && guard(argv[1], strcmp(argv[2], "unit") == 0) ? 0 : 1;
}
