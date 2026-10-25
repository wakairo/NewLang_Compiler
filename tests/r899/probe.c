#include "copy_body_internal.h"
#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/syntax.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    uint64_t old, sibling, incoming[2];
    size_t field_index, observations, rejected, failed;
    bool seen[2], expect_positive, expect_no_call, expect_oom;
} R899Case;

static R899Case current;

static void problem(const char *reason)
{
    fprintf(stderr, "R899 FAIL %s: %s\n", current.name, reason);
    ++current.failed;
}

static void demand_rejection(const NLDefinitionCallCapture *capture,
                             const NLDefinitionCallView *v, const char *name)
{
    NLDefinitionCallResult result = {0};
    NLCheckStatus s = nl_definition_call_validate(capture, v, &result);
    if (s == NL_CHECK_OK || s == NL_CHECK_OUT_OF_MEMORY) {
        fprintf(stderr, "R899 BAD NEGATIVE %s: %s status=%d\n",
                current.name, name, (int)s);
        ++current.failed;
    } else {
        ++current.rejected;
    }
}

static void graph_mutants(const NLDefinitionCallCapture *capture,
                          const NLDefinitionCallView *v)
{
    NLCheckedFragment *caller = (NLCheckedFragment *)(void *)v->caller;
    NLCheckedFragment *callee = (NLCheckedFragment *)(void *)v->callee;
    if (!caller || !callee || !v->call || v->call > caller->count) {
        problem("no actual checked caller/callee graph");
        return;
    }

    /* Mutate the real checked owner/graph, not merely a copied view. */
    const size_t saved_arm = caller->if_arm_index;
    caller->if_arm_index = 3;
    demand_rejection(capture, v, "actual foreign IF-arm owner");
    caller->if_arm_index = saved_arm;

    NLCheckedNodeView *call = &caller->nodes[v->call - 1];
    const size_t saved_function = call->function;
    call->function = 0;
    demand_rejection(capture, v, "actual registered CALL target tamper");
    call->function = saved_function;

    if (!v->loan || v->loan > caller->count) {
        problem("no real scoped loan checked node");
        return;
    }
    NLCheckedNodeView *loan = &caller->nodes[v->loan - 1];
    const NLPlaceId saved_child = loan->field.child;
    loan->field.child = 0;
    demand_rejection(capture, v, "actual child source claim removed");
    loan->field.child = saved_child;

    bool found_return = false;
    for (size_t i = 0; i < callee->count; ++i) {
        NLCheckedNodeView *n = &callee->nodes[i];
        if (n->kind != NL_CHECKED_RETURN)
            continue;
        const NLValueId saved_return = n->returned.value;
        n->returned.value = 0;
        demand_rejection(capture, v, "actual callee RETURN-old def-use tamper");
        n->returned.value = saved_return;
        found_return = true;
        break;
    }
    if (!found_return)
        problem("no checked callee RETURN node");
}

