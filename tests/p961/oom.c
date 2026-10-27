#include "../../src/h_origin_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
typedef struct {
    void *result;
    size_t bytes;
    char kind;
} Allocation;
static Allocation trace[32768];
static size_t calls, at, count;
static bool injecting, recording, hit;
static bool fail(void)
{
    if (!injecting)
        return false;
    bool reached = calls++ == at;
    hit |= reached;
    return reached;
}
static void record(void *result, size_t bytes, char kind)
{
    if (recording && count < sizeof(trace) / sizeof(*trace))
        trace[count++] = (Allocation){result, bytes, kind};
}
void *__wrap_malloc(size_t bytes)
{
    void *p = fail() ? NULL : __real_malloc(bytes);
    record(p, bytes, 'm');
    return p;
}
void *__wrap_calloc(size_t n, size_t bytes)
{
    void *p = fail() ? NULL : __real_calloc(n, bytes);
    record(p, n * bytes, 'c');
    return p;
}
void *__wrap_realloc(void *old, size_t bytes)
{
    void *p = fail() ? NULL : __real_realloc(old, bytes);
    record(p, bytes, 'r');
    return p;
}
static size_t selected[256], selected_count;
static bool choose(const void *p, const char *label)
{
    if (!p)
        return true;
    size_t last = SIZE_MAX;
    for (size_t i = 0; i < count; ++i)
        if (trace[i].result == p)
            last = i;
    CHECK(last != SIZE_MAX);
    for (size_t i = 0; i < selected_count; ++i)
        if (selected[i] == last)
            return true;
    CHECK(selected_count < sizeof(selected) / sizeof(*selected));
    selected[selected_count++] = last;
    printf("P961 actual CHECK allocation %s position=%zu kind=%c bytes=%zu\n",
           label, last, trace[last].kind, trace[last].bytes);
    return true;
}
static bool collect(NLCheckedFragment *f, bool root)
{
    CHECK(choose(f, "fragment"));
    if (!root && f->destroy_context)
        CHECK(choose(f->context, "owned-world"));
    CHECK(choose(f->arms, "arm-save") && choose(f->bodies, "body-save") &&
          choose(f->body_calls, "call-save"));
    for (size_t i = 0; i < f->arm_count; ++i)
        CHECK(collect(f->arms[i].artifact, false));
    for (size_t i = 0; i < f->body_count; ++i)
        CHECK(collect(f->bodies[i], false));
    return true;
}
static bool probe(const char *path)
{
    /* Register once without injected faults; then execute the actual same
     * source-issued entry from independent registry-only clones. */
    NLSource *s = NULL;
    NLParser *p = NULL;
    NLSyntaxTree *unit = NULL, *entry = NULL;
    NLSemanticContext *base = NULL, *c = NULL;
    NLCheckedFragment *f = NULL;
    char *text = NULL;
    bool ok = false;
    /* Helpers own no resources; failures here must dispose all probe owners.
     * Keep the failing condition visible, then leave through one cleanup path.
     */
#define REQUIRE(condition)                                                     \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);    \
            goto cleanup;                                                      \
        }                                                                      \
    } while (0)
    REQUIRE(nl_source_load(path, &s) == NL_SOURCE_OK);
    REQUIRE(nl_parser_create(s, &p) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_function_unit(p, &unit, NULL) == NL_PARSE_OK);
    REQUIRE(nl_semantic_create(&base) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {unit};
    REQUIRE(nl_semantic_register_function_unit(base, inputs, 1, NULL) ==
            NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    unit = NULL;
    nl_parser_destroy(p);
    p = NULL;
    nl_source_destroy(s);
    s = NULL;
    REQUIRE(nl_source_create("main()", 6, "p961-oom-entry", &s) ==
            NL_SOURCE_OK);
    REQUIRE(nl_parser_create(s, &p) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(p, &entry, NULL) ==
            NL_PARSE_OK);
    REQUIRE(nl_sem_clone(base, &c) == NL_CHECK_OK);
    at = SIZE_MAX;
    injecting = recording = true;
    NLCheckStatus status = nl_semantic_check_expression(c, entry, &f, NULL);
    injecting = recording = false;
    REQUIRE(status == NL_CHECK_OK && f && f->h_origin && !hit &&
            count == calls);
    REQUIRE(choose(f->h_origin, "root-origin-calloc") && collect(f, true));
    REQUIRE(nl_h_origin_validate(f));
    nl_checked_destroy(f);
    f = NULL;
    nl_semantic_destroy(c);
    c = NULL;
    for (size_t i = 0; i < selected_count; ++i) {
        REQUIRE(nl_sem_clone(base, &c) == NL_CHECK_OK);
        f = NULL;
        calls = 0;
        at = selected[i];
        hit = false;
        injecting = true;
        status = nl_semantic_check_expression(c, entry, &f, NULL);
        injecting = false;
        REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && !f);
        REQUIRE(hit && calls > at && nl_packet_same_entry(base, c));
        REQUIRE(nl_semantic_check_expression(c, entry, &f, NULL) ==
                    NL_CHECK_OK &&
                f && nl_h_origin_validate(f));
        size_t bytes = 0;
        REQUIRE(nl_checked_c_node(f, &text, &bytes) == NL_NODE_C_OK && bytes);
        free(text);
        text = NULL;
        nl_checked_destroy(f);
        f = NULL;
        nl_semantic_destroy(c);
        c = NULL;
        printf("P961 OOM position=%zu unpublished/rollback/retry PASS\n", at);
    }
    printf("P961 reached CHECK OOM positions=%zu (close is infallible)\n",
           selected_count);
    ok = true;
cleanup:
    injecting = recording = false;
    free(text);
    nl_checked_destroy(f);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(entry);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    nl_semantic_destroy(base);
#undef REQUIRE
    return ok;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
