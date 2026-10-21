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
#endif
