#include "support.h"

static bool probe(const char *path, bool unit)
{
    NLSemanticContext *base = NULL, *other = NULL;
    TestNode a = {0}, b = {0}, f = {0};
    CHECK(registry(path, &base) && registry(path, &other));
    CHECK(checked(base, &a) && checked(base, &b) && checked(other, &f));
    P974Observation o, same_plan, foreign;
    CHECK(p974_find(a.entry, unit, &o) &&
          p974_find(b.entry, unit, &same_plan) &&
          p974_find(f.entry, unit, &foreign));
    CHECK(o.caller != same_plan.caller &&
          o.caller->context != same_plan.caller->context &&
          o.caller->source == same_plan.caller->source &&
          o.body->body_owner == same_plan.body->body_owner &&
          o.body->body_owner != foreign.body->body_owner);
    CHECK(p974_get(&o, unit) && p974_validate(&o, unit, true));
    NLSemanticContext *current = (NLSemanticContext *)o.caller->context;
    const NLCheckedNodeView *call = nl_checked_node_view(o.caller, o.call);
    CHECK(current && current->functions && call && call->function &&
          call->function <= current->function_count);
    NLFunctionEntry *fn = &current->functions[call->function - 1];
    CHECK(fn->body == o.body->body_owner && fn->count == 2 && fn->parameters);
    const NLCheckedNodeView *foreign_call =
        nl_checked_node_view(foreign.caller, foreign.call);
    CHECK(foreign_call && foreign_call->function == call->function);
    const NLFunctionEntry *foreign_fn =
        &foreign.caller->context->functions[foreign_call->function - 1];
    CHECK(foreign_fn->body == foreign.body->body_owner &&
          foreign_fn->count == fn->count && foreign_fn->result == fn->result &&
          !memcmp(foreign_fn->parameters, fn->parameters,
                  2 * sizeof(*fn->parameters)));
    const NLFunctionEntry saved = *fn;
    const NLSemanticContext saved_world = *current;
    const NLCheckedFragment saved_caller = *o.caller;
    NLTypeId parameters[2] = {fn->parameters[0], fn->parameters[1]};
    fn->parameters =
        parameters; /* Equal signature values do not mint a plan. */
    CHECK(p974_validate(&o, unit, true));
    *fn = saved;
    for (unsigned fault = 0; fault < 18; ++fault) {
        switch (fault) {
        case 0:
            fn->body = (NLFunctionBody *)foreign.body->body_owner;
            break;
        case 1:
            fn->body = NULL;
            break;
        case 2:
            fn->result = unit ? nl_semantic_core_type(current, NL_TYPE_U8)
                              : nl_semantic_unit_type(current);
            break;
        case 3:
            fn->kind = NL_CHECKED_U8_LITERAL;
            break;
        case 4:
            fn->caller_effects = !saved.caller_effects;
            break;
        case 5:
            fn->hidden_dependencies = !saved.hidden_dependencies;
            break;
        case 6:
            fn->owner_receiver = !saved.owner_receiver;
            break;
        case 7:
            fn->owner_producer = !saved.owner_producer;
            break;
        case 8:
            fn->custody_recipient = !saved.custody_recipient;
            break;
        case 9:
            fn->count = 1;
            break;
        case 10:
            fn->parameters = NULL;
            break;
        case 11:
            parameters[1] = nl_semantic_unit_type(current);
            fn->parameters = parameters;
            break;
        case 12:
            o.caller->context = same_plan.caller->context;
            break;
        case 13:
            o.caller->source = foreign.caller->source;
            break;
        case 14:
            current->functions = NULL;
            break;
        case 15:
            current->functions = NULL;
            current->function_count = 0;
            break;
        case 16:
            current->function_count = 0;
            break;
        case 17:
            current->function_count = call->function - 1;
            break;
        }
        bool observed = p974_get(&o, unit);
        bool refused = p974_validate(&o, unit, false);
        *fn = saved;
        *current = saved_world;
        *o.caller = saved_caller;
        parameters[1] = saved.parameters[1];
        CHECK(observed && refused);
        CHECK(p974_get(&o, unit) && p974_validate(&o, unit, true));
        printf("P974 %s fault=%u historical_GET=original CURRENT=INVALID "
               "restored=OK\n",
               unit ? "unit" : "scalar", fault);
    }
    node_checked_destroy(&a);
    CHECK(p974_get(&same_plan, unit) && p974_validate(&same_plan, unit, true));
    CHECK(p974_get(&foreign, unit) && p974_validate(&foreign, unit, true));
    node_checked_destroy(&b);
    node_checked_destroy(&f);
    nl_semantic_destroy(base);
    nl_semantic_destroy(other);
    puts("P974 actual shared-plan B and separate-registration F healthy");
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && probe(argv[1], strcmp(argv[2], "unit") == 0) ? 0 : 1;
}
