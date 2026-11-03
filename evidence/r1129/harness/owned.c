#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/syntax.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define R_LIMIT 512

static const NLCheckedFragment *fragments[R_LIMIT];
static const NLSemanticContext *worlds[R_LIMIT];
static size_t fragment_count;
static size_t world_count;

static void quote(const char *s)
{
    putchar('"');
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        if (*p == '"' || *p == '\\') {
            putchar('\\');
            putchar(*p);
        } else if (*p < 32) {
            printf("\\u%04x", (unsigned)*p);
        } else {
            putchar(*p);
        }
    }
    putchar('"');
}

static void begin(const char *event)
{
    fputs("{\"event\":", stdout);
    quote(event);
}

static void number(const char *name, size_t value)
{
    printf(",\"%s\":%zu", name, value);
}

static void flag(const char *name, bool value)
{
    printf(",\"%s\":%s", name, value ? "true" : "false");
}

static void string(const char *name, const char *value)
{
    printf(",\"%s\":", name);
    quote(value ? value : "");
}

static void end(void)
{
    puts("}");
}

static size_t world_id(const NLSemanticContext *context)
{
    for (size_t i = 0; i < world_count; ++i) {
        if (worlds[i] == context)
            return i + 1;
    }
    if (!context || world_count == R_LIMIT) {
        fputs("R1129 observer world limit\n", stderr);
        exit(70);
    }
    worlds[world_count++] = context;
    return world_count;
}

static const char *kind_name(NLCheckedKind kind)
{
    switch (kind) {
    case NL_CHECKED_LOOP:
        return "LOOP";
    case NL_CHECKED_CONTINUE:
        return "CONTINUE";
    case NL_CHECKED_BREAK:
        return "BREAK";
    case NL_CHECKED_IF:
        return "IF";
    case NL_CHECKED_REGISTERED_CALL:
        return "CALL";
    case NL_CHECKED_REPLACE:
        return "REPLACE";
    case NL_CHECKED_BINDING:
        return "BINDING";
    case NL_CHECKED_IDENTIFIER:
        return "IDENTIFIER";
    case NL_CHECKED_FIELD_READ:
        return "FIELD_READ";
    case NL_CHECKED_AGGREGATE:
        return "AGGREGATE";
    case NL_CHECKED_LOAN_HEADER:
        return "LOAN";
    case NL_CHECKED_USIZE_LITERAL:
        return "USIZE_LITERAL";
    case NL_CHECKED_USIZE_EQUALITY:
        return "USIZE_EQUALITY";
    case NL_CHECKED_U8_LITERAL:
        return "U8_LITERAL";
    case NL_CHECKED_RETURN:
        return "RETURN";
    default:
        return "OTHER";
    }
}

static void type_view(const NLSemanticContext *context, NLTypeId type)
{
    NLSemanticTypeView view;
    if (!type || !nl_semantic_type_view(context, type, &view))
        return;
    begin("type");
    number("world", world_id(context));
    number("type", type);
    number("kind", view.kind);
    flag("bool", view.kind == NL_TYPE_BOOL);
    flag("usize", view.kind == NL_TYPE_USIZE);
    flag("u8", view.kind == NL_TYPE_U8);
    flag("nominal", view.kind == NL_TYPE_NOMINAL);
    flag("copy", view.is_copy);
    flag("discardable", view.is_discardable);
    number("field_count", view.field_count);
    end();
    if (view.kind == NL_TYPE_NOMINAL) {
        for (size_t i = 0; i < view.field_count; ++i) {
            NLAggregateField field;
            if (!nl_semantic_aggregate_field_view(context, type, i, &field))
                continue;
            begin("declared_field");
            number("world", world_id(context));
            number("type", type);
            number("index", i);
            number("field_type", field.type);
            string("name", field.name);
            end();
        }
    }
}

static void value_view(const NLSemanticContext *context, NLValueId value)
{
    NLSemanticValueView view;
    if (!value || !nl_semantic_value_view(context, value, &view))
        return;
    begin("value");
    number("world", world_id(context));
    number("value", value);
    number("type", view.type);
    number("carrier", view.carrier);
    number("owner_place", view.owner_place);
    number("dependencies", view.dependencies);
    flag("known", view.scalar_known);
    number("scalar", view.scalar_value);
    number("field_count", view.field_count);
    number("reference_count", view.reference_count);
    flag("singular_reference_observed", view.reference_count == 0);
    if (view.reference_count == 0) {
        number("reference_place", view.reference.place);
        number("reference_incarnation", view.reference.incarnation);
        number("reference_scope", view.reference.scope);
    }
    end();
    type_view(context, view.type);
}

static void place_view(const NLSemanticContext *context, NLPlaceId place)
{
    NLSemanticPlaceView view;
    if (!place || !nl_semantic_place_view(context, place, &view))
        return;
    begin("place");
    number("world", world_id(context));
    number("place", place);
    number("type", view.type);
    flag("live", view.live);
    flag("independent_root", view.independent_root);
    flag("implicit_local", view.implicit_local);
    number("incarnation", view.incarnation);
    number("current_fact", view.current_fact);
    number("current_value", view.current_value);
    number("parent", view.parent_aggregate);
    number("parent_incarnation", view.parent_incarnation);
    number("field_index", view.parent_field_index);
    number("field_count", view.fixed_field_count);
    number("backing_region", view.placement.region);
    number("backing_start", view.placement.start);
    number("backing_length", view.placement.length);
    end();
    value_view(context, view.current_value);
}

