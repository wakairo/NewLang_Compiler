#ifndef NEWLANG_TEST_SEMANTIC_CHECK_H
#define NEWLANG_TEST_SEMANTIC_CHECK_H

#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/raw_storage.h"
#include "test.h"

#include <string.h>

typedef enum {
    TEST_TYPE,
    TEST_EXPRESSION,
    TEST_BINDING,
    TEST_LOAN,
    TEST_SOURCE
} TestEntry;
typedef NLParseStatus (*TestParse)(NLParser *, NLSyntaxTree **,
                                   NLParseDiagnostic *);
typedef NLCheckStatus (*TestCheck)(NLSemanticContext *, const NLSyntaxTree *,
                                   NLCheckedFragment **, NLCheckDiagnostic *);
static inline TestParse test_parse(TestEntry entry)
{
    const TestParse entries[] = {
        nl_parser_parse_type_fragment, nl_parser_parse_expression_fragment,
        nl_parser_parse_binding_fragment, nl_parser_parse_loan_fragment,
        nl_parser_parse_source_fragment};
    return entries[entry];
}
static inline TestCheck test_check(TestEntry entry)
{
    const TestCheck entries[] = {
        nl_semantic_check_type, nl_semantic_check_expression,
        nl_semantic_check_binding, nl_semantic_check_loan_header,
        nl_semantic_check_source_fragment};
    return entries[entry];
}

typedef struct {
    NLSource *source;
    NLCheckedFragment *artifact;
    NLCheckDiagnostic diagnostic;
} TestChecked;
static inline void test_checked_destroy(TestChecked *checked)
{
    nl_checked_destroy(checked->artifact);
    nl_source_destroy(checked->source);
    *checked = (TestChecked){0};
}
static inline const NLCheckedNodeView *test_root(const TestChecked *checked)
{
    return nl_checked_node_view(checked->artifact,
                                nl_checked_root(checked->artifact));
}
static inline bool test_run(NLSemanticContext *context, const char *text,
                            TestEntry entry, NLCheckStatus expected,
                            const char *code, TestChecked *out)
{
    CHECK(nl_source_create(text, strlen(text), "semantic-fixture",
                           &out->source) == NL_SOURCE_OK);
    NLParser *parser = NULL;
    NLSyntaxTree *syntax = NULL;
    CHECK(nl_parser_create(out->source, &parser) == NL_PARSE_OK);
    CHECK(test_parse(entry)(parser, &syntax, NULL) == NL_PARSE_OK);
    const NLCheckStatus status =
        test_check(entry)(context, syntax, &out->artifact, &out->diagnostic);
    if (status != expected)
        fprintf(stderr, "%s: check %d expected %d (%s)\n", text, status,
                expected, out->diagnostic.diagnostic.code);
    CHECK(status == expected);
    nl_syntax_tree_destroy(syntax);
    nl_parser_destroy(parser);
    if (expected == NL_CHECK_OK) {
        CHECK(test_root(out) != NULL &&
              nl_source_span_valid(out->source, test_root(out)->span));
        CHECK(nl_checked_source(out->artifact) == out->source &&
              nl_checked_context(out->artifact) == context);
    } else {
        CHECK(out->artifact == NULL &&
              nl_source_span_valid(out->source, out->diagnostic.span));
        CHECK(out->diagnostic.diagnostic.severity == NL_DIAG_ERROR);
        CHECK(out->diagnostic.diagnostic.code != NULL &&
              out->diagnostic.diagnostic.message != NULL);
        if (code != NULL) {
            if (strcmp(out->diagnostic.diagnostic.code, code) != 0) {
                fprintf(stderr, "%s: expected %s, received %s\n", text, code,
                        out->diagnostic.diagnostic.code);
            }
            CHECK(strcmp(out->diagnostic.diagnostic.code, code) == 0);
        }
    }
    return true;
}

