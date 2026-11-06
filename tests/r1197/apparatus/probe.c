#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const NLCheckedFragment *seen[1024];
static size_t seen_count;

static const char *kind_name(NLCheckedKind kind)
{
    switch (kind) {
    case NL_CHECKED_BINDING:
        return "binding";
    case NL_CHECKED_REGISTERED_CALL:
        return "call";
    case NL_CHECKED_BLOCK:
        return "block";
    case NL_CHECKED_RETURN:
        return "return";
    case NL_CHECKED_IF:
        return "if";
    case NL_CHECKED_LOOP:
        return "loop";
    case NL_CHECKED_CONTINUE:
        return "continue";
    case NL_CHECKED_BREAK:
        return "break";
    case NL_CHECKED_REPLACE:
        return "replace";
    case NL_CHECKED_FIELD_READ:
        return "field_read";
    case NL_CHECKED_LOAN_HEADER:
        return "loan";
    case NL_CHECKED_AGGREGATE:
        return "aggregate";
    case NL_CHECKED_USIZE_EQUALITY:
        return "eq";
    case NL_CHECKED_USIZE_LESS_THAN:
        return "lt";
    case NL_CHECKED_USIZE_ADDITION:
        return "add";
    default:
        return "other";
    }
}

static void span_json(const NLSource *source, NLSourceSpan span)
{
    NLSourceView view;
    if (!nl_source_view(source, span, &view)) {
        fputs("null", stdout);
        return;
    }
    putchar('"');
    for (size_t i = 0; i < view.length; ++i) {
        unsigned char c = view.bytes[i];
        if (c == '"' || c == '\\') {
            putchar('\\');
            putchar(c);
        } else if (c < 32 || c > 126) {
            printf("\\u%04x", (unsigned)c);
        } else {
            putchar(c);
        }
    }
    putchar('"');
}

static void value_json(const NLSemanticContext *context, NLValueId value)
{
    NLSemanticValueView view;
    if (value == 0 || !nl_semantic_value_view(context, value, &view)) {
        fputs("null", stdout);
        return;
    }
    printf("{\"id\":%zu,\"type\":%zu,\"carrier\":%u,"
           "\"owner_place\":%zu,\"known\":%s,\"scalar\":%zu}",
           value, view.type, (unsigned)view.carrier, view.owner_place,
           view.scalar_known ? "true" : "false", view.scalar_value);
}

