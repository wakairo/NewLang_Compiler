#include "../support/semantic_check.h"

#include <stdint.h>
#include <stdlib.h>

/* Linux linker wrapping is test-only; production has no allocator hook. */
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t size)
{
    if (injecting && allocation_index++ == fail_at) {
        return NULL;
    }
    return __real_malloc(size);
}
void *__wrap_realloc(void *storage, size_t size)
{
    if (injecting && allocation_index++ == fail_at) {
        return NULL;
    }
    return __real_realloc(storage, size);
}

static bool type_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    const char *texts[] = {"CopyT",
                           "ptr<CopyT>",
                           "ptr<ptr<CopyT>>",
                           "ref<read,CopyT>",
                           "ref<write,CopyT>",
                           "exclusive ref<read,CopyT>",
                           "exclusive ref<write,CopyT>"};
    const NLSemanticTypeKind kinds[] = {
        NL_TYPE_NOMINAL, NL_TYPE_PTR, NL_TYPE_PTR, NL_TYPE_REF,
        NL_TYPE_REF,     NL_TYPE_REF, NL_TYPE_REF};
    for (size_t i = 0; i < sizeof(texts) / sizeof(texts[0]); ++i) {
        TestChecked a = {0}, b = {0};
        CHECK(test_run(f.context, texts[i], TEST_TYPE, NL_CHECK_OK, NULL, &a));
        CHECK(test_run(f.context, texts[i], TEST_TYPE, NL_CHECK_OK, NULL, &b));
        CHECK(test_root(&a)->type == test_root(&b)->type);
        NLSemanticTypeView type;
        CHECK(nl_semantic_type_view(f.context, test_root(&a)->type, &type));
        CHECK(type.kind == kinds[i] && type.is_copy == (i < 5) &&
              type.is_discardable);
        if (i >= 3) {
            CHECK(type.access ==
                  (i == 4 || i == 6 ? NL_ACCESS_WRITE : NL_ACCESS_READ));
            CHECK(type.is_exclusive == (i >= 5));
        }
        test_checked_destroy(&a);
        test_checked_destroy(&b);
    }
    NLSemanticTypeView type;
    CHECK(nl_semantic_type_view(f.context, f.discardable, &type) &&
          !type.is_copy && type.is_discardable);
    CHECK(nl_semantic_type_view(f.context, f.linear, &type) && !type.is_copy &&
          !type.is_discardable);
    CHECK(nl_semantic_type_view(f.context, nl_semantic_domain_type(f.context),
                                &type) &&
          !type.is_copy && !type.is_discardable);
    NLTypeId slot = 0;
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_SLOT, f.linear,
                                    NL_ACCESS_READ, false,
                                    &slot) == NL_CHECK_OK);
    CHECK(nl_semantic_type_view(f.context, slot, &type) && !type.is_copy &&
          !type.is_discardable);
    TestState before;
    CHECK(test_state(f.context, &before));
    NLTypeId untouched = SIZE_MAX;
    CHECK(nl_semantic_nominal(f.context, "Invalid", true, false, &untouched) ==
              NL_CHECK_SEMANTIC_ERROR &&
          untouched == SIZE_MAX);
    CHECK(test_unchanged(f.context, &before));
    CHECK(test_rejected(f.context, "UnknownType", TEST_TYPE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-TYPE"));
    nl_semantic_destroy(f.context);
    return true;
}

/* Type registry contract only; no runtime root/Allocation/domain is seeded.
 * Source H completion establishes the ptr target used by the opt-in triad. */
static bool original_grant_registry_tests(void)
{
    NLSemanticContext *c = NULL;
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *syntax = NULL;
    const char text[] = "struct Node{next:Option<ptr<Node>>,payload:u8}";
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(nl_source_create(text, sizeof(text) - 1, "registry-H", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &syntax, NULL) == NL_PARSE_OK);
    const NLSyntaxTree *inputs[] = {syntax};
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    TestChecked checked = {0};
    CHECK(test_run(c, "ptr<Node>", TEST_TYPE, NL_CHECK_OK, NULL, &checked));
    const NLTypeId pointer = test_root(&checked)->type;
    test_checked_destroy(&checked);
    NLAggregateField fields[] = {
        {"p", pointer},
        {"a", nl_semantic_core_type(c, NL_TYPE_ALLOCATION)},
        {"d", nl_semantic_domain_type(c)}};
    TestState before;
    CHECK(test_state(c, &before));
    NLTypeId result = SIZE_MAX;
#ifndef NEWLANG_EXPERIMENTAL_ORIGINAL_GRANT
    CHECK(nl_semantic_register_aggregate(c, "Parcel", fields, 3, &result) ==
              NL_CHECK_SEMANTIC_UNSUPPORTED &&
          result == SIZE_MAX);
    CHECK(test_unchanged(c, &before));
#else
    CHECK(nl_semantic_register_aggregate(c, "Parcel", fields, 3, &result) ==
          NL_CHECK_OK);
    NLSemanticTypeView t;
    CHECK(nl_semantic_type_view(c, result, &t) && t.field_count == 3 &&
          !t.is_copy && !t.is_discardable && !t.layout_known);
    NLSemanticSnapshot state;
    CHECK(nl_semantic_snapshot(c, &state) && state.values == 0 &&
          state.backing_regions == 0 && state.domains == 0);
    CHECK(test_state(c, &before));
    result = SIZE_MAX;
    CHECK(nl_semantic_register_aggregate(c, "Parcel", fields, 3, &result) ==
              NL_CHECK_SEMANTIC_ERROR &&
          result == SIZE_MAX);
    CHECK(test_unchanged(c, &before));
    fields[1].name = "p";
    CHECK(nl_semantic_register_aggregate(c, "Duplicate", fields, 3, &result) ==
              NL_CHECK_SEMANTIC_ERROR &&
          result == SIZE_MAX);
    CHECK(test_unchanged(c, &before));
    fields[1].name = "a";
    CHECK(nl_semantic_register_aggregate(c, "Partial", fields, 2, &result) ==
              NL_CHECK_SEMANTIC_UNSUPPORTED &&
          result == SIZE_MAX);
    CHECK(test_unchanged(c, &before));
    bool success = false;
    for (size_t nth = 0; nth < 256; ++nth) {
        fail_at = nth;
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status = nl_semantic_register_aggregate(
            c, "FailureTrial", fields, 3, &result);
        injecting = false;
        if (status == NL_CHECK_OK) {
            success = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && result == SIZE_MAX);
        CHECK(test_unchanged(c, &before));
    }
    CHECK(success);
#endif
    nl_syntax_tree_destroy(syntax);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}

static bool value_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x, y;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f.context, "y", f.linear, NL_DEPENDENCY_FREE,
                                 &y) == NL_CHECK_OK);
    NLSemanticBindingView original;
    CHECK(nl_semantic_binding_view(f.context, x, &original));
    NLValueId copied[2];
    for (size_t i = 0; i < 2; ++i) {
        TestChecked c = {0};
        CHECK(test_run(f.context, "x", TEST_EXPRESSION, NL_CHECK_OK, NULL, &c));
        copied[i] = test_root(&c)->results[0].value;
        CHECK(copied[i] != original.value &&
              test_root(&c)->value_use == NL_VALUE_COPIED);
        NLSemanticValueView v;
        CHECK(nl_semantic_value_view(f.context, copied[i], &v) &&
              v.carrier == NL_CARRIER_LOOSE);
        test_checked_destroy(&c);
        CHECK(nl_semantic_value_view(f.context, copied[i], &v) &&
              v.carrier ==
                  NL_CARRIER_LOOSE); /* artifact destruction is not discard */
    }
    CHECK(copied[0] != copied[1]);
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(f.context, x, &b) &&
          b.availability == NL_AVAILABLE && b.value == original.value);
    CHECK(nl_semantic_binding_view(f.context, y, &original));
    TestChecked c = {0};
    CHECK(test_run(f.context, "let moved = y", TEST_BINDING, NL_CHECK_OK, NULL,
                   &c));
    CHECK(nl_semantic_binding_view(f.context, y, &b) &&
          b.availability == NL_CONSUMED);
    CHECK(nl_semantic_binding_view(f.context, test_root(&c)->symbol, &b) &&
          b.value == original.value && b.availability == NL_AVAILABLE);
    CHECK(nl_checked_node_view(c.artifact, test_root(&c)->initializer)
              ->value_use == NL_VALUE_CONSUMED);
    test_checked_destroy(&c);
    CHECK(test_rejected(f.context, "y", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-USE-AFTER-CONSUME"));
    CHECK(test_rejected(f.context, "let moved = x", TEST_BINDING,
                        NL_CHECK_SEMANTIC_ERROR, "P3-DUPLICATE-BINDING"));
    CHECK(test_rejected(f.context, "missing", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-BINDING"));
    const NLTypeId copy_params[] = {f.copy}, linear_params[] = {f.linear};
    CHECK(nl_semantic_register_function(f.context, "copy_call", copy_params, 1,
                                        f.copy, false, false) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "consume", linear_params, 1,
                                        nl_semantic_unit_type(f.context), false,
                                        false) == NL_CHECK_OK);
    CHECK(test_run(f.context, "copy_call(x)", TEST_EXPRESSION, NL_CHECK_OK,
                   NULL, &c));
    CHECK(test_root(&c)->kind == NL_CHECKED_REGISTERED_CALL &&
          test_root(&c)->result_count == 1);
    CHECK(nl_checked_node_view(c.artifact, test_root(&c)->first_argument)
              ->value_use == NL_VALUE_COPIED);
    test_checked_destroy(&c);
    CHECK(test_run(f.context, "consume(moved)", TEST_EXPRESSION, NL_CHECK_OK,
                   NULL, &c));
    CHECK(test_root(&c)->result_count == 0);
    test_checked_destroy(&c);
    CHECK(test_rejected(f.context, "unknown(x)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-CALLEE"));
    CHECK(test_rejected(f.context, "take()", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-ARITY"));
    CHECK(test_rejected(f.context, "take(x,x,x)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-ARITY"));
    /* Prelude resolution is separate from value names, with no
     * overload/shadowing rules. */
    NLSymbolId take_name;
    CHECK(nl_semantic_seed_value(f.context, "take", f.copy, NL_DEPENDENCY_FREE,
                                 &take_name) == NL_CHECK_OK);
    CHECK(test_run(f.context, "take", TEST_EXPRESSION, NL_CHECK_OK, NULL, &c));
    test_checked_destroy(&c);
    CHECK(test_rejected(f.context, "take()", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-ARITY"));
    nl_semantic_destroy(f.context);
    return true;
}

static bool compatibility_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x, read, write;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(f.context, x, &b));
    CHECK(test_reference(f.context, "r", b.place, NL_TYPE_REF, NL_ACCESS_READ,
                         false, &read, NULL));
    CHECK(test_reference(f.context, "w", b.place, NL_TYPE_REF, NL_ACCESS_WRITE,
                         false, &write, NULL));
    NLTypeId rt, wt;
    CHECK(nl_semantic_binding_view(f.context, read, &b));
    rt = b.type;
    CHECK(nl_semantic_binding_view(f.context, write, &b));
    wt = b.type;
    CHECK(nl_semantic_register_function(f.context, "reads", &rt, 1, f.copy,
                                        false, false) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "writes", &wt, 1, f.copy,
                                        false, false) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "reads(x)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
    CHECK(test_rejected(f.context, "writes(r)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
    TestChecked c = {0};
    CHECK(test_run(f.context, "reads(w)", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &c));
    const NLCheckedNodeView *arg =
        nl_checked_node_view(c.artifact, test_root(&c)->first_argument);
    CHECK(arg->contextually_weakened && arg->parameter_type == rt &&
          arg->value_use == NL_VALUE_COPIED && arg->type == wt);
    test_checked_destroy(&c);
    CHECK(test_run(f.context, "w", TEST_EXPRESSION, NL_CHECK_OK, NULL, &c));
    CHECK(!test_root(&c)->contextually_weakened);
    test_checked_destroy(&c);
    nl_semantic_destroy(f.context);
    /* Exclusive parameter paths reborrow, and the parent is suspended across
     * later argument evaluation (including a nested call). */
    f = (TestSemantic){0};
    CHECK(test_semantic_create(&f));
    NLSymbolId ending;
    NLScopeId parent;
    CHECK(
        test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending, &parent));
    CHECK(nl_semantic_binding_view(f.context, ending, &b));
    const NLTypeId ex = b.type;
    CHECK(nl_semantic_register_function(f.context, "uses", &ex, 1, f.copy,
                                        false, false) == NL_CHECK_OK);
    const NLTypeId params[] = {ex, f.copy};
    CHECK(nl_semantic_register_function(f.context, "two", params, 2, f.copy,
                                        false, false) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "two(ending,uses(ending))", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-SUSPENDED-AUTHORITY"));
    for (size_t i = 0; i < 2; ++i) {
        CHECK(test_run(f.context, "uses(ending)", TEST_EXPRESSION, NL_CHECK_OK,
                       NULL, &c));
        arg = nl_checked_node_view(c.artifact, test_root(&c)->first_argument);
        NLSemanticScopeView child;
        CHECK(arg->value_use == NL_VALUE_REBORROWED &&
              arg->reborrow_scope != parent);
        CHECK(nl_semantic_scope_view(f.context, arg->reborrow_scope, &child) &&
              !child.active && child.parent == parent &&
              child.parent_authority == b.value);
        CHECK(nl_semantic_binding_view(f.context, ending, &b) &&
              b.availability == NL_AVAILABLE);
        test_checked_destroy(&c);
    }
    NLScopeId external_child;
    CHECK(nl_semantic_scope(f.context, parent, true, &external_child) ==
          NL_CHECK_OK);
    CHECK(test_rejected(f.context, "uses(ending)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-SUSPENDED-AUTHORITY"));
    CHECK(nl_semantic_end_scope(f.context, parent) == NL_CHECK_SEMANTIC_ERROR);
    CHECK(nl_semantic_end_scope(f.context, external_child) == NL_CHECK_OK);
    CHECK(test_run(f.context, "uses(ending)", TEST_EXPRESSION, NL_CHECK_OK,
                   NULL, &c));
    test_checked_destroy(&c);
    CHECK(nl_semantic_end_scope(f.context, parent) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "uses(ending)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-DEAD-SCOPE"));
    nl_semantic_destroy(f.context);
    f = (TestSemantic){0};
    CHECK(test_semantic_create(&f));
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_WRITE, true, &ending, NULL));
    CHECK(nl_semantic_binding_view(f.context, ending, &b));
    const NLValueId package = b.value;
    NLTypeId read_domain, exclusive_read_domain;
    CHECK(nl_semantic_compound_type(
              f.context, NL_TYPE_REF, nl_semantic_domain_type(f.context),
              NL_ACCESS_READ, false, &read_domain) == NL_CHECK_OK);
    CHECK(nl_semantic_compound_type(
              f.context, NL_TYPE_REF, nl_semantic_domain_type(f.context),
              NL_ACCESS_READ, true, &exclusive_read_domain) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "reads", &read_domain, 1,
                                        f.copy, false, false) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "exclusive_reads",
                                        &exclusive_read_domain, 1, f.copy,
                                        false, false) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "reads(ending)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-EXCLUSIVE-MODE-UNSUPPORTED"));
    CHECK(test_rejected(f.context, "exclusive_reads(ending)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-EXCLUSIVE-MODE-UNSUPPORTED"));
    CHECK(test_run(f.context, "let transferred = ending", TEST_BINDING,
                   NL_CHECK_OK, NULL, &c));
    CHECK(nl_semantic_binding_view(f.context, ending, &b) &&
          b.availability == NL_CONSUMED);
    CHECK(nl_semantic_binding_view(f.context, test_root(&c)->symbol, &b) &&
          b.availability == NL_AVAILABLE && b.value == package);
    CHECK(nl_checked_node_view(c.artifact, test_root(&c)->initializer)
              ->value_use == NL_VALUE_CONSUMED);
    test_checked_destroy(&c);
    nl_semantic_destroy(f.context);
    return true;
}

