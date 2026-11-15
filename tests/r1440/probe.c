#include "newlang/checked.h"
#include "newlang/checked_c_node.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/syntax.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t assumptions;
    bool invalid_origin;
    bool invalid_record;
} R1440Scan;

static const char *optional(const char *s)
{
    return s != NULL ? s : "(none)";
}

static void parse_diagnostic(const NLSource *source, const NLParseDiagnostic *d)
{
    printf("R1440_PARSE_DIAG code=%s category=%s span=%zu:%zu message=%s\n",
           optional(d->diagnostic.code), optional(d->diagnostic.category),
           d->span.start_byte, d->span.end_byte,
           optional(d->diagnostic.message));
    (void)nl_parse_diagnostic_render(stdout, source, d);
}

static void check_diagnostic(const NLSource *source, const NLCheckDiagnostic *d)
{
    printf("R1440_CHECK_DIAG code=%s category=%s span=%zu:%zu message=%s\n",
           optional(d->diagnostic.code), optional(d->diagnostic.category),
           d->span.start_byte, d->span.end_byte,
           optional(d->diagnostic.message));
    (void)nl_check_diagnostic_render(stdout, source, d);
}

static const NLSyntaxNode *actual_main(const NLSyntaxTree *tree,
                                       const NLSource *source)
{
    const NLSyntaxView *root = nl_syntax_node_view(nl_syntax_tree_root(tree));
    if (root == NULL || root->kind != NL_SYNTAX_FUNCTION_UNIT) {
        return NULL;
    }
    const NLSyntaxNode *node = root->data.function_unit.declarations;
    while (node != NULL) {
        const NLSyntaxView *v = nl_syntax_node_view(node);
        if (v != NULL && v->kind == NL_SYNTAX_FUNCTION) {
            NLSourceView name = {0};
            if (nl_source_view(source, v->data.function.name, &name) &&
                name.length == 4 && memcmp(name.bytes, "main", 4) == 0) {
                return v->data.function.body;
            }
        }
        node = nl_syntax_next_argument(node);
    }
    return NULL;
}

