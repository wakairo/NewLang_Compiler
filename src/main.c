#include "newlang/checked.h"
#include "newlang/diagnostic.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char help[] =
    "Usage: newlangc [--version | --help | SOURCE]\n"
    "NewLang production compiler P0 bootstrap.\n"
    "  --version  Print the deterministic compiler version.\n"
    "  --help     Print this help.\n"
    "  SOURCE     Validate North Star V0/V1/AVS and emit Checked-C to stdout.\n"
    "Checked-C is a bounded bootstrap/reference execution path; LLVM remains "
    "the planned primary backend.\n";

static int report_error(const char *code, const char *message,
                        const char *argument, int status)
{
    const NLDiagnosticNote note = {argument, NULL};
    const NLDiagnostic diagnostic = {NL_DIAG_ERROR, "cli", code, message,
                                     NULL,          &note, 1};
    return nl_diagnostic_render(stderr, &diagnostic) ? status : 1;
}

typedef struct {
    size_t function;
    const NLCheckedFragment *body;
} V0Function;

typedef struct {
    V0Function functions[NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS];
    size_t count;
    size_t entry_function;
    NLTypeId unit, u8, aggregate;
} V0Program;

static V0Function *v0_find_function(V0Program *program, size_t function)
{
    for (size_t i = 0; i < program->count; ++i)
        if (program->functions[i].function == function)
            return &program->functions[i];
    return NULL;
}

/* V1 consumes only immutable checked evidence. Neither syntax trees nor
 * source bytes enter scalar lowering. Symbol IDs produce safe C local names. */
static bool v1_scalar(const NLCheckedFragment *fragment, NLCheckedNodeId id,
                      const V0Program *program)
{
    const NLCheckedNodeView *v = nl_checked_node_view(fragment, id);
    if (v == NULL || v->type != program->u8 || v->result_count != 1 ||
        v->results[0].type != program->u8 || v->terminates)
        return false;
    if (v->kind == NL_CHECKED_U8_LITERAL)
        return v->has_scalar_result && v->scalar_result.known &&
               v->scalar_result.type == program->u8 &&
               v->scalar_result.value <= 255;
    if (v->kind != NL_CHECKED_IDENTIFIER || v->symbol == 0 ||
        v->value_use != NL_VALUE_COPIED)
        return false;
    /* Exclude host-known parameters/seeds: the C local must have a source
     * binding in this owned body. Lexical visibility is checker-established. */
    for (size_t i = 1; i <= nl_checked_node_count(fragment); ++i) {
        const NLCheckedNodeView *binding = nl_checked_node_view(fragment, i);
        if (binding != NULL && binding->kind == NL_CHECKED_RECEIVER &&
            binding->type == program->u8 && binding->symbol == v->symbol)
            return true;
    }
    return false;
}

/* Only one flat two-u8 nominal representation. The associated checked
 * context supplies semantic shape facts; no names or layout are consulted. */
static bool avs_shape(const NLCheckedFragment *fragment, NLTypeId type,
                      V0Program *program)
{
    const NLSemanticContext *context = nl_checked_context(fragment);
    NLSemanticTypeView t;
    if (!nl_semantic_type_view(context, type, &t) ||
        t.kind != NL_TYPE_NOMINAL || t.field_count != 2 || !t.is_copy ||
        !t.is_discardable ||
        (program->aggregate != 0 && program->aggregate != type))
        return false;
    for (size_t i = 0; i < 2; ++i) {
        NLAggregateField field;
        if (!nl_semantic_aggregate_field_view(context, type, i, &field) ||
            field.type != program->u8)
            return false;
    }
    program->aggregate = type;
    return true;
}

static bool avs_construction(const NLCheckedFragment *fragment,
                             NLCheckedNodeId id, V0Program *program)
{
    const NLCheckedNodeView *aggregate = nl_checked_node_view(fragment, id);
    if (aggregate == NULL || aggregate->kind != NL_CHECKED_AGGREGATE ||
        aggregate->result_count != 1 || aggregate->argument_count != 2 ||
        !avs_shape(fragment, aggregate->type, program))
        return false;
    bool seen[2] = {false, false};
    NLCheckedNodeId field = aggregate->first_argument;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(fragment, field);
        if (v == NULL || v->kind != NL_CHECKED_AGGREGATE_FIELD ||
            v->field_index >= 2 || seen[v->field_index] ||
            v->type != program->u8 ||
            !v1_scalar(fragment, v->initializer, program))
            return false;
        seen[v->field_index] = true;
        field = v->next_argument;
    }
    return field == 0;
}

