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
    NLCheckStatus status =
        __real_nl_guarded_usize_capture(f, id, s, c, entry, fn, formal);
    fprintf(stderr, "guard capture function=%zu formal=%zu status=%d\n", fn,
            formal, (int)status);
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
    NLCheckStatus status = __real_nl_h_custody_seal_result(p, f, id, c);
    fprintf(stderr, "result seal call=%zu world=%d status=%d\n", id, c != NULL,
            (int)status);
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
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    return (!strcmp(argv[1], "owned")      ? owned(argv[2])
            : !strcmp(argv[1], "negative") ? negative(argv[2])
                                           : false)
               ? 0
               : 1;
}
