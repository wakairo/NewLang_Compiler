#ifndef NEWLANG_NODE_JOIN_H
#define NEWLANG_NODE_JOIN_H

#include "newlang/checked.h"

/* Non-normative finite proof view. All pointers borrow the owning fragment;
 * IDs are qualified by the indicated world, never addresses or authority.
 * Exactly two lexical original Nodes, one fixed next, two normal unit arms.
 * Copying this view does not extend the artifact/context lifetime. */
typedef struct {
    const NLSemanticContext *entry, *post;
    NLCheckedNodeId match;
    NLValueId input;
    NLPlaceId originals[2], changed, sibling;
    struct {
        const NLCheckedFragment *artifact;
        const NLSemanticContext *entry, *world;
        const NLSemanticContext *before_change, *after_change;
        size_t variant;
        NLCheckedNodeId operation; /* zero for the unchanged arm */
        NLCheckedField change;
    } arms[2];
} NLNodeJoinView;

/* Observation only, output unchanged if absent. No proof for an old artifact,
 * unchanged-frame match, heap owner join or other unsupported profile. */
bool nl_checked_node_join_view(const NLCheckedFragment *, NLCheckedNodeId,
                               NLNodeJoinView *);
/* Independently reverify owned entry/fork/effects and rederive joined post.
 * candidate may be NULL (validate issued proof) or a copied view to validate
 * against this exact owner. Arbitrary/foreign candidate pointers are compared,
 * not dereferenced. Returns ANALYSIS_PRECISION_LIMIT for absent/invalid proof,
 * explicit OOM/resource failure; never mutates source worlds or candidate.
 * Validity grants no new liveness/domain/ownership or C17 permission. Link
 * semantic library. The artifact and its semantic context must remain alive. */
NLCheckStatus nl_checked_node_join_validate(const NLCheckedFragment *,
                                            NLCheckedNodeId,
                                            const NLNodeJoinView *candidate);
#endif
