#ifndef NEWLANG_COPY_BODY_H
#define NEWLANG_COPY_BODY_H
#include "newlang/checked.h"

/* Historical source-issued observations, not live capabilities. All pointers
 * borrow the actual registered call/body owner. No host-issued world is valid.
 */
typedef enum {
    NL_COPY_BODY_INITIALIZE,
    NL_COPY_BODY_READ,
    NL_COPY_BODY_CHANGE
} NLCopyBodyEventKind;
typedef struct {
    NLCopyBodyEventKind kind;
    NLCheckedNodeId node;
    const NLSemanticContext *before, *after;
    NLCheckedField field;
    NLSymbolId binding;
    NLValueId result, incoming;
} NLCopyBodyEventView;
typedef struct {
    const NLCheckedFragment *body;
    size_t function, event_count;
    NLCheckedNodeId root;
    NLCheckedResult result;
    const NLSemanticContext *live_exit;
} NLCopyBodyView;

/* Observation is not validation. Outputs are untouched on failure. */
bool nl_checked_copy_body_view(const NLCheckedFragment *caller,
                               NLCheckedNodeId call, NLCopyBodyView *out);
bool nl_checked_copy_body_event(const NLCheckedFragment *caller,
                                NLCheckedNodeId call, size_t index,
                                NLCopyBodyEventView *out);
/* Recheck exact source/call ownership, complete straight-line graph and ordered
 * live checkpoints, replaying Change on private clones. No public mutation;
 * explicit OOM/resource failure. Missing/out-of-profile evidence fails closed.
 * A candidate is a copied view, never a supplied authority or dereferenced
 * foreign world. NULL means validate the actual source-issued observation. */
NLCheckStatus nl_checked_copy_body_validate(const NLCheckedFragment *caller,
                                            NLCheckedNodeId call,
                                            const NLCopyBodyView *candidate);
NLCheckStatus
nl_checked_copy_body_event_validate(const NLCheckedFragment *caller,
                                    NLCheckedNodeId call, size_t index,
                                    const NLCopyBodyEventView *candidate);
#endif
