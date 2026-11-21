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
           nl_source_create("main()", 6, "p1615-entry", &p->source) ==
               NL_SOURCE_OK &&
           nl_parser_create(p->source, &p->parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(p->parser, &p->expression,
                                               NULL) == NL_PARSE_OK;
}

static void dispose(Program *p)
{
    nl_checked_destroy(p->root);
    nl_syntax_tree_destroy(p->expression);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    nl_semantic_destroy(p->context);
}

static NLCheckStatus check(Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->expression, &p->root, &diagnostic);
    printf("CHECK %d %s\n", status,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
    return status;
}

typedef enum {
    NO_FAULT,
    SECOND_FORMAL,
    RESULT_SEAL
} Fault;
static Fault fault;
static bool fail_next, reached;
static size_t hits, bytes;

void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
    if (!fail_next)
        return __real_malloc(size);
    fail_next = false;
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
        fault == SECOND_FORMAL && !hits && !strcmp(name, "chosen");
    if (target) {
        reached = c->binding_count == floor + 1 &&
                  !strcmp(c->bindings[floor].name, "ignored") &&
                  c->bindings[floor].view.value != value &&
                  c->bindings[floor].view.availability == NL_AVAILABLE &&
                  c->values[value - 1].carrier == NL_CARRIER_LOOSE;
        fail_next = true;
    }
    const NLCheckStatus status =
        __real_nl_sem_bind_in_scope(c, name, value, floor, symbol);
    if (target)
        fail_next = false;
    return status;
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
    const bool target = fault == RESULT_SEAL && !hits && !world && call &&
                        call->argument_count == 2;
    if (target) {
        reached = call->result_count == 1 && call->results[0].value &&
                  !nl_h_custody_result_valid(parent, id);
        fail_next = true;
    }
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, id, world);
    if (target)
        fail_next = false;
    return status;
}

static bool trace_positions(const NLCheckedFragment *caller, NLCheckedNodeId id,
                            const NLDivisionUsizePlan *plan, const char *name)
{
    const NLSemanticContext *c = caller->context;
    const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
    const NLCheckedNodeView *result =
        nl_checked_node_view(plan->body, plan->result);
    NLSemanticValueView returned;
    NLCheckedNodeId site = 0;
    REQUIRE(call->argument_count == 2 && call->result_count == 1 && result &&
            result->result_count == 1 &&
            result->results[0].value == call->results[0].value &&
            nl_semantic_value_view(c, call->results[0].value, &returned) &&
            nl_h_custody_body_caller(plan->body, c, &site) == caller &&
            site == id);
    printf("RESULT %s id=%zu known=%d number=%zu\n", name,
           call->results[0].value, returned.scalar_known,
           returned.scalar_value);
    const NLSymbolId symbols[] = {plan->formal, plan->second_formal};
    const NLFunctionEntry *function = &c->functions[plan->function - 1];
    REQUIRE(symbols[0] && symbols[1] == symbols[0] + 1);
    NLCheckedNodeId actual = call->first_argument, previous = id;
    NLValueId earlier = 0;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *arg = nl_checked_node_view(caller, actual);
        const NLBindingEntry *formal = &c->bindings[symbols[i] - 1];
        NLSemanticPlaceView place;
        NLSemanticValueView value;
        REQUIRE(actual > previous && arg && arg->result_count == 1 &&
                arg->results[0].value > earlier &&
                arg->results[0].value == formal->view.value && formal->hidden &&
                formal->view.availability == NL_CONSUMED &&
                !strcmp(formal->name, function->body->parameter_names[i]) &&
                nl_semantic_value_view(c, formal->view.value, &value) &&
                value.carrier == NL_CARRIER_ENDED && !value.owner_place &&
                nl_semantic_place_view(c, formal->view.place, &place) &&
                place.independent_root && place.incarnation && !place.live &&
                !place.current_value);
        printf("POSITION %s %zu node=%zu value=%zu formal=%zu name=%s "
               "place=%zu incarnation=%zu known=%d number=%zu\n",
               name, i, actual, formal->view.value, symbols[i], formal->name,
               formal->view.place, place.incarnation, value.scalar_known,
               value.scalar_value);
        earlier = formal->view.value;
        previous = actual;
        actual = arg->next_argument;
    }
    return !actual;
}

