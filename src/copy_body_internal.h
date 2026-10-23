#ifndef NEWLANG_COPY_BODY_INTERNAL_H
#define NEWLANG_COPY_BODY_INTERNAL_H
#include "newlang/copy_body.h"
#include "semantic_internal.h"
typedef struct CopyEvent {
    NLCopyBodyEventView view, issued;
    NLSemanticContext *before, *after;
    NLSemanticPlaceView pre[3], post[3];
    NLSemanticValueView pre_values[3], post_values[3], result, incoming;
    NLSemanticScopeView *scopes_before, *scopes_after;
    size_t before_count, after_count;
    struct CopyEvent *next;
} CopyEvent;
typedef struct {
    NLSemanticPlaceView *places;
    NLSemanticValueView *values;
    NLSemanticScopeView *scopes;
    NLSemanticBindingView *bindings;
    size_t place_count, value_count, scope_count, binding_count;
} NLCopyCallAnchor;
typedef struct NLCopyBodyProof {
    const NLCheckedFragment *owner, *caller;
    const NLFunctionBody *body;
    const NLSource *source;
    NLCheckedNodeId call;
    NLCopyBodyView view;
    CopyEvent *first, *last;
    NLSemanticContext *pending, *exit;
    NLSemanticScopeView *exit_scopes;
    size_t scope_count;
    NLCheckedNodeId pending_node;
    NLCheckedNodeView *nodes;
    const NLCheckedNodeView *nodes_origin;
    size_t count;
    bool sealed;
    bool formal_write, unit_call;
    NLSemanticContext *normal_end;
    NLCopyCallAnchor normal_end_anchor;
    NLCheckedNodeId conditional;
    size_t binding_floor, place_floor, scope_floor;
    NLCopyBodyParameters parameters, issued_parameters;
    NLSemanticContext *entry, *handoff;
    NLCopyCallAnchor entry_anchor, handoff_anchor;
    NLCheckedNodeView arguments[2], call_node, loan_node;
    NLCheckedNodeView *caller_nodes;
    const NLCheckedNodeView *caller_nodes_origin;
    size_t caller_count;
    NLCheckedNodeId caller_root;
    const NLSource *caller_source;
    const NLFunctionBody *caller_body;
    NLTypeId parameter_types[2];

} NLCopyBodyProof;

NLCheckStatus nl_copy_body_event(NLCheckedFragment *, NLCheckedNodeId,
                                 const NLSemanticContext *, bool placement);
NLCheckStatus nl_copy_body_before(NLCheckedFragment *, NLCheckedNodeId,
                                  const NLSemanticContext *);
NLCheckStatus nl_copy_body_exit(NLCheckedFragment *, const NLSemanticContext *,
                                NLCheckedResult);
NLCheckStatus nl_copy_body_seal(NLCheckedFragment *, const NLCheckedFragment *,
                                NLCheckedNodeId, size_t,
                                const NLSemanticContext *);
NLCheckStatus nl_copy_body_seal_calls(NLCheckedFragment *);

NLCheckStatus nl_copy_body_parameters(NLCheckedFragment *,
                                      const NLCheckedFragment *,
                                      NLCheckedNodeId, const NLSymbolId[2],
                                      const NLSemanticContext *);
NLCheckStatus nl_copy_unit_entry_validate(const NLCheckedFragment *);
NLCheckStatus nl_copy_end_locals(NLSemanticContext *, size_t, size_t, size_t);
void nl_copy_state_destroy(NLCopyCallAnchor *);
NLCheckStatus nl_copy_state_anchor(const NLSemanticContext *,
                                   NLCopyCallAnchor *);
bool nl_copy_state_matches(const NLSemanticContext *, const NLCopyCallAnchor *);
bool nl_copy_trio(const NLSemanticContext *, NLPlaceId, NLSemanticPlaceView[3]);
bool nl_copy_plain(const NLSemanticContext *, NLValueId, NLTypeId);
bool nl_copy_same_types(const NLSemanticContext *, const NLSemanticContext *);
#endif
