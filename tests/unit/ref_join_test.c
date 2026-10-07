#include "../support/ref_join.h"
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

static bool lifecycle(void)
{
    JoinFixture f = {0};
    CHECK(join_create(&f, true));
    const NLValueId inputs[] = {f.b, f.a, f.a};
    NLValueId result = 0;
    CHECK(nl_semantic_join_references(f.sem.context, inputs, 3, &result) ==
          NL_CHECK_OK);
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(f.sem.context, result, &v));
    CHECK(v.reference_count == 2 && v.reference.place == 0 &&
          v.type == f.read_type && v.carrier == NL_CARRIER_LOOSE &&
          v.references[0].place == f.a_place &&
          v.references[1].place == f.b_place);
    NLSymbolId symbol;
    CHECK(nl_semantic_bind_result(f.sem.context, "joined", result, &symbol) ==
          NL_CHECK_OK);
    CHECK(join_ok(&f, "let copy=joined;"));
    CHECK(join_ok(&f, "observe(copy);"));
    NLSemanticValueView copy;
    CHECK(join_value(&f, "copy", &copy) && join_facts_equal(v, copy));
    const NLTypeId unit = nl_semantic_unit_type(f.sem.context);
    CHECK(nl_semantic_register_function(f.sem.context, "accept_unit", &unit, 1,
                                        unit, false, false) == NL_CHECK_OK);
    CHECK(join_ok(&f, "accept_unit(observe(copy))"));
    /* A copied public view is not mutable context storage. */
    copy.references[0].scope = 0;
    CHECK(join_value(&f, "copy", &copy) && join_facts_equal(v, copy));
    CHECK(nl_semantic_end_scope(f.sem.context, f.b_scope) == NL_CHECK_OK);
    CHECK(join_reject(&f, "copy", NL_CHECK_SEMANTIC_ERROR, "P3-DEAD-SCOPE"));
    TestState before;
    CHECK(test_state(f.sem.context, &before));
    NLValueId sentinel = 999;
    CHECK(nl_semantic_join_references(f.sem.context, &result, 1, &sentinel) ==
          NL_CHECK_SEMANTIC_ERROR);
    CHECK(sentinel == 999 && test_unchanged(f.sem.context, &before));
    nl_semantic_destroy(f.sem.context);
    return true;
}

static bool boundary(void)
{
    JoinFixture f = {0};
    CHECK(join_create(&f, true));
    NLValueId inputs[NL_SEMANTIC_MAX_REF_ALTERNATIVES + 1];
    for (size_t i = 0; i < NL_SEMANTIC_MAX_REF_ALTERNATIVES + 1; ++i) {
        NLScopeId scope;
        NLSymbolId symbol;
        CHECK(nl_semantic_scope(f.sem.context, 0, true, &scope) == NL_CHECK_OK);
        NLSemanticPlaceView place;
        CHECK(nl_semantic_place_view(f.sem.context, f.a_place, &place));
        char name[24];
        (void)snprintf(name, sizeof(name), "scope_ref_%zu", i);
        CHECK(nl_semantic_seed_reference(
                  f.sem.context, name, f.read_type,
                  (NLReferenceFacts){f.a_place, place.incarnation, scope,
                                     NL_PROVENANCE_VALID, true, false, 0},
                  &symbol) == NL_CHECK_OK);
        NLSemanticBindingView binding;
        CHECK(nl_semantic_binding_view(f.sem.context, symbol, &binding));
        inputs[i] = binding.value;
    }
    NLValueId result;
    CHECK(nl_semantic_join_references(f.sem.context, inputs,
                                      NL_SEMANTIC_MAX_REF_ALTERNATIVES,
                                      &result) == NL_CHECK_OK);
    NLSemanticValueView view;
    CHECK(nl_semantic_value_view(f.sem.context, result, &view) &&
          view.reference_count == NL_SEMANTIC_MAX_REF_ALTERNATIVES);
    TestState before;
    CHECK(test_state(f.sem.context, &before));
    NLValueId sentinel = 999;
    CHECK(nl_semantic_join_references(f.sem.context, inputs,
                                      NL_SEMANTIC_MAX_REF_ALTERNATIVES + 1,
                                      &sentinel) == NL_CHECK_RESOURCE_LIMIT);
    CHECK(sentinel == 999 && test_unchanged(f.sem.context, &before));
    CHECK(nl_semantic_join_references(f.sem.context, inputs, 0, &sentinel) ==
          NL_CHECK_INTERNAL_ERROR);
    CHECK(sentinel == 999 && test_unchanged(f.sem.context, &before));
    nl_semantic_destroy(f.sem.context);
    return true;
}

