#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/syntax.h"

#include <stdio.h>
#include <string.h>

static void name(const NLCheckedFragment *fragment, NLSourceSpan span)
{
    NLSourceView text;
    if (nl_source_view(nl_checked_source(fragment), span, &text)) {
        for (size_t i = 0; i < text.length; ++i) {
            unsigned char c = text.bytes[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_')
                putchar((int)c);
            else
                putchar('_');
        }
    }
}

static void dump(const NLCheckedFragment *fragment, const char *label,
                 size_t depth)
{
    if (depth > 8)
        return;
    printf("{\"fragment\":\"%s\",\"count\":%zu,\"root\":%zu}\n", label,
           nl_checked_node_count(fragment), nl_checked_root(fragment));
    for (size_t i = 1; i <= nl_checked_node_count(fragment); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(fragment, i);
        printf("{\"fragment\":\"%s\",\"id\":%zu,\"kind\":%d,\"symbol\":%zu,"
               "\"function\":%zu,\"type\":%zu,\"initializer\":%zu,"
               "\"copy\":%s,\"binding\":%s,\"call\":%s,\"loop\":%s,"
               "\"name\":\"",
               label, i, (int)v->kind, v->symbol, v->function, v->type,
               v->initializer, v->value_use == NL_VALUE_COPIED ? "true" : "false",
               v->kind == NL_CHECKED_BINDING ? "true" : "false",
               v->kind == NL_CHECKED_REGISTERED_CALL ? "true" : "false",
               v->kind == NL_CHECKED_LOOP ? "true" : "false");
        name(fragment, v->name);
        printf("\",\"result_count\":%zu,\"value\":%zu,"
               "\"header_inductive\":%s,\"continues\":%zu,\"breaks\":%zu,"
               "\"field\":%s,\"field_index\":%zu,\"base\":%zu,"
               "\"parent\":%zu,\"child\":%zu,\"parent_incarnation\":%zu,"
               "\"child_incarnation\":%zu,\"parent_fact\":%zu,"
               "\"parent_post_fact\":%zu,\"child_fact\":%zu,"
               "\"child_post_fact\":%zu,\"old_value\":%zu,\"new_value\":%zu}\n",
               v->result_count, v->result_count ? v->results[0].value : 0,
               v->header_inductive ? "true" : "false", v->continue_edges,
               v->break_edges, v->field.present ? "true" : "false",
               v->field.index, v->field.base, v->field.parent, v->field.child,
               v->field.parent_incarnation, v->field.child_incarnation,
               v->field.parent_fact, v->field.parent_post_fact,
               v->field.child_fact, v->field.child_post_fact, v->field.old_value,
               v->field.new_value);
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(fragment, i);
            if (body)
                dump(body, "callee", depth + 1);
        }
        if (v->kind == NL_CHECKED_LOOP) {
            const NLCheckedFragment *body = nl_checked_loop_body(fragment, i);
            if (body)
                dump(body, "loop_body", depth + 1);
        }
    }
}

int main(int argc, char **argv)
{
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit = NULL, *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *entry = NULL;
    NLParseDiagnostic parse = {0};
    NLFunctionUnitDiagnostic declaration = {0};
    NLCheckDiagnostic check = {0};
    int result = 1;
    if (argc != 2 || nl_source_load(argv[1], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK)
        goto done;
    if (nl_parser_parse_function_unit(parser, &unit, &parse) != NL_PARSE_OK) {
        nl_parse_diagnostic_render(stderr, source, &parse);
        goto done;
    }
    if (nl_semantic_create(&context) != NL_CHECK_OK)
        goto done;
    const NLSyntaxTree *inputs[] = {unit};
    if (nl_semantic_register_function_unit(context, inputs, 1, &declaration) !=
        NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, source, &declaration.diagnostic);
        goto done;
    }
    if (nl_source_create("main()", strlen("main()"), "<r1151-entry>",
                         &entry_source) != NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(entry_parser, &entry_tree, &parse) !=
            NL_PARSE_OK ||
        nl_semantic_check_expression(context, entry_tree, &entry, &check) !=
            NL_CHECK_OK)
        goto done;
    const NLCheckedFragment *body =
        nl_checked_call_body(entry, nl_checked_root(entry));
    if (!body)
        goto done;
    printf("{\"authentic_owned_entry\":true,\"entry_function\":%zu,"
           "\"probe\":\"R1151-public-only\"}\n",
           nl_checked_node_view(entry, nl_checked_root(entry))->function);
    dump(body, "main", 0);
    result = 0;
done:
    nl_checked_destroy(entry);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(entry_tree);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(entry_parser);
    nl_parser_destroy(parser);
    nl_source_destroy(entry_source);
    nl_source_destroy(source);
    return result;
}
