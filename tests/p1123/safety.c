#define main p1123_cli_main
#include "main.c"
#undef main
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
    printf("C220 REGISTER=%d\n", status);
    if (status != NL_CHECK_OK)
        (void)nl_check_diagnostic_render(stderr, a->source,
                                         &diagnostic.diagnostic);
    assert(status == NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C220-actual-entry",
                            &a->entry_source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->entry_source, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(a->entry_parser, &a->entry_tree,
                                               NULL) == NL_PARSE_OK);
    status =
        nl_semantic_check_expression(a->world, a->entry_tree, &a->root, NULL);
    printf("C220 INVOKE=%d\n", status);
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
        !proof.stuttering)
        return false;
    V0Program p = {.unit = nl_semantic_unit_type(a->world),
                   .u8 = nl_semantic_core_type(a->world, NL_TYPE_U8)};
    return v0_validate_call(a->root, nl_checked_root(a->root), &p);
}
#ifndef P1123_OOM
int main(int argc, char **argv)
{
    assert(argc == 2);
    setvbuf(stdout, NULL, _IONBF, 0);
    Input a = {0}, b = {0};
    prepare(&a, argv[1]);
    NLLinearBreakView proof = {0};
    NLCheckStatus status = nl_linear_break_validate(a.owner, a.loop, &proof);
    printf("C220 PRIVATE=%d stuttering=%d base=%zu field=%zu old-id=%zu "
           "incoming=%zu phase=%zu,%zu limit=%zu\n",
           status, proof.stuttering, proof.base, proof.index,
           proof.result.value, proof.incoming, proof.initial_phase,
           proof.next_phase, proof.limit);
    assert(status == NL_CHECK_OK && proof.stuttering && valid(&a));
    NLCopyBodyView public_copy = {0};
    NLCopyBodyParameters parameters = {0};
    NLCopyBodyEventView event = {0};
    assert(proof.caller && proof.call &&
           nl_checked_copy_body_view(proof.caller, proof.call, &public_copy) &&
           nl_checked_copy_body_validate(proof.caller, proof.call,
                                         &public_copy) == NL_CHECK_OK &&
           nl_checked_copy_body_parameters(proof.caller, proof.call,
                                           &parameters) &&
           public_copy.event_count == 1 &&
           nl_checked_copy_body_event(proof.caller, proof.call, 0, &event) &&
           event.kind == NL_COPY_BODY_CHANGE && event.field.present &&
           event.field.parent == proof.root &&
           event.field.index == proof.index);
    printf("C220 genuine public Copy GET/current0 parameters=%zu,%zu Change "
           "old=%zu post=%zu same-root=%zu\n",
           parameters.parameters[0], parameters.parameters[1],
           event.field.old_value, event.field.new_value, event.field.parent);
    NLCheckedNodeView *loop = &a.owner->nodes[a.loop - 1];
    size_t old_count = loop->continue_edges;
    loop->continue_edges = 0;
    assert(!valid(&a));
    loop->continue_edges = old_count;
    NLControlExits *saved = a.graph->exits;
    a.graph->exits = NULL;
    assert(!valid(&a));
    a.graph->exits = saved;
    NLCheckedNodeId saved_root = a.graph->root;
    a.graph->root = 0;
    assert(!valid(&a));
    a.graph->root = saved_root;
    NLCheckedFragment *arm = (NLCheckedFragment *)nl_checked_if_arm(
        a.graph, nl_checked_node_view(a.graph, a.graph->root)->tail, 0);
    assert(arm);
    const NLControlExitView *edge = nl_control_exit_view(arm->exits, 0);
    NLControlExitView *mutable_edge = (NLControlExitView *)edge;
    const NLControlTarget *target = edge->target;
    NLControlTarget *foreign = NULL;
    assert(nl_control_target_create(NL_TARGET_LOOP, &foreign) == NL_CHECK_OK);
    mutable_edge->target = foreign;
    assert(!valid(&a));
    mutable_edge->target = target;
    nl_control_target_destroy(foreign);
    NLSemanticContext *post =
        nl_control_state_context((NLControlState *)edge->state);
    NLValueFactId fact = post->places[proof.root - 1].current_fact;
    post->places[proof.root - 1].current_fact = 0;
    assert(!valid(&a));
    post->places[proof.root - 1].current_fact = fact;
    assert(valid(&a));
    /* The consumer must reject current registry/source/plan and graph edits. */
    NLSemanticContext *world = (NLSemanticContext *)a.owner->context;
    NLFunctionEntry *functions = world->functions;
    world->functions = NULL;
    assert(!valid(&a));
    world->functions = functions;
    size_t count = a.graph->arm_count;
    a.graph->arm_count = SIZE_MAX;
    assert(!valid(&a));
    a.graph->arm_count = count;
    count = a.graph->count;
    a.graph->count = SIZE_MAX;
    assert(!valid(&a));
    a.graph->count = count;
    const NLSource *source = a.graph->source;
    a.graph->source = a.entry_source;
    assert(!valid(&a));
    a.graph->source = source;
    NLCheckedNodeId condition_id =
        a.graph->nodes[a.graph->nodes[a.graph->root - 1].tail - 1].initializer;
    NLCheckedNodeView condition = a.graph->nodes[condition_id - 1];
    a.graph->nodes[condition_id - 1].scalar_result.known = true;
    assert(!valid(&a));
    a.graph->nodes[condition_id - 1] = condition;
    const NLCheckedFragment *normal = nl_checked_match_arm(a.owner, a.loop, 1);
    assert(normal && proof.caller == normal);
    NLCheckedNodeView *call =
        &((NLCheckedFragment *)normal)->nodes[proof.call - 1];
    size_t function = call->function;
    call->function = SIZE_MAX;
    assert(!valid(&a));
    call->function = function;
    assert(valid(&a));
    puts("C220 missing-edge/count/root/wrong-target/stale-current refusals "
         "PASS");
    prepare(&b, argv[1]);
    assert(valid(&b));
    size_t body_slot = SIZE_MAX;
    for (size_t i = 0; i < a.owner->arm_count; ++i)
        if (a.owner->arms[i].match == a.loop &&
            a.owner->arms[i].artifact == a.graph)
            body_slot = i;
    assert(body_slot != SIZE_MAX);
    /* Real B stays alive, and A remains unrestored during A destruction.
     * Canonical custody, not display arrays, must select A's actual child. */
    a.owner->arms[body_slot].artifact = b.graph;
    assert(!valid(&a) && valid(&b));
    dispose(&a);
    assert(valid(&b));
    dispose(&b);
    puts("C220 canonical A destruction / B survives PASS; altered graph never "
         "emitted/native");
    /* Cardinality and whole-array destruction are independent of views. */
    for (size_t trial = 0; trial < 6; ++trial) {
        Input x = {0}, y = {0};
        prepare(&x, argv[1]);
        prepare(&y, argv[1]);
        assert(valid(&x) && valid(&y));
        if (trial == 0)
            x.graph->arm_count = 0;
        if (trial == 1)
            x.graph->arm_count = SIZE_MAX;
        assert(x.root->body_count > 0);
        if (trial == 2)
            x.root->body_count = SIZE_MAX;
        if (trial == 3)
            x.root->body_count = 0;
        if (trial == 4) {
            x.graph->arms = y.graph->arms;
            x.graph->arm_count = y.graph->arm_count;
        }
        if (trial == 5) {
            x.root->bodies = y.root->bodies;
            x.root->body_count = y.root->body_count;
        }
        assert(!valid(&x) && valid(&y));
        dispose(&x);
        assert(valid(&y));
        dispose(&y);
    }
    puts("C220 whole IF/body foreign array and short/long cardinality "
         "canonical cleanup PASS");
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
    assert(nl_source_create("main()", 6, "C220-OOM-entry", &i->entry_source) ==
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
            printf("C220 %s ordinal=%zu kind=%c pc=%p status=%d\n",
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
        "C220 reached %s OOM=%zu rollback/no-publication/current retry PASS\n",
        invoke ? "invocation" : "registration", reached);
}
static const char original_output[] = "P1123 immutable output\n";
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
    char directory[] = "/tmp/p1123-named-XXXXXX";
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
        int s = p1123_cli_main(4, args);
        injecting = false;
        if (f == SIZE_MAX) {
            assert(!s);
            reached = allocation;
            assert(reached);
        } else {
            const int expected = fault_phase ? 3 : 1;
            printf("C220 named ordinal=%zu kind=%c pc=%p phase=%d "
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
            assert(!p1123_cli_main(4, args));
        }
        if (f != SIZE_MAX && f + 1 == reached)
            break;
    }
    assert(!unlink(output) && !rmdir(directory));
    printf("C220 reached named CLI OOM=%zu exact bytes/inode/mtime/temp/retry "
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
