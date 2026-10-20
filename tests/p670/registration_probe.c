/* Read-only public frontend observation. No checked-evidence alteration. */
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include <stdio.h>

static bool same(NLSemanticSnapshot a, NLSemanticSnapshot b)
{
    return a.types == b.types && a.bindings == b.bindings &&
           a.values == b.values && a.places == b.places &&
           a.domains == b.domains && a.scopes == b.scopes &&
           a.functions == b.functions &&
           a.last_incarnation == b.last_incarnation &&
           a.last_value_fact == b.last_value_fact &&
           a.backing_regions == b.backing_regions &&
           a.raw_intervals == b.raw_intervals && a.occurrences == b.occurrences;
}

int main(int argc, char **argv)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *context = NULL;
    NLSemanticSnapshot before = {0}, after = {0};
    int result = 1;
    if (argc != 2 || nl_source_load(argv[1], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK)
        goto cleanup;
    const NLParseStatus parsed =
        nl_parser_parse_function_unit(parser, &tree, NULL);
    if (parsed != NL_PARSE_OK || nl_semantic_create(&context) != NL_CHECK_OK ||
        !nl_semantic_snapshot(context, &before))
        goto cleanup;
    const NLSyntaxTree *inputs[] = {tree};
    NLFunctionUnitDiagnostic diagnostic = {0};
    const NLCheckStatus registered =
        nl_semantic_register_function_unit(context, inputs, 1, &diagnostic);
    if (!nl_semantic_snapshot(context, &after) || !same(before, after) ||
        (registered != NL_CHECK_SEMANTIC_ERROR &&
         registered != NL_CHECK_SEMANTIC_UNSUPPORTED))
        goto cleanup;
    printf("{\"parse\":%d,\"registration\":%d,\"category\":\"%s\","
           "\"diagnostic\":\"%s\",\"span\":[%zu,%zu],"
           "\"public_snapshot_unchanged\":true,\"entry\":\"NOT_ENTERED\","
           "\"owned\":\"NOT_ISSUED\",\"c17\":\"NOT_ENTERED\"}\n",
           (int)parsed, (int)registered,
           diagnostic.diagnostic.diagnostic.category,
           diagnostic.diagnostic.diagnostic.code,
           diagnostic.diagnostic.span.start_byte,
           diagnostic.diagnostic.span.end_byte);
    result = 0;
cleanup:
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return result;
}
