/* R825: observe the real named-output CLI with one reached allocation fault. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
NLParseStatus __real_nl_parser_parse_function_unit(NLParser *, NLSyntaxTree **,
                                                   NLParseDiagnostic *);
NLCheckStatus
__real_nl_semantic_register_function_unit(NLSemanticContext *,
                                          const NLSyntaxTree *const *, size_t,
                                          NLFunctionUnitDiagnostic *);
NLCheckStatus __real_nl_semantic_check_expression(NLSemanticContext *,
                                                  const NLSyntaxTree *,
                                                  NLCheckedFragment **,
                                                  NLCheckDiagnostic *);

enum {
    PREPARING,
    PARSING,
    AFTER_PARSE,
    REGISTERING,
    AFTER_REGISTER,
    CHECKING_ENTRY,
    AFTER_ENTRY,
    PHASE_COUNT
};
static bool active, reached;
static size_t position, allocations, first[PHASE_COUNT], last[PHASE_COUNT];
static int phase, reached_phase;

static bool inject(void)
{
    if (!active)
        return false;
    const size_t index = allocations++;
    if (first[phase] == SIZE_MAX)
        first[phase] = index;
    last[phase] = index;
    if (index != position)
        return false;
    reached = true;
    reached_phase = phase;
    return true;
}

void *__wrap_malloc(size_t size)
{
    return inject() ? NULL : __real_malloc(size);
}

void *__wrap_calloc(size_t count, size_t size)
{
    return inject() ? NULL : __real_calloc(count, size);
}

void *__wrap_realloc(void *p, size_t size)
{
    return inject() ? NULL : __real_realloc(p, size);
}

NLParseStatus
__wrap_nl_parser_parse_function_unit(NLParser *parser, NLSyntaxTree **tree,
                                     NLParseDiagnostic *diagnostic)
{
    if (active)
        phase = PARSING;
    const NLParseStatus result =
        __real_nl_parser_parse_function_unit(parser, tree, diagnostic);
    if (active)
        phase = AFTER_PARSE;
    return result;
}

NLCheckStatus __wrap_nl_semantic_register_function_unit(
    NLSemanticContext *context, const NLSyntaxTree *const *inputs, size_t count,
    NLFunctionUnitDiagnostic *diagnostic)
{
    if (active)
        phase = REGISTERING;
    const NLCheckStatus result = __real_nl_semantic_register_function_unit(
        context, inputs, count, diagnostic);
    if (active)
        phase = AFTER_REGISTER;
    return result;
}

NLCheckStatus __wrap_nl_semantic_check_expression(NLSemanticContext *context,
                                                  const NLSyntaxTree *tree,
                                                  NLCheckedFragment **out,
                                                  NLCheckDiagnostic *diagnostic)
{
    if (active)
        phase = CHECKING_ENTRY;
    const NLCheckStatus result =
        __real_nl_semantic_check_expression(context, tree, out, diagnostic);
    if (active)
        phase = AFTER_ENTRY;
    return result;
}

int main(int argc, char **argv)
{
    if (argc != 4)
        return 9;
    position = SIZE_MAX;
    if (strcmp(argv[1], "trace") != 0) {
        char *end = NULL;
        errno = 0;
        unsigned long long parsed = strtoull(argv[1], &end, 10);
        if (errno || end == argv[1] || *end || parsed >= SIZE_MAX)
            return 9;
        position = (size_t)parsed;
    }
    for (size_t i = 0; i < PHASE_COUNT; ++i) {
        first[i] = SIZE_MAX;
        last[i] = SIZE_MAX;
    }
    char *arguments[] = {"newlangc", "--emit-c17-to", argv[2], argv[3], NULL};
    active = true;
    const int status = newlang_driver_main(4, arguments);
    active = false;
    fprintf(stderr,
            "R825_META rc=%d hit=%d hit_phase=%d calls=%zu "
            "parser=%zu:%zu register=%zu:%zu entry=%zu:%zu tail=%zu:%zu\n",
            status, reached ? 1 : 0, reached_phase, allocations, first[PARSING],
            last[PARSING], first[REGISTERING], last[REGISTERING],
            first[CHECKING_ENTRY], last[CHECKING_ENTRY], first[AFTER_ENTRY],
            last[AFTER_ENTRY]);
    return status;
}
