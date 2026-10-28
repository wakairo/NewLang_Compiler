#define main p974_actual_driver_main
#include "../../src/main.c"
#undef main
#include "support.h"

static bool inject;
static NLCheckedFragment *injected_entry;
static NLSemanticContext *injected_world;
static NLFunctionEntry *saved_functions;
static size_t injected, restored;
NLCheckStatus __real_nl_semantic_check_expression(NLSemanticContext *,
                                                  const NLSyntaxTree *,
                                                  NLCheckedFragment **,
                                                  NLCheckDiagnostic *);
NLCheckStatus __wrap_nl_semantic_check_expression(NLSemanticContext *c,
                                                  const NLSyntaxTree *tree,
                                                  NLCheckedFragment **out,
                                                  NLCheckDiagnostic *d)
{
    NLCheckStatus s = __real_nl_semantic_check_expression(c, tree, out, d);
    if (inject && s == NL_CHECK_OK && *out && c->functions &&
        c->function_count) {
        injected_entry = *out;
        injected_world = c;
        saved_functions = c->functions;
        c->functions = NULL; /* Keep genuine original positive count. */
        ++injected;
    }
    return s;
}
void __real_nl_checked_destroy(NLCheckedFragment *);
void __wrap_nl_checked_destroy(NLCheckedFragment *f)
{
    if (f && f == injected_entry) {
        injected_world->functions = saved_functions;
        injected_entry = NULL;
        ++restored;
    }
    __real_nl_checked_destroy(f);
}
static bool probe(const char *path, bool unit)
{
    NLSemanticContext *base = NULL;
    TestNode a = {0};
    CHECK(registry(path, &base) && checked(base, &a));
    P974Observation o;
    CHECK(p974_find(a.entry, unit, &o));
    const NLSemanticContext *c = o.caller->context;
    V0Program program = {.unit = nl_semantic_unit_type(c),
                         .u8 = nl_semantic_core_type(c, NL_TYPE_U8),
                         .aggregate = o.parameters.field.nominal};
    NLSymbolId parameter = 0;
    CHECK(v1_call_parameter(o.caller, o.call, &program, &parameter) &&
          parameter);
    V0Program before;
    memcpy(&before, &program, sizeof(before));
    NLSemanticContext *current = (NLSemanticContext *)c;
    NLFunctionEntry *functions = current->functions;
    size_t count = current->function_count;
    CHECK(functions && count);
    parameter = SIZE_MAX;
    current->functions = NULL;
    bool observed = p974_get(&o, unit);
    bool admitted = v1_call_parameter(o.caller, o.call, &program, &parameter);
    bool validated = v0_validate_call(o.caller, o.call, &program);
    size_t exported = v0_export_function(current, "p974_absent_export");
    char *fallback = NULL;
    size_t length = 777;
    NLCheckStatus array = nl_checked_array4_c17(a.entry, &fallback, &length);
    current->functions = functions;
    CHECK(observed && !admitted && !validated && !exported &&
          parameter == SIZE_MAX && current->function_count == count &&
          !memcmp(&before, &program, sizeof(program)) &&
          array == NL_CHECK_SEMANTIC_UNSUPPORTED && !fallback && length == 777);
    CHECK(v1_call_parameter(o.caller, o.call, &program, &parameter) &&
          parameter);
    CHECK(p974_validate(&o, unit, true));
    node_checked_destroy(&a);
    nl_semantic_destroy(base);
    puts("P974 V0 first consumer NULL table refused; parameter/program "
         "unchanged; "
         "original table restored before cleanup");
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "probe"))
        return probe(argv[2], !strcmp(argv[3], "unit")) ? 0 : 1;
    if (argc != 5 || strcmp(argv[1], "emit"))
        return 1;
    inject = !strcmp(argv[4], "invalid");
    char *args[] = {argv[0], "--emit-c17-to", argv[3], argv[2]};
    int result = p974_actual_driver_main(4, args);
    if (inject && (injected != 1 || restored != 1 || injected_entry))
        return 1;
    return result;
}
