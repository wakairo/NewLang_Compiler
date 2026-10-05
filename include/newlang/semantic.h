#ifndef NEWLANG_SEMANTIC_H
#define NEWLANG_SEMANTIC_H

#include "newlang/diagnostic.h"
#include "newlang/syntax.h"

typedef struct NLSemanticContext NLSemanticContext;
typedef struct NLCheckedFragment NLCheckedFragment;

/* Stable, append-only, context-local identities; zero is absent/invalid.
 * Identity categories are distinct contracts even though C uses size_t.
 * Never mix IDs from different contexts or categories. */
typedef size_t NLTypeId;
typedef size_t NLSymbolId;
typedef size_t NLValueId;
typedef size_t NLDomainId;
typedef size_t NLPlaceId;
typedef size_t NLIncarnationId;
typedef size_t NLValueFactId;
typedef size_t NLScopeId;

typedef enum {
    NL_CHECK_OK,
    NL_CHECK_SEMANTIC_ERROR,
    NL_CHECK_SEMANTIC_UNSUPPORTED,
    NL_CHECK_ANALYSIS_PRECISION_LIMIT,
    NL_CHECK_OUT_OF_MEMORY,
    NL_CHECK_RESOURCE_LIMIT,
    NL_CHECK_INTERNAL_ERROR
} NLCheckStatus;

typedef enum {
    NL_TYPE_UNIT,
    NL_TYPE_NOMINAL,
    NL_TYPE_PTR,
    NL_TYPE_REF,
    NL_TYPE_SLOT
} NLSemanticTypeKind;
typedef struct {
    NLSemanticTypeKind kind;
    bool is_copy;
    bool is_discardable;
    NLTypeId target;
    NLAccessSyntax access;
    bool is_exclusive;
} NLSemanticTypeView;

typedef enum {
    NL_AVAILABLE,
    NL_CONSUMED
} NLAvailability;
typedef enum {
    NL_DEPENDENCY_FREE,
    NL_HIDDEN_DEPENDENCIES,
    NL_DEPENDENCIES_UNKNOWN
} NLDependencyKnowledge;
typedef enum {
    NL_PROVENANCE_UNKNOWN,
    NL_PROVENANCE_VALID,
    NL_PROVENANCE_INVALID
} NLProvenance;
typedef enum {
    NL_CARRIER_LOOSE,
    NL_CARRIER_PLACE,
    NL_CARRIER_ENDED
} NLValueCarrier;
typedef struct {
    NLPlaceId place;
    NLIncarnationId incarnation;
    NLScopeId scope; /* refs only; ptrs are scope-independent */
    NLProvenance provenance;
    bool readable;
    bool writable;
} NLReferenceFacts;
typedef struct {
    NLTypeId type;
    NLValueCarrier carrier;
    NLPlaceId owner_place; /* carrier location, not pointer/ref referent */
    NLDependencyKnowledge dependencies;
    NLDomainId domain; /* LifetimeDomain value identity only */
    NLReferenceFacts reference;
    NLPlaceId slot_place; /* unique empty typed occupancy responsibility */
} NLSemanticValueView;
typedef struct {
    NLAvailability availability;
    NLTypeId type;
    NLValueId value;
    NLPlaceId place; /* compiler-managed local carrier; not ref referent */
} NLSemanticBindingView;
typedef struct {
    NLTypeId type;
    bool live;
    bool independent_root;
    NLDomainId governing_domain; /* zero: implicit compiler-managed local */
    NLIncarnationId incarnation;
    NLValueFactId current_fact;
    NLValueId current_value;
} NLSemanticPlaceView;
typedef struct {
    bool live;
    NLValueId value;
} NLSemanticDomainView;
typedef struct {
    bool active;
    NLScopeId parent;
    NLValueId
        parent_authority; /* generated child suspends this exact package */
} NLSemanticScopeView;
typedef struct {
    size_t types, bindings, values, places, domains, scopes, functions;
    NLIncarnationId last_incarnation;
    NLValueFactId last_value_fact;
} NLSemanticSnapshot;
typedef struct {
    NLDiagnostic diagnostic;
    NLSourceSpan span;
} NLCheckDiagnostic;

/* Implementation budgets, not NewLang limits. */
#define NL_SEMANTIC_MAX_ENTRIES 4096
#define NL_SEMANTIC_MAX_PARAMETERS 128
#define NL_SEMANTIC_MAX_DEPTH 128

/* Context owns names/tables/packages. It registers unit/LifetimeDomain types
 * and a fixed semantic prelude, separate from parser keywords. Names passed
 * below are borrowed NUL-terminated host API strings, copied on success.
 * Every mutating API is transactional. Failures leave observable state and
 * output IDs unchanged. Owner slots must be initialized NULL. NULL destruction
 * is allowed; no globals/LLVM/oracle/Lean dependencies or source declarations.
 * IDs/views are meaningful only in their original live context. Getters copy
 * views (no pointer into mutable storage); IDs survive successful mutations. */
NLCheckStatus nl_semantic_create(NLSemanticContext **out_context);
void nl_semantic_destroy(NLSemanticContext *context);
NLTypeId nl_semantic_unit_type(const NLSemanticContext *context);
NLTypeId nl_semantic_domain_type(const NLSemanticContext *context);
bool nl_semantic_type_view(const NLSemanticContext *, NLTypeId,
                           NLSemanticTypeView *);
