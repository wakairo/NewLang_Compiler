#ifndef NEWLANG_IF_FIELD_H
#define NEWLANG_IF_FIELD_H
#include "newlang/checked.h"
/* Borrowed historical source-issued observations. These are not present
 * references, lifetime/ownership authority, or a serialized proof format. */
typedef struct {
    const NLCheckedFragment *artifact;
    const NLSemanticContext *world;
    size_t event_count, change_count;
} NLIfFieldArm;
typedef struct {
    NLCheckedNodeId conditional, condition;
    const NLSemanticContext *entry, *post;
    NLPlaceId root, child, sibling;
    NLIncarnationId incarnation, child_incarnation;
    NLTypeId nominal;
    NLSymbolId base;
    size_t index;
    NLCheckedResult result;
    NLIfFieldArm arms[2];
} NLIfFieldView;
typedef struct {
    NLCheckedNodeId node;
    bool change;
    const NLSemanticContext *before, *after;
    NLCheckedField field;
    NLValueId result, incoming;
} NLIfFieldEvent;
bool nl_checked_if_field_view(const NLCheckedFragment *, NLCheckedNodeId,
                              NLIfFieldView *);
bool nl_checked_if_field_event(const NLCheckedFragment *, NLCheckedNodeId,
                               size_t arm, size_t index, NLIfFieldEvent *);
NLCheckStatus nl_checked_if_field_validate(const NLCheckedFragment *,
                                           NLCheckedNodeId,
                                           const NLIfFieldView *candidate);
NLCheckStatus
nl_checked_if_field_event_validate(const NLCheckedFragment *, NLCheckedNodeId,
                                   size_t arm, size_t index,
                                   const NLIfFieldEvent *candidate);
/* One NORMAL edge and one distinct function RETURN. The field view is the
 * normal continuation only; returned IDs belong to return_live/return_ended,
 * never to that continuation. All pointers borrow the actual checked owner. */
typedef struct {
    NLIfFieldView field;
    size_t normal_arm;
    NLCheckedNodeId replay_root, return_node;
    const NLSemanticContext *return_live, *return_ended;
    NLCheckedResult returned;
    size_t replay_events, replay_changes;
} NLIfReturnView;
bool nl_checked_if_return_view(const NLCheckedFragment *, NLCheckedNodeId,
                               NLIfReturnView *);
NLCheckStatus nl_checked_if_return_validate(const NLCheckedFragment *,
                                            NLCheckedNodeId,
                                            const NLIfReturnView *);
/* stream 0/1 denotes the original fork; stream 2 is the genuine re-execution
 * of the sole normal source arm in the common world. */
bool nl_checked_if_return_event(const NLCheckedFragment *, NLCheckedNodeId,
                                size_t stream, size_t index, NLIfFieldEvent *);
NLCheckStatus nl_checked_if_return_event_validate(const NLCheckedFragment *,
                                                  NLCheckedNodeId,
                                                  size_t stream, size_t index,
                                                  const NLIfFieldEvent *);
#endif
