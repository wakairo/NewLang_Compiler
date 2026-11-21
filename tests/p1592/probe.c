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
    NLSemanticContext *context;
    NLSource *source;
    NLParser *parser;
    NLSyntaxTree *entry;
    NLCheckedFragment *artifact;
} Program;

static bool prepare_program(Program *program, const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    const bool parsed =
        nl_source_load(path, &source) == NL_SOURCE_OK &&
        nl_parser_create(source, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
        nl_semantic_create(&program->context) == NL_CHECK_OK;
    const NLSyntaxTree *units[] = {unit};
    const bool registered =
        parsed && nl_semantic_register_function_unit(program->context, units, 1,
                                                     NULL) == NL_CHECK_OK;
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return registered &&
           nl_source_create("main()", 6, "p1592-entry", &program->source) ==
               NL_SOURCE_OK &&
           nl_parser_create(program->source, &program->parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(program->parser, &program->entry,
                                               NULL) == NL_PARSE_OK;
}

static NLCheckStatus check_program(Program *program)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        program->context, program->entry, &program->artifact, &diagnostic);
    printf("CHECK %d %s\n", status,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
    return status;
}

static void destroy_program(Program *program)
{
    nl_checked_destroy(program->artifact);
    nl_syntax_tree_destroy(program->entry);
    nl_parser_destroy(program->parser);
    nl_source_destroy(program->source);
    nl_semantic_destroy(program->context);
}

static bool usize_value(const NLSemanticContext *c, NLCheckedResult result,
                        NLSemanticValueView *value)
{
    return result.type == nl_semantic_core_type(c, NL_TYPE_USIZE) &&
           nl_semantic_value_view(c, result.value, value) &&
           value->type == result.type &&
           value->dependencies == NL_DEPENDENCY_FREE &&
           !value->value_dependency_count &&
           (value->scalar_known || value->scalar_value == 0);
}

static bool observe(const NLCheckedFragment *, const NLCheckedFragment *,
                    size_t);

typedef enum {
    NO_FAULT,
    INNER_SEAL,
    OUTER_SEAL
} Fault;
static Fault fault;
static bool fail_allocation;
static size_t seal_visits, allocation_hits, allocation_bytes;
static NLCheckStatus injected;

void *__real_malloc(size_t);

void *__wrap_malloc(size_t size)
{
    if (!fail_allocation)
        return __real_malloc(size);
    ++allocation_hits;
    allocation_bytes = size;
    return NULL;
}

NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId call,
                                              const NLSemanticContext *world)
{
    if (fault != NO_FAULT && !world) {
        const NLCheckedNodeView *node = nl_checked_node_view(parent, call);
        const NLCheckedNodeView *actual =
            node ? nl_checked_node_view(parent, node->first_argument) : NULL;
        const NLTypeId usize =
            nl_semantic_core_type(parent->context, NL_TYPE_USIZE);
        if (node && node->type == usize &&
            ((fault == INNER_SEAL && node->parameter_type == usize) ||
             (fault == OUTER_SEAL && !node->parameter_type && actual &&
              actual->kind == NL_CHECKED_REGISTERED_CALL))) {
            ++seal_visits;
            fail_allocation = true;
            injected =
                __real_nl_h_custody_seal_result(parent, child, call, world);
            fail_allocation = false;
            return injected;
        }
    }
    return __real_nl_h_custody_seal_result(parent, child, call, world);
}

