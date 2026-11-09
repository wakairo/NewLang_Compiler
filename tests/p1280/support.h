#ifndef P1280_SUPPORT_H
#define P1280_SUPPORT_H
#define main p1280_cli_main
#include "main.c"
#undef main
#include "h_origin_internal.h"
#include "newlang/copy_body.h"
#include <assert.h>

typedef struct {
    NLSource *source, *call;
    NLParser *parser, *call_parser;
    NLSyntaxTree *tree, *call_tree;
    NLSemanticContext *context;
    NLCheckedFragment *entry, *read_owner;
    NLCheckedNodeId read;
    size_t reads;
} PInput;

static void p_load(PInput *p, const char *path)
{
    NLParseDiagnostic d = {0};
    NLFunctionUnitDiagnostic u = {0};
    assert(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    assert(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p->parser, &p->tree, &d) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&p->context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {p->tree};
    assert(nl_semantic_register_function_unit(p->context, inputs, 1, &u) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "P1280 genuine main", &p->call) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(p->call, &p->call_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(p->call_parser, &p->call_tree,
                                               &d) == NL_PARSE_OK);
}
static NLCheckStatus p_check(PInput *p)
{
    NLCheckDiagnostic d = {0};
    return nl_semantic_check_expression(p->context, p->call_tree, &p->entry,
                                        &d);
}
static void p_dispose(PInput *p)
{
    nl_checked_destroy(p->entry);
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->call_tree);
    nl_parser_destroy(p->call_parser);
    nl_source_destroy(p->call);
    nl_syntax_tree_destroy(p->tree);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
}
#endif
