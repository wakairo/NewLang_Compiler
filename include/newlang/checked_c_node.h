#ifndef NEWLANG_CHECKED_C_NODE_H
#define NEWLANG_CHECKED_C_NODE_H
#include "newlang/checked.h"
typedef enum {
    NL_NODE_C_OK,
    NL_NODE_C_UNSUPPORTED,
    NL_NODE_C_OUT_OF_MEMORY,
    NL_NODE_C_RESOURCE_LIMIT
} NLNodeCStatus;
/* Borrow the completed entry artifact/context. Return caller-owned (free)
 * staged C into an initialized NULL slot on success ONLY; failure leaves both
 * output slots untouched. Bounded two-lexical-root or one allocated/one lexical
 * H topology, or the closed two-allocated-H nested profile; heap-root/field
 * access requires owned checked operands and projection evidence. The closed
 * §18.1a proven live-tail receiver may use one real direct C call preserving
 * the original heap/owner/domain carriers; stack params never mint heap roots.
 * The closed §18.1b two-item producer validates its owned entry/return
 * certificate, performs the checked projected write in a separate C function,
 * returns the original ptr/Allocation/Domain carriers by value, and permits
 * one whole destructure before the independently proven terminal receiver.
 * Other LiveTail body/result profiles remain explicitly unsupported.
 * Not a general recursive or allocator backend. */
NLNodeCStatus nl_checked_c_node(const NLCheckedFragment *, char **out,
                                size_t *length);
#endif
