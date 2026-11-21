#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                                     \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "line %d: %s\n", __LINE__, #condition);            \
            return false;                                                      \
        }                                                                      \
    } while (0)

typedef struct {
    NLSource *source;
    NLParser *parser;
    NLSyntaxTree *expression;
    NLSemanticContext *context;
    NLCheckedFragment *root;
} Program;

static bool prepare(Program *p, const char *file)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    bool ok =
        nl_source_load(file, &source) == NL_SOURCE_OK &&
        nl_parser_create(source, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK &&
        nl_semantic_create(&p->context) == NL_CHECK_OK;
    const NLSyntaxTree *units[] = {tree};
    ok = ok && nl_semantic_register_function_unit(p->context, units, 1, NULL) ==
                   NL_CHECK_OK;
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok &&
           nl_source_create("main()", 6, "p1623-entry", &p->source) ==
               NL_SOURCE_OK &&
           nl_parser_create(p->source, &p->parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(p->parser, &p->expression,
                                               NULL) == NL_PARSE_OK;
}

static NLCheckStatus check(Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->expression, &p->root, &diagnostic);
    printf("CHECK %d %s\n", status,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
    return status;
}

typedef enum {
    NO_FAULT,
    BOOL_FORMAL,
    MIXED_SEAL,
    ENTRY_WORLD
} Fault;

static Fault fault;
static bool reached;
static size_t fail_after, visits, hits, bytes;

void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
    if (!fail_after)
        return __real_malloc(size);
    ++visits;
    if (--fail_after)
        return __real_malloc(size);
    ++hits;
    bytes = size;
    return NULL;
}

NLCheckStatus __real_nl_sem_bind_in_scope(NLSemanticContext *, const char *,
                                          NLValueId, size_t, NLSymbolId *);
NLCheckStatus __wrap_nl_sem_bind_in_scope(NLSemanticContext *c,
                                          const char *name, NLValueId value,
                                          size_t floor, NLSymbolId *symbol)
{
    const bool target =
        fault == BOOL_FORMAL && !hits && !strcmp(name, "permit");
    if (target) {
        reached = c->binding_count == floor + 1 &&
                  !strcmp(c->bindings[floor].name, "payload") &&
                  c->bindings[floor].view.type ==
                      nl_semantic_core_type(c, NL_TYPE_USIZE) &&
                  c->bindings[floor].view.value != value &&
                  c->values[value - 1].type ==
                      nl_semantic_core_type(c, NL_TYPE_BOOL) &&
                  c->values[value - 1].carrier == NL_CARRIER_LOOSE;
        fail_after = 1;
    }
    const NLCheckStatus status =
        __real_nl_sem_bind_in_scope(c, name, value, floor, symbol);
    if (target)
        fail_after = 0;
    return status;
}

static bool mixed_call(const NLCheckedFragment *f,
                       const NLCheckedNodeView *call)
{
    if (!call || call->argument_count != 2)
        return false;
    const NLCheckedNodeView *first =
        nl_checked_node_view(f, call->first_argument);
    const NLCheckedNodeView *second =
        first ? nl_checked_node_view(f, first->next_argument) : NULL;
    const NLTypeId usize = nl_semantic_core_type(f->context, NL_TYPE_USIZE);
    const NLTypeId boolean = nl_semantic_core_type(f->context, NL_TYPE_BOOL);
    return second && first->parameter_type == usize &&
           second->parameter_type == boolean;
}

NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId id,
                                              const NLSemanticContext *world)
{
    const NLCheckedNodeView *call = nl_checked_node_view(parent, id);
    if (!hits && fault == MIXED_SEAL && !world && mixed_call(parent, call)) {
        reached = call->result_count == 1 && call->results[0].value &&
                  !nl_h_custody_result_valid(parent, id);
        fail_after = 1;
    } else if (!hits && fault == ENTRY_WORLD && world && call &&
               !call->argument_count && child->body_count == 2) {
        const NLCheckedNodeId first = child->body_calls[0],
                              second = child->body_calls[1];
        reached = mixed_call(child, nl_checked_node_view(child, first)) &&
                  mixed_call(child, nl_checked_node_view(child, second)) &&
                  nl_h_custody_result_valid(child, first) &&
                  nl_h_custody_result_valid(child, second) &&
                  !nl_h_custody_result_valid(parent, id);
        /* The entry body's nodes precede the actual world bytes. */
        fail_after = 2;
    }
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, id, world);
    fail_after = 0;
    return status;
}

