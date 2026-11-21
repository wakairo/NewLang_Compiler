#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_addition.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"
#include "newlang/usize_subtraction.h"

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
    NLSyntaxTree *call;
    NLCheckedFragment *checked;
} Program;

static bool prepare(Program *p, const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    const NLSyntaxTree *units[1];
    bool ready =
        nl_source_load(path, &source) == NL_SOURCE_OK &&
        nl_parser_create(source, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
        nl_semantic_create(&p->context) == NL_CHECK_OK;
    units[0] = unit;
    ready = ready && nl_semantic_register_function_unit(p->context, units, 1,
                                                        NULL) == NL_CHECK_OK;
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ready &&
           nl_source_create("main()", 6, "p1596-entry", &p->source) ==
               NL_SOURCE_OK &&
           nl_parser_create(p->source, &p->parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(p->parser, &p->call, NULL) ==
               NL_PARSE_OK;
}

static NLCheckStatus check(Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->call, &p->checked, &diagnostic);
    printf("CHECK %d %s\n", status,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
    return status;
}

static const char *arithmetic_receipt(const NLCheckedFragment *f,
                                      NLCheckedNodeId id, NLCheckedKind kind)
{
    switch (kind) {
    case NL_CHECKED_USIZE_DIVISION:
        return nl_checked_usize_division_valid(f, id, NULL) ? "/" : NULL;
    case NL_CHECKED_USIZE_REMAINDER:
        return nl_checked_usize_remainder_valid(f, id, NULL) ? "%" : NULL;
    case NL_CHECKED_USIZE_ADDITION:
        return nl_checked_usize_addition_valid(f, id, NULL) ? "+" : NULL;
    case NL_CHECKED_USIZE_SUBTRACTION:
        return nl_checked_usize_subtraction_valid(f, id, NULL) ? "-" : NULL;
    case NL_CHECKED_REGISTERED_CALL:
        return nl_h_custody_result_valid(f, id) ? "call" : NULL;
    default:
        return "atom";
    }
}

static bool observe(const NLCheckedFragment *f, size_t depth, bool strict)
{
    REQUIRE(f && depth < NL_SEMANTIC_MAX_DEPTH);
    const NLSemanticContext *c = f->context;
    for (size_t i = 0; i < f->body_count; ++i) {
        const NLCheckedNodeId id = f->body_calls[i];
        const NLCheckedNodeView *call = nl_checked_node_view(f, id);
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        REQUIRE(call && call->kind == NL_CHECKED_REGISTERED_CALL &&
                call->body_backed && body && body == f->bodies[i] &&
                call->function && call->function <= c->function_count);
        for (size_t earlier = 0; earlier < i; ++earlier)
            REQUIRE(f->bodies[earlier] != body && f->body_calls[earlier] != id);
        const NLFunctionEntry *function = &c->functions[call->function - 1];
        REQUIRE(body->source == function->body->source &&
                body->body_owner == function->body);
        const bool sealed = nl_h_custody_result_valid(f, id);
        NLDivisionUsizePlan plan = {0};
        const bool valid = nl_quantity_usize_call_valid(f, id, &plan);
        const bool body_valid =
            valid ||
            nl_quantity_usize_body_at(body, c, call->function, !depth, &plan);
        printf("FUNCTION %zu %s\n", call->function, function->name);
        printf("CALL %s depth=%zu ordinal=%zu id=%zu seal=%d plan=%d body=%d\n",
               function->name, depth, i, id, sealed, valid, body_valid);
        REQUIRE(!strict || (sealed && valid && plan.body == body));
        if (!depth && strict)
            REQUIRE(nl_h_custody_result_world_present(f, id));
        if (call->argument_count) {
            const NLCheckedNodeId actual = call->first_argument;
            const NLCheckedNodeView *arg = nl_checked_node_view(f, actual);
            const NLCheckedNodeView *root =
                nl_checked_node_view(body, body->root);
            const NLCheckedResult body_result =
                root->terminates ? root->returned : root->results[0];
            const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE);
            NLSemanticValueView value;
            REQUIRE(call->argument_count == 1 && arg && !arg->next_argument &&
                    actual > id && arg->result_count == 1 &&
                    arg->type == usize && arg->parameter_type == usize &&
                    call->result_count == 1 && call->results[0].type == usize &&
                    call->results[0].value == body_result.value &&
                    call->results[0].type == body_result.type &&
                    nl_semantic_value_view(c, arg->results[0].value, &value) &&
                    value.type == usize &&
                    value.dependencies == NL_DEPENDENCY_FREE &&
                    !value.value_dependency_count);
            const char *op = arithmetic_receipt(f, actual, arg->kind);
            REQUIRE(op);
            NLValueId formal = 0;
            if (body_valid) {
                REQUIRE(plan.formal && plan.formal <= c->binding_count);
                formal = c->bindings[plan.formal - 1].view.value;
                REQUIRE(formal == arg->results[0].value);
            }
            printf("ACTUAL %s op=%s node=%zu result=%zu formal=%zu "
                   "known=%d number=%zu call_result=%zu\n",
                   function->name, op, actual, arg->results[0].value, formal,
                   value.scalar_known, value.scalar_value, body_result.value);
        }
        REQUIRE(observe(body, depth + 1, strict));
    }
    for (NLCheckedNodeId id = 1; id <= f->count; ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(f, id);
        if (depth == 1 && node->kind == NL_CHECKED_BINDING)
            printf("BIND %zu %s\n", node->symbol,
                   c->bindings[node->symbol - 1].name);
        if (node->kind == NL_CHECKED_IF) {
            NLSemanticValueView joined;
            REQUIRE(
                node->normal_arms == 2 && node->result_count == 1 &&
                nl_semantic_value_view(c, node->results[0].value, &joined) &&
                joined.type == nl_semantic_core_type(c, NL_TYPE_USIZE) &&
                !joined.scalar_known);
            printf("JOIN %zu Unknown\n", id);
        }
    }
    return true;
}

static bool preserves_output(const NLCheckedFragment *entry,
                             NLCheckStatus wanted)
{
    char sentinel[] = "unwritten";
    char *out = sentinel;
    size_t length = 43;
    NLCheckStatus status = nl_quantity_usize_c17(entry, &out, &length);
    bool preserved =
        out == sentinel && length == 43 && !strcmp(sentinel, "unwritten");
    if (out != sentinel)
        free(out);
    return status == wanted && preserved;
}

static bool matches_c(const NLCheckedFragment *entry, const char *expected,
                      size_t length)
{
    char *out = NULL;
    size_t bytes = 0;
    bool matched =
        nl_quantity_usize_c17(entry, &out, &bytes) == NL_CHECK_OK && out &&
        bytes &&
        (!expected || (bytes == length && !memcmp(out, expected, bytes)));
    free(out);
    return matched;
}

static bool other_origin(Program *p)
{
    NLCheckedFragment *entry = p->checked;
    NLCheckedFragment *main = (NLCheckedFragment *)nl_checked_call_body(
        entry, nl_checked_root(entry));
    REQUIRE(main && main->body_count >= 2);
    const NLCheckedNodeId first = main->body_calls[0],
                          second = main->body_calls[1];
    NLCheckedNodeView *call = &main->nodes[first - 1];
    const NLCheckedNodeId actual = call->first_argument;
    const NLCheckedNodeId foreign = main->nodes[second - 1].first_argument;
    NLCheckedNodeView *argument = &main->nodes[actual - 1];
    const NLCheckedNodeView *other = &main->nodes[foreign - 1];
    NLDivisionUsizePlan plan = {0};
    REQUIRE(argument->kind == NL_CHECKED_USIZE_DIVISION &&
            other->kind == NL_CHECKED_USIZE_REMAINDER && actual != foreign &&
            argument->scalar_result.known && other->scalar_result.known &&
            argument->scalar_result.value == other->scalar_result.value &&
            nl_checked_usize_division_valid(main, actual, NULL) &&
            nl_checked_usize_remainder_valid(main, foreign, NULL) &&
            nl_quantity_usize_call_valid(main, first, &plan) && plan.formal);
    const NLValueId original = argument->results[0].value;
    const NLValueId substitute = other->results[0].value;
    REQUIRE(original != substitute &&
            p->context->bindings[plan.formal - 1].view.value == original);
    char *clean = NULL;
    size_t length = 0;
    REQUIRE(nl_quantity_usize_c17(entry, &clean, &length) == NL_CHECK_OK);

    call->first_argument = foreign;
    bool edge = preserves_output(entry, NL_CHECK_SEMANTIC_UNSUPPORTED);
    call->first_argument = actual;
    argument->results[0].value = substitute;
    bool result = preserves_output(entry, NL_CHECK_SEMANTIC_UNSUPPORTED);
    argument->results[0].value = original;
    p->context->bindings[plan.formal - 1].view.value = substitute;
    bool formal = preserves_output(entry, NL_CHECK_SEMANTIC_UNSUPPORTED);
    p->context->bindings[plan.formal - 1].view.value = original;
    NLCheckedFragment *body = main->bodies[0];
    main->bodies[0] = main->bodies[1];
    bool invocation = preserves_output(entry, NL_CHECK_SEMANTIC_UNSUPPORTED);
    main->bodies[0] = body;
    const NLTypeId type = argument->parameter_type;
    argument->parameter_type = 0;
    bool parameter = preserves_output(entry, NL_CHECK_SEMANTIC_UNSUPPORTED);
    argument->parameter_type = type;

    bool retry = matches_c(entry, clean, length);
    free(clean);
    REQUIRE(edge && result && formal && invocation && parameter && retry);
    puts("IDENTITY other same-valued edge/result/formal/body and missing type "
         "refused, output held, byte-equal restore");
    return true;
}

typedef enum {
    NO_FAULT,
    ACTUAL_SEAL,
    ENTRY_WORLD
} Fault;
static Fault fault;
static bool fail_malloc;
static size_t visits, hits, allocation;

void *__real_malloc(size_t);

void *__wrap_malloc(size_t size)
{
    if (!fail_malloc)
        return __real_malloc(size);
    ++hits;
    allocation = size;
    return NULL;
}

NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);

NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *body,
                                              NLCheckedNodeId id,
                                              const NLSemanticContext *world)
{
    const NLCheckedNodeView *call = nl_checked_node_view(parent, id);
    const NLCheckedNodeView *arg =
        call ? nl_checked_node_view(parent, call->first_argument) : NULL;
    if ((fault == ACTUAL_SEAL && !world && arg &&
         arg->kind == NL_CHECKED_USIZE_DIVISION) ||
        (fault == ENTRY_WORLD && world && call && !call->argument_count)) {
        ++visits;
        fail_malloc = true;
        const NLCheckStatus status =
            __real_nl_h_custody_seal_result(parent, body, id, world);
        fail_malloc = false;
        return status;
    }
    return __real_nl_h_custody_seal_result(parent, body, id, world);
}

static bool seal_oom(Program *p, Fault site)
{
    NLSemanticContext *c = p->context;
    const NLScalarValue scalar = {nl_semantic_core_type(c, NL_TYPE_USIZE), true,
                                  61};
    NLSymbolId anchor = 0;
    REQUIRE(nl_semantic_seed_scalar(c, "p1596_anchor", scalar, &anchor) ==
                NL_CHECK_OK &&
            anchor);
    const NLSemanticBindingView before = c->bindings[anchor - 1].view;
    REQUIRE(before.place && before.value);
    unsigned char header[sizeof(*c)], binding[sizeof(*c->bindings)],
        place[sizeof(*c->places)], value[sizeof(*c->values)];
    memcpy(header, c, sizeof(header));
    memcpy(binding, c->bindings + anchor - 1, sizeof(binding));
    memcpy(place, c->places + before.place - 1, sizeof(place));
    memcpy(value, c->values + before.value - 1, sizeof(value));
    fault = site;
    const NLCheckStatus status = check(p);
    fault = NO_FAULT;
    printf("OOM %s visits=%zu hits=%zu bytes=%zu status=%d\n",
           site == ACTUAL_SEAL ? "actual-seal" : "entry-world", visits, hits,
           allocation, status);
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && visits == 1 && hits == 1 &&
            allocation && !p->checked && !memcmp(header, c, sizeof(header)) &&
            !memcmp(binding, c->bindings + anchor - 1, sizeof(binding)) &&
            !memcmp(place, c->places + before.place - 1, sizeof(place)) &&
            !memcmp(value, c->values + before.value - 1, sizeof(value)) &&
            nl_sem_validate(c) == NL_CHECK_OK);
    REQUIRE(check(p) == NL_CHECK_OK && nl_h_custody_validate(p->checked) &&
            observe(p->checked, 0, true) && matches_c(p->checked, NULL, 0));
    puts("OOM rollback context/binding/place/value, no artifact, retry PASS");
    return true;
}