static bool domain_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    TestChecked c = {0};
    CHECK(test_run(f.context, "let d = LifetimeDomain()", TEST_BINDING,
                   NL_CHECK_OK, NULL, &c));
    NLSymbolId d = test_root(&c)->symbol;
    NLSemanticBindingView b;
    NLSemanticValueView v;
    CHECK(nl_semantic_binding_view(f.context, d, &b) &&
          nl_semantic_value_view(f.context, b.value, &v));
    const NLDomainId domain = v.domain;
    const NLValueId package = b.value;
    CHECK(domain != 0 && domain != f.domain);
    test_checked_destroy(&c);
    CHECK(
        test_run(f.context, "let d2 = d", TEST_BINDING, NL_CHECK_OK, NULL, &c));
    CHECK(nl_semantic_binding_view(f.context, d, &b) &&
          b.availability == NL_CONSUMED);
    CHECK(nl_semantic_binding_view(f.context, test_root(&c)->symbol, &b) &&
          b.availability == NL_AVAILABLE && b.value == package);
    CHECK(nl_semantic_value_view(f.context, b.value, &v) && v.domain == domain);
    test_checked_destroy(&c);
    CHECK(test_run(f.context, "finalize_domain(d2)", TEST_EXPRESSION,
                   NL_CHECK_OK, NULL, &c));
    NLSemanticDomainView dv;
    CHECK(nl_semantic_domain_view(f.context, domain, &dv) && !dv.live &&
          dv.value == package);
    CHECK(test_root(&c)->kind == NL_CHECKED_DOMAIN_FINALIZE &&
          test_root(&c)->result_count == 0);
    test_checked_destroy(&c);
    NLPlaceId place;
    NLValueId value;
    CHECK(nl_semantic_seed_root(f.context, f.copy, f.domain, true,
                                NL_DEPENDENCY_FREE, &place,
                                &value) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "finalize_domain(life)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-GOVERNED-ROOTS-LIVE"));
    NLSymbolId stable;
    NLScopeId scope;
    CHECK(
        test_domain_ref(&f, "stable", NL_ACCESS_READ, false, &stable, &scope));
    CHECK(test_rejected(f.context, "finalize_domain(life)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-REF-CONFLICT"));
    CHECK(nl_semantic_end_scope(f.context, scope) == NL_CHECK_OK);
    nl_semantic_destroy(f.context);
    return true;
}

