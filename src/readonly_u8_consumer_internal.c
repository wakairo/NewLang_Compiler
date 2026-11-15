#include "readonly_u8_consumer_internal.h"
#include "pure_u8_internal.h"

/* Separate TU: the older H emitter has its own literal formatting helper. */
bool nl_readonly_u8_consumer(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    NLPureU8Call call;
    return nl_h_custody_result_valid(f, id) &&
           nl_readonly_u8_contents(f, id, &call);
}
