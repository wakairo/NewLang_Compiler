#ifndef NEWLANG_PARSER_H
#define NEWLANG_PARSER_H

#include "newlang/diagnostic.h"
#include "newlang/syntax.h"

typedef struct NLParser NLParser;

typedef enum {
    NL_PARSE_OK,
    NL_PARSE_LEXICALLY_UNSUPPORTED,
    NL_PARSE_SYNTAX_UNSUPPORTED,
    NL_PARSE_SYNTAX_ERROR,
    NL_PARSE_OUT_OF_MEMORY,
    NL_PARSE_RESOURCE_LIMIT,
    NL_PARSE_INTERNAL_ERROR
} NLParseStatus;

/* Implementation budgets, not NewLang grammar/language limits. */
#define NL_PARSER_MAX_DEPTH 128
#define NL_PARSER_MAX_NODES 4096

typedef struct {
    NLDiagnostic diagnostic; /* Static borrowed strings; range/notes NULL. */
    NLSourceSpan span;       /* Canonical location in the parser's source. */
} NLParseDiagnostic;

/* Owned parser state (streaming lexer + one lookahead), borrowing source.
 * source must remain live until parser destruction and all returned tree uses.
 * create requires a NULL owner slot; failure leaves it unchanged. NULL/invalid
 * API inputs report INTERNAL_ERROR; malloc failure reports OUT_OF_MEMORY.
 * Destroy releases only parser state, never source or returned syntax trees.
 * Synchronous/non-reentrant; independent parser instances do not interfere. */
NLParseStatus nl_parser_create(const NLSource *source, NLParser **out_parser);
void nl_parser_destroy(NLParser *parser);

/* Each call resets scanning to source start, parses one production plus EOF.
 * out_tree must point to a NULL owner slot, borrowed for the call. Success
 * returns an immutable owned tree; diagnostic output is untouched. Failure
 * destroys partial syntax, leaves out_tree unchanged, and writes the first
 * structured diagnostic if non-NULL. Invalid API arguments leave all outputs
 * and parser state untouched. Parser remains reusable after any result.
 * No source mutation/ownership transfer, semantic checking or process abort.
 * Scope/error classification: docs/P2_MINIMAL_SYNTAX_CONTRACT.md. */
NLParseStatus nl_parser_parse_type_fragment(NLParser *parser,
                                            NLSyntaxTree **out_tree,
                                            NLParseDiagnostic *out_diagnostic);
NLParseStatus
nl_parser_parse_expression_fragment(NLParser *parser, NLSyntaxTree **out_tree,
                                    NLParseDiagnostic *out_diagnostic);
NLParseStatus
nl_parser_parse_binding_fragment(NLParser *parser, NLSyntaxTree **out_tree,
                                 NLParseDiagnostic *out_diagnostic);
NLParseStatus nl_parser_parse_loan_fragment(NLParser *parser,
                                            NLSyntaxTree **out_tree,
                                            NLParseDiagnostic *out_diagnostic);

/* Draft 17.10 closed source profile: P5 bindings/blocks/aggregates plus
 * qualified sum constructors and match with the three closed variant patterns.
 * P2 entries retain their original subset. No sum/aggregate/callable
 * declarations, general patterns, operators or opaque loan body parsing. Same
 * ownership, reset, resource and failure contracts as fragment APIs. */
NLParseStatus nl_parser_parse_source_fragment(NLParser *, NLSyntaxTree **,
                                              NLParseDiagnostic *);

/* Draft 17.13 bounded fn-only top-level container, one or more declarations.
 * Same reset/ownership/budget contracts as fragments. Bodies/types reuse their
 * existing grammar; no general top-level item language or generic declarations.
 * Multiple trees may be collected as one semantic unit by the semantic API. */
NLParseStatus nl_parser_parse_function_unit(NLParser *, NLSyntaxTree **,
                                            NLParseDiagnostic *);

/* Synchronous adapter to existing renderer: byte columns/LF line presentation
 * only, CRLF one physical newline, tabs/lone CR one byte column. The diagnostic
 * and source are borrowed for the call; false on invalid span/output failure.
 * Source must be the source whose bytes the diagnostic span addresses. */
bool nl_parse_diagnostic_render(FILE *stream, const NLSource *source,
                                const NLParseDiagnostic *diagnostic);

#endif
