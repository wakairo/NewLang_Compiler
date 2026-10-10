/* P281: public read-only parse/register boundary; no seeded owner facts. */
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *syntax = NULL;
    NLSemanticContext *context = NULL;
    NLSemanticSnapshot before = {0}, after = {0};
    NLParseDiagnostic parsed = {0};
    NLFunctionUnitDiagnostic registered = {0};
    int result = EXIT_FAILURE;
    if (argc != 2 || nl_source_load(argv[1], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_semantic_create(&context) != NL_CHECK_OK ||
        !nl_semantic_snapshot(context, &before))
        goto done;
    const NLParseStatus ps =
        nl_parser_parse_function_unit(parser, &syntax, &parsed);
    NLCheckStatus cs = NL_CHECK_OK;
    const char *code = parsed.diagnostic.code;
    NLSourceSpan span = parsed.span;
    if (ps == NL_PARSE_OK) {
        const NLSyntaxTree *units[] = {syntax};
        cs = nl_semantic_register_function_unit(context, units, 1, &registered);
        code = registered.diagnostic.diagnostic.code;
        span = registered.diagnostic.span;
    }
    if (!nl_semantic_snapshot(context, &after) ||
        (ps != NL_PARSE_OK && syntax != NULL) ||
        ((ps != NL_PARSE_OK || cs != NL_CHECK_OK) &&
         memcmp(&before, &after, sizeof(before)) != 0))
        goto done;
    printf("{\"parse_status\":%d,\"syntax_tree\":%s,"
           "\"registration_attempted\":%s,\"registration_status\":",
           ps, syntax ? "true" : "false", ps == NL_PARSE_OK ? "true" : "false");
    if (ps == NL_PARSE_OK)
        printf("%d", cs);
    else
        printf("null");
    printf(",\"code\":\"%s\",\"start_byte\":%zu,\"end_byte\":%zu,"
           "\"snapshot_unchanged\":%s,\"owned_entry_artifact\":false,"
           "\"entry_check_attempted\":false,\"backend_attempted\":false}\n",
           code ? code : "", span.start_byte, span.end_byte,
           memcmp(&before, &after, sizeof(before)) == 0 ? "true" : "false");
    result = EXIT_SUCCESS;
done:
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(syntax);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return result;
}