static bool avs_destructuring(const NLCheckedFragment *fragment,
                              const NLCheckedNodeView *binding,
                              V0Program *program)
{
    const NLCheckedNodeView *rhs =
        nl_checked_node_view(fragment, binding->initializer);
    if (binding->argument_count != 2 || rhs == NULL ||
        rhs->kind != NL_CHECKED_IDENTIFIER ||
        rhs->value_use != NL_VALUE_COPIED || rhs->result_count != 1 ||
        rhs->symbol == 0 || !avs_shape(fragment, rhs->type, program))
        return false;
    bool local = false;
    for (size_t i = 1; i <= nl_checked_node_count(fragment); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(fragment, i);
        const NLCheckedNodeView *r =
            v == NULL ? NULL
                      : nl_checked_node_view(fragment, v->first_argument);
        if (v != NULL && v->kind == NL_CHECKED_BINDING &&
            v->symbol == rhs->symbol && r != NULL && r->type == rhs->type)
            local = true;
    }
    if (!local)
        return false;
    bool seen[2] = {false, false};
    NLCheckedNodeId receiver = binding->first_argument;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(fragment, receiver);
        if (v == NULL || v->kind != NL_CHECKED_RECEIVER || v->symbol == 0 ||
            v->type != program->u8 || v->field_index >= 2 ||
            seen[v->field_index])
            return false;
        seen[v->field_index] = true;
        receiver = v->next_argument;
    }
    return receiver == 0;
}

static bool v1_emit_scalar(FILE *stream, const NLCheckedNodeView *value)
{
    if (value->kind == NL_CHECKED_U8_LITERAL)
        return fprintf(stream, "%zu", value->scalar_result.value) >= 0;
    if (value->kind == NL_CHECKED_IDENTIFIER)
        return fprintf(stream, "nl_local_%zu", value->symbol) >= 0;
    return false;
}

static bool v0_validate_node(const NLCheckedFragment *, NLCheckedNodeId,
                             V0Program *);

static bool v0_validate_call(const NLCheckedFragment *fragment,
                             NLCheckedNodeId id, V0Program *program)
{
    const NLCheckedNodeView *call = nl_checked_node_view(fragment, id);
    if (call == NULL || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || call->argument_count != 0 ||
        call->type != program->unit || call->result_count != 0 ||
        call->terminates)
        return false;

    const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
    if (body == NULL)
        return false;
    if (v0_find_function(program, call->function) != NULL)
        return true;
    if (program->count == NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS)
        return false;

    program->functions[program->count++] = (V0Function){call->function, body};
    return v0_validate_node(body, nl_checked_root(body), program);
}

static bool v0_validate_block(const NLCheckedFragment *fragment,
                              const NLCheckedNodeView *block,
                              V0Program *program)
{
    size_t count = 0;
    NLCheckedNodeId item = block->first_item;
    while (item != 0) {
        const NLCheckedNodeView *view = nl_checked_node_view(fragment, item);
        if (view == NULL || !v0_validate_node(fragment, item, program))
            return false;
        item = view->next_item;
        ++count;
    }
    if (count != block->item_count)
        return false;
    return block->tail == 0 || v0_validate_node(fragment, block->tail, program);
}

