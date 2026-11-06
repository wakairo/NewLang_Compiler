/* Actual CHECK-owned roles only; this observer never emits or runs a guest. */
#define main p254_unused_cli_main
#include "main.c"
#undef main
#include <assert.h>

int main(int argc, char **argv)
{
    assert(argc == 2);
    NLSource *source = NULL, *entry = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *tree = NULL, *entry_tree = NULL;
    NLSemanticContext *world = NULL;
    NLCheckedFragment *root = NULL;
    assert(nl_source_load(argv[1], &source) == NL_SOURCE_OK);
    assert(nl_parser_create(source, &parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    assert(nl_semantic_create(&world) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {tree};
    assert(nl_semantic_register_function_unit(world, units, 1, NULL) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C254 genuine roles", &entry) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(entry, &entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(entry_parser, &entry_tree,
                                               NULL) == NL_PARSE_OK);
    assert(nl_semantic_check_expression(world, entry_tree, &root, NULL) ==
           NL_CHECK_OK);
    assert(nl_h_custody_validate(root));
    const NLCheckedFragment *body =
        nl_checked_call_body(root, nl_checked_root(root));
    assert(body);
    NLCheckedNodeId loop = 0;
    NLSymbolId result = 0;
    for (size_t i = 0; i < body->count; ++i)
        if (body->nodes[i].kind == NL_CHECKED_LOOP) {
            assert(!loop);
            loop = i + 1;
        }
    assert(loop);
    for (size_t i = 0; i < body->count; ++i)
        if (body->nodes[i].kind == NL_CHECKED_BINDING &&
            body->nodes[i].initializer == loop)
            result = body->nodes[i].symbol;
    NLLinearBreakView proof;
    assert(result &&
           nl_linear_break_validate(body, loop, &proof) == NL_CHECK_OK);
    assert(proof.changing_current && proof.scalar_loop && proof.incoming_call);
    const NLCheckedFragment *arm = proof.iteration;
    const NLCheckedNodeView *call =
        nl_checked_node_view(arm, proof.incoming_call);
    assert(call);
    NLPureU8Call pure;
    assert(nl_h_custody_result_valid(arm, proof.incoming_call) &&
           nl_pure_u8_contents(arm, proof.incoming_call, &pure));
    const NLCheckedNodeView *selection=nl_checked_node_view(pure.body,pure.selection);
    printf("C256 HELPER formal=%zu selection=%zu normal=%zu returns=%zu\n",pure.formal,pure.selection,selection->normal_arms,nl_control_exits_count(pure.body->exits));
    for(size_t i=0;i<2;++i){
        const NLCheckedFragment *branch=nl_checked_if_arm(pure.body,pure.selection,i);
        const NLCheckedNodeView *literal=predicate_arm_value(branch);
        assert(literal&&copy_if_literal(branch,literal));
        printf("C256 LITERAL arm=%zu value=%zu\n",i,literal->scalar_result.value);
    }
    printf("C254 MAP loop=%zu phase=%zu result=%zu base=%zu index=%zu "
           "helper=%zu\n",
           loop, proof.phase_symbol, result, proof.base, proof.index,
           call->function);
    const NLTypeId byte = nl_semantic_core_type(world, NL_TYPE_U8);
    const NLTypeId quantity = nl_semantic_core_type(world, NL_TYPE_USIZE);
    const NLCheckedNodeView *loop_node = nl_checked_node_view(body, loop);
    bool prefix[NL_SEMANTIC_MAX_ENTRIES] = {0};
    for (size_t i = 0; i < body->count; ++i) {
        const NLCheckedNodeView *n = &body->nodes[i];
        if (n->kind != NL_CHECKED_BINDING ||
            n->span.end_byte > loop_node->span.start_byte)
            continue;
        const NLCheckedNodeView *receiver =
            nl_checked_node_view(body, n->first_argument);
        const NLCheckedNodeView *init =
            nl_checked_node_view(body, n->initializer);
        assert(receiver && init);
        if (receiver->type != byte)
            continue;
        assert(n->symbol && n->symbol <= world->binding_count &&
               n->symbol <= NL_SEMANTIC_MAX_ENTRIES);
        prefix[n->symbol - 1] = true;
        const char *kind = init->kind == NL_CHECKED_IDENTIFIER   ? "copy"
                           : init->kind == NL_CHECKED_U8_LITERAL ? "literal"
                           : init->kind == NL_CHECKED_FIELD_READ ? "field"
                                                                 : "other";
        printf("C254 PREFIX symbol=%zu name=%s kind=%s from=%zu index=%zu\n",
               n->symbol, world->bindings[n->symbol - 1].name, kind,
               init->symbol, init->field.index);
    }
    for (size_t i = 0; i < arm->count; ++i) {
        const NLCheckedNodeView *n = &arm->nodes[i];
        if (n->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *receiver =
                nl_checked_node_view(arm, n->first_argument);
            assert(receiver && n->symbol &&
                   n->symbol <= arm->context->binding_count);
            printf("C254 LOCAL symbol=%zu name=%s quantity=%d\n", n->symbol,
                   arm->context->bindings[n->symbol - 1].name,
                   receiver->type == quantity);
        }
        if (n->kind == NL_CHECKED_IDENTIFIER && n->type == byte && n->symbol &&
            n->symbol <= NL_SEMANTIC_MAX_ENTRIES && prefix[n->symbol - 1]) {
            bool statement = false;
            for (size_t j = 0; j < arm->count; ++j)
                statement |= arm->nodes[j].kind == NL_CHECKED_STATEMENT &&
                             arm->nodes[j].initializer == i + 1;
            printf("C254 CAPTURE node=%zu symbol=%zu statement=%d\n", i + 1,
                   n->symbol, statement);
        }
    }
    puts("C254 OWNED public-body=YES private-current=YES canonical-root=YES");
    nl_checked_destroy(root);
    nl_semantic_destroy(world);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return 0;
}
