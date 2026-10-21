#ifndef P688_SOURCE_CALL_H
#define P688_SOURCE_CALL_H
#include "../support/node_checked.h"

/* Test-only loader: registration, AST release, genuine fresh named call.
 * No semantic values, functions, bindings or authority are host-seeded. */
static bool selected_load(const char *path, const char *expression, TestNode *n)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    CHECK(nl_semantic_create(&n->context) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {tree};
    CHECK(nl_semantic_register_function_unit(n->context, units, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    CHECK(nl_source_create(expression, strlen(expression), "selected-call",
                           &n->entry_source) == NL_SOURCE_OK);
    parser = NULL;
    tree = NULL;
    CHECK(nl_parser_create(n->entry_source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &tree, NULL) ==
          NL_PARSE_OK);
    CHECK(nl_semantic_check_expression(n->context, tree, &n->entry, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    return true;
}
#endif