static bool v0_validate_node(const NLCheckedFragment *fragment,
                             NLCheckedNodeId id, V0Program *program)
{
    const NLCheckedNodeView *view = nl_checked_node_view(fragment, id);
    if (view == NULL)
        return false;

    switch (view->kind) {
    case NL_CHECKED_U8_LITERAL:
    case NL_CHECKED_IDENTIFIER:
        return v1_scalar(fragment, id, program);
    case NL_CHECKED_AGGREGATE_BINDING:
        return avs_destructuring(fragment, view, program);
    case NL_CHECKED_BINDING: {
        const NLCheckedNodeView *receiver =
            nl_checked_node_view(fragment, view->first_argument);
        return view->type == program->unit && view->argument_count == 1 &&
               view->symbol != 0 && receiver != NULL &&
               receiver->kind == NL_CHECKED_RECEIVER &&
               receiver->symbol == view->symbol &&
               ((receiver->type == program->u8 &&
                 v1_scalar(fragment, view->initializer, program)) ||
                (avs_shape(fragment, receiver->type, program) &&
                 avs_construction(fragment, view->initializer, program)));
    }
    case NL_CHECKED_BLOCK:
        return v0_validate_block(fragment, view, program);
    case NL_CHECKED_STATEMENT:
        return view->initializer != 0 &&
               v0_validate_node(fragment, view->initializer, program);
    case NL_CHECKED_REGISTERED_CALL:
        return v0_validate_call(fragment, id, program);
    case NL_CHECKED_RETURN:
        return view->terminates && view->initializer != 0 &&
               view->returned.type == program->unit &&
               v0_validate_node(fragment, view->initializer, program);
    case NL_CHECKED_UNIT:
        return view->type == program->unit && view->result_count == 0;
    default:
        return false;
    }
}

static bool v0_indent(FILE *stream, size_t depth)
{
    for (size_t i = 0; i < depth; ++i)
        if (fputs("    ", stream) < 0)
            return false;
    return true;
}

static bool v0_emit_node(FILE *, const NLCheckedFragment *, NLCheckedNodeId,
                         size_t);

static bool v0_emit_block(FILE *stream, const NLCheckedFragment *fragment,
                          const NLCheckedNodeView *block, size_t depth)
{
    if (!v0_indent(stream, depth) || fputs("{\n", stream) < 0)
        return false;

    NLCheckedNodeId item = block->first_item;
    while (item != 0) {
        const NLCheckedNodeView *view = nl_checked_node_view(fragment, item);
        if (view == NULL || !v0_emit_node(stream, fragment, item, depth + 1))
            return false;
        item = view->next_item;
    }
    if (block->tail != 0 &&
        !v0_emit_node(stream, fragment, block->tail, depth + 1))
        return false;
    return v0_indent(stream, depth) && fputs("}\n", stream) >= 0;
}

static bool avs_emit_initializer(FILE *stream,
                                 const NLCheckedFragment *fragment,
                                 const NLCheckedNodeView *aggregate)
{
    if (fputs("{ ", stream) < 0)
        return false;
    for (NLCheckedNodeId id = aggregate->first_argument; id != 0;) {
        const NLCheckedNodeView *field = nl_checked_node_view(fragment, id);
        const NLCheckedNodeView *value =
            nl_checked_node_view(fragment, field->initializer);
        if (fprintf(stream, ".f%zu = ", field->field_index) < 0 ||
            !v1_emit_scalar(stream, value) || fputs(", ", stream) < 0)
            return false;
        id = field->next_argument;
    }
    return fputs("}", stream) >= 0;
}