static void receive(const NLDefinitionCallCapture *capture,
                    const NLDefinitionCallView *v, NLCheckStatus status,
                    void *unused)
{
    (void)unused;
    if (status == NL_CHECK_OUT_OF_MEMORY && !capture && !v) {
        if (!current.expect_oom)
            problem("unexpected host observation OOM");
        ++current.observations;
        return;
    }
    if (current.expect_no_call) {
        problem("no-call control produced a private CALL observation");
        return;
    }
    if (!capture || !v || status != NL_CHECK_OK) {
        problem("missing completed borrowed private observation");
        return;
    }
    if (v->arm > 1 || current.seen[v->arm]) {
        problem("wrong or duplicated IF arm observation");
        return;
    }
    current.seen[v->arm] = true;
    ++current.observations;

    NLDefinitionCallResult output = {0};
    const NLCheckStatus s = nl_definition_call_validate(capture, v, &output);
    if (current.expect_oom) {
        if (s != NL_CHECK_OK)
            problem("unaffected sibling arm failed after host observation OOM");
        return;
    }
    if (!current.expect_positive) {
        if (s == NL_CHECK_OK || s == NL_CHECK_OUT_OF_MEMORY)
            problem("wrong/non-def-use RETURN accepted privately");
        else
            ++current.rejected;
        return;
    }
    if (s != NL_CHECK_OK) {
        fprintf(stderr, "R899 validator reject arm=%zu status=%d line=%zu\n",
                v->arm, (int)s, output.failure_line);
        problem("frozen genuine-old source rejected by private validator");
        return;
    }
    if (!output.pre_verified || !output.return_old_verified ||
        !output.loan_closed || !output.old_known || !output.sibling_known ||
        output.replacements != 1 || !output.returned ||
        output.old_value != current.old ||
        output.sibling != current.sibling ||
        output.incoming != current.incoming[v->arm]) {
        problem("PRE / source old-Copy / sibling / loan mismatch");
        return;
    }
    if (!v->caller || !v->callee || !v->parent || !v->world ||
        !v->pre || !v->post || v->caller == v->callee ||
        v->caller->if_parent != v->parent ||
        v->caller->if_arm_index != v->arm ||
        !v->field.present || v->field.index != current.field_index ||
        !v->field.parent || !v->field.child ||
        v->field.parent == v->other || v->field.child == v->sibling ||
        !v->arguments[0] || !v->arguments[1] || !v->loan ||
        !v->scope || v->pending_count != 1 || v->ordinal != 0) {
        problem("qualified root/child/arm/callee/loan provenance invalid");
        return;
    }
    fprintf(stdout,
            "R899 POS %s arm=%zu PRE-old=%" PRIu64
            " incoming=%" PRIu64 " sibling=%" PRIu64
            " field=%zu CALL=%zu loan=%zu callee RETURN-old=YES\n",
            current.name, v->arm, output.old_value, output.incoming,
            output.sibling, v->field.index, v->call, v->loan);

    NLDefinitionCallView bad = *v;
    bad.arm ^= 1u;
    demand_rejection(capture, &bad, "metadata foreign arm same local IDs");
    bad = *v;
    bad.function = 0;
    demand_rejection(capture, &bad, "metadata wrong callee");
    bad = *v;
    bad.field.index ^= 1u;
    demand_rejection(capture, &bad, "metadata wrong declared field index");
    bad = *v;
    bad.field.child = 0;
    demand_rejection(capture, &bad, "metadata wrong original child");
    bad = *v;
    bad.other = v->field.parent;
    demand_rejection(capture, &bad, "metadata swapped second original");
    bad = *v;
    bad.sibling = v->field.child;
    demand_rejection(capture, &bad, "metadata sibling mismatch");
    bad = *v;
    bad.scope = 0;
    demand_rejection(capture, &bad, "metadata expired loan");
    bad = *v;
    bad.arguments[0] = v->arguments[1];
    demand_rejection(capture, &bad, "metadata actual/formal swap");
    bad = *v;
    bad.call = 0;
    demand_rejection(capture, &bad, "metadata missing CALL");
    graph_mutants(capture, v);
}

#ifdef R899_OOM
static bool inside_begin, armed = true;
static int fail_kind;
static size_t faults_hit;
extern void *__real_calloc(size_t, size_t);
extern void *__real_malloc(size_t);
extern NLDefinitionCallCapture *__real_nl_definition_call_begin(
    const NLCheckedFragment *, NLCheckedNodeId, const NLCheckedNodeId[2],
    size_t, const NLSemanticContext *);

void *__wrap_calloc(size_t count, size_t size)
{
    if (inside_begin && armed && fail_kind == 1) {
        armed = false;
        ++faults_hit;
        return NULL;
    }
    return __real_calloc(count, size);
}

void *__wrap_malloc(size_t size)
{
    if (inside_begin && armed && fail_kind == 2) {
        armed = false;
        ++faults_hit;
        return NULL;
    }
    return __real_malloc(size);
}

NLDefinitionCallCapture *__wrap_nl_definition_call_begin(
    const NLCheckedFragment *caller, NLCheckedNodeId call,
    const NLCheckedNodeId arguments[2], size_t function,
    const NLSemanticContext *context)
{
    inside_begin = true;
    NLDefinitionCallCapture *result = __real_nl_definition_call_begin(
        caller, call, arguments, function, context);
    inside_begin = false;
    return result;
}
#endif

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: r899_probe MODE path.nl\n");
        return 2;
    }
    current.name = argv[1];
    current.expect_positive = !strcmp(argv[1], "meridian") ||
                              !strcmp(argv[1], "relay");
    current.expect_no_call = !strcmp(argv[1], "no-call");
    current.expect_oom = !strcmp(argv[1], "capture-oom") ||
                         !strcmp(argv[1], "clone-oom");
