#ifndef NEWLANG_UNCHECKED_REF_INTERNAL_H
#define NEWLANG_UNCHECKED_REF_INTERNAL_H
#include "semantic_internal.h"

/* One finite value-only IF and one lexical read operation per fragment.
 * Neither the recipe nor its conditional anchor is a reference/owner proof.
 * Canonical custody owns this metadata; its child/source pointers borrow. */
#define NL_UNCHECKED_COPY_LIMIT 16
typedef struct {
    NLValueId from, to;
    NLCheckedNodeId node;
    NLSymbolId binding;
} NLUncheckedCopy;
typedef struct {
    bool has_pointer, has_read;
    NLTypeId pointer_type;
    NLCheckedNodeId pointer_if, condition, tails[2];
    NLValueId pointer_result, prefix_values[2];
    const NLCheckedFragment *arms[2];
    NLUncheckedCopy copies[NL_UNCHECKED_COPY_LIMIT];
    size_t copy_count;
    NLCheckedLocalAssumptionView read;
} NLUncheckedProof;

/* CHECK-only staged allocation. No record can be appended after sealing. */
NLCheckStatus nl_h_unchecked_prepare(NLCheckedFragment *);
NLUncheckedProof *nl_h_unchecked_build(NLCheckedFragment *);
const NLUncheckedProof *nl_h_unchecked_data(const NLCheckedFragment *);
bool nl_h_unchecked_machine_only(const NLCheckedFragment *);
bool nl_h_unchecked_present(const NLCheckedFragment *);
/* A finite temporal guard, not a universal one-Domain/one-root law. */
NLCheckStatus nl_unchecked_domain_target(const NLSemanticContext *, NLTypeId,
                                         NLDomainId, NLPlaceId *);
#endif
