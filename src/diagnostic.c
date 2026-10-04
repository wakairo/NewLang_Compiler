#include "newlang/diagnostic.h"

static bool range_valid(const NLSourceRange *range)
{
    return range == NULL ||
           (range->path != NULL && range->start_line > 0 &&
            range->start_column > 0 && range->end_column > 0 &&
            (range->end_line > range->start_line ||
             (range->end_line == range->start_line &&
              range->end_column >= range->start_column)));
}

static bool render_range(FILE *stream, const NLSourceRange *range)
{
    return range == NULL ||
           fprintf(stream, "%s:%zu:%zu-%zu:%zu: ", range->path,
                   range->start_line, range->start_column,
                   range->end_line, range->end_column) >= 0;
}

bool nl_diagnostic_render(FILE *stream, const NLDiagnostic *diagnostic)
{
    if (stream == NULL || diagnostic == NULL || diagnostic->message == NULL ||
        diagnostic->severity < NL_DIAG_NOTE ||
        diagnostic->severity > NL_DIAG_ERROR || !range_valid(diagnostic->range) ||
        (diagnostic->note_count > 0 && diagnostic->notes == NULL)) {
        return false;
    }
    for (size_t i = 0; i < diagnostic->note_count; ++i) {
        if (diagnostic->notes[i].message == NULL ||
            !range_valid(diagnostic->notes[i].range)) {
            return false;
        }
    }

    const char *severity = "note";
    if (diagnostic->severity == NL_DIAG_WARNING) {
        severity = "warning";
    } else if (diagnostic->severity == NL_DIAG_ERROR) {
        severity = "error";
    }
    if (!render_range(stream, diagnostic->range) ||
        fprintf(stream, "%s", severity) < 0) {
        return false;
    }
    if (diagnostic->category != NULL &&
        fprintf(stream, "(%s)", diagnostic->category) < 0) {
        return false;
    }
    if (diagnostic->code != NULL &&
        fprintf(stream, "[%s]", diagnostic->code) < 0) {
        return false;
    }
    if (fprintf(stream, ": %s\n", diagnostic->message) < 0) {
        return false;
    }
    for (size_t i = 0; i < diagnostic->note_count; ++i) {
        if (!render_range(stream, diagnostic->notes[i].range) ||
            fprintf(stream, "note: %s\n", diagnostic->notes[i].message) < 0) {
            return false;
        }
    }
    return !ferror(stream);
}
