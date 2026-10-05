#include "../support/syntax_check.h"
#include "newlang/parser.h"

#include <stdlib.h>

typedef NLParseStatus (*Parse)(NLParser *, NLSyntaxTree **,
                               NLParseDiagnostic *);

static bool parse_file(const char *path, size_t index)
{
    const char *const expected[] = {
        "ref < write , ptr < T > >\r\n", " \tf(g(x),\r\n y)\t",
        "let old = replace(dst, value)\r\n",
        ("\tloan exclusive read p using stable as r "
         "{\r\n { f(r) } if x { y; }\r\n}\n")};
    const Parse parse[] = {
        nl_parser_parse_type_fragment, nl_parser_parse_expression_fragment,
        nl_parser_parse_binding_fragment, nl_parser_parse_loan_fragment};
    const NLSyntaxKind kinds[] = {NL_SYNTAX_TYPE_REF, NL_SYNTAX_EXPR_CALL,
                                  NL_SYNTAX_BINDING, NL_SYNTAX_LOAN};
    NLSource *source = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(strcmp(nl_source_name(source), path) == 0);
    NLSourceView original;
    CHECK(nl_source_view(source, (NLSourceSpan){0, nl_source_length(source)},
                         &original));
    CHECK(original.length == strlen(expected[index]));
    CHECK(memcmp(original.bytes, expected[index], original.length) == 0);
    NLParser *parser = NULL;
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    NLSyntaxTree *first = NULL;
    NLSyntaxTree *second = NULL;
    CHECK(parse[index](parser, &first, NULL) == NL_PARSE_OK);
    CHECK(parse[index](parser, &second, NULL) == NL_PARSE_OK);
    CHECK(test_tree(first) && test_tree(second));
    CHECK(test_equivalent(nl_syntax_tree_root(first),
                          nl_syntax_tree_root(second)));
    const NLSyntaxView *const root =
        nl_syntax_node_view(nl_syntax_tree_root(first));
    CHECK(root->kind == kinds[index]);
    if (index == 0) {
        CHECK(root->data.ref_type.access == NL_ACCESS_WRITE &&
              !root->data.ref_type.is_exclusive);
        const NLSyntaxView *const target =
            nl_syntax_node_view(root->data.ref_type.target);
        CHECK(target->kind == NL_SYNTAX_TYPE_PTR);
        CHECK(test_name(
            source,
            nl_syntax_node_view(target->data.ptr_type.target)->data.name, "T"));
        CHECK(root->span.start_byte == 0 &&
              root->span.end_byte == original.length - 2);
    } else if (index == 1) {
        CHECK(root->span.start_byte == 2 && root->span.end_byte == 14);
        CHECK(root->data.call.argument_count == 2 &&
              test_name(source, root->data.call.callee, "f"));
        const NLSyntaxNode *const first_arg = root->data.call.arguments;
        CHECK(nl_syntax_node_view(first_arg)->kind == NL_SYNTAX_EXPR_CALL);
        const NLSyntaxView *const second_arg =
            nl_syntax_node_view(nl_syntax_next_argument(first_arg));
        CHECK(second_arg->span.start_byte == 12 &&
              second_arg->span.end_byte == 13);
        CHECK(test_name(source, second_arg->data.name, "y"));
    } else if (index == 2) {
        CHECK(test_name(source, root->data.binding.name, "old"));
        const NLSyntaxView *const init =
            nl_syntax_node_view(root->data.binding.initializer);
        CHECK(init->kind == NL_SYNTAX_EXPR_CALL &&
              init->data.call.argument_count == 2);
        CHECK(test_name(source, init->data.call.callee, "replace"));
    } else {
        CHECK(root->data.loan.access == NL_ACCESS_READ &&
              root->data.loan.is_exclusive);
        CHECK(test_name(source,
                        nl_syntax_node_view(root->data.loan.source)->data.name,
                        "p"));
        CHECK(test_name(
            source, nl_syntax_node_view(root->data.loan.stability)->data.name,
            "stable"));
        CHECK(test_name(source, root->data.loan.binding, "r"));
        CHECK(test_name(source, root->data.loan.body_interior,
                        "\r\n { f(r) } if x { y; }\r\n"));
        CHECK(root->span.start_byte == 1 &&
              root->span.end_byte == original.length - 1);
    }
    nl_parser_destroy(parser);
    CHECK(test_tree(first));
    CHECK(memcmp(original.bytes, expected[index], original.length) == 0);
    nl_syntax_tree_destroy(first);
    nl_syntax_tree_destroy(second);
    nl_source_destroy(source);
    return true;
}

static bool unsupported_file(const char *path)
{
    NLSource *source = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    NLParser *parser = NULL;
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    NLSyntaxTree *tree = NULL;
    NLParseDiagnostic diagnostic;
    CHECK(nl_parser_parse_loan_fragment(parser, &tree, &diagnostic) ==
          NL_PARSE_LEXICALLY_UNSUPPORTED);
    CHECK(tree == NULL && diagnostic.span.start_byte == 19 &&
          diagnostic.span.end_byte == 20);
    CHECK(strcmp(diagnostic.diagnostic.code, "P2-LEXICAL-UNSUPPORTED") == 0);
    NLSourceView view;
    CHECK(nl_source_view(source, diagnostic.span, &view) && view.length == 1 &&
          view.bytes[0] == 0);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 6) {
        return EXIT_FAILURE;
    }
    for (size_t index = 0; index < 4; ++index) {
        if (!parse_file(argv[index + 1], index)) {
            return EXIT_FAILURE;
        }
    }
    if (!unsupported_file(argv[5])) {
        return EXIT_FAILURE;
    }
    puts("source -> lexer -> parser: exact bytes, syntax roles, spans and "
         "diagnostics passed");
    return EXIT_SUCCESS;
}
