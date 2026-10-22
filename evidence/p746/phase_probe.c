/* P746 public parse boundary only. No seed world, fabricated guest value,
 * semantic registration after a failed parse, or compiler implementation edit.
 */
#include "newlang/lexer.h"
#include "newlang/parser.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 2)
        return 1;
    NLSource *source = NULL;
    NLParser *parser = NULL;
    if (nl_source_load(argv[1], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK) {
        nl_source_destroy(source);
        return 1;
    }
    NLLexer lexer;
    NLToken token;
    size_t tokens = 0;
    if (!nl_lexer_init_source(&lexer, source))
        return 1;
    NLLexResult lex;
    while ((lex = nl_lexer_next(&lexer, &token)) == NL_LEX_TOKEN)
        ++tokens;
    printf("lexer=%d tokens=%zu eof=%zu\n", (int)lex, tokens,
           token.span.start_byte);
    bool valid = lex == NL_LEX_EOF;
    for (size_t attempt = 0; attempt < 2; ++attempt) {
        NLSyntaxTree *tree = NULL;
        NLParseDiagnostic diagnostic = {0};
        NLParseStatus status =
            nl_parser_parse_function_unit(parser, &tree, &diagnostic);
        printf("attempt=%zu parser=%d tree=%s code=%s span=%zu:%zu "
               "registration=NOT_ATTEMPTED owned=ABSENT\n",
               attempt + 1, (int)status, tree ? "present" : "NULL",
               diagnostic.diagnostic.code, diagnostic.span.start_byte,
               diagnostic.span.end_byte);
        valid = valid && status == NL_PARSE_SYNTAX_UNSUPPORTED && !tree;
        nl_syntax_tree_destroy(tree);
    }
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return valid ? 0 : 1;
}
