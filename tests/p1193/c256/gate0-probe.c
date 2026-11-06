/* Gate0 observations only. Never modify a CHECK graph or issue a receipt. */
#define main p1193_unused_cli_main
#include "main.c"
#undef main
#include <assert.h>

static bool p1193_bad_scalar;

static void inspect_unit(const NLSyntaxTree *unit, const char *stage,
                         const char *expression_text, size_t expected)
{
    NLSemanticContext *world = NULL;
    NLCheckedFragment *root = NULL;
    NLSource *entry = NULL;
    NLParser *entry_parser = NULL;
    NLSyntaxTree *entry_tree = NULL;
    NLFunctionUnitDiagnostic diagnostic = {0};
    assert(nl_semantic_create(&world) == NL_CHECK_OK);
    NLSemanticSnapshot before = {0}, after = {0};
    assert(nl_semantic_snapshot(world, &before));
    const NLSyntaxTree *inputs[] = {unit};
    const NLCheckStatus registered =
        nl_semantic_register_function_unit(world, inputs, 1, &diagnostic);
    assert(nl_semantic_snapshot(world, &after));
    printf("P1193 REGISTER stage=%s status=%d code=%s span=%zu:%zu "
           "functions=%zu:%zu values=%zu:%zu places=%zu:%zu\n",
           stage, registered,
           diagnostic.diagnostic.diagnostic.code
               ? diagnostic.diagnostic.diagnostic.code
               : "none",
           diagnostic.diagnostic.span.start_byte,
           diagnostic.diagnostic.span.end_byte, before.functions,
           after.functions, before.values, after.values, before.places,
           after.places);
    if (registered != NL_CHECK_OK) {
        assert(before.functions == after.functions &&
               before.types == after.types && before.values == after.values &&
               before.bindings == after.bindings &&
               before.places == after.places && before.scopes == after.scopes);
        printf("P1193 STOP-LAYER stage=%s ownership=NOT_ISSUED rollback=YES\n",
               stage);
        goto done;
    }
    assert(nl_source_create(expression_text, strlen(expression_text),
                            "P1193 genuine invocation",
                            &entry) == NL_SOURCE_OK);
    assert(nl_parser_create(entry, &entry_parser) == NL_PARSE_OK);
    const NLParseStatus parsed =
        !strcmp(stage, "full")
            ? nl_parser_parse_expression_fragment(entry_parser, &entry_tree,
                                                  NULL)
            : nl_parser_parse_source_fragment(entry_parser, &entry_tree, NULL);
    assert(parsed == NL_PARSE_OK);
    NLCheckDiagnostic check = {0};
    const NLCheckStatus checked =
        !strcmp(stage, "full")
            ? nl_semantic_check_expression(world, entry_tree, &root, &check)
            : nl_semantic_check_source_fragment(world, entry_tree, &root, &check);
    printf("P1193 CHECK stage=%s status=%d code=%s output_null=%d\n", stage,
           checked, check.diagnostic.code ? check.diagnostic.code : "none",
           root == NULL);
    if (checked != NL_CHECK_OK)
        goto done;
    const NLCheckedNodeId call_id = nl_checked_root(root);
    const NLCheckedNodeView *call = nl_checked_node_view(root, call_id);
    const NLCheckedFragment *body = nl_checked_call_body(root, call_id);
    const bool owned = nl_h_custody_validate(root);
    const bool sealed = nl_h_custody_result_valid(root, call_id);
    assert(call && body && owned);
    const bool unit_call = !strcmp(stage, "full");
    /* The numeric-result seal is issued only for a numeric predicate call.
     * The main UNIT call instead proves its real owned body and loop below. */
    printf("P1193 AUTH stage=%s owned=%d body_GET=%d numeric_seal=%d "
           "unit_call=%d\n", stage, owned, body != NULL, sealed, unit_call);
    if (unit_call)
        assert(call->kind == NL_CHECKED_REGISTERED_CALL && !call->terminates &&
               call->type == nl_semantic_unit_type(world) &&
               call->result_count == 0 && nl_checked_context(body) == world);
    else
        assert(sealed);
    const NLCheckedNodeView *block =
        nl_checked_node_view(body, nl_checked_root(body));
    assert(block);
    NLPureU8Call pure;
    const bool predicate = nl_pure_u8_call(root, call_id, &pure);
    NLSemanticValueView result = {0};
    if (call->result_count)
        assert(nl_semantic_value_view(world, call->results[0].value, &result));
    printf("P1193 OWNED stage=%s public_body_GET=1 graph_current=%d "
           "result_seal=%d same_context=%d call_terminates=%d call_results=%zu "
           "known=%d value=%zu pure_consumer=%d body_terminates=%d "
           "body_results=%zu body_return_type=%zu exits=%zu\n",
           stage, owned, sealed, nl_checked_context(body) == world,
           call->terminates, call->result_count, result.scalar_known,
           result.scalar_value, predicate, block->terminates,
           block->result_count, block->returned.type,
           nl_control_exits_count(nl_checked_control_exits(body)));
    if (expected != SIZE_MAX && call->result_count == 1 &&
        result.scalar_known && result.scalar_value != expected) {
        fprintf(stderr,
                "P1193 UNSAFE known result stage=%s expected=%zu actual=%zu\n",
                stage, expected, result.scalar_value);
        p1193_bad_scalar = true;
        goto done;
    }
    for (size_t i = 0; i < nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, i + 1);
        if (node->kind == NL_CHECKED_IF) {
            printf("P1193 IF stage=%s node=%zu normal_arms=%zu terminates=%d\n",
                   stage, i + 1, node->normal_arms, node->terminates);
            for (size_t a = 0; a < 2; ++a) {
                const NLCheckedFragment *arm =
                    nl_checked_if_arm(body, i + 1, a);
                const NLCheckedNodeView *view =
                    arm ? nl_checked_node_view(arm, nl_checked_root(arm))
                        : NULL;
                assert(view);
                printf(
                    "P1193 ARM stage=%s arm=%zu terminates=%d results=%zu "
                    "return_type=%zu exits=%zu target_is_callee=%d\n",
                    stage, a, view->terminates, view->result_count,
                    view->returned.type,
                    nl_control_exits_count(nl_checked_control_exits(arm)),
                    nl_control_exits_only_return(nl_checked_control_exits(arm),
                                                 body->function_target));
            }
        } else if (node->kind == NL_CHECKED_LOOP) {
            NLLinearBreakView loop;
            const NLCheckStatus status =
                nl_linear_break_validate(body, i + 1, &loop);
            printf("P1193 LOOP stage=%s current_validate=%d\n", stage, status);
            assert(status == NL_CHECK_OK);
        }
    }
