/* Public API only: every original and witness comes from the unchanged real
 * source unit. Copied diagnostic candidates cannot issue proof or authority. */
#include "../support/semantic_check.h"
#include "newlang/node_join.h"
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
static bool injecting;
static size_t index_alloc, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && index_alloc++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && index_alloc++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t size)
{
    if (injecting && index_alloc++ == fail_at)
        return NULL;
    return __real_calloc(n, size);
}
typedef struct {
    NLSemanticContext *context;
    TestChecked call;
    const NLCheckedFragment *body;
    NLNodeJoinView proof;
} Fixture;
static bool load(const char *path, Fixture *f)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *syntax = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &syntax, NULL) == NL_PARSE_OK);
    CHECK(nl_semantic_create(&f->context) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {syntax};
    CHECK(nl_semantic_register_function_unit(f->context, units, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(syntax);
    nl_parser_destroy(parser);
    nl_source_destroy(source); /* registered body owns its source */
    CHECK(test_run(f->context, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &f->call));
    f->body = nl_checked_call_body(f->call.artifact,
                                   nl_checked_root(f->call.artifact));
    CHECK(f->body != NULL);
    size_t certificates = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f->body); ++i)
        if (nl_checked_node_join_view(f->body, i, &f->proof))
            ++certificates;
    CHECK(certificates == 1);
    return true;
}
static void destroy(Fixture *f)
{
    test_checked_destroy(&f->call);
    nl_semantic_destroy(f->context);
}
static bool valid(const Fixture *f)
{
    CHECK(nl_checked_node_join_validate(f->body, f->proof.match, NULL) ==
          NL_CHECK_OK);
    CHECK(nl_checked_node_join_validate(f->body, f->proof.match, &f->proof) ==
          NL_CHECK_OK);
    const NLNodeJoinView *v = &f->proof;
    NLSemanticPlaceView before, after, sibling_before, sibling_after;
    CHECK(nl_semantic_place_view(v->entry, v->changed, &before) &&
          nl_semantic_place_view(v->post, v->changed, &after));
    CHECK(before.live && after.live &&
          before.incarnation == after.incarnation &&
          before.governing_domain == after.governing_domain &&
          before.current_fact != after.current_fact);
    CHECK(nl_semantic_place_view(v->entry, v->sibling, &sibling_before) &&
          nl_semantic_place_view(v->post, v->sibling, &sibling_after));
    CHECK(sibling_before.current_fact == sibling_after.current_fact &&
          sibling_before.current_value == sibling_after.current_value);
    NLSemanticValueView top;
    CHECK(nl_semantic_value_view(v->post, after.current_value, &top));
    CHECK(top.variant == 0 && top.sum_payload == 0 &&
          top.dependencies == NL_DEPENDENCY_FREE &&
          top.value_dependency_count == 0 && after.payload_occurrence == 0);
    if (before.payload_occurrence != 0) {
        NLSemanticOccurrenceView old;
        CHECK(nl_semantic_occurrence_view(v->post, before.payload_occurrence,
                                          &old) &&
              !old.live);
    }
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticPlaceView a, z;
        CHECK(nl_semantic_place_view(v->entry, v->originals[i], &a) &&
              nl_semantic_place_view(v->post, v->originals[i], &z));
        CHECK(a.live && z.live && a.incarnation == z.incarnation &&
              a.governing_domain == z.governing_domain &&
              test_range_equal(a.placement, z.placement));
    }
    printf("PUBLIC VALID: two original lexical roots, entry/fork/post; "
           "F/P/RF reset, sibling stable, unknown Option; native not tested\n");
    return true;
}
static bool reject(const Fixture *f, const NLNodeJoinView *bad,
                   const char *label)
{
    NLNodeJoinView saved = *bad;
    CHECK(nl_checked_node_join_validate(f->body, f->proof.match, bad) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(memcmp(&saved, bad, sizeof(saved)) == 0);
    CHECK(valid(f));
    printf("REJECT copied actual witness: %s\n", label);
    return true;
}
static bool negatives(const Fixture *f, const char *path)
{
    Fixture foreign = {0};
    CHECK(load(path, &foreign) && valid(&foreign));
    const NLNodeJoinView original = f->proof;
    NLNodeJoinView bad = original;
    bad.entry = nl_checked_context(f->body);
    CHECK(reject(f, &bad, "post-function context forged as entry"));
    bad = original;
    bad.entry = foreign.proof.entry;
    CHECK(reject(f, &bad, "foreign common entry"));
    bad = original;
    bad.post = original.entry;
    CHECK(reject(f, &bad, "stale joined root/field facts"));
    bad = original;
    bad.changed = original.sibling;
    CHECK(reject(f, &bad, "wrong fixed projection"));
    bad = original;
    bad.originals[0] = bad.originals[1];
    CHECK(reject(f, &bad, "duplicated original root"));
    bad = original;
    bad.arms[0] = original.arms[1];
    CHECK(reject(f, &bad, "duplicate arm"));
    bad = original;
    memset(&bad.arms[0], 0, sizeof(bad.arms[0]));
    CHECK(reject(f, &bad, "truncated arm"));
    bad = original;
    bad.arms[0] = original.arms[1];
    bad.arms[1] = original.arms[0];
    CHECK(reject(f, &bad, "reordered arms"));
    bad = original;
    CHECK(original.originals[0] == foreign.proof.originals[0] &&
          original.originals[1] == foreign.proof.originals[1]);
    bad.arms[0].world = foreign.proof.arms[0].world;
    CHECK(reject(f, &bad, "foreign world with SAME NUMERIC original IDs"));
    size_t changed = original.arms[0].operation != 0 ? 0 : 1;
    bad = original;
    bad.arms[changed].operation = 0;
    CHECK(reject(f, &bad, "missing source Change"));
    bad = original;
    bad.arms[changed].change.parent_post_fact =
        original.arms[changed].change.parent_fact;
    CHECK(reject(f, &bad, "changed field but stale enclosing root RF"));
    bad = original;
    ++bad.arms[changed].change.payload_occurrence;
    CHECK(reject(f, &bad, "same locator, forged old Some occurrence"));
    bad = original;
    bad.arms[changed].change.parent = original.originals[1];
    CHECK(reject(f, &bad, "wrong original root"));
    bad = original;
    bad.arms[changed].change.parent_incarnation = 0;
    CHECK(reject(f, &bad, "expired original incarnation"));
    bad = original;
    bad.match = 0;
    CHECK(reject(f, &bad, "foreign match"));
    NLNodeJoinView untouched = original;
    CHECK(!nl_checked_node_join_view(NULL, original.match, &untouched));
    CHECK(memcmp(&untouched, &original, sizeof(original)) == 0);
    CHECK(nl_checked_node_join_validate(NULL, original.match, NULL) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    destroy(&foreign);
    return true;
}
static bool oom(const Fixture *f)
{
    /* calloc/malloc/realloc positions swept in independent revalidation.
     * Compare public state of ALL live proof worlds, including hypothetical
     * arms; this test never uses private layouts or seeds any rights. */
    const NLSemanticContext *worlds[] = {f->context,
                                         f->proof.entry,
                                         f->proof.post,
                                         f->proof.arms[0].entry,
                                         f->proof.arms[0].world,
                                         f->proof.arms[1].entry,
                                         f->proof.arms[1].world,
                                         f->proof.arms[0].before_change,
                                         f->proof.arms[0].after_change,
                                         f->proof.arms[1].before_change,
                                         f->proof.arms[1].after_change};
    const size_t count = sizeof(worlds) / sizeof(worlds[0]);
    size_t live_worlds = 0;
    TestState *states = malloc(sizeof(*states) * count);
    CHECK(states != NULL);
    for (size_t i = 0; i < count; ++i)
        if (worlds[i] != NULL) {
            ++live_worlds;
            CHECK(test_state((NLSemanticContext *)worlds[i], &states[i]));
        }
    bool success = false;
    for (fail_at = 0; fail_at < 4096; ++fail_at) {
        index_alloc = 0;
        injecting = true;
        NLCheckStatus s =
            nl_checked_node_join_validate(f->body, f->proof.match, &f->proof);
        injecting = false;
        for (size_t i = 0; i < count; ++i)
            if (worlds[i] != NULL)
                CHECK(
                    test_unchanged((NLSemanticContext *)worlds[i], &states[i]));
        if (s == NL_CHECK_OK) {
            success = true;
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY);
    }
    free(states);
    CHECK(success);
    printf("public validator OOM positions=%zu all %zu worlds unchanged\n",
           fail_at, live_worlds);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Fixture f = {0};
    if (!load(argv[2], &f) || !valid(&f))
        return 1;
    bool ok = strcmp(argv[1], "negative") == 0 ? negatives(&f, argv[2])
              : strcmp(argv[1], "oom") == 0    ? oom(&f)
                                               : true;
    destroy(&f);
    return ok ? 0 : 1;
}
