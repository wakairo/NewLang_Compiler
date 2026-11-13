#include "newlang/captured_closure.h"
#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/syntax.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
    size_t bodies;
    size_t nodes;
    size_t changes;
    size_t reads;
    size_t certified;
} R1395Stats;

static void inspect(const NLCheckedFragment *fragment, unsigned depth,
                    R1395Stats *stats)
{
    if (fragment == NULL || depth > 20) {
        return;
    }
    size_t count = nl_checked_node_count(fragment);
    if (count > 4096) {
        return;
    }
    stats->bodies++;
    stats->nodes += count;
    for (NLCheckedNodeId id = 1; id <= count; ++id) {
        const NLCheckedNodeView *view = nl_checked_node_view(fragment, id);
        if (view == NULL) {
            continue;
        }
        if (view->kind == NL_CHECKED_REPLACE) {
            stats->changes++;
        }
        if (view->kind == NL_CHECKED_LINK_READ) {
            stats->reads++;
        }
        if (view->kind == NL_CHECKED_MATCH) {
            bool postfree_arm = false;
            for (size_t arm = 0; arm < view->normal_arms && arm < 16; ++arm) {
                size_t changes_before = stats->changes;
                size_t reads_before = stats->reads;
                inspect(nl_checked_match_arm(fragment, id, arm), depth + 1,
                        stats);
                if (stats->changes - changes_before >= 9 &&
                    stats->reads - reads_before >= 2) {
                    postfree_arm = true;
                }
            }
            if (postfree_arm) {
                if (nl_checked_captured_closure_validate(fragment, id) ==
                    NL_CHECK_OK) {
                    stats->certified++;
                }
            }
        }
        if (view->kind == NL_CHECKED_REGISTERED_CALL) {
            inspect(nl_checked_call_body(fragment, id), depth + 1, stats);
        }
    }
}

static int failure(const char *stage, int status)
{
    fprintf(stderr, "R1395_PUBLIC_OWNED_NOT_VERIFIED stage=%s status=%d\n",
            stage, status);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return failure("ARGUMENTS", argc);
    }
    int result = 1;
    NLSource *program = NULL;
    NLSource *call_source = NULL;
    NLParser *parser = NULL;
    NLParser *call_parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSyntaxTree *call_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *artifact = NULL;
    NLParseDiagnostic parse_diagnostic = {0};
    NLCheckDiagnostic diagnostic = {0};
    NLFunctionUnitDiagnostic unit_diagnostic = {0};
    NLSourceStatus source_status = nl_source_load(argv[1], &program);
    if (source_status != NL_SOURCE_OK) {
        result = failure("LOAD", source_status);
        goto cleanup;
    }
    NLParseStatus parse_status = nl_parser_create(program, &parser);
    if (parse_status != NL_PARSE_OK) {
        result = failure("PARSER_CREATE", parse_status);
        goto cleanup;
    }
    parse_status =
        nl_parser_parse_function_unit(parser, &unit, &parse_diagnostic);
    if (parse_status != NL_PARSE_OK) {
        result = failure("FUNCTION_UNIT", parse_status);
        goto cleanup;
    }
    NLCheckStatus check_status = nl_semantic_create(&context);
    if (check_status != NL_CHECK_OK) {
        result = failure("SEMANTIC_CREATE", check_status);
        goto cleanup;
    }
    const NLSyntaxTree *unit_array[] = {unit};
    check_status = nl_semantic_register_function_unit(context, unit_array, 1,
                                                      &unit_diagnostic);
    if (check_status != NL_CHECK_OK) {
        result = failure("UNIT_REGISTER", check_status);
        goto cleanup;
    }
    static const char call_bytes[] = "main()";
    source_status = nl_source_create(call_bytes, sizeof(call_bytes) - 1,
                                     "r1395-readonly-main-call", &call_source);
    if (source_status != NL_SOURCE_OK) {
        result = failure("CALL_SOURCE", source_status);
        goto cleanup;
    }
    parse_status = nl_parser_create(call_source, &call_parser);
    if (parse_status != NL_PARSE_OK) {
        result = failure("CALL_PARSER", parse_status);
        goto cleanup;
    }
    parse_status = nl_parser_parse_expression_fragment(call_parser, &call_tree,
                                                       &parse_diagnostic);
    if (parse_status != NL_PARSE_OK) {
        result = failure("CALL_PARSE", parse_status);
        goto cleanup;
    }
    check_status = nl_semantic_check_expression(context, call_tree, &artifact,
                                                &diagnostic);
    if (check_status != NL_CHECK_OK || artifact == NULL) {
        result = failure("CALL_CHECK", check_status);
        goto cleanup;
    }
    R1395Stats stats = {0};
    inspect(artifact, 0, &stats);
    fprintf(stderr,
            "R1395_PUBLIC_OWNED_VIEW artifact_nodes=%zu fragments=%zu "
            "replace_nodes=%zu link_read_nodes=%zu validated_matches=%zu\n",
            stats.nodes, stats.bodies, stats.changes, stats.reads,
            stats.certified);
    if (stats.bodies < 2 || stats.changes < 9 || stats.reads < 2) {
        result = failure("ARTIFACT_COVERAGE_NOT_VERIFIED", 0);
        goto cleanup;
    }
    if (stats.certified < 1) {
        result = failure("POSTFREE_CAPTURED_CLOSURE_NOT_VERIFIED", 0);
        goto cleanup;
    }
    fprintf(stderr, "R1395_PUBLIC_OWNED_SOURCE_ARTIFACT_PRESENT\n");
    fprintf(stderr, "R1395_PUBLIC_PROOF_REDERIVATION=%s\n",
            stats.certified > 0 ? "OBSERVED" : "NOT_VERIFIED");
    result = 0;
cleanup:
    nl_checked_destroy(artifact);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(call_tree);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(call_parser);
    nl_parser_destroy(parser);
    nl_source_destroy(call_source);
    nl_source_destroy(program);
    return result;
}