static bool pointer_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x, r, p;
    NLScopeId scope;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(f.context, x, &b));
    const NLPlaceId place = b.place;
    CHECK(test_reference(f.context, "r", place, NL_TYPE_REF, NL_ACCESS_READ,
                         false, &r, &scope));
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(f.context, &before));
    TestChecked c = {0};
    CHECK(test_run(f.context, "let p = ptr_from_ref(r)", TEST_BINDING,
                   NL_CHECK_OK, NULL, &c));
    p = test_root(&c)->symbol;
    CHECK(nl_semantic_snapshot(f.context, &after));
    CHECK(after.domains == before.domains &&
          after.places == before.places + 1); /* only p's local carrier */
    CHECK(nl_semantic_binding_view(f.context, p, &b));
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(f.context, b.value, &v) &&
          v.reference.place == place && v.reference.scope == 0);
    NLSemanticPlaceView root;
    CHECK(nl_semantic_place_view(f.context, place, &root) &&
          root.incarnation == v.reference.incarnation);
    test_checked_destroy(&c);
    CHECK(nl_semantic_end_scope(f.context, scope) == NL_CHECK_OK);
    CHECK(test_run(f.context, "p", TEST_EXPRESSION, NL_CHECK_OK, NULL, &c));
    test_checked_destroy(&c);
    CHECK(test_rejected(f.context, "ptr_from_ref(r)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-DEAD-SCOPE"));
    CHECK(test_rejected(f.context, "ptr_from_ref(x)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-REF-REQUIRED"));
    CHECK(test_reference(f.context, "write", place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &r, NULL));
    CHECK(test_run(f.context, "ptr_from_ref(write)", TEST_EXPRESSION,
                   NL_CHECK_OK, NULL, &c));
    CHECK(nl_semantic_value_view(f.context, test_root(&c)->results[0].value,
                                 &v) &&
          v.reference.place == place &&
          v.reference.incarnation == root.incarnation &&
          v.reference.scope == 0);
    test_checked_destroy(&c);
    CHECK(test_reference(f.context, "exclusive", place, NL_TYPE_REF,
                         NL_ACCESS_READ, true, &r, NULL));
    CHECK(test_rejected(f.context, "ptr_from_ref(exclusive)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-EXCLUSIVE-CONVERSION-UNSUPPORTED"));
    nl_semantic_destroy(f.context);
    return true;
}