typedef struct {
    NLSemanticContext *context;
    NLTypeId copy, discardable, linear;
    NLSymbolId life;
    NLDomainId domain;
} TestSemantic;
static inline bool test_semantic_create(TestSemantic *fixture)
{
    CHECK(nl_semantic_create(&fixture->context) == NL_CHECK_OK);
    CHECK(nl_semantic_nominal(fixture->context, "CopyT", true, true,
                              &fixture->copy) == NL_CHECK_OK);
    CHECK(nl_semantic_nominal(fixture->context, "AffineT", false, true,
                              &fixture->discardable) == NL_CHECK_OK);
    CHECK(nl_semantic_nominal(fixture->context, "LinearT", false, false,
                              &fixture->linear) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_domain(fixture->context, "life", &fixture->life,
                                  &fixture->domain) == NL_CHECK_OK);
    return true;
}
static inline bool test_reference(NLSemanticContext *context, const char *name,
                                  NLPlaceId place, NLSemanticTypeKind kind,
                                  NLAccessSyntax access, bool exclusive,
                                  NLSymbolId *out, NLScopeId *out_scope)
{
    NLSemanticPlaceView p;
    CHECK(nl_semantic_place_view(context, place, &p));
    NLTypeId type = 0;
    NLScopeId scope = 0;
    CHECK(nl_semantic_compound_type(context, kind, p.type, access, exclusive,
                                    &type) == NL_CHECK_OK);
    if (kind == NL_TYPE_REF) {
        CHECK(nl_semantic_scope(context, 0, true, &scope) == NL_CHECK_OK);
    }
    CHECK(nl_semantic_seed_reference(
              context, name, type,
              (NLReferenceFacts){place, p.incarnation, scope,
                                 NL_PROVENANCE_VALID, true, true, 0},
              out) == NL_CHECK_OK);
    if (out_scope != NULL) {
        *out_scope = scope;
    }
    return true;
}
static inline bool test_domain_ref(TestSemantic *fixture, const char *name,
                                   NLAccessSyntax access, bool exclusive,
                                   NLSymbolId *out, NLScopeId *scope)
{
    NLSemanticBindingView binding;
    CHECK(nl_semantic_binding_view(fixture->context, fixture->life, &binding));
    return test_reference(fixture->context, name, binding.place, NL_TYPE_REF,
                          access, exclusive, out, scope);
}

static inline bool test_type_equal(NLSemanticTypeView a, NLSemanticTypeView b)
{
    return a.kind == b.kind && a.is_copy == b.is_copy &&
           a.is_discardable == b.is_discardable && a.target == b.target &&
           a.access == b.access && a.is_exclusive == b.is_exclusive &&
           a.layout_known == b.layout_known && a.field_count == b.field_count &&
           a.variant_count == b.variant_count && a.size == b.size &&
           a.alignment == b.alignment;
}
static inline bool test_reference_equal(NLReferenceFacts a, NLReferenceFacts b)
{
    return a.place == b.place && a.incarnation == b.incarnation &&
           a.scope == b.scope &&
           a.occurrence_dependency == b.occurrence_dependency &&
           a.provenance == b.provenance && a.readable == b.readable &&
           a.writable == b.writable;
}
static inline bool test_range_equal(NLBackingRange a, NLBackingRange b)
{
    return a.region == b.region && a.start == b.start && a.length == b.length;
}
static inline bool test_place_equal(NLSemanticPlaceView a,
                                    NLSemanticPlaceView b)
{
    return a.type == b.type && a.live == b.live &&
           a.independent_root == b.independent_root &&
           a.governing_domain == b.governing_domain &&
           a.incarnation == b.incarnation && a.current_fact == b.current_fact &&
           a.current_value == b.current_value &&
           a.payload_occurrence == b.payload_occurrence &&
           a.parent_sum == b.parent_sum &&
           test_range_equal(a.placement, b.placement);
}

/* Snapshot public state, not private layout or allocation capacities. Small
 * fixtures stay below 256 entries; resource-limit tests use counts separately.
 */