static bool observe_call(const NLCheckedFragment *parent,
                         const NLCheckedFragment *entry, size_t ordinal,
                         size_t depth)
{
    const NLCheckedNodeId id = parent->body_calls[ordinal];
    const NLCheckedNodeView *call = nl_checked_node_view(parent, id);
    const NLCheckedFragment *body = nl_checked_call_body(parent, id);
    const NLSemanticContext *c = parent->context;
    REQUIRE(call && call->kind == NL_CHECKED_REGISTERED_CALL &&
            call->body_backed && body && body == parent->bodies[ordinal] &&
            call->function && call->function <= c->function_count);
    const NLFunctionEntry *fn = &c->functions[call->function - 1];
    REQUIRE(body->source == fn->body->source && body->body_owner == fn->body);
    for (size_t previous = 0; previous < ordinal; ++previous)
        REQUIRE(parent->bodies[previous] != body &&
                parent->body_calls[previous] != id);
    const bool is_main = parent == entry;
    NLDivisionUsizePlan plan = {0};
    REQUIRE(nl_h_custody_result_valid(parent, id) &&
            nl_quantity_usize_call_valid(parent, id, &plan) &&
            plan.body == body);
    printf("FUNCTION %zu %s\n", call->function, fn->name);
    printf("CUSTODY %s ordinal=%zu call=%zu PASS\n", fn->name, ordinal, id);
    if (!is_main) {
        const NLCheckedNodeView *arg =
            nl_checked_node_view(parent, call->first_argument);
        const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
        NLSemanticValueView actual, result;
        REQUIRE(call->argument_count == 1 && call->result_count == 1 && arg &&
                root && (root->terminates || root->result_count == 1) &&
                arg->result_count == 1 && !arg->next_argument &&
                call->first_argument > id &&
                usize_value(c, arg->results[0], &actual) &&
                usize_value(c, call->results[0], &result));
        const NLCheckedResult owned_result =
            root->terminates ? root->returned : root->results[0];
        REQUIRE(owned_result.type == call->results[0].type &&
                owned_result.value == call->results[0].value);
        REQUIRE(plan.formal && plan.formal <= c->binding_count);
        const NLValueId formal_value = c->bindings[plan.formal - 1].view.value;
        REQUIRE(formal_value == arg->results[0].value);
        printf("ACTUAL %s call=%zu arg=%zu inner=%d actual=%zu formal=%zu "
               "result=%zu known=%d/%d\n",
               fn->name, id, call->first_argument,
               arg->kind == NL_CHECKED_REGISTERED_CALL, arg->results[0].value,
               formal_value, owned_result.value, actual.scalar_known,
               result.scalar_known);
        if (arg->kind == NL_CHECKED_REGISTERED_CALL) {
            bool previous_body = false;
            for (size_t i = 0; i < ordinal; ++i)
                if (parent->body_calls[i] == call->first_argument)
                    previous_body = true;
            REQUIRE(previous_body && arg->parameter_type == arg->type &&
                    nl_h_custody_result_valid(parent, call->first_argument));
        }
    } else {
        REQUIRE(nl_checked_proof_class(entry) == NL_CHECKED_MACHINE_PROVED &&
                nl_h_custody_result_world_present(entry, id));
        puts("ENTRY machine-proved world PASS");
    }
    return observe(body, entry, depth + 1);
}

static bool observe(const NLCheckedFragment *fragment,
                    const NLCheckedFragment *entry, size_t depth)
{
    REQUIRE(fragment && depth < NL_SEMANTIC_MAX_DEPTH);
    const NLCheckedFragment *main =
        nl_checked_call_body(entry, nl_checked_root(entry));
    for (size_t i = 0; i < fragment->body_count; ++i)
        REQUIRE(observe_call(fragment, entry, i, depth));
    for (NLCheckedNodeId id = 1; id <= fragment->count; ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (node->kind == NL_CHECKED_BINDING && fragment == main)
            printf("BIND %zu %s\n", node->symbol,
                   fragment->context->bindings[node->symbol - 1].name);
        if (node->kind != NL_CHECKED_IF)
            continue;
        NLSemanticValueView joined;
        REQUIRE(node->normal_arms == 2 && node->result_count == 1 &&
                usize_value(fragment->context, node->results[0], &joined) &&
                !joined.scalar_known);
        printf("IF-JOIN %zu Unknown\n", id);
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedFragment *arm = nl_checked_if_arm(fragment, id, i);
            REQUIRE(arm && arm->if_parent == fragment &&
                    arm->if_parent_node == id && arm->if_arm_index == i &&
                    observe(arm, entry, depth + 1));
        }
    }
    return true;
}

