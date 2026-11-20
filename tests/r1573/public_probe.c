#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/syntax.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

typedef struct {
    size_t division;
    size_t remainder;
    size_t nested_calls;
    bool invalid;
} R1573Evidence;

static bool span_equals(const NLSource *source, NLSourceSpan span,
                        const char *text)
{
    NLSourceView view;
    size_t length = strlen(text);
    return nl_source_span_valid(source, span) &&
           nl_source_view(source, span, &view) && view.length == length &&
           memcmp(view.bytes, text, length) == 0;
}

static bool authentic_main_ast(const NLSource *source, const NLSyntaxTree *tree,
                               bool mixed)
{
    const NLSyntaxView *root = nl_syntax_node_view(nl_syntax_tree_root(tree));
    size_t count = mixed ? 2u : 1u;
    if (root == NULL || root->kind != NL_SYNTAX_FUNCTION_UNIT ||
        root->data.function_unit.count != count) {
        return false;
    }
    size_t mains = 0;
    size_t helpers = 0;
    const NLSyntaxNode *fn_node = root->data.function_unit.declarations;
    for (size_t i = 0; i < count; ++i) {
        const NLSyntaxView *fn = nl_syntax_node_view(fn_node);
        if (fn == NULL || fn->kind != NL_SYNTAX_FUNCTION) {
            return false;
        }
        if (span_equals(source, fn->data.function.name, "main")) {
            const NLSyntaxView *result =
                nl_syntax_node_view(fn->data.function.result);
            if (fn->data.function.count != 0 || result == NULL ||
                result->kind != NL_SYNTAX_TYPE_NAME ||
                !span_equals(source, result->data.name, "unit") ||
                fn->data.function.body == NULL) {
                return false;
            }
            ++mains;
        } else if (span_equals(source, fn->data.function.name, "carve")) {
            if (!mixed || fn->data.function.count != 1 ||
                fn->data.function.body == NULL) {
                return false;
            }
            ++helpers;
        } else {
            return false;
        }
        fn_node = nl_syntax_next_argument(fn_node);
    }
    return mains == 1 && helpers == (mixed ? 1u : 0u);
}

static void inspect(const NLCheckedFragment *fragment, size_t depth,
                    R1573Evidence *result)
{
    size_t total = nl_checked_node_count(fragment);
    const NLSource *source = nl_checked_source(fragment);
    if (depth > 4 || total == 0) {
        result->invalid = true;
        return;
    }
    for (size_t i = 1; i <= total; ++i) {
        NLCheckedNodeId id = (NLCheckedNodeId)i;
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (node == NULL) {
            continue;
        }
        if (node->kind == NL_CHECKED_USIZE_DIVISION) {
            NLUsizeDivisionView view = {0};
            bool issued = nl_checked_usize_division_view(fragment, id, &view);
            bool valid = issued &&
                         nl_checked_usize_division_valid(fragment, id, NULL) &&
                         nl_checked_usize_division_valid(fragment, id, &view);
            if (!valid || source == NULL ||
                !span_equals(source, view.operator_span, "/")) {
                result->invalid = true;
                printf("R1573_DIV_NOT_AUTHENTIC|depth=%zu\n", depth);
                continue;
            }
            ++result->division;
            printf("R1573_DIV|depth=%zu|known=%d,%d|n=%zu|k=%zu|"
                   "q=%zu|target_max=%zu|node=%zu\n",
                   depth, (int)view.numerator_known, (int)view.quotient_known,
                   view.numbers[0], view.numbers[1], view.quotient,
                   view.target_max, (size_t)id);
        } else if (node->kind == NL_CHECKED_USIZE_REMAINDER) {
            NLUsizeRemainderView view = {0};
            bool issued = nl_checked_usize_remainder_view(fragment, id, &view);
            bool valid = issued &&
                         nl_checked_usize_remainder_valid(fragment, id, NULL) &&
                         nl_checked_usize_remainder_valid(fragment, id, &view);
            if (!valid || source == NULL ||
                !span_equals(source, view.operator_span, "%")) {
                result->invalid = true;
                printf("R1573_REMAINDER_NOT_AUTHENTIC|depth=%zu\n", depth);
                continue;
            }
            ++result->remainder;
            printf("R1573_REMAINDER|depth=%zu|known=%d,%d|n=%zu|k=%zu|"
                   "r=%zu|target_max=%zu|node=%zu\n",
                   depth, (int)view.numerator_known, (int)view.remainder_known,
                   view.numbers[0], view.numbers[1], view.remainder,
                   view.target_max, (size_t)id);
        } else if (node->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
            printf("R1573_CALL|depth=%zu|function=%zu|body_backed=%d\n", depth,
                   node->function, (int)(node->body_backed && body != NULL));
            if (!node->body_backed || body == NULL) {
                result->invalid = true;
                continue;
            }
            if (depth > 0) {
                ++result->nested_calls;
            }
            inspect(body, depth + 1, result);
        }
    }
}

