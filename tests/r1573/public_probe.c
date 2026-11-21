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
    int role;
    NLCheckedKind kind;
    const char *whole, *left, *right;
    size_t n, k, answer;
} R1573Expected;

/* Independent frozen FIRST 7036152423: never calibrated to P. */
static const R1573Expected math_oracle[] = {
    {0, NL_CHECKED_USIZE_DIVISION, "kept / usize(7)", "kept", "usize(7)", 93, 7,
     13},
    {0, NL_CHECKED_USIZE_REMAINDER, "cargo % usize(7)", "cargo", "usize(7)", 93,
     7, 2},
    {1, NL_CHECKED_USIZE_DIVISION, "pack / usize(4)", "pack", "usize(4)", 86, 4,
     21},
    {1, NL_CHECKED_USIZE_REMAINDER, "second % usize(6)", "second", "usize(6)",
     21, 6, 3},
    {2, NL_CHECKED_USIZE_DIVISION, "before / usize(4)", "before", "usize(4)",
     21, 4, 5},
    {3, NL_CHECKED_USIZE_DIVISION, "before / usize(4)", "before", "usize(4)",
     86, 4, 21},
};

typedef struct {
    int role;
    NLCheckedKind kind;
    const char *whole;
    size_t answer;
} R1573ScalarExpected;

static const R1573ScalarExpected scalar_oracle[] = {
    {0, NL_CHECKED_USIZE_SUBTRACTION, "quotient - remainder", 11},
    {0, NL_CHECKED_USIZE_ADDITION, "reduced + usize(4)", 15},
    {1, NL_CHECKED_USIZE_ADDITION, "residue + first", 8},
    {1, NL_CHECKED_USIZE_SUBTRACTION, "back - usize(2)", 6},
};

