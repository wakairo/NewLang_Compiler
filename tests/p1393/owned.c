#include "../../src/h_origin_internal.h"
#include "../support/semantic_check.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>

typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *entry_syntax;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
} Program;
static void destroy(Program *p)
{
    nl_checked_destroy(p->entry);
    nl_syntax_tree_destroy(p->entry_syntax);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    *p = (Program){0};
}
static NLCheckStatus prepare(const char *path, Program *p)
{
    if (nl_source_load(path, &p->source) != NL_SOURCE_OK ||
        nl_parser_create(p->source, &p->parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(p->parser, &p->unit, NULL) !=
            NL_PARSE_OK ||
        nl_semantic_create(&p->context) != NL_CHECK_OK)
        return NL_CHECK_INTERNAL_ERROR;
    NLSemanticSnapshot before, after;
    if (!nl_semantic_snapshot(p->context, &before))
        return NL_CHECK_INTERNAL_ERROR;
    const NLSyntaxTree *units[] = {p->unit};
    NLCheckStatus s =
        nl_semantic_register_function_unit(p->context, units, 1, NULL);
    if (s != NL_CHECK_OK) {
        if (!nl_semantic_snapshot(p->context, &after) ||
            memcmp(&before, &after, sizeof(before)))
            return NL_CHECK_INTERNAL_ERROR;
        return s;
    }
    if (nl_source_create("main()", 6, "p1387-actual-entry", &p->entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(p->entry_source, &p->entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(p->entry_parser, &p->entry_syntax,
                                            NULL) != NL_PARSE_OK ||
        !nl_semantic_snapshot(p->context, &before))
        return NL_CHECK_INTERNAL_ERROR;
    s = nl_semantic_check_expression(p->context, p->entry_syntax, &p->entry,
                                     NULL);
    if (s != NL_CHECK_OK && (!nl_semantic_snapshot(p->context, &after) ||
                             memcmp(&before, &after, sizeof(before))))
        return NL_CHECK_INTERNAL_ERROR;
    return s;
}
static NLCheckedNodeId match(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_MATCH)
            return i;
    return 0;
}

static bool verify(const char *path)
{
    Program p = {0};
    CHECK(prepare(path, &p) == NL_CHECK_OK);
    CHECK(p.entry && nl_h_custody_validate(p.entry));
    const NLCheckedFragment *top =
        nl_checked_call_body(p.entry, nl_checked_root(p.entry));
    CHECK(top);
    NLCheckedNodeId first = match(top);
    CHECK(first &&
          nl_checked_captured_closure_validate(top, first) == NL_CHECK_OK);
    NLCapturedClosureView proof;
    CHECK(nl_checked_captured_closure_view(top, first, &proof));
    for (size_t i = 0; i <= 5; ++i)
        CHECK(proof.release_worlds[i] == 1);
    const NLCheckedFragment *leaf = top;
    for (size_t i = 0; i < 5; ++i) {
        leaf = nl_checked_match_arm(leaf, match(leaf), 1);
        CHECK(leaf);
    }
    CHECK(nl_checked_captured_change_count(leaf) == 9);
    size_t reads = 0, matches = 0;
    for (size_t i = 1; i <= nl_checked_node_count(leaf); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(leaf, i);
        if (n->kind == NL_CHECKED_LINK_READ)
            ++reads;
        if (n->kind == NL_CHECKED_MATCH)
            ++matches;
    }
    CHECK(reads == 1 && matches == 1);
    printf("CHECK/owned/custody PASS; original release worlds 0..5 once; "
           "reads=%zu matches=%zu\n",
           reads, matches);
    destroy(&p);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && verify(argv[1]) ? 0 : 1;
}
