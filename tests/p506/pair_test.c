/* Stage 1 actual-source observations and transactions. No Pair certificate or
 * independently validating public join API is claimed by this test. */
#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
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
void *__wrap_calloc(size_t n, size_t s)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_calloc(n, s);
}
static bool path(char *out, size_t cap, const char *dir, const char *name)
{
    const int n = snprintf(out, cap, "%s/%s", dir, name);
    CHECK(n > 0 && (size_t)n < cap);
    return true;
}
static bool syntax(const char *file, NLSource **source, NLSyntaxTree **tree)
{
    NLParser *parser = NULL;
    CHECK(nl_source_load(file, source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(*source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, tree, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    return true;
}
static bool observed(const char *file, size_t expected_bool)
{
    TestNode n = {0};
    CHECK(node_checked_load(file, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *joined = NULL, *read[2] = {NULL, NULL};
    NLCheckedNodeId id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_IF) {
            CHECK(joined == NULL);
            joined = v;
            id = i;
        }
        if (v->kind == NL_CHECKED_FIELD_READ) {
            CHECK(v->field.index < 2 && read[v->field.index] == NULL);
            read[v->field.index] = v;
        }
    }
    CHECK(nl_checked_node_join_validate(f, id, NULL) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(joined != NULL && joined->normal_arms == 2 &&
          joined->type == nl_semantic_core_type(c, NL_TYPE_UNIT) &&
          !joined->terminates && read[0] != NULL && read[1] != NULL);
    const NLCheckedNodeView *condition =
        nl_checked_node_view(f, joined->initializer);
    NLSemanticValueView boolean, left, right;
    CHECK(condition != NULL && condition->result_count == 1 &&
          nl_semantic_value_view(c, condition->results[0].value, &boolean));
    CHECK(boolean.scalar_known && boolean.scalar_value == expected_bool &&
          boolean.type == nl_semantic_core_type(c, NL_TYPE_BOOL));
    CHECK(nl_semantic_value_view(c, read[0]->results[0].value, &left) &&
          nl_semantic_value_view(c, read[1]->results[0].value, &right));
    CHECK(left.type == nl_semantic_core_type(c, NL_TYPE_U8) &&
          !left.scalar_known && left.dependencies == NL_DEPENDENCY_FREE &&
          left.value_dependency_count == 0 && left.domain == 0 &&
          left.allocation_region == 0 && left.reference.place == 0 &&
          right.scalar_known && right.scalar_value == 9);
    const NLCheckedField post = read[0]->field, sibling = read[1]->field;
    CHECK(post.parent == sibling.parent && post.child != sibling.child &&
          post.parent_fact == sibling.parent_fact && post.index == 0 &&
          sibling.index == 1);
    NLSemanticPlaceView root, child;
    CHECK(nl_semantic_place_view(c, post.parent, &root) &&
          nl_semantic_place_view(c, post.child, &child));
    CHECK(root.incarnation == post.parent_incarnation &&
          child.incarnation == post.child_incarnation &&
          child.parent_aggregate == post.parent &&
          child.parent_field_index == 0 && root.governing_domain == 0 &&
          root.placement.region == 0);
    /* Function cleanup has ended the lexical root. Read nodes retain their
     * point-in-time facts; do not mislabel final ended state as live post. */
    CHECK(!root.live && !child.live);
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(c, &snapshot));
    size_t packages = 0;
    for (size_t i = 1; i <= snapshot.values; ++i) {
        NLSemanticValueView aggregate;
        CHECK(nl_semantic_value_view(c, i, &aggregate));
        if (aggregate.type == root.type && aggregate.field_count == 2 &&
            aggregate.fields[0] == post.old_value &&
            aggregate.fields[1] == sibling.old_value) {
            ++packages;
            CHECK(aggregate.carrier == NL_CARRIER_ENDED);
        }
    }
    CHECK(packages == 1);
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, a);
        CHECK(arm != NULL && nl_checked_context(arm) != c);
        const NLSemanticContext *world = nl_checked_context(arm);
        const NLCheckedNodeView *write = NULL, *loan = NULL;
        for (size_t i = 1; i <= nl_checked_node_count(arm); ++i) {
            const NLCheckedNodeView *v = nl_checked_node_view(arm, i);
            if (v->kind == NL_CHECKED_REPLACE) {
                CHECK(write == NULL);
                write = v;
            }
            if (v->kind == NL_CHECKED_LOAN_HEADER) {
                CHECK(loan == NULL);
                loan = v;
            }
        }
        CHECK(write != NULL && loan != NULL);
        const NLCheckedField change = write->field;
        NLSemanticValueView old, incoming;
        NLSemanticPlaceView arm_right;
        NLSemanticScopeView scope;
        CHECK(nl_semantic_value_view(world, change.old_value, &old) &&
              nl_semantic_value_view(world, change.new_value, &incoming) &&
              nl_semantic_place_view(world, sibling.child, &arm_right) &&
              nl_semantic_scope_view(world, loan->loan.scope, &scope));
        CHECK(old.scalar_known && old.scalar_value == 0 &&
              incoming.scalar_known && incoming.scalar_value == 7 + a &&
              change.parent == post.parent && change.child == post.child &&
              change.parent_incarnation == post.parent_incarnation &&
              change.child_incarnation == post.child_incarnation &&
              change.parent_fact != post.parent_fact &&
              change.child_fact != post.child_fact &&
              arm_right.current_fact == sibling.child_fact &&
              loan->loan.place == post.child &&
              loan->loan.access == NL_ACCESS_WRITE && !scope.active &&
              loan->loan.body_nonescape_proved &&
              write->results[0].value == change.old_value);
        CHECK(nl_sem_validate(world) == NL_CHECK_OK);
        printf("arm=%zu root=%zu child=%zu root_fact=%zu->%zu "
               "child_fact=%zu->%zu old=%zu new=%zu scope=%zu closed=1\n",
               a, change.parent, change.child, change.parent_fact,
               change.parent_post_fact, change.child_fact,
               change.child_post_fact, old.scalar_value, incoming.scalar_value,
               loan->loan.scope);
    }
    CHECK(nl_sem_validate(c) == NL_CHECK_OK);
    printf("CHECK=0 owned=1 bool=%zu original_root=%zu root_inc=%zu "
           "left_inc=%zu post_root_fact=%zu post_left_fact=%zu "
           "left_unknown=1 right=9 public_join_validator=NOT_IMPLEMENTED "
           "C17=NOT_RUN NATIVE=NOT_RUN\n",
           expected_bool, post.parent, post.parent_incarnation,
           post.child_incarnation, post.parent_fact, post.child_fact);
    node_checked_destroy(&n);
    return true;
}
static bool rejected(const char *file, NLCheckStatus expected, const char *code)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(syntax(file, &source, &tree));
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before != NULL && test_state(c, before));
    NLFunctionUnitDiagnostic d = {0};
    const NLSyntaxTree *units[] = {tree};
    CHECK(nl_semantic_register_function_unit(c, units, 1, &d) == expected);
    CHECK(strcmp(d.diagnostic.diagnostic.code, code) == 0 &&
          test_unchanged(c, before));
    printf("negative=%s diagnostic=%s public_rollback=PASS\n", file, code);
    free(before);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
