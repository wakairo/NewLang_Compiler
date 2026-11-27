#ifndef NEWLANG_NUMERIC_CONVERSION_INTERNAL_H
#define NEWLANG_NUMERIC_CONVERSION_INTERNAL_H

#include "newlang/semantic.h"

#include <stdint.h>

static inline uintmax_t nl_conversion_limit(NLSemanticTypeKind kind)
{
    switch (kind) {
    case NL_TYPE_BYTE:
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

static inline bool nl_scalar_conversion_pair(NLSemanticTypeKind source,
                                             NLSemanticTypeKind target)
{
    if (source == NL_TYPE_BYTE || target == NL_TYPE_BYTE)
        return (source == NL_TYPE_BYTE && target == NL_TYPE_U8) ||
               (source == NL_TYPE_U8 && target == NL_TYPE_BYTE);
    return source != target && nl_conversion_limit(source) != 0 &&
           nl_conversion_limit(target) != 0;
}

static inline bool nl_numeric_runtime_widening(NLSemanticTypeKind source,
                                               NLSemanticTypeKind target)
{
    return source == NL_TYPE_U8 && target == NL_TYPE_USIZE &&
           nl_conversion_limit(source) <= nl_conversion_limit(target);
}

/* A shared precondition for the selected explicit conversions. The checked
 * scalar carrier is size_t; callers require an established known value. */
static inline bool nl_scalar_conversion_fits(NLSemanticTypeKind source,
                                             NLSemanticTypeKind target,
                                             size_t value)
{
    return nl_scalar_conversion_pair(source, target) &&
           (uintmax_t)value <= nl_conversion_limit(source) &&
           (uintmax_t)value <= nl_conversion_limit(target);
}

#endif