static bool refuses(const NLCheckedFragment *entry)
{
    char sentinel[] = "preserve output";
    char *out = sentinel;
    size_t length = 53;
    const NLCheckStatus status = nl_quantity_usize_c17(entry, &out, &length);
    if (status == NL_CHECK_OK && out != sentinel)
        free(out);
    return status == NL_CHECK_SEMANTIC_UNSUPPORTED && out == sentinel &&
           length == 53 && !strcmp(sentinel, "preserve output");
}

static bool emit_same(const NLCheckedFragment *entry, const char *expected,
                      size_t bytes)
{
    char *out = NULL;
    size_t length = 0;
    const bool matches =
        nl_quantity_usize_c17(entry, &out, &length) == NL_CHECK_OK && out &&
        length &&
        (!expected || (bytes == length && !memcmp(out, expected, bytes)));
    free(out);
    return matches;
}

static bool identity(Program *p)
{
    NLCheckedFragment *entry = p->artifact;
    NLCheckedFragment *main = (NLCheckedFragment *)nl_checked_call_body(
        entry, nl_checked_root(entry));
    REQUIRE(main && main->body_count == 4 && refuses(main));
    const NLCheckedNodeId inner = main->body_calls[0],
                          outer = main->body_calls[1],
                          other = main->body_calls[2];
    NLCheckedNodeView *first = &main->nodes[inner - 1];
    NLCheckedNodeView *result = &main->nodes[outer - 1];
    const NLCheckedNodeView *second = nl_checked_node_view(main, other);
    NLDivisionUsizePlan plan;
    NLSemanticValueView a, b;
    const NLValueId original = first->results[0].value,
                    substitute = second->results[0].value;
    REQUIRE(inner != other && original != substitute &&
            first->function == second->function &&
            result->first_argument == inner &&
            main->bodies[0] != main->bodies[2] &&
            main->bodies[0]->body_owner == main->bodies[2]->body_owner &&
            nl_quantity_usize_call_valid(main, outer, &plan) && plan.formal &&
            usize_value(p->context, first->results[0], &a) &&
            usize_value(p->context, second->results[0], &b) && a.scalar_known &&
            b.scalar_known && a.scalar_value == b.scalar_value &&
            first->parameter_type == first->type &&
            p->context->bindings[plan.formal - 1].view.value == original);
    char *clean = NULL;
    size_t length = 0;
    REQUIRE(nl_quantity_usize_c17(entry, &clean, &length) == NL_CHECK_OK &&
            clean && length);

    result->first_argument = other;
    const bool rejects_edge = refuses(entry);
    result->first_argument = inner;

    first->results[0].value = substitute;
    const bool rejects_result = refuses(entry);
    first->results[0].value = original;

    p->context->bindings[plan.formal - 1].view.value = substitute;
    const bool rejects_formal = refuses(entry);
    p->context->bindings[plan.formal - 1].view.value = original;

    NLCheckedFragment *body = main->bodies[0];
    main->bodies[0] = main->bodies[2];
    const bool rejects_body = refuses(entry);
    main->bodies[0] = body;

    const NLTypeId parameter = first->parameter_type;
    first->parameter_type = 0;
    const bool rejects_type = refuses(entry);
    first->parameter_type = parameter;

    const bool retry = emit_same(entry, clean, length);
    free(clean);
    REQUIRE(rejects_edge && rejects_result && rejects_formal && rejects_body &&
            rejects_type && retry);
    puts("IDENTITY same-valued actual edge, result, formal, body and missing "
         "parameter type refused; byte-equal restore PASS");
    return true;
}

