/* Observe only actual CHECK-owned roles; never run the emitted guest here. */
#define main p1188_c250a_cli_main
#include "main.c"
#undef main
#include <assert.h>

int main(int argc, char **argv)
{
    assert(argc == 2);
    NLSource *source = NULL, *entry = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *syntax = NULL, *entry_syntax = NULL;
    NLSemanticContext *world = NULL;
    NLCheckedFragment *root = NULL;
    assert(nl_source_load(argv[1], &source) == NL_SOURCE_OK);
    assert(nl_parser_create(source, &parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(parser, &syntax, NULL) == NL_PARSE_OK);
    assert(nl_semantic_create(&world) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {syntax};
    assert(nl_semantic_register_function_unit(world, units, 1, NULL) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C250a actual source roles", &entry) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(entry, &entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(entry_parser, &entry_syntax,
                                               NULL) == NL_PARSE_OK);
    assert(nl_semantic_check_expression(world, entry_syntax, &root, NULL) ==
           NL_CHECK_OK);
    assert(nl_h_custody_validate(root));
    const NLCheckedFragment *body =
        nl_checked_call_body(root, nl_checked_root(root));
    assert(body);
    NLCheckedNodeId loop = 0;
    for (size_t i = 0; i < body->count; ++i)
        if (body->nodes[i].kind == NL_CHECKED_LOOP) {
            assert(!loop);
            loop = i + 1;
        }
    NLLinearBreakView proof;
    assert(loop && nl_linear_break_validate(body, loop, &proof) == NL_CHECK_OK);
    assert(proof.changing_current && proof.scalar_loop && proof.iteration);
    NLSymbolId result = 0;
    for (size_t i = 0; i < body->count; ++i)
        if (body->nodes[i].kind == NL_CHECKED_BINDING &&
            body->nodes[i].initializer == loop) {
            assert(!result);
            result = body->nodes[i].symbol;
        }
    assert(result);
    printf("C250a MAP loop=%zu phase=%zu result=%zu index=%zu helper=%zu\n",
           loop, proof.phase_symbol, result, proof.index, proof.incoming_call);
    const NLCheckedFragment *arm = proof.iteration;
    const NLCheckedNodeView *wrapper = nl_checked_node_view(arm, arm->root);
    const NLCheckedNodeView *block =
        wrapper ? nl_checked_node_view(arm, wrapper->initializer) : NULL;
    assert(block && block->kind == NL_CHECKED_BLOCK);
    const NLTypeId quantity =
        nl_semantic_core_type(arm->context, NL_TYPE_USIZE);
    size_t statements = 0;
    for (NLCheckedNodeId id = block->first_item; id;) {
        const NLCheckedNodeView *node = nl_checked_node_view(arm, id);
        assert(node);
        const NLCheckedNodeView *value =
            nl_checked_node_view(arm, node->initializer);
        if (node->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *receiver =
                nl_checked_node_view(arm, node->first_argument);
            assert(receiver && node->symbol &&
                   node->symbol <= arm->context->binding_count);
            printf("C250a LOCAL symbol=%zu name=%s quantity=%d\n", node->symbol,
                   arm->context->bindings[node->symbol - 1].name,
                   receiver->type == quantity);
        } else if (node->kind == NL_CHECKED_STATEMENT && value &&
                   value->type == quantity) {
            assert(value->kind == NL_CHECKED_IDENTIFIER &&
                   value->value_use == NL_VALUE_COPIED);
            printf("C250a DISCARD node=%zu symbol=%zu phase=%d\n", id,
                   value->symbol, value->symbol == proof.phase_symbol);
            ++statements;
        }
        id = node->next_item;
    }
    printf("C250a OWNED current=YES source-statements=%zu\n", statements);
    nl_checked_destroy(root);
    nl_semantic_destroy(world);
    nl_syntax_tree_destroy(entry_syntax);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry);
    nl_syntax_tree_destroy(syntax);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return 0;
}
