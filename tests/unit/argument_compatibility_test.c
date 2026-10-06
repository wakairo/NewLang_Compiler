#include "../support/function_body.h"

/* Both call paths and every ordinary-write operand use the same selected
 * parameter compatibility rule. Full public snapshots include scopes, values,
 * parent availability, places/facts/occurrences and raw state on rejection. */
static bool selected_refs(NLAccessSyntax mode)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId root, exclusive;
    CHECK(nl_semantic_seed_value(f.context, "root", f.copy, NL_DEPENDENCY_FREE,
                                 &root) == NL_CHECK_OK);
    NLSemanticBindingView b, parent;
    CHECK(nl_semantic_binding_view(f.context, root, &b));
    NLScopeId parent_scope;
    CHECK(test_reference(f.context, "exclusive", b.place, NL_TYPE_REF, mode,
                         true, &exclusive, &parent_scope));
    CHECK(nl_semantic_binding_view(f.context, exclusive, &parent));
    NLTypeId ordinary, read, exclusive_read;
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_REF, f.copy, mode, false,
                                    &ordinary) == NL_CHECK_OK);
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_REF, f.copy,
                                    NL_ACCESS_READ, false,
                                    &read) == NL_CHECK_OK);
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_REF, f.copy,
                                    NL_ACCESS_READ, true,
                                    &exclusive_read) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "ordinary", &ordinary, 1, 1,
                                        false, false) == NL_CHECK_OK);
    NLFunctionParameter parameter[] = {{"x", ordinary}};
    CHECK(register_body(f.context, "body", parameter, 1, 1, "{}", NL_CHECK_OK,
                        NULL));
    /* N1/N2 and N3/N4: same-mode exclusive -> ordinary, not a scope conflict.
     */
    CHECK(test_rejected(f.context, "ordinary(exclusive)", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
    CHECK(test_rejected(f.context, "body(exclusive)", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
    if (mode == NL_ACCESS_WRITE) {
        /* N5, including the second swap operand and a prior evaluated arg. */
        CHECK(test_rejected(f.context, "replace(exclusive,root)", TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
        CHECK(test_rejected(f.context, "store(exclusive,root)", TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
        CHECK(test_rejected(f.context, "swap(exclusive,exclusive)", TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
        CHECK(nl_semantic_register_function(f.context, "read", &read, 1, 1,
                                            false, false) == NL_CHECK_OK);
        CHECK(nl_semantic_register_function(f.context, "exclusive_read",
                                            &exclusive_read, 1, 1, false,
                                            false) == NL_CHECK_OK);
        CHECK(test_rejected(f.context, "read(exclusive)", TEST_SOURCE,
                            NL_CHECK_SEMANTIC_UNSUPPORTED,
                            "P3-EXCLUSIVE-MODE-UNSUPPORTED"));
        CHECK(test_rejected(f.context, "exclusive_read(exclusive)", TEST_SOURCE,
                            NL_CHECK_SEMANTIC_UNSUPPORTED,
                            "P3-EXCLUSIVE-MODE-UNSUPPORTED"));
    }
    /* P1: generic registered path retains parent and ends the child. */
    CHECK(nl_semantic_register_function(f.context, "uses", &parent.type, 1, 1,
                                        false, false) == NL_CHECK_OK);
    for (size_t i = 0; i < 2; ++i) {
        TestChecked a = {0};
        CHECK(test_run(f.context, "uses(exclusive)", TEST_SOURCE, NL_CHECK_OK,
                       NULL, &a));
        const NLCheckedNodeView *arg =
            nl_checked_node_view(a.artifact, test_root(&a)->first_argument);
        NLSemanticScopeView child;
        CHECK(arg->value_use == NL_VALUE_REBORROWED &&
              arg->type == parent.type && arg->reborrow_scope != parent_scope);
        CHECK(nl_semantic_scope_view(f.context, arg->reborrow_scope, &child) &&
              !child.active && child.parent == parent_scope &&
              child.parent_authority == parent.value);
        CHECK(nl_semantic_binding_view(f.context, exclusive, &b) &&
              b.availability == NL_AVAILABLE && b.value == parent.value);
        test_checked_destroy(&a);
    }
    /* An independently constructed ordinary child is already ordinary. This
     * host fixture invokes existing public scope/ref APIs, not call adaptation.
     */
    NLScopeId child_scope;
    CHECK(nl_semantic_scope(f.context, parent_scope, true, &child_scope) ==
          NL_CHECK_OK);
    NLSemanticValueView original;
    CHECK(nl_semantic_value_view(f.context, parent.value, &original));
    NLReferenceFacts fact = original.reference;
    fact.scope = child_scope;
    NLSymbolId child_symbol;
    CHECK(nl_semantic_seed_reference(f.context, "child", ordinary, fact,
                                     &child_symbol) == NL_CHECK_OK);
    CHECK(body_ok(f.context, "ordinary(child)"));
    CHECK(body_ok(f.context, "body(child)"));
    if (mode == NL_ACCESS_WRITE) {
        /* P2: ordinary contextual weakening still copies, with no child scope.
         */
        TestChecked a = {0};
        CHECK(test_run(f.context, "read(child)", TEST_SOURCE, NL_CHECK_OK, NULL,
                       &a));
        const NLCheckedNodeView *arg =
            nl_checked_node_view(a.artifact, test_root(&a)->first_argument);
        CHECK(arg->contextually_weakened && arg->value_use == NL_VALUE_COPIED &&
              arg->reborrow_scope == 0);
        test_checked_destroy(&a);
        CHECK(test_rejected(f.context, "swap(child,exclusive)", TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
        CHECK(nl_semantic_binding_view(f.context, child_symbol, &b) &&
              b.availability == NL_AVAILABLE);
    }
    CHECK(nl_semantic_end_scope(f.context, child_scope) == NL_CHECK_OK);
    CHECK(nl_semantic_binding_view(f.context, exclusive, &b) &&
          b.availability == NL_AVAILABLE);
    nl_semantic_destroy(f.context);
    return true;
}
static bool stability_operand(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId ending, empty, incoming;
    NLScopeId ending_scope;
    NLPlaceId vacant;
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending,
                          &ending_scope));
    CHECK(nl_semantic_seed_slot(f.context, "empty", f.linear, &empty,
                                &vacant) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f.context, "incoming", f.linear,
                                 NL_DEPENDENCY_FREE, &incoming) == NL_CHECK_OK);
    /* N6: failure after both non-Copy arguments have been evaluated must undo
     * their consume as well as leaving the exclusive stability actual intact.
     */
    CHECK(test_rejected(f.context, "initialize(empty,incoming,ending)",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-TYPE-MISMATCH"));
    CHECK(nl_semantic_end_scope(f.context, ending_scope) == NL_CHECK_OK);
    NLSymbolId stable;
    CHECK(test_domain_ref(&f, "stable", NL_ACCESS_READ, false, &stable, NULL));
    CHECK(body_ok(f.context, "let p=initialize(empty,incoming,stable)"));
    nl_semantic_destroy(f.context);
    return true;
}
int main(void)
{
    return selected_refs(NL_ACCESS_READ) && selected_refs(NL_ACCESS_WRITE) &&
                   stability_operand()
               ? 0
               : 1;
}
