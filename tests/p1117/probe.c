/* Source-first P1117 observer; no host-issued value is used as authority. */
#define main p1117_driver_main
#include "main.c"
#undef main
#include "if_field_internal.h"
#include <assert.h>
NLCheckStatus __real_nl_if_field_prepare(NLCheckedFragment *, NLCheckedNodeId,
                                         const NLSemanticContext *);
NLCheckStatus __wrap_nl_if_field_prepare(NLCheckedFragment *f,
                                         NLCheckedNodeId id,
                                         const NLSemanticContext *c)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const NLCheckedNodeView *p = nl_checked_node_view(f, n->initializer);
    NLSemanticValueView v;
    assert(p && p->result_count == 1 &&
           nl_semantic_value_view(c, p->results[0].value, &v));
    printf("P1117 actual source IF bool_known=%d node_known=%d "
           "dependencies=%d count=%zu\n",
           v.scalar_known, p->scalar_result.known, v.dependencies,
           v.value_dependency_count);
    return __real_nl_if_field_prepare(f, id, c);
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit = NULL, *tree = NULL;
    NLSemanticContext *c = NULL;
    NLCheckedFragment *owned = NULL;
    assert(nl_source_load(argv[1], &source) == NL_SOURCE_OK);
    assert(nl_parser_create(source, &parser) == NL_PARSE_OK);
    NLParseStatus parse = nl_parser_parse_function_unit(parser, &unit, NULL);
    printf("P1117 parse=%d\n", parse);
    if (parse != NL_PARSE_OK)
        goto done;
    assert(nl_semantic_create(&c) == NL_CHECK_OK);
    void *snapshot = malloc(sizeof(*c));
    assert(snapshot);
    memcpy(snapshot, c, sizeof(*c));
    const NLSyntaxTree *units[] = {unit};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus s = nl_semantic_register_function_unit(c, units, 1, &d);
    bool unchanged = memcmp(snapshot, c, sizeof(*c)) == 0;
    free(snapshot);
    printf("P1117 registration=%d unchanged=%d no_owner=%d\n", s, unchanged,
           owned == NULL);
    if (s != NL_CHECK_OK) {
        assert(unchanged && !owned);
        (void)nl_check_diagnostic_render(stderr, source, &d.diagnostic);
        goto done;
    }
    assert(nl_source_create("main()", 6, "<P1117-entry>", &entry_source) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(entry_source, &entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(entry_parser, &tree, NULL) ==
           NL_PARSE_OK);
    s = nl_semantic_check_expression(c, tree, &owned, NULL);
    printf("P1117 actual invocation=%d owner=%d\n", s, owned != NULL);
    if (s == NL_CHECK_OK) {
        V0Program program = {.unit = nl_semantic_unit_type(c),
                             .u8 = nl_semantic_core_type(c, NL_TYPE_U8)};
        bool native = v0_validate_call(owned, nl_checked_root(owned), &program);
        printf("P1117 actual checked consumer=%d functions=%zu\n", native,
               program.count);
    }
done:
    nl_checked_destroy(owned);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return 0;
}
