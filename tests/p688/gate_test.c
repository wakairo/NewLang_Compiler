/* Shared backend gates only; no alternate export-body validator. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "source_call.h"

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
static bool controls(const char *file, const char *expression)
{
    TestNode n = {0};
    CHECK(selected_load(file, expression, &n) && valid(&n));
    NLCheckedNodeView *call = &n.entry->nodes[n.entry->root - 1];
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    CHECK(body);
    const NLCheckedNodeView saved = *call;
    size_t rejects = 0;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(!valid(&n));                                                     \
        restore;                                                               \
        CHECK(valid(&n));                                                      \
        ++rejects;                                                             \
    } while (0)
    REFUSE(call->results[0].value = 0, *call = saved);
    REFUSE(call->result_count = 0, *call = saved);
    REFUSE(call->argument_count = 1, *call = saved);
    REFUSE(call->body_backed = false, *call = saved);
    const NLSource *source = body->source;
    REFUSE(body->source = n.entry_source, body->source = source);
    NLFunctionBody *owner = body->body_owner;
    REFUSE(body->body_owner = NULL, body->body_owner = owner);
    const NLSemanticContext *world = body->context;
    TestNode foreign = {0};
    CHECK(selected_load(file, expression, &foreign));
    REFUSE(body->context = foreign.context, body->context = world);
    node_checked_destroy(&foreign);
#undef REFUSE
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(n.context, &before));
    calls = fail_at = 0;
    injecting = true;
    bool pass = valid(&n);
    injecting = false;
    CHECK(pass && calls == 0 && nl_semantic_snapshot(n.context, &after) &&
          memcmp(&before, &after, sizeof(before)) == 0);
    printf("selected shared gate rejects=%zu allocations=0 state=UNCHANGED\n",
           rejects);

    /* The public issuer must roll back each failed fresh selected call. */
    TestState *state = malloc(sizeof(*state));
    CHECK(state && test_state(n.context, state));
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_parser_create(n.entry_source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_expression_fragment(parser, &tree, NULL) ==
              NL_PARSE_OK);
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckedFragment *out = NULL;
        NLCheckStatus s =
            nl_semantic_check_expression(n.context, tree, &out, NULL);
        injecting = false;
        if (s == NL_CHECK_OK) {
            nl_checked_destroy(out);
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && !out &&
              test_unchanged(n.context, state));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("selected-call OOM=%zu rollback=PASS artifact=NULL\n", fail_at);
    free(state);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    node_checked_destroy(&n);
    return true;
}
static bool driver_oom(const char *file, const char *selector)
{
    /* Linux test-only stream redirection: check the actual driver including
     * new call-text allocation. Never rewrite or include emitted C. */
    FILE *saved = stdout;
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        FILE *sink = tmpfile();
        CHECK(sink);
        stdout = sink;
        calls = 0;
        injecting = true;
        int status = compile_v0(file, false, selector);
        injecting = false;
        long bytes = ftell(sink);
        stdout = saved;
        CHECK(fclose(sink) == 0);
        if (status == 0) {
            CHECK(bytes > 0);
            break;
        }
        CHECK((status == 1 || status == 3) && bytes == 0);
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("driver allocation positions=%zu zero-output failures=PASS\n",
           fail_at);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && controls(argv[1], "low_threshold()") &&
                   controls(argv[2], "upper_limit()") &&
                   driver_oom(argv[1], "low_threshold") &&
                   driver_oom(argv[2], "upper_limit")
               ? 0
               : 1;
}
