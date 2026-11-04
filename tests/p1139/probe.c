#define main c223_cli_main
#include "main.c"
#undef main
#include <assert.h>
int main(int argc, char **argv)
{
    assert(argc == 2);
    setvbuf(stdout, NULL, _IONBF, 0);
    NLSource *source = NULL, *entry = NULL;
    NLParser *parser = NULL, *ep = NULL;
    NLSyntaxTree *tree = NULL, *et = NULL;
    NLSemanticContext *c = NULL;
    NLCheckedFragment *root = NULL;
    assert(nl_source_load(argv[1], &source) == NL_SOURCE_OK);
    assert(nl_parser_create(source, &parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    assert(nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {tree};
    assert(nl_semantic_register_function_unit(c, units, 1, NULL) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C223 actual", &entry) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(entry, &ep) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(ep, &et, NULL) == NL_PARSE_OK);
    assert(nl_semantic_check_expression(c, et, &root, NULL) == NL_CHECK_OK);
    const NLCheckedFragment *f =
        nl_checked_call_body(root, nl_checked_root(root));
    assert(f);
    V0Program p = {.unit = nl_semantic_unit_type(c),
                   .u8 = nl_semantic_core_type(c, NL_TYPE_U8)};
    printf("owner %d\n", nl_linear_break_owner_validate(f));
    for (size_t id = 1; id <= f->count; ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n->kind == NL_CHECKED_LOOP) {
            NLLinearBreakView v = {0};
            printf("loop %zu status=%d ", id,
                   nl_linear_break_validate(f, id, &v));
            printf("scalar=%d type=%zu value=%zu\n", v.scalar_loop,
                   v.result.type, v.result.value);
        }
        printf("node %zu kind=%d type=%zu valid=%d\n", id, n->kind, n->type,
               v0_validate_node(f, id, &p));
    }
    printf("root_valid=%d\n",
           v0_validate_call(root, nl_checked_root(root), &p));
    nl_checked_destroy(root);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(et);
    nl_parser_destroy(ep);
    nl_source_destroy(entry);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return 0;
}