#ifdef R899_OOM
    fail_kind = !strcmp(argv[1], "capture-oom")
                    ? 1
                    : !strcmp(argv[1], "clone-oom") ? 2 : 0;
    current.expect_positive = false;
#else
    if (current.expect_oom) {
        problem("OOM test requires wrapped executable");
        return 1;
    }
#endif
    if (current.expect_positive || current.expect_oom) {
        if (!strcmp(argv[1], "relay")) {
            current.old = 212;
            current.sibling = 43;
            current.incoming[0] = 65;
            current.incoming[1] = 147;
            current.field_index = 1;
        } else {
            current.old = 37;
            current.sibling = 163;
            current.incoming[0] = 81;
            current.incoming[1] = 118;
            current.field_index = 0;
        }
    }
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *context = NULL;
    NLSourceStatus loaded = nl_source_load(argv[2], &source);
    NLParseStatus parsed = NL_PARSE_OK;
    NLCheckStatus checked = NL_CHECK_OK;
    NLParseDiagnostic pdiagnostic = {0};
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLSemanticSnapshot original = {0}, after = {0};

    if (loaded != NL_SOURCE_OK ||
        (parsed = nl_parser_create(source, &parser)) != NL_PARSE_OK ||
        (parsed = nl_parser_parse_function_unit(parser, &tree, &pdiagnostic)) !=
            NL_PARSE_OK ||
        (checked = nl_semantic_create(&context)) != NL_CHECK_OK ||
        !nl_semantic_snapshot(context, &original)) {
        problem("source/parser/context setup failed BEFORE CHECK3");
        goto finish;
    }

    nl_definition_call_observer(receive, NULL);
    const NLSyntaxTree *inputs[] = {tree};
    checked = nl_semantic_register_function_unit(context, inputs, 1,
                                                  &diagnostic);
    nl_definition_call_observer(NULL, NULL);

    printf("R899 %s checked=%d first=%s callbacks=%zu rejects=%zu\n",
           current.name, (int)checked,
           diagnostic.diagnostic.diagnostic.code
               ? diagnostic.diagnostic.diagnostic.code : "(none)",
           current.observations, current.rejected);
    if (!current.expect_no_call) {
        if (checked != NL_CHECK_ANALYSIS_PRECISION_LIMIT ||
            !diagnostic.diagnostic.diagnostic.code ||
            strcmp(diagnostic.diagnostic.diagnostic.code,
                   "P6-JOIN-PRECISION"))
            problem("first CHECK barrier not CHECK3/P6-JOIN-PRECISION");
    }
    if (current.expect_positive &&
        (current.observations != 2 || !current.seen[0] ||
         !current.seen[1] || current.rejected < 22))
        problem("two actual armed private observations/negatives absent");
    if (!current.expect_positive && !current.expect_oom &&
        !current.expect_no_call &&
        (current.observations != 2 || current.rejected != 2))
        problem("wrong-RETURN negative must be observed and privately rejected");
    if (current.expect_no_call && current.observations != 0)
        problem("missing CALL yielded a receipt");
#ifdef R899_OOM
    if (current.expect_oom &&
        (faults_hit != 1 || current.observations < 1))
        problem("capture or PRE-clone allocation OOM was not REACHED");
#endif
    if (!nl_semantic_snapshot(context, &after) ||
        memcmp(&original, &after, sizeof(original)))
        problem("failed definition check committed semantic context state");

    /* Even after the injected host failure, reusing this same context must
     * preserve conservative CHECK3 and must not issue public owner state. */
    if (current.expect_oom) {
        diagnostic = (NLFunctionUnitDiagnostic){0};
        const NLCheckStatus second =
            nl_semantic_register_function_unit(context, inputs, 1,
                                               &diagnostic);
        if (second != NL_CHECK_ANALYSIS_PRECISION_LIMIT ||
            !nl_semantic_snapshot(context, &after) ||
            memcmp(&original, &after, sizeof(original)))
            problem("same-context no-observer retry / rollback failed");
    }
finish:
    nl_definition_call_observer(NULL, NULL);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return current.failed ? 1 : 0;
}