static bool staging_oom(const NLCheckedFragment *entry)
{
    char *clean = NULL;
    size_t length = 0;
    REQUIRE(nl_quantity_usize_c17(entry, &clean, &length) == NL_CHECK_OK);
    fail_malloc = true;
    bool preserved = preserves_output(entry, NL_CHECK_OUT_OF_MEMORY);
    fail_malloc = false;
    const bool retry = matches_c(entry, clean, length);
    free(clean);
    printf("OOM staging hits=%zu bytes=%zu\n", hits, allocation);
    REQUIRE(preserved && hits == 1 && allocation > length && retry);
    puts("OOM C17 output pointer/length/bytes held, byte-equal retry PASS");
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0};
    const char *mode = argv[1];
    bool ok = prepare(&p, argv[2]);
    if (ok && !strcmp(mode, "actual-oom"))
        ok = seal_oom(&p, ACTUAL_SEAL);
    else if (ok && !strcmp(mode, "entry-oom"))
        ok = seal_oom(&p, ENTRY_WORLD);
    else if (ok) {
        ok = check(&p) == NL_CHECK_OK && nl_h_custody_validate(p.checked);
        if (ok && (!strcmp(mode, "baseline") || !strcmp(mode, "strict")))
            ok = observe(p.checked, 0, !strcmp(mode, "strict"));
        else if (ok && !strcmp(mode, "identity"))
            ok = other_origin(&p);
        else if (ok && !strcmp(mode, "staging-oom"))
            ok = staging_oom(p.checked);
        else
            ok = false;
    }
    nl_checked_destroy(p.checked);
    nl_syntax_tree_destroy(p.call);
    nl_parser_destroy(p.parser);
    nl_source_destroy(p.source);
    nl_semantic_destroy(p.context);
    return ok ? 0 : 1;
}