static void binding_view(const NLSemanticContext *context, NLSymbolId symbol)
{
    NLSemanticBindingView view;
    if (!symbol || !nl_semantic_binding_view(context, symbol, &view))
        return;
    begin("binding");
    number("world", world_id(context));
    number("symbol", symbol);
    number("type", view.type);
    number("availability", view.availability);
    number("value", view.value);
    number("place", view.place);
    end();
    value_view(context, view.value);
    place_view(context, view.place);
}

static void name_view(const NLSource *source, NLSourceSpan span)
{
    NLSourceView view;
    fputs(",\"name\":\"", stdout);
    if (nl_source_view(source, span, &view)) {
        for (size_t i = 0; i < view.length; ++i) {
            unsigned char c = view.bytes[i];
            if (c == '"' || c == '\\') {
                putchar('\\');
                putchar(c);
            } else if (c < 32) {
                printf("\\u%04x", (unsigned)c);
            } else {
                putchar(c);
            }
        }
    }
    putchar('"');
}

static void walk(const NLCheckedFragment *fragment, size_t parent,
                 size_t owner_node, const char *edge)
{
    if (!fragment)
        return;
    const NLSemanticContext *context = nl_checked_context(fragment);
    const NLSource *source = nl_checked_source(fragment);
    size_t id = 0;
    for (size_t i = 0; i < fragment_count; ++i) {
        if (fragments[i] == fragment)
            id = i + 1;
    }
    bool visited = id != 0;
    if (!visited) {
        if (fragment_count == R_LIMIT) {
            fputs("R1129 observer fragment limit\n", stderr);
            exit(70);
        }
        fragments[fragment_count++] = fragment;
        id = fragment_count;
    }
    begin("fragment");
    number("fragment", id);
    number("world", world_id(context));
    number("parent_fragment", parent);
    number("owner_node", owner_node);
    string("edge", edge);
    string("source", nl_source_name(source));
    number("source_length", nl_source_length(source));
    number("root", nl_checked_root(fragment));
    number("nodes", nl_checked_node_count(fragment));
    flag("already_seen", visited);
    end();
    if (visited)
        return;
    for (size_t i = 1; i <= nl_checked_node_count(fragment); ++i) {
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, i);
        if (!node)
            continue;
        begin("node");
        number("fragment", id);
        number("world", world_id(context));
        number("node", i);
        number("kind", node->kind);
        string("kind_name", kind_name(node->kind));
        number("start", node->span.start_byte);
        number("end", node->span.end_byte);
        name_view(source, node->name);
        number("type", node->type);
        number("symbol", node->symbol);
        number("function", node->function);
        flag("body_backed", node->body_backed);
        flag("terminates", node->terminates);
        flag("header_inductive", node->header_inductive);
        number("continue_edges", node->continue_edges);
        number("break_edges", node->break_edges);
        number("return_edges", node->return_edges);
        number("value_use", node->value_use);
        number("first_argument", node->first_argument);
        number("next_argument", node->next_argument);
        number("argument_count", node->argument_count);
        number("initializer", node->initializer);
        number("first_item", node->first_item);
        number("next_item", node->next_item);
        number("tail", node->tail);
        number("result_count", node->result_count);
        flag("scalar_present", node->has_scalar_result);
        flag("scalar_known", node->scalar_result.known);
        number("scalar", node->scalar_result.value);
        flag("field_present", node->field.present);
        flag("field_compatible", node->field.dependency_compatible);
        number("field_base", node->field.base);
        number("field_nominal", node->field.nominal);
        number("field_index", node->field.index);
        number("field_parent", node->field.parent);
        number("field_child", node->field.child);
        number("parent_incarnation", node->field.parent_incarnation);
        number("child_incarnation", node->field.child_incarnation);
        number("parent_fact", node->field.parent_fact);
        number("child_fact", node->field.child_fact);
        number("parent_post_fact", node->field.parent_post_fact);
        number("child_post_fact", node->field.child_post_fact);
        number("old_value", node->field.old_value);
        number("new_value", node->field.new_value);
        number("loan_ref_symbol", node->loan.ref_symbol);
        number("loan_place", node->loan.place);
        number("loan_incarnation", node->loan.incarnation);
        number("loan_scope", node->loan.scope);
        flag("loan_nonescape", node->loan.body_nonescape_proved);
        flag("loan_forwarded", node->loan.normal_result_forwarded);
        if (node->kind == NL_CHECKED_LOOP) {
            flag("direct_break_validator_observation",
                 nl_checked_loop_break_provenance_valid(fragment, i));
        }
        end();
        type_view(context, node->type);
        binding_view(context, node->symbol);
        binding_view(context, node->field.base);
        place_view(context, node->field.parent);
        place_view(context, node->field.child);
        place_view(context, node->loan.place);
        value_view(context, node->field.old_value);
        value_view(context, node->field.new_value);
        for (size_t j = 0; j < node->result_count && j < 2; ++j) {
            begin("result");
            number("world", world_id(context));
            number("fragment", id);
            number("node", i);
            number("index", j);
            number("type", node->results[j].type);
            number("value", node->results[j].value);
            end();
            value_view(context, node->results[j].value);
        }
        if (node->kind == NL_CHECKED_REGISTERED_CALL)
            walk(nl_checked_call_body(fragment, i), id, i, "call");
        if (node->kind == NL_CHECKED_LOOP)
            walk(nl_checked_loop_body(fragment, i), id, i, "loop");
        if (node->kind == NL_CHECKED_IF) {
            walk(nl_checked_if_arm(fragment, i, 0), id, i, "if0");
            walk(nl_checked_if_arm(fragment, i, 1), id, i, "if1");
        }
    }
}

