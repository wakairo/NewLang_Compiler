#ifndef NEWLANG_IF_FIELD_INTERNAL_H
#define NEWLANG_IF_FIELD_INTERNAL_H
#include "copy_body_internal.h"
#include "newlang/if_field.h"
typedef struct NLIfFieldTrace {
    NLIfFieldEvent view, issued;
    const NLCheckedFragment *origin;
    size_t stream_index;
    NLSemanticContext *before, *after;
    NLCopyCallAnchor pre_anchor, post_anchor;
    struct NLIfFieldTrace *next;
} NLIfFieldTrace;
typedef struct NLIfFieldProof {
    NLIfFieldView view, issued;
    const NLCheckedFragment *owner;
    const NLSource *source;
    const NLFunctionBody *body;
    NLSemanticContext *entry, *post;
    NLCopyCallAnchor entry_anchor, post_anchor, arm_anchor[2];
    const NLCheckedFragment *arms[2];
    NLCheckedNodeView *nodes[2], *parent_nodes;
    const NLCheckedNodeView *node_origin[2], *parent_origin;
    size_t count[2], parent_count;
    NLCheckedNodeId arm_root[2], parent_root;
    NLIfFieldTrace *first[3], *last[3];
    NLSemanticContext *pending[3];
    NLCheckedNodeId pending_node[3];
    bool one_normal, replaying;
    NLIfReturnView return_view, return_issued;
    NLIfFieldArm replay;
    NLSemanticContext *return_live;
    NLCopyCallAnchor return_anchor, exit_anchor;
    const NLControlTarget *exit_target;
    const NLControlState *exit_state;
    size_t binding_floor, place_floor, scope_floor, return_arm;
    bool ready, complete, sealed;
    struct NLIfFieldProof *next;
} NLIfFieldProof;
NLPlaceId nl_if_call_root(const NLCheckedFragment *);
NLCheckStatus nl_if_call_join(NLCheckedFragment *, NLSemanticContext *);
NLCheckStatus nl_if_field_prepare(NLCheckedFragment *, NLCheckedNodeId,
                                  const NLSemanticContext *);
NLCheckStatus nl_if_field_fork(NLCheckedFragment *, NLCheckedNodeId, size_t,
                               NLCheckedFragment *);
NLCheckStatus nl_if_field_before(NLCheckedFragment *, NLCheckedNodeId,
                                 const NLSemanticContext *);
NLCheckStatus nl_if_field_event(NLCheckedFragment *, NLCheckedNodeId,
                                const NLSemanticContext *);
NLCheckStatus nl_if_field_plan(NLCheckedFragment *, NLCheckedNodeId,
                               const NLSemanticContext *, NLPlaceId *);
NLCheckStatus nl_if_field_commit(NLCheckedFragment *, NLCheckedNodeId,
                                 NLSemanticContext *);
NLCheckStatus nl_if_field_finish(NLCheckedFragment *, NLCheckedNodeId,
                                 const NLSemanticContext *);
NLCheckStatus nl_if_field_seal(NLCheckedFragment *);
NLCheckStatus nl_if_return_before(NLCheckedFragment *,
                                  const NLSemanticContext *, size_t bindings,
                                  size_t places, size_t scopes);
NLCheckStatus nl_if_return_after(NLCheckedFragment *, NLCheckedNodeId,
                                 const NLSemanticContext *);
NLCheckStatus nl_if_return_begin(NLCheckedFragment *, NLCheckedNodeId,
                                 size_t normal_arm);
NLCheckStatus nl_if_return_finish(NLCheckedFragment *, NLCheckedNodeId,
                                  const NLSemanticContext *);
#endif
