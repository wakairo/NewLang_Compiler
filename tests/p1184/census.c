/* Passive clone lifetime census of the unchanged real CHECK implementation. */
#include "linear_break_internal.h"
#include "newlang/checked.h"
#include "newlang/parser.h"
#include "semantic_internal.h"
#include <assert.h>
#include <stdio.h>

static NLSemanticContext *worlds[1024];
static size_t live, peak, untracked;
static void remember(NLSemanticContext *c)
{
    for (size_t i = 0; i < 1024; ++i)
        if (worlds[i] == c)
            return;
    for (size_t i = 0; i < 1024; ++i)
        if (!worlds[i]) {
            worlds[i] = c;
            ++live;
            if (live > peak)
                peak = live;
            return;
        }
    assert(0);
}
NLCheckStatus __real_nl_semantic_create(NLSemanticContext **);
NLCheckStatus __wrap_nl_semantic_create(NLSemanticContext **out)
{
    NLCheckStatus s = __real_nl_semantic_create(out);
    if (s == NL_CHECK_OK)
        remember(*out);
    return s;
}
NLCheckStatus __real_nl_sem_clone(const NLSemanticContext *,
                                  NLSemanticContext **);
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,
                                  NLSemanticContext **out)
{
    NLCheckStatus s = __real_nl_sem_clone(c, out);
    if (s == NL_CHECK_OK)
        remember(*out);
    return s;
}
void __real_nl_semantic_destroy(NLSemanticContext *);
void __wrap_nl_semantic_destroy(NLSemanticContext *c)
{
    bool found = false;
    for (size_t i = 0; c && i < 1024; ++i)
        if (worlds[i] == c) {
            worlds[i] = NULL;
            --live;
            found = true;
            break;
        }
    if (c && !found)
        ++untracked;
    __real_nl_semantic_destroy(c);
}
/* nl_sem_commit consumes its candidate via a same-TU destroy in semantic.c.
 * GNU --wrap cannot intercept that internal definition reference. Record the
 * real consumption here; the exact original commit still performs all work. */
void __real_nl_sem_commit(NLSemanticContext *, NLSemanticContext *);
void __wrap_nl_sem_commit(NLSemanticContext *c, NLSemanticContext *candidate)
{
    bool found = false;
    for (size_t i = 0; i < 1024; ++i)
        if (worlds[i] == candidate) {
            worlds[i] = NULL;
            --live;
            found = true;
            break;
        }
    assert(found);
    __real_nl_sem_commit(c, candidate);
}
static const NLCheckedFragment *visited[1024];
static size_t graphs, nodes;
static void walk(const NLCheckedFragment *f)
{
    assert(f && graphs < 1024 && f->count <= NL_SEMANTIC_MAX_ENTRIES);
    for (size_t i = 0; i < graphs; ++i)
        if (visited[i] == f)
            return;
    visited[graphs++] = f;
    nodes += f->count;
    for (size_t i = 0; i < f->arm_count; ++i)
        walk(f->arms[i].artifact);
    for (size_t i = 0; i < f->body_count; ++i)
        walk(f->bodies[i]);
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    NLSource *s = NULL, *entry = NULL;
    NLParser *p = NULL, *q = NULL;
    NLSyntaxTree *tree = NULL, *call = NULL;
    NLSemanticContext *world = NULL;
    NLCheckedFragment *f = NULL;
    assert(nl_source_load(argv[1], &s) == NL_SOURCE_OK);
    assert(nl_parser_create(s, &p) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p, &tree, NULL) == NL_PARSE_OK);
    assert(nl_semantic_create(&world) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {tree};
    assert(nl_semantic_register_function_unit(world, units, 1, NULL) ==
           NL_CHECK_OK);
    size_t before = live;
    assert(nl_source_create("main()", 6, "C245 real source census", &entry) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(entry, &q) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(q, &call, NULL) == NL_PARSE_OK);
    assert(nl_semantic_check_expression(world, call, &f, NULL) == NL_CHECK_OK);
    const NLCheckedFragment *body = nl_checked_call_body(f, nl_checked_root(f));
    assert(body);
    for (size_t i = 0; i < body->count; ++i)
        if (body->nodes[i].kind == NL_CHECKED_LOOP) {
            NLLinearBreakView v;
            assert(nl_linear_break_validate(body, i + 1, &v) == NL_CHECK_OK);
        }
    walk(body);
    printf("C245 contexts registered=%zu owned=%zu additional=%zu peak=%zu "
           "untracked=%zu graphs=%zu nodes=%zu\n",
           before, live, live - before, peak, untracked, graphs, nodes);
    nl_checked_destroy(f);
    nl_semantic_destroy(world);
    nl_syntax_tree_destroy(call);
    nl_parser_destroy(q);
    nl_source_destroy(entry);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    printf("C245 context cleanup live=%zu untracked=%zu\n", live, untracked);
    assert(!live && !untracked);
    return 0;
}
