#ifndef NEWLANG_COPY_BODY_INTERNAL_H
#define NEWLANG_COPY_BODY_INTERNAL_H
#include "newlang/copy_body.h"
#include "semantic_internal.h"
#include <stdint.h>
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

/* Stage0 diagnostics ONLY. No observation is a proof or a live capability.
 * Explicitly enabled by a private synchronous observer; ordinary CLI is off.
 * All pointers borrow the current source check, and expire at callback return.
 */
typedef struct NLDefinitionCallCapture NLDefinitionCallCapture;
typedef struct {
    const NLCheckedFragment *caller, *callee, *parent;
    const NLSemanticContext *world, *pre, *post;
    const NLFunctionBody *body;
    size_t caller_function, function, arm, ordinal, pending_count;
    NLCheckedNodeId call, arguments[2], loan;
    NLSymbolId formals[2];
    NLCheckedField field;
    NLPlaceId sibling, other;
    NLScopeId scope;
} NLDefinitionCallView;
typedef struct {
    bool pre_verified, return_old_verified, loan_closed;
    bool old_known, sibling_known;
    uint64_t old_value, incoming, sibling;
    NLValueId returned;
    size_t replacements, failure_line;
} NLDefinitionCallResult;
typedef void (*NLDefinitionCallObserver)(const NLDefinitionCallCapture *,
                                         const NLDefinitionCallView *,
                                         NLCheckStatus, void *);
void nl_definition_call_observer(NLDefinitionCallObserver, void *);
NLDefinitionCallCapture *nl_definition_call_begin(const NLCheckedFragment *,
                                                  NLCheckedNodeId,
                                                  const NLCheckedNodeId[2],
                                                  size_t,
                                                  const NLSemanticContext *);
void nl_definition_call_attach(NLDefinitionCallCapture *,
                               const NLCheckedFragment *, const NLSymbolId[2],
                               const NLSemanticContext *);
void nl_definition_call_push(NLDefinitionCallCapture **,
                             NLDefinitionCallCapture *);
void nl_definition_call_finish(NLDefinitionCallCapture **, bool);
NLCheckStatus nl_definition_call_validate(const NLDefinitionCallCapture *,
                                          const NLDefinitionCallView *,
                                          NLDefinitionCallResult *);

/* P917 private NONPUBLISHING construction experiment. An explicit session
 * owns the actual failed definition tree and independent PRE/POST copies.
 * These are NOT Stage0 observer borrows or public checked proof. No callback
 * retention/reentry, public admission, IF join or backend entitlement. */
typedef struct NLDefinitionOwned NLDefinitionOwned;
typedef struct NLDefinitionOwnedPending NLDefinitionOwnedPending;
typedef struct {
    const NLCheckedFragment *caller, *callee, *parent;
    const NLSemanticContext *world, *pre, *post;
    const NLFunctionBody *body;
    size_t caller_function, function, arm, ordinal, pending_count;
    NLCheckedNodeId call, arguments[2], loan;
    NLSymbolId formals[2];
    NLCheckedField field;
    NLPlaceId sibling, other;
    NLScopeId scope;
} NLDefinitionOwnedLink;
NLCheckStatus nl_definition_owned_open(NLDefinitionOwned **);
void nl_definition_owned_close(NLDefinitionOwned *);
void nl_definition_owned_destroy(NLDefinitionOwned *);
size_t nl_definition_owned_count(const NLDefinitionOwned *);
NLCheckStatus nl_definition_owned_status(const NLDefinitionOwned *);
const NLDefinitionOwnedLink *nl_definition_owned_link(const NLDefinitionOwned *,
                                                      size_t);
NLCheckStatus nl_definition_owned_validate(const NLDefinitionOwned *, size_t,
                                           const NLDefinitionOwnedLink *,
                                           NLDefinitionCallResult *);
NLDefinitionOwnedPending *nl_definition_owned_call(const NLCheckedFragment *,
                                                   NLCheckedNodeId,
                                                   const NLCheckedNodeId[2],
                                                   size_t,
                                                   const NLSemanticContext *);
void nl_definition_owned_attach(NLDefinitionOwnedPending *,
                                const NLCheckedFragment *, const NLSymbolId[2],
                                const NLSemanticContext *);
void nl_definition_owned_take(NLCheckedFragment **, NLSemanticContext **);

/* P926 separate PRIVATE prefix experiment. Partial typed C fixed-memory only;
 * no accepted-rich/Domain reflection, public certificate, Change or join. */
typedef struct {
    bool registry_verified, roots_verified, condition_verified, entry_verified;
    bool truth;
    NLPlaceId roots[2];
    NLCheckedNodeId condition, conditional;
    size_t failure_line;
} NLDefinitionPrefixResult;
NLCheckStatus nl_definition_prefix_open(NLDefinitionOwned **);
void nl_definition_prefix_unit_begin(const NLSource *, size_t);
void nl_definition_prefix_plan(const NLSource *, NLSourceSpan, size_t,
                               NLFunctionBody *);
void nl_definition_prefix_unit_end(void);
NLCheckStatus nl_definition_prefix_validate(const NLDefinitionOwned *,
                                            NLDefinitionPrefixResult *);
#endif
