#ifndef NEWLANG_CAPTURED_CLOSURE_H
#define NEWLANG_CAPTURED_CLOSURE_H

#include "newlang/checked.h"

#define NL_CAPTURED_MAX_ORIGINALS 4
#define NL_CAPTURED_MAX_RELEASES 5

/* An original is qualified by this certificate's owned ancestor world.
 * These IDs are not addresses, new authorities, or branch-local suffix IDs.
 * Views borrow the immutable artifact; access/validation never transfers.
 */
typedef struct {
    const NLSemanticContext *origin;
    NLBackingRange extent;
    NLPlaceId root;
    NLIncarnationId incarnation;
    NLDomainId domain;
    NLSymbolId allocation_binding, domain_binding;
    NLValueId allocation_value, domain_value;
    NLAvailability allocation_availability, domain_availability;
} NLCapturedOriginal;

typedef struct {
    const NLSemanticContext *ancestor, *closed_post;
    size_t count;
    NLCapturedOriginal originals[NL_CAPTURED_MAX_ORIGINALS];
    /* Number of independently checked terminal worlds with 0..5 releases.
     * This is semantic proof evidence, not a runtime allocation counter. */
    size_t release_worlds[NL_CAPTURED_MAX_RELEASES + 1];
} NLCapturedClosureView;

/* Actual field Change snapshots, borrowed from this immutable arm artifact.
 * after the last change is the live pre-teardown topology checkpoint.
 * Static opaque projection identity is (nominal,index); child is its current
 * incarnation-specific place, never an offset or another backing root. */
typedef struct {
    NLCheckedNodeId operation;
    const NLSemanticContext *world, *before, *after;
} NLCapturedChangeView;
size_t nl_checked_captured_change_count(const NLCheckedFragment *);
bool nl_checked_captured_change_view(const NLCheckedFragment *, size_t,
                                     NLCapturedChangeView *);

bool nl_checked_captured_closure_view(const NLCheckedFragment *,
                                      NLCheckedNodeId, NLCapturedClosureView *);
/* Read-only rederivation and both-arm evidence validation. OOM is explicit;
 * invalid/Unknown proof returns ANALYSIS_PRECISION_LIMIT, never a grant.
 * No AST/source reparse or caller state mutation. Link semantic library. */
NLCheckStatus nl_checked_captured_closure_validate(const NLCheckedFragment *,
                                                   NLCheckedNodeId);

#endif
