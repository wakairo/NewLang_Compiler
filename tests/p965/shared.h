#ifndef P965_SHARED_H
#define P965_SHARED_H
#include "../../src/h_origin_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool node_backend = true;

static inline bool registry(const char *path, NLSemanticContext **out)
{
    NLSource *s = NULL;
    NLParser *p = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(nl_source_load(path, &s) == NL_SOURCE_OK);
    CHECK(nl_parser_create(s, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p, &t, NULL) == NL_PARSE_OK);
    CHECK(nl_semantic_create(out) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {t};
    CHECK(nl_semantic_register_function_unit(*out, inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(t);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    return true;
}
static inline bool entry_tree(TestNode *n, NLSyntaxTree **out)
{
    NLParser *p = NULL;
    CHECK(nl_source_create("main()", 6, "p965-original-check",
                           &n->entry_source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(n->entry_source, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(p, out, NULL) == NL_PARSE_OK);
    nl_parser_destroy(p);
    return true;
}
static inline bool checked(NLSemanticContext *base, TestNode *n)
{
    NLSyntaxTree *t = NULL;
    CHECK(nl_sem_clone(base, &n->context) == NL_CHECK_OK && entry_tree(n, &t));
    CHECK(nl_semantic_check_expression(n->context, t, &n->entry, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(t);
    CHECK(nl_h_custody_validate(n->entry));
    return true;
}
static inline bool emit(NLCheckedFragment *f, bool accept, const char *expected,
                        size_t bytes)
{
    char *c = NULL;
    size_t z = 0;
    NLNodeCStatus s = nl_checked_c_node(f, &c, &z);
    if (accept && !node_backend) {
        CHECK(s == NL_NODE_C_UNSUPPORTED && !c && !z);
    } else if (accept) {
        CHECK(s == NL_NODE_C_OK && c && z);
        CHECK(!expected || (z == bytes && !memcmp(c, expected, z)));
    } else {
        CHECK(s == NL_NODE_C_UNSUPPORTED && !c && !z);
    }
    free(c);
    return true;
}
#endif
