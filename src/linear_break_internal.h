#ifndef NEWLANG_LINEAR_BREAK_INTERNAL_H
#define NEWLANG_LINEAR_BREAK_INTERNAL_H

#include "semantic_internal.h"

/* Compiler-private borrow, filled only after independent validation of an
 * actual CHECK-owned entry/effect/result record. No issuer or public API. */
typedef struct {
    NLSymbolId base;
    NLTypeId nominal;
    NLPlaceId root, child;
    NLIncarnationId incarnation, child_incarnation;
    size_t index, incoming;
    NLCheckedResult result;
    const NLCheckedFragment *caller; /* actual owned helper caller, or NULL */
    NLCheckedNodeId call;
    /* Populated only by the independently checked private cyclic relation. */
    bool stuttering, less, continue_on_true;
    bool computed_carry, subtract_carry;
    bool scalar_loop; /* free usize normal result, no field authority */
    bool changing_current;
    const NLCheckedFragment *iteration;
    /* Borrowed only after complete current/exit validation. The actual Break
     * operand is distinct from the runtime phase and the caller summary. */
    const NLCheckedFragment *breaking;
    NLCheckedNodeId break_value;
    NLCheckedNodeId continue_node;
    NLCheckedNodeId initial_node; /* caller-owned authenticated Copy source */
    NLCheckedNodeId incoming_if; /* authenticated iteration-local scalar join */
    NLCheckedNodeId incoming_call; /* authenticated iteration-local pure call */
    NLSymbolId phase_symbol;
    /* Tagged payload: literal successor normally; arithmetic step when
     * computed_carry is true. A computed successor is never known here. */
    size_t initial_phase, next_phase, limit;
    /* Borrowed only after the complete current-cycle validator succeeds.
     * These are the already sealed CHECK entry and normal-Break summary;
     * no world, graph or authority is constructed for a backend consumer. */
    const NLSemanticContext *entry_world, *post_world;
    const NLCheckedFragment *helper_body;
    NLSymbolId helper_formal;
    NLCheckedNodeId helper_selection;
} NLLinearBreakView;
NLCheckStatus nl_linear_break_validate(const NLCheckedFragment *,
                                       NLCheckedNodeId, NLLinearBreakView *);
/* Authenticate retained relation before the driver walks mutable views. */
NLCheckStatus nl_linear_break_owner_validate(const NLCheckedFragment *);
#endif
