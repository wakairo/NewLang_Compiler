#include "newlang/checked.h"
#include "newlang/copy_body.h"
#include "newlang/parser.h"
#include "newlang/source.h"
#include "newlang/usize_addition.h"
#include "newlang/usize_division.h"
#include "newlang/usize_subtraction.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void json_bytes(const unsigned char *bytes, size_t length)
{
    putchar('"');
    for (size_t i = 0; i < length; ++i) {
        const unsigned char c = bytes[i];
        if (c == '"' || c == '\\')
            printf("\\%c", c);
        else if (c < 32)
            printf("\\u%04x", (unsigned)c);
        else
            putchar(c);
    }
    putchar('"');
}

static void json_string(const char *text)
{
    if (text == NULL)
        text = "";
    json_bytes((const unsigned char *)text, strlen(text));
}

static void json_span(const NLSource *source, NLSourceSpan span)
{
    NLSourceView view = {0};
    if (source && nl_source_view(source, span, &view))
        json_bytes(view.bytes, view.length);
    else
        json_string("");
}

static bool retained_in_original(const NLSource *retained,
                                 const NLSource *original)
{
    NLSourceView a = {0}, b = {0};
    if (!retained || !original ||
        !nl_source_view(retained, (NLSourceSpan){0, nl_source_length(retained)},
                        &a) ||
        !nl_source_view(original, (NLSourceSpan){0, nl_source_length(original)},
                        &b) ||
        !a.length || a.length > b.length)
        return false;
    for (size_t i = 0; i <= b.length - a.length; ++i) {
        if (memcmp(b.bytes + i, a.bytes, a.length) == 0)
            return true;
    }
    return false;
}

static const char *kind_name(NLCheckedKind kind)
{
    switch (kind) {
    case NL_CHECKED_IDENTIFIER:
        return "IDENTIFIER";
    case NL_CHECKED_BINDING:
        return "BINDING";
    case NL_CHECKED_REGISTERED_CALL:
        return "CALL";
    case NL_CHECKED_BLOCK:
        return "BLOCK";
    case NL_CHECKED_STATEMENT:
        return "STATEMENT";
    case NL_CHECKED_RETURN:
        return "RETURN";
    case NL_CHECKED_UNIT:
        return "UNIT";
    case NL_CHECKED_USIZE_LITERAL:
        return "USIZE_LITERAL";
    case NL_CHECKED_U8_LITERAL:
        return "U8_LITERAL";
    case NL_CHECKED_USIZE_DIVISION:
        return "DIVISION";
    case NL_CHECKED_USIZE_ADDITION:
        return "ADDITION";
    case NL_CHECKED_USIZE_SUBTRACTION:
        return "SUBTRACTION";
    default:
        return "OTHER";
    }
}

static void result_json(const NLSemanticContext *context,
                        NLCheckedResult result)
{
    NLSemanticValueView value = {0};
    const bool visible =
        result.value && nl_semantic_value_view(context, result.value, &value);
    printf("{\"type\":%zu,\"value\":%zu,\"visible\":%s,\"known\":%s,"
           "\"number\":%zu,\"dependencies\":%d,\"carrier\":%d}",
           result.type, result.value, visible ? "true" : "false",
           visible && value.scalar_known ? "true" : "false",
           visible ? value.scalar_value : 0,
           visible ? (int)value.dependencies : -1,
           visible ? (int)value.carrier : -1);
}

static bool snapshots_equal(const NLSemanticSnapshot *a,
                            const NLSemanticSnapshot *b)
{
    return a->types == b->types && a->bindings == b->bindings &&
           a->values == b->values && a->places == b->places &&
           a->domains == b->domains && a->scopes == b->scopes &&
           a->functions == b->functions &&
           a->last_incarnation == b->last_incarnation &&
           a->last_value_fact == b->last_value_fact &&
           a->backing_regions == b->backing_regions &&
           a->raw_intervals == b->raw_intervals &&
           a->occurrences == b->occurrences;
}

static void diagnostic_json(const char *stage, int status,
                            const NLDiagnostic *diagnostic, NLSourceSpan span,
                            bool unchanged)
{
    printf("{\"record\":\"stage\",\"stage\":");
    json_string(stage);
    printf(",\"status\":%d,\"code\":", status);
    json_string(diagnostic ? diagnostic->code : NULL);
    printf(",\"span\":[%zu,%zu],\"snapshot_unchanged\":%s,"
           "\"entry_issued\":false}\n",
           span.start_byte, span.end_byte, unchanged ? "true" : "false");
}

