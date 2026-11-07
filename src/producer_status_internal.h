#ifndef NEWLANG_PRODUCER_STATUS_INTERNAL_H
#define NEWLANG_PRODUCER_STATUS_INTERNAL_H
#include "semantic_internal.h"
/* Same read-only predicates and scratch evaluations as the public bool
 * consumers. The emitter needs the original failure cause, never a second
 * evaluation or an inference of OOM from false. No evidence is issued. */
NLCheckStatus nl_producer_validate(const NLCheckedFragment *, NLCheckedNodeId,
                                   bool raw);
NLCheckStatus
nl_producer_local_return_validate(const NLCheckedFragment *, NLCheckedNodeId,
                                  const NLProducerLocalReturnView *);
#endif
