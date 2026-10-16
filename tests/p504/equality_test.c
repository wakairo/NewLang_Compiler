#include "../support/semantic_check.h"
#include <stdint.h>
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

static bool tree(const char *text, bool unit, NLSource **source,
                 NLSyntaxTree **syntax)
{
    CHECK(nl_source_create(text, strlen(text), "p504-source", source) ==
          NL_SOURCE_OK);
    NLParser *parser = NULL;
    CHECK(nl_parser_create(*source, &parser) == NL_PARSE_OK);
    NLParseStatus status =
        unit ? nl_parser_parse_function_unit(parser, syntax, NULL)
             : nl_parser_parse_source_fragment(parser, syntax, NULL);
    nl_parser_destroy(parser);
    CHECK(status == NL_PARSE_OK);
    return true;
}

static bool parser(void)
{
    NLSource *source = NULL;
    NLSyntaxTree *syntax = NULL;
    CHECK(tree("usize(2) == usize(3)", false, &source, &syntax));
    const NLSyntaxView *v = nl_syntax_node_view(nl_syntax_tree_root(syntax));
    CHECK(v->kind == NL_SYNTAX_USIZE_EQUALITY && v->span.start_byte == 0 &&
          v->span.end_byte == 20);
    const NLSyntaxView *left = nl_syntax_node_view(v->data.equality.left);
    const NLSyntaxView *right = nl_syntax_node_view(v->data.equality.right);
    CHECK(left->kind == NL_SYNTAX_USIZE_LITERAL &&
          right->kind == NL_SYNTAX_USIZE_LITERAL && left->span.end_byte == 8 &&
          right->span.start_byte == 12);
    NLSourceView op;
    CHECK(nl_source_view(source, v->data.equality.operator_span, &op) &&
          op.length == 2 && memcmp(op.bytes, "==", 2) == 0);
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source);
    const char *rejected[] = {"a==b==c",
                              "a= =b",
                              "a===b",
                              "a!=b",
                              "a<b",
                              "a+ b==c",
                              "{usize(2)}==usize(2)"};
    for (size_t i = 0; i < sizeof rejected / sizeof rejected[0]; ++i) {
        source = NULL;
        syntax = NULL;
        NLParser *p = NULL;
        CHECK(nl_source_create(rejected[i], strlen(rejected[i]), "unsupported",
                               &source) == NL_SOURCE_OK);
        CHECK(nl_parser_create(source, &p) == NL_PARSE_OK);
        NLParseDiagnostic d = {0};
        CHECK(nl_parser_parse_source_fragment(p, &syntax, &d) != NL_PARSE_OK &&
              syntax == NULL);
        CHECK(nl_source_span_valid(source, d.span));
        nl_parser_destroy(p);
        nl_source_destroy(source);
    }
    return true;
}

static bool values(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    NLSemanticTypeView type;
    CHECK(nl_semantic_type_view(c, boolean, &type) && type.is_copy &&
          type.is_discardable);
    const char *inputs[] = {"usize(0)==usize(0)", "usize(0)==usize(1)",
                            "usize(17)==usize(17)", "usize(0002)==usize(2)",
                            "{let x=usize(9);let y=x;let out=y==x;out}"};
    const size_t expected[] = {1, 0, 1, 1, 1};
    for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; ++i) {
        TestChecked result = {0};
        CHECK(test_run(c, inputs[i], TEST_SOURCE, NL_CHECK_OK, NULL, &result));
        const NLCheckedNodeView *v = test_root(&result);
        NLSemanticValueView value;
        CHECK(v->type == boolean && v->result_count == 1 &&
              nl_semantic_value_view(c, v->results[0].value, &value) &&
              value.scalar_known && value.scalar_value == expected[i] &&
              value.domain == 0 && value.allocation_region == 0 &&
              value.reference.place == 0 &&
              value.dependencies == NL_DEPENDENCY_FREE);
        test_checked_destroy(&result);
    }
    char boundary[192];
    CHECK(snprintf(boundary, sizeof boundary, "usize(%zu)==usize(%zu)",
                   SIZE_MAX, SIZE_MAX) > 0);
    TestChecked maximum = {0};
    CHECK(test_run(c, boundary, TEST_SOURCE, NL_CHECK_OK, NULL, &maximum));
    CHECK(test_root(&maximum)->scalar_result.value == 1);
    test_checked_destroy(&maximum);
    CHECK(snprintf(boundary, sizeof boundary, "usize(%zu)==usize(0)",
                   SIZE_MAX) > 0);
    CHECK(test_run(c, boundary, TEST_SOURCE, NL_CHECK_OK, NULL, &maximum));
    CHECK(test_root(&maximum)->scalar_result.value == 0);
    test_checked_destroy(&maximum);
    nl_semantic_destroy(c);
    return true;
}