typedef struct {
    bool seen[6], n_known[6], result_known[6];
    bool scalar_seen[4], scalar_known[4];
    bool call_seen[2], actual_known[2], returned_known[2];
    bool function_set;
    size_t function, division, remainder, nested_calls;
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

static bool identical_source(const NLSource *actual, const NLSource *expected)
{
    NLSourceView a, b;
    if (actual == NULL || expected == NULL ||
        nl_source_length(actual) != nl_source_length(expected)) {
        return false;
    }
    size_t n = nl_source_length(expected);
    NLSourceSpan whole = {0, n};
    return nl_source_view(actual, whole, &a) &&
           nl_source_view(expected, whole, &b) && a.length == n &&
           b.length == n && (n == 0 || memcmp(a.bytes, b.bytes, n) == 0);
}

static bool public_known_usize(const NLCheckedFragment *fragment,
                               const NLCheckedNodeView *node, size_t *value)
{
    if (node == NULL || value == NULL) {
        return false;
    }
    const NLSemanticContext *world = nl_checked_context(fragment);
    NLTypeId usize = nl_semantic_core_type(world, NL_TYPE_USIZE);
    if (node->has_scalar_result && node->scalar_result.type == usize &&
        node->scalar_result.known) {
        *value = node->scalar_result.value;
        return true;
    }
    if (node->result_count == 1 && node->results[0].type == usize &&
        node->results[0].value != 0) {
        NLSemanticValueView view;
        if (nl_semantic_value_view(world, node->results[0].value, &view) &&
            view.type == usize && view.scalar_known) {
            *value = view.scalar_value;
            return true;
        }
    }
    return false;
}

static void assert_math(const NLCheckedFragment *fragment,
                        const NLSource *source, int role, NLCheckedNodeId id,
                        NLCheckedKind kind, R1573Evidence *out)
{
    NLSourceSpan expression = {0}, operator_span = {0};
    NLSourceSpan operands[2] = {{0}, {0}};
    size_t numerator = 0, divisor = 0, answer = 0, maximum = 0;
    bool n_known = false, result_known = false, valid = false;
    if (!identical_source(nl_checked_source(fragment), source)) {
        printf("R1573_STOP|source_owned_body_cannot_match_original\n");
        out->invalid = true;
        return;
    }
    if (kind == NL_CHECKED_USIZE_DIVISION) {
        NLUsizeDivisionView view = {0};
        valid = nl_checked_usize_division_view(fragment, id, &view) &&
                view.origin == fragment && view.node == id &&
                nl_checked_usize_division_valid(fragment, id, NULL) &&
                nl_checked_usize_division_valid(fragment, id, &view);
        if (valid) {
            expression = view.span;
            operator_span = view.operator_span;
            operands[0] = view.operand_spans[0];
            operands[1] = view.operand_spans[1];
            numerator = view.numbers[0];
            divisor = view.numbers[1];
            answer = view.quotient;
            maximum = view.target_max;
            n_known = view.numerator_known;
            result_known = view.quotient_known;
        }
    } else {
        NLUsizeRemainderView view = {0};
        valid = nl_checked_usize_remainder_view(fragment, id, &view) &&
                view.origin == fragment && view.node == id &&
                identical_source(view.source, source) &&
                nl_checked_usize_remainder_valid(fragment, id, NULL) &&
                nl_checked_usize_remainder_valid(fragment, id, &view);
        if (valid) {
            expression = view.span;
            operator_span = view.operator_span;
            operands[0] = view.operand_spans[0];
            operands[1] = view.operand_spans[1];
            numerator = view.numbers[0];
            divisor = view.numbers[1];
            answer = view.remainder;
            maximum = view.target_max;
            n_known = view.numerator_known;
            result_known = view.remainder_known;
        }
    }
    if (!valid) {
        printf("R1573_STOP|genuine_source_issuer_validator_failed\n");
        out->invalid = true;
        return;
    }
    for (size_t j = 0; j < 6; ++j) {
        const R1573Expected *e = &math_oracle[j];
        if (e->role != role || e->kind != kind ||
            !span_equals(source, expression, e->whole)) {
            continue;
        }
        bool syntax =
            span_equals(source, operands[0], e->left) &&
            span_equals(source, operands[1], e->right) &&
            span_equals(source, operator_span,
                        kind == NL_CHECKED_USIZE_DIVISION ? "/" : "%");
        size_t premise = role == 0 ? 93u : 86u;
        bool target = maximum >= premise && divisor == e->k && divisor != 0 &&
                      divisor <= maximum;
        bool numerics = (!n_known || numerator == e->n) &&
                        (!result_known || answer == e->answer);
        bool placeholders =
            (n_known || numerator == 0) && (result_known || answer == 0);
        if (out->seen[j] || !syntax || !target || !numerics || !placeholders) {
            printf("R1573_STOP|FROZEN_MATH_MISMATCH|index=%zu|role=%d|"
                   "n=%zu|k=%zu|result=%zu|max=%zu|known=%d,%d|"
                   "source=%d|target=%d\n",
                   j, role, numerator, divisor, answer, maximum, (int)n_known,
                   (int)result_known, (int)syntax, (int)target);
            out->invalid = true;
            return;
        }
        out->seen[j] = true;
        out->n_known[j] = n_known;
        out->result_known[j] = result_known;
        printf("R1573_FROZEN_MATH_ASSERT|role=%d|expression=%s|"
               "n_known=%d|n=%zu|k=%zu|answer_known=%d|answer=%zu|"
               "target_max=%zu\n",
               role, e->whole, (int)n_known, numerator, divisor,
               (int)result_known, answer, maximum);
        if (!n_known || !result_known) {
            printf("R1573_NOT_OBSERVABLE|role=%d|expression=%s|"
                   "unknown_numerator=%d|unknown_result=%d|"
                   "zero_placeholders_not_known_zero\n",
                   role, e->whole, (int)!n_known, (int)!result_known);
        }
        return;
    }
    printf("R1573_STOP|unrecognized_source_operation|role=%d|kind=%d\n", role,
           (int)kind);
    out->invalid = true;
}

static void observe_scalar(const NLCheckedFragment *fragment,
                           const NLSource *source, int role,
                           const NLCheckedNodeView *node, R1573Evidence *out)
{
    for (size_t i = 0; i < 4; ++i) {
        const R1573ScalarExpected *e = &scalar_oracle[i];
        if (e->role != role || e->kind != node->kind ||
            !span_equals(source, node->span, e->whole)) {
            continue;
        }
        size_t result = 0;
        bool known = public_known_usize(fragment, node, &result);
        if (out->scalar_seen[i] || (known && result != e->answer)) {
            printf("R1573_STOP|plus_minus_oracle_disagrees|expr=%s|"
                   "known=%d|result=%zu\n",
                   e->whole, (int)known, result);
            out->invalid = true;
        }
        out->scalar_seen[i] = true;
        out->scalar_known[i] = known;
        printf("R1573_PUBLIC_PLUS_MINUS|role=%d|expr=%s|"
               "Known=%d|value=%zu\n",
               role, e->whole, (int)known, result);
        if (!known) {
            printf("R1573_NOT_OBSERVABLE|plus_minus_Known_value|%s\n",
                   e->whole);
        }
        return;
    }
}

static void inspect(const NLCheckedFragment *fragment, const NLSource *source,
                    int role, size_t depth, R1573Evidence *out)
{
    if (fragment == NULL || depth > 3 ||
        !identical_source(nl_checked_source(fragment), source)) {
        printf("R1573_STOP|checked_callee_source_not_original\n");
        out->invalid = true;
        return;
    }
    size_t n = nl_checked_node_count(fragment);
    if (n == 0) {
        out->invalid = true;
        return;
    }
    for (size_t index = 1; index <= n; ++index) {
        NLCheckedNodeId id = (NLCheckedNodeId)index;
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (node == NULL) {
            continue;
        }
        if (node->kind == NL_CHECKED_USIZE_DIVISION ||
            node->kind == NL_CHECKED_USIZE_REMAINDER) {
            assert_math(fragment, source, role, id, node->kind, out);
            if (node->kind == NL_CHECKED_USIZE_DIVISION) {
                ++out->division;
            } else {
                ++out->remainder;
            }
        } else if (node->kind == NL_CHECKED_USIZE_ADDITION ||
                   node->kind == NL_CHECKED_USIZE_SUBTRACTION) {
            observe_scalar(fragment, source, role, node, out);
        } else if (node->kind == NL_CHECKED_REGISTERED_CALL) {
            if (role != 1 || depth != 1) {
                printf("R1573_STOP|unexpected_call_context\n");
                out->invalid = true;
                continue;
            }
            size_t which;
            if (span_equals(source, node->span, "carve(pre)")) {
                which = 0;
            } else if (span_equals(source, node->span, "carve(pack)")) {
                which = 1;
            } else {
                printf("R1573_STOP|unrecognized_helper_call_site\n");
                out->invalid = true;
                continue;
            }
            const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
            if (out->call_seen[which] || body == NULL || !node->body_backed) {
                out->invalid = true;
                continue;
            }
            out->call_seen[which] = true;
            ++out->nested_calls;
            if (!out->function_set) {
                out->function = node->function;
                out->function_set = true;
            } else if (out->function != node->function) {
                printf(
                    "R1573_STOP|two_calls_resolved_to_different_functions\n");
                out->invalid = true;
            }
            size_t actual = 0, returned = 0;
            const NLCheckedNodeView *arg =
                nl_checked_node_view(fragment, node->first_argument);
            bool actual_known = public_known_usize(fragment, arg, &actual);
            bool returned_known = public_known_usize(fragment, node, &returned);
            out->actual_known[which] = actual_known;
            out->returned_known[which] = returned_known;
            printf("R1573_ACTUAL_CALL|site=%s|function_id=%zu|"
                   "actual_Known=%d|actual=%zu|result_Known=%d|result=%zu\n",
                   which == 0 ? "carve(pre)" : "carve(pack)", node->function,
                   (int)actual_known, actual, (int)returned_known, returned);
            if ((actual_known && actual != (which == 0 ? 21u : 86u)) ||
                (returned_known && returned != (which == 0 ? 5u : 21u))) {
                printf("R1573_STOP|actual_or_callee_result_mismatch\n");
                out->invalid = true;
            }
            if (!actual_known || !returned_known) {
                printf("R1573_NOT_OBSERVABLE|call=%s|actual=%d|result=%d|"
                       "formal_world_custody_not_certified\n",
                       which == 0 ? "carve(pre)" : "carve(pack)",
                       (int)!actual_known, (int)!returned_known);
            }
            inspect(body, source, which == 0 ? 2 : 3, depth + 1, out);
        }
    }
}

static bool finish_evidence(const R1573Evidence *out, bool mixed)
{
    size_t numeric_known = 0, numeric_unknown = 0, observed = 0;
    bool valid = !out->invalid;
    for (size_t i = 0; i < 6; ++i) {
        bool required = mixed ? i >= 2 : i < 2;
        if (required && !out->seen[i]) {
            printf("R1573_STOP|frozen_operation_not_seen|index=%zu\n", i);
            valid = false;
        }
        if (!required && out->seen[i])
            valid = false;
        if (required && out->seen[i]) {
            ++observed;
            if (out->n_known[i] && out->result_known[i]) {
                ++numeric_known;
            } else {
                ++numeric_unknown;
            }
        }
    }
    for (size_t i = 0; i < 4; ++i) {
        bool required = mixed ? i >= 2 : i < 2;
        if (required && !out->scalar_seen[i]) {
            printf("R1573_NOT_OBSERVABLE|public_plus_minus_node=%s\n",
                   scalar_oracle[i].whole);
        }
    }
    if (mixed
            ? (!out->call_seen[0] || !out->call_seen[1] || out->division != 3 ||
               out->remainder != 1 || out->nested_calls != 2)
            : (out->division != 1 || out->remainder != 1 ||
               out->nested_calls != 0)) {
        valid = false;
    }
    printf("R1573_FROZEN_NUMERIC_LEDGER|case=%s|arithmetic=%zu|"
           "Known_value_pairs=%zu|Unknown_value_pairs=%zu|"
           "formal_result_world=NOT_OBSERVABLE|"
           "final_copy_binding=NOT_OBSERVABLE_without_public_value\n",
           mixed ? "P2" : "P1", observed, numeric_known, numeric_unknown);
    return valid;
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
    const NLCheckedFragment *real_main_body =
        nl_checked_call_body(checked, root_id);
    inspect(real_main_body, source, mixed ? 1 : 0, 1, &evidence);
    printf("R1573_NOT_OBSERVABLE|definition_time_Unknown|"
           "executable_main_vs_observer_call|formal_result_world_custody\n");
    if (!finish_evidence(&evidence, mixed)) {
        fprintf(stderr, "R1573_STOP|FROZEN_PUBLIC_NUMERIC_ORIGIN_ASSERT\n");
        goto cleanup;
    }
    printf("R1573_PUBLIC_KNOWN_MATH_CHECK_COMPLETE\n");
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