static void scan_owned(const NLCheckedFragment *f, unsigned depth,
                       R1440Scan *scan)
{
    if (f == NULL) {
        return;
    }
    if (depth > 32) {
        scan->invalid_record = true;
        return;
    }
    const size_t count = nl_checked_node_count(f);
    for (size_t i = 1; i <= count; ++i) {
        const NLCheckedNodeView *node = nl_checked_node_view(f, i);
        NLCheckedLocalAssumptionView a = {0};
        if (nl_checked_local_assumption_view(f, i, &a)) {
            const NLCheckStatus origin =
                nl_checked_local_assumption_origin(f, i);
            ++scan->assumptions;
            printf("R1440_LOCAL_ASSUMPTION node=%zu span=%zu:%zu "
                   "conditions=%u domain=%zu scope=%zu origin=%d "
                   "stable=%d original_live=%d loan_closed=%d\n",
                   i, a.span.start_byte, a.span.end_byte, a.assumed_conditions,
                   (size_t)a.domain, (size_t)a.scope, (int)origin,
                   (int)a.stable_at_operation,
                   (int)a.original_live_at_operation, (int)a.loan_closed);
            if (origin != NL_CHECK_OK) {
                scan->invalid_origin = true;
            }
            if (a.source == NULL || a.owner == NULL ||
                !nl_source_span_valid(a.source, a.span) ||
                (a.assumed_conditions & NL_AUTHOR_PTR_TARGET_DOMAIN) == 0) {
                scan->invalid_record = true;
            }
        }
        if (node == NULL) {
            continue;
        }
        if (node->kind == NL_CHECKED_MATCH) {
            for (size_t arm = 0; arm < 2; ++arm) {
                scan_owned(nl_checked_match_arm(f, i, arm), depth + 1, scan);
            }
        }
        if (node->kind == NL_CHECKED_IF) {
            for (size_t arm = 0; arm < 2; ++arm) {
                scan_owned(nl_checked_if_arm(f, i, arm), depth + 1, scan);
            }
        }
        if (node->kind == NL_CHECKED_REGISTERED_CALL) {
            scan_owned(nl_checked_call_body(f, i), depth + 1, scan);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 3 ||
        (strcmp(argv[2], "R1") != 0 && strcmp(argv[2], "R2") != 0 &&
         strcmp(argv[2], "R3") != 0)) {
        fprintf(stderr, "R1440_STOP invalid runner arguments\n");
        return 2;
    }

    const bool positive = strcmp(argv[2], "R1") == 0;
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSemanticContext *context = NULL;
    NLSource *body_source = NULL;
    NLParser *body_parser = NULL;
    NLSyntaxTree *body = NULL;
    NLCheckedFragment *owned = NULL;
    int rc = 0;

    const NLSourceStatus load = nl_source_load(argv[1], &source);
    printf("R1440_STAGE=INPUT_SOURCE status=%d\n", (int)load);
    if (load != NL_SOURCE_OK) {
        rc = 2;
        goto done;
    }
    if (nl_parser_create(source, &parser) != NL_PARSE_OK) {
        printf("R1440_STOP=PUBLIC_PARSER_CREATE\n");
        rc = 2;
        goto done;
    }

    NLParseDiagnostic pd = {0};
    const NLParseStatus parsed =
        nl_parser_parse_function_unit(parser, &unit, &pd);
    printf("R1440_STAGE=SOURCE_UNIT_PARSE status=%d\n", (int)parsed);
    if (parsed != NL_PARSE_OK) {
        parse_diagnostic(source, &pd);
        printf("R1440_FIRST_LAYER=SOURCE_REFUSAL "
               "CHECK=NOT_REACHED OWNED=NOT_REACHED C17=NOT_REACHED\n");
        goto done;
    }

    if (nl_semantic_create(&context) != NL_CHECK_OK) {
        printf("R1440_STOP=PUBLIC_CONTEXT_CREATE\n");
        rc = 2;
        goto done;
    }
    const NLSyntaxTree *inputs[1] = {unit};
    NLFunctionUnitDiagnostic fd = {0};
    const NLCheckStatus registration =
        nl_semantic_register_function_unit(context, inputs, 1, &fd);
    printf("R1440_STAGE=DEFINITION_REGISTRATION status=%d\n",
           (int)registration);
    if (registration != NL_CHECK_OK) {
        printf("R1440_REGISTRATION_INPUT=%zu\n", fd.input_index);
        check_diagnostic(source, &fd.diagnostic);
        printf("R1440_FIRST_LAYER=DEFINITION_REGISTRATION_REFUSAL "
               "ACTUAL_CALL_CHECK=NOT_REACHED OWNED=NOT_REACHED "
               "C17=NOT_REACHED\n");
        goto done;
    }

    const NLSyntaxNode *main_body = actual_main(unit, source);
    const NLSyntaxView *main_view = nl_syntax_node_view(main_body);
    NLSourceView original_body = {0};
    if (main_view == NULL ||
        !nl_source_view(source, main_view->span, &original_body) ||
        original_body.length == 0 ||
        nl_source_create(original_body.bytes, original_body.length,
                         "R1440_exact_original_main_body",
                         &body_source) != NL_SOURCE_OK) {
        printf("R1440_STOP=ORIGINAL_MAIN_SPAN_UNAVAILABLE\n");
        rc = 2;
        goto done;
    }
    printf("R1440_MAIN_BODY_BYTES=%zu "
           "SOURCE_DERIVED_EXACT_SPAN=%zu:%zu\n",
           original_body.length, main_view->span.start_byte,
           main_view->span.end_byte);

    if (nl_parser_create(body_source, &body_parser) != NL_PARSE_OK) {
        printf("R1440_STOP=MAIN_BODY_PARSER_CREATE\n");
        rc = 2;
        goto done;
    }
    NLParseDiagnostic body_pd = {0};
    const NLParseStatus body_parse =
        nl_parser_parse_source_fragment(body_parser, &body, &body_pd);
    printf("R1440_STAGE=ACTUAL_MAIN_SOURCE_FRAGMENT status=%d\n",
           (int)body_parse);
    if (body_parse != NL_PARSE_OK) {
        parse_diagnostic(body_source, &body_pd);
        printf("R1440_FIRST_LAYER=MAIN_BODY_FRAGMENT_UNSUPPORTED "
               "ACTUAL_CALL_CHECK=NOT_REACHED OWNED=NOT_REACHED "
               "C17=NOT_REACHED\n");
        goto done;
    }

    NLCheckDiagnostic cd = {0};
    const NLCheckStatus checked =
        nl_semantic_check_source_fragment(context, body, &owned, &cd);
    printf("R1440_STAGE=ACTUAL_MAIN_CHECK status=%d\n", (int)checked);
    if (checked != NL_CHECK_OK) {
        check_diagnostic(body_source, &cd);
        printf("R1440_FIRST_LAYER=ACTUAL_CHECK_REFUSAL "
               "OWNED=NOT_REACHED C17=NOT_REACHED\n");
        goto done;
    }
    if (owned == NULL) {
        printf("R1440_STOP=CHECK_OK_WITHOUT_OWNED_ARTIFACT\n");
        rc = 3;
        goto done;
    }

    const NLCheckedProofClass proof = nl_checked_proof_class(owned);
    R1440Scan scan = {0};
    scan_owned(owned, 0, &scan);
    printf("R1440_STAGE=PUBLIC_OWNED node_count=%zu proof_class=%d "
           "author_records=%zu invalid_origin=%d invalid_record=%d\n",
           nl_checked_node_count(owned), (int)proof, scan.assumptions,
           (int)scan.invalid_origin, (int)scan.invalid_record);
    if (!positive) {
        printf("R1440_STOP=NEGATIVE_SEMANTIC_ACCEPTED "
               "GUEST=FORBIDDEN C17=NOT_REACHED\n");
        rc = 3;
        goto done;
    }
    if (proof != NL_CHECKED_AUTHOR_PREMISE || scan.assumptions != 1 ||
        scan.invalid_origin || scan.invalid_record) {
        printf("R1440_STOP=AUTHOR_PREMISE_PROOF_INTEGRITY "
               "C17=NOT_REACHED\n");
        rc = 3;
        goto done;
    }

    char *c17 = NULL;
    size_t c17_bytes = 0;
    const NLNodeCStatus emission = nl_checked_c_node(owned, &c17, &c17_bytes);
    printf("R1440_STAGE=R1_STRICT_C17_API status=%d bytes=%zu "
           "NATIVE=NOT_RUN\n",
           (int)emission, c17_bytes);
    if ((emission == NL_NODE_C_OK && c17 == NULL) ||
        (emission != NL_NODE_C_OK && (c17 != NULL || c17_bytes != 0))) {
        printf("R1440_STOP=C17_NO_CLOBBER_OR_OUTPUT_INTEGRITY\n");
        rc = 3;
    } else {
        printf("R1440_FIRST_LAYER=%s NATIVE=NOT_RUN\n",
               emission == NL_NODE_C_OK ? "C17_API_REACHED"
                                        : "C17_API_REFUSAL");
    }
    free(c17);

done:
    nl_checked_destroy(owned);
    nl_syntax_tree_destroy(body);
    nl_parser_destroy(body_parser);
    nl_source_destroy(body_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return rc;
}
