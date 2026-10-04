#ifndef NEWLANG_TOKEN_H
#define NEWLANG_TOKEN_H

#include "newlang/source.h"

/* P1 atom kinds, not a final NewLang token/keyword/operator grammar. */
typedef enum {
    NL_TOKEN_EOF,
    NL_TOKEN_WORD,
    NL_TOKEN_DIGITS,
    NL_TOKEN_PUNCTUATION,
    NL_TOKEN_UNSUPPORTED
} NLTokenKind;

/* Small non-owning value. Interpret span with the source used by its lexer.
 * No copied text, decoded numeric value or pointer into transient storage.
 * Values survive lexer reinitialization; source views require a live source. */
typedef struct {
    NLTokenKind kind;
    NLSourceSpan span;
} NLToken;

#endif
