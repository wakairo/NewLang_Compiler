#ifndef P350_SHADOW_H
#define P350_SHADOW_H
#ifndef NEWLANG_TEST_CHRONOLOGY_SHADOW
#error "P350 is restricted to the separately linked dedicated test binary"
#endif
#include "semantic_internal.h"

typedef struct {
    const char *reason;
    size_t eligible, replayed, originals, scalar_checks, gaps_checked;
    NLCheckedNodeId node;
} P350Report;
NLCheckStatus nl_test_p350_primitive(const NLCheckedFragment *,
                                     const NLCapturedChange *);
NLCheckStatus p350_shadow(const NLCheckedFragment *,
                          const NLCapturedChange *const *, size_t,
                          P350Report *);
#endif
