#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* R1421: independent public API observer, not a provenance/owner issuer. */
static void parse_result(const char *phase, NLParseStatus status,
                         const NLParseDiagnostic *diagnostic)
{
    printf("R1421 phase=%s parse_status=%d code=%s span=%zu:%zu\n", phase,
           (int)status,
           diagnostic->diagnostic.code ? diagnostic->diagnostic.code : "NONE",
           diagnostic->span.start_byte, diagnostic->span.end_byte);
}

static void check_result(const char *phase, NLCheckStatus status,
                         const NLCheckDiagnostic *diagnostic)
{
    printf("R1421 phase=%s check_status=%d code=%s span=%zu:%zu\n", phase,
           (int)status,
           diagnostic->diagnostic.code ? diagnostic->diagnostic.code : "NONE",
           diagnostic->span.start_byte, diagnostic->span.end_byte);
}

static void snapshot(const NLSemanticContext *context, const char *phase)
{
    NLSemanticSnapshot state = {0};
    if (!nl_semantic_snapshot(context, &state)) {
        printf("R1421 phase=%s snapshot=NOT_OBSERVED\n", phase);
        return;
    }
    printf("R1421 phase=%s bindings=%zu functions=%zu values=%zu places=%zu "
           "domains=%zu scopes=%zu regions=%zu occurrences=%zu\n",
           phase, state.bindings, state.functions, state.values, state.places,
           state.domains, state.scopes, state.backing_regions,
           state.occurrences);
}

