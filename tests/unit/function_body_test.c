#include "../support/function_body.h"
/* W4 uses existing INTERNAL semantic constructors to construct a state which
 * current source syntax cannot spell. No production .c/private field access.
 * The tested exit helper is also called by registration and real direct calls.
 */
#include "../../src/semantic_internal.h"
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}
static bool registration(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLFunctionParameter p[] = {{"x", f.linear}};
    CHECK(register_body(f.context, "missing", p, 1, f.linear, "{unknown}",
                        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-BINDING"));
    CHECK(register_body(f.context, "bad_exit", p, 1, 1, "{}",
                        NL_CHECK_SEMANTIC_ERROR, "P5-SCOPE-OBLIGATION"));
    CHECK(register_body(f.context, "no_tail", NULL, 0, f.copy, "{}",
                        NL_CHECK_SEMANTIC_ERROR, "P8-BODY-RESULT"));
    NLFunctionParameter copy[] = {{"x", f.copy}};
    CHECK(register_body(f.context, "wrong_tail", copy, 1, f.linear, "{x}",
                        NL_CHECK_SEMANTIC_ERROR, "P8-BODY-RESULT"));
    NLFunctionParameter dup[] = {{"x", f.copy}, {"x", f.copy}};
    CHECK(register_body(f.context, "duplicate", dup, 2, 1, "{}",
                        NL_CHECK_SEMANTIC_ERROR, "P8-DUPLICATE-PARAMETER"));
    CHECK(register_body(f.context, "bad_discard", p, 1, 1, "{x;}",
                        NL_CHECK_SEMANTIC_ERROR, "P5-DISCARDABLE-REQUIRED"));
    CHECK(
        register_body(f.context, "noop", NULL, 0, 1, "{}", NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "noop()"));
    CHECK(register_body(f.context, "noop", NULL, 0, 1, "{}",
                        NL_CHECK_SEMANTIC_ERROR, "P8-DUPLICATE-FUNCTION"));
    CHECK(register_body(f.context, "nested_statements", copy, 1, 1,
                        "{x;noop();}", NL_CHECK_OK, NULL));
    CHECK(nl_semantic_register_function(f.context, "unit_fixture", NULL, 0, 1,
                                        false, false) == NL_CHECK_OK);
    CHECK(register_body(f.context, "statements", copy, 1, 1,
                        "{x;unit_fixture();}", NL_CHECK_OK, NULL));
    NLSymbolId x;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    CHECK(body_ok(f.context, "statements(x)"));
    CHECK(body_ok(f.context, "nested_statements(x)"));
    CHECK(register_body(f.context, "capture", NULL, 0, f.copy, "{x}",
                        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-BINDING"));
    CHECK(register_body(f.context, "rebind", copy, 1, f.copy, "{let x=x;x}",
                        NL_CHECK_SEMANTIC_ERROR, "P5-DUPLICATE-BINDING"));
    CHECK(register_body(f.context, "self", copy, 1, f.copy, "{self(x)}",
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P11-RECURSIVE-ANALYSIS-PRECISION"));
    CHECK(register_body(f.context, "nonblock", copy, 1, f.copy, "x",
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "P8-BODY-BLOCK"));
    CHECK(nl_semantic_register_function(f.context, "effects", &f.copy, 1, 1,
                                        true, false) == NL_CHECK_OK);
    CHECK(register_body(f.context, "effect_wrapper", copy, 1, 1,
                        "{effects(x);}", NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-EFFECT-SUMMARY-UNSUPPORTED"));
    CHECK(nl_semantic_register_function(f.context, "hidden", &f.copy, 1, 1,
                                        false, true) == NL_CHECK_OK);
    CHECK(register_body(f.context, "dependency_wrapper", copy, 1, 1,
                        "{hidden(x);}", NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P3-DEPENDENCIES-UNSUPPORTED"));
    /* Caller names and body local names must not be accidentally shared. */
    CHECK(register_body(f.context, "local_copy", copy, 1, f.copy,
                        "{let local=x;local}", NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "let y=local_copy(x)"));
    CHECK(nl_semantic_find_binding(f.context, "local") == 0);
    /* P9 adds return items only under registered function context. */
    const char *unsupported[] = {"fn f() {}"};
    for (size_t i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); ++i) {
        NLSource *source = NULL;
        NLParser *parser = NULL;
        NLSyntaxTree *tree = NULL;
        CHECK(nl_source_create(unsupported[i], strlen(unsupported[i]),
                               "source-boundary", &source) == NL_SOURCE_OK);
        CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
        CHECK(nl_parser_parse_source_fragment(parser, &tree, NULL) !=
                  NL_PARSE_OK &&
              tree == NULL);
        nl_parser_destroy(parser);
        nl_source_destroy(source);
    }
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(body_tree("{}", &source, &tree));
    TestState before;
    CHECK(test_state(f.context, &before));
    CHECK(nl_semantic_register_function_body(
              f.context, "too_many", copy, NL_SEMANTIC_MAX_PARAMETERS + 1, 1,
              tree, NULL) == NL_CHECK_RESOURCE_LIMIT);
    CHECK(test_unchanged(f.context, &before));
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    char calls[600] = "{";
    for (size_t i = 0; i < 65; ++i)
        strcat(calls, "noop();");
    strcat(calls, "}");
    CHECK(test_rejected(f.context, calls, TEST_SOURCE, NL_CHECK_RESOURCE_LIMIT,
                        "P3-RESOURCE-LIMIT"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool exit_compatibility(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId root, global;
    CHECK(nl_semantic_seed_value(f.context, "root", f.copy, NL_DEPENDENCY_FREE,
                                 &root) == NL_CHECK_OK);
    NLSemanticBindingView target;
    CHECK(nl_semantic_binding_view(f.context, root, &target));
    CHECK(test_reference(f.context, "global_ref", target.place, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &global, NULL));
    NLSemanticBindingView binding;
    NLSemanticValueView original;
    CHECK(nl_semantic_binding_view(f.context, global, &binding) &&
          nl_semantic_value_view(f.context, binding.value, &original));
    NLValueId current;
    NLPlaceId carrier;
    CHECK(nl_sem_copy_value(f.context, binding.value, &current) == NL_CHECK_OK);
    CHECK(nl_sem_new_place(f.context, binding.type, 0, true, current,
                           &carrier) == NL_CHECK_OK);
    NLSemanticSnapshot floors;
    CHECK(nl_semantic_snapshot(f.context, &floors));
    NLScopeId local_scope;
    CHECK(nl_semantic_scope(f.context, original.reference.scope, true,
                            &local_scope) == NL_CHECK_OK);
    NLSemanticValueView local = original;
    local.reference.scope = local_scope;
    NLValueId local_value;
    CHECK(nl_sem_new_value(f.context, local, &local_value) == NL_CHECK_OK);
    /* Surviving tail/loose capability is already illegal on this exit. */
    CHECK(nl_sem_function_exit(f.context, floors.scopes, floors.places) ==
          NL_CHECK_SEMANTIC_ERROR);
    nl_sem_end_value(f.context, current);
    CHECK(nl_sem_install(f.context, carrier, local_value, 0) == NL_CHECK_OK);
    CHECK(nl_sem_function_exit(f.context, floors.scopes, floors.places) ==
          NL_CHECK_SEMANTIC_ERROR);
    /* A P7 may-set containing global and local scopes is not reduced to the
     * first scope. Replace the current package with the joined package. */
    NLValueId inputs[] = {binding.value, local_value}, joined;
    CHECK(nl_semantic_join_references(f.context, inputs, 2, &joined) ==
          NL_CHECK_OK);
    nl_sem_end_value(f.context, local_value);
    CHECK(nl_sem_install(f.context, carrier, joined, 0) == NL_CHECK_OK);
    CHECK(nl_sem_function_exit(f.context, floors.scopes, floors.places) ==
          NL_CHECK_SEMANTIC_ERROR);
    CHECK(nl_semantic_end_scope(f.context, local_scope) == NL_CHECK_OK);
    /* Dead scope does not erase an installed dependency. */
    CHECK(nl_sem_function_exit(f.context, floors.scopes, floors.places) ==
          NL_CHECK_SEMANTIC_ERROR);
    nl_sem_end_value(f.context, joined);
    CHECK(nl_sem_copy_value(f.context, binding.value, &current) == NL_CHECK_OK);
    CHECK(nl_sem_install(f.context, carrier, current, 0) == NL_CHECK_OK);
    CHECK(nl_sem_function_exit(f.context, floors.scopes, floors.places) ==
          NL_CHECK_OK);
    CHECK(nl_sum_validate(f.context) == NL_CHECK_OK &&
          nl_raw_validate(f.context) == NL_CHECK_OK);
    nl_semantic_destroy(f.context);
    return true;
}
static bool ownership(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    char name[] = "parameter";
    NLFunctionParameter p[] = {{name, f.copy}};
    CHECK(register_body(f.context, "copy", p, 1, f.copy,
                        "{let local=parameter;local}", NL_CHECK_OK, NULL));
    memset(name, 'z', sizeof(name) - 1);
    NLSymbolId x;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    TestChecked a = {0};
    CHECK(test_run(f.context, "copy(x)", TEST_SOURCE, NL_CHECK_OK, NULL, &a));
    const NLCheckedFragment *body =
        nl_checked_call_body(a.artifact, nl_checked_root(a.artifact));
    CHECK(body != NULL && nl_checked_node_count(body) != 0);
    /* A subsequent successful public clone/commit must not invalidate the
     * immutable retained plan/source or checked evidence. */
    CHECK(body_ok(f.context, "copy(x)"));
    NLSourceView bytes;
    CHECK(
        nl_source_view(nl_checked_source(body), (NLSourceSpan){0, 1}, &bytes) &&
        bytes.bytes[0] == '{');
    nl_semantic_destroy(f.context);
    /* Destruction is safe even when the borrowed PUBLIC context is gone. */
    test_checked_destroy(&a);
    return true;
}
static bool failures(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLFunctionParameter p[] = {{"p", f.linear}};
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(body_tree("{p}", &source, &tree));
    TestState before;
    CHECK(test_state(f.context, &before));
    bool completed = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        injecting = true;
        allocation_index = 0;
        const NLCheckStatus status = nl_semantic_register_function_body(
            f.context, "identity", p, 1, f.linear, tree, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            completed = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
              test_unchanged(f.context, &before));
    }
    CHECK(completed && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    NLSymbolId x;
    CHECK(nl_semantic_seed_value(f.context, "x", f.linear, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    source = NULL;
    tree = NULL;
    CHECK(body_tree("let y=identity(x)", &source, &tree));
    CHECK(test_state(f.context, &before));
    completed = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        injecting = true;
        allocation_index = 0;
        const NLCheckStatus status =
            nl_semantic_check_source_fragment(f.context, tree, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(artifact);
            completed = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(f.context, &before));
    }
    CHECK(completed && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    /* Faults after a caller-visible write, including owned body attachment,
     * must also discard the complete candidate state. */
    NLSymbolId target, ref;
    CHECK(nl_semantic_seed_value(f.context, "target", f.copy,
                                 NL_DEPENDENCY_FREE, &target) == NL_CHECK_OK);
    NLSemanticBindingView binding;
    CHECK(nl_semantic_binding_view(f.context, target, &binding));
    CHECK(test_reference(f.context, "ref", binding.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &ref, NULL));
    CHECK(nl_semantic_binding_view(f.context, ref, &binding));
    NLFunctionParameter writes[] = {{"a", binding.type}, {"v", f.copy}};
    CHECK(register_body(f.context, "write", writes, 2, 1, "{store(a,v);}",
                        NL_CHECK_OK, NULL));
    source = NULL;
    tree = NULL;
    CHECK(body_tree("write(ref,target)", &source, &tree));
    CHECK(test_state(f.context, &before));
    completed = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        injecting = true;
        allocation_index = 0;
        const NLCheckStatus status =
            nl_semantic_check_source_fragment(f.context, tree, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(artifact);
            completed = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(f.context, &before));
    }
    CHECK(completed && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    nl_semantic_destroy(f.context);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "registration") == 0)
        return registration() ? 0 : 1;
    if (strcmp(argv[1], "exit") == 0)
        return exit_compatibility() ? 0 : 1;
    if (strcmp(argv[1], "ownership") == 0)
        return ownership() ? 0 : 1;
    if (strcmp(argv[1], "failure") == 0)
        return failures() ? 0 : 1;
    return 2;
}
