#include "copy_body_internal.h"
#include "newlang/parser.h"
#include "newlang/source.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Site { NONE, SESSION, PENDING, PRE_CLONE, POST_CLONE };
static enum Site site;
static bool in_open, in_call, in_attach, in_clone;
static unsigned hits, clone_entries, allocations, call_count, attach_count;

void *__real_calloc(size_t, size_t);
void *__wrap_calloc(size_t count, size_t bytes)
{
    if (!hits && ((site == SESSION && in_open) ||
                  (site == PENDING && in_call && !in_clone))) {
        ++hits;
        fprintf(stderr, "R919 REACHED owned calloc site=%d n=%zu size=%zu\n",
                site, count, bytes);
        return NULL;
    }
    return __real_calloc(count, bytes);
}

void *__real_malloc(size_t);
void *__wrap_malloc(size_t bytes)
{
    if (in_clone && !hits && bytes == sizeof(NLSemanticContext) &&
        ((site == PRE_CLONE && in_call) ||
         (site == POST_CLONE && in_attach))) {
        ++allocations;
        ++hits;
        fprintf(stderr,
                "R919 REACHED genuine owned %s clone first malloc=%zu\n",
                in_call ? "PRE" : "POST", bytes);
        return NULL;
    }
    return __real_malloc(bytes);
}

NLCheckStatus __real_nl_sem_clone(const NLSemanticContext *,
                                 NLSemanticContext **);
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *source,
                                 NLSemanticContext **out)
{
    bool relevant = (site == PRE_CLONE && in_call) ||
                    (site == POST_CLONE && in_attach);
    if (relevant)
        ++clone_entries;
    bool saved = in_clone;
    in_clone = relevant;
    NLCheckStatus result = __real_nl_sem_clone(source, out);
    in_clone = saved;
    return result;
}

NLCheckStatus __real_nl_definition_owned_open(NLDefinitionOwned **);
NLCheckStatus __wrap_nl_definition_owned_open(NLDefinitionOwned **out)
{
    in_open = true;
    NLCheckStatus result = __real_nl_definition_owned_open(out);
    in_open = false;
    return result;
}

NLDefinitionOwnedPending *__real_nl_definition_owned_call(
    const NLCheckedFragment *, NLCheckedNodeId, const NLCheckedNodeId[2],
    size_t, const NLSemanticContext *);
NLDefinitionOwnedPending *__wrap_nl_definition_owned_call(
    const NLCheckedFragment *fragment, NLCheckedNodeId call,
    const NLCheckedNodeId args[2], size_t function,
    const NLSemanticContext *context)
{
    ++call_count;
    in_call = true;
    NLDefinitionOwnedPending *p = __real_nl_definition_owned_call(
        fragment, call, args, function, context);
    in_call = false;
    return p;
}

void __real_nl_definition_owned_attach(NLDefinitionOwnedPending *,
                                       const NLCheckedFragment *,
                                       const NLSymbolId[2],
                                       const NLSemanticContext *);
void __wrap_nl_definition_owned_attach(
    NLDefinitionOwnedPending *p, const NLCheckedFragment *body,
    const NLSymbolId formals[2], const NLSemanticContext *world)
{
    if (p)
        ++attach_count;
    in_attach = true;
    __real_nl_definition_owned_attach(p, body, formals, world);
    in_attach = false;
}

typedef struct {
    NLSource *source;
    NLSyntaxTree *tree;
    NLSemanticContext *context;
} Input;

static bool open_input(Input *in, const char *path)
{
    NLParser *parser = NULL;
    NLParseDiagnostic diagnostic = {0};
    bool success = nl_source_load(path, &in->source) == NL_SOURCE_OK &&
                   nl_parser_create(in->source, &parser) == NL_PARSE_OK &&
                   nl_parser_parse_function_unit(parser, &in->tree,
                                                 &diagnostic) == NL_PARSE_OK &&
                   nl_semantic_create(&in->context) == NL_CHECK_OK;
    nl_parser_destroy(parser);
    return success;
}