static bool trace(const NLCheckedFragment *caller, bool entry)
{
    const NLSemanticContext *c = caller->context;
    for (size_t i = 0; i < caller->body_count; ++i) {
        const NLCheckedNodeId id = caller->body_calls[i];
        NLDivisionUsizePlan plan;
        REQUIRE(nl_quantity_usize_call_valid(caller, id, &plan) &&
                nl_h_custody_result_valid(caller, id) &&
                (!entry || nl_h_custody_result_world_present(caller, id)) &&
                plan.body == nl_checked_call_body(caller, id) &&
                plan.body->context == c &&
                plan.body->body_owner == c->functions[plan.function - 1].body);
        const char *name = c->functions[plan.function - 1].name;
        printf("FUNCTION %zu %s\n", plan.function, name);
        if (plan.second_formal)
            REQUIRE(trace_positions(caller, id, &plan, name));
        for (NLCheckedNodeId j = 1; j <= plan.body->count; ++j) {
            const NLCheckedNodeView *n = nl_checked_node_view(plan.body, j);
            if (entry && n->kind == NL_CHECKED_BINDING)
                printf("BIND %zu %s\n", n->symbol,
                       c->bindings[n->symbol - 1].name);
            if (n->kind == NL_CHECKED_IF) {
                NLSemanticValueView value;
                REQUIRE(
                    n->result_count == 1 &&
                    nl_semantic_value_view(c, n->results[0].value, &value) &&
                    !value.scalar_known && !value.scalar_value);
                printf("JOIN %s %zu Unknown\n", name, j);
            }
        }
        REQUIRE(trace(plan.body, false));
    }
    for (size_t i = 0; i < caller->arm_count; ++i)
        REQUIRE(trace(caller->arms[i].artifact, false));
    return true;
}

