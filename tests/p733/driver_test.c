#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t calls, fail_at, host_diagnostics, generic_diagnostics;
void *__wrap_malloc(size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && calls++ == fail_at ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_realloc(p, n);
}
static bool rejection(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(c, before));
    const NLSyntaxTree *units[] = {unit};
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(c, units, 1, &diagnostic);
    const char *phase = "registration";
    if (status == NL_CHECK_OK) {
        CHECK(test_state(c, before));
        NLSource *entry = NULL;
        NLParser *call_parser = NULL;
        NLSyntaxTree *call = NULL;
        NLCheckedFragment *out = NULL;
        CHECK(nl_source_create("main()", 6, "entry", &entry) == NL_SOURCE_OK &&
              nl_parser_create(entry, &call_parser) == NL_PARSE_OK &&
              nl_parser_parse_expression_fragment(call_parser, &call, NULL) ==
                  NL_PARSE_OK);
        status = nl_semantic_check_expression(c, call, &out, NULL);
        CHECK(!out);
        nl_syntax_tree_destroy(call);
        nl_parser_destroy(call_parser);
        nl_source_destroy(entry);
        phase = "actual_main";
    }
    CHECK(status != NL_CHECK_OK && status != NL_CHECK_OUT_OF_MEMORY &&
          status != NL_CHECK_INTERNAL_ERROR && test_unchanged(c, before));
    printf("rejection phase=%s status=%d rollback=PASS artifact=NULL\n", phase,
           (int)status);
    free(before);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool driver_oom(const char *path)
{
    FILE *diagnostics = tmpfile();
    CHECK(diagnostics);
    int saved = dup(STDERR_FILENO);
    CHECK(saved >= 0 && fflush(stderr) == 0 &&
          dup2(fileno(diagnostics), STDERR_FILENO) >= 0);
    bool ok = true;
    size_t status_count[5] = {0};
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        FILE *output = tmpfile();
        if (!output || setvbuf(output, NULL, _IONBF, 0) != 0) {
            ok = false;
            break;
        }
        if (fflush(stderr) || fflush(diagnostics) ||
            ftruncate(fileno(diagnostics), 0) ||
            fseek(diagnostics, 0, SEEK_SET)) {
            fclose(output);
            ok = false;
            break;
        }
        calls = 0;
        injecting = true;
        int result = compile_v0_to(path, false, NULL, output);
        injecting = false;
        long length = ftell(output);
        if (fclose(output)) {
            ok = false;
            break;
        }
        if (result == 0) {
            ok = length > 0;
            break;
        }
        /* Legacy bool producer validators cannot distinguish scratch OOM
         * from unsupported. Record injected CLI4 separately, never claim it
         * is a semantic/profile failure of the unchanged valid source. */
        if (calls <= fail_at || length != 0 ||
            (result != 1 && result != 3 && result != 4)) {
            fprintf(diagnostics, "driver stop point=%zu status=%d bytes=%ld\n",
                    fail_at, result, length);
            ok = false;
            break;
        }
        if (fflush(stderr) || fflush(diagnostics) ||
            fseek(diagnostics, 0, SEEK_SET)) {
            ok = false;
            break;
        }
        char message[8192] = {0};
        size_t got = fread(message, 1, sizeof(message) - 1, diagnostics);
        message[got] = 0;
        bool classified =
            result == 1   ? strstr(message, "error(internal)") == NULL
            : result == 4 ? strstr(message, "V1-BACKEND-UNSUPPORTED") != NULL
                          : ((strstr(message, "OUT-OF-MEMORY") != NULL ||
                              strstr(message, "[V0-CHECK]") != NULL) &&
                             strstr(message, "error(internal)") == NULL);
        if (!classified) {
            fseek(diagnostics, 0, SEEK_END);
            fprintf(diagnostics,
                    "unclassified failure position=%zu status=%d\n", fail_at,
                    result);
            ok = false;
            break;
        }
        if (result == 3) {
            if (strstr(message, "OUT-OF-MEMORY"))
                ++host_diagnostics;
            else
                ++generic_diagnostics;
        }
        ++status_count[result];
    }
    CHECK(fflush(stderr) == 0 && dup2(saved, STDERR_FILENO) >= 0 &&
          close(saved) == 0);
    if (!ok) {
        CHECK(fflush(diagnostics) == 0 && fseek(diagnostics, 0, SEEK_SET) == 0);
        char line[512];
        while (fgets(line, sizeof(line), diagnostics))
            fputs(line, stderr);
    }
    CHECK(fclose(diagnostics) == 0 && ok && fail_at && fail_at < 10000);
    printf("normal driver fail-at-N positions=%zu CLI1=%zu CLI3=%zu "
           "legacy_bool_CLI4=%zu zero_partial_C=PASS recovery=PASS\n",
           fail_at, status_count[1], status_count[3], status_count[4]);
    printf("CLI3 explicit_OOM=%zu existing_generic_V0_CHECK=%zu internal=0\n",
           host_diagnostics, generic_diagnostics);
    return true;
}
int main(int argc, char **argv)
{
    if (argc < 3)
        return 1;
    if (!strcmp(argv[1], "oom"))
        return driver_oom(argv[2]) ? 0 : 1;
    if (strcmp(argv[1], "reject"))
        return 1;
    for (int i = 2; i < argc; ++i)
        if (!rejection(argv[i]))
            return 1;
    return 0;
}