static void diagnostic(const char *stage, size_t status,
                       const NLDiagnostic *diag, const NLSource *source,
                       NLSourceSpan span)
{
    begin("status");
    string("stage", stage);
    number("status", status);
    if (diag) {
        string("code", diag->code);
        string("message", diag->message);
        number("start", span.start_byte);
        number("end", span.end_byte);
        flag("span_valid_for_input", nl_source_span_valid(source, span));
    }
    end();
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 64;
    int rc = 70;
    NLSource *unit_source = NULL;
    NLParser *unit_parser = NULL;
    NLSyntaxTree *unit_tree = NULL;
    NLSemanticContext *context = NULL;
    NLSource *entry_source = NULL;
    NLParser *entry_parser = NULL;
    NLSyntaxTree *entry_tree = NULL;
    NLCheckedFragment *checked = NULL;
    NLParseDiagnostic parse_diag = {0};
    NLFunctionUnitDiagnostic unit_diag = {0};
    NLCheckDiagnostic check_diag = {0};
    NLSourceStatus loaded = nl_source_load(argv[1], &unit_source);
    diagnostic("LOAD", loaded, NULL, NULL, (NLSourceSpan){0, 0});
    if (loaded != NL_SOURCE_OK)
        goto cleanup;
    NLParseStatus parsed = nl_parser_create(unit_source, &unit_parser);
    if (parsed == NL_PARSE_OK) {
        parsed =
            nl_parser_parse_function_unit(unit_parser, &unit_tree, &parse_diag);
    }
    diagnostic("PARSE_UNIT", parsed, &parse_diag.diagnostic, unit_source,
               parse_diag.span);
    if (parsed != NL_PARSE_OK) {
        rc = parsed < NL_PARSE_OUT_OF_MEMORY ? 3 : 70;
        goto cleanup;
    }
    NLCheckStatus status = nl_semantic_create(&context);
    diagnostic("CREATE_CONTEXT", status, NULL, NULL, (NLSourceSpan){0, 0});
    if (status != NL_CHECK_OK)
        goto cleanup;
    const NLSyntaxTree *inputs[] = {unit_tree};
    status = nl_semantic_register_function_unit(context, inputs, 1, &unit_diag);
    diagnostic("REGISTER", status, &unit_diag.diagnostic.diagnostic,
               unit_source, unit_diag.diagnostic.span);
    if (status != NL_CHECK_OK) {
        rc = status < NL_CHECK_OUT_OF_MEMORY ? 3 : 70;
        goto cleanup;
    }
    const char entry_text[] = "main()";
    loaded = nl_source_create(entry_text, sizeof(entry_text) - 1,
                              "<r1129 ordinary main selection>", &entry_source);
    if (loaded != NL_SOURCE_OK)
        goto cleanup;
    parsed = nl_parser_create(entry_source, &entry_parser);
    if (parsed == NL_PARSE_OK) {
        parsed = nl_parser_parse_expression_fragment(entry_parser, &entry_tree,
                                                     &parse_diag);
    }
    diagnostic("PARSE_ENTRY", parsed, &parse_diag.diagnostic, entry_source,
               parse_diag.span);
    if (parsed != NL_PARSE_OK)
        goto cleanup;
    status = nl_semantic_check_expression(context, entry_tree, &checked,
                                          &check_diag);
    diagnostic("CHECK_ENTRY", status, &check_diag.diagnostic, entry_source,
               check_diag.span);
    if (status != NL_CHECK_OK) {
        rc = status < NL_CHECK_OUT_OF_MEMORY ? 3 : 70;
        goto cleanup;
    }
    walk(checked, 0, 0, "entry");
    rc = fflush(stdout) == EOF || ferror(stdout) ? 74 : 0;
cleanup:
    if (checked)
        nl_checked_destroy(checked);
    nl_syntax_tree_destroy(entry_tree);
    nl_syntax_tree_destroy(unit_tree);
    if (entry_parser)
        nl_parser_destroy(entry_parser);
    if (unit_parser)
        nl_parser_destroy(unit_parser);
    nl_source_destroy(entry_source);
    nl_source_destroy(unit_source);
    nl_semantic_destroy(context);
    return rc;
}
