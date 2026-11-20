#ifndef NEWLANG_USIZE_DIVISION_H
#define NEWLANG_USIZE_DIVISION_H
#include "newlang/checked.h"

/* Borrowed source-issued quotient relation, never memory/resource authority.
 * Unknown numerator and quotient fields are canonical zero placeholders, NOT
 * Known zero. The divisor is always Known and target-representable/nonzero.
 * Historical binding facts describe operand evaluation, not live authority.
 * Observation alone does not authenticate the event or its caller. */
typedef struct {
    const NLCheckedFragment *origin;
    NLCheckedNodeId node, operands[2];
    NLTypeId type;
    NLValueId values[2], result;
    NLSymbolId symbols[2];
    NLPlaceId places[2];
    NLIncarnationId incarnations[2];
    NLValueFactId facts[2];
    NLSourceSpan span, operator_span, operand_spans[2];
    size_t numbers[2], quotient, target_max;
    bool numerator_known, quotient_known;
} NLUsizeDivisionView;
/* Failure leaves out untouched. Requires genuine retained registered source. */
bool nl_checked_usize_division_view(const NLCheckedFragment *, NLCheckedNodeId,
                                    NLUsizeDivisionView *out);
/* Allocation-free recheck of source, issuer, packages, exact optional facts,
 * and q=floor(n/k), 0<=q<=n<=target_max for every admitted numerator.
 * NULL validates the actual event. Copies are accepted for independent audit.
 * Complete call/result/world custody is additionally required by consumers;
 * an authentic isolated arithmetic event does not certify a whole program. */
bool nl_checked_usize_division_valid(const NLCheckedFragment *, NLCheckedNodeId,
                                     const NLUsizeDivisionView *candidate);
#endif
