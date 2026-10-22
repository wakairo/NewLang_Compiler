#ifndef NEWLANG_PRODUCER_LOCAL_INTERNAL_H
#define NEWLANG_PRODUCER_LOCAL_INTERNAL_H
#include "semantic_internal.h"
/* Observation seal of an actual completed source check, never an authority
 * registry or a substitute for original-owner/consumption validation. */
typedef struct NLProducerLocalProof {
    const NLCheckedFragment *caller, *body;
    const NLSemanticContext *context;
    NLCheckedNodeId call, root;
    size_t count;
    const NLCheckedNodeView *origin;
    NLCheckedNodeView *nodes;
    NLCheckedNodeView call_node;
    const NLSource *source;
    const NLFunctionBody *plan;
    const NLControlTarget *target;
    const NLControlState *exit_state;
    NLProducerLocalReturnView issued;
} NLProducerLocalProof;
NLCheckStatus nl_producer_local_seal(NLCheckedFragment *,
                                     const NLCheckedFragment *,
                                     NLCheckedNodeId);
#endif