static bool seal_oom(Program *p, Fault site)
{
    NLSemanticContext *c = p->context;
    NLSymbolId anchor = 0;
    const NLScalarValue scalar = {nl_semantic_core_type(c, NL_TYPE_USIZE), true,
                                  53};
    REQUIRE(nl_semantic_seed_scalar(c, "p1592_anchor", scalar, &anchor) ==
                NL_CHECK_OK &&
            anchor);
    const NLSemanticBindingView prior = c->bindings[anchor - 1].view;
    REQUIRE(prior.place && prior.value);
    unsigned char header[sizeof(*c)], binding[sizeof(*c->bindings)],
        place[sizeof(*c->places)], value[sizeof(*c->values)];
    memcpy(header, c, sizeof(header));
    memcpy(binding, &c->bindings[anchor - 1], sizeof(binding));
    memcpy(place, &c->places[prior.place - 1], sizeof(place));
    memcpy(value, &c->values[prior.value - 1], sizeof(value));
    fault = site;
    const NLCheckStatus status = check_program(p);
    fault = NO_FAULT;
    printf("OOM %s visits=%zu hits=%zu bytes=%zu status=%d\n",
           site == INNER_SEAL ? "inner-seal" : "outer-seal", seal_visits,
           allocation_hits, allocation_bytes, status);
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && injected == status &&
            !p->artifact && seal_visits == 1 && allocation_hits == 1 &&
            allocation_bytes && !memcmp(header, c, sizeof(header)) &&
            !memcmp(binding, &c->bindings[anchor - 1], sizeof(binding)) &&
            !memcmp(place, &c->places[prior.place - 1], sizeof(place)) &&
            !memcmp(value, &c->values[prior.value - 1], sizeof(value)) &&
            nl_semantic_find_binding(c, "p1592_anchor") == anchor &&
            nl_sem_validate(c) == NL_CHECK_OK);
    REQUIRE(check_program(p) == NL_CHECK_OK &&
            nl_h_custody_validate(p->artifact) &&
            observe(p->artifact, p->artifact, 0) &&
            emit_same(p->artifact, NULL, 0));
    puts("OOM no artifact and caller context/binding/place/value unchanged; "
         "normal retry PASS");
    return true;
}

static bool staging_oom(const NLCheckedFragment *entry)
{
    char *clean = NULL;
    size_t bytes = 0;
    REQUIRE(nl_quantity_usize_c17(entry, &clean, &bytes) == NL_CHECK_OK &&
            clean && bytes);
    char sentinel[] = "preserve staging";
    char *out = sentinel;
    size_t length = 53;
    fail_allocation = true;
    const NLCheckStatus status = nl_quantity_usize_c17(entry, &out, &length);
    fail_allocation = false;
    const bool held = status == NL_CHECK_OUT_OF_MEMORY &&
                      allocation_hits == 1 && allocation_bytes > bytes &&
                      out == sentinel && length == 53 &&
                      !strcmp(sentinel, "preserve staging");
    if (out != sentinel)
        free(out);
    const bool retry = emit_same(entry, clean, bytes);
    free(clean);
    printf("OOM staging hits=%zu bytes=%zu status=%d\n", allocation_hits,
           allocation_bytes, status);
    REQUIRE(held && retry);
    puts(
        "OOM C17 output pointer, length and bytes held; byte-equal retry PASS");
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0};
    bool ok = prepare_program(&p, argv[2]);
    const char *mode = argv[1];
    if (ok) {
        if (!strcmp(mode, "inner-oom") || !strcmp(mode, "outer-oom"))
            ok = seal_oom(&p, mode[0] == 'i' ? INNER_SEAL : OUTER_SEAL);
        else if (check_program(&p) != NL_CHECK_OK ||
                 !nl_h_custody_validate(p.artifact))
            ok = false;
        else if (!strcmp(mode, "strict"))
            ok = observe(p.artifact, p.artifact, 0);
        else if (!strcmp(mode, "identity"))
            ok = identity(&p);
        else if (!strcmp(mode, "staging-oom"))
            ok = staging_oom(p.artifact);
        else
            ok = false;
    }
    destroy_program(&p);
    return ok ? 0 : 1;
}
