#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/usize_addition.h"
#include "newlang/usize_division.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    size_t division_events;
    size_t authenticated_quotients;
    size_t authenticated_sums;
} R1546Audit;

static int inspect(const NLCheckedFragment *fragment, unsigned depth,
                   R1546Audit *audit) {
    if (fragment == NULL || depth > 12) {
        fprintf(stderr, "R1546 STOP: missing body or recursive body depth\n");
        return 0;
    }
    size_t count = nl_checked_node_count(fragment);
    if (count == 0 || count > 4096) {
        fprintf(stderr, "R1546 STOP: unexpected checked node count %zu\n",
                count);
        return 0;
    }
    for (NLCheckedNodeId id = 1; id <= count; ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (node == NULL) {
            fprintf(stderr, "R1546 STOP: missing checked node %zu\n", id);
            return 0;
        }
        if (node->kind == NL_CHECKED_USIZE_DIVISION) {
            NLUsizeDivisionView view = {0};
            if (!nl_checked_usize_division_view(fragment, id, &view) ||
                !nl_checked_usize_division_valid(fragment, id, NULL) ||
                !nl_checked_usize_division_valid(fragment, id, &view)) {
                fprintf(stderr, "R1546 STOP: genuine division issuer invalid\n");
                return 0;
            }
            if (view.numbers[1] != 23 || view.target_max != SIZE_MAX) {
                fprintf(stderr, "R1546 STOP: divisor/target mismatch\n");
                return 0;
            }
            NLUsizeDivisionView forged = view;
            forged.numbers[1] = 0;
            if (nl_checked_usize_division_valid(fragment, id, &forged)) {
                fprintf(stderr, "R1546 STOP: forged zero divisor accepted\n");
                return 0;
            }
            forged = view;
            forged.origin = NULL;
            if (nl_checked_usize_division_valid(fragment, id, &forged)) {
                fprintf(stderr, "R1546 STOP: foreign origin accepted\n");
                return 0;
            }
            forged = view;
            forged.quotient_known = true;
            forged.quotient = SIZE_MAX;
            if (nl_checked_usize_division_valid(fragment, id, &forged)) {
                fprintf(stderr, "R1546 STOP: invented Known accepted\n");
                return 0;
            }
            NLUsizeDivisionView unchanged;
            memset(&unchanged, 0xa5, sizeof(unchanged));
            NLUsizeDivisionView before = unchanged;
            if (nl_checked_usize_division_view(fragment, 0, &unchanged) ||
                memcmp(&unchanged, &before, sizeof(unchanged)) != 0) {
                fprintf(stderr, "R1546 STOP: failure clobbered output view\n");
                return 0;
            }
            audit->division_events++;
            if (view.numerator_known) {
                if (!view.quotient_known || view.numbers[0] != 1601 ||
                    view.quotient != 69) {
                    fprintf(stderr, "R1546 STOP: actual quotient incorrect\n");
                    return 0;
                }
                audit->authenticated_quotients++;
                printf("R1546_PUBLIC_QUOTIENT=%zu\n", view.quotient);
            } else if (view.quotient_known) {
                fprintf(stderr, "R1546 STOP: definition Unknown made Known\n");
                return 0;
            }
        }
        if (node->kind == NL_CHECKED_USIZE_ADDITION) {
            NLUsizeAdditionView view = {0};
            if (!nl_checked_usize_addition_view(fragment, id, &view) ||
                !nl_checked_usize_addition_valid(fragment, id, NULL) ||
                !nl_checked_usize_addition_valid(fragment, id, &view) ||
                view.sum != 70) {
                fprintf(stderr, "R1546 STOP: caller addition not authenticated\n");
                return 0;
            }
            audit->authenticated_sums++;
            printf("R1546_PUBLIC_SUCCESSOR=%zu\n", view.sum);
        }
        const NLCheckedFragment *callee = nl_checked_call_body(fragment, id);
        if (callee != NULL && !inspect(callee, depth + 1, audit)) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3 ||
        (strcmp(argv[1], "positive") != 0 &&
         strcmp(argv[1], "negative") != 0)) {
        fprintf(stderr, "usage: r1546_public_probe positive|negative SOURCE\n");
        return 2;
    }

    int outcome = 3;
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSemanticContext *context = NULL;
    NLSource *call_source = NULL;
    NLParser *call_parser = NULL;
    NLSyntaxTree *call_tree = NULL;
    NLCheckedFragment *checked = NULL;

    if (nl_source_load(argv[2], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK) {
        fprintf(stderr, "R1546 SOURCE read/parser setup failure\n");
        goto cleanup;
    }
    NLParseDiagnostic parse_diagnostic = {0};
    NLParseStatus parsed =
        nl_parser_parse_function_unit(parser, &unit, &parse_diagnostic);
    if (parsed != NL_PARSE_OK) {
        fprintf(stderr, "R1546 SOURCE refusal: parse status %d\n",
                (int)parsed);
        goto cleanup;
    }
    if (nl_semantic_create(&context) != NL_CHECK_OK) {
        fprintf(stderr, "R1546 semantic context failure\n");
        goto cleanup;
    }
    const NLSyntaxTree *inputs[1] = {unit};
    NLFunctionUnitDiagnostic registration = {0};
    NLCheckStatus registered = nl_semantic_register_function_unit(
        context, inputs, 1, &registration);

    if (strcmp(argv[1], "negative") == 0) {
        if (registered == NL_CHECK_SEMANTIC_ERROR) {
            puts("R1546_NEGATIVE=SEMANTIC_REJECT_NONZERO_PRECONDITION");
            outcome = 0;
        } else {
            fprintf(stderr,
                    "R1546 STOP: expected semantic zero refusal, got %d\n",
                    (int)registered);
        }
        goto cleanup;
    }
    if (registered != NL_CHECK_OK) {
        fprintf(stderr, "R1546 STOP: positive registration status %d\n",
                (int)registered);
        goto cleanup;
    }
    static const char call_bytes[] = "main()";
    if (nl_source_create(call_bytes, sizeof(call_bytes) - 1, "R1546-main",
                         &call_source) != NL_SOURCE_OK ||
        nl_parser_create(call_source, &call_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(call_parser, &call_tree,
                                             &parse_diagnostic) != NL_PARSE_OK) {
        fprintf(stderr, "R1546 STOP: independent main call setup failed\n");
        goto cleanup;
    }
    NLCheckDiagnostic check_diagnostic = {0};
    NLCheckStatus called = nl_semantic_check_expression(
        context, call_tree, &checked, &check_diagnostic);
    if (called != NL_CHECK_OK || checked == NULL) {
        fprintf(stderr, "R1546 STOP: checked main call status %d\n",
                (int)called);
        goto cleanup;
    }
    R1546Audit audit = {0};
    if (!inspect(checked, 0, &audit) ||
        audit.authenticated_quotients == 0 ||
        audit.authenticated_sums == 0) {
        fprintf(stderr,
                "R1546 INCONCLUSIVE: real quotient/caller not observable "
                "(div=%zu known=%zu sum=%zu)\n",
                audit.division_events, audit.authenticated_quotients,
                audit.authenticated_sums);
        goto cleanup;
    }
    printf("R1546_OWNED=PASS div=%zu known=%zu sum=%zu\n",
           audit.division_events, audit.authenticated_quotients,
           audit.authenticated_sums);
    outcome = 0;

cleanup:
    nl_checked_destroy(checked);
    nl_syntax_tree_destroy(call_tree);
    nl_parser_destroy(call_parser);
    nl_source_destroy(call_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return outcome;
}