static bool positions(const NLCheckedFragment *f, NLCheckedNodeId site,
                      const NLDivisionUsizePlan *plan,
                      const NLFunctionEntry *fn)
{
    const NLSemanticContext *c = f->context;
    const NLCheckedNodeView *call = nl_checked_node_view(f, site);
    const NLCheckedNodeView *result =
        nl_checked_node_view(plan->body, plan->result);
    NLSemanticValueView returned;
    NLCheckedNodeId stored = 0;
    REQUIRE(call->argument_count == 2 && call->result_count == 1 && result &&
            result->result_count == 1 &&
            result->results[0].value == call->results[0].value &&
            nl_semantic_value_view(c, result->results[0].value, &returned) &&
            returned.type == nl_semantic_core_type(c, NL_TYPE_USIZE) &&
            nl_h_custody_body_caller(plan->body, c, &stored) == f &&
            stored == site);
    printf("RESULT %s %d %zu\n", fn->name, returned.scalar_known,
           returned.scalar_value);
    const NLSymbolId formals[] = {plan->formal, plan->second_formal};
    REQUIRE(formals[0] && formals[0] + 1 == formals[1] &&
            formals[1] <= c->binding_count);
    NLCheckedNodeId previous = site, actual = call->first_argument;
    NLValueId prior_value = 0;
    NLPlaceId prior_place = 0;
    NLIncarnationId prior_incarnation = 0;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *arg = nl_checked_node_view(f, actual);
        const NLBindingEntry *binding = &c->bindings[formals[i] - 1];
        const NLTypeId type = fn->parameters[i];
        const bool boolean = type == nl_semantic_core_type(c, NL_TYPE_BOOL);
        NLSemanticValueView value;
        NLSemanticPlaceView place;
        REQUIRE(actual > previous && arg && arg->type == type &&
                arg->parameter_type == type && arg->result_count == 1 &&
                arg->results[0].type == type && binding->view.type == type &&
                arg->results[0].value == binding->view.value &&
                arg->results[0].value > prior_value && binding->hidden &&
                binding->view.availability == NL_CONSUMED &&
                !strcmp(binding->name, fn->body->parameter_names[i]) &&
                nl_semantic_value_view(c, binding->view.value, &value) &&
                value.type == type && value.carrier == NL_CARRIER_ENDED &&
                !value.owner_place && (!boolean || value.scalar_value <= 1) &&
                nl_semantic_place_view(c, binding->view.place, &place) &&
                place.type == type && place.independent_root &&
                place.incarnation && !place.live && !place.current_value &&
                (!i || (prior_place + 1 == binding->view.place &&
                        prior_incarnation + 1 == place.incarnation)));
        printf("POSITION %s %zu %s %d %zu node=%zu formal=%zu value=%zu "
               "place=%zu incarnation=%zu\n",
               fn->name, i, boolean ? "bool" : "usize", value.scalar_known,
               value.scalar_value, actual, formals[i], binding->view.value,
               binding->view.place, place.incarnation);
        if (!i && arg->kind == NL_CHECKED_REGISTERED_CALL) {
            REQUIRE(arg->next_argument > actual &&
                    nl_h_custody_result_valid(f, actual));
            printf("EARLY_LINK %s %zu %zu\n", fn->name, actual,
                   arg->next_argument);
        }
        previous = actual;
        prior_value = binding->view.value;
        prior_place = binding->view.place;
        prior_incarnation = place.incarnation;
        actual = arg->next_argument;
    }
    return !actual;
}

static bool trace(const NLCheckedFragment *f, bool entry)
{
    const NLSemanticContext *c = f->context;
    for (size_t i = 0; i < f->body_count; ++i) {
        const NLCheckedNodeId site = f->body_calls[i];
        NLDivisionUsizePlan plan;
        REQUIRE(nl_h_custody_result_valid(f, site) &&
                nl_quantity_usize_call_valid(f, site, &plan) &&
                (!entry || nl_h_custody_result_world_present(f, site)) &&
                plan.body == nl_checked_call_body(f, site) &&
                plan.body->context == c &&
                plan.body->body_owner == c->functions[plan.function - 1].body);
        const NLFunctionEntry *fn = &c->functions[plan.function - 1];
        printf("FUNCTION %zu %s\n", plan.function, fn->name);
        if (plan.second_formal)
            REQUIRE(positions(f, site, &plan, fn));
        for (NLCheckedNodeId j = 1; j <= plan.body->count; ++j) {
            const NLCheckedNodeView *n = nl_checked_node_view(plan.body, j);
            if (n->kind == NL_CHECKED_IF) {
                NLSemanticValueView value;
                REQUIRE(
                    n->result_count == 1 &&
                    nl_semantic_value_view(c, n->results[0].value, &value) &&
                    !value.scalar_known && !value.scalar_value);
                printf("JOIN %s Unknown\n", fn->name);
            } else if (n->kind == NL_CHECKED_BINDING) {
                const NLCheckedNodeView *init =
                    nl_checked_node_view(plan.body, n->initializer);
                if (init->type == nl_semantic_core_type(c, NL_TYPE_BOOL)) {
                    NLSemanticValueView value;
                    REQUIRE(nl_semantic_value_view(c, init->results[0].value,
                                                   &value) &&
                            value.type == init->type &&
                            value.scalar_value <= 1);
                    printf("BOOL %s %s %d %zu\n", fn->name,
                           c->bindings[n->symbol - 1].name, value.scalar_known,
                           value.scalar_value);
                }
            }
        }
        REQUIRE(trace(plan.body, false));
    }
    for (size_t i = 0; i < f->arm_count; ++i)
        REQUIRE(trace(f->arms[i].artifact, false));
    return true;
}

