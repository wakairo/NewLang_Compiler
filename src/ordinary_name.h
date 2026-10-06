#ifndef NEWLANG_ORDINARY_NAME_H
#define NEWLANG_ORDINARY_NAME_H

#include <stddef.h>
#include <string.h>

/* Borrowed bytes; pure ordinary-namespace admission, not lexer tokenization
 * or member-label policy. Draft 17.15 fixes this exact, case-sensitive set. */
typedef enum {
    NL_NAME_ADMISSIBLE,
    NL_NAME_CORE_UNIT,
    NL_NAME_RESERVED_STRUCTURAL
} NLOrdinaryNameClass;

static inline NLOrdinaryNameClass nl_ordinary_name_class(const void *bytes,
                                                         size_t length)
{
    if (length == 4 && memcmp(bytes, "unit", 4) == 0)
        return NL_NAME_CORE_UNIT;
    if ((length == 2 && memcmp(bytes, "fn", 2) == 0) ||
        (length == 3 && memcmp(bytes, "let", 3) == 0) ||
        (length == 6 && memcmp(bytes, "return", 6) == 0) ||
        (length == 5 && memcmp(bytes, "match", 5) == 0) ||
        (length == 2 && memcmp(bytes, "if", 2) == 0) ||
        (length == 4 && memcmp(bytes, "else", 4) == 0))
        return NL_NAME_RESERVED_STRUCTURAL;
    return NL_NAME_ADMISSIBLE;
}

/* Static borrowed diagnostic strings for an inadmissible classification. */
static inline const char *nl_ordinary_name_code(NLOrdinaryNameClass kind)
{
    return kind == NL_NAME_CORE_UNIT ? "P10-RESERVED-NAME"
                                     : "P12-RESERVED-STRUCTURAL-NAME";
}
static inline const char *nl_ordinary_name_message(NLOrdinaryNameClass kind)
{
    return kind == NL_NAME_CORE_UNIT ? "unit is a core spelling and cannot be "
                                       "an ordinary lexical name"
                                     : "structural spelling is reserved from "
                                       "the ordinary lexical namespace";
}

#endif
