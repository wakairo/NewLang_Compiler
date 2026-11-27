#ifndef NEWLANG_ADDRESS_DATA_INTERNAL_H
#define NEWLANG_ADDRESS_DATA_INTERNAL_H

#include "newlang/checked.h"

#include <stdint.h>

static inline bool nl_coordinate_operation(NLCheckedKind kind)
{
    return kind == NL_CHECKED_UINTPTR_REMAINDER ||
           kind == NL_CHECKED_UINTPTR_ADDITION ||
           kind == NL_CHECKED_UINTPTR_SUBTRACTION;
}

/* Use the same finite math in CHECK and in the C17 gate. The arguments are
 * ordinary scalar facts from one checked body. No memory identity is derived.
 */
static inline NLCheckStatus nl_coordinate_arithmetic(NLCheckedKind kind,
                                                     NLScalarValue left,
                                                     NLScalarValue right,
                                                     NLScalarValue *result)
{
    if (!right.known)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    if (kind == NL_CHECKED_UINTPTR_REMAINDER) {
        if (right.value == 0)
            return NL_CHECK_SEMANTIC_ERROR;
        result->known = left.known || right.value == 1;
        result->value = left.known ? left.value % right.value : 0;
        return NL_CHECK_OK;
    }
    if (!left.known)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const uintmax_t limit = UINTPTR_MAX < SIZE_MAX ? UINTPTR_MAX : SIZE_MAX;
    if (left.value > limit)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    if (kind == NL_CHECKED_UINTPTR_ADDITION) {
        if (right.value > limit - left.value)
            return NL_CHECK_SEMANTIC_ERROR;
        result->value = left.value + right.value;
    } else if (kind == NL_CHECKED_UINTPTR_SUBTRACTION) {
        if (left.value < right.value)
            return NL_CHECK_SEMANTIC_ERROR;
        result->value = left.value - right.value;
    } else {
        return NL_CHECK_INTERNAL_ERROR;
    }
    result->known = true;
    return NL_CHECK_OK;
}

#endif
