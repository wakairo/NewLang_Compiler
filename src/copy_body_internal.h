#ifndef NEWLANG_COPY_BODY_INTERNAL_H
#define NEWLANG_COPY_BODY_INTERNAL_H
#include "newlang/copy_body.h"
#include "semantic_internal.h"
/* Allocation extents only, not semantic evidence or an authority issuer.
 * A replay clone reads every registered signature/name/body in its world. */
typedef struct {
    const NLTypeId *parameters;
    size_t count, name_length;
    const char *name;
    const NLFunctionBody *body;
    NLCheckedKind kind;
} CopySignatureOrigin;
typedef struct {
    const NLFunctionEntry *functions;
    size_t count;
    CopySignatureOrigin *entries;
} CopySignatureSet;
typedef struct CopyEvent {
    NLCopyBodyEventView view, issued;
    NLSemanticContext *before, *after;
    NLSemanticPlaceView pre[3], post[3];
    NLSemanticValueView pre_values[3], post_values[3], result, incoming;
    NLSemanticScopeView *scopes_before, *scopes_after;
    size_t before_count, after_count;
    NLSemanticContext before_storage, after_storage;
    CopySignatureSet before_signatures, after_signatures;
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
    /* Borrowed node identity of the existing CHECK-owned current relation.
     * No event, world or independent proof is issued here. */
    NLCheckedNodeId current_loop;
    /* The exact existing current-call child forwarded by this Copy body.
     * Its own current record remains the sole authority for the inner loop. */
    NLCheckedNodeId current_call;
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
    /* The same straight-line event replay with a free readonly u8 input.
     * There is no inner current loop or second certificate. */
    bool readonly_local;
    CopySignatureSet entry_signatures, exit_signatures, handoff_signatures,
        caller_signatures;
    NLSemanticContext handoff_storage;
    const NLSemanticContext *local_caller_world;
    NLSemanticContext local_caller_storage;
    const NLSyntaxTree *readonly_syntax;
    /* A plain read-only scalar formal, distinct from ref-write parameters.
     * The existing entry anchor and final caller seal own this handoff. */
    NLSymbolId readonly_parameter;
    NLCheckedNodeId readonly_argument;
    NLSemanticContext readonly_storage;
    NLSemanticContext readonly_exit_storage;
    const NLTypeId *readonly_entry_parameters;
    const char *readonly_name, *readonly_plan_name;
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
NLCheckStatus nl_copy_body_current(NLCheckedFragment *, NLCheckedNodeId);
NLCheckStatus nl_copy_body_forward_current(NLCheckedFragment *,
                                           const NLCheckedFragment *,
                                           NLCheckedNodeId);
/* Origin/graph checks only: deliberately does not recurse into a validator. */
bool nl_copy_body_forward_origin(const NLCheckedFragment *);
NLCheckStatus nl_copy_body_readonly(NLCheckedFragment *,
                                    const NLCheckedFragment *, NLCheckedNodeId,
                                    NLSymbolId, const NLSemanticContext *);
NLCheckStatus nl_copy_body_local_entry(NLCheckedFragment *,
                                       const NLCheckedFragment *,
                                       NLCheckedNodeId, NLSymbolId,
                                       const NLSemanticContext *, size_t,
                                       size_t, size_t);
/* Borrowed contents; validation still consumes the existing full Copy replay.
 * Definition uses it privately; it is never a new public source issuer. */
typedef struct {
    const NLCheckedFragment *body;
    NLSymbolId formal;
    size_t function;
} NLCopyLocalCall;
bool nl_copy_body_local_contents(const NLCheckedFragment *, NLCheckedNodeId,
                                 NLCopyLocalCall *);
NLCheckStatus nl_copy_body_local_validate(const NLCheckedFragment *,
                                          NLCheckedNodeId);
/* Called after the current relation authenticates current signature origins.
 * Consumes this same Copy entry/caller seal; never calls a current validator.
 */
bool nl_copy_body_readonly_valid(const NLCheckedFragment *);

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