static bool oom(const char *file)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(syntax(file, &source, &tree));
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before != NULL && test_state(c, before));
    const NLSyntaxTree *units[] = {tree};
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        const NLCheckStatus status =
            nl_semantic_register_function_unit(c, units, 1, NULL);
        injecting = false;
        if (status == NL_CHECK_OK)
            break;
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && test_unchanged(c, before));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("P506_OOM registration=%zu rollback=PASS\n", fail_at);
    CHECK(test_state(c, before));
    NLSource *call_source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *call = NULL;
    CHECK(nl_source_create("main()", 6, "call", &call_source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(call_source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &call, NULL) ==
          NL_PARSE_OK);
    nl_parser_destroy(parser);
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckedFragment *out = NULL;
        const NLCheckStatus status =
            nl_semantic_check_expression(c, call, &out, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(out);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && out == NULL &&
              test_unchanged(c, before));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("P506_OOM actual_main=%zu output_NULL_and_rollback=PASS\n", fail_at);
    free(before);
    nl_syntax_tree_destroy(call);
    nl_source_destroy(call_source);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    char file[4096];
    if (strcmp(argv[1], "owned") == 0) {
        const char *names[] = {"full_guard_true.nl", "full_guard_false.nl"};
        for (size_t i = 0; i < 2; ++i)
            if (!path(file, sizeof file, argv[2], names[i]) ||
                !observed(file, i == 0 ? 1 : 0))
                return 1;
        return 0;
    }
    if (strcmp(argv[1], "rollback") == 0) {
        const char *names[] = {"wrong_root.nl",   "wrong_field.nl",
                               "extra_change.nl", "invalid_else.nl",
                               "escaping_ref.nl", "live_root_ref.nl"};
        for (size_t i = 0; i < 6; ++i) {
            const NLCheckStatus statuses[] = {NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                                              NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                                              NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                                              NL_CHECK_SEMANTIC_ERROR,
                                              NL_CHECK_SEMANTIC_ERROR,
                                              NL_CHECK_SEMANTIC_UNSUPPORTED};
            const char *codes[] = {
                "P6-JOIN-PRECISION",  "P6-JOIN-PRECISION",
                "P6-JOIN-PRECISION",  "P3-WRITE-REF-REQUIRED",
                "P8-EXIT-DEPENDENCY", "LOCAL-LOAN-PROFILE"};
            if (!path(file, sizeof file, argv[2], names[i]) ||
                !rejected(file, statuses[i], codes[i]))
                return 1;
        }
        return 0;
    }
    if (strcmp(argv[1], "oom") == 0)
        return path(file, sizeof file, argv[2], "full_guard_true.nl") &&
                       oom(file)
                   ? 0
                   : 1;
    return 2;
}