static bool v0_emit_node(FILE *stream, const NLCheckedFragment *fragment,
                         NLCheckedNodeId id, size_t depth)
{
    const NLCheckedNodeView *view = nl_checked_node_view(fragment, id);
    if (view == NULL)
        return false;

    switch (view->kind) {
    case NL_CHECKED_U8_LITERAL:
    case NL_CHECKED_IDENTIFIER:
        return v0_indent(stream, depth) && fputs("(void)", stream) >= 0 &&
               v1_emit_scalar(stream, view) && fputs(";\n", stream) >= 0;
    case NL_CHECKED_AGGREGATE_BINDING: {
        const NLCheckedNodeView *rhs =
            nl_checked_node_view(fragment, view->initializer);
        if (!v0_indent(stream, depth) ||
            fprintf(stream,
                    "const nl_type_%zu nl_destructure_%zu = nl_local_%zu;\n",
                    rhs->type, id, rhs->symbol) < 0)
            return false;
        for (NLCheckedNodeId receiver = view->first_argument; receiver != 0;) {
            const NLCheckedNodeView *v =
                nl_checked_node_view(fragment, receiver);
            if (!v0_indent(stream, depth) ||
                fprintf(
                    stream,
                    "const uint8_t nl_local_%zu = nl_destructure_%zu.f%zu;\n",
                    v->symbol, id, v->field_index) < 0 ||
                !v0_indent(stream, depth) ||
                fprintf(stream, "(void)nl_local_%zu;\n", v->symbol) < 0)
                return false;
            receiver = v->next_argument;
        }
        return true;
    }
    case NL_CHECKED_BINDING: {
        const NLCheckedNodeView *initializer =
            nl_checked_node_view(fragment, view->initializer);
        if (initializer == NULL || !v0_indent(stream, depth))
            return false;
        if (initializer->kind == NL_CHECKED_AGGREGATE) {
            if (fprintf(stream, "const nl_type_%zu nl_local_%zu = ",
                        initializer->type, view->symbol) < 0 ||
                !avs_emit_initializer(stream, fragment, initializer))
                return false;
        } else if (fprintf(stream,
                           "const uint8_t nl_local_%zu = ", view->symbol) < 0 ||
                   !v1_emit_scalar(stream, initializer))
            return false;
        if (fputs(";\n", stream) < 0)
            return false;
        /* Suppress C unused-local warnings even when NewLang never reads this
         * Discardable binding. This emits no language-level operation. */
        return v0_indent(stream, depth) &&
               fprintf(stream, "(void)nl_local_%zu;\n", view->symbol) >= 0;
    }
    case NL_CHECKED_BLOCK:
        return v0_emit_block(stream, fragment, view, depth);
    case NL_CHECKED_STATEMENT:
        return v0_emit_node(stream, fragment, view->initializer, depth);
    case NL_CHECKED_REGISTERED_CALL:
        return v0_indent(stream, depth) &&
               fprintf(stream, "nl_fn_%zu();\n", view->function) >= 0;
    case NL_CHECKED_RETURN:
        if (!v0_emit_node(stream, fragment, view->initializer, depth))
            return false;
        return v0_indent(stream, depth) && fputs("return;\n", stream) >= 0;
    case NL_CHECKED_UNIT:
        return v0_indent(stream, depth) && fputs("(void)0;\n", stream) >= 0;
    default:
        return false;
    }
}

static bool v0_emit_c(FILE *stream, const V0Program *program)
{
    if (fputs(
            "/* North Star V0/V1/AVS Checked-C reference output. */\n#include "
            "<stdint.h>\n\n",
            stream) < 0)
        return false;

    if (program->aggregate != 0 &&
        fprintf(stream,
                "typedef struct { uint8_t f0; uint8_t f1; } nl_type_%zu;\n\n",
                program->aggregate) < 0)
        return false;

    for (size_t i = 0; i < program->count; ++i)
        if (fprintf(stream, "static void nl_fn_%zu(void);\n",
                    program->functions[i].function) < 0)
            return false;
    if (fputc('\n', stream) == EOF)
        return false;

    for (size_t i = 0; i < program->count; ++i) {
        const V0Function function = program->functions[i];
        const NLCheckedNodeView *root =
            nl_checked_node_view(function.body, nl_checked_root(function.body));
        if (root == NULL ||
            fprintf(stream, "static void nl_fn_%zu(void)\n",
                    function.function) < 0 ||
            !v0_emit_block(stream, function.body, root, 0) ||
            fputc('\n', stream) == EOF)
            return false;
    }

    return fputs("int main(void)\n{\n", stream) >= 0 &&
           fprintf(stream, "    nl_fn_%zu();\n", program->entry_function) >=
               0 &&
           fputs("    return 0;\n}\n", stream) >= 0;
}

static int render_parse_failure(const NLSource *source,
                                const NLParseDiagnostic *diagnostic)
{
    if (diagnostic->diagnostic.code != NULL &&
        nl_parse_diagnostic_render(stderr, source, diagnostic))
        return 3;
    return report_error("V0-PARSE", "source parsing failed",
                        nl_source_name(source), 3);
}

