#define main p1492_retained_cli_main
#include "../../src/main.c"
#undef main
#include "guarded_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_addition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);            \
            return false;                                                      \
        }                                                                      \
    } while (0)
typedef struct {
    NLSource *source, *call_source;
    NLParser *parser, *call_parser;
    NLSyntaxTree *unit, *call;
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
} Program;
static unsigned fault, phase;
static size_t seen, hits, clone_count;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool fail_allocation(unsigned kind)
{
    bool selected = (fault == 1 && kind == 1) || (fault == 2 && kind == 2) ||
                    (fault == 3 && kind == 3) ||
                    (fault == 4 && phase == 1 && kind == 1) ||
                    (fault == 5 && phase == 2 && kind == 2) ||
                    ((fault == 6 || fault == 7) && phase == 3 && kind == 1);
    if (selected && ++seen == (fault == 7 ? 2U : 1U)) {
        ++hits;
        return true;
    }
    return false;
}
void *__wrap_malloc(size_t n)
{
    return fail_allocation(1) ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t width)
{
    return fail_allocation(2) ? NULL : __real_calloc(n, width);
}
void *__wrap_realloc(void *p, size_t n)
{
    return fail_allocation(3) ? NULL : __real_realloc(p, n);
}
NLCheckStatus __real_nl_sem_clone(const NLSemanticContext *,
                                  NLSemanticContext **);
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,
                                  NLSemanticContext **out)
{
    unsigned saved = phase;
    if (fault == 4 && ++clone_count == 2)
        phase = 1;
    NLCheckStatus status = __real_nl_sem_clone(c, out);
    phase = saved;
    return status;
}
NLCheckStatus
__real_nl_guarded_usize_capture(NLCheckedFragment *, NLCheckedNodeId,
                                const NLSyntaxNode *, const NLSemanticContext *,
                                const NLSemanticContext *, size_t, NLSymbolId);
