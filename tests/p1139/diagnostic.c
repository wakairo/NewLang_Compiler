#include "semantic_check.c"
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
    const NLLinearBreakProof *p = f->linear_breaks;
    assert(p && p->cycle);
    const NLStutterProof *s = p->cycle;
    NLLinearBreakView out = {0};
    LinearBreakEffect effect = {0};
    printf("scalar=%d sealed=%d owned=%d def=%d cycle_valid=%d relation=%d "
           "frame=%d\n",
           s->scalar, s->sealed, s->graph_owned, s->definition,
           validate_stutter_cycle(p, &out),
           stutter_relation(s, p->owner, &effect),
           stutter_frame(nl_control_state_context(s->entry),
                         nl_control_state_context(p->entry), 0));
    const NLCheckedNodeView *n = nl_checked_node_view(p->owner, p->loop);
    printf("loop kind=%d args=%zu c=%zu b=%zu r=%zu header=%d init=%zu "
           "graphroot=%zu\n",
           n->kind, n->argument_count, n->continue_edges, n->break_edges,
           n->return_edges, n->header_inductive, s->initial_node,
           s->graph->root);
    for (size_t i = 0; i < 3; ++i) {
        const StutterGraph *a = &s->graphs[i];
        printf("graph%zu pointer=%d source=%d nodes=%d worldptr=%d world=%d\n",
               i,
               !i || nl_checked_if_arm(s->graph, s->conditional, i - 1) ==
                         a->fragment,
               a->fragment->source == a->source,
               linear_graph_matches(a->fragment, a->base, a->nodes, a->count,
                                    a->root),
               a->fragment->context == a->world.world,
               linear_world_matches(a->world.world, &a->world.storage,
                                    a->world.copy, &a->world.anchor));
    }
    for (size_t i = 0; i < 4; ++i) {
        const NLControlExitView *x =
            i < 2 ? nl_control_exit_view(s->graphs[i + 1].fragment->exits, 0)
                  : nl_control_exit_view(s->graph->exits, i - 2);
        printf("exit%zu view=%d world=%d\n", i,
               !memcmp(x, &s->exit_views[i], sizeof(*x)),
               linear_world_matches(s->exits[i].world, &s->exits[i].storage,
                                    s->exits[i].copy, &s->exits[i].anchor));
    }
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
