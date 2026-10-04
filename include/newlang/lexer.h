#ifndef NEWLANG_LEXER_H
#define NEWLANG_LEXER_H

#include "newlang/token.h"

/* Allocation-free streaming P1 atom scanner. Borrows an immutable source;
 * source must outlive all operations. Owns cursor only (0 <= cursor <= length).
 * Storage is public for stack allocation, fields module-private by contract:
 * initialize before next and operate only through this API; never pass
 * uninitialized storage to next. No resource-owning fields/deinit.
 * Tests observe tokens and views, never these fields. */
typedef struct {
    const NLSource *source;
    size_t next_byte;
} NLLexer;

typedef enum {
    NL_LEX_TOKEN,
    NL_LEX_EOF,
    NL_LEX_UNSUPPORTED,
    NL_LEX_INTERNAL_ERROR
} NLLexResult;

/* Initializes or resets a lexer; source is borrowed, never consumed/retained
 * beyond the lexer lifetime. NULL source returns false and leaves a non-NULL
 * lexer inert (next reports INTERNAL_ERROR). No allocation/partial ownership.
 * Caller may abandon the lexer at any time; it owns no resources. */
bool nl_lexer_init(NLLexer *lexer, const NLSource *source);

/* TOKEN advances and writes kind/span; EOF writes EOF [length,length) and is
 * repeatable. UNSUPPORTED writes a stable nonempty unsupported span, commits
 * preceding supported spacing only, and does not consume offending bytes.
 * Repeated calls return the same unsupported result; init resets scanning.
 * INTERNAL_ERROR (NULL argument, inert/invalid module) leaves outputs/state
 * untouched and is not a source diagnostic. There are no allocation failures
 * during scanning. Locale/environment never participate in classification.
 * Supported subset/deferred grammar: docs/P1_LEXICAL_CONTRACT_AUDIT.md. */
NLLexResult nl_lexer_next(NLLexer *lexer, NLToken *out_token);

#endif
