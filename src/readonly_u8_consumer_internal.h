#ifndef NEWLANG_READONLY_U8_CONSUMER_INTERNAL_H
#define NEWLANG_READONLY_U8_CONSUMER_INTERNAL_H
#include "newlang/checked.h"

/* Existing source-owned result seal and Copy-only body walk. No receipt,
 * issuer, semantic state or allocation is created by this private adapter. */
bool nl_readonly_u8_consumer(const NLCheckedFragment *, NLCheckedNodeId);

typedef struct {
    const NLCheckedFragment *body;
    NLSymbolId parameters[2];
} NLReadonlySelection;
bool nl_readonly_selection_consumer(const NLCheckedFragment *, NLCheckedNodeId,
                                    NLReadonlySelection *);

#endif