static bool withheld(const NLCheckedFragment *root, NLCheckStatus expected)
{
    char sentinel[] = "keep\0typed";
    char original[sizeof(sentinel)];
    memcpy(original, sentinel, sizeof(original));
    char *output = sentinel;
    size_t length = sizeof(sentinel);
    const NLCheckStatus status = nl_quantity_usize_c17(root, &output, &length);
    const bool unchanged = output == sentinel && length == sizeof(sentinel) &&
                           !memcmp(original, sentinel, sizeof(original));
    if (output != sentinel)
        free(output);
    return status == expected && unchanged;
}

static bool retry(const NLCheckedFragment *root, const char *pristine,
                  size_t size)
{
    char *out = NULL;
    size_t length = 0;
    const NLCheckStatus status = nl_quantity_usize_c17(root, &out, &length);
    const bool ok =
        status == NL_CHECK_OK && out && length &&
        (!pristine || (length == size && !memcmp(pristine, out, size)));
    free(out);
    return ok;
}

static bool rejected(const NLCheckedFragment *root,
                     const NLCheckedFragment *body, NLCheckedNodeId id)
{
    NLDivisionUsizePlan plan;
    return !nl_quantity_usize_call_valid(body, id, &plan) &&
           withheld(root, NL_CHECK_SEMANTIC_UNSUPPORTED);
}

static bool identity(Program *p)
{
    REQUIRE(p->root->body_count == 1 && p->root->bodies[0]->body_count == 2);
    NLCheckedFragment *body = p->root->bodies[0];
    const NLCheckedNodeId id = body->body_calls[0],
                          other_id = body->body_calls[1];
    NLDivisionUsizePlan plan, other;
    REQUIRE(nl_quantity_usize_call_valid(body, id, &plan) &&
            nl_quantity_usize_call_valid(body, other_id, &other) &&
            plan.function == other.function && plan.second_formal &&
            other.second_formal && plan.second_formal != other.second_formal);
    NLCheckedNodeView *call = &body->nodes[id - 1];
    const NLCheckedNodeId first = call->first_argument;
    NLCheckedNodeView *a = &body->nodes[first - 1];
    const NLCheckedNodeId second = a->next_argument;
    NLCheckedNodeView *b = &body->nodes[second - 1];
    const NLValueId payload = a->results[0].value, policy = b->results[0].value;
    NLSemanticContext *c = p->context;
    const NLValueId foreign = c->bindings[other.second_formal - 1].view.value;
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE);
    REQUIRE(!b->next_argument && payload != policy && policy != foreign &&
            c->values[policy - 1].type == boolean &&
            c->values[foreign - 1].type == boolean &&
            c->values[payload - 1].type == usize &&
            c->values[policy - 1].scalar_known &&
            c->values[foreign - 1].scalar_known &&
            c->values[payload - 1].scalar_known &&
            c->values[policy - 1].scalar_value == 1 &&
            c->values[foreign - 1].scalar_value == 1 &&
            c->values[payload - 1].scalar_value == 1);
    char *pristine = NULL;
    size_t size = 0;
    REQUIRE(nl_quantity_usize_c17(p->root, &pristine, &size) == NL_CHECK_OK);

    call->first_argument = second;
    a->next_argument = 0;
    b->next_argument = first;
    const bool order = rejected(p->root, body, id);
    b->next_argument = 0;
    a->next_argument = second;
    call->first_argument = first;

    b->results[0].value = foreign;
    const bool package = rejected(p->root, body, id);
    b->results[0].value = policy;
    b->parameter_type = b->type = b->results[0].type = usize;
    b->results[0].value = payload;
    const bool alias = rejected(p->root, body, id);
    b->parameter_type = b->type = b->results[0].type = boolean;
    b->results[0].value = policy;

    NLValueId *formal = &c->bindings[plan.second_formal - 1].view.value;
    *formal = foreign;
    const bool binding = rejected(p->root, body, id);
    *formal = policy;
    NLCheckedFragment *invocation = body->bodies[0];
    body->bodies[0] = body->bodies[1];
    const bool call_body = rejected(p->root, body, id);
    body->bodies[0] = invocation;

    NLSemanticContext *clone = NULL;
    const bool cloned = nl_sem_clone(c, &clone) == NL_CHECK_OK;
    if (cloned)
        invocation->context = clone;
    const bool world = cloned && rejected(p->root, body, id);
    invocation->context = c;
    nl_semantic_destroy(clone);

    const bool restored =
        nl_h_custody_validate(p->root) && retry(p->root, pristine, size);
    free(pristine);
    printf(
        "IDENTITY swapped_positions=%d foreign_bool=%d integer_alias=%d "
        "foreign_formal=%d foreign_body=%d duplicate_world=%d byte_retry=%d\n",
        order, package, alias, binding, call_body, world, restored);
    return order && package && alias && binding && call_body && world &&
           restored;
}

