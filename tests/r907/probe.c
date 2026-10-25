#include "copy_body_internal.h"
#include "newlang/parser.h"
#include "newlang/source.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* R907 independent source-owned oracle. No producer harness or source imported.
 * This private observer never mints public authority. */
typedef struct {
    const char *name;
    unsigned old, incoming[2], sibling;
    size_t field_index;
    bool expect_private_ok, exercise_tamper;
    size_t calls, good, rejected;
    unsigned arm_mask;
    bool failed;
} RCase;

static void problem(RCase *c, const char *what)
{
    fprintf(stderr, "R907 FAIL %s: %s\n", c->name, what);
    c->failed = true;
}

static void on_call(const NLDefinitionCallCapture *capture,
                    const NLDefinitionCallView *view, NLCheckStatus status,
                    void *user)
{
    RCase *c = user;
    if (!capture || !view) {
        problem(c, "capture allocation or incomplete callback");
        return;
    }
    ++c->calls;
    if (view->arm >= 2 || c->arm_mask & (1u << view->arm)) {
        problem(c, "duplicate or invalid arm");
        return;
    }
    c->arm_mask |= 1u << view->arm;
    NLDefinitionCallResult result = {0};
    const NLCheckStatus checked =
        nl_definition_call_validate(capture, view, &result);
    fprintf(stderr,
            "R907 %s arm=%zu status=%d private=%d pre=%d oldcopy=%d "
            "loan=%d old=%llu in=%llu sibling=%llu field=%zu "
            "root=%zu child=%zu inc=%zu other=%zu failure_line=%zu\n",
            c->name, view->arm, status, checked, result.pre_verified,
            result.return_old_verified, result.loan_closed,
            (unsigned long long)result.old_value,
            (unsigned long long)result.incoming,
            (unsigned long long)result.sibling, view->field.index,
            view->field.parent, view->field.child,
            view->field.child_incarnation, view->other, result.failure_line);
    if (status != NL_CHECK_OK) {
        problem(c, "callback status not OK");
        return;
    }
    if (!c->expect_private_ok) {
        if (checked == NL_CHECK_OK || !result.pre_verified)
            problem(c, "invalid RETURN not rejected after real PRE");
        else
            ++c->rejected;
        return;
    }
    if (checked != NL_CHECK_OK || !result.pre_verified ||
        !result.return_old_verified || !result.loan_closed ||
        result.replacements != 1 || !result.old_known ||
        !result.sibling_known || result.old_value != c->old ||
        result.incoming != c->incoming[view->arm] ||
        result.sibling != c->sibling ||
        view->field.index != c->field_index || !view->field.present ||
        !view->field.parent || !view->field.child || !view->other ||
        view->other == view->field.parent || !view->field.child_incarnation ||
        !view->field.parent_incarnation || !view->scope ||
        !view->caller || !view->callee || !view->parent || !view->pre ||
        !view->post || !view->world || !view->call ||
        !view->arguments[0] || !view->arguments[1] ||
        !view->formals[0] || !view->formals[1]) {
        problem(c, "positive old-Copy/PRE/original/loan mismatch");
        return;
    }
    ++c->good;
    if (!c->exercise_tamper)
        return;

    /* All mutations are on borrowed, live callback data and restored before
     * returning. A real checked CALL-node corruption is tested separately
     * from weaker observer-view-only corruptions. */
    NLDefinitionCallView wrong = *view;
    NLDefinitionCallResult bad = {0};
    wrong.other = view->field.parent;
    if (nl_definition_call_validate(capture, &wrong, &bad) == NL_CHECK_OK)
        problem(c, "same-root other accepted");
    wrong = *view;
    wrong.field.index = 1u - view->field.index;
    if (nl_definition_call_validate(capture, &wrong, &bad) == NL_CHECK_OK)
        problem(c, "swapped declared index accepted");
    wrong = *view;
    wrong.field.child_incarnation++;
    if (nl_definition_call_validate(capture, &wrong, &bad) == NL_CHECK_OK)
        problem(c, "stale incarnation accepted");
    wrong = *view;
    wrong.arguments[0] = view->arguments[1];
    wrong.arguments[1] = view->arguments[0];
    if (nl_definition_call_validate(capture, &wrong, &bad) == NL_CHECK_OK)
        problem(c, "reordered actual/formal accepted");
    wrong = *view;
    wrong.scope = 0;
    if (nl_definition_call_validate(capture, &wrong, &bad) == NL_CHECK_OK)
        problem(c, "expired loan scope accepted");
    wrong = *view;
    wrong.callee = view->caller;
    if (nl_definition_call_validate(capture, &wrong, &bad) == NL_CHECK_OK)
        problem(c, "caller graph impersonated callee");
    wrong = *view;
    wrong.call++;
    if (nl_definition_call_validate(capture, &wrong, &bad) == NL_CHECK_OK)
        problem(c, "wrong source-owned CALL accepted");

    NLCheckedNodeView *owned =
        (NLCheckedNodeView *)(void *)nl_checked_node_view(view->caller,
                                                        view->call);
    if (!owned) {
        problem(c, "missing checked source CALL");
        return;
    }
    const NLCheckedKind saved = owned->kind;
    owned->kind = NL_CHECKED_U8_LITERAL;
    if (nl_definition_call_validate(capture, view, &bad) == NL_CHECK_OK)
        problem(c, "actual checked graph mutation accepted");
    owned->kind = saved;
    NLDefinitionCallResult recovered = {0};
    if (nl_definition_call_validate(capture, view, &recovered) != NL_CHECK_OK ||
        !recovered.loan_closed || recovered.old_value != c->old)
        problem(c, "post-mutation recovery failed");
}

