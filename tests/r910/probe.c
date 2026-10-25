#include "copy_body_internal.h"
#include "newlang/parser.h"
#include "newlang/source.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Mode {
    CAPTURE,
    PRE,
    FOREIGN,
    BORROW
};
typedef struct {
    const char *file;
    enum Mode mode;
    bool nested, failed, spliced;
    unsigned old, arrival[2], sibling, snapshot_old, snapshot_in;
    size_t callbacks, successes, oom_callbacks, arms;
} Case;
typedef struct {
    NLSource *source;
    NLSyntaxTree *tree;
    NLSemanticContext *context;
} Fixture;
static enum Mode active;
static bool in_begin, in_validate;
static unsigned begins, hit, validation_allocs;
static const NLDefinitionCallCapture *outer_capture;
static NLDefinitionCallView outer_view;
static Case *outer_case;

void *__real_calloc(size_t, size_t);
void *__wrap_calloc(size_t n, size_t size)
{
    if (in_begin && active == CAPTURE && begins == 1 && !hit) {
        ++hit;
        fprintf(stderr, "R910 REACHED host capture calloc n=%zu s=%zu\n", n,
                size);
        return NULL;
    }
    if (in_validate && active == PRE) {
        ++validation_allocs;
        if (!hit) {
            ++hit;
            fprintf(stderr,
                    "R910 REACHED host PRE validator calloc n=%zu s=%zu\n", n,
                    size);
            return NULL;
        }
    }
    return __real_calloc(n, size);
}
NLDefinitionCallCapture *
__real_nl_definition_call_begin(const NLCheckedFragment *, NLCheckedNodeId,
                                const NLCheckedNodeId[2], size_t,
                                const NLSemanticContext *);
NLDefinitionCallCapture *__wrap_nl_definition_call_begin(
    const NLCheckedFragment *caller, NLCheckedNodeId call,
    const NLCheckedNodeId args[2], size_t fn, const NLSemanticContext *context)
{
    ++begins;
    in_begin = true;
    NLDefinitionCallCapture *result =
        __real_nl_definition_call_begin(caller, call, args, fn, context);
    in_begin = false;
    return result;
}

static void error(Case *s, const char *reason)
{
    fprintf(stderr, "R910 STOP %s: %s\n", s->file, reason);
    s->failed = true;
}
static bool open_fixture(Fixture *f, const char *file)
{
    NLParser *parser = NULL;
    NLParseDiagnostic diagnostic = {0};
    bool success = false;
    if (nl_source_load(file, &f->source) != NL_SOURCE_OK ||
        nl_parser_create(f->source, &parser) != NL_PARSE_OK)
        goto done;
    if (nl_parser_parse_function_unit(parser, &f->tree, &diagnostic) !=
            NL_PARSE_OK ||
        nl_semantic_create(&f->context) != NL_CHECK_OK)
        goto done;
    success = true;
done:
    nl_parser_destroy(parser);
    return success;
}
static void close_fixture(Fixture *f)
{
    nl_semantic_destroy(f->context);
    nl_syntax_tree_destroy(f->tree);
    nl_source_destroy(f->source);
}
static void observe(const NLDefinitionCallCapture *,
                    const NLDefinitionCallView *, NLCheckStatus, void *);
