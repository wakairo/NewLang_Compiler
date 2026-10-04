#ifndef NEWLANG_DIAGNOSTIC_H
#define NEWLANG_DIAGNOSTIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    NL_DIAG_NOTE,
    NL_DIAG_WARNING,
    NL_DIAG_ERROR
} NLDiagnosticSeverity;

/* One-based coordinates; end is exclusive. No range is represented by NULL. */
typedef struct {
    const char *path;
    size_t start_line;
    size_t start_column;
    size_t end_line;
    size_t end_column;
} NLSourceRange;

typedef struct {
    const char *message;
    const NLSourceRange *range;
} NLDiagnosticNote;

typedef struct {
    NLDiagnosticSeverity severity;
    const char *category; /* Optional. */
    const char *code;     /* Optional; P0 codes are implementation codes. */
    const char *message;
    const NLSourceRange *range;
    const NLDiagnosticNote *notes;
    size_t note_count;
} NLDiagnostic;

/* All pointers, including stream, are borrowed for this synchronous call.
 * The caller owns their storage and stream; no data is retained or freed.
 * Invalid structures produce no output. False also reports output failure;
 * a stream failure may leave partial output. No mutable global state. */
bool nl_diagnostic_render(FILE *stream, const NLDiagnostic *diagnostic);

#endif
