/* Read-only evidence observation on this exact CHECK execution, no minting. */
#define main p1214_original_cli_main
#include "main.c"
#undef main
#include <assert.h>

static const NLCheckedFragment *visited[256];
static size_t seen, producers, loops;
static void inspect(const NLCheckedFragment *f)
{
    if (!f)
        return;
    for (size_t i = 0; i < seen; ++i)
        if (visited[i] == f)
            return;
    assert(seen < 256);
    visited[seen++] = f;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        assert(n);
        if (n->producer.entry_proved) {
            bool valid = nl_checked_producer_valid(f, id);
            printf("PRODUCER source=%zu issued=%d\n", n->span.start_byte,
                   valid);
            assert(valid);
            ++producers;
        }
        if (n->kind == NL_CHECKED_LOOP) {
            NLLinearBreakView p = {0};
            NLCheckStatus status = nl_linear_break_validate(f, id, &p);
            bool break_source = nl_checked_loop_break_provenance_valid(f, id);
            printf("LOOP node=%zu public_break_source=%d "
                   "private_current_status=%d changing=%d scalar=%d root=%zu "
                   "child=%zu field=%zu initial=%zu limit=%zu step=%zu\n",
                   id, break_source, status, p.changing_current, p.scalar_loop,
                   p.root, p.child, p.index, p.initial_phase, p.limit,
                   p.next_phase);
            assert(status == NL_CHECK_OK && p.scalar_loop);
            if (!p.changing_current) {
                ++loops;
                continue;
            }
            assert(p.incoming_call && p.entry_world && p.post_world);
            NLPureU8Call pure = {0};
            bool lineage =
                nl_h_custody_result_valid(p.iteration, p.incoming_call);
            bool contents =
                nl_pure_u8_contents(p.iteration, p.incoming_call, &pure);
            printf("HELPER source_result_lineage=%d pure_current_contents=%d "
                   "normal_arm=%zu returns=%zu\n",
                   lineage, contents, pure.plan.normal_arm,
                   pure.plan.body_returns);
            assert(lineage && contents);
            ++loops;
            const NLCheckedNodeView *wrap =
                nl_checked_node_view(p.iteration, nl_checked_root(p.iteration));
            const NLCheckedNodeView *block =
                nl_checked_node_view(p.iteration, wrap->initializer);
            size_t incoming = 0, old = 0, old_copy = 0, changed = 0,
                   sibling = 0;
            for (NLCheckedNodeId q = block->first_item;
                 q && q != p.continue_node;) {
                const NLCheckedNodeView *binding =
                    nl_checked_node_view(p.iteration, q);
                const NLCheckedNodeView *init =
                    nl_checked_node_view(p.iteration, binding->initializer);
                assert(init);
                if (binding->kind == NL_CHECKED_BINDING) {
                    if (init->kind == NL_CHECKED_REGISTERED_CALL)
                        incoming = binding->symbol;
                    if (init->kind == NL_CHECKED_LOAN_HEADER)
                        old = binding->symbol;
                    if (init->kind == NL_CHECKED_IDENTIFIER &&
                        init->symbol == old)
                        old_copy = binding->symbol;
                    if (init->kind == NL_CHECKED_FIELD_READ) {
                        if (init->field.index == p.index)
                            changed = binding->symbol;
                        else
                            sibling = binding->symbol;
                    }
                }
                q = binding->next_item;
            }
            assert(incoming && old && old_copy && changed && sibling);
            printf("ROLES incoming=%zu old=%zu old_copy=%zu current=%zu "
                   "sibling=%zu index=%zu\n",
                   incoming, old, old_copy, changed, sibling, p.index);
        }
        inspect(nl_checked_call_body(f, id));
        if (n->kind == NL_CHECKED_IF)
            for (size_t arm = 0; arm < 2; ++arm)
                inspect(nl_checked_if_arm(f, id, arm));
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t arm = 0; arm < n->item_count; ++arm)
                inspect(nl_checked_match_arm(f, id, arm));
        if (n->kind == NL_CHECKED_LOOP)
            inspect(nl_checked_loop_body(f, id));
    }
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *tree = NULL, *entry_tree = NULL;
    NLSemanticContext *world = NULL;
    NLCheckedFragment *entry = NULL;
    NLParseDiagnostic parse = {0};
    NLCheckDiagnostic check = {0};
    assert(nl_source_load(argv[1], &source) == NL_SOURCE_OK);
    assert(nl_parser_create(source, &parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(parser, &tree, &parse) == NL_PARSE_OK);
    assert(nl_semantic_create(&world) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {tree};
    NLCheckStatus registration =
        nl_semantic_register_function_unit(world, units, 1, NULL);
    printf("REGISTER=%d\n", registration);
    assert(registration == NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "P1214 actual entry", &entry_source) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(entry_source, &entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(entry_parser, &entry_tree,
                                               &parse) == NL_PARSE_OK);
    NLCheckStatus checked =
        nl_semantic_check_expression(world, entry_tree, &entry, &check);
    printf("CHECK=%d\n", checked);
    assert(checked == NL_CHECK_OK);
    bool custody = nl_h_custody_validate(entry),
         has_h = nl_h_origin_has_sites(entry);
    bool h_valid = !has_h || nl_h_origin_validate(entry);
    printf("OWNED root_custody=%d has_H=%d H_origin=%d\n", custody, has_h,
           h_valid);
    assert(custody && h_valid);
    inspect(entry);
    assert(loops == 1);
    printf("COMPLETE fragments=%zu loops=%zu producers=%zu\n", seen, loops,
           producers);
    nl_checked_destroy(entry);
    nl_semantic_destroy(world);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return 0;
}
