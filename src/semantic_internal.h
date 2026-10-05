#ifndef NEWLANG_SEMANTIC_INTERNAL_H
#define NEWLANG_SEMANTIC_INTERNAL_H

#include "newlang/checked.h"
#include "newlang/raw_storage.h"

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
typedef struct {
    const char *name; /* static for prelude; owned for registered entries */
    NLCheckedKind kind;
    NLTypeId *parameters;
    size_t count;
    NLTypeId result;
    bool caller_effects, hidden_dependencies;
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
    const NLSource *source;
    const NLSemanticContext *context;
    NLCheckedNodeView *nodes;
    size_t count, capacity;
    NLCheckedNodeId root;
    NLCheckedArm *arms;
    size_t arm_count;
    void (*destroy_context)(NLSemanticContext *); /* arm snapshots only */
};

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

#endif
