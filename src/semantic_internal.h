#ifndef NEWLANG_SEMANTIC_INTERNAL_H
#define NEWLANG_SEMANTIC_INTERNAL_H

#include "control.h"
#include "newlang/checked.h"
#include "newlang/raw_storage.h"
#include "ordinary_name.h"

/* Borrowed spelling; only ordinary lexical names, never member labels.
 * Draft 17.14 preserves core/structural reasons without changing lexer tokens.
 */
bool nl_sem_lexical_name_admissible(const void *bytes, size_t length);

typedef struct {
    size_t start, length;
    NLRawRepView state;
} NLRawInterval;
typedef struct {
    NLSemanticBackingView view;
    NLRawInterval *intervals; /* owning full-range partition */
    size_t count;
} NLRawRegionEntry;

typedef struct {
    char *name;
    bool incomplete, recursive_header;
    NLTypeId
        option_target; /* canonical Option instantiation argument, not name */
    NLSemanticTypeView view;
    char *field_names[NL_SEMANTIC_MAX_FIELDS];
    NLTypeId field_types[NL_SEMANTIC_MAX_FIELDS];
    char *variant_names[NL_SEMANTIC_MAX_VARIANTS];
    NLTypeId variant_types[NL_SEMANTIC_MAX_VARIANTS];
} NLTypeEntry;
typedef struct {
    char *name;
    NLSemanticBindingView view;
    bool hidden; /* lexical scope ended; stable historical IDs remain */
} NLBindingEntry;
typedef struct NLFunctionBody {
    size_t owners;
    NLSource *source;
    NLSyntaxTree *syntax;
    char *parameter_names[NL_SEMANTIC_MAX_PARAMETERS];
    size_t count;
} NLFunctionBody; /* immutable plan; only ownership count is mutable */

typedef struct {
    const char *name; /* static for prelude; owned for registered entries */
    NLCheckedKind kind;
    NLTypeId *parameters;
    size_t count;
    NLTypeId result;
    bool caller_effects, hidden_dependencies;
    NLFunctionBody *body; /* owned retained plan, NULL for signature-only */
} NLFunctionEntry;
struct NLSemanticContext {
    NLSemanticOccurrenceView *occurrences;
    size_t occurrence_count;
    NLTypeEntry *types;
    size_t type_count;
    NLBindingEntry *bindings;
    size_t binding_count;
    NLSemanticValueView *values;
    size_t value_count;
    NLSemanticPlaceView *places;
    size_t place_count;
    NLSemanticDomainView *domains;
    size_t domain_count;
    NLSemanticScopeView *scopes;
    size_t scope_count;
    NLFunctionEntry *functions;
    size_t function_count;
    NLIncarnationId last_incarnation;
    NLValueFactId last_value_fact;
    NLRawRegionEntry *regions;
    size_t region_count, raw_interval_count;
};
typedef struct {
    NLCheckedNodeId match;
    struct NLCheckedFragment *artifact; /* owns hypothetical arm context */
} NLCheckedArm;
struct NLCheckedFragment {
    NLControlExits *loop_returns; /* owned early loop Return evidence */
    NLControlExits *exits;        /* owned complete finite control evidence */
    NLControlTarget *function_target; /* owned boundary identity, if any */
    const NLSource *source;
    const NLSemanticContext *context;
    NLCheckedNodeView *nodes;
    size_t count, capacity;
    NLCheckedNodeId root;
    NLCheckedArm *arms;
    size_t arm_count;
    NLValueFactId branch_fact_prefix; /* original current-value facts */
    size_t branch_value_prefix; /* proven original IDs for IF exit rebasing */
    void (*destroy_context)(NLSemanticContext *); /* arm snapshots only */
    struct NLCheckedFragment **bodies;
    NLCheckedNodeId *body_calls;
    size_t body_count;
    NLFunctionBody *body_owner;
    void (*release_body)(NLFunctionBody *);
};

/* All ref consumers either iterate this may-set or explicitly reject it.
 * A concrete fact remains the P3 representation when reference_count == 0. */
static inline size_t nl_sem_ref_count(NLSemanticValueView v)
{
    return v.reference_count == 0 ? 1 : v.reference_count;
}
static inline NLReferenceFacts nl_sem_ref_fact(NLSemanticValueView v, size_t i)
{
    return v.reference_count == 0 ? v.reference : v.references[i];
}

NLCheckStatus nl_body_create(const NLSyntaxTree *, const NLFunctionParameter *,
                             size_t, NLFunctionBody **);
NLCheckStatus nl_body_create_span(const NLSource *, NLSourceSpan,
                                  const NLFunctionParameter *, size_t,
                                  NLFunctionBody **);
NLCheckStatus nl_body_retain(NLFunctionBody *);
void nl_body_release(NLFunctionBody *);
/* Surviving packages after body/parameter cleanup may not reference newly
 * ending local scopes/places. Used by registration, real calls and fixtures. */
NLCheckStatus nl_sem_function_exit(const NLSemanticContext *,
                                   size_t scope_floor, size_t place_floor);

/* Concrete candidate state helpers; no general allocator/transaction DSL. */
NLCheckStatus nl_sem_clone(const NLSemanticContext *, NLSemanticContext **);
void nl_sem_commit(NLSemanticContext *, NLSemanticContext *);
NLCheckStatus nl_sem_compound(NLSemanticContext *, NLSemanticTypeKind, NLTypeId,
                              NLAccessSyntax, bool, NLTypeId *);
NLCheckStatus nl_sem_new_value(NLSemanticContext *, NLSemanticValueView,
                               NLValueId *);