static int render_check_failure(const NLSource *source,
                                const NLCheckDiagnostic *diagnostic)
{
    if (diagnostic->diagnostic.code != NULL &&
        nl_check_diagnostic_render(stderr, source, diagnostic))
        return 3;
    return report_error("V0-CHECK", "semantic validation failed",
                        nl_source_name(source), 3);
}

static int compile_v0(const char *path)
{
    static const char entry_text[] = "main()";
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit = NULL, *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *entry = NULL;
    int result = 1;

    if (nl_source_load(path, &source) != NL_SOURCE_OK) {
        result = report_error("V0-SOURCE-IO", "cannot read source", path, 1);
        goto cleanup;
    }

    NLParseDiagnostic parse_diagnostic = {0};
    NLParseStatus parse = nl_parser_create(source, &parser);
    if (parse != NL_PARSE_OK) {
        result = report_error("V0-PARSER", "cannot create parser", path, 1);
        goto cleanup;
    }
    parse = nl_parser_parse_function_unit(parser, &unit, &parse_diagnostic);
    if (parse != NL_PARSE_OK) {
        result = render_parse_failure(source, &parse_diagnostic);
        goto cleanup;
    }

    if (nl_semantic_create(&context) != NL_CHECK_OK) {
        result = report_error("V0-CHECKER", "cannot create semantic context",
                              path, 1);
        goto cleanup;
    }

    const NLSyntaxTree *inputs[] = {unit};
    NLFunctionUnitDiagnostic unit_diagnostic = {0};
    const NLCheckStatus registered = nl_semantic_register_function_unit(
        context, inputs, 1, &unit_diagnostic);
    if (registered != NL_CHECK_OK) {
        result = render_check_failure(source, &unit_diagnostic.diagnostic);
        goto cleanup;
    }

    if (nl_source_create(entry_text, sizeof(entry_text) - 1, "<v0-entry>",
                         &entry_source) != NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK) {
        result = report_error("V0-ENTRY", "cannot create checked entry call",
                              path, 1);
        goto cleanup;
    }

    parse_diagnostic = (NLParseDiagnostic){0};
    parse = nl_parser_parse_expression_fragment(entry_parser, &entry_tree,
                                                &parse_diagnostic);
    if (parse != NL_PARSE_OK) {
        result = render_parse_failure(entry_source, &parse_diagnostic);
        goto cleanup;
    }

    NLCheckDiagnostic check_diagnostic = {0};
    const NLCheckStatus checked = nl_semantic_check_expression(
        context, entry_tree, &entry, &check_diagnostic);
    if (checked != NL_CHECK_OK) {
        result = render_check_failure(entry_source, &check_diagnostic);
        goto cleanup;
    }

    const NLCheckedNodeId root = nl_checked_root(entry);
    const NLCheckedNodeView *root_view = nl_checked_node_view(entry, root);
    V0Program program = {.unit = nl_semantic_unit_type(context),
                         .u8 = nl_semantic_core_type(context, NL_TYPE_U8)};
    if (root_view == NULL || !v0_validate_call(entry, root, &program)) {
        result = report_error("V1-BACKEND-UNSUPPORTED",
                              "accepted program uses a construct outside the "
                              "V0/V1 Checked-C spine",
                              path, 4);
        goto cleanup;
    }
    program.entry_function = root_view->function;

    if (!v0_emit_c(stdout, &program) || fflush(stdout) != 0) {
        result =
            report_error("V0-C-OUTPUT", "failed to emit Checked-C", path, 1);
        goto cleanup;
    }
    result = 0;

cleanup:
    nl_checked_destroy(entry);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return result;
}

int main(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
        return fputs(help, stdout) >= 0 && fflush(stdout) == 0 ? 0 : 1;
    }
    if (argc != 2) {
        return report_error("P0-CLI-ARGUMENT", "expected exactly one argument",
                            "use --help for usage", 2);
    }
    if (strcmp(argv[1], "--version") == 0) {
        return fputs("newlangc 0.1.0 (P0 bootstrap)\n", stdout) >= 0 &&
                       fflush(stdout) == 0
                   ? 0
                   : 1;
    }
    if (argv[1][0] == '-') {
        return report_error("P0-CLI-OPTION", "unknown option", argv[1], 2);
    }
    return compile_v0(argv[1]);
}
