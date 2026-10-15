#ifndef NEWLANG_ORDINARY_CALL_H
#define NEWLANG_ORDINARY_CALL_H

#include "newlang/checked.h"

/* EXPERIMENTAL diagnostic proof, not a NewLang API/ABI or source permission.
 * Issuance is default-OFF. All pointers borrow the actual owning artifact;
 * node IDs are qualified by caller/body. Semantic IDs use the indicated
 * immutable source-point worlds, never the final lexical teardown context.
 * Finite profile: two original lexical two-field Nodes, ordinary write ref,
 * exact one-replace body, old None Copy / incoming Some(original locator),
 * no opaque/exact value dependencies. Real ref-scope dependencies are retained.
 * Historical validation grants no current liveness or backend permission. */
typedef struct {
    const NLCheckedFragment *caller, *body;
    NLCheckedNodeId call, loan, arguments[2], operation;
    const NLSemanticContext *entry, *before_change, *after_change, *post,
        *closed_loan;
    NLSymbolId parameters[2];
    NLPlaceId originals[2], root, field;
    NLReferenceFacts reference;
    NLValueId returned;
} NLOrdinaryCallView;

/* Observation only; absent/unsupported leaves out untouched. A view is not
 * authority. No general call, branch, heap or ownership certificate. */
bool nl_checked_ordinary_call_view(const NLCheckedFragment *caller,
                                   NLCheckedNodeId call,
                                   NLOrdinaryCallView *out);
/* Independent finite graph/core replay, read-only even on OOM/invalid input.
 * candidate NULL validates issued evidence. Foreign world/body pointers are
 * compared to owned anchors before dereference. No new authority is issued. */
NLCheckStatus
nl_checked_ordinary_call_validate(const NLCheckedFragment *caller,
                                  NLCheckedNodeId call,
                                  const NLOrdinaryCallView *candidate);

#endif