static void close_input(Input *in)
{
    nl_semantic_destroy(in->context);
    nl_syntax_tree_destroy(in->tree);
    nl_source_destroy(in->source);
}

static bool register_source(Input *in)
{
    const NLSyntaxTree *unit[] = {in->tree};
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(in->context, unit, 1, &diagnostic);
    fprintf(stderr, "R919 OOM source CHECK=%d first=%s\n", status,
            diagnostic.diagnostic.diagnostic.code);
    return status == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
           strcmp(diagnostic.diagnostic.diagnostic.code,
                  "P6-JOIN-PRECISION") == 0;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    if (!strcmp(argv[1], "session"))
        site = SESSION;
    else if (!strcmp(argv[1], "pending"))
        site = PENDING;
    else if (!strcmp(argv[1], "pre"))
        site = PRE_CLONE;
    else if (!strcmp(argv[1], "post"))
        site = POST_CLONE;
    else
        return 2;

    Input in = {0};
    bool ok = open_input(&in, argv[2]);
    if (!ok) {
        fprintf(stderr, "R919 STOP OOM source setup\n");
        close_input(&in);
        return 1;
    }
    NLDefinitionOwned *owner = NULL;
    NLCheckStatus opened = nl_definition_owned_open(&owner);
    if (site == SESSION) {
        ok &= opened == NL_CHECK_OUT_OF_MEMORY && !owner;
        ok &= register_source(&in);
    } else {
        ok &= opened == NL_CHECK_OK && owner;
        if (owner) {
            ok &= register_source(&in);
            nl_definition_owned_close(owner);
            ok &= nl_definition_owned_status(owner) == NL_CHECK_OUT_OF_MEMORY;
            ok &= nl_definition_owned_count(owner) == 0;
            ok &= nl_definition_owned_link(owner, 0) == NULL;
            nl_definition_owned_destroy(owner);
        }
    }
    ok &= hits == 1;
    if (site == PRE_CLONE || site == POST_CLONE)
        ok &= allocations == 1 && clone_entries >= 1;
    if (site == PENDING || site == PRE_CLONE)
        ok &= call_count >= 1;
    if (site == POST_CLONE)
        ok &= attach_count >= 1;
    fprintf(stderr,
            "R919 fault=%d reached=%u clone_entry=%u malloc=%u "
            "call=%u attach=%u\n",
            site, hits, clone_entries, allocations, call_count, attach_count);

    /* No new semantic context. The registration and source inputs remain the
     * original ones; retry only after turning off precisely our fault. */
    site = NONE;
    hits = clone_entries = allocations = call_count = attach_count = 0;
    owner = NULL;
    opened = nl_definition_owned_open(&owner);
    ok &= opened == NL_CHECK_OK && owner != NULL;
    if (owner) {
        ok &= register_source(&in);
        nl_definition_owned_close(owner);
    }
    close_input(&in);
    if (owner) {
        ok &= nl_definition_owned_status(owner) == NL_CHECK_OK;
        ok &= nl_definition_owned_count(owner) == 2;
        unsigned arm_mask = 0;
        for (size_t i = 0; i < nl_definition_owned_count(owner); ++i) {
            const NLDefinitionOwnedLink *view =
                nl_definition_owned_link(owner, i);
            NLDefinitionCallResult result = {0};
            NLCheckStatus valid =
                nl_definition_owned_validate(owner, i, view, &result);
            if (view && view->arm < 2)
                arm_mask |= 1u << view->arm;
            ok &= valid == NL_CHECK_OK && result.old_known &&
                  result.old_value == 216 && result.return_old_verified &&
                  result.loan_closed && result.replacements == 1;
        }
        ok &= arm_mask == 3;
        nl_definition_owned_destroy(owner);
    }
    fprintf(stderr, "R919 same-context rollback/retry success=%d\n", ok);
    return ok ? 0 : 1;
}