static bool check_fault(Program *p, Fault at)
{
    NLSemanticContext *c = p->context;
    NLSymbolId symbol = 0;
    const NLScalarValue seed = {nl_semantic_core_type(c, NL_TYPE_USIZE), true,
                                143};
    REQUIRE(nl_semantic_seed_scalar(c, "p1623_anchor", seed, &symbol) ==
                NL_CHECK_OK &&
            symbol);
    const NLSemanticBindingView anchor = c->bindings[symbol - 1].view;
    REQUIRE(anchor.place && anchor.value);
    unsigned char context[sizeof(*c)], binding[sizeof(*c->bindings)],
        place[sizeof(*c->places)], value[sizeof(*c->values)];
    memcpy(context, c, sizeof(context));
    memcpy(binding, c->bindings + symbol - 1, sizeof(binding));
    memcpy(place, c->places + anchor.place - 1, sizeof(place));
    memcpy(value, c->values + anchor.value - 1, sizeof(value));

    fault = at;
    const NLCheckStatus status = check(p);
    fault = NO_FAULT;
    printf("OOM reached=%d allocations=%zu hits=%zu bytes=%zu status=%d\n",
           reached, visits, hits, bytes, status);
    REQUIRE(reached && visits == (at == ENTRY_WORLD ? 2 : 1) && hits == 1 &&
            bytes && status == NL_CHECK_OUT_OF_MEMORY && !p->root &&
            !memcmp(context, c, sizeof(context)) &&
            !memcmp(binding, c->bindings + symbol - 1, sizeof(binding)) &&
            !memcmp(place, c->places + anchor.place - 1, sizeof(place)) &&
            !memcmp(value, c->values + anchor.value - 1, sizeof(value)) &&
            nl_sem_validate(c) == NL_CHECK_OK);
    REQUIRE(check(p) == NL_CHECK_OK && nl_h_custody_validate(p->root) &&
            trace(p->root, true) && retry(p->root, NULL, 0));
    puts("ROLLBACK checked output and caller state preserved; CHECK/C17 retry");
    return true;
}

static bool stage_fault(const NLCheckedFragment *root)
{
    char *pristine = NULL;
    size_t size = 0;
    REQUIRE(nl_quantity_usize_c17(root, &pristine, &size) == NL_CHECK_OK);
    fail_after = 1;
    const bool preserved = withheld(root, NL_CHECK_OUT_OF_MEMORY);
    fail_after = 0;
    const bool restored = retry(root, pristine, size);
    free(pristine);
    printf("OOM staging hits=%zu bytes=%zu no_clobber=%d byte_retry=%d\n", hits,
           bytes, preserved, restored);
    return visits == 1 && hits == 1 && bytes > size && preserved && restored;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0};
    const char *mode = argv[1];
    bool ok = prepare(&p, argv[2]);
    if (ok && !strcmp(mode, "bool-formal-oom"))
        ok = check_fault(&p, BOOL_FORMAL);
    else if (ok && !strcmp(mode, "mixed-seal-oom"))
        ok = check_fault(&p, MIXED_SEAL);
    else if (ok && !strcmp(mode, "entry-world-oom"))
        ok = check_fault(&p, ENTRY_WORLD);
    else if (ok) {
        ok = check(&p) == NL_CHECK_OK && nl_h_custody_validate(p.root);
        if (ok && !strcmp(mode, "trace"))
            ok = trace(p.root, true);
        else if (ok && !strcmp(mode, "identity"))
            ok = identity(&p);
        else if (ok && !strcmp(mode, "staging-oom"))
            ok = stage_fault(p.root);
        else
            ok = false;
    }
    nl_checked_destroy(p.root);
    nl_syntax_tree_destroy(p.expression);
    nl_parser_destroy(p.parser);
    nl_source_destroy(p.source);
    nl_semantic_destroy(p.context);
    return ok ? 0 : 1;
}