NLCheckStatus __wrap_nl_guarded_usize_capture(NLCheckedFragment *f,
                                              NLCheckedNodeId id,
                                              const NLSyntaxNode *s,
                                              const NLSemanticContext *c,
                                              const NLSemanticContext *entry,
                                              size_t fn, NLSymbolId formal)
{
    unsigned saved = phase;
    phase = 2;
    NLCheckStatus status =
        __real_nl_guarded_usize_capture(f, id, s, c, entry, fn, formal);
    phase = saved;
    return status;
}
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *p,
                                              NLCheckedFragment *f,
                                              NLCheckedNodeId id,
                                              const NLSemanticContext *c)
{
    unsigned saved = phase;
    phase = 3;
    NLCheckStatus status = __real_nl_h_custody_seal_result(p, f, id, c);
    phase = saved;
    return status;
}
static void destroy(Program *p)
{
    nl_checked_destroy(p->artifact);
    nl_syntax_tree_destroy(p->call);
    nl_parser_destroy(p->call_parser);
    nl_source_destroy(p->call_source);
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    *p = (Program){0};
}
static bool load(const char *path, Program *p)
{
    CHECK(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p->parser, &p->unit, NULL) ==
          NL_PARSE_OK);
    CHECK(nl_semantic_create(&p->context) == NL_CHECK_OK);
    CHECK(nl_source_create("main()", 6, "p1492-actual", &p->call_source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(p->call_source, &p->call_parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(p->call_parser, &p->call, NULL) ==
          NL_PARSE_OK);
    return true;
}
static NLCheckStatus define(Program *p, NLFunctionUnitDiagnostic *d)
{
    const NLSyntaxTree *units[] = {p->unit};
    return nl_semantic_register_function_unit(p->context, units, 1, d);
}
static bool owned(const char *path)
{
    Program p = {0};
    CHECK(load(path, &p));
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLCheckStatus status = define(&p, &diagnostic);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "register status=%d code=%s\n", (int)status,
                diagnostic.diagnostic.diagnostic.code
                    ? diagnostic.diagnostic.diagnostic.code
                    : "(null)");
    CHECK(status == NL_CHECK_OK);
    NLCheckDiagnostic checked = {0};
    status =
        nl_semantic_check_expression(p.context, p.call, &p.artifact, &checked);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "actual status=%d code=%s\n", (int)status,
                checked.diagnostic.code ? checked.diagnostic.code : "(null)");
    CHECK(status == NL_CHECK_OK && p.artifact);
    printf("%s CHECK=%d class=%d\n", path, (int)status,
           (int)nl_checked_proof_class(p.artifact));
    CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_MACHINE_PROVED);
    NLGuardedUsizePlan main, helper;
    CHECK(nl_guarded_usize_call_valid(p.artifact, p.artifact->root, &main));
    CHECK(main.call && !main.formal);
    CHECK(nl_guarded_usize_call_valid(main.body, main.call, &helper));
    CHECK(helper.formal && helper.conditional && helper.addition &&
          !helper.call);
    const NLCheckedFragment *arm =
        nl_checked_if_arm(helper.body, helper.conditional, 0);
    const NLCheckedNodeView *add = nl_checked_node_view(arm, helper.addition);
    NLSemanticValueView result;
    CHECK(
        add && !add->scalar_result.known && !add->scalar_result.value &&
        nl_semantic_value_view(arm->context, add->results[0].value, &result) &&
        !result.scalar_known && !result.scalar_value);
    NLUsizeAdditionView untouched, before;
    memset(&untouched, 0x71, sizeof(untouched));
    memcpy(&before, &untouched, sizeof(before));
    CHECK(!nl_checked_usize_addition_view(arm, helper.addition, &untouched) &&
          !memcmp(&untouched, &before, sizeof(before)) &&
          !nl_checked_usize_addition_valid(arm, helper.addition, NULL));
    printf("actual/formal=%zu; bound=%zu step=%zu; Unknown result=%zu; "
           "public Known receipt remains unavailable\n",
           helper.formal, helper.limit, helper.step, add->results[0].value);
    destroy(&p);
    return true;
}
static bool negative(const char *path)
{
    Program p = {0};
    CHECK(load(path, &p));
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus status = define(&p, &d);
    printf("%s rejection status=%d code=%s\n", path, (int)status,
           d.diagnostic.diagnostic.code ? d.diagnostic.diagnostic.code
                                        : "(null)");
    CHECK(status == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          d.diagnostic.diagnostic.code &&
          !strcmp(d.diagnostic.diagnostic.code, "USIZE-ADDITION-PRECISION"));
    destroy(&p);
    return true;
}
static bool ready(const char *path, Program *p)
{
    CHECK(load(path, p));
    NLFunctionUnitDiagnostic d = {0};
    CHECK(define(p, &d) == NL_CHECK_OK);
    CHECK(nl_semantic_check_expression(p->context, p->call, &p->artifact,
                                       NULL) == NL_CHECK_OK);
    CHECK(nl_checked_proof_class(p->artifact) == NL_CHECKED_MACHINE_PROVED);
    return true;
}
static bool rejected(Program *p)
{
    NLGuardedUsizePlan out;
    memset(&out, 0x59, sizeof(out));
    NLGuardedUsizePlan before;
    memcpy(&before, &out, sizeof(before));
    CHECK(nl_checked_proof_class(p->artifact) == NL_CHECKED_PROOF_INVALID);
    CHECK(!nl_guarded_usize_call_valid(p->artifact, p->artifact->root, &out) &&
          !memcmp(&before, &out, sizeof(out)));
    FILE *stream = tmpfile();
    CHECK(stream && !guarded_usize_emit(stream, p->artifact) &&
          ftell(stream) == 0);
    CHECK(!fclose(stream));
    return true;
}
static bool tamper(const char *path)
{
    Program p = {0}, other = {0};
    CHECK(ready(path, &p) && ready(path, &other));
    NLGuardedUsizePlan main, helper;
    CHECK(nl_guarded_usize_call_valid(p.artifact, p.artifact->root, &main) &&
          nl_guarded_usize_call_valid(main.body, main.call, &helper));
    NLCheckedFragment *body = (NLCheckedFragment *)helper.body;
    NLCheckedFragment *arm =
        (NLCheckedFragment *)nl_checked_if_arm(body, helper.conditional, 0);
    NLCheckedNodeView *add = &arm->nodes[helper.addition - 1];
    size_t cases = 0;
#define MUTATE(item, replacement)                                              \
    do {                                                                       \
        unsigned char saved[sizeof(item)];                                     \
        memcpy(saved, &(item), sizeof(item));                                  \
        (item) = (replacement);                                                \
        CHECK(rejected(&p));                                                   \
        memcpy(&(item), saved, sizeof(item));                                  \
        ++cases;                                                               \
    } while (0)
    MUTATE(p.artifact->context, other.context);
    MUTATE(arm->if_arm_index, 1);
    MUTATE(add->scalar_result.known, true);
    MUTATE(add->results[0].value, add->results[0].value + 1);
    MUTATE(arm->context->values[add->results[0].value - 1].scalar_known, true);
    MUTATE(
        arm->context
            ->places[arm->context->bindings[helper.formal - 1].view.place - 1]
            .current_fact,
        0);
    MUTATE(p.context->functions[helper.function - 1].parameters, NULL);
    MUTATE(p.context->functions[helper.function - 1].result,
           nl_semantic_core_type(p.context, NL_TYPE_U8));
    MUTATE(body->nodes[body->root - 1].results[0].value, 1);
    MUTATE(
        ((NLCheckedFragment *)main.body)->nodes[main.call - 1].first_argument,
        1);
#undef MUTATE
    CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_MACHINE_PROVED);
    /* Mutable display children never transfer B's actual ownership to A.
     * Destroy A with the foreign replacement still present. */
    p.artifact->bodies[0] = other.artifact->bodies[0];
    CHECK(rejected(&p));
    destroy(&p);
    CHECK(nl_checked_proof_class(other.artifact) == NL_CHECKED_MACHINE_PROVED &&
          nl_guarded_usize_call_valid(other.artifact, other.artifact->root,
                                      &main));
    destroy(&other);
    printf("tamper %zu + foreign-child destroy/reuse rejected; C17 untouched\n",
           cases);
    return true;
}
static bool oom(const char *path)
{
    for (unsigned selected = 1; selected <= 7; ++selected) {
        Program p = {0};
        CHECK(load(path, &p));
        NLFunctionUnitDiagnostic d = {0};
        CHECK(define(&p, &d) == NL_CHECK_OK);
        const NLSemanticContext before = *p.context;
        seen = hits = clone_count = 0;
        fault = selected;
        NLCheckStatus status =
            nl_semantic_check_expression(p.context, p.call, &p.artifact, NULL);
        fault = phase = 0;
        printf("OOM case=%u actual_hits=%zu status=%d out_NULL=%d "
               "context_unchanged=%d\n",
               selected, hits, (int)status, p.artifact == NULL,
               !memcmp(p.context, &before, sizeof(before)));
        CHECK(hits == 1 && status == NL_CHECK_OUT_OF_MEMORY && !p.artifact &&
              !memcmp(p.context, &before, sizeof(before)) &&
              nl_sem_validate(p.context) == NL_CHECK_OK);
        CHECK(nl_semantic_check_expression(p.context, p.call, &p.artifact,
                                           NULL) == NL_CHECK_OK &&
              nl_checked_proof_class(p.artifact) == NL_CHECKED_MACHINE_PROVED);
        destroy(&p);
    }
    return true;
}
static bool named(const char *path, const char *target)
{
    static const char sentinel[] = "P1492 original named output\n";
    FILE *f = fopen(target, "wb");
    CHECK(f && fwrite(sentinel, 1, sizeof(sentinel), f) == sizeof(sentinel) &&
          !fclose(f));
    seen = hits = 0;
    fault = 1;
    int status = emit_c17_to(target, path);
    fault = phase = 0;
    char actual[sizeof(sentinel)] = {0};
    f = fopen(target, "rb");
    CHECK(f && fread(actual, 1, sizeof(actual), f) == sizeof(actual) &&
          !fclose(f));
    printf("OOM case=8 named_status=%d actual_hits=%zu no_clobber=%d\n", status,
           hits, !memcmp(actual, sentinel, sizeof(actual)));
    CHECK(status == 1 && hits == 1 &&
          !memcmp(actual, sentinel, sizeof(actual)));
    CHECK(emit_c17_to(target, path) == 0);
    CHECK(remove(target) == 0);
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "named"))
        return named(argv[2], argv[3]) ? 0 : 1;
    if (argc != 3)
        return 2;
    return (!strcmp(argv[1], "owned")      ? owned(argv[2])
            : !strcmp(argv[1], "negative") ? negative(argv[2])
            : !strcmp(argv[1], "tamper")   ? tamper(argv[2])
            : !strcmp(argv[1], "oom")      ? oom(argv[2])
                                           : false)
               ? 0
               : 1;
}