bool nl_semantic_binding_view(const NLSemanticContext *, NLSymbolId,
                              NLSemanticBindingView *);
bool nl_semantic_value_view(const NLSemanticContext *, NLValueId,
                            NLSemanticValueView *);
bool nl_semantic_place_view(const NLSemanticContext *, NLPlaceId,
                            NLSemanticPlaceView *);
bool nl_semantic_domain_view(const NLSemanticContext *, NLDomainId,
                             NLSemanticDomainView *);
bool nl_semantic_scope_view(const NLSemanticContext *, NLScopeId,
                            NLSemanticScopeView *);
bool nl_semantic_snapshot(const NLSemanticContext *, NLSemanticSnapshot *);
NLSymbolId nl_semantic_find_binding(const NLSemanticContext *,
                                    const char *name);

NLCheckStatus nl_semantic_nominal(NLSemanticContext *, const char *name,
                                  bool is_copy, bool is_discardable,
                                  NLTypeId *out);
NLCheckStatus nl_semantic_compound_type(NLSemanticContext *, NLSemanticTypeKind,
                                        NLTypeId target, NLAccessSyntax access,
                                        bool is_exclusive, NLTypeId *out);
/* Trusted fixture/context facts, not source declarations or a proof of physical
 * storage. Scalar seeds require nominal/unit types (domain has its own seed).
 * Root sites are distinct/disjoint flat typed sites; independent_root=false
 * exists for negative fixtures. References may explicitly have stale/unknown/
 * invalid facts, which safe operations check rather than assume away. */
NLCheckStatus nl_semantic_seed_value(NLSemanticContext *, const char *name,
                                     NLTypeId, NLDependencyKnowledge,
                                     NLSymbolId *out);
NLCheckStatus nl_semantic_seed_domain(NLSemanticContext *, const char *name,
                                      NLSymbolId *out_symbol,
                                      NLDomainId *out_domain);
NLCheckStatus nl_semantic_seed_root(NLSemanticContext *, NLTypeId, NLDomainId,
                                    bool independent_root,
                                    NLDependencyKnowledge, NLPlaceId *out_place,
                                    NLValueId *out_value);
NLCheckStatus nl_semantic_seed_slot(NLSemanticContext *, const char *name,
                                    NLTypeId target, NLSymbolId *out,
                                    NLPlaceId *out_place);
NLCheckStatus nl_semantic_seed_reference(NLSemanticContext *, const char *name,
                                         NLTypeId ptr_or_ref, NLReferenceFacts,
                                         NLSymbolId *out);
NLCheckStatus nl_semantic_scope(NLSemanticContext *, NLScopeId parent,
                                bool active, NLScopeId *out);
NLCheckStatus nl_semantic_end_scope(NLSemanticContext *, NLScopeId);
/* Transfer a loose result responsibility into a fixture binding; this does not
 * implement multi-result source receiving syntax or silently discard results.
 */
NLCheckStatus nl_semantic_bind_result(NLSemanticContext *, const char *name,
                                      NLValueId, NLSymbolId *out);
/* One exact signature, nominal/unit result, no overloads. Effects/dependencies
 * can be marked for explicit unsupported/precision rejection by the checker.
 * Ref arguments are nonescaping in this tiny registered-call contract; no
 * returned scope-dependent capability or affine core-authority summary. */
NLCheckStatus nl_semantic_register_function(NLSemanticContext *,
                                            const char *name,
                                            const NLTypeId *parameters,
                                            size_t count, NLTypeId result,
                                            bool caller_effects,
                                            bool hidden_dependencies);

/* Borrow input tree only for synchronous check; never reparse source. All
 * checks clone candidate state and commit on complete success only. Success
 * returns a caller-owned immutable checked artifact and leaves diagnostic
 * untouched. Failure cleans candidate/artifact, leaves context/owner slot
 * unchanged, writes first diagnostic if supplied. Invalid API arguments leave
 * all outputs unchanged. Context remains reusable; non-reentrant per context.
 * Artifact survives syntax destruction. It owns nodes/spans, borrows context
 * identity meanings and source only for optional text access; those owners must
 * outlive corresponding lookups/access, never artifact destruction itself. */
NLCheckStatus nl_semantic_check_type(NLSemanticContext *, const NLSyntaxTree *,
                                     NLCheckedFragment **out,
                                     NLCheckDiagnostic *);
NLCheckStatus nl_semantic_check_expression(NLSemanticContext *,
                                           const NLSyntaxTree *,
                                           NLCheckedFragment **out,
                                           NLCheckDiagnostic *);
NLCheckStatus nl_semantic_check_binding(NLSemanticContext *,
                                        const NLSyntaxTree *,
                                        NLCheckedFragment **out,
                                        NLCheckDiagnostic *);
NLCheckStatus nl_semantic_check_loan_header(NLSemanticContext *,
                                            const NLSyntaxTree *,
                                            NLCheckedFragment **out,
                                            NLCheckDiagnostic *);
bool nl_check_diagnostic_render(FILE *, const NLSource *,
                                const NLCheckDiagnostic *);

#endif