static bool initialize_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId slot, incoming, stable;
    NLPlaceId place;
    CHECK(nl_semantic_seed_slot(f.context, "empty", f.linear, &slot, &place) ==
          NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f.context, "incoming", f.linear,
                                 NL_DEPENDENCY_FREE, &incoming) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "stable", NL_ACCESS_WRITE, false, &stable, NULL));
    NLSymbolId wrong;
    CHECK(nl_semantic_seed_value(f.context, "wrong", f.copy, NL_DEPENDENCY_FREE,
                                 &wrong) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "initialize(empty,wrong,stable)",
                        TEST_EXPRESSION, NL_CHECK_SEMANTIC_ERROR,
                        "P3-TYPE-MISMATCH"));
    CHECK(test_rejected(f.context, "initialize(empty,incoming,wrong)",
                        TEST_EXPRESSION, NL_CHECK_SEMANTIC_ERROR,
                        "P3-TYPE-MISMATCH"));
    CHECK(nl_semantic_register_function(f.context, "expects_copy", &f.copy, 1,
                                        f.copy, false, false) == NL_CHECK_OK);
    CHECK(test_rejected(
        f.context, "expects_copy(initialize(empty,incoming,stable))",
        TEST_EXPRESSION, NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
    CHECK(test_rejected(f.context, "initialize(empty,incoming,incoming)",
                        TEST_EXPRESSION, NL_CHECK_SEMANTIC_ERROR,
                        "P3-USE-AFTER-CONSUME"));
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(f.context, incoming, &b));
    const NLValueId package = b.value;
    TestChecked c = {0};
    CHECK(test_run(f.context, "initialize(empty,incoming,stable)",
                   TEST_EXPRESSION, NL_CHECK_OK, NULL, &c));
    CHECK(test_root(&c)->kind == NL_CHECKED_INITIALIZE &&
          test_root(&c)->result_count == 1);
    NLSemanticPlaceView root;
    CHECK(nl_semantic_place_view(f.context, place, &root) && root.live &&
          root.independent_root && root.governing_domain == f.domain &&
          root.current_value == package && root.current_fact != 0);
    NLSemanticValueView ptr;
    CHECK(nl_semantic_value_view(f.context, test_root(&c)->results[0].value,
                                 &ptr));
    CHECK(ptr.reference.place == place &&
          ptr.reference.incarnation == root.incarnation &&
          ptr.reference.provenance == NL_PROVENANCE_VALID);
    NLCheckedNodeId arg = test_root(&c)->first_argument;
    arg = nl_checked_node_view(c.artifact, arg)->next_argument;
    arg = nl_checked_node_view(c.artifact, arg)->next_argument;
    CHECK(nl_checked_node_view(c.artifact, arg)->contextually_weakened);
    CHECK(nl_semantic_binding_view(f.context, slot, &b) &&
          b.availability == NL_CONSUMED);
    CHECK(nl_semantic_binding_view(f.context, incoming, &b) &&
          b.availability == NL_CONSUMED);
    CHECK(nl_semantic_binding_view(f.context, stable, &b) &&
          b.availability == NL_AVAILABLE);
    test_checked_destroy(&c);
    CHECK(test_rejected(f.context, "initialize(empty,incoming,stable)",
                        TEST_EXPRESSION, NL_CHECK_SEMANTIC_ERROR,
                        "P3-USE-AFTER-CONSUME"));
    nl_semantic_destroy(f.context);
    return true;
}

static bool ending_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLPlaceId first, second;
    NLValueId first_value, second_value;
    NLSymbolId p1, p2, ending, stable;
    NLScopeId ending_scope, stable_scope;
    CHECK(nl_semantic_seed_root(f.context, f.linear, f.domain, true,
                                NL_DEPENDENCY_FREE, &first,
                                &first_value) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_root(f.context, f.discardable, f.domain, true,
                                NL_DEPENDENCY_FREE, &second,
                                &second_value) == NL_CHECK_OK);
    CHECK(test_reference(f.context, "p1", first, NL_TYPE_PTR, NL_ACCESS_READ,
                         false, &p1, NULL));
    CHECK(test_reference(f.context, "p2", second, NL_TYPE_PTR, NL_ACCESS_READ,
                         false, &p2, NULL));
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending,
                          &ending_scope));
    CHECK(test_rejected(f.context, "destroy(p1,ending)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-DISCARDABLE-REQUIRED"));
    CHECK(test_rejected(f.context, "let one = take(p1,ending)", TEST_BINDING,
                        NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-DIRECT-RECEIVING-UNSUPPORTED"));
    TestChecked c = {0};
    CHECK(test_run(f.context, "take(p1,ending)", TEST_EXPRESSION, NL_CHECK_OK,
                   NULL, &c));
    const NLCheckedNodeView result = *test_root(&c);
    CHECK(result.kind == NL_CHECKED_TAKE && result.result_count == 2 &&
          result.results[0].value == first_value &&
          result.results[1].value != first_value);
    NLSemanticValueView value;
    CHECK(nl_semantic_value_view(f.context, first_value, &value) &&
          value.carrier == NL_CARRIER_LOOSE);
    CHECK(nl_semantic_value_view(f.context, result.results[1].value, &value) &&
          value.slot_place == first && value.carrier == NL_CARRIER_LOOSE);
    NLSemanticPlaceView root;
    CHECK(nl_semantic_place_view(f.context, first, &root) && !root.live &&
          root.governing_domain == 0 && root.current_fact == 0 &&
          root.current_value == 0);
    const NLCheckedNodeView *arg =
        nl_checked_node_view(c.artifact, result.first_argument);
    arg = nl_checked_node_view(c.artifact, arg->next_argument);
    CHECK(arg->value_use == NL_VALUE_REBORROWED);
    NLSemanticScopeView child;
    CHECK(nl_semantic_scope_view(f.context, arg->reborrow_scope, &child) &&
          !child.active);
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(f.context, ending, &b) &&
          b.availability == NL_AVAILABLE);
    test_checked_destroy(&c);
    NLPlaceId third;
    NLValueId third_value;
    NLSymbolId p3;
    CHECK(nl_semantic_seed_root(f.context, f.linear, f.domain, true,
                                NL_DEPENDENCY_FREE, &third,
                                &third_value) == NL_CHECK_OK);
    CHECK(test_reference(f.context, "p3", third, NL_TYPE_PTR, NL_ACCESS_READ,
                         false, &p3, NULL));
    CHECK(test_run(f.context, "take(p3,ending)", TEST_EXPRESSION, NL_CHECK_OK,
                   NULL, &c));
    CHECK(test_root(&c)->results[0].value == third_value);
    CHECK(nl_semantic_binding_view(f.context, ending, &b) &&
          b.availability == NL_AVAILABLE);
    test_checked_destroy(&c);
    CHECK(test_rejected(f.context, "take(p1,ending)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
    CHECK(test_run(f.context, "destroy(p2,ending)", TEST_EXPRESSION,
                   NL_CHECK_OK, NULL, &c));
    CHECK(test_root(&c)->kind == NL_CHECKED_DESTROY &&
          test_root(&c)->result_count == 1);
    CHECK(nl_semantic_value_view(f.context, second_value, &value) &&
          value.carrier == NL_CARRIER_ENDED);
    CHECK(nl_semantic_binding_view(f.context, ending, &b) &&
          b.availability == NL_AVAILABLE);
    test_checked_destroy(&c);
    /* Typed slot/value responsibilities can be handed back separately by a
     * host fixture; this is not tuple syntax. Reinitialize fresh incarnation.
     */
    NLSymbolId recovered, vacant;
    CHECK(nl_semantic_bind_result(f.context, "recovered", first_value,
                                  &recovered) == NL_CHECK_OK);
    CHECK(nl_semantic_bind_result(f.context, "vacant", result.results[1].value,
                                  &vacant) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "initialize(vacant,recovered,ending)",
                        TEST_EXPRESSION, NL_CHECK_SEMANTIC_ERROR,
                        "P3-TYPE-MISMATCH"));
    CHECK(nl_semantic_end_scope(f.context, ending_scope) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "stable", NL_ACCESS_READ, false, &stable,
                          &stable_scope));
    CHECK(test_run(f.context, "initialize(vacant,recovered,stable)",
                   TEST_EXPRESSION, NL_CHECK_OK, NULL, &c));
    CHECK(nl_semantic_place_view(f.context, first, &root) && root.live &&
          root.current_value == first_value);
    CHECK(nl_semantic_binding_view(f.context, p1, &b));
    CHECK(nl_semantic_value_view(f.context, b.value, &value));
    CHECK(root.incarnation != value.reference.incarnation);
    CHECK(nl_semantic_binding_view(f.context, ending, &b) &&
          b.availability == NL_AVAILABLE);
    test_checked_destroy(&c);
    CHECK(nl_semantic_end_scope(f.context, stable_scope) == NL_CHECK_OK);
    CHECK(test_domain_ref(&f, "ending2", NL_ACCESS_READ, true, &ending, NULL));
    CHECK(test_rejected(f.context, "take(p1,ending2)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-STALE-POINTER"));
    nl_semantic_destroy(f.context);
    return true;
}