static bool rollback(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(test_rejected(c, "{let a=u8(2);let b=usize(2);a==b}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "USIZE-EQUALITY-TYPE"));
    CHECK(test_rejected(
        c, "{let x=u8(1);let p=loan_read(x){|r|ptr_from_ref(r)};p==p}",
        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR, "USIZE-EQUALITY-TYPE"));
    CHECK(test_rejected(c,
                        "{let a=usize(2);let b=usize(2);let "
                        "equal=a==b;replace(equal,u8(7));unit}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-WRITE-REF-REQUIRED"));
    CHECK(test_rejected(c,
                        "{let equal=usize(2)==usize(2);if (equal) {unit} else "
                        "{replace(equal,u8(7));unit}}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-WRITE-REF-REQUIRED"));
    NLSource *source = NULL;
    NLSyntaxTree *syntax = NULL;
    CHECK(tree("fn same(a:usize,b:usize)->bool{a==b}\nfn "
               "main()->unit{same(usize(2),usize(2));unit}\n",
               true, &source, &syntax));
    TestState before;
    CHECK(test_state(c, &before));
    const NLSyntaxTree *inputs[] = {syntax};
    NLFunctionUnitDiagnostic d = {0};
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, &d) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(strcmp(d.diagnostic.diagnostic.code, "USIZE-EQUALITY-PRECISION") ==
              0 &&
          test_unchanged(c, &before));
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}

static bool oom(void)
{
    const char *fragment =
        "{let a=usize(2);let b=usize(3);let equal=a==b;equal;unit}";
    NLSource *source = NULL;
    CHECK(nl_source_create(fragment, strlen(fragment), "p504-parser-oom",
                           &source) == NL_SOURCE_OK);
    bool completed = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        NLParser *p = NULL;
        NLSyntaxTree *syntax = NULL;
        CHECK(nl_parser_create(source, &p) == NL_PARSE_OK);
        allocation_index = 0;
        injecting = true;
        NLParseStatus status =
            nl_parser_parse_source_fragment(p, &syntax, NULL);
        injecting = false;
        nl_parser_destroy(p);
        if (status == NL_PARSE_OK) {
            nl_syntax_tree_destroy(syntax);
            completed = true;
            break;
        }
        CHECK(status == NL_PARSE_OUT_OF_MEMORY && syntax == NULL);
    }
    CHECK(completed && fail_at > 0);
    printf("P504_OOM parser=%zu\n", fail_at);
    nl_source_destroy(source);
    source = NULL;
    NLSyntaxTree *syntax = NULL;
    CHECK(tree(fragment, false, &source, &syntax));
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState before;
    CHECK(test_state(c, &before));
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        allocation_index = 0;
        injecting = true;
        const NLCheckStatus status =
            nl_semantic_check_source_fragment(c, syntax, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(artifact);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(c, &before));
    }
    CHECK(fail_at < 5000 && fail_at > 0);
    printf("P504_OOM semantic=%zu rollback=PASS\n", fail_at);
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source);
    source = NULL;
    syntax = NULL;
    CHECK(tree("fn main()->unit{let a=usize(2);let b=usize(3);let "
               "equal=a==b;equal;unit}",
               true, &source, &syntax));
    const NLSyntaxTree *inputs[] = {syntax};
    CHECK(test_state(c, &before));
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, NULL);
        injecting = false;
        if (status == NL_CHECK_OK)
            break;
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && test_unchanged(c, &before));
    }
    CHECK(fail_at < 5000 && fail_at > 0);
    printf("P504_OOM registration=%zu rollback=PASS\n", fail_at);
    nl_syntax_tree_destroy(syntax);
    nl_source_destroy(source);
    source = NULL;
    syntax = NULL;
    CHECK(tree("main()", false, &source, &syntax));
    CHECK(test_state(c, &before));
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_check_source_fragment(c, syntax, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(artifact);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(c, &before));
    }
    CHECK(fail_at < 5000 && fail_at > 0);
    printf("P504_OOM actual_main=%zu rollback=PASS\n", fail_at);
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
    if (strcmp(argv[1], "rollback") == 0)
        return rollback() ? 0 : 1;
    if (strcmp(argv[1], "oom") == 0)
        return oom() ? 0 : 1;
    return 2;
}
