/* R402 FRESH BLIND public owned-evidence probe. No production test imports. */
#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    NLSource *source;
    NLSyntaxTree *tree;
} Input;

static void release_input(Input *input)
{
    nl_syntax_tree_destroy(input->tree);
    nl_source_destroy(input->source);
    input->tree = NULL;
    input->source = NULL;
}

static int parse_input(const char *text, bool function_unit, Input *out)
{
    NLParser *parser = NULL;
    NLParseDiagnostic diagnostic = {0};
    out->source = NULL;
    out->tree = NULL;
    NLSourceStatus source_status =
        nl_source_create(text, strlen(text), "r402-independent", &out->source);
    if (source_status != NL_SOURCE_OK) {
        printf("R402 owned source_create=%d\n", (int)source_status);
        return 0;
    }
    NLParseStatus status = nl_parser_create(out->source, &parser);
    if (status != NL_PARSE_OK) {
        printf("R402 owned parser_create=%d\n", (int)status);
        release_input(out);
        return 0;
    }
    if (function_unit) {
        status = nl_parser_parse_function_unit(parser, &out->tree, &diagnostic);
    } else {
        status = nl_parser_parse_source_fragment(parser, &out->tree,
                                                 &diagnostic);
    }
    nl_parser_destroy(parser);
    if (status != NL_PARSE_OK) {
        printf("R402 owned parse=%d code=%s\n", (int)status,
               diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
        release_input(out);
        return 0;
    }
    return 1;
}

static int observe_owned(const NLCheckedFragment *checked)
{
    size_t count = nl_checked_node_count(checked);
    size_t matches = 0, verified = 0;
    for (NLCheckedNodeId i = 1; i <= count; ++i) {
        const NLCheckedNodeView *view = nl_checked_node_view(checked, i);
        if (view == NULL || view->kind != NL_CHECKED_MATCH)
            continue;
        ++matches;
        const NLCheckedFragment *arm0 = nl_checked_match_arm(checked, i, 0);
        const NLCheckedFragment *arm1 = nl_checked_match_arm(checked, i, 1);
        size_t n0 = arm0 ? nl_checked_node_count(arm0) : 0;
        size_t n1 = arm1 ? nl_checked_node_count(arm1) : 0;
        bool separate_worlds = arm0 != NULL && arm1 != NULL;
        if (separate_worlds) {
            const NLSemanticContext *world = nl_checked_context(checked);
            const NLSemanticContext *left = nl_checked_context(arm0);
            const NLSemanticContext *right = nl_checked_context(arm1);
            separate_worlds =
                left != right && left != world && right != world;
        }
        printf("R402 owned arms=%zu counts=%zu,%zu worlds=%d\n",
               view->normal_arms, n0, n1, separate_worlds);
        if (view->normal_arms == 2 && n0 > 0 && n1 > 0 && separate_worlds)
            ++verified;
        /* IDs in different arm contexts are intentionally never compared. */
    }
    printf("R402 owned nodes=%zu matches=%zu verified=%zu\n", count,
           matches, verified);
    return verified == 1;
}

int main(void)
{
    static const char declaration[] =
        "struct Node { next: Option<ptr<Node>>, payload: u8, }\n"
        "fn seed() -> unit { unit }\n";
    static const char good[] =
        "{\n"
        " let a = Node { next: Option<ptr<Node>>::None, payload: u8(1) };\n"
        " let b = Node { next: Option<ptr<Node>>::None, payload: u8(2) };\n"
        " let pa = loan_read(a) { |ra| ptr_from_ref(ra) };\n"
        " let pb = loan_read(b) { |rb| ptr_from_ref(rb) };\n"
        " let option = Option<ptr<Node>>::Some(pa);\n"
        " match option {\n"
        "  Some(q) => { loan_write(a@next) { |w| replace(w, "
        "Option<ptr<Node>>::Some(pb)) }; unit },\n"
        "  None => { loan_write(a@next) { |w| replace(w, "
        "Option<ptr<Node>>::None) }; unit },\n"
        " };\n"
        " let after = a@next;\n"
        " loan_write(a@next) { |w| replace(w, Option<ptr<Node>>::None) };\n"
        " unit\n"
        "}\n";
    static const char bad[] =
        "{\n"
        " let a = Node { next: Option<ptr<Node>>::None, payload: u8(1) };\n"
        " let b = Node { next: Option<ptr<Node>>::None, payload: u8(2) };\n"
        " let pa = loan_read(a) { |ra| ptr_from_ref(ra) };\n"
        " let opt = Option<ptr<Node>>::Some(pa);\n"
        " match opt {\n"
        "  Some(q) => { unit },\n"
        "  None => { r402_unknown_binding; unit },\n"
        " };\n"
        " unit\n"
        "}\n";
    Input d = {0}, p = {0}, n = {0};
    NLSemanticContext *ctx = NULL;
    NLCheckedFragment *owned = NULL;
    NLCheckDiagnostic diagnostic = {0};
    NLFunctionUnitDiagnostic unit_diag = {0};
    int passed = 0;
    NLCheckStatus positive = NL_CHECK_INTERNAL_ERROR;
    if (!parse_input(declaration, true, &d))
        goto cleanup;
    if (!parse_input(good, false, &p))
        goto cleanup;
    if (!parse_input(bad, false, &n))
        goto cleanup;
    NLCheckStatus s = nl_semantic_create(&ctx);
    if (s != NL_CHECK_OK) {
        printf("R402 owned semantic_create=%d\n", (int)s);
        goto cleanup;
    }
    const NLSyntaxTree *unit[] = {d.tree};
    s = nl_semantic_register_function_unit(ctx, unit, 1, &unit_diag);
    const char *reg_code = unit_diag.diagnostic.diagnostic.code;
    printf("R402 owned register_unit=%d code=%s\n", (int)s,
           reg_code ? reg_code : "-");
    if (s != NL_CHECK_OK)
        goto cleanup;
    s = nl_semantic_check_source_fragment(ctx, p.tree, &owned, &diagnostic);
    printf("R402 owned check_positive=%d code=%s output=%d\n", (int)s,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-",
           owned != NULL);
    positive = s;
    if (s == NL_CHECK_OK && owned != NULL) {
        passed = observe_owned(owned);
    } else if (s == NL_CHECK_ANALYSIS_PRECISION_LIMIT && owned == NULL) {
        /* Public fragment precision barrier, not owned-artifact validation. */
        passed = 1;
        printf("R402 owned BOUNDARY: no public owned fragment\n");
    }
    nl_checked_destroy(owned);
    owned = NULL;
    diagnostic = (NLCheckDiagnostic){0};
    s = nl_semantic_check_source_fragment(ctx, n.tree, &owned, &diagnostic);
    printf("R402 owned check_negative=%d code=%s output=%d\n", (int)s,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-",
           owned != NULL);
    if (s != NL_CHECK_SEMANTIC_ERROR || owned != NULL)
        passed = 0;
    nl_checked_destroy(owned);
    owned = NULL;
    diagnostic = (NLCheckDiagnostic){0};
    s = nl_semantic_check_source_fragment(ctx, p.tree, &owned, &diagnostic);
    printf("R402 owned post_failed_check_positive=%d code=%s output=%d\n",
           (int)s,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-",
           owned != NULL);
    if (s != positive || (s == NL_CHECK_OK) != (owned != NULL))
        passed = 0;
cleanup:
    printf("R402 owned survey valid=%d\n", passed);
    nl_checked_destroy(owned);
    nl_semantic_destroy(ctx);
    release_input(&n);
    release_input(&p);
    release_input(&d);
    /* A stable precision barrier is PASS of the probe, NOT owned validation. */
    return passed ? 0 : 1;
}
