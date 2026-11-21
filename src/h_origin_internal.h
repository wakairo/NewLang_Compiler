#ifndef NEWLANG_H_ORIGIN_INTERNAL_H
#define NEWLANG_H_ORIGIN_INTERNAL_H
#include "semantic_internal.h"

/* Memory custody only: zero H sites confer no H-origin authority. Every real
 * CHECK fragment joins before it can publish children. The root owns metadata;
 * child attachments borrow it. Pending edges do not own their endpoints. */
typedef enum {
    NL_CUSTODY_ARM,
    NL_CUSTODY_BODY,
    NL_CUSTODY_CONTINUATION
} NLCustodyRole;
NLCheckStatus nl_h_custody_attach(NLCheckedFragment *parent,
                                  NLCheckedFragment *child);
NLCheckStatus nl_h_custody_reserve(NLCheckedFragment *parent,
                                   NLCheckedFragment *child, NLCustodyRole role,
                                   NLCheckedNodeId site, size_t slot);
void nl_h_custody_commit(NLCheckedFragment *parent, NLCheckedFragment *child,
                         NLCustodyRole role, size_t slot);
/* CHECK-only result lineage. Seal records the actual call plus its complete
 * owned callee subtree before publication. Validation authenticates those
 * issued bytes; it never mints a value, owner, or source permission. A ptr IF
 * or normal Bool control supplies its genuine commit-pending candidate to
 * freeze the same receipt's semantic storage; NULL retains the existing
 * node-only seal. */
NLCheckStatus nl_h_custody_seal_result(NLCheckedFragment *parent,
                                       NLCheckedFragment *child,
                                       NLCheckedNodeId call,
                                       const NLSemanticContext *result_world);
bool nl_h_custody_result_valid(const NLCheckedFragment *parent,
                               NLCheckedNodeId call);
/* Coverage observation of this same canonical receipt, not validation or a
 * truth/owner grant. Consumers needing world bytes require this AND the
 * existing result validator; a legacy node-only receipt is insufficient. */
bool nl_h_custody_result_world_present(const NLCheckedFragment *parent,
                                       NLCheckedNodeId call);
/* Historical observation only. The actual parent and recorded call select a
 * live COMMITTED BODY; current world/view integrity is not authenticated. */
const NLCheckedFragment *nl_h_custody_observe_body(const NLCheckedFragment *,
                                                   NLCheckedNodeId);
bool nl_h_custody_validate(const NLCheckedFragment *root);
/* Only the unpublished CHECK transaction may validate its commit candidate.
 * Published validation always follows the canonical recorded world. */
bool nl_h_custody_validate_candidate(const NLCheckedFragment *root,
                                     const NLSemanticContext *candidate);
/* Recognize the canonical BODY and its attached CHECK state before reserve.
 * The arithmetic caller still authenticates the registered source event. */
bool nl_h_custody_is_body(const NLCheckedFragment *);
/* Resolve the existing BODY edge. CHECK may inspect its pending reservation.
 * Published consumers read the original world through the existing seals. */
const NLCheckedFragment *
nl_h_custody_body_caller(const NLCheckedFragment *,
                         const NLSemanticContext *working,
                         NLCheckedNodeId *call);
/* Locate the owning registered BODY invocation of a canonical normal IF arm.
 * The CHECK planner supplies its exact parent while the graph is unpublished.
 * Callers verify the registered source, IF and exact syntax event separately.
 */
const NLCheckedFragment *
nl_h_custody_if_body(const NLCheckedFragment *arm,
                     const NLCheckedFragment *if_parent, size_t *function);
/* Borrowed structural views only; never an owner/source-authority receipt. */
bool nl_h_custody_borrow_valid(const NLCheckedFragment *fragment);
bool nl_h_origin_has_sites(const NLCheckedFragment *root);
/* Returns false only for artifacts outside the CHECK-owned protocol. */
bool nl_h_custody_destroy_children(NLCheckedFragment *, bool continuations);

/* CHECK-only construction; no public authority or externally supplied receipt.
 * The incomplete type is deliberately not defined in the shared header. */
NLCheckStatus nl_h_origin_site(NLCheckedFragment *root,
                               NLCheckedFragment *invocation,
                               NLCheckedNodeId call, NLCheckedFragment *parent,
                               NLCheckedNodeId match, NLCheckedNodeId trial,
                               const NLSemanticContext *cut);
void nl_h_origin_fork(NLCheckedFragment *parent, NLCheckedNodeId match,
                      size_t ordinal, NLCheckedFragment *child);
void nl_h_origin_arm(NLCheckedFragment *parent, NLCheckedNodeId match,
                     size_t ordinal, NLCheckedFragment *child);
void nl_h_origin_take_arm(NLCheckedFragment *parent, NLCheckedNodeId match,
                          size_t ordinal, size_t slot);
void nl_h_origin_finish_site(NLCheckedFragment *, NLCheckedNodeId);
void nl_h_origin_take_body(NLCheckedFragment *, NLCheckedFragment *,
                           NLCheckedNodeId, size_t);
void nl_h_origin_close(NLCheckedFragment *);
bool nl_h_origin_validate(const NLCheckedFragment *);
bool nl_h_origin_edge(const NLCheckedFragment *root,
                      const NLCheckedFragment *parent, NLCheckedNodeId match,
                      size_t ordinal, const NLCheckedFragment *child);
/* Canonical custody is independent of exchangeable graph-view entries. */
void nl_h_origin_release_fragment(NLCheckedFragment *);
#endif