static bool withheld(const NLCheckedFragment *root, NLCheckStatus expected)
{
    char sentinel[] = "keep\0pair";
    unsigned char original[sizeof(sentinel)];
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

static bool equal_number(const NLSemanticContext *world, NLValueId left,
                         NLValueId right)
{
    NLSemanticValueView a, b;
    return left != right && nl_semantic_value_view(world, left, &a) &&
           nl_semantic_value_view(world, right, &b) && a.scalar_known &&
           b.scalar_known && a.scalar_value == b.scalar_value;
}

static bool rejected(const NLCheckedFragment *root,
                     const NLCheckedFragment *caller, NLCheckedNodeId site)
{
    NLDivisionUsizePlan unused;
    return !nl_quantity_usize_call_valid(caller, site, &unused) &&
           withheld(root, NL_CHECK_SEMANTIC_UNSUPPORTED);
}

static bool identity(Program *p)
{
    /* The second and third keep_second calls both receive 11, 11 from
     * distinct actuals and have distinct result packages. */
    REQUIRE(p->root->body_count == 1);
    NLCheckedFragment *main_body = p->root->bodies[0];
    REQUIRE(main_body->body_count == 6);
    const size_t slot = 2, other_slot = 5;
    const NLCheckedNodeId id = main_body->body_calls[slot];
    const NLCheckedNodeId other_id = main_body->body_calls[other_slot];
    NLDivisionUsizePlan pair, other;
    REQUIRE(nl_quantity_usize_call_valid(main_body, id, &pair) &&
            nl_quantity_usize_call_valid(main_body, other_id, &other) &&
            pair.function == other.function && pair.second_formal &&
            other.second_formal && pair.second_formal != other.second_formal);
    NLCheckedNodeView *call = &main_body->nodes[id - 1];
    NLCheckedNodeView *other_call = &main_body->nodes[other_id - 1];
    const NLCheckedNodeId first = call->first_argument;
    NLCheckedNodeView *a = &main_body->nodes[first - 1];
    const NLCheckedNodeId second = a->next_argument;
    NLCheckedNodeView *b = &main_body->nodes[second - 1];
    NLCheckedNodeView *other_a =
        &main_body->nodes[other_call->first_argument - 1];
    NLCheckedNodeView *other_b = &main_body->nodes[other_a->next_argument - 1];
    const NLValueId av = a->results[0].value, bv = b->results[0].value;
    const NLValueId output = call->results[0].value;
    NLSemanticContext *world = p->context;
    REQUIRE(!b->next_argument && equal_number(world, av, bv) &&
            equal_number(world, bv, other_b->results[0].value) &&
            equal_number(world, output, other_call->results[0].value));
    char *pristine = NULL;
    size_t size = 0;
    REQUIRE(nl_quantity_usize_c17(p->root, &pristine, &size) == NL_CHECK_OK);

    call->first_argument = second;
    a->next_argument = 0;
    b->next_argument = first;
    const bool position = rejected(p->root, main_body, id);
    b->next_argument = 0;
    a->next_argument = second;
    call->first_argument = first;

    a->results[0].value = bv;
    const bool duplicate = rejected(p->root, main_body, id);
    a->results[0].value = av;

    b->results[0].value = other_b->results[0].value;
    const bool actual = rejected(p->root, main_body, id);
    b->results[0].value = bv;

    NLValueId *formal = &world->bindings[pair.second_formal - 1].view.value;
    *formal = world->bindings[other.second_formal - 1].view.value;
    const bool binding = rejected(p->root, main_body, id);
    *formal = bv;

    main_body->bodies[slot] = main_body->bodies[other_slot];
    const bool body = rejected(p->root, main_body, id);
    main_body->bodies[slot] = (NLCheckedFragment *)pair.body;

    call->results[0].value = other_call->results[0].value;
    const bool result = rejected(p->root, main_body, id);
    call->results[0].value = output;

    NLCheckedNodeId site = 0;
    NLSemanticContext *clone = NULL;
    const bool cloned = nl_sem_clone(world, &clone) == NL_CHECK_OK;
    NLCheckedFragment *invocation = main_body->bodies[slot];
    if (cloned)
        invocation->context = clone;
    const bool wrong_world =
        cloned && !nl_h_custody_body_caller(pair.body, clone, &site) &&
        rejected(p->root, main_body, id);
    invocation->context = world;
    nl_semantic_destroy(clone);

    const bool restored =
        nl_h_custody_validate(p->root) && retry(p->root, pristine, size);
    free(pristine);
    printf("IDENTITY swapped_position=%d duplicate_actual=%d other_actual=%d "
           "other_formal=%d other_body=%d other_world=%d other_result=%d "
           "byte_retry=%d\n",
           position, duplicate, actual, binding, body, wrong_world, result,
           restored);
    return position && duplicate && actual && binding && body && wrong_world &&
           result && restored;
}

static bool check_fault(Program *p, Fault at)
{
    NLSemanticContext *c = p->context;
    NLSymbolId symbol = 0;
    const NLScalarValue seed = {nl_semantic_core_type(c, NL_TYPE_USIZE), true,
                                123};
    REQUIRE(nl_semantic_seed_scalar(c, "p1615_anchor", seed, &symbol) ==
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
    printf("OOM reached=%d hits=%zu bytes=%zu status=%d\n", reached, hits,
           bytes, status);
    REQUIRE(reached && hits == 1 && bytes && status == NL_CHECK_OUT_OF_MEMORY &&
            !p->root && !memcmp(context, c, sizeof(context)) &&
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
    fail_next = true;
    const bool preserved = withheld(root, NL_CHECK_OUT_OF_MEMORY);
    fail_next = false;
    const bool restored = retry(root, pristine, size);
    free(pristine);
    printf("OOM staging hits=%zu bytes=%zu no_clobber=%d byte_retry=%d\n", hits,
           bytes, preserved, restored);
    return hits == 1 && bytes > size && preserved && restored;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0};
    const char *mode = argv[1];
    bool ok = prepare(&p, argv[2]);
    if (ok && !strcmp(mode, "second-formal-oom"))
        ok = check_fault(&p, SECOND_FORMAL);
    else if (ok && !strcmp(mode, "result-seal-oom"))
        ok = check_fault(&p, RESULT_SEAL);
    else if (ok) {
        ok = check(&p) == NL_CHECK_OK && nl_h_custody_validate(p.root);
        if (ok && !strcmp(mode, "trace"))
            ok = trace(p.root, true);
        else if (ok && !strcmp(mode, "identity"))
            ok = identity(&p);
        else if (ok && !strcmp(mode, "c17-staging-oom"))
            ok = stage_fault(p.root);
        else
            ok = false;
    }
    dispose(&p);
    return ok ? 0 : 1;
}
