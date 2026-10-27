#ifndef NEWLANG_H_ORIGIN_INTERNAL_H
#define NEWLANG_H_ORIGIN_INTERNAL_H
#include "semantic_internal.h"

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
NLCheckedFragment *nl_h_origin_owned_child(NLCheckedFragment *, size_t, bool);
void nl_h_origin_release_fragment(NLCheckedFragment *);
#endif
