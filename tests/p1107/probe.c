#define main p1107_driver_main
#include "../../src/main.c"
#undef main
#include "../../src/copy_body_internal.h"
#include "../../src/h_origin_internal.h"
#include <assert.h>
#include <dirent.h>

#define REQUIRE(c)                                                             \
    do {                                                                       \
        if (!(c)) {                                                            \
            fprintf(stderr, "P1107 FIRST RED %d: %s\n", __LINE__, #c);         \
            return false;                                                      \
        }                                                                      \
    } while (0)
typedef struct {
    NLSource *source, *spelling;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *tree;
    NLSemanticContext *context;
    NLCheckedFragment *root;
    NLCheckedFragment *owner, *body;
    NLCheckedNodeId loop;
} Input;
static void destroy(Input *i)
{
    nl_checked_destroy(i->root);
    nl_semantic_destroy(i->context);
    nl_syntax_tree_destroy(i->tree);
    nl_parser_destroy(i->entry_parser);
    nl_source_destroy(i->spelling);
    nl_syntax_tree_destroy(i->unit);
    nl_parser_destroy(i->parser);
    nl_source_destroy(i->source);
    *i = (Input){0};
}
static bool entry(Input *i)
{
    REQUIRE(nl_source_create("main()", 6, "P1107-entry", &i->spelling) ==
            NL_SOURCE_OK);
    REQUIRE(nl_parser_create(i->spelling, &i->entry_parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(i->entry_parser, &i->tree,
                                                NULL) == NL_PARSE_OK);
    return true;
}
static bool prepare(const char *path, Input *i)
{
    REQUIRE(nl_source_load(path, &i->source) == NL_SOURCE_OK);
    REQUIRE(nl_parser_create(i->source, &i->parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_function_unit(i->parser, &i->unit, NULL) ==
            NL_PARSE_OK);
    REQUIRE(nl_semantic_create(&i->context) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {i->unit};
    NLCheckStatus registered =
        nl_semantic_register_function_unit(i->context, units, 1, NULL);
    printf("P1107 actual registration status=%d\n", (int)registered);
    REQUIRE(registered == NL_CHECK_OK);
    return entry(i);
}
static bool checked(Input *i)
{
    REQUIRE(nl_semantic_check_expression(i->context, i->tree, &i->root, NULL) ==
            NL_CHECK_OK);
    i->owner = (NLCheckedFragment *)nl_checked_call_body(
        i->root, nl_checked_root(i->root));
    REQUIRE(i->owner);
    for (size_t n = 0; n < i->owner->count; ++n)
        if (i->owner->nodes[n].kind == NL_CHECKED_LOOP) {
            REQUIRE(!i->loop);
            i->loop = n + 1;
        }
    REQUIRE(i->loop);
    i->body = (NLCheckedFragment *)nl_checked_loop_body(i->owner, i->loop);
    REQUIRE(i->body);
    return true;
}
static NLCheckedNodeId copy_call(const NLCheckedFragment *f)
{
    NLCheckedNodeId call = 0;
    for (size_t n = 0; n < f->count; ++n)
        if (f->nodes[n].kind == NL_CHECKED_REGISTERED_CALL) {
            if (call)
                return 0;
            call = n + 1;
        }
    return call;
}
static bool public_copy(const NLCheckedFragment *f, NLCheckedNodeId call)
{
    NLCopyBodyView v;
    NLCopyBodyParameters a;
    NLCopyBodyEventView event;
    REQUIRE(call && nl_checked_copy_body_view(f, call, &v));
    REQUIRE(nl_checked_copy_body_parameters(f, call, &a));
    REQUIRE(nl_checked_copy_body_validate(f, call, &v) == NL_CHECK_OK);
    REQUIRE(nl_checked_copy_body_parameters_validate(f, call, &a) ==
            NL_CHECK_OK);
    REQUIRE(v.event_count == 1 &&
            nl_checked_copy_body_event(f, call, 0, &event));
    REQUIRE(event.kind == NL_COPY_BODY_CHANGE);
    REQUIRE(nl_checked_copy_body_event_validate(f, call, 0, &event) ==
            NL_CHECK_OK);
    REQUIRE(event.field.parent == a.field.parent &&
            event.field.child == a.field.child);
    printf("P1107 graph seal nodes=%zu node_bytes=%zu snapshot_bytes=%zu "
           "private_view_bytes=%zu\n",
           f->count, sizeof(NLCheckedNodeView),
           f->count * sizeof(NLCheckedNodeView), sizeof(NLLinearBreakView));
    printf("P1107 genuine public GET/current call=%zu root=%zu child=%zu "
           "event=%zu\n",
           call, a.field.parent, a.field.child, event.node);
    return true;
}
static bool outside(const char *path)
{
    Input i = {0};
    REQUIRE(prepare(path, &i));
    REQUIRE(nl_semantic_check_expression(i.context, i.tree, &i.root, NULL) ==
            NL_CHECK_OK);
    const NLCheckedFragment *owner =
        nl_checked_call_body(i.root, nl_checked_root(i.root));
    REQUIRE(owner && public_copy(owner, copy_call(owner)));
    destroy(&i);
    return true;
}
static NLTypeId observed_u8;
static V0Program program(const Input *i)
{
    return (V0Program){.unit = nl_semantic_unit_type(i->context),
                       .u8 = observed_u8};
}
static bool positive(Input *i, size_t index, size_t old, size_t next)
{
    observed_u8 = nl_semantic_core_type(i->context, NL_TYPE_U8);
    NLLinearBreakView v = {0};
    REQUIRE(nl_h_custody_validate(i->root));
    REQUIRE(nl_checked_loop_break_provenance_valid(i->owner, i->loop));
    REQUIRE(nl_linear_break_validate(i->owner, i->loop, &v) == NL_CHECK_OK);
    REQUIRE(v.index == index && v.incoming == next && v.root && v.child &&
            v.base);
    REQUIRE(v.result.value && v.result.value <= i->context->value_count);
    REQUIRE(i->context->values[v.result.value - 1].scalar_known);
    REQUIRE(i->context->values[v.result.value - 1].scalar_value == old);
    REQUIRE(public_copy(i->body, copy_call(i->body)));
    V0Program p = program(i);
    REQUIRE(v0_validate_call(i->root, nl_checked_root(i->root), &p));
    REQUIRE(p.linear_break && p.aggregate == v.nominal);
    FILE *out = tmpfile();
    REQUIRE(out);
    const bool ok = v0_emit_c(out, &p) && fflush(out) == 0 && ftell(out) > 0;
    REQUIRE(fclose(out) == 0 && ok);
    return true;
}
#ifndef P1107_OOM
static size_t negatives;
static bool rejected(Input *i, const char *name)
{
    NLLinearBreakView v = {.base = 987654};
    REQUIRE(nl_linear_break_validate(i->owner, i->loop, &v) ==
            NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    REQUIRE(v.base == 987654);
    V0Program p = program(i);
    REQUIRE(!v0_validate_call(i->root, nl_checked_root(i->root), &p));
    printf("P1107 refusal %zu %s\n", ++negatives, name);
    return true;
}
#define MUTATE(type, object, member, value)                                    \
    do {                                                                       \
        type saved = (object)->member;                                         \
        REQUIRE(saved != (value));                                             \
        (object)->member = (value);                                            \
        bool refused = rejected(&a, #object "." #member);                      \
        (object)->member = saved;                                              \
        REQUIRE(refused);                                                      \
    } while (0)
static bool observe(const char *path, size_t index, size_t old, size_t next)
{
    Input a = {0}, b = {0};
    REQUIRE(prepare(path, &a));
    REQUIRE(nl_sem_clone(a.context, &b.context) == NL_CHECK_OK && entry(&b));
    REQUIRE(checked(&a) && checked(&b));
    REQUIRE(positive(&a, index, old, next) && positive(&b, index, old, next));
    NLCheckedNodeId call = copy_call(a.body), foreign_call = copy_call(b.body);
    REQUIRE(call && foreign_call);
    NLCheckedNodeView *node = &a.body->nodes[call - 1];
    NLCopyBodyView view;
    NLCopyBodyParameters params;
    REQUIRE(nl_checked_copy_body_view(a.body, call, &view));
    REQUIRE(nl_checked_copy_body_parameters(a.body, call, &params));
    NLCopyBodyParameters wrong = params;
    wrong.field.index = 1 - index;
    REQUIRE(nl_checked_copy_body_parameters_validate(a.body, call, &wrong) ==
            NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    wrong = params;
    wrong.field.parent_incarnation++;
    REQUIRE(nl_checked_copy_body_parameters_validate(a.body, call, &wrong) ==
            NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    NLCopyBodyView wrong_view = view;
    wrong_view.body = nl_checked_call_body(b.body, foreign_call);
    REQUIRE(nl_checked_copy_body_validate(a.body, call, &wrong_view) ==
            NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    NLCheckedNodeView *loop = &a.owner->nodes[a.loop - 1];
    MUTATE(size_t, node, function, 0);
    MUTATE(size_t, node, function, a.context->function_count + 1);
    MUTATE(size_t, node, results[0].value, 0);
    MUTATE(bool, node, body_backed, false);
    MUTATE(size_t, loop, break_edges, 2);
    MUTATE(size_t, loop, continue_edges, 1);
    MUTATE(bool, loop, header_inductive, false);
    MUTATE(size_t, a.body, count, a.body->count + 1);
    MUTATE(size_t, a.body, body_count, 0);
    MUTATE(size_t, a.body, body_count, a.body->body_count + 1);
    MUTATE(size_t, a.owner, arm_count, 0);
    MUTATE(size_t, a.owner, arm_count, a.owner->arm_count + 1);
    MUTATE(const NLSource *, a.body, source, b.spelling);
    MUTATE(const NLSemanticContext *, a.body, context, b.body->context);
    NLCheckedNodeView *loan = &a.body->nodes[params.loan - 1];
    MUTATE(size_t, loan, loan.scope, loan->loan.scope + 1);
    MUTATE(bool, loan, loan.body_nonescape_proved, false);
    MUTATE(bool, loan, loan.normal_result_forwarded, false);
    NLFunctionEntry *outer_fn = &a.context->functions[node->function - 1];
    MUTATE(NLFunctionBody *, outer_fn, body, a.owner->body_owner);
    MUTATE(size_t, outer_fn, result, 1);
    MUTATE(NLCheckedKind, outer_fn, kind, NL_CHECKED_REPLACE);
    MUTATE(bool, outer_fn, caller_effects, true);
    MUTATE(bool, outer_fn, hidden_dependencies, true);
    MUTATE(bool, outer_fn, owner_receiver, true);
    MUTATE(bool, outer_fn, owner_producer, true);
    MUTATE(bool, outer_fn, custody_recipient, true);
    MUTATE(size_t, outer_fn, count, 0);
    MUTATE(NLTypeId *, outer_fn, parameters, NULL);
    NLFunctionBody *plan = outer_fn->body;
    MUTATE(NLSyntaxTree *, plan, syntax, b.tree);
    MUTATE(NLSource *, plan, source, b.spelling);
    MUTATE(size_t, plan, count, 0);
    MUTATE(char *, plan, parameter_names[0], NULL);
    NLSemanticContext *world = (NLSemanticContext *)a.body->context;
    REQUIRE(world->functions && node->function &&
            node->function <= world->function_count);
    NLFunctionEntry *fn = &world->functions[node->function - 1];
    MUTATE(size_t, fn, result, 1);
    MUTATE(NLCheckedKind, fn, kind, NL_CHECKED_REPLACE);
    MUTATE(bool, fn, caller_effects, true);
    MUTATE(bool, fn, hidden_dependencies, true);
    MUTATE(bool, fn, owner_receiver, true);
    MUTATE(bool, fn, owner_producer, true);
    MUTATE(bool, fn, custody_recipient, true);
    MUTATE(size_t, fn, count, 0);
    NLFunctionBody *saved_body = fn->body;
    fn->body = a.owner->body_owner;
    REQUIRE(nl_checked_copy_body_view(a.body, call,
                                      &wrong_view)); /* historical GET */
    bool refused =
        rejected(&a, "changed current callable; historical GET remains");
    REQUIRE(nl_checked_copy_body_validate(a.body, call, &view) ==
            NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    fn->body = saved_body;
    REQUIRE(refused);
    MUTATE(NLFunctionEntry *, world, functions, NULL);
    MUTATE(size_t, world, function_count, 0);
    NLControlExitView *edge =
        (NLControlExitView *)nl_control_exit_view(a.body->exits, 0);
    const NLControlExitView *other = nl_control_exit_view(b.body->exits, 0);
    REQUIRE(edge && other && edge->state != other->state);
    MUTATE(const NLControlState *, edge, state, other->state);
    MUTATE(const NLControlTarget *, edge, target, other->target);
    MUTATE(const NLCheckedFragment *, edge, source_owner, b.body);
    NLCheckedFragment shallow = *a.owner;
    NLLinearBreakView v;
    REQUIRE(nl_linear_break_validate(&shallow, a.loop, &v) ==
            NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    REQUIRE(positive(&a, index, old, next));
    size_t slot = 0;
    while (slot < a.owner->arm_count && a.owner->arms[slot].match != a.loop)
        ++slot;
    REQUIRE(slot < a.owner->arm_count && a.body->body_count == 1 &&
            b.body->body_count == 1);
    a.body->bodies[0] =
        b.body->bodies[0]; /* genuine foreign child, never restored */
    a.owner->arms[slot].artifact = b.body;
    REQUIRE(rejected(&a, "genuine foreign callee and entire LOOP body"));
    a.body->body_count = 0;
    a.owner->arm_count = 0;
    a.root->body_count++;
    destroy(&a);
    REQUIRE(positive(&b, index, old, next));
    destroy(&b);
    printf("P1107 COMPLETE negatives=%zu foreign B survived canonical A "
           "destruction\n",
           negatives);
    return true;
}

#else
static bool injecting;
static size_t allocation, fail_at;
static char reached;
static void *pc;
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
/* Observe actual API boundary/status, without replacing or replaying it. The
 * existing CLI intentionally renders parser/register/CHECK errors as exit3. */
static int phase, fault_phase, fault_status;
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
    int saved = phase;
    phase = 1;
    NLParseStatus status = __real_nl_parser_parse_function_unit(p, t, d);
    if (reached && fault_phase == 1)
        fault_status = (int)status;
    phase = saved;
    return status;
}
NLParseStatus __wrap_nl_parser_parse_expression_fragment(NLParser *p,
                                                         NLSyntaxTree **t,
                                                         NLParseDiagnostic *d)
{
    int saved = phase;
    phase = 1;
    NLParseStatus status = __real_nl_parser_parse_expression_fragment(p, t, d);
    if (reached && fault_phase == 1)
        fault_status = (int)status;
    phase = saved;
    return status;
}
NLCheckStatus __wrap_nl_semantic_register_function_unit(
    NLSemanticContext *c, const NLSyntaxTree *const *inputs, size_t count,
    NLFunctionUnitDiagnostic *d)
{
    int saved = phase;
    phase = 2;
    NLCheckStatus status =
        __real_nl_semantic_register_function_unit(c, inputs, count, d);
    if (reached && fault_phase == 2)
        fault_status = (int)status;
    phase = saved;
    return status;
}
NLCheckStatus __wrap_nl_semantic_check_expression(NLSemanticContext *c,
                                                  const NLSyntaxTree *t,
                                                  NLCheckedFragment **f,
                                                  NLCheckDiagnostic *d)
{
    int saved = phase;
    phase = 3;
    NLCheckStatus status = __real_nl_semantic_check_expression(c, t, f, d);
    if (reached && fault_phase == 3)
        fault_status = (int)status;
    phase = saved;
    return status;
}
static bool denied(char kind, void *caller)
{
    if (!injecting || allocation++ != fail_at)
        return false;
    reached = kind;
    fault_phase = phase;
    pc = caller;
    return true;
}
void *__wrap_malloc(size_t n)
{
    return denied('m', __builtin_return_address(0)) ? NULL : __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    return denied('r', __builtin_return_address(0)) ? NULL
                                                    : __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t z)
{
    return denied('c', __builtin_return_address(0)) ? NULL
                                                    : __real_calloc(n, z);
}
static bool named_bytes(const char *path, ino_t inode)
{
    static const char sentinel[] = "P1107 OOM named output must survive\n";
    struct stat st;
    REQUIRE(stat(path, &st) == 0 && st.st_ino == inode);
    FILE *f = fopen(path, "rb");
    REQUIRE(f);
    char bytes[sizeof(sentinel)];
    size_t n = fread(bytes, 1, sizeof(bytes), f);
    const bool ok = n == sizeof(sentinel) - 1 &&
                    !memcmp(bytes, sentinel, sizeof(sentinel) - 1) &&
                    !ferror(f);
    REQUIRE(fclose(f) == 0 && ok);
    return true;
}
static bool named_oom(const char *source)
{
    char dir[] = "/tmp/p1107-named-XXXXXX", path[128];
    REQUIRE(mkdtemp(dir));
    REQUIRE(snprintf(path, sizeof(path), "%s/out.c", dir) > 0);
    char *args[] = {"newlangc", "--emit-c17-to", path, (char *)source, NULL};
    for (fail_at = 0; fail_at < 20000; ++fail_at) {
        FILE *f = fopen(path, "wb");
        REQUIRE(f && fputs("P1107 OOM named output must survive\n", f) >= 0);
        REQUIRE(fclose(f) == 0);
        struct stat before;
        REQUIRE(stat(path, &before) == 0);
        allocation = 0;
        reached = 0;
        pc = NULL;
        phase = 0;
        fault_phase = 0;
        fault_status = -1;
        injecting = true;
        int status = p1107_driver_main(4, args);
        injecting = false;
        if (!reached) {
            REQUIRE(status == 0);
            REQUIRE(unlink(path) == 0 && rmdir(dir) == 0);
            printf("P1107 NAMED OOM COMPLETE reached=%zu byte/inode/no-temp + "
                   "retry PASS\n",
                   fail_at);
            return true;
        }
        printf("P1107 NAMED OOM n=%zu kind=%c pc=%p status=%d\n", fail_at,
               reached, pc, status);
        const int expected = fault_phase ? 3 : 1;
        printf("P1107 NAMED phase=%d api_status=%d expected_cli=%d\n",
               fault_phase, fault_status, expected);
        REQUIRE(status == expected && named_bytes(path, before.st_ino));
        REQUIRE(!fault_phase ||
                fault_status == (fault_phase == 1
                                     ? (int)NL_PARSE_OUT_OF_MEMORY
                                     : (int)NL_CHECK_OUT_OF_MEMORY));
        DIR *d = opendir(dir);
        REQUIRE(d);
        size_t count = 0;
        for (struct dirent *e = readdir(d); e; e = readdir(d))
            if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
                REQUIRE(!strcmp(e->d_name, "out.c"));
                ++count;
            }
        REQUIRE(closedir(d) == 0 && count == 1);
        REQUIRE(p1107_driver_main(4, args) == 0);
    }
    return false;
}
static bool observe(const char *path, size_t index, size_t old, size_t next)
{
    Input input = {0};
    REQUIRE(prepare(path, &input));
    for (fail_at = 0; fail_at < 20000; ++fail_at) {
        Input i = {.tree = input.tree}; /* tree borrowed, not destroyed below */
        REQUIRE(nl_sem_clone(input.context, &i.context) == NL_CHECK_OK);
        NLCopyCallAnchor before = {0};
        REQUIRE(nl_copy_state_anchor(i.context, &before) == NL_CHECK_OK);
        allocation = 0;
        reached = 0;
        pc = NULL;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_check_expression(i.context, i.tree, &i.root, NULL);
        injecting = false;
        const bool unchanged = nl_copy_state_matches(i.context, &before);
        nl_copy_state_destroy(&before);
        if (!reached) {
            REQUIRE(status == NL_CHECK_OK && i.root);
            i.owner = (NLCheckedFragment *)nl_checked_call_body(
                i.root, nl_checked_root(i.root));
            for (size_t n = 0; n < i.owner->count; ++n)
                if (i.owner->nodes[n].kind == NL_CHECKED_LOOP)
                    i.loop = n + 1;
            i.body = (NLCheckedFragment *)nl_checked_loop_body(i.owner, i.loop);
            REQUIRE(positive(&i, index, old, next));
            nl_checked_destroy(i.root);
            nl_semantic_destroy(i.context);
            destroy(&input);
            printf("P1107 OOM COMPLETE reached=%zu rollback+no-publish+retry "
                   "PASS\n",
                   fail_at);
            return named_oom(path);
        }
        printf("P1107 OOM n=%zu kind=%c pc=%p status=%d rollback=%d "
               "no-publish=%d\n",
               fail_at, reached, pc, status, unchanged, !i.root);
        REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && unchanged && !i.root);
        REQUIRE(checked(&i) && positive(&i, index, old, next));
        nl_checked_destroy(i.root);
        nl_semantic_destroy(i.context);
    }
    destroy(&input);
    return false;
}
#endif
int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc == 3 && !strcmp(argv[1], "--outside"))
        return outside(argv[2]) ? 0 : 1;
    if (argc != 5)
        return 2;
    return observe(argv[1], (size_t)strtoul(argv[2], NULL, 10),
                   (size_t)strtoul(argv[3], NULL, 10),
                   (size_t)strtoul(argv[4], NULL, 10))
               ? 0
               : 1;
}
