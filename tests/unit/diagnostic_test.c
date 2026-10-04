#include "newlang/diagnostic.h"

#include <stdio.h>
#include <string.h>

/* Checks stay active in Release builds; no dependence on assert/NDEBUG. */
static bool expect_render(const NLDiagnostic *diagnostic, bool valid,
                          const char *expected)
{
    FILE *const stream = tmpfile(); /* Owned, closed on all paths. */
    if (stream == NULL) {
        return false;
    }
    const bool rendered = nl_diagnostic_render(stream, diagnostic);
    bool ok = rendered == valid;
    if (fflush(stream) != 0 || fseek(stream, 0, SEEK_SET) != 0) {
        ok = false;
    }
    char text[512] = {0};
    const size_t count = fread(text, 1, sizeof(text) - 1, stream);
    if (ferror(stream) || count != strlen(expected) ||
        strcmp(text, expected) != 0) {
        ok = false;
    }
    if (fclose(stream) != 0) {
        ok = false;
    }
    return ok;
}

int main(void)
{
    const NLSourceRange range = {"sample.nl", 2, 3, 2, 8};
    const NLDiagnosticNote notes[] = {
        {"related source", &range}, {"follow-up", NULL}
    };
    NLDiagnostic diagnostic = {
        NL_DIAG_ERROR, "implementation", "P0-TEST", "primary", &range, notes, 2
    };
    if (!expect_render(&diagnostic, true,
            "sample.nl:2:3-2:8: error(implementation)[P0-TEST]: primary\n"
            "sample.nl:2:3-2:8: note: related source\nnote: follow-up\n")) {
        return 1;
    }
    diagnostic = (NLDiagnostic){NL_DIAG_WARNING, NULL, NULL, "warning",
                                NULL, NULL, 0};
    if (!expect_render(&diagnostic, true, "warning: warning\n")) {
        return 1;
    }
    diagnostic.severity = NL_DIAG_NOTE;
    if (!expect_render(&diagnostic, true, "note: warning\n")) {
        return 1;
    }
    diagnostic.message = NULL;
    if (!expect_render(&diagnostic, false, "")) {
        return 1;
    }
    diagnostic.message = "primary";
    diagnostic.severity = (NLDiagnosticSeverity)99;
    if (!expect_render(&diagnostic, false, "")) {
        return 1;
    }
    diagnostic.severity = NL_DIAG_ERROR;
    diagnostic.note_count = 1;
    if (!expect_render(&diagnostic, false, "")) {
        return 1;
    }
    const NLDiagnosticNote invalid_note = {NULL, NULL};
    diagnostic.notes = &invalid_note;
    if (!expect_render(&diagnostic, false, "")) {
        return 1;
    }
    const NLSourceRange invalid_range = {"sample.nl", 2, 3, 1, 1};
    diagnostic.note_count = 0;
    diagnostic.range = &invalid_range;
    if (!expect_render(&diagnostic, false, "") ||
        !expect_render(NULL, false, "") ||
        nl_diagnostic_render(NULL, &diagnostic)) {
        return 1;
    }
    puts("diagnostics: 10 checks passed");
    return 0;
}
