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
bool nl_h_custody_validate(const NLCheckedFragment *root);
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
