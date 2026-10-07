#include "../support/semantic_check.h"
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}
static bool tree(const char *text, bool function_unit, NLSource **source,
                 NLSyntaxTree **syntax)
{
    CHECK(nl_source_create(text, strlen(text), "v1-source", source) ==
          NL_SOURCE_OK);
    NLParser *parser = NULL;
    CHECK(nl_parser_create(*source, &parser) == NL_PARSE_OK);
    const NLParseStatus status =
        function_unit ? nl_parser_parse_function_unit(parser, syntax, NULL)
                      : nl_parser_parse_source_fragment(parser, syntax, NULL);
    nl_parser_destroy(parser);
    CHECK(status == NL_PARSE_OK);
    return true;
}
static bool parser(void)
{
    const char *inputs[] = {"u8(7)", "u8(255)", "u8(256)"};
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        NLSource *source = NULL;
        NLSyntaxTree *syntax = NULL;
        CHECK(tree(inputs[i], false, &source, &syntax));
        const NLSyntaxView *v =
            nl_syntax_node_view(nl_syntax_tree_root(syntax));
        CHECK(v->kind == NL_SYNTAX_U8_LITERAL);
        NLSourceView payload;
        CHECK(nl_source_view(source, v->data.u8_digits, &payload));
        CHECK(payload.length == strlen(inputs[i]) - 4);
        CHECK(memcmp(payload.bytes, inputs[i] + 3, payload.length) == 0);
        nl_syntax_tree_destroy(syntax);
        nl_source_destroy(source);
    }
    NLSource *source = NULL;
    CHECK(nl_source_create("let x=7;", 8, "bare", &source) == NL_SOURCE_OK);
    NLParser *p = NULL;
    CHECK(nl_parser_create(source, &p) == NL_PARSE_OK);
    NLSyntaxTree *syntax = NULL;
    NLParseDiagnostic diagnostic = {0};
    CHECK(nl_parser_parse_source_fragment(p, &syntax, &diagnostic) !=
          NL_PARSE_OK);
    CHECK(syntax == NULL);
    CHECK(strcmp(diagnostic.diagnostic.code, "P5-EXPECTED-EXPRESSION") == 0);
    nl_parser_destroy(p);
    nl_source_destroy(source);
    return true;
}
static bool values(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    NLSemanticTypeView t;
    CHECK(nl_semantic_type_view(c, u8, &t) && t.is_copy && t.is_discardable);
    const char *inputs[] = {"u8(0)", "u8(7)", "u8(255)"};
    const size_t values[] = {0, 7, 255};
    for (size_t i = 0; i < 3; ++i) {
        TestChecked a = {0};
        CHECK(test_run(c, inputs[i], TEST_SOURCE, NL_CHECK_OK, NULL, &a));
        const NLCheckedNodeView *v = test_root(&a);
        CHECK(v->kind == NL_CHECKED_U8_LITERAL && v->type == u8);
        CHECK(v->has_scalar_result && v->scalar_result.known &&
              v->scalar_result.type == u8 &&
              v->scalar_result.value == values[i]);
        CHECK(v->result_count == 1 && v->results[0].type == u8);
        NLSemanticValueView package;
        CHECK(nl_semantic_value_view(c, v->results[0].value, &package));
        CHECK(package.type == u8 && package.scalar_known &&
              package.scalar_value == values[i]);
        test_checked_destroy(&a);
    }
    CHECK(test_rejected(c, "{let good=u8(7);let bad=u8(256);unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "V1-U8-LITERAL-RANGE"));
    CHECK(test_rejected(c, "u8(9999999999999999999999999999999999999999)",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "V1-U8-LITERAL-RANGE"));
    nl_semantic_destroy(c);
    return true;
}
static bool body(void)
{
    /* Actual source declarations; no seeded literal, parameter or prelude. */
    NLSource *source = NULL;
    NLSyntaxTree *syntax = NULL;
    CHECK(tree("fn main()->unit{let x=u8(7);x;unit}", true, &source, &syntax));
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {syntax};
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source);
    TestChecked call = {0};
    CHECK(test_run(c, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL, &call));
    const NLCheckedFragment *b =
        nl_checked_call_body(call.artifact, nl_checked_root(call.artifact));
    CHECK(b != NULL);
    const NLCheckedNodeView *root = nl_checked_node_view(b, nl_checked_root(b));
    const NLCheckedNodeView *binding =
        nl_checked_node_view(b, root->first_item);
    const NLCheckedNodeView *literal =
        nl_checked_node_view(b, binding->initializer);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    CHECK(binding->kind == NL_CHECKED_BINDING && binding->symbol != 0);
    CHECK(literal->kind == NL_CHECKED_U8_LITERAL && literal->type == u8 &&
          literal->has_scalar_result && literal->scalar_result.type == u8 &&
          literal->scalar_result.known && literal->scalar_result.value == 7);
    const NLCheckedNodeView *statement =
        nl_checked_node_view(b, binding->next_item);
    const NLCheckedNodeView *use =
        nl_checked_node_view(b, statement->initializer);
    CHECK(use->kind == NL_CHECKED_IDENTIFIER && use->type == u8 &&
          use->symbol == binding->symbol && use->value_use == NL_VALUE_COPIED);
    NLSemanticValueView copy;
    CHECK(nl_semantic_value_view(c, use->results[0].value, &copy));
    CHECK(copy.scalar_known && copy.scalar_value == 7);
    test_checked_destroy(&call);
    nl_semantic_destroy(c);
    return true;
}
static bool failures(void)
{
    NLSource *source = NULL;
    NLSyntaxTree *syntax = NULL;
    CHECK(tree("{let x=u8(7);x;unit}", false, &source, &syntax));
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState before;
    CHECK(test_state(c, &before));
    bool completed = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        allocation_index = 0;
        injecting = true;
        const NLCheckStatus status =
            nl_semantic_check_source_fragment(c, syntax, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(artifact);
            completed = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL);
        CHECK(test_unchanged(c, &before));
    }
    CHECK(completed && fail_at > 0);
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    source = NULL;
    syntax = NULL;
    c = NULL;
    CHECK(tree("fn main()->unit{let x=u8(7);x;unit}", true, &source, &syntax));
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(test_state(c, &before));
    const NLSyntaxTree *inputs[] = {syntax};
    completed = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        const NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            completed = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && test_unchanged(c, &before));
    }
    CHECK(completed && fail_at > 0);
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source);
    source = NULL;
    syntax = NULL;
    CHECK(tree("main()", false, &source, &syntax));
    CHECK(test_state(c, &before));
    completed = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        allocation_index = 0;
        injecting = true;
        const NLCheckStatus status =
            nl_semantic_check_source_fragment(c, syntax, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(artifact);
            completed = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL);
        CHECK(test_unchanged(c, &before));
    }
    CHECK(completed && fail_at > 0);
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "parser") == 0)
        return parser() ? 0 : 1;
    if (strcmp(argv[1], "values") == 0)
        return values() ? 0 : 1;
    if (strcmp(argv[1], "body") == 0)
        return body() ? 0 : 1;
    if (strcmp(argv[1], "failures") == 0)
        return failures() ? 0 : 1;
    return 2;
}
