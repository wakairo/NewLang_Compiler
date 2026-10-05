#ifndef NEWLANG_SEMANTIC_INTERNAL_H
#define NEWLANG_SEMANTIC_INTERNAL_H

#include "newlang/checked.h"

typedef struct {
    char *name;
    NLSemanticTypeView view;
} NLTypeEntry;
typedef struct {
    char *name;
    NLSemanticBindingView view;
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
};
struct NLCheckedFragment {
    const NLSource *source;
    const NLSemanticContext *context;
    NLCheckedNodeView *nodes;
    size_t count, capacity;
    NLCheckedNodeId root;
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
NLCheckStatus nl_sem_new_domain(NLSemanticContext *, NLDomainId *, NLValueId *);
NLCheckStatus nl_sem_new_scope(NLSemanticContext *, NLScopeId, bool,
                               NLScopeId *);
NLCheckStatus nl_sem_fresh_fact(NLSemanticContext *, NLValueFactId *);
NLCheckStatus nl_checked_add(NLCheckedFragment *, NLCheckedNodeView,
                             NLCheckedNodeId *);

#endif
