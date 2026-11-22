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
    NLCheckedFragment *entry;
    bool registered;
} Program;

static bool prepare(Program *p, const char *file)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLParseDiagnostic parse_diag = {0};
    NLFunctionUnitDiagnostic register_diag = {0};
    bool ok = nl_source_load(file, &source) == NL_SOURCE_OK &&
              nl_parser_create(source, &parser) == NL_PARSE_OK;
    if (ok) {
        const NLParseStatus parse =
            nl_parser_parse_function_unit(parser, &unit, &parse_diag);
        printf("PARSE %d %s\n", parse,
               parse_diag.diagnostic.code ? parse_diag.diagnostic.code : "-");
        ok = parse == NL_PARSE_OK;
    }
    ok = ok && nl_semantic_create(&p->context) == NL_CHECK_OK;
    if (ok) {
        const NLSyntaxTree *units[] = {unit};
        const NLCheckStatus status = nl_semantic_register_function_unit(
            p->context, units, 1, &register_diag);
        const char *code = register_diag.diagnostic.diagnostic.code;
        printf("REGISTER %d %s\n", status, code ? code : "-");
        p->registered = status == NL_CHECK_OK;
        ok = p->registered || status == NL_CHECK_SEMANTIC_ERROR;
    }
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok && (!p->registered ||
                  (nl_source_create("main()", 6, "p1632-entry", &p->source) ==
                       NL_SOURCE_OK &&
                   nl_parser_create(p->source, &p->parser) == NL_PARSE_OK &&
                   nl_parser_parse_expression_fragment(
                       p->parser, &p->expression, NULL) == NL_PARSE_OK));
}

static NLCheckStatus check(Program *p)
{
    NLCheckDiagnostic diag = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->expression, &p->entry, &diag);
    printf("CHECK %d %s\n", status,
           diag.diagnostic.code ? diag.diagnostic.code : "-");
    return status;
}

static bool span_equal(NLSourceSpan a, NLSourceSpan b)
{
    return a.start_byte == b.start_byte && a.end_byte == b.end_byte;
}

static bool named_discard(const NLCheckedFragment *body, NLCheckedNodeId id,
                          const NLSyntaxView *syntax, const char *function)
{
    const NLSemanticContext *c = body->context;
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    const NLCheckedNodeView *stmt = nl_checked_node_view(body, id);
    const NLCheckedNodeView *copy =
        nl_checked_node_view(body, stmt->initializer);
    const NLSyntaxView *name =
        nl_syntax_node_view(syntax->data.statement.expression);
    REQUIRE(!stmt->terminates && stmt->type == nl_semantic_unit_type(c) &&
            !stmt->result_count && !stmt->results[0].value && copy && name &&
            name->kind == NL_SYNTAX_EXPR_NAME &&
            copy->kind == NL_CHECKED_IDENTIFIER &&
            copy->value_use == NL_VALUE_COPIED && copy->result_count == 1 &&
            copy->results[0].type == boolean && copy->symbol &&
            copy->symbol <= c->binding_count &&
            span_equal(copy->span, name->span) &&
            span_equal(copy->name, name->data.name));

    const NLBindingEntry *source = &c->bindings[copy->symbol - 1];
    NLSourceView spelling;
    REQUIRE(source->view.type == boolean &&
            nl_source_view(body->source, copy->name, &spelling) &&
            spelling.length == strlen(source->name) &&
            !memcmp(spelling.bytes, source->name, spelling.length));
    const NLValueId original_id = source->view.value;
    const NLValueId discard_id = copy->results[0].value;
    NLSemanticValueView original, discarded;
    REQUIRE(original_id < discard_id &&
            nl_semantic_value_view(c, original_id, &original) &&
            nl_semantic_value_view(c, discard_id, &discarded) &&
            original.type == boolean && discarded.type == boolean &&
            discarded.carrier == NL_CARRIER_ENDED && !discarded.owner_place &&
            discarded.scalar_known == original.scalar_known &&
            discarded.scalar_value == original.scalar_value);

    const NLCheckedNodeView *reuse = NULL;
    for (NLCheckedNodeId later = id + 1; later <= body->count; ++later) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, later);
        if (node->kind == NL_CHECKED_IDENTIFIER &&
            node->symbol == copy->symbol &&
            node->span.start_byte > copy->span.end_byte) {
            reuse = node;
            break;
        }
    }
    NLSemanticValueView reused;
    REQUIRE(reuse && reuse->type == boolean &&
            reuse->value_use == NL_VALUE_COPIED && reuse->result_count == 1 &&
            reuse->results[0].type == boolean &&
            discard_id < reuse->results[0].value &&
            nl_semantic_value_view(c, reuse->results[0].value, &reused) &&
            reused.scalar_known == original.scalar_known &&
            reused.scalar_value == original.scalar_value);
    printf("DISCARD %s %s %d %zu %zu original=%zu discard=%zu reuse=%zu "
           "symbol=%zu span=%zu:%zu later=%zu:%zu\n",
           function, source->name, original.scalar_known, original.scalar_value,
           stmt->result_count, original_id, discard_id, reuse->results[0].value,
           copy->symbol, copy->span.start_byte, copy->span.end_byte,
           reuse->span.start_byte, reuse->span.end_byte);
    return true;
}

