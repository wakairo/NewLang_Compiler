/* R598 frozen independent NON-BLIND short oracle; source-led, not scalar-seeded.
 * Independent evidence branch only. Never compile into production.
 */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
#include <stdint.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && allocation_index++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && allocation_index++ == fail_at ? NULL : __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && allocation_index++ == fail_at ? NULL : __real_calloc(n, s);
}
static bool gate(const TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}
static bool find_addition(const NLCheckedFragment *f,
                          NLCheckedNodeId *out, NLUsizeAdditionView *v)
{
    *out = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_USIZE_ADDITION) {
            CHECK(!*out);
            *out = i;
        }
    return *out && nl_checked_usize_addition_view(f, *out, v);
}
static bool foreign(const char *a, const char *b)
{
    TestNode na = {0}, nb = {0};
    CHECK(node_checked_load(a, &na) && node_checked_load(b, &nb));
    const NLCheckedFragment *fa = nl_checked_call_body(
        na.entry, nl_checked_root(na.entry));
    NLCheckedFragment *fb = (NLCheckedFragment *)nl_checked_call_body(
        nb.entry, nl_checked_root(nb.entry));
    CHECK(fa && fb && fa != fb && fa->context != fb->context);
    NLCheckedNodeId ai, bi;
    NLUsizeAdditionView va, vb, untouched;
    CHECK(find_addition(fa, &ai, &va) && find_addition(fb, &bi, &vb));
    CHECK(va.origin == fa && vb.origin == fb);
    CHECK(va.numbers[0] == 37 && vb.numbers[0] == 37 &&
          va.numbers[1] == 59 && vb.numbers[1] == 59 &&
          va.sum == 96 && vb.sum == 96);
    CHECK(nl_checked_usize_addition_valid(fa, ai, &va) &&
          nl_checked_usize_addition_valid(fb, bi, &vb) &&
          nl_checked_usize_addition_valid(fa, ai, NULL) &&
          nl_checked_usize_addition_valid(fb, bi, NULL));
    CHECK(gate(&na) && gate(&nb));
    printf("FOREIGN two-authentic-contexts: A fragment=%p context=%p node=%zu "
           "operandIDs=%zu,%zu valueIDs=%zu,%zu result=%zu "
           "placeIDs=%zu,%zu; B fragment=%p context=%p node=%zu "
           "operandIDs=%zu,%zu valueIDs=%zu,%zu result=%zu placeIDs=%zu,%zu\n",
           (void *)fa, (void *)fa->context, ai, va.operands[0], va.operands[1],
           va.values[0], va.values[1], va.result, va.places[0], va.places[1],
           (void *)fb, (void *)fb->context, bi, vb.operands[0], vb.operands[1],
           vb.values[0], vb.values[1], vb.result, vb.places[0], vb.places[1]);
    printf("FOREIGN collision node=%d operandIDs=%d valueIDs=%d result=%d "
           "scalar-equal=1\n",
           ai == bi, va.operands[0] == vb.operands[0] &&
           va.operands[1] == vb.operands[1],
           va.values[0] == vb.values[0] && va.values[1] == vb.values[1],
           va.result == vb.result);
    CHECK(!nl_checked_usize_addition_valid(fa, ai, &vb));
    CHECK(!nl_checked_usize_addition_valid(fb, bi, &va));
    CHECK(!nl_checked_usize_addition_valid(fa, bi, &vb));
    CHECK(!nl_checked_usize_addition_valid(fb, ai, &va));
    untouched = va;
    CHECK(!nl_checked_usize_addition_view(fa, 0, &va) &&
          memcmp(&untouched, &va, sizeof va) == 0);
    /* Test-private owned receipt substitution; restore before any destructor. */
    struct NLUsizeAdditionProof *saved = fb->usize_additions;
    fb->usize_additions = fa->usize_additions;
    const bool wrong_owned_valid = nl_checked_usize_addition_valid(fb, bi, NULL);
    const bool wrong_backend_gate = gate(&nb);
    fb->usize_additions = saved;
    CHECK(!wrong_owned_valid && !wrong_backend_gate &&
          nl_checked_usize_addition_valid(fb, bi, &vb) && gate(&nb));
    /* No C17 stream is created unless the normal backend gate succeeds. */
    printf("FOREIGN copied-view A->B/B->A=REJECT; "
           "foreign-owned-issuer/B-backend-gate=REJECT; restored=PASS\n");
    node_checked_destroy(&nb);
    node_checked_destroy(&na);
    return true;
}
static bool parse_file(const char *path, NLSource **source,
                       NLSyntaxTree **tree)
{
    NLParser *p = NULL;
    CHECK(nl_source_load(path, source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(*source, &p) == NL_PARSE_OK);
    NLParseStatus status = nl_parser_parse_function_unit(p, tree, NULL);
    nl_parser_destroy(p);
    return status == NL_PARSE_OK;
}
static bool oom(const char *path)
{
    NLSource *source = NULL, *call_source = NULL;
    NLSyntaxTree *tree = NULL, *call = NULL;
    NLParser *parser = NULL;
    CHECK(parse_file(path, &source, &tree));
    CHECK(nl_source_create("main()", 6, "r598-entry",
                           &call_source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(call_source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &call, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    const NLSyntaxTree *units[] = {tree};
    const size_t reg_samples[] = {0, 1, 2, 3, 9, 27, 60, 83};
    size_t reg_failures = 0, reg_nohit = 0;
    for (size_t i = 0; i < sizeof reg_samples / sizeof reg_samples[0]; ++i) {
        NLSemanticContext *c = NULL;
        NLSemanticSnapshot before = {0}, after = {0};
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        CHECK(nl_semantic_snapshot(c, &before));
        fail_at = reg_samples[i];
        allocation_index = 0;
        injecting = true;
        NLCheckStatus s = nl_semantic_register_function_unit(c, units, 1, NULL);
        injecting = false;
        if (s == NL_CHECK_OK) {
            CHECK(allocation_index <= fail_at);
            ++reg_nohit;
        } else {
            CHECK(s == NL_CHECK_OUT_OF_MEMORY);
            CHECK(nl_semantic_snapshot(c, &after) &&
                  memcmp(&before, &after, sizeof before) == 0);
            ++reg_failures;
            CHECK(nl_semantic_register_function_unit(c, units, 1, NULL) ==
                  NL_CHECK_OK);
        }
        printf("OOM registration fail-at=%zu status=%d allocations=%zu "
               "rollback/retry=%s\n", fail_at, (int)s, allocation_index,
               s == NL_CHECK_OK ? "NO-FAULT" : "PASS");
        nl_semantic_destroy(c);
    }
    const size_t issue_samples[] = {0, 1, 2, 3, 9, 19, 30, 38};
    size_t issue_failures = 0, issue_nohit = 0;
    for (size_t i = 0; i < sizeof issue_samples / sizeof issue_samples[0]; ++i) {
        NLSemanticContext *c = NULL;
        NLSemanticSnapshot before = {0}, after = {0};
        NLCheckedFragment *out = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        CHECK(nl_semantic_register_function_unit(c, units, 1, NULL) == NL_CHECK_OK);
        CHECK(nl_semantic_snapshot(c, &before));
        fail_at = issue_samples[i];
        allocation_index = 0;
        injecting = true;
        NLCheckStatus s = nl_semantic_check_expression(c, call, &out, NULL);
        injecting = false;
        if (s == NL_CHECK_OK) {
            CHECK(out && allocation_index <= fail_at);
            ++issue_nohit;
        } else {
            CHECK(s == NL_CHECK_OUT_OF_MEMORY && out == NULL);
            CHECK(nl_semantic_snapshot(c, &after) &&
                  memcmp(&before, &after, sizeof before) == 0);
            ++issue_failures;
            CHECK(nl_semantic_check_expression(c, call, &out, NULL) ==
                  NL_CHECK_OK && out);
        }
        const NLCheckedFragment *f =
            nl_checked_call_body(out, nl_checked_root(out));
        NLCheckedNodeId id;
        NLUsizeAdditionView v;
        CHECK(f && find_addition(f, &id, &v) &&
              nl_checked_usize_addition_valid(f, id, &v));
        printf("OOM issuer fail-at=%zu status=%d allocations=%zu "
               "out-null-on-failure/retry=%s\n",
               fail_at, (int)s, allocation_index,
               s == NL_CHECK_OK ? "NO-FAULT" : "PASS");
        nl_checked_destroy(out);
        nl_semantic_destroy(c);
    }
    CHECK(reg_failures && issue_failures);
    printf("OOM registration=%zu injected failures/%zu nohit; "
           "issuer=%zu injected failures/%zu nohit; emitter OOM=NOT EXERCISED "
           "(FILE* streaming / no injected output allocator interface)\n",
           reg_failures, reg_nohit, issue_failures, issue_nohit);
    nl_syntax_tree_destroy(call);
    nl_source_destroy(call_source);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3 || !foreign(argv[1], argv[2]) || !oom(argv[1]))
        return 1;
    return 0;
}
