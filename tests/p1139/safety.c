#define main p1139_cli_main
#include "main.c"
#undef main
#include "newlang/usize_addition.h"
#include "newlang/usize_subtraction.h"
#include <assert.h>
#include <dirent.h>

typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *tree, *entry_tree;
    NLSemanticContext *world;
    NLCheckedFragment *root;
    NLCheckedFragment *owner, *graph;
    NLCheckedNodeId loop;
} Input;
static void dispose(Input *a)
{
    nl_checked_destroy(a->root);
    nl_semantic_destroy(a->world);
    nl_syntax_tree_destroy(a->entry_tree);
    nl_parser_destroy(a->entry_parser);
    nl_source_destroy(a->entry_source);
    nl_syntax_tree_destroy(a->tree);
    nl_parser_destroy(a->parser);
    nl_source_destroy(a->source);
}
static void prepare(Input *a, const char *path)
{
    assert(nl_source_load(path, &a->source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->source, &a->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(a->parser, &a->tree, NULL) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&a->world) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {a->tree};
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(a->world, units, 1, &diagnostic);
    printf("C223 REGISTER=%d\n", status);
    if (status != NL_CHECK_OK)
        (void)nl_check_diagnostic_render(stderr, a->source,
                                         &diagnostic.diagnostic);
    assert(status == NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C223-actual-entry",
                            &a->entry_source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->entry_source, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(a->entry_parser, &a->entry_tree,
                                               NULL) == NL_PARSE_OK);
    status =
        nl_semantic_check_expression(a->world, a->entry_tree, &a->root, NULL);
    printf("C223 INVOKE=%d\n", status);
    assert(status == NL_CHECK_OK);
    a->owner = (NLCheckedFragment *)nl_checked_call_body(
        a->root, nl_checked_root(a->root));
    assert(a->owner);
    for (size_t i = 1; i <= a->owner->count; ++i)
        if (a->owner->nodes[i - 1].kind == NL_CHECKED_LOOP) {
            assert(!a->loop);
            a->loop = i;
        }
    assert(a->loop);
    a->graph = (NLCheckedFragment *)nl_checked_loop_body(a->owner, a->loop);
    assert(a->graph);
}
static bool valid(Input *a)
{
    NLLinearBreakView proof = {0};
    if (nl_linear_break_validate(a->owner, a->loop, &proof) != NL_CHECK_OK ||
        !proof.stuttering || !proof.scalar_loop)
        return false;
    V0Program p = {.unit = nl_semantic_unit_type(a->world),
                   .u8 = nl_semantic_core_type(a->world, NL_TYPE_U8)};
    return v0_validate_call(a->root, nl_checked_root(a->root), &p);
}
#ifndef P1139_OOM
int main(int argc, char **argv)
{
    assert(argc == 2);
    setvbuf(stdout, NULL, _IONBF, 0);
    Input a = {0}, b = {0};
    prepare(&a, argv[1]);
    assert(valid(&a));
    NLLinearBreakView proof = {0};
    assert(nl_linear_break_validate(a.owner, a.loop, &proof) == NL_CHECK_OK &&
           proof.scalar_loop);
    NLSemanticValueView value;
    assert(
        nl_semantic_value_view(a.owner->context, proof.result.value, &value) &&
        !value.scalar_known && value.dependencies == NL_DEPENDENCY_FREE &&
        !value.value_dependency_count);
    V0Program p = {.unit = nl_semantic_unit_type(a.world),
                   .u8 = nl_semantic_core_type(a.world, NL_TYPE_U8)};
    size_t calls = 0;
    NLCheckedNodeView *call = NULL;
    for (size_t id = 1; id <= a.owner->count; ++id)
        if (a.owner->nodes[id - 1].kind == NL_CHECKED_REGISTERED_CALL) {
            call = &a.owner->nodes[id - 1];
            NLSymbolId formal = 0;
            assert(v1_call_parameter_kind(a.owner, id, &p, &formal, true) &&
                   formal && predicate_call(a.owner, id, &p));
            const NLCheckedNodeView *arg =
                nl_checked_node_view(a.owner, call->first_argument);
            assert(loop_quantity_scalar(a.owner, call->first_argument, &p) &&
                   nl_semantic_value_view(a.owner->context,
                                          arg->results[0].value, &value) &&
                   !value.scalar_known);
            assert(!quantity_scalar(a.owner, call->first_argument, &p));
            ++calls;
        }
    assert(calls == 1 && call);
    puts("C223 authentic Unknown loop result -> Copy -> actual/formal -> "
         "source predicate/current PASS");
    NLCheckedNodeView *loop = &a.owner->nodes[a.loop - 1];
    NLCheckedNodeView before = *loop;
    for (size_t t = 0; t < 5; ++t) {
        if (t == 0)
            loop->results[0].value = proof.result.value - 1;
        if (t == 1)
            loop->continue_edges = 0;
        if (t == 2)
            loop->header_inductive = false;
        if (t == 3)
            loop->first_argument = 0;
        if (t == 4)
            loop->type = p.u8;
        assert(!valid(&a));
        *loop = before;
        assert(valid(&a));
    }
    NLCheckedNodeView saved = *call;
    call->function = SIZE_MAX;
    assert(!valid(&a));
    *call = saved;
    NLCheckedNodeView *arg = &a.owner->nodes[call->first_argument - 1];
    saved = *arg;
    arg->parameter_type = p.u8;
    assert(!valid(&a));
    *arg = saved;
    NLSemanticContext *world = (NLSemanticContext *)a.owner->context;
    NLFunctionEntry *functions = world->functions;
    world->functions = NULL;
    assert(!valid(&a));
    world->functions = functions;
    NLSemanticValueView *v = &world->values[proof.result.value - 1];
    NLSemanticValueView original = *v;
    v->scalar_known = true;
    assert(!valid(&a));
    *v = original;
    v->dependencies = NL_DEPENDENCIES_UNKNOWN;
    assert(!valid(&a));
    *v = original;
    assert(valid(&a));
    NLCheckedFragment *arm = (NLCheckedFragment *)nl_checked_if_arm(
        a.graph, a.graph->nodes[a.graph->root - 1].tail, 0);
    const NLControlExitView *edge = nl_control_exit_view(arm->exits, 0);
    NLControlTarget *foreign = NULL;
    assert(nl_control_target_create(NL_TARGET_LOOP, &foreign) == NL_CHECK_OK);
    const NLControlTarget *target = edge->target;
    ((NLControlExitView *)edge)->target = foreign;
    assert(!valid(&a));
    ((NLControlExitView *)edge)->target = target;
    nl_control_target_destroy(foreign);
    const NLSource *source = a.graph->source;
    a.graph->source = a.entry_source;
    assert(!valid(&a));
    a.graph->source = source;
    assert(valid(&a));
    puts("C223 wrong-result/target/source/consumer/type/current/dependency "
         "refusals PASS");
    dispose(&a);
    for (size_t t = 0; t < 6; ++t) {
        a = (Input){0};
        b = (Input){0};
        prepare(&a, argv[1]);
        prepare(&b, argv[1]);
        assert(valid(&a) && valid(&b));
        if (t == 0)
            a.graph->arm_count = 0;
        if (t == 1)
            a.graph->arm_count = SIZE_MAX;
        if (t == 2)
            a.root->body_count = 0;
        if (t == 3)
            a.root->body_count = SIZE_MAX;
        if (t == 4) {
            a.graph->arms = b.graph->arms;
            a.graph->arm_count = b.graph->arm_count;
        }
        if (t == 5) {
            a.root->bodies = b.root->bodies;
            a.root->body_count = b.root->body_count;
        }
        assert(!valid(&a) && valid(&b));
        dispose(&a);
        assert(valid(&b));
        dispose(&b);
    }
    puts("C223 canonical A destruction / B survives PASS");
    return 0;
}
#else
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting, hit;
static size_t allocation, failure;
static void *pc;
static char allocation_kind;
static int phase, fault_phase, fault_status;
static bool reject(char kind, void *caller)
{
    if (!injecting || allocation++ != failure)
        return false;
    hit = true;
    pc = caller;
    allocation_kind = kind;
    fault_phase = phase;
    return true;
}
void *__wrap_malloc(size_t n)
{
    return reject('m', __builtin_return_address(0)) ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return reject('c', __builtin_return_address(0)) ? NULL
                                                    : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return reject('r', __builtin_return_address(0)) ? NULL
                                                    : __real_realloc(p, n);
}
/* Observe the existing CLI's exact layer mapping without altering its calls,
 * returned status, arguments, diagnostics, allocation sequence or output. */
NLParseStatus __real_nl_parser_parse_function_unit(NLParser *, NLSyntaxTree **,
                                                   NLParseDiagnostic *);
NLParseStatus __real_nl_parser_parse_expression_fragment(NLParser *,
                                                         NLSyntaxTree **,
                                                         NLParseDiagnostic *);
NLCheckStatus
__real_nl_semantic_register_function_unit(NLSemanticContext *,
                                          const NLSyntaxTree *const *, size_t,
                                          NLFunctionUnitDiagnostic *);
NLCheckStatus __real_nl_semantic_check_expression(NLSemanticContext *,
                                                  const NLSyntaxTree *,
                                                  NLCheckedFragment **,
                                                  NLCheckDiagnostic *);
NLParseStatus __wrap_nl_parser_parse_function_unit(NLParser *p,
                                                   NLSyntaxTree **t,
                                                   NLParseDiagnostic *d)
{
    int old = phase;
    phase = 1;
    NLParseStatus s = __real_nl_parser_parse_function_unit(p, t, d);
    if (hit && fault_phase == 1)
        fault_status = s;
    phase = old;
    return s;
}
NLParseStatus __wrap_nl_parser_parse_expression_fragment(NLParser *p,
                                                         NLSyntaxTree **t,
                                                         NLParseDiagnostic *d)
{
    int old = phase;
    phase = 1;
    NLParseStatus s = __real_nl_parser_parse_expression_fragment(p, t, d);
    if (hit && fault_phase == 1)
        fault_status = s;
    phase = old;
    return s;
}
NLCheckStatus
__wrap_nl_semantic_register_function_unit(NLSemanticContext *c,
                                          const NLSyntaxTree *const *t,
                                          size_t n, NLFunctionUnitDiagnostic *d)
{
    int old = phase;
    phase = 2;
    NLCheckStatus s = __real_nl_semantic_register_function_unit(c, t, n, d);
    if (hit && fault_phase == 2)
        fault_status = s;
    phase = old;
    return s;
}
NLCheckStatus __wrap_nl_semantic_check_expression(NLSemanticContext *c,
                                                  const NLSyntaxTree *t,
                                                  NLCheckedFragment **f,
                                                  NLCheckDiagnostic *d)
{
    int old = phase;
    phase = 3;
    NLCheckStatus s = __real_nl_semantic_check_expression(c, t, f, d);
    if (hit && fault_phase == 3)
        fault_status = s;
    phase = old;
    return s;
}
static bool current(const Input *i)
{
    V0Program p = {.unit = nl_semantic_unit_type(i->world),
                   .u8 = nl_semantic_core_type(i->world, NL_TYPE_U8)};
    return i->root && v0_validate_call(i->root, nl_checked_root(i->root), &p);
}
static void syntax(Input *i, const char *path)
{
    assert(nl_source_load(path, &i->source) == NL_SOURCE_OK);
    assert(nl_parser_create(i->source, &i->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(i->parser, &i->tree, NULL) ==
           NL_PARSE_OK);
    assert(nl_source_create("main()", 6, "C223-OOM-entry", &i->entry_source) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(i->entry_source, &i->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(i->entry_parser, &i->entry_tree,
                                               NULL) == NL_PARSE_OK);
}
static NLCheckStatus registration(Input *i)
{
    const NLSyntaxTree *units[] = {i->tree};
    return nl_semantic_register_function_unit(i->world, units, 1, NULL);
}
static NLCheckStatus invocation(Input *i)
{
    return nl_semantic_check_expression(i->world, i->entry_tree, &i->root,
                                        NULL);
}
static void api_oom(const char *path, bool invoke)
{
    Input i = {0};
    syntax(&i, path);
    size_t reached = 0;
    for (size_t f = SIZE_MAX;; f = f == SIZE_MAX ? 0 : f + 1) {
        assert(nl_semantic_create(&i.world) == NL_CHECK_OK);
        if (invoke)
            assert(registration(&i) == NL_CHECK_OK);
        NLSemanticContext before = *i.world;
        allocation = 0;
        failure = f;
        hit = false;
        injecting = true;
        NLCheckStatus s = invoke ? invocation(&i) : registration(&i);
        injecting = false;
        if (f == SIZE_MAX) {
            assert(s == NL_CHECK_OK);
            reached = allocation;
            assert(reached);
        } else {
            printf("C223 %s ordinal=%zu kind=%c pc=%p status=%d\n",
                   invoke ? "invocation" : "registration", f, allocation_kind,
                   pc, s);
            assert(!fflush(stdout));
            assert(hit && allocation > f && s == NL_CHECK_OUT_OF_MEMORY);
            assert(!i.root && !memcmp(&before, i.world, sizeof(before)));
            assert((invoke ? invocation(&i) : registration(&i)) == NL_CHECK_OK);
        }
        if (!invoke)
            assert(invocation(&i) == NL_CHECK_OK);
        assert(current(&i));
        nl_checked_destroy(i.root);
        i.root = NULL;
        nl_semantic_destroy(i.world);
        i.world = NULL;
        if (f != SIZE_MAX && f + 1 == reached)
            break;
    }
    dispose(&i);
    printf(
        "C223 reached %s OOM=%zu rollback/no-publication/current retry PASS\n",
        invoke ? "invocation" : "registration", reached);
}
static const char original_output[] = "P1139 immutable output\n";
static void sentinel(const char *path)
{
    FILE *f = fopen(path, "wb");
    assert(f &&
           fwrite(original_output, 1, sizeof(original_output) - 1, f) ==
               sizeof(original_output) - 1 &&
           !fclose(f));
}
static void cli_oom(const char *path)
{
    char directory[] = "/tmp/p1139-named-XXXXXX";
    assert(mkdtemp(directory));
    char output[128];
    assert(snprintf(output, sizeof(output), "%s/out.c", directory) > 0);
    char *args[] = {"newlangc", "--emit-c17-to", output, (char *)path};
    size_t reached = 0;
    for (size_t f = SIZE_MAX;; f = f == SIZE_MAX ? 0 : f + 1) {
        sentinel(output);
        struct stat before, after;
        assert(!stat(output, &before));
        allocation = 0;
        failure = f;
        hit = false;
        phase = fault_phase = 0;
        fault_status = -1;
        injecting = true;
        int s = p1139_cli_main(4, args);
        injecting = false;
        if (f == SIZE_MAX) {
            assert(!s);
            reached = allocation;
            assert(reached);
        } else {
            const int expected = fault_phase ? 3 : 1;
            printf("C223 named ordinal=%zu kind=%c pc=%p phase=%d "
                   "api_status=%d status=%d expected=%d\n",
                   f, allocation_kind, pc, fault_phase, fault_status, s,
                   expected);
            assert(!fflush(stdout));
            assert(hit && allocation > f && s == expected &&
                   !stat(output, &after));
            assert(!fault_phase ||
                   fault_status == (fault_phase == 1
                                        ? (int)NL_PARSE_OUT_OF_MEMORY
                                        : (int)NL_CHECK_OUT_OF_MEMORY));
            assert(before.st_ino == after.st_ino &&
                   before.st_size == after.st_size &&
                   before.st_mtim.tv_sec == after.st_mtim.tv_sec &&
                   before.st_mtim.tv_nsec == after.st_mtim.tv_nsec);
            FILE *file = fopen(output, "rb");
            char bytes[64] = {0};
            assert(file);
            const size_t length = fread(bytes, 1, sizeof(bytes), file);
            assert(!fclose(file) && length == sizeof(original_output) - 1);
            assert(
                !memcmp(bytes, original_output, sizeof(original_output) - 1));
            DIR *d = opendir(directory);
            assert(d);
            size_t files = 0;
            struct dirent *e;
            while ((e = readdir(d)))
                if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
                    assert(!strcmp(e->d_name, "out.c"));
                    ++files;
                }
            assert(files == 1 && !closedir(d));
            assert(!p1139_cli_main(4, args));
        }
        if (f != SIZE_MAX && f + 1 == reached)
            break;
    }
    assert(!unlink(output) && !rmdir(directory));
    printf("C223 reached named CLI OOM=%zu exact bytes/inode/mtime/temp/retry "
           "PASS\n",
           reached);
}
int main(int argc, char **argv)
{
    assert(argc == 2 || argc == 3);
    setvbuf(stdout, NULL, _IONBF, 0);
    Input i = {0};
    prepare(&i, argv[1]);
    assert(valid(&i));
    dispose(&i);
    if (argc == 2) {
        api_oom(argv[1], false);
        api_oom(argv[1], true);
    } else
        assert(!strcmp(argv[2], "--named-only"));
    cli_oom(argv[1]);
    return 0;
}
#endif