static bool inspect_items(const NLCheckedFragment *body,
                          const NLFunctionEntry *function)
{
    const NLSemanticContext *c = body->context;
    const NLCheckedNodeView *block = nl_checked_node_view(body, body->root);
    const NLSyntaxView *source =
        nl_syntax_node_view(nl_syntax_tree_root(function->body->syntax));
    REQUIRE(block && source && block->kind == NL_CHECKED_BLOCK &&
            source->kind == NL_SYNTAX_BLOCK &&
            span_equal(block->span, source->span));
    const NLSyntaxNode *item = source->data.block.items;
    for (NLCheckedNodeId id = block->first_item; id;) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, id);
        const NLSyntaxView *syntax = nl_syntax_node_view(item);
        REQUIRE(node && syntax && span_equal(node->span, syntax->span) &&
                (!node->next_item || node->next_item > id));
        if (node->kind == NL_CHECKED_BINDING) {
            REQUIRE(node->symbol && node->symbol <= c->binding_count);
            printf("LOCAL %s %zu %s\n", function->name, node->symbol,
                   c->bindings[node->symbol - 1].name);
        } else if (node->kind == NL_CHECKED_STATEMENT) {
            REQUIRE(syntax->kind == NL_SYNTAX_STATEMENT);
            const NLCheckedNodeView *init =
                nl_checked_node_view(body, node->initializer);
            REQUIRE(init);
            if (init->kind == NL_CHECKED_IDENTIFIER &&
                init->type == nl_semantic_core_type(c, NL_TYPE_BOOL))
                REQUIRE(named_discard(body, id, syntax, function->name));
        }
        item = nl_syntax_next_argument(item);
        id = node->next_item;
    }
    return !item;
}

static bool walk(const NLCheckedFragment *parent)
{
    for (size_t i = 0; i < parent->body_count; ++i) {
        const NLCheckedNodeId id = parent->body_calls[i];
        const NLCheckedNodeView *call = nl_checked_node_view(parent, id);
        const NLCheckedFragment *body = nl_checked_call_body(parent, id);
        REQUIRE(call && body && body->context == parent->context &&
                call->kind == NL_CHECKED_REGISTERED_CALL && call->body_backed &&
                call->function &&
                call->function <= parent->context->function_count &&
                nl_h_custody_observe_body(parent, id) == body);
        const NLFunctionEntry *fn =
            &parent->context->functions[call->function - 1];
        REQUIRE(fn->body && body->body_owner == fn->body &&
                body->source == fn->body->source);
        NLDivisionUsizePlan plan;
        printf("EDGE %s %zu body=%zu result=%zu seal=%d world=%d plan=%d\n",
               fn->name, call->function, body->root, call->result_count,
               nl_h_custody_result_valid(parent, id),
               nl_h_custody_result_world_present(parent, id),
               nl_quantity_usize_call_valid(parent, id, &plan));
        REQUIRE(inspect_items(body, fn) && walk(body));
    }
    return true;
}

static bool inspect(const Program *p)
{
    const NLCheckedNodeId root = nl_checked_root(p->entry);
    const NLCheckedNodeView *entry = nl_checked_node_view(p->entry, root);
    REQUIRE(entry && p->entry->body_count == 1 &&
            nl_h_custody_validate(p->entry) &&
            entry->type == nl_semantic_unit_type(p->context) &&
            !entry->result_count && !entry->results[0].value);
    printf("ROOT custody=1 machine=%d\n",
           nl_checked_proof_class(p->entry) == NL_CHECKED_MACHINE_PROVED);
    return walk(p->entry);
}

typedef enum {
    NO_FAULT,
    ENTRY_WORLD,
    HELPER_SEAL
} Fault;