static void observe(const NLCheckedFragment *fragment, unsigned depth,
                    size_t *seen, size_t *assumptions, int *invalid)
{
    if (!fragment || depth > 16 || *seen > 16384) {
        return;
    }
    printf("R1421 owned_depth=%u class=%d nodes=%zu source_present=%d\n", depth,
           (int)nl_checked_proof_class(fragment),
           nl_checked_node_count(fragment),
           nl_checked_source(fragment) != NULL);
    size_t count = nl_checked_node_count(fragment);
    for (size_t id = 1; id <= count && *seen < 16384; ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (!node) {
            continue;
        }
        ++*seen;
        if (node->kind == NL_CHECKED_REF_FROM_PTR ||
            node->kind == NL_CHECKED_INITIALIZE ||
            node->kind == NL_CHECKED_DESTROY ||
            node->kind == NL_CHECKED_REPLACE ||
            node->kind == NL_CHECKED_FIELD_REF ||
            node->kind == NL_CHECKED_LINK_READ ||
            node->kind == NL_CHECKED_LOAN_HEADER ||
            node->kind == NL_CHECKED_ALLOCATE ||
            node->kind == NL_CHECKED_DEALLOCATE) {
            printf("R1421 node depth=%u id=%zu kind=%d span=%zu:%zu "
                   "place=%zu incarnation=%zu domain=%zu backing=%zu "
                   "loan_domain=%zu loan_scope=%zu field_parent=%zu "
                   "field_child=%zu\n",
                   depth, id, (int)node->kind, node->span.start_byte,
                   node->span.end_byte, node->lifetime_place,
                   node->lifetime_incarnation, node->lifetime_domain,
                   node->backing, node->loan.domain, node->loan.scope,
                   node->field.parent, node->field.child);
        }
        NLCheckedLocalAssumptionView record = {0};
        if (nl_checked_local_assumption_view(fragment, id, &record)) {
            ++*assumptions;
            NLCheckStatus origin =
                nl_checked_local_assumption_origin(fragment, id);
            printf("R1421 historical_assumption depth=%u id=%zu "
                   "span=%zu:%zu conditions=%u operation=%zu "
                   "pointer_argument=%zu stability_argument=%zu "
                   "pointer_if=%zu projection=%zu read=%zu "
                   "target=%zu incarnation=%zu region=%zu "
                   "domain=%zu scope=%zu stable=%d original_live=%d "
                   "loan_closed=%d origin_integrity=%d "
                   "origin_is_not_premise_truth=1\n",
                   depth, id, record.span.start_byte, record.span.end_byte,
                   record.assumed_conditions, record.operation,
                   record.pointer_argument, record.stability_argument,
                   record.pointer_if, record.projection, record.read,
                   record.conditional_target, record.conditional_incarnation,
                   record.conditional_range.region, record.domain, record.scope,
                   record.stable_at_operation,
                   record.original_live_at_operation, record.loan_closed,
                   (int)origin);
            if (origin != NL_CHECK_OK) {
                *invalid = 1;
            }
            if (record.source) {
                NLSourceView view = {0};
                if (nl_source_view(record.source, record.span, &view)) {
                    printf("R1421 retained_source_length=%zu record_text=",
                           nl_source_length(record.source));
                    if (view.length) {
                        fwrite(view.bytes, 1, view.length, stdout);
                    }
                    putchar('\n');
                } else {
                    puts("R1421 record_source_span=NOT_OBSERVED");
                }
            } else {
                puts("R1421 retained_record_source=NOT_OBSERVED");
            }
        }
        const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
        if (body && body != fragment) {
            observe(body, depth + 1, seen, assumptions, invalid);
        }
        if (node->kind == NL_CHECKED_MATCH) {
            for (size_t arm = 0; arm < 2; ++arm) {
                const NLCheckedFragment *part =
                    nl_checked_match_arm(fragment, id, arm);
                if (part && part != fragment) {
                    observe(part, depth + 1, seen, assumptions, invalid);
                }
            }
        }
        if (node->kind == NL_CHECKED_IF) {
            for (size_t arm = 0; arm < 2; ++arm) {
                const NLCheckedFragment *part =
                    nl_checked_if_arm(fragment, id, arm);
                if (part && part != fragment) {
                    observe(part, depth + 1, seen, assumptions, invalid);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fputs("R1421 usage: observe positive|wrong|ended source fn\n", stderr);
        return 2;
    }
    int negative = strcmp(argv[1], "positive") != 0;
    if ((!negative && strcmp(argv[3], "author_route") != 0) ||
        (strcmp(argv[1], "wrong") == 0 &&
         strcmp(argv[3], "wrong_domain") != 0) ||
        (strcmp(argv[1], "ended") == 0 &&
         strcmp(argv[3], "ended_original") != 0) ||
        (negative && strcmp(argv[1], "wrong") != 0 &&
         strcmp(argv[1], "ended") != 0)) {
        fputs("R1421 unexpected case/function mapping\n", stderr);
        return 2;
    }

    int result = 0;
    unsigned char input[8192] = {0};
    FILE *file = fopen(argv[2], "rb");
    if (!file) {
        fputs("R1421 cannot open frozen source\n", stderr);
        return 3;
    }
    size_t length = fread(input, 1, sizeof(input), file);
    int extra = fgetc(file);
    int io_failure = ferror(file);
    fclose(file);
    if (io_failure || extra != EOF || length == 0) {
        fputs("R1421 frozen source read failure/oversize\n", stderr);
        return 3;
    }

    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSemanticContext *context = NULL;
    NLSource *call_source = NULL;
    NLParser *call_parser = NULL;
    NLSyntaxTree *call = NULL;
    NLCheckedFragment *owned = NULL;
    NLSourceStatus ss = nl_source_create(input, length, argv[2], &source);
    printf("R1421 phase=source_create status=%d bytes=%zu owner=%d\n", (int)ss,
           length, source != NULL);
    if (ss != NL_SOURCE_OK) {
        result = 4;
        goto cleanup;
    }
    NLParseStatus ps = nl_parser_create(source, &parser);
    printf("R1421 phase=parser_create status=%d owner=%d\n", (int)ps,
           parser != NULL);
    if (ps != NL_PARSE_OK) {
        result = 4;
        goto cleanup;
    }
    NLParseDiagnostic pd = {0};
    ps = nl_parser_parse_function_unit(parser, &unit, &pd);
    parse_result("parse_function_unit", ps, &pd);
    printf("R1421 unit_owner=%d\n", unit != NULL);
    if (ps != NL_PARSE_OK) {
        puts("R1421 result=NOT_REACHED phase=source_parser");
        goto cleanup;
    }
    NLCheckStatus cs = nl_semantic_create(&context);
    printf("R1421 phase=semantic_create status=%d owner=%d\n", (int)cs,
           context != NULL);
    if (cs != NL_CHECK_OK) {
        result = 4;
        goto cleanup;
    }
    snapshot(context, "before_registration");
    const NLSyntaxTree *inputs[] = {unit};
    NLFunctionUnitDiagnostic fd = {0};
    cs = nl_semantic_register_function_unit(context, inputs, 1, &fd);
    check_result("definition_registration", cs, &fd.diagnostic);
    printf("R1421 registration_input_index=%zu\n", fd.input_index);
    snapshot(context, "after_registration");
    if (cs != NL_CHECK_OK) {
        puts("R1421 result=NOT_REACHED phase=definition_registration");
        goto cleanup;
    }

    /* Registration owns a durable body plan. Destroy original input owners
     * BEFORE checking a genuine call, without manufacturing semantic state. */
    nl_syntax_tree_destroy(unit);
    unit = NULL;
    nl_parser_destroy(parser);
    parser = NULL;
    nl_source_destroy(source);
    source = NULL;
    puts("R1421 original_unit_parser_and_source_released=1");

    char call_text[96] = {0};
    int written = snprintf(call_text, sizeof(call_text), "%s()", argv[3]);
    if (written < 0 || (size_t)written >= sizeof(call_text)) {
        result = 4;
        goto cleanup;
    }
    ss = nl_source_create(call_text, (size_t)written, "r1421-call",
                          &call_source);
    if (ss != NL_SOURCE_OK) {
        result = 4;
        goto cleanup;
    }
    ps = nl_parser_create(call_source, &call_parser);
    if (ps != NL_PARSE_OK) {
        result = 4;
        goto cleanup;
    }
    NLParseDiagnostic call_pd = {0};
    ps = nl_parser_parse_expression_fragment(call_parser, &call, &call_pd);
    parse_result("parse_actual_call", ps, &call_pd);
    if (ps != NL_PARSE_OK) {
        puts("R1421 result=NOT_REACHED phase=call_parser");
        goto cleanup;
    }
    NLCheckDiagnostic cd = {0};
    cs = nl_semantic_check_expression(context, call, &owned, &cd);
    check_result("actual_function_call_CHECK", cs, &cd);
    printf("R1421 actual_call_owned=%d\n", owned != NULL);
    snapshot(context, "after_actual_call");
    if (cs != NL_CHECK_OK) {
        puts("R1421 result=NOT_REACHED phase=actual_call_CHECK "
             "target_condition_not_proven_reached");
        goto cleanup;
    }
    if (!owned) {
        puts("R1421 STOP CHECK_OK_WITHOUT_OWNED");
        result = 51;
        goto cleanup;
    }
    if (negative) {
        puts("R1421 STOP UNSAFE_ACCEPTANCE_OF_INTENTIONALLY_FALSE_PREMISE");
        result = 51;
        goto cleanup;
    }

    NLCheckedProofClass proof = nl_checked_proof_class(owned);
    printf("R1421 positive_accepted=1 public_proof_class=%d "
           "not_a_truth_certificate=1\n",
           (int)proof);
    if (proof == NL_CHECKED_PROOF_INVALID) {
        puts("R1421 STOP PUBLIC_PROOF_CLASS_INVALID");
        result = 51;
        goto cleanup;
    }
    size_t seen = 0;
    size_t assumptions = 0;
    int invalid = 0;
    observe(owned, 0, &seen, &assumptions, &invalid);
    printf("R1421 public_nodes_observed=%zu historical_records=%zu "
           "source_issued_owned_observed=1\n",
           seen, assumptions);
    if (invalid) {
        puts("R1421 STOP ORIGIN_INTEGRITY_FAILURE");
        result = 51;
    } else if (assumptions == 0) {
        puts("R1421 author_assumption_seam=NOT_OBSERVED "
             "machine_proof_requires_independent_evidence");
    } else {
        puts("R1421 author_assumption_seam=HISTORICAL_RECORD_OBSERVED "
             "author_premise_truth_not_machine_proved");
    }

cleanup:
    nl_checked_destroy(owned);
    nl_syntax_tree_destroy(call);
    nl_parser_destroy(call_parser);
    nl_source_destroy(call_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return result;
}
