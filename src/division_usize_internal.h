#ifndef NEWLANG_DIVISION_USIZE_INTERNAL_H
#define NEWLANG_DIVISION_USIZE_INTERNAL_H
#include "semantic_internal.h"

/* Borrowed finite plan, using the existing complete function-result seal. */
typedef struct {
    const NLCheckedFragment *body;
    size_t function;
    NLSymbolId formal;
    NLCheckedNodeId division, result,
        call; /* division is the helper operation */
    size_t divisor;
} NLDivisionUsizePlan;
bool nl_usize_division_operands(const NLCheckedFragment *,
                                const NLSemanticContext *, const NLSyntaxNode *,
                                NLCheckedNodeId, NLCheckedNodeId);
bool nl_usize_remainder_operands(const NLCheckedFragment *,
                                 const NLSemanticContext *,
                                 const NLSyntaxNode *, NLCheckedNodeId,
                                 NLCheckedNodeId);
bool nl_division_usize_body_at(const NLCheckedFragment *,
                               const NLSemanticContext *, size_t,
                               NLDivisionUsizePlan *);
bool nl_division_usize_call_valid(const NLCheckedFragment *, NLCheckedNodeId,
                                  NLDivisionUsizePlan *);
bool nl_remainder_usize_body_at(const NLCheckedFragment *,
                                const NLSemanticContext *, size_t,
                                NLDivisionUsizePlan *);
bool nl_remainder_usize_call_valid(const NLCheckedFragment *, NLCheckedNodeId,
                                   NLDivisionUsizePlan *);
/* Staged text: every failure leaves both caller outputs untouched. */
NLCheckStatus nl_division_usize_c17(const NLCheckedFragment *, char **,
                                    size_t *);
NLCheckStatus nl_remainder_usize_c17(const NLCheckedFragment *, char **,
                                     size_t *);
#endif
