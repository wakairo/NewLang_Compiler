#ifndef NEWLANG_DIVISION_USIZE_INTERNAL_H
#define NEWLANG_DIVISION_USIZE_INTERNAL_H
#include "semantic_internal.h"

/* Existing per-parent body-call bound, shared by CHECK and C17. */
enum {
    NL_CHECKED_MAX_BODY_CALLS = 64,
    NL_QUANTITY_MAX_ARGUMENTS = 2
};

/* Borrowed finite plan, using the existing complete function-result seal. */
typedef struct {
    const NLCheckedFragment *body;
    size_t function;
    NLSymbolId formal, second_formal;
    NLCheckedNodeId division, result,
        call; /* division identifies the specialized single operation */
    size_t divisor;
} NLDivisionUsizePlan;
bool nl_usize_division_operands(const NLCheckedFragment *,
                                const NLSemanticContext *, const NLSyntaxNode *,
                                NLCheckedNodeId, NLCheckedNodeId);
bool nl_usize_remainder_operands(const NLCheckedFragment *,
                                 const NLSemanticContext *,
                                 const NLSyntaxNode *, NLCheckedNodeId,
                                 NLCheckedNodeId);
bool nl_division_usize_call_valid(const NLCheckedFragment *, NLCheckedNodeId,
                                  NLDivisionUsizePlan *);
bool nl_remainder_usize_call_valid(const NLCheckedFragment *, NLCheckedNodeId,
                                   NLDivisionUsizePlan *);
const NLFunctionEntry *nl_quantity_usize_function(const NLSemanticContext *,
                                                  size_t function,
                                                  size_t parameters);
bool nl_quantity_usize_body_at(const NLCheckedFragment *,
                               const NLSemanticContext *, size_t, bool entry,
                               NLDivisionUsizePlan *);
bool nl_quantity_usize_call_valid(const NLCheckedFragment *, NLCheckedNodeId,
                                  NLDivisionUsizePlan *);
/* Staged text: every failure leaves both caller outputs untouched. */
NLCheckStatus nl_division_usize_c17(const NLCheckedFragment *, char **,
                                    size_t *);
NLCheckStatus nl_remainder_usize_c17(const NLCheckedFragment *, char **,
                                     size_t *);
NLCheckStatus nl_quantity_usize_c17(const NLCheckedFragment *, char **,
                                    size_t *);
#endif
