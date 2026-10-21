#ifndef NEWLANG_IF_FIELD_INTERNAL_H
#define NEWLANG_IF_FIELD_INTERNAL_H
#include "copy_body_internal.h"
#include "newlang/if_field.h"
typedef struct NLIfFieldTrace {
    NLIfFieldEvent view, issued;
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
    NLIfFieldTrace *first[2], *last[2];
    NLSemanticContext *pending[2];
    NLCheckedNodeId pending_node[2];
    bool ready, complete, sealed;
    struct NLIfFieldProof *next;
} NLIfFieldProof;
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
#endif