done:
    nl_checked_destroy(root);
    nl_semantic_destroy(world);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry);
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    assert(argc == 6);
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    assert(nl_source_load(argv[1], &source) == NL_SOURCE_OK);
    assert(nl_parser_create(source, &parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    inspect_unit(tree, "full", "main()", SIZE_MAX);
    const NLSyntaxView *unit = nl_syntax_node_view(nl_syntax_tree_root(tree));
    assert(unit && unit->kind == NL_SYNTAX_FUNCTION_UNIT);
    const NLSyntaxView *helper = NULL;
    NLSourceView name = {0}, declaration = {0};
    for (const NLSyntaxNode *n = unit->data.function_unit.declarations; n;
         n = nl_syntax_next_argument(n)) {
        const NLSyntaxView *node = nl_syntax_node_view(n);
        if (node->kind != NL_SYNTAX_FUNCTION)
            continue;
        NLSourceView candidate;
        assert(nl_source_view(source, node->data.function.name, &candidate));
        if (candidate.length == 4 && !memcmp(candidate.bytes, "main", 4))
            continue;
        assert(!helper);
        helper = node;
        name = candidate;
    }
    assert(helper && name.length < 64);
    assert(nl_source_view(source, helper->span, &declaration));
    printf("P1193 EXACT-HELPER bytes=%zu full_span=%zu:%zu\n",
           declaration.length, helper->span.start_byte, helper->span.end_byte);
    NLSource *helper_source = NULL;
    NLParser *helper_parser = NULL;
    NLSyntaxTree *helper_unit = NULL;
    assert(nl_source_create((const char *)declaration.bytes, declaration.length,
                            "P1193 exact unchanged helper declaration",
                            &helper_source) == NL_SOURCE_OK);
    assert(nl_parser_create(helper_source, &helper_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(helper_parser, &helper_unit, NULL) ==
           NL_PARSE_OK);
    for (size_t i = 0; i < 2; ++i) {
        char expression[128];
        const size_t actual = (size_t)strtoul(argv[2 + 2 * i], NULL, 10);
        snprintf(expression, sizeof(expression), "%.*s(usize(%zu))",
                 (int)name.length, (const char *)name.bytes, actual);
        const char *expected_text = argv[3 + 2 * i];
        const size_t expected = !strcmp(expected_text, "-")
                                    ? SIZE_MAX
                                    : (size_t)strtoul(expected_text, NULL, 10);
        char stage[64];
        snprintf(stage, sizeof(stage), "helper%zu", actual);
        inspect_unit(helper_unit, stage, expression, expected);
        if (p1193_bad_scalar)
            break;
    }
    nl_syntax_tree_destroy(helper_unit);
    nl_parser_destroy(helper_parser);
    nl_source_destroy(helper_source);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return p1193_bad_scalar ? 67 : 0;
}
