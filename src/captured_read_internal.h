#ifndef NEWLANG_CAPTURED_READ_INTERNAL_H
#define NEWLANG_CAPTURED_READ_INTERNAL_H
#include "semantic_internal.h"

/* Private source-event storage shared only with focused ownership fault tests.
 * The captured closure remains the single authority validator. */
typedef struct {
    NLCapturedChange event;
    NLCheckedKind kind;
    const NLCheckedFragment *arms[2];
    const NLSemanticContext *arm_worlds[2];
} ReadEvent;
struct NLCapturedReads {
    ReadEvent events[2]; /* one read, then its exhaustive Copy match */
    size_t count;
};

#endif
