#include "newlang/diagnostic.h"

#include <stdio.h>
#include <string.h>

static const char help[] =
    "Usage: newlangc [--version | --help | SOURCE]\n"
    "NewLang production compiler P0 bootstrap.\n"
    "  --version  Print the deterministic compiler version.\n"
    "  --help     Print this help.\n"
    "Source compilation is not implemented in P0.\n";

static int report_error(const char *code, const char *message,
                        const char *argument, int status)
{
    const NLDiagnosticNote note = {argument, NULL};
    const NLDiagnostic diagnostic = {NL_DIAG_ERROR, "cli", code, message,
                                     NULL,          &note, 1};
    return nl_diagnostic_render(stderr, &diagnostic) ? status : 1;
}

int main(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
        return fputs(help, stdout) >= 0 && fflush(stdout) == 0 ? 0 : 1;
    }
    if (argc != 2) {
        return report_error("P0-CLI-ARGUMENT", "expected exactly one argument",
                            "use --help for usage", 2);
    }
    if (strcmp(argv[1], "--version") == 0) {
        return fputs("newlangc 0.1.0 (P0 bootstrap)\n", stdout) >= 0 &&
                       fflush(stdout) == 0
                   ? 0
                   : 1;
    }
    if (argv[1][0] == '-') {
        return report_error("P0-CLI-OPTION", "unknown option", argv[1], 2);
    }
    return report_error("P0-COMPILE-UNSUPPORTED",
                        "source compilation is not implemented in P0", argv[1],
                        3);
}