int main(int argc, char **argv)
{
    NLSource *source = NULL;
    NLSource *observer = NULL;
    NLParser *parser = NULL;
    NLParser *observer_parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSyntaxTree *call = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    NLParseDiagnostic parse_diagnostic = {0};
    NLCheckDiagnostic check_diagnostic = {0};
    NLFunctionUnitDiagnostic unit_diagnostic = {0};
    R1573Evidence evidence = {0};
    const char expression[] = "main()";
    int status = 1;
    if (argc != 3) {
        fprintf(stderr, "R1573 public probe: CASE SOURCE\n");
        return 2;
    }
    bool mixed = strcmp(argv[1], "P2") == 0;
    if ((!mixed && strcmp(argv[1], "P1") != 0) ||
        nl_source_load(argv[2], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(parser, &unit, &parse_diagnostic) !=
            NL_PARSE_OK) {
        fprintf(stderr, "R1573_SOURCE_OR_PARSER_FAILED\n");
        goto cleanup;
    }
    if (!authentic_main_ast(source, unit, mixed)) {
        fprintf(stderr, "R1573_MAIN_AST_UNMATCHED\n");
        goto cleanup;
    }
    if (nl_semantic_create(&context) != NL_CHECK_OK) {
        fprintf(stderr, "R1573_PUBLIC_CONTEXT_ERROR\n");
        goto cleanup;
    }
    const NLSyntaxTree *inputs[] = {unit};
    NLCheckStatus register_status = nl_semantic_register_function_unit(
        context, inputs, 1, &unit_diagnostic);
    printf("R1573_REAL_UNIT_REGISTER|case=%s|status=%d\n", argv[1],
           (int)register_status);
    if (register_status != NL_CHECK_OK) {
        fprintf(stderr, "R1573_REAL_UNIT_REGISTRATION_FAILED\n");
        goto cleanup;
    }
    if (nl_source_create(expression, sizeof(expression) - 1,
                         "R1573_observer_expression",
                         &observer) != NL_SOURCE_OK ||
        nl_parser_create(observer, &observer_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(observer_parser, &call,
                                            &parse_diagnostic) != NL_PARSE_OK) {
        fprintf(stderr, "R1573_OBSERVER_EXPRESSION_PARSE_FAILED\n");
        goto cleanup;
    }
    NLCheckStatus checked_status = nl_semantic_check_expression(
        context, call, &checked, &check_diagnostic);
    printf("R1573_OBSERVER_CHECK|status=%d\n", (int)checked_status);
    if (checked_status != NL_CHECK_OK || checked == NULL) {
        fprintf(stderr, "R1573_MAIN_OBSERVER_NOT_OBSERVABLE\n");
        goto cleanup;
    }
    NLCheckedNodeId root_id = nl_checked_root(checked);
    const NLCheckedNodeView *root = nl_checked_node_view(checked, root_id);
    if (root == NULL || root->kind != NL_CHECKED_REGISTERED_CALL ||
        !root->body_backed || nl_checked_call_body(checked, root_id) == NULL) {
        fprintf(stderr, "R1573_MAIN_CHECKED_BODY_NOT_OBSERVABLE\n");
        goto cleanup;
    }
    inspect(checked, 0, &evidence);
    printf("R1573_OBSERVED|div=%zu|rem=%zu|calls=%zu|invalid=%d\n",
           evidence.division, evidence.remainder, evidence.nested_calls,
           (int)evidence.invalid);
    printf("R1573_NOT_OBSERVABLE|definition_time_unknown|"
           "full_formal_result_world_custody|executable_entry_link\n");
    if (evidence.invalid || evidence.division != (mixed ? 3u : 1u) ||
        evidence.remainder != 1u ||
        evidence.nested_calls != (mixed ? 2u : 0u)) {
        fprintf(stderr, "R1573_PUBLIC_EVENT_CHAIN_UNATTESTED\n");
        goto cleanup;
    }
    printf("R1573_PUBLIC_ARITHMETIC_EVENTS_VERIFIED\n");
    status = 0;
cleanup:
    nl_checked_destroy(checked);
    nl_syntax_tree_destroy(call);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(observer_parser);
    nl_parser_destroy(parser);
    nl_semantic_destroy(context);
    nl_source_destroy(observer);
    nl_source_destroy(source);
    return status;
}