static int dump(const NLCheckedFragment *fragment, const char *path,
                size_t depth)
{
    if (fragment == NULL)
        return 0;
    for (size_t i = 0; i < seen_count; ++i) {
        if (seen[i] == fragment)
            return 0;
    }
    if (depth > 64 || seen_count == 1024)
        return 1;
    seen[seen_count++] = fragment;
    const NLSemanticContext *context = nl_checked_context(fragment);
    const NLSource *source = nl_checked_source(fragment);
    size_t count = nl_checked_node_count(fragment);
    printf("{\"record\":\"fragment\",\"path\":\"%s\","
           "\"owner\":\"%p\",\"context\":\"%p\",\"root\":%zu,"
           "\"node_count\":%zu}\n",
           path, (const void *)fragment, (const void *)context,
           nl_checked_root(fragment), count);
    for (size_t id = 1; id <= count; ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (node == NULL)
            continue;
        printf("{\"record\":\"node\",\"path\":\"%s\",\"id\":%zu,"
               "\"kind\":\"%s\",\"kind_id\":%u,\"name\":",
               path, id, kind_name(node->kind), (unsigned)node->kind);
        span_json(source, node->name);
        fputs(",\"source_text\":", stdout);
        span_json(source, node->span);
        printf(",\"initializer\":%zu,\"first_argument\":%zu,"
               "\"next_argument\":%zu,\"argument_count\":%zu,"
               "\"first_item\":%zu,\"next_item\":%zu,\"tail\":%zu,"
               "\"item_count\":%zu,\"field_index\":%zu,\"body_backed\":%s",
               node->initializer, node->first_argument, node->next_argument,
               node->argument_count, node->first_item, node->next_item,
               node->tail, node->item_count, node->field_index,
               node->body_backed ? "true" : "false");
        printf(",\"span\":[%zu,%zu],\"symbol\":%zu,\"type\":%zu,"
               "\"function\":%zu,\"terminates\":%s,\"normal_arms\":%zu,"
               "\"header_inductive\":%s,\"continue_edges\":%zu,"
               "\"break_edges\":%zu,\"return_edges\":%zu,"
               "\"returned_type\":%zu,\"returned\":",
               node->span.start_byte, node->span.end_byte, node->symbol,
               node->type, node->function, node->terminates ? "true" : "false",
               node->normal_arms, node->header_inductive ? "true" : "false",
               node->continue_edges, node->break_edges, node->return_edges,
               node->returned.type);
        value_json(context, node->returned.value);
        printf(",\"result_count\":%zu,\"results\":[", node->result_count);
        for (size_t r = 0; r < node->result_count && r < 2; ++r) {
            if (r != 0)
                putchar(',');
            value_json(context, node->results[r].value);
        }
        printf("],\"field\":{\"present\":%s,\"compatible\":%s,"
               "\"base\":%zu,\"nominal\":%zu,\"index\":%zu,"
               "\"parent\":%zu,\"child\":%zu,\"parent_incarnation\":%zu,"
               "\"child_incarnation\":%zu,\"parent_fact\":%zu,"
               "\"child_fact\":%zu,\"parent_post_fact\":%zu,"
               "\"child_post_fact\":%zu,\"old_value\":%zu,"
               "\"new_value\":%zu},\"loan\":{\"scope\":%zu,"
               "\"place\":%zu,\"incarnation\":%zu,\"nonescape\":%s,"
               "\"normal_result_forwarded\":%s}}\n",
               node->field.present ? "true" : "false",
               node->field.dependency_compatible ? "true" : "false",
               node->field.base, node->field.nominal, node->field.index,
               node->field.parent, node->field.child,
               node->field.parent_incarnation, node->field.child_incarnation,
               node->field.parent_fact, node->field.child_fact,
               node->field.parent_post_fact, node->field.child_post_fact,
               node->field.old_value, node->field.new_value, node->loan.scope,
               node->loan.place, node->loan.incarnation,
               node->loan.body_nonescape_proved ? "true" : "false",
               node->loan.normal_result_forwarded ? "true" : "false");
        char child[512];
        if (node->kind == NL_CHECKED_REGISTERED_CALL && node->body_backed) {
            int n = snprintf(child, sizeof child, "%s/call%zu", path, id);
            if (n < 0 || (size_t)n >= sizeof child ||
                dump(nl_checked_call_body(fragment, id), child, depth + 1))
                return 1;
        }
        if (node->kind == NL_CHECKED_LOOP) {
            printf("{\"record\":\"limited_break_provenance\","
                   "\"path\":\"%s\",\"id\":%zu,\"valid\":%s,"
                   "\"whole_loop_validator\":false}\n",
                   path, id,
                   nl_checked_loop_break_provenance_valid(fragment, id)
                       ? "true"
                       : "false");
            int n = snprintf(child, sizeof child, "%s/loop%zu", path, id);
            if (n < 0 || (size_t)n >= sizeof child ||
                dump(nl_checked_loop_body(fragment, id), child, depth + 1))
                return 1;
        }
        if (node->kind == NL_CHECKED_IF) {
            for (size_t arm = 0; arm < 2; ++arm) {
                int n = snprintf(child, sizeof child, "%s/if%zu/arm%zu", path,
                                 id, arm);
                if (n < 0 || (size_t)n >= sizeof child ||
                    dump(nl_checked_if_arm(fragment, id, arm), child,
                         depth + 1))
                    return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    static const char entry[] = "main()";
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *tree = NULL, *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    NLParseDiagnostic parse_diagnostic = {0};
    NLFunctionUnitDiagnostic unit_diagnostic = {0};
    NLCheckDiagnostic check_diagnostic = {0};
    int result = 2;
    if (argc != 2)
        return 2;
    NLSourceStatus load = nl_source_load(argv[1], &source);
    printf("{\"record\":\"load\",\"status\":%u}\n", (unsigned)load);
    if (load != NL_SOURCE_OK)
        goto done;
    NLParseStatus parse = nl_parser_create(source, &parser);
    if (parse == NL_PARSE_OK)
        parse = nl_parser_parse_function_unit(parser, &tree, &parse_diagnostic);
    printf("{\"record\":\"parse_unit\",\"status\":%u}\n", (unsigned)parse);
    if (parse != NL_PARSE_OK) {
        nl_parse_diagnostic_render(stdout, source, &parse_diagnostic);
        goto done;
    }
    NLCheckStatus status = nl_semantic_create(&context);
    if (status != NL_CHECK_OK)
        goto done;
    const NLSyntaxTree *inputs[] = {tree};
    status = nl_semantic_register_function_unit(context, inputs, 1,
                                                &unit_diagnostic);
    printf("{\"record\":\"register_unit\",\"status\":%u}\n", (unsigned)status);
    if (status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stdout, source, &unit_diagnostic.diagnostic);
        goto done;
    }
    if (nl_source_create(entry, strlen(entry), "r1197-entry.nl", &entry_source) !=
        NL_SOURCE_OK)
        goto done;
    parse = nl_parser_create(entry_source, &entry_parser);
    if (parse == NL_PARSE_OK)
        parse = nl_parser_parse_source_fragment(entry_parser, &entry_tree,
                                                &parse_diagnostic);
    if (parse != NL_PARSE_OK) {
        nl_parse_diagnostic_render(stdout, entry_source, &parse_diagnostic);
        goto done;
    }
    status = nl_semantic_check_source_fragment(context, entry_tree, &checked,
                                                &check_diagnostic);
    printf("{\"record\":\"source_issued_main_call\",\"status\":%u}\n",
           (unsigned)status);
    if (status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stdout, entry_source, &check_diagnostic);
        goto done;
    }
    if (nl_checked_root(checked) == 0)
        goto done;
    result = dump(checked, "entry", 0);
    printf("{\"record\":\"probe_complete\",\"status\":%d,"
           "\"fragments\":%zu,\"whole_loop_validator\":false}\n",
           result, seen_count);
done:
    nl_checked_destroy(checked);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return result;
}
