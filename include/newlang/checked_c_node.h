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
 * access requires owned checked operands and projection evidence. Not a general
 * recursive or allocator backend. */
NLNodeCStatus nl_checked_c_node(const NLCheckedFragment *, char **out,
                                size_t *length);
#endif
