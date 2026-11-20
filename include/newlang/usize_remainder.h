#ifndef NEWLANG_USIZE_REMAINDER_H
#define NEWLANG_USIZE_REMAINDER_H
#include "newlang/checked.h"

/* Borrowed source-issued evidence for n % k. Operand 0 is a Copy name;
 * operand 1 is its Known, target-representable nonzero typed divisor.
 * At an Unknown numerator, both numbers[0] and remainder are placeholders
 * distinguished from Known zero by their flags. Binding facts describe
 * operand evaluation. The source and origin borrow the checked lifetime. */
typedef struct {
    const NLCheckedFragment *origin;
    const NLSource *source;
    NLCheckedNodeId node, operands[2];
    NLTypeId type;
    NLValueId values[2], result;
    NLSymbolId symbols[2];
    NLPlaceId places[2];
    NLIncarnationId incarnations[2];
    NLValueFactId facts[2];
    NLSourceSpan span, operator_span, operand_spans[2];
    size_t numbers[2], remainder, target_max;
    bool numerator_known, remainder_known;
} NLUsizeRemainderView;

/* Borrow the issuer's observation. Failure leaves out untouched. */
bool nl_checked_usize_remainder_view(const NLCheckedFragment *, NLCheckedNodeId,
                                     NLUsizeRemainderView *out);
/* Allocation-free recheck of a retained registered source event, including
 * exact value identities and
 * r = n % k with 0 <= r < k. NULL validates the actual issued event. A
 * consumer of a function result also needs the call/result/world seals. */
bool nl_checked_usize_remainder_valid(const NLCheckedFragment *,
                                      NLCheckedNodeId,
                                      const NLUsizeRemainderView *candidate);
#endif