static Fault fault;
static size_t fail_after, visits, hits, bytes;
static bool reached;

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
    if (fault != NO_FAULT && !hits && call &&
        ((fault == ENTRY_WORLD && world && !call->argument_count) ||
         (fault == HELPER_SEAL && !world && call->argument_count == 2))) {
        for (NLCheckedNodeId node = 1; node <= child->count; ++node) {
            const NLCheckedNodeView *stmt = nl_checked_node_view(child, node);
            const NLCheckedNodeView *init =
                nl_checked_node_view(child, stmt->initializer);
            if (stmt->kind == NL_CHECKED_STATEMENT && init &&
                init->type ==
                    nl_semantic_core_type(child->context, NL_TYPE_BOOL)) {
                reached = !nl_h_custody_result_valid(parent, id) &&
                          call->result_count == (world ? 0 : 1);
                /* The fresh entry BODY's node copy comes before its world. */
                fail_after = world ? 2 : 1;
                break;
            }
        }
    }
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, id, world);
    fail_after = 0;
    return status;
}

static bool same_emission(const Program *p)
{
    char *first = NULL, *second = NULL;
    size_t first_len = 0, second_len = 0;
    const bool ok =
        nl_quantity_usize_c17(p->entry, &first, &first_len) == NL_CHECK_OK &&
        nl_quantity_usize_c17(p->entry, &second, &second_len) == NL_CHECK_OK &&
        first && second && first_len && first_len == second_len &&
        !memcmp(first, second, first_len);
    free(second);
    free(first);
    return ok;
}

static bool rollback(Program *p, Fault at)
{
    NLSemanticContext *c = p->context;
    NLSymbolId symbol = 0;
    const NLScalarValue seed = {nl_semantic_core_type(c, NL_TYPE_USIZE), true,
                                241};
    REQUIRE(nl_semantic_seed_scalar(c, "p1632_anchor", seed, &symbol) ==
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
    printf("OOM check reached=%d visits=%zu hits=%zu bytes=%zu status=%d\n",
           reached, visits, hits, bytes, status);
    REQUIRE(reached && visits == (at == ENTRY_WORLD ? 2 : 1) && hits == 1 &&
            bytes && status == NL_CHECK_OUT_OF_MEMORY && !p->entry &&
            !memcmp(context, c, sizeof(context)) &&
            !memcmp(binding, c->bindings + symbol - 1, sizeof(binding)) &&
            !memcmp(place, c->places + anchor.place - 1, sizeof(place)) &&
            !memcmp(value, c->values + anchor.value - 1, sizeof(value)) &&
            nl_sem_validate(c) == NL_CHECK_OK);
    REQUIRE(check(p) == NL_CHECK_OK && inspect(p) && same_emission(p));
    puts("ROLLBACK unpublished CHECK and caller preserved; CHECK/C17 retry");
    return true;
}

static bool staging(Program *p)
{
    REQUIRE(check(p) == NL_CHECK_OK && inspect(p));
    char *first = NULL, *retry = NULL;
    size_t length = 0, retry_length = 0;
    REQUIRE(nl_quantity_usize_c17(p->entry, &first, &length) == NL_CHECK_OK);
    char sentinel[] = "keep\0bool";
    char original[sizeof(sentinel)];
    memcpy(original, sentinel, sizeof(original));
    char *out = sentinel;
    size_t out_length = sizeof(sentinel);
    fail_after = 1;
    const NLCheckStatus status =
        nl_quantity_usize_c17(p->entry, &out, &out_length);
    fail_after = 0;
    const bool preserved = out == sentinel && out_length == sizeof(sentinel) &&
                           !memcmp(sentinel, original, sizeof(sentinel));
    if (out != sentinel)
        free(out);
    const bool restored =
        nl_quantity_usize_c17(p->entry, &retry, &retry_length) == NL_CHECK_OK &&
        retry && retry_length == length && !memcmp(first, retry, length);
    free(retry);
    free(first);
    printf("OOM staging visits=%zu hits=%zu bytes=%zu no_clobber=%d "
           "byte_retry=%d\n",
           visits, hits, bytes, preserved, restored);
    return status == NL_CHECK_OUT_OF_MEMORY && visits == 1 && hits == 1 &&
           bytes > length && preserved && restored;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0};
    const char *mode = argv[1];
    bool ok = prepare(&p, argv[2]);
    if (ok && !strcmp(mode, "inspect"))
        ok = !p.registered || (check(&p) == NL_CHECK_OK && inspect(&p));
    else if (ok && p.registered && !strcmp(mode, "entry-world-oom"))
        ok = rollback(&p, ENTRY_WORLD);
    else if (ok && p.registered && !strcmp(mode, "helper-seal-oom"))
        ok = rollback(&p, HELPER_SEAL);
    else if (ok && p.registered && !strcmp(mode, "staging-oom"))
        ok = staging(&p);
    else
        ok = false;
    nl_checked_destroy(p.entry);
    nl_syntax_tree_destroy(p.expression);
    nl_parser_destroy(p.parser);
    nl_source_destroy(p.source);
    nl_semantic_destroy(p.context);
    return ok ? 0 : 1;
}
