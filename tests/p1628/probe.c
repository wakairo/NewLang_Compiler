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
    NLSyntaxTree *expression;
    NLCheckedFragment *entry;
} Program;

static bool prepare(Program *p, const char *file)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    bool ok =
        nl_source_load(file, &source) == NL_SOURCE_OK &&
        nl_parser_create(source, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
        nl_semantic_create(&p->context) == NL_CHECK_OK;
    const NLSyntaxTree *units[] = {unit};
    ok = ok && nl_semantic_register_function_unit(p->context, units, 1, NULL) ==
                   NL_CHECK_OK;
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok &&
           nl_source_create("main()", 6, "p1628-entry", &p->source) ==
               NL_SOURCE_OK &&
           nl_parser_create(p->source, &p->parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(p->parser, &p->expression,
                                               NULL) == NL_PARSE_OK;
}

static NLCheckStatus check(Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->expression, &p->entry, &diagnostic);
    printf("CHECK %d %s\n", status,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
    return status;
}

static bool walk_calls(const NLCheckedFragment *parent, size_t *calls)
{
    for (size_t i = 0; i < parent->body_count; ++i) {
        const NLCheckedNodeId site = parent->body_calls[i];
        const NLCheckedFragment *body = nl_checked_call_body(parent, site);
        NLDivisionUsizePlan plan;
        NLCheckedNodeId stored = 0;
        REQUIRE(body && nl_h_custody_result_valid(parent, site) &&
                nl_quantity_usize_call_valid(parent, site, &plan) &&
                plan.body == body &&
                nl_h_custody_body_caller(body, body->context, &stored) ==
                    parent &&
                stored == site);
        ++*calls;
        REQUIRE(walk_calls(body, calls));
    }
    for (size_t i = 0; i < parent->arm_count; ++i)
        REQUIRE(walk_calls(parent->arms[i].artifact, calls));
    return true;
}

static bool implicit_entry(Program *p)
{
    const NLCheckedNodeId site = nl_checked_root(p->entry);
    const NLCheckedNodeView *call = nl_checked_node_view(p->entry, site);
    const NLCheckedFragment *body = nl_checked_call_body(p->entry, site);
    NLDivisionUsizePlan plan;
    REQUIRE(nl_checked_proof_class(p->entry) == NL_CHECKED_MACHINE_PROVED &&
            nl_h_custody_validate(p->entry) && call && body &&
            nl_h_custody_result_valid(p->entry, site) &&
            nl_h_custody_result_world_present(p->entry, site) &&
            nl_quantity_usize_call_valid(p->entry, site, &plan) &&
            plan.body == body && plan.function == call->function &&
            !plan.formal && !plan.second_formal && !plan.result &&
            call->kind == NL_CHECKED_REGISTERED_CALL &&
            !call->argument_count && !call->first_argument &&
            call->type == nl_semantic_unit_type(p->context) &&
            !call->result_count && !call->results[0].value &&
            body->context == p->context && body->source &&
            body->body_owner);
    const NLFunctionEntry *function =
        &p->context->functions[call->function - 1];
    const NLSyntaxView *source =
        nl_syntax_node_view(nl_syntax_tree_root(function->body->syntax));
    const NLCheckedNodeView *block =
        nl_checked_node_view(body, body->root);
    REQUIRE(!strcmp(function->name, "main") && !function->count &&
            function->result == nl_semantic_unit_type(p->context) &&
            body->body_owner == function->body &&
            body->source == function->body->source && source &&
            source->kind == NL_SYNTAX_BLOCK && !source->data.block.tail &&
            block && block->kind == NL_CHECKED_BLOCK &&
            block->type == function->result && !block->tail &&
            !block->result_count && !block->results[0].value &&
            !block->terminates && !nl_control_exits_count(body->exits));
    size_t calls = 0;
    REQUIRE(walk_calls(p->entry, &calls));
    printf("ENTRY implicit function=%zu body=%zu items=%zu calls=%zu "
           "world=1 result=unit\n",
           call->function, body->root, block->item_count, calls);
    return true;
}

static bool withheld(const NLCheckedFragment *entry, NLCheckStatus expected)
{
    char sentinel[] = "keep\0implicit";
    char original[sizeof(sentinel)];
    memcpy(original, sentinel, sizeof(original));
    char *output = sentinel;
    size_t length = sizeof(sentinel);
    const NLCheckStatus status =
        nl_quantity_usize_c17(entry, &output, &length);
    const bool unchanged =
        output == sentinel && length == sizeof(sentinel) &&
        !memcmp(sentinel, original, sizeof(sentinel));
    if (output != sentinel)
        free(output);
    return status == expected && unchanged;
}

static bool emit(const NLCheckedFragment *entry, char **output, size_t *length)
{
    return nl_quantity_usize_c17(entry, output, length) == NL_CHECK_OK &&
           *output && *length;
}

static bool rejected(const Program *p)
{
    NLDivisionUsizePlan plan;
    return !nl_quantity_usize_call_valid(p->entry, nl_checked_root(p->entry),
                                         &plan) &&
           withheld(p->entry, NL_CHECK_SEMANTIC_UNSUPPORTED);
}

static bool identity(Program *p)
{
    REQUIRE(implicit_entry(p));
    char *pristine = NULL;
    size_t size = 0;
    REQUIRE(emit(p->entry, &pristine, &size));
    const NLCheckedNodeId site = nl_checked_root(p->entry);
    NLCheckedNodeView *call = &p->entry->nodes[site - 1];
    NLCheckedFragment *body = p->entry->bodies[0];
    NLCheckedNodeView *block = &body->nodes[body->root - 1];

    block->tail = block->first_item;
    const bool fake_tail = rejected(p);
    block->tail = 0;

    const NLSource *source = body->source;
    body->source = p->source;
    const bool foreign_source = rejected(p);
    body->source = source;

    NLFunctionBody *owner = body->body_owner;
    body->body_owner = NULL;
    const bool missing_owner = rejected(p);
    body->body_owner = owner;

    const NLSemanticContext *world = body->context;
    NLSemanticContext *clone = NULL;
    const bool cloned = nl_sem_clone(world, &clone) == NL_CHECK_OK;
    if (cloned)
        body->context = clone;
    const bool foreign_world = cloned && rejected(p);
    body->context = world;
    nl_semantic_destroy(clone);

    call->result_count = 1;
    const bool fake_result = rejected(p);
    call->result_count = 0;

    char *retry = NULL;
    size_t retry_size = 0;
    const bool restored =
        implicit_entry(p) && emit(p->entry, &retry, &retry_size) &&
        retry_size == size && !memcmp(retry, pristine, size);
    free(retry);
    free(pristine);
    printf("IDENTITY fake_tail=%d foreign_source=%d missing_owner=%d "
           "foreign_world=%d fake_result=%d byte_retry=%d\n",
           fake_tail, foreign_source, missing_owner, foreign_world,
           fake_result, restored);
    return fake_tail && foreign_source && missing_owner && foreign_world &&
           fake_result && restored;
}

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

static bool staging_oom(Program *p)
{
    REQUIRE(implicit_entry(p));
    char *pristine = NULL;
    size_t size = 0;
    REQUIRE(emit(p->entry, &pristine, &size));
    fail_after = 1;
    const bool preserved = withheld(p->entry, NL_CHECK_OUT_OF_MEMORY);
    fail_after = 0;
    char *retry = NULL;
    size_t retry_size = 0;
    const bool restored =
        emit(p->entry, &retry, &retry_size) && retry_size == size &&
        !memcmp(retry, pristine, size);
    free(retry);
    free(pristine);
    printf("OOM staging visits=%zu hits=%zu bytes=%zu no_clobber=%d "
           "byte_retry=%d\n",
           visits, hits, bytes, preserved, restored);
    return visits == 1 && hits == 1 && bytes > size && preserved && restored;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0};
    bool ok = prepare(&p, argv[2]) && check(&p) == NL_CHECK_OK;
    if (ok && !strcmp(argv[1], "inspect"))
        ok = implicit_entry(&p);
    else if (ok && !strcmp(argv[1], "identity"))
        ok = identity(&p);
    else if (ok && !strcmp(argv[1], "staging-oom"))
        ok = staging_oom(&p);
    else
        ok = false;
    nl_checked_destroy(p.entry);
    nl_syntax_tree_destroy(p.expression);
    nl_parser_destroy(p.parser);
    nl_source_destroy(p.source);
    nl_semantic_destroy(p.context);
    return ok ? 0 : 1;
}
