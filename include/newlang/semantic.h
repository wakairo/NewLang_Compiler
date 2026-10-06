#ifndef NEWLANG_SEMANTIC_H
#define NEWLANG_SEMANTIC_H

#include "newlang/diagnostic.h"
#include "newlang/syntax.h"

typedef struct NLSemanticContext NLSemanticContext;
typedef struct NLCheckedFragment NLCheckedFragment;

/* Stable, append-only, context-local identities; zero is absent/invalid.
 * Identity categories are distinct contracts even though C uses size_t.
 * Never mix IDs from different contexts or categories. */
#define NL_SEMANTIC_MAX_FIELDS                                                 \
    16 /* P5 implementation budget, not language limit */

typedef size_t NLTypeId;
typedef size_t NLSymbolId;
typedef size_t NLValueId;
typedef size_t NLDomainId;
typedef size_t NLPlaceId;
typedef size_t NLIncarnationId;
typedef size_t NLValueFactId;
typedef size_t NLScopeId;
typedef size_t
    NLOccurrenceId; /* conditional memory occurrence, never value-owned */
typedef size_t NLBackingRegionId; /* never a numeric address or authority */

typedef struct {
    NLBackingRegionId region;
    size_t start, length;
} NLBackingRange;

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
    NL_TYPE_SLOT,
    NL_TYPE_ALLOCATION,
    NL_TYPE_STORAGE,
    NL_TYPE_BYTE,
    NL_TYPE_U8,
    NL_TYPE_USIZE,
    NL_TYPE_ADDR,
    NL_TYPE_SUM
} NLSemanticTypeKind;
typedef struct {
    NLSemanticTypeKind kind;
    bool is_copy;
    bool is_discardable;
    NLTypeId target;
    NLAccessSyntax access;
    bool is_exclusive;
    bool layout_known;
    size_t variant_count; /* registered closed sum, zero otherwise */
    size_t field_count;   /* registered aggregate shape; zero for flat types */
    size_t size, alignment; /* compiler/target facts, not aggregate ABI */
} NLSemanticTypeView;

typedef struct {
    NLTypeId type;
    bool known;
    size_t value;
} NLScalarValue;

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
    NL_CARRIER_ENDED,
    NL_CARRIER_AGGREGATE,
    NL_CARRIER_SUM
} NLValueCarrier;
typedef struct {
    NLPlaceId place;
    NLIncarnationId incarnation;
    NLScopeId scope; /* refs only; ptrs are scope-independent */
    NLProvenance provenance;
    bool readable;
    bool writable;
    NLOccurrenceId
        occurrence_dependency; /* refs only, separate from provenance */
} NLReferenceFacts;
/* Bounded may-set, not a language limit. Inline facts are owned by the value
 * table; no borrowed pointers or branch snapshot IDs. When count != 0 the
 * singular reference field is absent and must not be used by consumers. */
#define NL_SEMANTIC_MAX_REF_ALTERNATIVES 16
typedef struct {
    NLTypeId type;
    NLValueCarrier carrier;
    NLPlaceId owner_place; /* carrier location, not pointer/ref referent */
    NLDependencyKnowledge dependencies;
    NLDomainId domain; /* LifetimeDomain value identity only */
    NLReferenceFacts reference;
    size_t reference_count; /* zero: concrete reference; nonzero: joined ref */
    NLReferenceFacts references[NL_SEMANTIC_MAX_REF_ALTERNATIVES];
    NLPlaceId slot_place; /* unique empty typed occupancy responsibility */
    NLBackingRegionId allocation_region; /* final-deallocation authority */
    NLBackingRange occupancy; /* Storage/slot responsibility, value-owned */
    NLValueId
        aggregate_owner; /* member package carrier, not place/incarnation */
    size_t field_count;
    NLValueId
        fields[NL_SEMANTIC_MAX_FIELDS]; /* declaration order; owned members */
    size_t variant; /* one-based registered variant; zero means unknown */
    NLValueId sum_payload,
        sum_owner; /* owned package, no occurrence identity */
    bool scalar_known;
    size_t scalar_value;
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
    NLOccurrenceId payload_occurrence; /* live sum root only */
    NLPlaceId parent_sum;     /* conditional subplace, not independent root */
    NLBackingRange placement; /* live root only; not part of its value */
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
    size_t backing_regions, raw_intervals, occurrences;
} NLSemanticSnapshot;
typedef struct {
    NLDiagnostic diagnostic;
    NLSourceSpan span;
} NLCheckDiagnostic;

/* Implementation budgets, not NewLang limits. */
#define NL_SEMANTIC_MAX_ENTRIES 4096
#define NL_SEMANTIC_MAX_PARAMETERS 128
#define NL_SEMANTIC_MAX_DEPTH 128

/* Context owns names/tables/packages. It registers unit/LifetimeDomain and P4
 * scalar/authority types and a fixed semantic prelude, separate from parser
 * keywords. Names passed below are borrowed NUL-terminated host API strings,
 * copied on success. Every mutating API is transactional. Failures leave
 * observable state and output IDs unchanged. Owner slots must be initialized
 * NULL. NULL destruction is allowed; no globals/LLVM/oracle/Lean dependencies
 * or source declarations. IDs/views are meaningful only in their original live
 * context. Getters copy views (no pointer into mutable storage); IDs survive
 * successful mutations. */