NLCheckStatus nl_sem_new_place(NLSemanticContext *, NLTypeId, NLDomainId, bool,
                               NLValueId, NLPlaceId *);
NLCheckStatus nl_sem_install(NLSemanticContext *, NLPlaceId, NLValueId,
                             NLDomainId);
NLCheckStatus nl_sem_bind(NLSemanticContext *, const char *, NLValueId,
                          NLSymbolId *);
NLCheckStatus nl_sem_bind_in_scope(NLSemanticContext *, const char *, NLValueId,
                                   size_t binding_floor, NLSymbolId *);
NLCheckStatus nl_sem_copy_value(NLSemanticContext *, NLValueId, NLValueId *);
void nl_sem_end_value(NLSemanticContext *, NLValueId);
NLCheckStatus nl_sem_new_domain(NLSemanticContext *, NLDomainId *, NLValueId *);
NLCheckStatus nl_sem_new_scope(NLSemanticContext *, NLScopeId, bool,
                               NLScopeId *);
NLCheckStatus nl_sem_fresh_fact(NLSemanticContext *, NLValueFactId *);
bool nl_fixed_type(const NLSemanticContext *, NLTypeId);
bool nl_recursive_local_type(const NLSemanticContext *, NLTypeId);
/* Private transaction helpers. No header is committed until unit completion.
 * Strings/arrays are borrowed for the call; successful types own copies. */
NLCheckStatus nl_sem_nominal(NLSemanticContext *, const char *, bool, bool,
                             NLTypeId *);
NLCheckStatus nl_recursive_header(NLSemanticContext *, const char *,
                                  NLTypeId *);
NLCheckStatus nl_recursive_option(NLSemanticContext *, NLTypeId, NLTypeId *);
NLCheckStatus nl_recursive_complete(NLSemanticContext *, NLTypeId,
                                    const NLAggregateField *, size_t);
NLCheckStatus nl_recursive_validate(const NLSemanticContext *);
bool nl_recursive_value_type(const NLSemanticContext *, NLTypeId);
NLCheckStatus nl_fixed_attach(NLSemanticContext *, NLPlaceId);
void nl_fixed_detach(NLSemanticContext *, NLPlaceId);
NLCheckStatus nl_fixed_validate(const NLSemanticContext *);
NLCheckStatus nl_sem_validate(const NLSemanticContext *);
bool nl_fixed_overlap(const NLSemanticContext *, NLPlaceId, NLPlaceId);
bool nl_fixed_live(const NLSemanticContext *, NLPlaceId);
NLCheckStatus nl_fixed_dependencies(const NLSemanticContext *);
NLCheckStatus nl_fixed_change_dependencies(const NLSemanticContext *, NLPlaceId,
                                           NLValueId discarded);
NLCheckStatus nl_fixed_end_dependencies(const NLSemanticContext *, NLPlaceId,
                                        size_t ending_binding_floor);
NLCheckStatus nl_fixed_change(NLSemanticContext *, NLPlaceId, NLValueId, bool);
static inline bool nl_fixed_frame_same(NLSemanticPlaceView a,
                                       NLSemanticPlaceView b)
{
    if (a.parent_aggregate != b.parent_aggregate ||
        a.parent_incarnation != b.parent_incarnation ||
        a.parent_field_index != b.parent_field_index ||
        a.fixed_field_count != b.fixed_field_count)
        return false;
    for (size_t i = 0; i < NL_SEMANTIC_MAX_FIELDS; ++i)
        if (a.fixed_fields[i] != b.fixed_fields[i])
            return false;
    return true;
}
NLCheckStatus nl_checked_add(NLCheckedFragment *, NLCheckedNodeView,
                             NLCheckedNodeId *);

NLCheckStatus nl_sum_validate(const NLSemanticContext *);
NLCheckStatus nl_sum_attach(NLSemanticContext *, NLPlaceId);
void nl_sum_detach(NLSemanticContext *, NLPlaceId);
NLCheckStatus nl_sum_payload_changed(NLSemanticContext *, NLPlaceId);
bool nl_sum_authority(const NLSemanticContext *, NLTypeId);

void nl_raw_dispose(NLSemanticContext *);
NLCheckStatus nl_raw_clone(const NLSemanticContext *, NLSemanticContext *);
NLCheckStatus nl_raw_validate(const NLSemanticContext *);
NLCheckStatus nl_raw_start_root(NLSemanticContext *, NLValueId slot, NLPlaceId);
NLCheckStatus nl_raw_end_root(NLSemanticContext *, NLBackingRange,
                              NLValueId slot);
NLCheckStatus nl_raw_apply(NLSemanticContext *, const NLRawOperation *,
                           const NLValueId inputs[2], NLCheckedNodeView *,
                           NLCheckDiagnostic *);

/* Trusted host-known ordinary-function boundary, INTERNAL ONLY. The caller
 * constructs/owns the semantic setup; these floors define which scopes/places
 * end at function exit. No source declaration/context admission is exposed.
 * Clone/check/commit; failure leaves context and *out unchanged. Tree/source
 * borrowed, source must outlive the owned returned checked artifact.
 */
typedef struct {
    NLTypeId result;
    size_t bindings, scopes, places;
} NLFunctionBoundary;
NLCheckStatus nl_sem_check_function_block(NLSemanticContext *,
                                          const NLSyntaxTree *,
                                          NLFunctionBoundary,
                                          NLCheckedFragment **,
                                          NLCheckDiagnostic *);

#endif
