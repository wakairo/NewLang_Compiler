#ifndef NEWLANG_NUMERIC_CONVERSION_INTERNAL_H
#define NEWLANG_NUMERIC_CONVERSION_INTERNAL_H

#include "newlang/semantic.h"

#include <stdint.h>

static inline uintmax_t nl_numeric_limit(NLSemanticTypeKind kind)
{
    switch (kind) {
    case NL_TYPE_U8:
        return UINT8_MAX;
    case NL_TYPE_USIZE:
        return SIZE_MAX;
    case NL_TYPE_UINTPTR:
        return UINTPTR_MAX;
    default:
        return 0;
    }
}

/* A shared precondition for the selected explicit conversions. The checked
 * scalar carrier is size_t; callers require an established known value. */
static inline bool nl_numeric_conversion_fits(NLSemanticTypeKind source,
                                              NLSemanticTypeKind target,
                                              size_t value)
{
    const uintmax_t from = nl_numeric_limit(source);
    const uintmax_t to = nl_numeric_limit(target);
    return source != target && from != 0 && to != 0 &&
           (uintmax_t)value <= from && (uintmax_t)value <= to;
}

#endif