static bool emit_graph(const NLCheckedFragment *fragment,
                       const NLSource *original, size_t parent,
                       NLCheckedNodeId parent_call,
                       const NLSemanticContext *parent_context, size_t depth,
                       size_t *serial)
{
    if (!fragment || depth > 4)
        return false;
    const size_t graph = ++*serial;
    const NLSemanticContext *context = nl_checked_context(fragment);
    const NLSource *source = nl_checked_source(fragment);
    printf("{\"record\":\"graph\",\"graph\":%zu,\"parent\":%zu,"
           "\"parent_call\":%zu,\"root\":%zu,\"count\":%zu,"
           "\"same_context\":%s,\"source_in_original\":%s,"
           "\"source_bytes\":%zu}\n",
           graph, parent, parent_call, nl_checked_root(fragment),
           nl_checked_node_count(fragment),
           !parent_context || context == parent_context ? "true" : "false",
           retained_in_original(source, original) ? "true" : "false",
           source ? nl_source_length(source) : 0);
    for (size_t id = 1; id <= nl_checked_node_count(fragment); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (!node)
            return false;
        printf("{\"record\":\"node\",\"graph\":%zu,\"id\":%zu,\"kind\":", graph,
               id);
        json_string(kind_name(node->kind));
        printf(",\"name\":");
        json_span(source, node->name);
        printf(",\"span\":[%zu,%zu],\"symbol\":%zu,\"function\":%zu,"
               "\"type\":%zu,\"use\":%d,\"body_backed\":%s,"
               "\"terminates\":%s,\"initializer\":%zu,"
               "\"first_argument\":%zu,\"next_argument\":%zu,"
               "\"argument_count\":%zu,\"first_item\":%zu,"
               "\"next_item\":%zu,\"tail\":%zu,\"item_count\":%zu,"
               "\"return_edges\":%zu,\"result_count\":%zu,\"results\":[",
               node->span.start_byte, node->span.end_byte, node->symbol,
               node->function, node->type, (int)node->value_use,
               node->body_backed ? "true" : "false",
               node->terminates ? "true" : "false", node->initializer,
               node->first_argument, node->next_argument, node->argument_count,
               node->first_item, node->next_item, node->tail, node->item_count,
               node->return_edges, node->result_count);
        for (size_t j = 0; j < node->result_count && j < 2; ++j) {
            if (j)
                putchar(',');
            result_json(context, node->results[j]);
        }
        printf("],\"returned\":");
        result_json(context, node->returned);
        NLSemanticBindingView binding = {0};
        const bool bound = node->symbol && nl_semantic_binding_view(
                                               context, node->symbol, &binding);
        printf(",\"binding_visible\":%s,\"availability\":%d,\"binding\":",
               bound ? "true" : "false",
               bound ? (int)binding.availability : -1);
        result_json(context, (NLCheckedResult){binding.type, binding.value});
        printf("}\n");
        if (node->kind == NL_CHECKED_USIZE_DIVISION) {
            NLUsizeDivisionView view = {0};
            const bool present =
                nl_checked_usize_division_view(fragment, id, &view);
            printf(
                "{\"record\":\"division\",\"graph\":%zu,\"node\":%zu,"
                "\"present\":%s,\"valid\":%s,\"copy_valid\":%s,"
                "\"origin_same\":%s,\"left_node\":%zu,"
                "\"right_node\":%zu,\"formal_symbol\":%zu,"
                "\"left_value\":%zu,\"right_value\":%zu,"
                "\"result\":%zu,\"numerator_known\":%s,"
                "\"quotient_known\":%s,\"numerator\":%zu,"
                "\"divisor\":%zu,\"quotient\":%zu,\"target_max\":%zu}\n",
                graph, id, present ? "true" : "false",
                nl_checked_usize_division_valid(fragment, id, NULL) ? "true"
                                                                    : "false",
                present && nl_checked_usize_division_valid(fragment, id, &view)
                    ? "true"
                    : "false",
                present && view.origin == fragment ? "true" : "false",
                view.operands[0], view.operands[1], view.symbols[0],
                view.values[0], view.values[1], view.result,
                view.numerator_known ? "true" : "false",
                view.quotient_known ? "true" : "false", view.numbers[0],
                view.numbers[1], view.quotient, view.target_max);
        }
        if (node->kind == NL_CHECKED_USIZE_ADDITION) {
            NLUsizeAdditionView view = {0};
            const bool present =
                nl_checked_usize_addition_view(fragment, id, &view);
            printf("{\"record\":\"addition\",\"graph\":%zu,\"node\":%zu,"
                   "\"present\":%s,\"valid\":%s,\"origin_same\":%s,"
                   "\"left\":%zu,\"right\":%zu,\"number\":%zu,"
                   "\"result\":%zu}\n",
                   graph, id, present ? "true" : "false",
                   nl_checked_usize_addition_valid(fragment, id, NULL)
                       ? "true"
                       : "false",
                   present && view.origin == fragment ? "true" : "false",
                   view.numbers[0], view.numbers[1], view.sum, view.result);
        }
        if (node->kind == NL_CHECKED_USIZE_SUBTRACTION) {
            NLUsizeSubtractionView view = {0};
            const bool present =
                nl_checked_usize_subtraction_view(fragment, id, &view);
            printf("{\"record\":\"subtraction\",\"graph\":%zu,\"node\":%zu,"
                   "\"present\":%s,\"valid\":%s,\"origin_same\":%s,"
                   "\"left\":%zu,\"right\":%zu,\"number\":%zu,"
                   "\"result\":%zu}\n",
                   graph, id, present ? "true" : "false",
                   nl_checked_usize_subtraction_valid(fragment, id, NULL)
                       ? "true"
                       : "false",
                   present && view.origin == fragment ? "true" : "false",
                   view.numbers[0], view.numbers[1], view.difference,
                   view.result);
        }
        if (node->kind == NL_CHECKED_REGISTERED_CALL) {
            NLCopyBodyParameters parameters = {0};
            const bool present =
                nl_checked_copy_body_parameters(fragment, id, &parameters);
            const int validated =
                present ? (int)nl_checked_copy_body_parameters_validate(
                              fragment, id, &parameters)
                        : -1;
            printf("{\"record\":\"parameters\",\"graph\":%zu,\"node\":%zu,"
                   "\"present\":%s,\"validation\":%d,"
                   "\"parameter0\":%zu,\"argument0\":%zu,"
                   "\"entry_is_context\":%s,\"handoff_is_context\":%s}\n",
                   graph, id, present ? "true" : "false", validated,
                   parameters.parameters[0], parameters.arguments[0],
                   parameters.entry == context ? "true" : "false",
                   parameters.handoff == context ? "true" : "false");
        }
    }
    for (size_t id = 1; id <= nl_checked_node_count(fragment); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (node->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
            if (body && !emit_graph(body, original, graph, id, context,
                                    depth + 1, serial))
                return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit = NULL, *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *entry = NULL;
    int result = 2;
    if (argc != 2)
        return 2;
    printf("{\"record\":\"target\",\"size_t_bits\":%zu,\"size_max\":%zu}\n",
           sizeof(size_t) * CHAR_BIT, SIZE_MAX);
    if (nl_source_load(argv[1], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK)
        goto cleanup;
    NLParseDiagnostic parse_diagnostic = {0};
    const NLParseStatus parsed =
        nl_parser_parse_function_unit(parser, &unit, &parse_diagnostic);
    diagnostic_json("parse", (int)parsed, &parse_diagnostic.diagnostic,
                    parse_diagnostic.span, true);
    if (parsed != NL_PARSE_OK) {
        result = 0;
        goto cleanup;
    }
    if (nl_semantic_create(&context) != NL_CHECK_OK)
        goto cleanup;
    NLSemanticSnapshot before = {0}, after = {0};
    if (!nl_semantic_snapshot(context, &before))
        goto cleanup;
    const NLSyntaxTree *inputs[] = {unit};
    NLFunctionUnitDiagnostic diagnostic = {0};
    const NLCheckStatus registered =
        nl_semantic_register_function_unit(context, inputs, 1, &diagnostic);
    if (!nl_semantic_snapshot(context, &after))
        goto cleanup;
    diagnostic_json(
        "definition", (int)registered, &diagnostic.diagnostic.diagnostic,
        diagnostic.diagnostic.span, snapshots_equal(&before, &after));
    if (registered != NL_CHECK_OK) {
        result = 0;
        goto cleanup;
    }
    const NLTypeId usize_type = nl_semantic_core_type(context, NL_TYPE_USIZE);
    printf("{\"record\":\"types\",\"usize\":%zu,\"unit\":%zu}\n", usize_type,
           nl_semantic_unit_type(context));
    if (nl_source_create("main()", 6, "<r1551-ordinary-entry>",
                         &entry_source) != NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK)
        goto cleanup;
    parse_diagnostic = (NLParseDiagnostic){0};
    if (nl_parser_parse_expression_fragment(entry_parser, &entry_tree,
                                            &parse_diagnostic) != NL_PARSE_OK)
        goto cleanup;
    NLCheckDiagnostic checked_diagnostic = {0};
    const NLCheckStatus checked = nl_semantic_check_expression(
        context, entry_tree, &entry, &checked_diagnostic);
    diagnostic_json("entry", (int)checked, &checked_diagnostic.diagnostic,
                    checked_diagnostic.span, false);
    if (checked != NL_CHECK_OK) {
        result = 0;
        goto cleanup;
    }
    printf("{\"record\":\"owned\",\"proof_class\":%d}\n",
           (int)nl_checked_proof_class(entry));
    size_t serial = 0;
    if (!emit_graph(entry, source, 0, 0, NULL, 0, &serial))
        goto cleanup;
    printf("{\"record\":\"complete\",\"graphs\":%zu,\"entry_issued\":true}\n",
           serial);
    result = ferror(stdout) ? 2 : 0;
cleanup:
    nl_checked_destroy(entry);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    if (fflush(stdout) == EOF || ferror(stdout))
        result = 2;
    return result;
}