typedef struct {
    NLSemanticSnapshot counts;
    NLSemanticTypeView types[256];
    NLSemanticBindingView bindings[256];
    NLSemanticValueView values[256];
    NLSemanticPlaceView places[256];
    NLSemanticDomainView domains[256];
    NLSemanticScopeView scopes[256];
    NLSemanticOccurrenceView occurrences[256];
    NLSemanticBackingView regions[16];
    NLRawRepView raw[16][64];
} TestState;
static inline bool test_state(NLSemanticContext *context, TestState *state)
{
    CHECK(nl_semantic_snapshot(context, &state->counts));
    const NLSemanticSnapshot c = state->counts;
    CHECK(c.types <= 256 && c.bindings <= 256 && c.values <= 256 &&
          c.places <= 256 && c.domains <= 256 && c.scopes <= 256);
    for (size_t i = 0; i < c.types; ++i) {
        CHECK(nl_semantic_type_view(context, i + 1, &state->types[i]));
    }
    for (size_t i = 0; i < c.bindings; ++i) {
        CHECK(nl_semantic_binding_view(context, i + 1, &state->bindings[i]));
    }
    for (size_t i = 0; i < c.values; ++i) {
        CHECK(nl_semantic_value_view(context, i + 1, &state->values[i]));
    }
    for (size_t i = 0; i < c.places; ++i) {
        CHECK(nl_semantic_place_view(context, i + 1, &state->places[i]));
    }
    for (size_t i = 0; i < c.domains; ++i) {
        CHECK(nl_semantic_domain_view(context, i + 1, &state->domains[i]));
    }
    for (size_t i = 0; i < c.scopes; ++i) {
        CHECK(nl_semantic_scope_view(context, i + 1, &state->scopes[i]));
    }
    CHECK(c.occurrences <= 256);
    for (size_t i = 0; i < c.occurrences; ++i)
        CHECK(nl_semantic_occurrence_view(context, i + 1,
                                          &state->occurrences[i]));
    CHECK(c.backing_regions <= 16);
    for (size_t i = 0; i < c.backing_regions; ++i) {
        CHECK(nl_semantic_backing_view(context, i + 1, &state->regions[i]));
        const NLSemanticBackingView r = state->regions[i];
        if (r.live) {
            CHECK(r.size <= 64);
            for (size_t j = 0; j < r.size; ++j) {
                CHECK(nl_semantic_raw_rep_view(context, i + 1, j,
                                               &state->raw[i][j]));
            }
        }
    }
    return true;
}
static inline bool test_unchanged(NLSemanticContext *context,
                                  const TestState *before)
{
    TestState after;
    CHECK(test_state(context, &after));
    const NLSemanticSnapshot a = before->counts, b = after.counts;
    CHECK(a.types == b.types && a.bindings == b.bindings &&
          a.values == b.values && a.places == b.places &&
          a.domains == b.domains && a.scopes == b.scopes &&
          a.functions == b.functions &&
          a.last_incarnation == b.last_incarnation &&
          a.last_value_fact == b.last_value_fact &&
          a.backing_regions == b.backing_regions &&
          a.raw_intervals == b.raw_intervals && a.occurrences == b.occurrences);
    for (size_t i = 0; i < a.types; ++i) {
        CHECK(test_type_equal(before->types[i], after.types[i]));
    }
    for (size_t i = 0; i < a.bindings; ++i) {
        const NLSemanticBindingView x = before->bindings[i],
                                    y = after.bindings[i];
        CHECK(x.availability == y.availability && x.type == y.type &&
              x.value == y.value && x.place == y.place);
    }
    for (size_t i = 0; i < a.values; ++i) {
        const NLSemanticValueView x = before->values[i], y = after.values[i];
        CHECK(x.type == y.type && x.carrier == y.carrier &&
              x.owner_place == y.owner_place &&
              x.aggregate_owner == y.aggregate_owner &&
              x.field_count == y.field_count && x.variant == y.variant &&
              x.sum_payload == y.sum_payload && x.sum_owner == y.sum_owner &&
              memcmp(x.fields, y.fields, sizeof(x.fields)) == 0 &&
              x.dependencies == y.dependencies && x.domain == y.domain &&
              x.slot_place == y.slot_place &&
              x.allocation_region == y.allocation_region &&
              test_range_equal(x.occupancy, y.occupancy) &&
              x.scalar_known == y.scalar_known &&
              x.scalar_value == y.scalar_value &&
              test_reference_equal(x.reference, y.reference) &&
              x.reference_count == y.reference_count);
        for (size_t j = 0; j < x.reference_count; ++j)
            CHECK(test_reference_equal(x.references[j], y.references[j]));
    }
    for (size_t i = 0; i < a.places; ++i) {
        CHECK(test_place_equal(before->places[i], after.places[i]));
    }
    for (size_t i = 0; i < a.domains; ++i) {
        CHECK(before->domains[i].live == after.domains[i].live &&
              before->domains[i].value == after.domains[i].value);
    }
    for (size_t i = 0; i < a.scopes; ++i) {
        CHECK(before->scopes[i].active == after.scopes[i].active &&
              before->scopes[i].parent == after.scopes[i].parent &&
              before->scopes[i].parent_authority ==
                  after.scopes[i].parent_authority);
    }
    for (size_t i = 0; i < a.occurrences; ++i) {
        NLSemanticOccurrenceView x = before->occurrences[i],
                                 y = after.occurrences[i];
        CHECK(x.live == y.live && x.root == y.root &&
              x.payload_place == y.payload_place && x.variant == y.variant);
    }
    for (size_t i = 0; i < a.backing_regions; ++i) {
        const NLSemanticBackingView x = before->regions[i],
                                    y = after.regions[i];
        CHECK(x.live == y.live && x.size == y.size &&
              x.alignment == y.alignment &&
              x.ordinary_read == y.ordinary_read &&
              x.ordinary_write == y.ordinary_write &&
              x.address_known == y.address_known && x.address == y.address);
        if (x.live) {
            for (size_t j = 0; j < x.size; ++j) {
                const NLRawRepView u = before->raw[i][j], v = after.raw[i][j];
                CHECK(u.validity == v.validity &&
                      u.value_known == v.value_known && u.value == v.value);
            }
        }
    }
    return true;
}
static inline bool test_rejected(NLSemanticContext *context, const char *text,
                                 TestEntry entry, NLCheckStatus status,
                                 const char *code)
{
    TestState before;
    CHECK(test_state(context, &before));
    for (size_t repeat = 0; repeat < 2; ++repeat) {
        TestChecked checked = {0};
        CHECK(test_run(context, text, entry, status, code, &checked));
        CHECK(test_unchanged(context, &before));
        test_checked_destroy(&checked);
    }
    return true;
}

#endif