static bool invalid_facts(void)
{
    JoinFixture f = {0};
    CHECK(join_create(&f, true));
    NLSemanticPlaceView p;
    CHECK(nl_semantic_place_view(f.sem.context, f.a_place, &p));
    const NLReferenceFacts bad[] = {{f.a_place, p.incarnation + 1, f.a_scope,
                                     NL_PROVENANCE_VALID, true, false, 0},
                                    {f.a_place, p.incarnation, f.a_scope,
                                     NL_PROVENANCE_UNKNOWN, true, false, 0},
                                    {f.a_place, p.incarnation, f.a_scope,
                                     NL_PROVENANCE_INVALID, true, false, 0},
                                    {f.a_place, p.incarnation, f.a_scope,
                                     NL_PROVENANCE_VALID, false, false, 0}};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        NLSymbolId symbol;
        char name[24];
        (void)snprintf(name, sizeof(name), "bad_%zu", i);
        CHECK(nl_semantic_seed_reference(f.sem.context, name, f.read_type,
                                         bad[i], &symbol) == NL_CHECK_OK);
        NLSemanticBindingView binding;
        CHECK(nl_semantic_binding_view(f.sem.context, symbol, &binding));
        const NLValueId inputs[] = {f.b, binding.value};
        TestState before;
        CHECK(test_state(f.sem.context, &before));
        NLValueId sentinel = 999;
        CHECK(
            nl_semantic_join_references(f.sem.context, inputs, 2, &sentinel) ==
            (i == 1 ? NL_CHECK_ANALYSIS_PRECISION_LIMIT
                    : NL_CHECK_SEMANTIC_ERROR));
        CHECK(sentinel == 999 && test_unchanged(f.sem.context, &before));
    }
    /* An occurrence-bearing payload cannot omit its dependency, even if the
     * underlying place/provenance/incarnation otherwise look valid. */
    NLSemanticPlaceView root;
    NLSemanticOccurrenceView occurrence;
    CHECK(nl_semantic_place_view(f.sem.context, f.root, &root) &&
          nl_semantic_occurrence_view(f.sem.context, root.payload_occurrence,
                                      &occurrence));
    CHECK(nl_semantic_place_view(f.sem.context, occurrence.payload_place, &p));
    NLSymbolId symbol;
    CHECK(nl_semantic_seed_reference(
              f.sem.context, "laundered", f.read_type,
              (NLReferenceFacts){occurrence.payload_place, p.incarnation,
                                 f.parent_scope, NL_PROVENANCE_VALID, true,
                                 false, 0},
              &symbol) == NL_CHECK_OK);
    CHECK(join_reject(&f, "observe(laundered)", NL_CHECK_SEMANTIC_ERROR,
                      "P6-OCCURRENCE-DEPENDENCY"));
    NLSymbolId domain_ref, exclusive_ref;
    CHECK(test_domain_ref(&f.sem, "domain_ref", NL_ACCESS_READ, false,
                          &domain_ref, NULL));
    CHECK(test_reference(f.sem.context, "exclusive_ref", f.b_place, NL_TYPE_REF,
                         NL_ACCESS_READ, true, &exclusive_ref, NULL));
    NLSemanticBindingView capability;
    NLValueId sentinel = 999;
    CHECK(nl_semantic_binding_view(f.sem.context, domain_ref, &capability));
    CHECK(nl_semantic_join_references(f.sem.context, &capability.value, 1,
                                      &sentinel) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(nl_semantic_binding_view(f.sem.context, exclusive_ref, &capability));
    CHECK(nl_semantic_join_references(f.sem.context, &capability.value, 1,
                                      &sentinel) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(sentinel == 999);
    /* Copying an ordinary ref never requires its target T to be Copy. */
    NLSymbolId linear;
    CHECK(nl_semantic_seed_value(f.sem.context, "linear", f.sem.linear,
                                 NL_DEPENDENCY_FREE, &linear) == NL_CHECK_OK);
    CHECK(nl_semantic_binding_view(f.sem.context, linear, &capability));
    NLSymbolId linear_ref;
    CHECK(test_reference(f.sem.context, "linear_ref", capability.place,
                         NL_TYPE_REF, NL_ACCESS_READ, false, &linear_ref,
                         NULL));
    CHECK(nl_semantic_binding_view(f.sem.context, linear_ref, &capability));
    CHECK(nl_semantic_join_references(f.sem.context, &capability.value, 1,
                                      &sentinel) == NL_CHECK_OK);
    NLSemanticValueView joined;
    CHECK(nl_semantic_value_view(f.sem.context, sentinel, &joined) &&
          joined.reference_count == 1);
    nl_semantic_destroy(f.sem.context);
    return true;
}

static bool arm_identity(void)
{
    JoinFixture f = {0};
    CHECK(join_create(&f, false));
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(f.sem.context, &before));
    TestChecked checked = {0};
    CHECK(test_run(f.sem.context,
                   "let chosen=match r {Some(v)=>{v},None=>{fallback}};",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &checked));
    const NLCheckedNodeView *binding = test_root(&checked);
    const NLCheckedFragment *arm =
        nl_checked_match_arm(checked.artifact, binding->initializer, 0);
    CHECK(arm != NULL && nl_checked_context(arm) != f.sem.context);
    const NLCheckedNodeView *pattern =
        nl_checked_node_view(arm, nl_checked_root(arm));
    NLSemanticBindingView pattern_binding;
    NLSemanticValueView hypothetical, result;
    CHECK(nl_semantic_binding_view(nl_checked_context(arm), pattern->symbol,
                                   &pattern_binding));
    CHECK(nl_semantic_value_view(nl_checked_context(arm), pattern_binding.value,
                                 &hypothetical));
    CHECK(hypothetical.reference.scope > before.scopes &&
          hypothetical.reference.occurrence_dependency > before.occurrences &&
          hypothetical.reference.place > before.places);
    CHECK(join_value(&f, "chosen", &result) && result.reference_count == 1 &&
          result.references[0].occurrence_dependency == 0 &&
          result.references[0].scope == f.a_scope);
    CHECK(nl_semantic_snapshot(f.sem.context, &after) &&
          after.occurrences == before.occurrences &&
          after.scopes == before.scopes);
    CHECK(join_ok(&f, "store(rw,Option::Some(x))"));
    /* Owned arm snapshot remains independent after a public transaction. */
    NLSemanticValueView still_hypothetical;
    CHECK(nl_semantic_value_view(nl_checked_context(arm), pattern_binding.value,
                                 &still_hypothetical) &&
          test_reference_equal(hypothetical.reference,
                               still_hypothetical.reference));
    test_checked_destroy(&checked);
    nl_semantic_destroy(f.sem.context);
    return true;
}