static bool register_once(Fixture *f, Case *s)
{
    const NLSyntaxTree *inputs[] = {f->tree};
    NLFunctionUnitDiagnostic d = {0};
    nl_definition_call_observer(observe, s);
    NLCheckStatus result =
        nl_semantic_register_function_unit(f->context, inputs, 1, &d);
    nl_definition_call_observer(NULL, NULL);
    fprintf(stderr, "R910 CHECK status=%d first=%s callbacks=%zu\n", result,
            d.diagnostic.diagnostic.code, s->callbacks);
    return result == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
           strcmp(d.diagnostic.diagnostic.code, "P6-JOIN-PRECISION") == 0;
}
static void splice_foreign(const NLDefinitionCallCapture *foreign_capture,
                           const NLDefinitionCallView *foreign, Case *s)
{
    if (!outer_capture || !outer_case || s->spliced)
        return;
    if (outer_view.call != foreign->call ||
        outer_view.caller == foreign->caller ||
        outer_view.callee == foreign->callee ||
        outer_view.world == foreign->world ||
        nl_checked_source(outer_view.caller) ==
            nl_checked_source(foreign->caller)) {
        error(s, "two genuine owners or colliding numeric NodeIds missing");
        return;
    }
    NLCheckedFragment *real = (NLCheckedFragment *)(void *)outer_view.caller;
    if (real->body_count != 1 || real->bodies[0] != outer_view.callee) {
        error(s, "actual original graph attachment missing");
        return;
    }
    NLCheckedFragment *original = real->bodies[0];
    real->bodies[0] = (NLCheckedFragment *)(void *)foreign->callee;
    NLDefinitionCallResult neg = {0};
    NLCheckStatus rejected =
        nl_definition_call_validate(outer_capture, &outer_view, &neg);
    real->bodies[0] = original;
    NLDefinitionCallResult positive = {0};
    NLCheckStatus recovered =
        nl_definition_call_validate(outer_capture, &outer_view, &positive);
    fprintf(stderr,
            "R910 foreign_graph=REAL foreign_source=%p local_source=%p "
            "colliding_nodeid=%zu rejected=%d recovered=%d\n",
            (const void *)nl_checked_source(foreign->caller),
            (const void *)nl_checked_source(outer_view.caller), foreign->call,
            rejected, recovered);
    if (rejected == NL_CHECK_OK || recovered != NL_CHECK_OK ||
        !positive.return_old_verified || !positive.loan_closed)
        error(s, "foreign-owned callee edge accepted or not recovered");
    else {
        s->spliced = true;
        outer_case->spliced = true;
    }
    (void)foreign_capture;
}
static void observe(const NLDefinitionCallCapture *capture,
                    const NLDefinitionCallView *view, NLCheckStatus status,
                    void *user)
{
    Case *s = user;
    if (!capture || !view) {
        if (s->mode == CAPTURE && status == NL_CHECK_OUT_OF_MEMORY)
            ++s->oom_callbacks;
        else
            error(s, "unexpected incomplete callback");
        return;
    }
    if (status != NL_CHECK_OK || view->arm > 1 ||
        (s->arms & (1u << view->arm))) {
        error(s, "unexpected owner or arm status");
        return;
    }
    ++s->callbacks;
    s->arms |= 1u << view->arm;
    NLDefinitionCallResult result = {0};
    NLCheckStatus checked;
    if (s->mode == PRE && !hit) {
        in_validate = true;
        checked = nl_definition_call_validate(capture, view, &result);
        in_validate = false;
        if (checked != NL_CHECK_OUT_OF_MEMORY || hit != 1) {
            error(s, "PRE validator calloc allocation site not reached");
            return;
        }
        ++s->oom_callbacks;
    }
    checked = nl_definition_call_validate(capture, view, &result);
    if (checked != NL_CHECK_OK || !result.pre_verified ||
        !result.return_old_verified || !result.loan_closed ||
        result.replacements != 1 || !result.old_known ||
        !result.sibling_known || result.old_value != s->old ||
        result.incoming != s->arrival[view->arm] ||
        result.sibling != s->sibling || !view->field.child ||
        !view->field.parent || !view->other ||
        view->field.parent == view->other) {
        error(s, "real PRE/old Copy/original/loan/sibling mismatch");
        return;
    }
    ++s->successes;
    s->snapshot_old = (unsigned)result.old_value;
    s->snapshot_in = (unsigned)result.incoming;
    fprintf(stderr,
            "R910 reached genuine arm=%zu old=%u incoming=%u root=%zu "
            "child=%zu other=%zu\n",
            view->arm, s->snapshot_old, s->snapshot_in, view->field.parent,
            view->field.child, view->other);
    if (s->nested) {
        splice_foreign(capture, view, s);
    } else if (s->mode == FOREIGN && s->callbacks == 1) {
        outer_capture = capture;
        outer_view = *view;
        outer_case = s;
        Fixture foreign = {0};
        Case second = {.file = s->file,
                       .mode = FOREIGN,
                       .nested = true,
                       .old = s->old,
                       .arrival = {s->arrival[0], s->arrival[1]},
                       .sibling = s->sibling};
        if (!open_fixture(&foreign, s->file)) {
            error(s,
                  "could not create second independent real checked context");
        } else {
            if (!register_once(&foreign, &second) || second.failed ||
                second.callbacks != 2 || !second.spliced)
                error(s, "independent source graph not witnessed");
            close_fixture(&foreign);
        }
        nl_definition_call_observer(observe, s);
        outer_capture = NULL;
        outer_case = NULL;
    }
}
static bool run(const char *file, enum Mode selected, unsigned old, unsigned a,
                unsigned b, unsigned sibling)
{
    Fixture f = {0};
    Case s = {.file = file,
              .mode = selected,
              .old = old,
              .arrival = {a, b},
              .sibling = sibling};
    active = selected;
    begins = 0;
    hit = 0;
    validation_allocs = 0;
    if (!open_fixture(&f, file)) {
        error(&s, "parse/semantic setup failed");
        close_fixture(&f);
        return false;
    }
    bool ok = register_once(&f, &s);
    if (selected == CAPTURE) {
        if (hit != 1 || s.oom_callbacks != 1 || s.successes != 1 ||
            s.callbacks != 1)
            error(&s, "actual capture calloc OOM missing");
        /* Do not reset the semantic context: retry same owner/state. */
        active = BORROW;
        Case retry = {.file = file,
                      .mode = BORROW,
                      .old = old,
                      .arrival = {a, b},
                      .sibling = sibling};
        if (!register_once(&f, &retry) || retry.failed ||
            retry.successes != 2 || retry.arms != 3)
            error(&s, "SAME-context capture rollback/retry not reached");
    } else {
        if (s.callbacks != 2 || s.successes != 2 || s.arms != 3)
            error(&s, "both normal arm callbacks not reached");
        if (selected == PRE &&
            (hit != 1 || validation_allocs < 1 || s.oom_callbacks != 1))
            error(&s, "PRE calloc reached count or retry absent");
        if (selected == FOREIGN && !s.spliced)
            error(&s, "foreign independent checked graph not witnessed");
    }
    /* Only owned primitive snapshots survive the borrowed callback. */
    fprintf(stderr, "R910 OBSERVER ENDED old_snapshot=%u in_snapshot=%u\n",
            s.snapshot_old, s.snapshot_in);
    close_fixture(&f);
    return ok && !s.failed;
}
int main(int argc, char **argv)
{
    if (argc != 4)
        return 2;
    enum Mode selected;
    if (!strcmp(argv[1], "capture"))
        selected = CAPTURE;
    else if (!strcmp(argv[1], "pre"))
        selected = PRE;
    else if (!strcmp(argv[1], "foreign"))
        selected = FOREIGN;
    else if (!strcmp(argv[1], "borrow"))
        selected = BORROW;
    else
        return 2;
    bool first = run(argv[2], selected, 181, 57, 96, 43);
    bool second = run(argv[3], selected, 153, 217, 102, 41);
    return first && second ? 0 : 1;
}
