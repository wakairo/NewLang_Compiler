#ifndef NEWLANG_TEST_NODE_CHECKED_H
#define NEWLANG_TEST_NODE_CHECKED_H
#include "semantic_check.h"
/* Test-owned actual-source → registered unit → actual main() artifact. No
 * seed, prelude, semantic fixture value or host-created pointer/topology. */
typedef struct {
    NLSemanticContext *context;
    NLSource *entry_source;
    NLCheckedFragment *entry;
} TestNode;
static inline void node_checked_destroy(TestNode *n)
{
    nl_checked_destroy(n->entry);
    nl_source_destroy(n->entry_source);
    nl_semantic_destroy(n->context);
    *n = (TestNode){0};
}
static inline bool node_checked_load(const char *path, TestNode *out)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    CHECK(nl_semantic_create(&out->context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {unit};
    CHECK(nl_semantic_register_function_unit(out->context, inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    CHECK(nl_source_create("main()", 6, "node-native-entry",
                           &out->entry_source) == NL_SOURCE_OK);
    parser = NULL;
    unit = NULL;
    CHECK(nl_parser_create(out->entry_source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &unit, NULL) ==
          NL_PARSE_OK);
    CHECK(nl_semantic_check_expression(out->context, unit, &out->entry, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    return true;
}
#endif
