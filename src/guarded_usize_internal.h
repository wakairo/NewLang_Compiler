#ifndef NEWLANG_GUARDED_USIZE_INTERNAL_H
#define NEWLANG_GUARDED_USIZE_INTERNAL_H
#include "semantic_internal.h"

/* The existing arithmetic issuer's finite true-edge relation. No public
 * numeric receipt, range lattice, memory authority or additional issuer. */
typedef struct {
    const NLCheckedFragment *body;
    size_t function;
    NLSymbolId formal;
    NLCheckedNodeId conditional, addition, call;
    size_t limit, step;
} NLGuardedUsizePlan;
bool nl_guarded_usize_same_current(const NLSemanticContext *,
                                   const NLSemanticContext *, NLSymbolId);
/* The u8 consumer separately requires the actual empty true-tail arm. */
bool nl_guarded_u8_operand(const NLCheckedFragment *, const NLSemanticContext *,
                           NLSymbolId, NLCheckedNodeId);
bool nl_guarded_usize_permitted(const NLCheckedFragment *,
                                const NLSemanticContext *,
                                const NLSemanticContext *, size_t, NLSymbolId,
                                const NLSyntaxNode *, NLCheckedNodeId,
                                NLCheckedNodeId);
NLCheckStatus nl_guarded_usize_capture(NLCheckedFragment *, NLCheckedNodeId,
                                       const NLSyntaxNode *,
                                       const NLSemanticContext *,
                                       const NLSemanticContext *, size_t,
                                       NLSymbolId);
/* Prepublication CHECK uses its actual transaction; publication consumers
 * must additionally authenticate the existing complete call/result seal. */
bool nl_guarded_usize_body_at(const NLCheckedFragment *,
                              const NLSemanticContext *, size_t,
                              NLGuardedUsizePlan *);
bool nl_guarded_usize_call_valid(const NLCheckedFragment *, NLCheckedNodeId,
                                 NLGuardedUsizePlan *);
#endif
