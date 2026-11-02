#ifndef NEWLANG_LINEAR_BREAK_INTERNAL_H
#define NEWLANG_LINEAR_BREAK_INTERNAL_H

#include "semantic_internal.h"

/* Compiler-private borrow, filled only after independent validation of an
 * actual CHECK-owned entry/effect/result record. No issuer or public API. */
typedef struct {
    NLSymbolId base;
    NLTypeId nominal;
    NLPlaceId root, child;
    NLIncarnationId incarnation, child_incarnation;
    size_t index, incoming;
    NLCheckedResult result;
} NLLinearBreakView;
NLCheckStatus nl_linear_break_validate(const NLCheckedFragment *,
                                       NLCheckedNodeId, NLLinearBreakView *);
#endif
