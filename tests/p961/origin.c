#include "../../src/copy_body_internal.h"
#include "../../src/h_origin_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>
#include <string.h>

static NLCheckedFragment *copy_caller(NLCheckedFragment *f)
{
    for (size_t i = 1; i <= f->count; ++i) {
        NLCopyBodyParameters p;
        if (nl_checked_copy_body_parameters(f, i, &p))
            return f;
    }
    for (size_t i = 0; i < f->arm_count; ++i) {
        NLCheckedFragment *found = copy_caller(f->arms[i].artifact);
        if (found)
            return found;
    }
    for (size_t i = 0; i < f->body_count; ++i) {
        NLCheckedFragment *found = copy_caller(f->bodies[i]);
        if (found)
            return found;
    }
    return NULL;
}
static bool registry(const char *path, NLSemanticContext **out)
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
static bool checked(NLSemanticContext *base, TestNode *n)
{
    CHECK(nl_sem_clone(base, &n->context) == NL_CHECK_OK);
    CHECK(nl_source_create("main()", 6, "p961-real-entry", &n->entry_source) ==
          NL_SOURCE_OK);
    NLParser *p = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(nl_parser_create(n->entry_source, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(p, &t, NULL) == NL_PARSE_OK);
    CHECK(nl_semantic_check_expression(n->context, t, &n->entry, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(t);
    nl_parser_destroy(p);
    return true;
}
static bool emit(NLCheckedFragment *f, bool accept, const char *expected,
                 size_t bytes)
{
    char *c = NULL;
    size_t z = 0;
    NLNodeCStatus s = nl_checked_c_node(f, &c, &z);
    if (accept) {
        CHECK(s == NL_NODE_C_OK && c && z);
        CHECK(!expected || (z == bytes && !memcmp(c, expected, z)));
    } else {
        CHECK(s != NL_NODE_C_OK && !c && !z);
    }
    free(c);
    return true;
}
static bool probe(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode a = {0}, b = {0};
    CHECK(registry(path, &base) && checked(base, &a) && checked(base, &b));
    NLCheckedFragment *ma = a.entry->bodies[0], *mb = b.entry->bodies[0];
    NLCheckedFragment *pa = ma->arms[1].artifact, *pb = mb->arms[1].artifact;
    NLCheckedFragment *fa = copy_caller(a.entry), *fb = copy_caller(b.entry);
    CHECK(ma != mb && ma->body_owner && ma->body_owner == mb->body_owner);
    CHECK(fa && fb && fa != fb && fa->source == fb->source &&
          a.entry->h_origin != b.entry->h_origin);
    CHECK(pa->arms[1].artifact == fa && pb->arms[1].artifact == fb);
    CHECK(nl_h_origin_validate(a.entry) && nl_h_origin_validate(b.entry));
    char *baseline = NULL;
    size_t bytes = 0;
    CHECK(nl_checked_c_node(a.entry, &baseline, &bytes) == NL_NODE_C_OK);
    size_t mutations = 0;
#define TRY(change, restore)                                                   \
    do {                                                                       \
        change;                                                                \
        CHECK(emit(a.entry, false, NULL, 0));                                  \
        restore;                                                               \
        CHECK(emit(a.entry, true, baseline, bytes));                           \
        ++mutations;                                                           \
    } while (0)
    NLCheckedArm saved = pa->arms[1];
    TRY(pa->arms[1] = pb->arms[1], pa->arms[1] = saved);
    NLCheckedArm outer = ma->arms[1];
    TRY(ma->arms[1] = mb->arms[1], ma->arms[1] = outer);
    TRY(a.entry->bodies[0] = mb, a.entry->bodies[0] = ma);
    NLCheckedArm first = pa->arms[0];
    TRY((pa->arms[0] = saved, pa->arms[1] = first),
        (pa->arms[0] = first, pa->arms[1] = saved));
    TRY(pa->arms[1].match = 0, pa->arms[1] = saved);
    TRY(pa->arms[1].context_origin = fb->context, pa->arms[1] = saved);
    NLCheckedNodeView arm = fa->nodes[fa->root - 1];
    TRY(fa->nodes[fa->root - 1].variant = 1, fa->nodes[fa->root - 1] = arm);
    NLCheckedNodeView grant = fa->nodes[arm.initializer - 1];
    TRY(fa->nodes[arm.initializer - 1].backing++,
        fa->nodes[arm.initializer - 1] = grant);
    NLCheckedNodeView match = pa->nodes[saved.match - 1];
    TRY(pa->nodes[saved.match - 1].span.start_byte++,
        pa->nodes[saved.match - 1] = match);
    printf("P961 same-plan/source two CHECK roots; %zu origin negatives "
           "reject/C0/restore; healthy bytes=%zu\n",
           mutations, bytes);
    /* Leave A's view tampered. Its canonical original subtree must be freed;
     * B must remain independently owned, live and usable after destroying A. */
    pa->arms[1] = pb->arms[1];
    CHECK(emit(a.entry, false, NULL, 0));
    node_checked_destroy(&a);
    CHECK(emit(b.entry, true, baseline, bytes));
    node_checked_destroy(&b);
    /* Repeat at the root->main boundary, not merely the nested child edge. */
    CHECK(checked(base, &a) && checked(base, &b));
    a.entry->bodies[0] = b.entry->bodies[0];
    CHECK(emit(a.entry, false, NULL, 0));
    node_checked_destroy(&a);
    CHECK(emit(b.entry, true, baseline, bytes));
    node_checked_destroy(&b);
    nl_semantic_destroy(base);
    free(baseline);
    puts("P961 tampered A canonical teardown; B healthy/destroy once");
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