static bool ending_negative_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLPlaceId root, nonroot;
    NLValueId value;
    NLSymbolId p, q, ending;
    CHECK(nl_semantic_seed_root(f.context, f.copy, f.domain, true,
                                NL_DEPENDENCY_FREE, &root,
                                &value) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_root(f.context, f.copy, f.domain, false,
                                NL_DEPENDENCY_FREE, &nonroot,
                                &value) == NL_CHECK_OK);
    CHECK(test_reference(f.context, "p", root, NL_TYPE_PTR, NL_ACCESS_READ,
                         false, &p, NULL));
    CHECK(test_reference(f.context, "q", nonroot, NL_TYPE_PTR, NL_ACCESS_READ,
                         false, &q, NULL));
    NLSymbolId other_life;
    NLDomainId other;
    CHECK(nl_semantic_seed_domain(f.context, "other", &other_life, &other) ==
          NL_CHECK_OK);
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(f.context, other_life, &b));
    CHECK(test_reference(f.context, "wrong", b.place, NL_TYPE_REF,
                         NL_ACCESS_READ, true, &ending, NULL));
    CHECK(test_rejected(f.context, "take(p,wrong)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-DOMAIN-MISMATCH"));
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending, NULL));
    CHECK(test_rejected(f.context, "take(q,ending)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-NOT-LIFETIME-ROOT"));
    NLSymbolId r;
    CHECK(test_reference(f.context, "r", root, NL_TYPE_REF, NL_ACCESS_READ,
                         false, &r, NULL));
    CHECK(test_rejected(f.context, "take(p,ending)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-REF-CONFLICT"));
    nl_semantic_destroy(f.context);
    return true;
}

