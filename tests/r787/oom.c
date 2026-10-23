/* R787: reach exact completed-arm issuer malloc site through real source.
 * No compiler mutation, no forged proof, no P environment. */
#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define R787_VERIFY(condition)                                                 \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "R787 issuer observer failed at %d\\n", __LINE__); \
            abort();                                                           \
        }                                                                      \
    } while (0)

static bool enabled, in_seal, did_fail;
static size_t seen, fail_at, issuer_failures, staged;
void *__real_malloc(size_t);
void *__wrap_malloc(size_t n)
{
    if (enabled && in_seal && seen++ == fail_at) {
        did_fail = true;
        return NULL;
    }
    return __real_malloc(n);
}
NLCheckStatus __real_nl_copy_body_seal_calls(NLCheckedFragment *);
NLCheckStatus __wrap_nl_copy_body_seal_calls(NLCheckedFragment *f)
{
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    bool exact_arm = enabled && root && root->kind == NL_CHECKED_MATCH_ARM;
    TestState *before = NULL;
    if (exact_arm) {
        before = calloc(1, sizeof(*before));
        R787_VERIFY(before &&
                    test_state((NLSemanticContext *)f->context, before));
    }
    size_t start = seen;
    in_seal = exact_arm;
    NLCheckStatus status = __real_nl_copy_body_seal_calls(f);
    in_seal = false;
    if (exact_arm && status == NL_CHECK_OUT_OF_MEMORY) {
        R787_VERIFY(did_fail && seen > start && before &&
                    test_unchanged((NLSemanticContext *)f->context, before));
        ++issuer_failures;
        staged = seen - start - 1;
        for (size_t i = 0; i < f->body_count; ++i) {
            const NLCopyBodyProof *proof = f->bodies[i]->copy_body;
            if (proof && proof->formal_write && proof->sealed)
                R787_VERIFY(proof->caller == f && !proof->caller_nodes &&
                            !proof->caller_nodes_origin &&
                            !proof->caller_count);
        }
    }
    free(before);
    return status;
}
static bool setup(const char *path, NLSemanticContext **context)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
          nl_semantic_create(context) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {unit};
    CHECK(nl_semantic_register_function_unit(*context, units, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool count_calls(const NLCheckedFragment *f, size_t *count, size_t depth)
{
    CHECK(f && depth < 24);
    for (size_t i = 0; i < f->body_count; ++i) {
        const NLCheckedFragment *b = f->bodies[i];
        if (b->copy_body && b->copy_body->formal_write) {
            NLCopyBodyView v = {0};
            NLCopyBodyParameters p = {0};
            CHECK(nl_checked_copy_body_view(f, f->body_calls[i], &v) &&
                  nl_checked_copy_body_parameters(f, f->body_calls[i], &p) &&
                  nl_checked_copy_body_validate(f, f->body_calls[i], &v) ==
                      NL_CHECK_OK &&
                  nl_checked_copy_body_parameters_validate(f, f->body_calls[i],
                                                           &p) == NL_CHECK_OK);
            ++*count;
        }
        CHECK(count_calls(b, count, depth + 1));
    }
    for (size_t i = 0; i < f->arm_count; ++i)
        CHECK(count_calls(f->arms[i].artifact, count, depth + 1));
    return true;
}
static bool exercise(const char *path, size_t expected)
{
    NLSource *entry_source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *entry = NULL;
    CHECK(nl_source_create("main()", 6, "r787-entry", &entry_source) ==
              NL_SOURCE_OK &&
          nl_parser_create(entry_source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_expression_fragment(parser, &entry, NULL) ==
              NL_PARSE_OK);
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    CHECK(setup(path, &context));
    enabled = true;
    fail_at = SIZE_MAX;
    seen = 0;
    did_fail = false;
    NLCheckStatus status =
        nl_semantic_check_expression(context, entry, &checked, NULL);
    enabled = false;
    size_t sites = seen;
    CHECK(status == NL_CHECK_OK && checked && !did_fail && sites == expected);
    nl_checked_destroy(checked);
    nl_semantic_destroy(context);
    for (size_t i = 0; i < sites; ++i) {
        context = NULL;
        checked = NULL;
        CHECK(setup(path, &context));
        TestState *original = calloc(1, sizeof(*original));
        CHECK(original && test_state(context, original));
        fail_at = i;
        seen = issuer_failures = 0;
        did_fail = false;
        enabled = true;
        status = nl_semantic_check_expression(context, entry, &checked, NULL);
        enabled = false;
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && did_fail &&
              issuer_failures == 1 && !checked &&
              test_unchanged(context, original));
        CHECK(nl_semantic_check_expression(context, entry, &checked, NULL) ==
                  NL_CHECK_OK &&
              checked);
        size_t valid_calls = 0;
        CHECK(count_calls(checked, &valid_calls, 0) && valid_calls == expected);
        printf("R787_OOM path=%s exact_arm_malloc_position=%zu/%zu "
               "did_fail=1 staged_before_failure=%zu "
               "transaction_unchanged=1 retry_public0=1\n",
               path, i, sites, staged);
        free(original);
        nl_checked_destroy(checked);
        nl_semantic_destroy(context);
    }
    nl_syntax_tree_destroy(entry);
    nl_parser_destroy(parser);
    nl_source_destroy(entry_source);
    return sites == expected;
}
int main(int argc, char **argv)
{
    return argc == 3 && exercise(argv[1], 2) && exercise(argv[2], 1) ? 0 : 1;
}
