#ifndef NEWLANG_USIZE_SUBTRACTION_H
#define NEWLANG_USIZE_SUBTRACTION_H
#include "newlang/checked.h"
/* Diagnostic, borrowed, SOURCE-ISSUED arithmetic receipt. Historical binding
 * facts describe operand evaluation, not currently live memory authority.
 * Original input source/AST may be destroyed; registered body owns its plan.
 * No ownership, references, domains, allocations or range assumptions granted.
 */
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
    size_t numbers[2], difference;
} NLUsizeSubtractionView;
/* Subtraction-only: the legacy addition view/validator reject these events.
 * Failure leaves out untouched. Observation alone is not validation. */
bool nl_checked_usize_subtraction_view(const NLCheckedFragment *,
                                       NLCheckedNodeId,
                                       NLUsizeSubtractionView *out);
/* Allocation-free independent recheck of the retained source, issued event,
 * actual checked operands/packages and guarded mathematical difference. NULL
 * candidate validates the actual event; a separate copied view is permitted for
 * auditing. Requires a registered source-function body, not a host-seeded
 * fragment. The immutable source/checker event issuer remains trusted; this is
 * not a cryptographic certificate or a whole-program control-flow validator.
 */
bool nl_checked_usize_subtraction_valid(
    const NLCheckedFragment *, NLCheckedNodeId,
    const NLUsizeSubtractionView *candidate);
#endif