static bool write_tests(void)
{
    for (size_t linear = 0; linear < 2; ++linear) {
        TestSemantic f = {0};
        CHECK(test_semantic_create(&f));
        const NLTypeId type = linear ? f.linear : f.discardable;
        NLPlaceId a, b;
        NLValueId av, bv;
        NLSymbolId ar, alias, br, incoming, rr;
        CHECK(nl_semantic_seed_root(f.context, type, f.domain, true,
                                    NL_DEPENDENCY_FREE, &a,
                                    &av) == NL_CHECK_OK);
        NLSymbolId other_life;
        NLDomainId other;
        CHECK(nl_semantic_seed_domain(f.context, "other", &other_life,
                                      &other) == NL_CHECK_OK);
        CHECK(nl_semantic_seed_root(f.context, type, other, true,
                                    NL_DEPENDENCY_FREE, &b,
                                    &bv) == NL_CHECK_OK);
        CHECK(test_reference(f.context, "a", a, NL_TYPE_REF, NL_ACCESS_WRITE,
                             false, &ar, NULL));
        CHECK(test_reference(f.context, "alias", a, NL_TYPE_REF,
                             NL_ACCESS_WRITE, false, &alias, NULL));
        CHECK(test_reference(f.context, "b", b, NL_TYPE_REF, NL_ACCESS_WRITE,
                             false, &br, NULL));
        CHECK(test_reference(f.context, "read", a, NL_TYPE_REF, NL_ACCESS_READ,
                             false, &rr, NULL));
        NLSemanticPlaceView ap, bp, now;
        CHECK(nl_semantic_place_view(f.context, a, &ap) &&
              nl_semantic_place_view(f.context, b, &bp));
        NLSemanticSnapshot before, after;
        CHECK(nl_semantic_snapshot(f.context, &before));
        TestChecked c = {0};
        CHECK(test_run(f.context, "swap(a,alias)", TEST_EXPRESSION, NL_CHECK_OK,
                       NULL, &c));
        CHECK(test_root(&c)->result_count == 0);
        test_checked_destroy(&c);
        CHECK(nl_semantic_place_view(f.context, a, &now) &&
              test_place_equal(ap, now));
        CHECK(nl_semantic_snapshot(f.context, &after) &&
              before.last_value_fact == after.last_value_fact &&
              before.last_incarnation == after.last_incarnation);
        CHECK(test_run(f.context, "swap(a,b)", TEST_EXPRESSION, NL_CHECK_OK,
                       NULL, &c));
        test_checked_destroy(&c);
        CHECK(nl_semantic_place_view(f.context, a, &now) &&
              now.current_value == bv && now.incarnation == ap.incarnation &&
              now.governing_domain == ap.governing_domain &&
              now.current_fact != ap.current_fact);
        CHECK(nl_semantic_place_view(f.context, b, &now) &&
              now.current_value == av && now.incarnation == bp.incarnation &&
              now.governing_domain == bp.governing_domain &&
              now.current_fact != bp.current_fact);
        CHECK(nl_semantic_seed_value(f.context, "incoming", type,
                                     NL_DEPENDENCY_FREE,
                                     &incoming) == NL_CHECK_OK);
        NLSemanticBindingView in;
        CHECK(nl_semantic_binding_view(f.context, incoming, &in));
        CHECK(test_rejected(f.context, "replace(read,incoming)",
                            TEST_EXPRESSION, NL_CHECK_SEMANTIC_ERROR,
                            "P3-TYPE-MISMATCH"));
        CHECK(test_rejected(f.context, "store(read,incoming)", TEST_EXPRESSION,
                            NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
        CHECK(test_rejected(f.context, "swap(a,read)", TEST_EXPRESSION,
                            NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
        CHECK(nl_semantic_place_view(f.context, a, &ap));
        CHECK(test_run(f.context, "replace(a,incoming)", TEST_EXPRESSION,
                       NL_CHECK_OK, NULL, &c));
        CHECK(test_root(&c)->result_count == 1 &&
              test_root(&c)->results[0].value == bv);
        test_checked_destroy(&c);
        CHECK(nl_semantic_place_view(f.context, a, &now) &&
              now.current_value == in.value &&
              now.incarnation == ap.incarnation &&
              now.governing_domain == ap.governing_domain &&
              now.current_fact != ap.current_fact);
        NLSemanticValueView old;
        CHECK(nl_semantic_value_view(f.context, bv, &old) &&
              old.carrier == NL_CARRIER_LOOSE);
        CHECK(nl_semantic_seed_value(f.context, "next", type,
                                     NL_DEPENDENCY_FREE,
                                     &incoming) == NL_CHECK_OK);
        if (linear) {
            CHECK(test_rejected(f.context, "store(a,next)", TEST_EXPRESSION,
                                NL_CHECK_SEMANTIC_ERROR,
                                "P3-DISCARDABLE-REQUIRED"));
        } else {
            CHECK(nl_semantic_binding_view(f.context, incoming, &in));
            CHECK(nl_semantic_place_view(f.context, a, &ap));
            CHECK(test_run(f.context, "store(a,next)", TEST_EXPRESSION,
                           NL_CHECK_OK, NULL, &c));
            CHECK(test_root(&c)->result_count == 0);
            test_checked_destroy(&c);
            CHECK(nl_semantic_place_view(f.context, a, &now) &&
                  now.current_value == in.value &&
                  now.incarnation == ap.incarnation &&
                  now.governing_domain == ap.governing_domain &&
                  now.current_fact != ap.current_fact);
            CHECK(nl_semantic_value_view(f.context, ap.current_value, &old) &&
                  old.carrier == NL_CARRIER_ENDED);
        }
        nl_semantic_destroy(f.context);
    }
    return true;
}

static bool loan_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x;
    CHECK(nl_semantic_seed_value(f.context, "x", f.linear, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    const char *local[] = {
        "loan read x as r { nonsense(unknown) }", "loan write x as r {}",
        "loan exclusive read x as r {}", "loan exclusive write x as r {}"};
    for (size_t i = 0; i < 4; ++i) {
        TestChecked c = {0};
        CHECK(test_run(f.context, local[i], TEST_LOAN, NL_CHECK_OK, NULL, &c));
        const NLCheckedLoanPlan plan = test_root(&c)->loan;
        CHECK(plan.source == x && plan.stability == 0 && plan.scope != 0 &&
              plan.access == (i % 2 == 0 ? NL_ACCESS_READ : NL_ACCESS_WRITE) &&
              plan.is_exclusive == (i >= 2));
        CHECK(plan.prevent_lifetime_end &&
              plan.prevent_conflicting_access == (i >= 2) &&
              !plan.body_nonescape_proved);
        NLSemanticScopeView scope;
        CHECK(nl_semantic_scope_view(f.context, plan.scope, &scope) &&
              !scope.active);
        NLSemanticBindingView b;
        CHECK(nl_semantic_binding_view(f.context, x, &b) &&
              b.availability == NL_AVAILABLE);
        CHECK(nl_semantic_find_binding(f.context, "r") == 0);
        test_checked_destroy(&c);
    }
    CHECK(test_rejected(f.context, "loan read x using life as r {}", TEST_LOAN,
                        NL_CHECK_SEMANTIC_ERROR, "P3-LOCAL-USING-FORBIDDEN"));
    NLPlaceId root;
    NLValueId value;
    NLSymbolId p, stable;
    NLScopeId stability_scope;
    CHECK(nl_semantic_seed_root(f.context, f.copy, f.domain, true,
                                NL_DEPENDENCY_FREE, &root,
                                &value) == NL_CHECK_OK);
    CHECK(test_reference(f.context, "p", root, NL_TYPE_PTR, NL_ACCESS_READ,
                         false, &p, NULL));
    CHECK(test_domain_ref(&f, "stable", NL_ACCESS_WRITE, false, &stable,
                          &stability_scope));
    const char *pointer[] = {"loan read p using stable as r {}",
                             "loan write p using stable as r {}",
                             "loan exclusive read p using stable as r {}"};
    for (size_t i = 0; i < 3; ++i) {
        TestChecked c = {0};
        CHECK(
            test_run(f.context, pointer[i], TEST_LOAN, NL_CHECK_OK, NULL, &c));
        const NLCheckedLoanPlan plan = test_root(&c)->loan;
        CHECK(plan.source == p && plan.stability == stable &&
              plan.stability_weakened && plan.place == root &&
              plan.domain == f.domain &&
              plan.dependency_scope == stability_scope &&
              plan.prevent_lifetime_end && !plan.body_nonescape_proved);
        CHECK(plan.is_exclusive == (i == 2));
        test_checked_destroy(&c);
    }
    CHECK(test_rejected(
        f.context, "loan exclusive write p using stable as r {}", TEST_LOAN,
        NL_CHECK_SEMANTIC_UNSUPPORTED, "P3-PTR-EXCLUSIVE-WRITE-UNSUPPORTED"));
    CHECK(test_rejected(f.context, "loan read p as r {}", TEST_LOAN,
                        NL_CHECK_SEMANTIC_ERROR, "P3-STABILITY-REQUIRED"));
    NLSymbolId other;
    NLDomainId d;
    CHECK(nl_semantic_seed_domain(f.context, "other", &other, &d) ==
          NL_CHECK_OK);
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(f.context, other, &b));
    CHECK(test_reference(f.context, "wrong", b.place, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &other, NULL));
    CHECK(test_rejected(f.context, "loan read p using wrong as r {}", TEST_LOAN,
                        NL_CHECK_SEMANTIC_ERROR, "P3-DOMAIN-MISMATCH"));
    NLSymbolId ordinary;
    NLScopeId ordinary_scope;
    CHECK(test_reference(f.context, "ordinary", root, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &ordinary, &ordinary_scope));
    TestChecked c = {0};
    CHECK(test_run(f.context, "loan write p using stable as r {}", TEST_LOAN,
                   NL_CHECK_OK, NULL, &c));
    test_checked_destroy(&c);
    CHECK(test_rejected(f.context, "loan exclusive read p using stable as r {}",
                        TEST_LOAN, NL_CHECK_SEMANTIC_ERROR, "P3-REF-CONFLICT"));
    CHECK(nl_semantic_end_scope(f.context, ordinary_scope) == NL_CHECK_OK);
    NLScopeId exclusive_scope;
    CHECK(test_reference(f.context, "ex", root, NL_TYPE_REF, NL_ACCESS_READ,
                         true, &ordinary, &exclusive_scope));
    CHECK(test_rejected(f.context, "loan read p using stable as r {}",
                        TEST_LOAN, NL_CHECK_SEMANTIC_ERROR, "P3-REF-CONFLICT"));
    CHECK(nl_semantic_end_scope(f.context, exclusive_scope) == NL_CHECK_OK);
    CHECK(nl_semantic_end_scope(f.context, stability_scope) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "loan read p using stable as r {}",
                        TEST_LOAN, NL_CHECK_SEMANTIC_ERROR, "P3-DEAD-SCOPE"));
    CHECK(test_domain_ref(&f, "exclusive_stable", NL_ACCESS_READ, true, &stable,
                          NULL));
    CHECK(test_rejected(f.context, "loan read p using exclusive_stable as r {}",
                        TEST_LOAN, NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-EXCLUSIVE-STABILITY-UNSUPPORTED"));
    nl_semantic_destroy(f.context);
    return true;
}

static bool failure_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId linear, copy;
    CHECK(nl_semantic_seed_value(f.context, "linear", f.linear,
                                 NL_DEPENDENCY_FREE, &linear) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f.context, "copy", f.copy, NL_DEPENDENCY_FREE,
                                 &copy) == NL_CHECK_OK);
    const NLTypeId params[] = {f.linear, f.linear};
    CHECK(nl_semantic_register_function(f.context, "pair", params, 2, f.copy,
                                        false, false) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "pair(linear,copy)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
    CHECK(test_rejected(f.context, "pair(linear,linear)", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_ERROR, "P3-USE-AFTER-CONSUME"));
    const NLTypeId domain = nl_semantic_domain_type(f.context);
    CHECK(nl_semantic_register_function(f.context, "affine_core", &domain, 1,
                                        f.copy, false, false) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "affine_core(LifetimeDomain())",
                        TEST_EXPRESSION, NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-AUTHORITY-SUMMARY-UNSUPPORTED"));
    const NLTypeId mismatch[] = {f.copy};
    CHECK(nl_semantic_register_function(f.context, "expects_copy", mismatch, 1,
                                        f.copy, false, false) == NL_CHECK_OK);
    CHECK(test_rejected(
        f.context, "expects_copy(LifetimeDomain())", TEST_EXPRESSION,
        NL_CHECK_SEMANTIC_ERROR,
        "P3-TYPE-MISMATCH")); /* fresh domain candidate is rolled back */
    CHECK(nl_semantic_register_function(f.context, "effect", NULL, 0, f.copy,
                                        true, false) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "hidden", NULL, 0, f.copy,
                                        false, true) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "effect()", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-EFFECT-SUMMARY-UNSUPPORTED"));
    CHECK(test_rejected(f.context, "hidden()", TEST_EXPRESSION,
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P3-DEPENDENCIES-UNSUPPORTED"));
    NLTypeId pointer_type;
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_PTR, f.copy,
                                    NL_ACCESS_READ, false,
                                    &pointer_type) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "returns_ptr", NULL, 0,
                                        pointer_type, false,
                                        false) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "returns_ptr()", TEST_EXPRESSION,
                        NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-RESULT-SUMMARY-UNSUPPORTED"));
    NLPlaceId place;
    NLValueId value;
    CHECK(nl_semantic_seed_root(f.context, f.copy, f.domain, true,
                                NL_DEPENDENCY_FREE, &place,
                                &value) == NL_CHECK_OK);
    NLSemanticPlaceView root;
    CHECK(nl_semantic_place_view(f.context, place, &root));
    NLSymbolId p, stable;
    CHECK(test_domain_ref(&f, "stable", NL_ACCESS_READ, false, &stable, NULL));
    CHECK(nl_semantic_seed_reference(
              f.context, "unknown_ptr", pointer_type,
              (NLReferenceFacts){0, 0, 0, NL_PROVENANCE_UNKNOWN, true, true, 0},
              &p) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "loan read unknown_ptr using stable as r {}",
                        TEST_LOAN, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P3-UNKNOWN-PROVENANCE"));
    CHECK(nl_semantic_seed_reference(
              f.context, "invalid_ptr", pointer_type,
              (NLReferenceFacts){place, root.incarnation, 0,
                                 NL_PROVENANCE_INVALID, true, true, 0},
              &p) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "loan read invalid_ptr using stable as r {}",
                        TEST_LOAN, NL_CHECK_SEMANTIC_ERROR,
                        "P3-INVALID-PROVENANCE"));
    CHECK(nl_semantic_seed_reference(f.context, "no_write", pointer_type,
                                     (NLReferenceFacts){place, root.incarnation,
                                                        0, NL_PROVENANCE_VALID,
                                                        true, false, 0},
                                     &p) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "loan write no_write using stable as r {}",
                        TEST_LOAN, NL_CHECK_SEMANTIC_ERROR, "P3-ACCESS"));
    nl_semantic_destroy(f.context);
    for (size_t knowledge = NL_HIDDEN_DEPENDENCIES;
         knowledge <= NL_DEPENDENCIES_UNKNOWN; ++knowledge) {
        f = (TestSemantic){0};
        CHECK(test_semantic_create(&f));
        CHECK(nl_semantic_seed_root(f.context, f.copy, f.domain, true,
                                    (NLDependencyKnowledge)knowledge, &place,
                                    &value) == NL_CHECK_OK);
        CHECK(test_reference(f.context, "a", place, NL_TYPE_REF,
                             NL_ACCESS_WRITE, false, &p, NULL));
        CHECK(nl_semantic_seed_value(f.context, "copy", f.copy,
                                     NL_DEPENDENCY_FREE, &copy) == NL_CHECK_OK);
        CHECK(test_rejected(f.context, "replace(a,copy)", TEST_EXPRESSION,
                            NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                            "P3-DEPENDENCIES-UNSUPPORTED"));
        CHECK(test_rejected(f.context, "store(a,copy)", TEST_EXPRESSION,
                            NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                            "P3-DEPENDENCIES-UNSUPPORTED"));
        CHECK(test_rejected(f.context, "swap(a,a)", TEST_EXPRESSION,
                            NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                            "P3-DEPENDENCIES-UNSUPPORTED"));
        nl_semantic_destroy(f.context);
    }
    return true;
}

static bool allocation_tests(void)
{
    /* Every malloc/realloc encountered on each success path is failed in turn.
     * Parsing and cleanup run outside injection, isolating P3 ownership. */
    const char *texts[] = {
        "let d = LifetimeDomain()",
        "ptr<ptr<LinearT>>",
        "let moved = linear",
        "initialize(empty,linear,stable)",
        "take(p,ending)",
        "replace(w,linear)",
        "loan read p using stable as r {}",
        "many(copy,copy,copy,copy,copy,copy,copy,copy,copy,copy,copy,copy,copy,"
        "copy,copy,copy,copy,copy,copy,copy,copy,copy,copy,copy,copy,copy,copy,"
        "copy,copy,copy,copy,copy)"};
    const TestEntry entries[] = {
        TEST_BINDING,    TEST_TYPE,       TEST_BINDING, TEST_EXPRESSION,
        TEST_EXPRESSION, TEST_EXPRESSION, TEST_LOAN,    TEST_EXPRESSION};
    for (size_t path = 0; path < sizeof(texts) / sizeof(texts[0]); ++path) {
        bool reached_success = false;
        for (size_t nth = 0; nth < 256; ++nth) {
            TestSemantic f = {0};
            CHECK(test_semantic_create(&f));
            CHECK(nl_semantic_register_function(f.context, "registered",
                                                &f.copy, 1, f.copy, false,
                                                false) == NL_CHECK_OK);
            NLSymbolId id;
            NLPlaceId root;
            NLValueId value;
            if (path == 7) {
                NLTypeId parameters[32];
                for (size_t i = 0; i < 32; ++i) {
                    parameters[i] = f.copy;
                }
                CHECK(nl_semantic_register_function(
                          f.context, "many", parameters, 32, f.copy, false,
                          false) == NL_CHECK_OK);
                CHECK(nl_semantic_seed_value(f.context, "copy", f.copy,
                                             NL_DEPENDENCY_FREE,
                                             &id) == NL_CHECK_OK);
            }
            CHECK(nl_semantic_seed_value(f.context, "linear", f.linear,
                                         NL_DEPENDENCY_FREE,
                                         &id) == NL_CHECK_OK);
            CHECK(nl_semantic_seed_root(f.context, f.linear, f.domain, true,
                                        NL_DEPENDENCY_FREE, &root,
                                        &value) == NL_CHECK_OK);
            if (path == 4) {
                CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &id,
                                      NULL));
            } else {
                CHECK(test_domain_ref(&f, "stable", NL_ACCESS_READ, false, &id,
                                      NULL));
            }
            CHECK(nl_semantic_seed_slot(f.context, "empty", f.linear, &id,
                                        &root) == NL_CHECK_OK);
            if (path == 5) {
                CHECK(nl_semantic_seed_root(f.context, f.linear, f.domain, true,
                                            NL_DEPENDENCY_FREE, &root,
                                            &value) == NL_CHECK_OK);
                CHECK(test_reference(f.context, "w", root, NL_TYPE_REF,
                                     NL_ACCESS_WRITE, false, &id, NULL));
            } else {
                CHECK(nl_semantic_seed_root(f.context, f.linear, f.domain, true,
                                            NL_DEPENDENCY_FREE, &root,
                                            &value) == NL_CHECK_OK);
                CHECK(test_reference(f.context, "p", root, NL_TYPE_PTR,
                                     NL_ACCESS_READ, false, &id, NULL));
            }
            TestState before;
            CHECK(test_state(f.context, &before));
            NLSource *source = NULL;
            NLParser *parser = NULL;
            NLSyntaxTree *syntax = NULL;
            NLCheckedFragment *artifact = NULL;
            CHECK(nl_source_create(texts[path], strlen(texts[path]), "oom",
                                   &source) == NL_SOURCE_OK);
            CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK &&
                  test_parse(entries[path])(parser, &syntax, NULL) ==
                      NL_PARSE_OK);
            fail_at = nth;
            allocation_index = 0;
            injecting = true;
            const NLCheckStatus status =
                test_check(entries[path])(f.context, syntax, &artifact, NULL);
            injecting = false;
            CHECK(status == NL_CHECK_OK || status == NL_CHECK_OUT_OF_MEMORY);
            if (status == NL_CHECK_OUT_OF_MEMORY) {
                CHECK(artifact == NULL && test_unchanged(f.context, &before));
            } else {
                CHECK(artifact != NULL);
                reached_success = true;
            }
            nl_checked_destroy(artifact);
            nl_syntax_tree_destroy(syntax);
            nl_parser_destroy(parser);
            nl_source_destroy(source);
            nl_semantic_destroy(f.context);
            if (reached_success) {
                break;
            }
        }
        CHECK(reached_success);
    }
    /* Context creation and registration own additional allocation paths. */
    bool success = false;
    for (size_t nth = 0; nth < 32; ++nth) {
        NLSemanticContext *context = NULL;
        fail_at = nth;
        allocation_index = 0;
        injecting = true;
        const NLCheckStatus status = nl_semantic_create(&context);
        injecting = false;
        CHECK(status == NL_CHECK_OK ||
              (status == NL_CHECK_OUT_OF_MEMORY && context == NULL));
        nl_semantic_destroy(context);
        if (status == NL_CHECK_OK) {
            success = true;
            break;
        }
    }
    CHECK(success);
    for (size_t path = 0; path < 7; ++path) {
        success = false;
        for (size_t nth = 0; nth < 128; ++nth) {
            TestSemantic f = {0};
            CHECK(test_semantic_create(&f));
            TestState before;
            CHECK(test_state(f.context, &before));
            NLTypeId type = SIZE_MAX;
            NLSymbolId symbol = SIZE_MAX;
            NLDomainId domain = SIZE_MAX;
            NLPlaceId place = SIZE_MAX;
            NLValueId value = SIZE_MAX;
            NLScopeId scope = SIZE_MAX;
            fail_at = nth;
            allocation_index = 0;
            injecting = true;
            NLCheckStatus status;
            switch (path) {
            case 0:
                status = nl_semantic_nominal(f.context, "OwnedName", false,
                                             true, &type);
                break;
            case 1:
                status =
                    nl_semantic_seed_value(f.context, "OwnedBinding", f.copy,
                                           NL_DEPENDENCY_FREE, &symbol);
                break;
            case 2:
                status = nl_semantic_seed_domain(f.context, "OwnedDomain",
                                                 &symbol, &domain);
                break;
            case 3:
                status =
                    nl_semantic_seed_root(f.context, f.linear, f.domain, true,
                                          NL_DEPENDENCY_FREE, &place, &value);
                break;
            case 4:
                status = nl_semantic_seed_slot(f.context, "OwnedSlot", f.copy,
                                               &symbol, &place);
                break;
            case 5:
                status = nl_semantic_scope(f.context, 0, true, &scope);
                break;
            default:
                status = nl_semantic_register_function(f.context,
                                                       "OwnedFunction", &f.copy,
                                                       1, f.copy, false, false);
                break;
            }
            injecting = false;
            CHECK(status == NL_CHECK_OK || status == NL_CHECK_OUT_OF_MEMORY);
            if (status == NL_CHECK_OUT_OF_MEMORY) {
                CHECK(test_unchanged(f.context, &before));
                CHECK(type == SIZE_MAX && symbol == SIZE_MAX &&
                      domain == SIZE_MAX && place == SIZE_MAX &&
                      value == SIZE_MAX && scope == SIZE_MAX);
            } else {
                success = true;
            }
            nl_semantic_destroy(f.context);
            if (success) {
                break;
            }
        }
        CHECK(success);
    }
    return true;
}

static bool limits_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    const NLTypeId type = f.copy;
    TestState before;
    CHECK(test_state(f.context, &before));
    CHECK(nl_semantic_register_function(
              f.context, "too_many", &type, NL_SEMANTIC_MAX_PARAMETERS + 1,
              type, false, false) == NL_CHECK_RESOURCE_LIMIT);
    CHECK(test_unchanged(f.context, &before));
    char text[1024];
    size_t length = 0;
    for (size_t i = 0; i < NL_SEMANTIC_MAX_DEPTH - 1; ++i) {
        memcpy(text + length, "ptr<", 4);
        length += 4;
    }
    memcpy(text + length, "CopyT", 5);
    length += 5;
    for (size_t i = 0; i < NL_SEMANTIC_MAX_DEPTH - 1; ++i) {
        text[length++] = '>';
    }
    text[length] = 0;
    TestChecked checked = {0};
    CHECK(test_run(f.context, text, TEST_TYPE, NL_CHECK_OK, NULL, &checked));
    test_checked_destroy(&checked);
    for (size_t i = 0; i < NL_SEMANTIC_MAX_ENTRIES; ++i) {
        NLScopeId scope;
        CHECK(nl_semantic_scope(f.context, 0, false, &scope) == NL_CHECK_OK);
    }
    NLSemanticSnapshot a, b;
    CHECK(nl_semantic_snapshot(f.context, &a));
    NLScopeId untouched = SIZE_MAX;
    CHECK(nl_semantic_scope(f.context, 0, false, &untouched) ==
              NL_CHECK_RESOURCE_LIMIT &&
          untouched == SIZE_MAX);
    CHECK(nl_semantic_snapshot(f.context, &b) && a.scopes == b.scopes &&
          a.values == b.values && a.last_value_fact == b.last_value_fact);
    nl_semantic_destroy(f.context);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return EXIT_FAILURE;
    }
    bool ok = false;
    if (strcmp(argv[1], "type") == 0) {
        ok = type_tests() && original_grant_registry_tests();
    } else if (strcmp(argv[1], "value_use") == 0) {
        ok = value_tests() && compatibility_tests();
    } else if (strcmp(argv[1], "domain") == 0) {
        ok = domain_tests() && pointer_tests();
    } else if (strcmp(argv[1], "transition") == 0) {
        ok = initialize_tests() && ending_tests() && ending_negative_tests() &&
             write_tests();
    } else if (strcmp(argv[1], "loan") == 0) {
        ok = loan_tests();
    } else if (strcmp(argv[1], "failure") == 0) {
        ok = failure_tests() && allocation_tests() && limits_tests();
    }
    if (ok) {
        printf("semantic %s: PASS\n", argv[1]);
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
