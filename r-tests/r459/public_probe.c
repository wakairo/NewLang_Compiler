/* R459 independent read-only source-issued checked-artifact probe.
 * No fixture seeding, no source edits, no authority creation by this observer. */
#include "newlang/source.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/checked.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void fail(const char *at, int status) {
    fprintf(stderr, "R459 PUBLIC EVIDENCE BARRIER at %s status=%d\n", at, status);
    exit(31);
}
int main(int argc, char **argv) {
    if (argc != 2) fail("argc", argc);
    NLSource *src = NULL, *call_src = NULL;
    NLParser *parser = NULL, *call_parser = NULL;
    NLSyntaxTree *unit_tree = NULL, *call_tree = NULL;
    NLSemanticContext *sem = NULL;
    NLCheckedFragment *owned = NULL;
    NLParseDiagnostic parse_diag = {0};
    NLFunctionUnitDiagnostic unit_diag = {0};
    NLCheckDiagnostic diag = {0};
    NLSourceStatus ss = nl_source_load(argv[1], &src);
    if (ss != NL_SOURCE_OK) fail("source_load", ss);
    NLParseStatus ps = nl_parser_create(src, &parser);
    if (ps != NL_PARSE_OK) fail("parser_create", ps);
    ps = nl_parser_parse_function_unit(parser, &unit_tree, &parse_diag);
    if (ps != NL_PARSE_OK) fail("parse_function_unit", ps);
    NLCheckStatus cs = nl_semantic_create(&sem);
    if (cs != NL_CHECK_OK) fail("semantic_create", cs);
    const NLSyntaxTree *inputs[1] = {unit_tree};
    cs = nl_semantic_register_function_unit(sem, inputs, 1, &unit_diag);
    if (cs != NL_CHECK_OK) fail("register_function_unit", cs);
    ss = nl_source_create("main()", strlen("main()"), "r459-internal-call", &call_src);
    if (ss != NL_SOURCE_OK) fail("call_source_create", ss);
    ps = nl_parser_create(call_src, &call_parser);
    if (ps != NL_PARSE_OK) fail("call_parser_create", ps);
    ps = nl_parser_parse_expression_fragment(call_parser, &call_tree, &parse_diag);
    if (ps != NL_PARSE_OK) fail("call_parse", ps);
    cs = nl_semantic_check_expression(sem, call_tree, &owned, &diag);
    if (cs != NL_CHECK_OK || !owned) fail("check_source_issued_call", cs);
    const NLCheckedFragment *body = NULL;
    size_t direct_calls = 0, ptr_from_refs = 0, sum_ctors = 0, aggregates = 0;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(owned); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(owned, i);
        if (!v) continue;
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            ++direct_calls;
            const NLCheckedFragment *b = nl_checked_call_body(owned, i);
            if (b) body = b;
        }
    }
    if (!body) fail("no_source_issued_owned_body", -1);
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(body, i);
        if (!v) continue;
        if (v->kind == NL_CHECKED_PTR_FROM_REF) {
            ++ptr_from_refs;
            if (!v->has_reference_result) fail("ptr_without_checked_reference", -1);
        }
        if (v->kind == NL_CHECKED_SUM_CONSTRUCTOR) ++sum_ctors;
        if (v->kind == NL_CHECKED_AGGREGATE) ++aggregates;
    }
    printf("R459 AUTHENTIC source-issued owned checked view: direct_calls=%zu body_nodes=%zu ptr_from_ref=%zu aggregate=%zu sum_constructors=%zu\n",
           direct_calls, nl_checked_node_count(body), ptr_from_refs, aggregates, sum_ctors);
    if (direct_calls == 0 || ptr_from_refs == 0 || aggregates < 2 || sum_ctors < 2)
        fail("owned_node_evidence_missing", -1);
    nl_checked_destroy(owned);
    nl_syntax_destroy(call_tree);
    nl_parser_destroy(call_parser);
    nl_source_destroy(call_src);
    nl_semantic_destroy(sem);
    nl_syntax_destroy(unit_tree);
    nl_parser_destroy(parser);
    nl_source_destroy(src);
    return 0;
}
