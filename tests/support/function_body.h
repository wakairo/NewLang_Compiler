#ifndef NEWLANG_TEST_FUNCTION_BODY_H
#define NEWLANG_TEST_FUNCTION_BODY_H
#include "semantic_check.h"

static inline bool body_tree(const char *text, NLSource **source,
                             NLSyntaxTree **tree)
{
    CHECK(nl_source_create(text, strlen(text), "registered-body", source) ==
          NL_SOURCE_OK);
    NLParser *parser = NULL;
    CHECK(nl_parser_create(*source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_source_fragment(parser, tree, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    return true;
}
static inline bool register_body(NLSemanticContext *c, const char *name,
                                 const NLFunctionParameter *params,
                                 size_t count, NLTypeId result,
                                 const char *text, NLCheckStatus expected,
                                 const char *code)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(body_tree(text, &source, &tree));
    TestState before;
    CHECK(test_state(c, &before));
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_register_function_body(
        c, name, params, count, result, tree, &diagnostic);
    if (status != expected)
        fprintf(stderr, "%s %s: status %d expected %d (%s)\n", name, text,
                status, expected, diagnostic.diagnostic.code);
    CHECK(status == expected);
    if (expected != NL_CHECK_OK) {
        CHECK(test_unchanged(c, &before));
        CHECK(nl_source_span_valid(source, diagnostic.span));
        if (code != NULL) {
            if (strcmp(diagnostic.diagnostic.code, code) != 0)
                fprintf(stderr, "expected %s got %s\n", code,
                        diagnostic.diagnostic.code);
            CHECK(strcmp(diagnostic.diagnostic.code, code) == 0);
        }
    }
    /* Real calls must survive destruction of ALL registration input owners. */
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
static inline bool body_ok(NLSemanticContext *c, const char *text)
{
    TestChecked checked = {0};
    CHECK(test_run(c, text, TEST_SOURCE, NL_CHECK_OK, NULL, &checked));
    test_checked_destroy(&checked);
    return true;
}
static inline bool body_binding(NLSemanticContext *c, const char *name,
                                NLSemanticBindingView *out)
{
    CHECK(nl_semantic_binding_view(c, nl_semantic_find_binding(c, name), out));
    return true;
}
#endif