static bool oom(void)
{
    JoinFixture f = {0};
    CHECK(join_create(&f, true));
    TestState before;
    CHECK(test_state(f.sem.context, &before));
    const NLValueId inputs[] = {f.a, f.b};
    bool completed = false;
    for (fail_at = 0; fail_at < 4000; ++fail_at) {
        NLValueId sentinel = 999;
        injecting = true;
        allocation_index = 0;
        const NLCheckStatus status =
            nl_semantic_join_references(f.sem.context, inputs, 2, &sentinel);
        injecting = false;
        if (status == NL_CHECK_OK) {
            completed = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && sentinel == 999 &&
              test_unchanged(f.sem.context, &before));
    }
    CHECK(completed && fail_at > 0);
    /* Sweep every clone, arm, alternative-owning value table, checked artifact,
     * and common result/binding allocation on the actual source operation. */
    CHECK(test_state(f.sem.context, &before));
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    const char *text =
        "{let chosen=match r {Some(v)=>{v},None=>{fallback}};observe(chosen);}";
    CHECK(nl_source_create(text, strlen(text), "join-oom", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_source_fragment(parser, &tree, NULL) == NL_PARSE_OK);
    completed = false;
    for (fail_at = 0; fail_at < 4000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        injecting = true;
        allocation_index = 0;
        const NLCheckStatus status = nl_semantic_check_source_fragment(
            f.sem.context, tree, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(artifact);
            completed = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(f.sem.context, &before));
    }
    CHECK(completed && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    nl_semantic_destroy(f.sem.context);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "lifecycle") == 0)
        return lifecycle() ? 0 : 1;
    if (strcmp(argv[1], "boundary") == 0)
        return boundary() ? 0 : 1;
    if (strcmp(argv[1], "invalid") == 0)
        return invalid_facts() ? 0 : 1;
    if (strcmp(argv[1], "identity") == 0)
        return arm_identity() ? 0 : 1;
    if (strcmp(argv[1], "failure") == 0)
        return oom() ? 0 : 1;
    return 2;
}