NLCheckStatus nl_semantic_create(NLSemanticContext **out_context);
void nl_semantic_destroy(NLSemanticContext *context);
NLTypeId nl_semantic_unit_type(const NLSemanticContext *context);
NLTypeId nl_semantic_domain_type(const NLSemanticContext *context);
NLTypeId nl_semantic_core_type(const NLSemanticContext *, NLSemanticTypeKind);
/* Trusted, stable target layout facts. A known layout cannot be revised by an
 * ordinary check/registration; zero-sized storable layouts are invalid. */
NLCheckStatus nl_semantic_set_layout(NLSemanticContext *, NLTypeId, size_t size,
                                     size_t alignment);
NLCheckStatus nl_semantic_seed_scalar(NLSemanticContext *, const char *name,
                                      NLScalarValue, NLSymbolId *out);
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
typedef struct {
    const char *name; /* borrowed during registration; context copies it */
    NLTypeId type;
} NLAggregateField;
/* Fixed flat nominal aggregate fixture; no source declaration/physical layout.
 * Properties derive from all fields. This slice supports dependency-free flat
 * nominal/scalar members, not nested aggregates or authority/capability fields.
 * Unsupported kinds are reported, never treated as invalid language. */
NLCheckStatus nl_semantic_register_aggregate(NLSemanticContext *, const char *,
                                             const NLAggregateField *, size_t,
                                             NLTypeId *out);

#define NL_SEMANTIC_MAX_VARIANTS 16 /* host budget, not language limit */
typedef struct {
    const char *name; /* borrowed registration input; copied on success */
    NLTypeId payload; /* zero: payloadless; otherwise exactly one type */
} NLSumVariant;
typedef struct {
    bool live;
    NLPlaceId root, payload_place;
    size_t variant;
} NLSemanticOccurrenceView;
/* Concrete nominal registry. Flat payloads only in P6; no source declarations,
 * nested sums, aggregates or ref/ptr payloads. Static traits use every variant.
 * Value copies own independent payload packages. Historical occurrences never
 * travel with a semantic value package. All mutations remain transactional. */
NLCheckStatus nl_semantic_register_sum(NLSemanticContext *, const char *,
                                       const NLSumVariant *, size_t,
                                       NLTypeId *);
bool nl_semantic_occurrence_view(const NLSemanticContext *, NLOccurrenceId,
                                 NLSemanticOccurrenceView *);

NLCheckStatus nl_semantic_compound_type(NLSemanticContext *, NLSemanticTypeKind,
                                        NLTypeId target, NLAccessSyntax access,
                                        bool is_exclusive, NLTypeId *out);
/* Trusted fixture/context facts, not source declarations or a proof of physical
 * storage. Value seeds permit nominal/unit and scalar types; explicit known
 * scalars use seed_scalar. Allocation/Storage cannot be seeded (domain has its
 * own seed). Root sites are distinct/disjoint flat typed sites;
 * independent_root=false exists for negative fixtures. References may
 * explicitly have stale/unknown/ invalid facts, which safe operations check
 * rather than assume away. */
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
/* Join ordinary flat ref packages already belonging to THIS public context.
 * Produces an owned loose Copy package with a bounded complete fact may-set.
 * Exact static types required. Every alternative must be live. No branch IDs,
 * implicit borrow, memory-state join, write mutation or ptr conversion. */
NLCheckStatus nl_semantic_join_references(NLSemanticContext *,
                                          const NLValueId *values, size_t count,
                                          NLValueId *out);
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

typedef struct {
    const char *name; /* borrowed for registration; copied on success */
    NLTypeId type;
} NLFunctionParameter;
/* Definition-time checked, non-generic, exact single result. Root must be the
 * existing lexical BLOCK. Owns a durable syntax/source plan independent of the
 * caller's tree/source/parameter names. Body calls are interpreted against
 * actual facts in a transactional context; no textual substitution. Invalid
 * uncalled bodies reject. Acyclic body calls share this model; recursive
 * analysis is precision-rejected. No overloads or generic declarations. */
NLCheckStatus nl_semantic_register_function_body(NLSemanticContext *,
                                                 const char *name,
                                                 const NLFunctionParameter *,
                                                 size_t count, NLTypeId result,
                                                 const NLSyntaxTree *body,
                                                 NLCheckDiagnostic *diagnostic);

/* Bounded Draft 17.13 fn-only semantic unit. Input array/trees/sources are
 * borrowed only for this synchronous call. All signatures and owned bodies
 * are established in a private candidate before definition checking; commit
 * only after every declaration succeeds. Success leaves diagnostic untouched.
 * Failure leaves public state unchanged and reports input index + source span.
 * Durable plans survive destruction of input trees/sources/array. No module,
 * driver, separate-compilation or source nominal-type declaration machinery.
 * Names are collected in deterministic spelling order; IDs are not file IDs. */
#define NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS 128
typedef struct {
    size_t input_index;
    NLCheckDiagnostic diagnostic;
} NLFunctionUnitDiagnostic;
NLCheckStatus nl_semantic_register_function_unit(
    NLSemanticContext *, const NLSyntaxTree *const *inputs, size_t count,
    NLFunctionUnitDiagnostic *diagnostic);

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
/* Draft 17.10 selected closed source profile; same candidate/artifact ownership
 * contracts. Blocks check lexical exits; registered sums support constructor/
 * match, with owned hypothetical arm evidence and conservative common joining.
 */
NLCheckStatus nl_semantic_check_source_fragment(NLSemanticContext *,
                                                const NLSyntaxTree *,
                                                NLCheckedFragment **,
                                                NLCheckDiagnostic *);
bool nl_check_diagnostic_render(FILE *, const NLSource *,
                                const NLCheckDiagnostic *);

#endif
