#ifndef NEWLANG_TEST_REF_JOIN_H
#define NEWLANG_TEST_REF_JOIN_H
#include "semantic_check.h"

typedef struct {
    TestSemantic sem;
    NLTypeId option, read_type;
    NLPlaceId root, a_place, b_place;
    NLScopeId parent_scope, a_scope, b_scope;
    NLValueId a, b;
} JoinFixture;
static inline bool join_ok(JoinFixture *f, const char *text)
{
    TestChecked checked = {0};
    CHECK(test_run(f->sem.context, text, TEST_SOURCE, NL_CHECK_OK, NULL,
                   &checked));
    test_checked_destroy(&checked);
    return true;
}
static inline bool join_binding(JoinFixture *f, const char *name,
                                NLSemanticBindingView *out)
{
    CHECK(nl_semantic_binding_view(
        f->sem.context, nl_semantic_find_binding(f->sem.context, name), out));
    return true;
}
static inline bool join_value(JoinFixture *f, const char *name,
                              NLSemanticValueView *out)
{
    NLSemanticBindingView binding;
    CHECK(join_binding(f, name, &binding));
    CHECK(nl_semantic_value_view(f->sem.context, binding.value, out));
    return true;
}
static inline bool join_create(JoinFixture *f, bool some)
{
    CHECK(test_semantic_create(&f->sem));
    const NLSumVariant variants[] = {{"Some", f->sem.copy}, {"None", 0}};
    CHECK(nl_semantic_register_sum(f->sem.context, "Option", variants, 2,
                                   &f->option) == NL_CHECK_OK);
    NLSymbolId x, y;
    CHECK(nl_semantic_seed_value(f->sem.context, "x", f->sem.copy,
                                 NL_DEPENDENCY_FREE, &x) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f->sem.context, "y", f->sem.copy,
                                 NL_DEPENDENCY_FREE, &y) == NL_CHECK_OK);
    CHECK(join_ok(f,
                  some ? "let state=Option.Some(x)" : "let state=Option.None"));
    NLSemanticBindingView binding;
    CHECK(join_binding(f, "state", &binding));
    f->root = binding.place;
    CHECK(join_binding(f, "x", &binding));
    f->a_place = binding.place;
    CHECK(join_binding(f, "y", &binding));
    f->b_place = binding.place;
    NLSymbolId symbol;
    CHECK(test_reference(f->sem.context, "r", f->root, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &symbol, &f->parent_scope));
    CHECK(test_reference(f->sem.context, "rw", f->root, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &symbol, NULL));
    CHECK(test_reference(f->sem.context, "fallback", f->a_place, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &symbol, &f->a_scope));
    CHECK(join_binding(f, "fallback", &binding));
    f->a = binding.value;
    f->read_type = binding.type;
    CHECK(test_reference(f->sem.context, "other", f->b_place, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &symbol, &f->b_scope));
    CHECK(join_binding(f, "other", &binding));
    f->b = binding.value;
    CHECK(nl_semantic_register_function(f->sem.context, "observe",
                                        &f->read_type, 1,
                                        nl_semantic_unit_type(f->sem.context),
                                        false, false) == NL_CHECK_OK);
    return true;
}
static inline bool join_reject(JoinFixture *f, const char *text,
                               NLCheckStatus status, const char *code)
{
    return test_rejected(f->sem.context, text, TEST_SOURCE, status, code);
}
static inline bool join_facts_equal(NLSemanticValueView a,
                                    NLSemanticValueView b)
{
    CHECK(a.type == b.type && a.reference_count == b.reference_count);
    for (size_t i = 0; i < a.reference_count; ++i)
        CHECK(test_reference_equal(a.references[i], b.references[i]));
    return true;
}
#endif
