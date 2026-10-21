/* Reuse the normal call gate; mutate only genuine test-owned artifacts. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t calls, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && calls++ == fail_at ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_realloc(p, n);
}
static bool valid(const TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}
static bool controls(const char *file)
{
    TestNode n = {0};
    CHECK(node_checked_load(file, &n) && valid(&n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    NLCheckedNodeView *call = NULL;
    size_t id = 0;
    for (size_t i = 1; i <= f->count; ++i)
        if (f->nodes[i - 1].kind == NL_CHECKED_REGISTERED_CALL) {
            call = &f->nodes[i - 1];
            id = i;
            break;
        }
    CHECK(call);
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(f, id);
    CHECK(body);
    const NLCheckedNodeView saved = *call;
    size_t rejected = 0;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(!valid(&n));                                                     \
        restore;                                                               \
        CHECK(valid(&n));                                                      \
        ++rejected;                                                            \
    } while (0)
    REFUSE(call->result_count = 0, *call = saved);
    REFUSE(call->results[0].value = 1, *call = saved);
    REFUSE(call->argument_count = 1, *call = saved);
    REFUSE(call->type = nl_semantic_unit_type(n.context), *call = saved);
    const NLSource *source = body->source;
    REFUSE(body->source = n.entry_source, body->source = source);
    NLFunctionBody *owner = body->body_owner;
    REFUSE(body->body_owner = NULL, body->body_owner = owner);
#undef REFUSE
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(n.context, &before));
    calls = fail_at = 0;
    injecting = true;
    bool pass = valid(&n);
    injecting = false;
    CHECK(pass && calls == 0 && nl_semantic_snapshot(n.context, &after) &&
          memcmp(&before, &after, sizeof(before)) == 0);
    printf("shared gate rejects=%zu allocations=0 state=UNCHANGED\n", rejected);
    node_checked_destroy(&n);
    return true;
}
static bool oom(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(c, before));
    const NLSyntaxTree *units[] = {unit};
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckStatus s = nl_semantic_register_function_unit(c, units, 1, NULL);
        injecting = false;
        if (s == NL_CHECK_OK)
            break;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && test_unchanged(c, before));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("registration OOM positions=%zu rollback=PASS\n", fail_at);
    CHECK(test_state(c, before));
    NLSource *entry = NULL;
    NLSyntaxTree *call = NULL;
    parser = NULL;
    CHECK(nl_source_create("main()", 6, "entry", &entry) == NL_SOURCE_OK &&
          nl_parser_create(entry, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &call, NULL) ==
          NL_PARSE_OK);
    nl_parser_destroy(parser);
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckedFragment *out = NULL;
        NLCheckStatus s = nl_semantic_check_expression(c, call, &out, NULL);
        injecting = false;
        if (s == NL_CHECK_OK) {
            nl_checked_destroy(out);
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && !out && test_unchanged(c, before));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("main-call OOM positions=%zu rollback=PASS artifact=NULL\n",
           fail_at);
    free(before);
    nl_syntax_tree_destroy(call);
    nl_source_destroy(entry);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(unit);
    nl_source_destroy(source);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && controls(argv[1]) && controls(argv[2]) &&
                   oom(argv[1]) && oom(argv[2])
               ? 0
               : 1;
}
