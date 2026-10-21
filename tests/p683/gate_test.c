/* Test-private corruption / allocation faults. No production API or seed. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && allocation_index++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && allocation_index++ == fail_at ? NULL
                                                      : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && allocation_index++ == fail_at ? NULL
                                                      : __real_realloc(p, n);
}
static bool valid(const TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return posix_u8_entry(n->entry, &p);
}
static bool corruption(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n) && valid(&n));
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    NLCheckedNodeView *call = (NLCheckedNodeView *)nl_checked_node_view(
        n.entry, nl_checked_root(n.entry));
    NLCheckedNodeView *root =
        (NLCheckedNodeView *)nl_checked_node_view(body, nl_checked_root(body));
    NLCheckedNodeView *tail =
        (NLCheckedNodeView *)nl_checked_node_view(body, root->tail);
    NLCheckedNodeView *write = NULL, *loan = NULL;
    for (size_t i = 0; i < body->count; ++i) {
        if (body->nodes[i].kind == NL_CHECKED_REPLACE)
            write = &body->nodes[i];
        if (body->nodes[i].kind == NL_CHECKED_LOAN_HEADER)
            loan = &body->nodes[i];
    }
    CHECK(write && loan);
    const NLCheckedNodeView cs = *call, ts = *tail, ws = *write, ls = *loan;
    size_t rejected = 0;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        V0Program p = {.unit = 1,                                              \
                       .u8 = nl_semantic_core_type(n.context, NL_TYPE_U8)};    \
        CHECK(!posix_u8_entry(n.entry, &p));                                   \
        restore;                                                               \
        CHECK(valid(&n));                                                      \
        ++rejected;                                                            \
    } while (0)
    REFUSE(call->results[0].value = 1, *call = cs);
    REFUSE(tail->kind = NL_CHECKED_U8_LITERAL, *tail = ts);
    REFUSE(tail->symbol = 2, *tail = ts);
    REFUSE(tail->value_use = NL_VALUE_CONSUMED, *tail = ts);
    REFUSE(write->field.parent_post_fact = write->field.parent_fact,
           *write = ws);
    REFUSE(write->field.parent = write->field.child, *write = ws);
    REFUSE(loan->loan.scope = 999, *loan = ls);
    REFUSE(loan->loan.body_nonescape_proved = false, *loan = ls);
#undef REFUSE
    allocation_index = fail_at = 0;
    injecting = true;
    bool pass = valid(&n);
    injecting = false;
    CHECK(pass && allocation_index == 0);
    printf("private gate rejects=%zu original=PASS allocations=0\n", rejected);
    node_checked_destroy(&n);
    return true;
}
static bool oom(const char *file)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_load(file, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(c, before));
    const NLSyntaxTree *units[] = {tree};
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckStatus s = nl_semantic_register_function_unit(c, units, 1, NULL);
        injecting = false;
        if (s == NL_CHECK_OK)
            break;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && test_unchanged(c, before));
    }
    CHECK(fail_at && fail_at < 8000);
    printf("registration fail-at-N=%zu rollback=PASS\n", fail_at);
    CHECK(test_state(c, before));
    parser = NULL;
    NLSource *entry = NULL;
    NLSyntaxTree *call = NULL;
    CHECK(nl_source_create("main()", 6, "entry", &entry) == NL_SOURCE_OK &&
          nl_parser_create(entry, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &call, NULL) ==
          NL_PARSE_OK);
    nl_parser_destroy(parser);
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckedFragment *out = NULL;
        NLCheckStatus s = nl_semantic_check_expression(c, call, &out, NULL);
        injecting = false;
        if (s == NL_CHECK_OK) {
            nl_checked_destroy(out);
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && out == NULL &&
              test_unchanged(c, before));
    }
    CHECK(fail_at && fail_at < 8000);
    printf("entry fail-at-N=%zu rollback=PASS artifact=NULL\n", fail_at);
    free(before);
    nl_syntax_tree_destroy(call);
    nl_source_destroy(entry);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && corruption(argv[1]) && oom(argv[1]) ? 0 : 1;
}
