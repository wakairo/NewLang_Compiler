#ifndef NEWLANG_NODE_LINKS_H
#define NEWLANG_NODE_LINKS_H
#include "newlang/captured_closure.h"

/* Finite historical evidence: three original lexical two-field Nodes and two
 * consecutive scoped next replacements. Borrowed worlds/IDs are observations,
 * never currently-live refs, Allocation/Domain or nonCopy owner grants.
 * Certification requires a real registered zero-argument unit function body,
 * obtained from an actual checked call after function-unit registration. */
typedef struct {
    const NLCheckedFragment *origin;
    NLPlaceId originals[3];
    NLCapturedChangeView changes[2];
} NLNodeLinksView;
/* Observation alone is not certification. Failure leaves out unchanged. */
bool nl_checked_node_links_view(const NLCheckedFragment *,
                                NLNodeLinksView *out);
/* Independent core Change/Reset replay on private clones, complete original
 * frame and consecutive-object-state checks. candidate may be NULL or a copied
 * view; its pointers are compared, not dereferenced. Does not mutate public
 * state/output; explicit OOM/resource failures remain failures. */
NLCheckStatus nl_checked_node_links_validate(const NLCheckedFragment *,
                                             const NLNodeLinksView *candidate);
#endif
