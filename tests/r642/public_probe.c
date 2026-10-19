#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/syntax.h"

#include <stdio.h>
#include <string.h>

static int examine(const char *path, bool negative)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSemanticContext *context = NULL;
    NLSource *entry_source = NULL;
    NLParser *entry_parser = NULL;
    NLSyntaxTree *entry_tree = NULL;
    NLCheckedFragment *entry = NULL;
    int result = 1;
    NLParseDiagnostic parsed = {0};
    NLFunctionUnitDiagnostic registered = {0};
    NLCheckDiagnostic checked = {0};

    if (nl_source_load(path, &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(parser, &unit, &parsed) != NL_PARSE_OK ||
        nl_semantic_create(&context) != NL_CHECK_OK) {
        fputs("R642: source/parser failure\n", stderr);
        goto cleanup;
    }
    const NLSyntaxTree *inputs[] = {unit};
    NLCheckStatus status =
        nl_semantic_register_function_unit(context, inputs, 1, &registered);
    if (negative) {
        if (status != NL_CHECK_SEMANTIC_ERROR) {
            fprintf(stderr, "R642: expected semantic error, status=%d\n",
                    (int)status);
            goto cleanup;
        }
        puts("R642: semantic-reject (parsed first)");
        result = 0;
        goto cleanup;
    }
    if (status != NL_CHECK_OK) {
        fprintf(stderr, "R642: unexpected semantic status=%d\n", (int)status);
        goto cleanup;
    }

    nl_syntax_tree_destroy(unit);
    unit = NULL;
    nl_parser_destroy(parser);
    parser = NULL;
    nl_source_destroy(source);
    source = NULL;

    const char invocation[] = "main()";
    if (nl_source_create(invocation, sizeof(invocation) - 1, "R642-entry",
                         &entry_source) != NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(entry_parser, &entry_tree,
                                            &parsed) != NL_PARSE_OK ||
        nl_semantic_check_expression(context, entry_tree, &entry, &checked) !=
            NL_CHECK_OK) {
        fputs("R642: registered entry check failure\n", stderr);
        goto cleanup;
    }

    const NLCheckedNodeId root = nl_checked_root(entry);
    const NLCheckedNodeView *call = nl_checked_node_view(entry, root);
    const NLCheckedFragment *body = nl_checked_call_body(entry, root);
    if (call == NULL || call->kind != NL_CHECKED_REGISTERED_CALL ||
        body == NULL) {
        fputs("R642: no real public owned call body\n", stderr);
        goto cleanup;
    }
    size_t conditionals = 0;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, i);
        if (node != NULL && node->kind == NL_CHECKED_IF) {
            if (nl_checked_if_arm(body, i, 0) == NULL ||
                nl_checked_if_arm(body, i, 1) == NULL) {
                fputs("R642: missing arm-owned worlds\n", stderr);
                goto cleanup;
            }
            ++conditionals;
        }
    }
    if (conditionals != 1) {
        fprintf(stderr, "R642: expected one owned IF, got %zu\n",
                conditionals);
        goto cleanup;
    }
    printf("R642: registered public-owned body nodes=%zu IF=%zu arms=2\n",
           nl_checked_node_count(body), conditionals);
    result = 0;

cleanup:
    nl_checked_destroy(entry);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return result;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    if (strcmp(argv[1], "positive") == 0)
        return examine(argv[2], false);
    if (strcmp(argv[1], "negative") == 0)
        return examine(argv[2], true);
    return 2;
}