static bool run_one(const char *path, RCase *scenario, bool expect_registration)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    NLParser *parser = NULL;
    NLSemanticContext *semantic = NULL;
    bool ok = false;
    if (nl_source_load(path, &source) != NL_SOURCE_OK)
        goto done;
    if (nl_parser_create(source, &parser) != NL_PARSE_OK)
        goto done;
    NLParseDiagnostic syntax = {0};
    const NLParseStatus parsed =
        nl_parser_parse_function_unit(parser, &tree, &syntax);
    if (parsed != NL_PARSE_OK) {
        fprintf(stderr, "R907 SOURCE GATE %s parse=%d first=%s\n",
                path, parsed, syntax.diagnostic.code);
        goto done;
    }
    nl_parser_destroy(parser);
    parser = NULL;
    if (nl_semantic_create(&semantic) != NL_CHECK_OK)
        goto done;
    const NLSyntaxTree *inputs[] = {tree};
    NLFunctionUnitDiagnostic diagnostic = {0};
    nl_definition_call_observer(on_call, scenario);
    const NLCheckStatus registered =
        nl_semantic_register_function_unit(semantic, inputs, 1, &diagnostic);
    nl_definition_call_observer(NULL, NULL);
    fprintf(stderr, "R907 SOURCE %s register=%d first=%s callbacks=%zu\n",
            path, registered, diagnostic.diagnostic.diagnostic.code,
            scenario->calls);
    if (expect_registration) {
        if (registered != NL_CHECK_OK)
            goto done;
    } else if (registered != NL_CHECK_ANALYSIS_PRECISION_LIMIT ||
               strcmp(diagnostic.diagnostic.diagnostic.code,
                      "P6-JOIN-PRECISION") != 0) {
        goto done;
    }
    if (!scenario->failed && scenario->calls == (expect_registration ? 0u : 2u) &&
        scenario->arm_mask == (expect_registration ? 0u : 3u) &&
        scenario->good ==
            (expect_registration ? 0u : scenario->expect_private_ok ? 2u : 0u) &&
        scenario->rejected ==
            (!scenario->expect_private_ok && !expect_registration ? 2u : 0u))
        ok = true;
done:
    nl_definition_call_observer(NULL, NULL);
    nl_semantic_destroy(semantic);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    if (!ok)
        fprintf(stderr, "R907 STOP first barrier %s\n", path);
    return ok;
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: r907_probe MODE SOURCE_A SOURCE_B\n");
        return 2;
    }
    const bool positive = !strcmp(argv[1], "positive");
    const bool tamper = !strcmp(argv[1], "tamper");
    const bool negative = !strcmp(argv[1], "negative");
    const bool nocall = !strcmp(argv[1], "nocall");
    if (!positive && !tamper && !negative && !nocall)
        return 2;
    RCase a = {.name = "quartz",
               .old = 181,
               .incoming = {57, 96},
               .sibling = 43,
               .field_index = 0,
               .expect_private_ok = !negative,
               .exercise_tamper = tamper};
    RCase b = {.name = "tide",
               .old = 153,
               .incoming = {217, 102},
               .sibling = 41,
               .field_index = 1,
               .expect_private_ok = !negative,
               .exercise_tamper = tamper};
    const bool first = run_one(argv[2], &a, nocall);
    const bool second = run_one(argv[3], &b, nocall);
    return first && second ? 0 : 1;
}
