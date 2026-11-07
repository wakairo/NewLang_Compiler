/* C280 read-only observation of genuine CHECK, never synthetic source facts. */
#define main p1227_preserved_cli_main
#include "main.c"
#undef main
#include <assert.h>

typedef struct {
    NLSource *source, *call_source;
    NLParser *parser, *call_parser;
    NLSyntaxTree *tree, *call_tree;
    NLSemanticContext *context;
    NLCheckedFragment *owned;
    const NLCheckedFragment *loop_owner;
    NLCheckedNodeId loop;
} Case;
static const NLCheckedFragment *seen[256];
static size_t seen_count, loops, producers;
static void inspect(Case *test, const NLCheckedFragment *f, bool early)
{
    if (!f)
        return;
    for (size_t i = 0; i < seen_count; ++i)
        if (seen[i] == f)
            return;
    assert(seen_count < 256);
    seen[seen_count++] = f;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        assert(n);
        if (n->producer.entry_proved) {
            assert(nl_checked_producer_valid(f, id));
            ++producers;
        }
        if (n->kind == NL_CHECKED_LOOP) {
            NLLinearBreakView view = {0};
            NLCheckStatus status = nl_linear_break_validate(f, id, &view);
            assert(status == NL_CHECK_OK && view.scalar_loop &&
                   view.changing_current);
            bool legacy_break = nl_checked_loop_break_provenance_valid(f, id);
            assert(n->argument_count == 1 && n->continue_edges == 1 &&
                   !legacy_break);
            assert(view.entry_world && view.post_world && view.root &&
                   view.child && view.incarnation);
            NLPureU8Call pure = {0};
            bool lineage =
                nl_h_custody_result_valid(view.iteration, view.incoming_call);
            bool contents =
                nl_pure_u8_contents(view.iteration, view.incoming_call, &pure);
            assert(lineage && contents && pure.body && pure.selection);
            const NLCheckedNodeView *choice =
                nl_checked_node_view(pure.body, pure.selection);
            const NLCheckedNodeView *body =
                nl_checked_node_view(pure.body, nl_checked_root(pure.body));
            assert(choice && body);
            bool old_lane = choice->normal_arms == 2 && !body->terminates &&
                            body->tail == pure.selection;
            printf("LOOP status=%d changing=1 scalar=1 legacy_direct_break=%d "
                   "root=%zu child=%zu incarnation=%zu index=%zu initial=%zu "
                   "limit=%zu step=%zu\n",
                   status, legacy_break, view.root, view.child,
                   view.incarnation, view.index, view.initial_phase, view.limit,
                   view.next_phase);
            printf("HELPER lineage=%d contents=%d normal_arms=%zu "
                   "body_returns=%zu selection=%zu body_tail=%zu "
                   "old_private_helper_lane=%d\n",
                   lineage, contents, choice->normal_arms,
                   pure.plan.body_returns, pure.selection, body->tail,
                   old_lane);
            assert(choice->normal_arms == (early ? 1u : 2u));
            assert((pure.plan.body_returns != 0) == early && old_lane != early);
            const NLCheckedNodeView *wrapper = nl_checked_node_view(
                view.iteration, nl_checked_root(view.iteration));
            const NLCheckedNodeView *block =
                wrapper
                    ? nl_checked_node_view(view.iteration, wrapper->initializer)
                    : NULL;
            assert(block);
            NLSymbolId incoming = 0, old = 0, copy = 0, current = 0,
                       sibling = 0;
            for (NLCheckedNodeId q = block->first_item;
                 q && q != view.continue_node;) {
                const NLCheckedNodeView *item =
                    nl_checked_node_view(view.iteration, q);
                const NLCheckedNodeView *init =
                    item ? nl_checked_node_view(view.iteration,
                                                item->initializer)
                         : NULL;
                assert(item && init);
                if (item->kind == NL_CHECKED_BINDING) {
                    if (init->kind == NL_CHECKED_REGISTERED_CALL)
                        incoming = item->symbol;
                    if (init->kind == NL_CHECKED_LOAN_HEADER)
                        old = item->symbol;
                    if (init->kind == NL_CHECKED_IDENTIFIER &&
                        init->symbol == old)
                        copy = item->symbol;
                    if (init->kind == NL_CHECKED_FIELD_READ) {
                        if (init->field.index == view.index)
                            current = item->symbol;
                        else
                            sibling = item->symbol;
                    }
                }
                q = item->next_item;
            }
            assert(incoming && old && copy && current && sibling);
            printf("ROLES incoming=%zu old=%zu old_copy=%zu current=%zu "
                   "sibling=%zu index=%zu\n",
                   incoming, old, copy, current, sibling, view.index);
            test->loop_owner = f;
            test->loop = id;
            ++loops;
        }
        inspect(test, nl_checked_call_body(f, id), early);
        if (n->kind == NL_CHECKED_IF)
            for (size_t a = 0; a < 2; ++a)
                inspect(test, nl_checked_if_arm(f, id, a), early);
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < n->item_count; ++a)
                inspect(test, nl_checked_match_arm(f, id, a), early);
        if (n->kind == NL_CHECKED_LOOP)
            inspect(test, nl_checked_loop_body(f, id), early);
    }
}
int main(int argc, char **argv)
{
    assert(argc > 1 && argc <= 25);
    setvbuf(stdout, NULL, _IONBF, 0);
    Case cases[24] = {{0}};
    for (size_t i = 0; i < (size_t)(argc - 1); ++i) {
        Case *t = &cases[i];
        NLParseDiagnostic p = {0};
        NLCheckDiagnostic d = {0};
        NLFunctionUnitDiagnostic u = {0};
        assert(nl_source_load(argv[i + 1], &t->source) == NL_SOURCE_OK);
        assert(nl_parser_create(t->source, &t->parser) == NL_PARSE_OK);
        assert(nl_parser_parse_function_unit(t->parser, &t->tree, &p) ==
               NL_PARSE_OK);
        assert(nl_semantic_create(&t->context) == NL_CHECK_OK);
        const NLSyntaxTree *input[] = {t->tree};
        NLCheckStatus registration =
            nl_semantic_register_function_unit(t->context, input, 1, &u);
        printf("CASE %s REGISTER=%d\n", argv[i + 1], registration);
        assert(registration == NL_CHECK_OK);
        assert(nl_source_create("main()", 6, "P1227 actual source-owned entry",
                                &t->call_source) == NL_SOURCE_OK);
        assert(nl_parser_create(t->call_source, &t->call_parser) ==
               NL_PARSE_OK);
        assert(nl_parser_parse_expression_fragment(
                   t->call_parser, &t->call_tree, &p) == NL_PARSE_OK);
        NLCheckStatus checked = nl_semantic_check_expression(
            t->context, t->call_tree, &t->owned, &d);
        printf("CHECK=%d owned=%d\n", checked, t->owned != NULL);
        assert(checked == NL_CHECK_OK && t->owned);
        bool custody = nl_h_custody_validate(t->owned),
             has_h = nl_h_origin_has_sites(t->owned),
             origin = nl_h_origin_validate(t->owned);
        printf("OWNED source_custody=%d has_H=%d H_origin=%d\n", custody, has_h,
               origin);
        assert(custody && has_h && origin);
        seen_count = loops = producers = 0;
        inspect(t, t->owned, true);
        assert(loops == 1);
        printf("COMPLETE source_observation=%zu loops=%zu producer_valid=%zu "
               "fragments=%zu\n",
               i + 1, loops, producers, seen_count);
    }
    for (size_t i = 0; i < (size_t)(argc - 1); ++i) {
        Case *t = &cases[i], *other = &cases[(i + 1) % (size_t)(argc - 1)];
        /* Every underlying pointer remains real/live. Only an unissued stack
         * descriptor is supplied; never destroy or publish that descriptor. */
        NLCheckedFragment shallow = *t->loop_owner;
        NLLinearBreakView bad = {0};
        bad.root = 987654;
        NLCheckStatus owner = nl_linear_break_validate(&shallow, t->loop, &bad);
        assert(owner != NL_CHECK_OK);
        shallow.context = other->loop_owner->context;
        NLCheckStatus foreign =
            nl_linear_break_validate(&shallow, t->loop, &bad);
        assert(foreign != NL_CHECK_OK);
        printf("REJECT unissued_owner=%d foreign_owner_world=%d "
               "output_root=%zu (owner guard may precede world guard)\n",
               owner, foreign, bad.root);
    }
    for (size_t i = 0; i < (size_t)(argc - 1); ++i) {
        Case *t = &cases[i];
        nl_checked_destroy(t->owned);
        nl_semantic_destroy(t->context);
        nl_syntax_tree_destroy(t->call_tree);
        nl_parser_destroy(t->call_parser);
        nl_source_destroy(t->call_source);
        nl_syntax_tree_destroy(t->tree);
        nl_parser_destroy(t->parser);
        nl_source_destroy(t->source);
    }
    puts("P1227 C283 all genuine owned/current checks and foreign-owner/world "
         "refusal PASS");
    return 0;
}
