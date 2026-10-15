/* Public-only definition/transaction test over the frozen real source.
 * Prefix unit checks independent helper definition, NOT the whole target or
 * caller state. No seed, synthetic original root or source-owned proof claim.
 */
#include "../support/semantic_check.h"
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
static bool injecting;
static size_t index_alloc, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && index_alloc++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && index_alloc++ == fail_at ? NULL : __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t size)
{
    return injecting && index_alloc++ == fail_at ? NULL
                                                 : __real_calloc(n, size);
}
static bool parse(NLSource *source, NLSyntaxTree **tree)
{
    NLParser *parser = NULL;
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    NLParseStatus s = nl_parser_parse_function_unit(parser, tree, NULL);
    nl_parser_destroy(parser);
    CHECK(s == NL_PARSE_OK);
    return true;
}
static bool empty_formals(const NLSemanticContext *c, size_t initial_functions)
{
    NLSemanticSnapshot s;
    CHECK(nl_semantic_snapshot(c, &s));
    CHECK(s.functions == initial_functions + 1 && s.bindings == 0 &&
          s.values == 0 && s.places == 0 && s.domains == 0 && s.scopes == 0 &&
          s.occurrences == 0 && s.backing_regions == 0);
    return true;
}
static bool test(const char *mode, const char *path)
{
    NLSource *source = NULL, *prefix = NULL;
    NLSyntaxTree *unit = NULL, *helper = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          parse(source, &unit));
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(c, before));
    const NLSyntaxTree *inputs[] = {unit};
    if (strcmp(mode, "definition") == 0 || strcmp(mode, "oom") == 0) {
        const NLSyntaxView *root =
            nl_syntax_node_view(nl_syntax_tree_root(unit));
        const NLSyntaxNode *decl = root->data.function_unit.declarations;
        while (decl && nl_syntax_node_view(decl)->kind != NL_SYNTAX_FUNCTION)
            decl = nl_syntax_next_argument(decl);
        CHECK(decl);
        NLSourceView bytes;
        CHECK(nl_source_view(
            source, (NLSourceSpan){0, nl_syntax_node_view(decl)->span.end_byte},
            &bytes));
        CHECK(nl_source_create(bytes.bytes, bytes.length,
                               "original-helper-prefix",
                               &prefix) == NL_SOURCE_OK &&
              parse(prefix, &helper));
        inputs[0] = helper;
        if (strcmp(mode, "oom") == 0) {
            bool success = false;
            for (fail_at = 0; fail_at < 4096; ++fail_at) {
                index_alloc = 0;
                injecting = true;
                NLCheckStatus status =
                    nl_semantic_register_function_unit(c, inputs, 1, NULL);
                injecting = false;
                if (status == NL_CHECK_OK) {
                    success = true;
                    break;
                }
                CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
                      test_unchanged(c, before));
            }
            CHECK(success && empty_formals(c, before->counts.functions));
            printf("independent Unknown-formal definition OOM positions=%zu; "
                   "failed registration public state unchanged\n",
                   fail_at);
        } else {
            CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
                      NL_CHECK_OK &&
                  empty_formals(c, before->counts.functions));
            puts("original helper definition checked without caller; no formal "
                 "root/value/domain exported");
        }
    } else {
        NLFunctionUnitDiagnostic d = {0};
        NLCheckStatus expected = strcmp(mode, "type") == 0
                                     ? NL_CHECK_SEMANTIC_ERROR
                                     : NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        const char *code = strcmp(mode, "type") == 0
                               ? "P3-TYPE-MISMATCH"
                               : "P9-CONTINUATION-PRECISION";
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, &d);
        CHECK(status == expected && d.diagnostic.diagnostic.code &&
              strcmp(d.diagnostic.diagnostic.code, code) == 0);
        CHECK(test_unchanged(c, before));
        printf(
            "%s; registration public state unchanged; no owned target issued\n",
            code);
    }
    free(before);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(helper);
    nl_source_destroy(prefix);
    nl_syntax_tree_destroy(unit);
    nl_source_destroy(source);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && test(argv[1], argv[2]) ? 0 : 1;
}
