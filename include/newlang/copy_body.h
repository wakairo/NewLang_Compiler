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

/* Explicit ordinary parameter substitution for the finite write-u8 call.
 * The entry/handoff worlds are historical observations, never live authority.
 * Actual source arguments and fresh formal symbols belong to this exact call.
 */
typedef struct {
    const NLSemanticContext *entry, *handoff;
    NLCheckedNodeId loan, arguments[2];
    NLSymbolId parameters[2];
    NLCheckedField field;
} NLCopyBodyParameters;
bool nl_checked_copy_body_parameters(const NLCheckedFragment *caller,
                                     NLCheckedNodeId call,
                                     NLCopyBodyParameters *out);
NLCheckStatus
nl_checked_copy_body_parameters_validate(const NLCheckedFragment *caller,
                                         NLCheckedNodeId call,
                                         const NLCopyBodyParameters *candidate);

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
/* Unit-returning direct call with one normal IF edge and one callee RETURN,
 * or two normal unit arms each containing one source-issued field Change.
 * The postcall world is a conservative structural join, never either fork.
 * normal_end is the cleanup-normalized normal end (common end for two normal
 * arms). Both original arms and actual/formal cleanup are replay-validated.
 * This receipt borrows immutable history and grants no present authority. */
typedef struct {
    NLCopyBodyParameters parameters;
    const NLCheckedFragment *body;
    NLCheckedNodeId conditional;
    const NLSemanticContext *normal_end, *postcall;
    size_t function;
} NLCopyUnitView;
bool nl_checked_copy_unit_view(const NLCheckedFragment *, NLCheckedNodeId,
                               NLCopyUnitView *);
NLCheckStatus nl_checked_copy_unit_validate(const NLCheckedFragment *,
                                            NLCheckedNodeId,
                                            const NLCopyUnitView *);
#endif
