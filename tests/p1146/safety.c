#define main p1146_cli_main
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
    printf("C226 REGISTER=%d\n", status);
    if (status != NL_CHECK_OK)
        (void)nl_check_diagnostic_render(stderr, a->source,
                                         &diagnostic.diagnostic);
    assert(status == NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C226-actual-entry",
                            &a->entry_source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->entry_source, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(a->entry_parser, &a->entry_tree,
                                               NULL) == NL_PARSE_OK);
    status =
        nl_semantic_check_expression(a->world, a->entry_tree, &a->root, NULL);
    printf("C226 INVOKE=%d\n", status);
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
#ifndef P1146_OOM
static NLCheckedNodeView *named_binding(Input *a, const char *name)
{
    NLCheckedNodeView *found = NULL;
    for (size_t i = 0; i < a->owner->count; ++i) {
        NLCheckedNodeView *n = &a->owner->nodes[i];
        if (n->kind != NL_CHECKED_BINDING || !n->symbol)
            continue;
        assert(n->symbol <= a->owner->context->binding_count);
        const char *actual = a->owner->context->bindings[n->symbol - 1].name;
        if (actual && !strcmp(actual, name)) {
            assert(!found);
            found = n;
        }
    }
    assert(found);
    return found;
}
static NLCheckedNodeView *single_node(Input *a, NLCheckedKind kind)
{
    NLCheckedNodeView *found = NULL;
    for (size_t i = 0; i < a->owner->count; ++i)
        if (a->owner->nodes[i].kind == kind) {
            assert(!found);
            found = &a->owner->nodes[i];
        }
    assert(found);
    return found;
}
static void composed_current(Input *a, V0Program *p, NLCheckedNodeId call_id)
{
    NLCheckedNodeView *original = named_binding(a, "original");
    NLCheckedNodeView *incoming = named_binding(a, "incoming");
    NLCheckedNodeView *old = named_binding(a, "old");
    NLCheckedNodeView *updated = named_binding(a, "updated");
    NLCheckedNodeView *sibling = named_binding(a, "sibling");
    NLCheckedNodeView *old_again = named_binding(a, "old_again");
    NLCheckedNodeView *change = single_node(a, NL_CHECKED_REPLACE);
    NLCheckedNodeView *loan = single_node(a, NL_CHECKED_LOAN_HEADER);
    const NLCheckedNodeView *read =
        nl_checked_node_view(a->owner, updated->initializer);
    const NLCheckedNodeView *other =
        nl_checked_node_view(a->owner, sibling->initializer);
    assert(read && other);
    /* The final caller world is after main's RETURN and teardown. The
     * validated loop-owned frame is the live original epoch; later field
     * views carry the actual issued Change/read facts, not new authority. */
    const NLSemanticContext *live = nl_checked_context(a->graph);
    NLSemanticBindingView binding;
    NLSemanticPlaceView root, child, neighbor;
    assert(nl_semantic_binding_view(live, original->symbol, &binding));
    assert(binding.availability == NL_AVAILABLE);
    assert(nl_semantic_place_view(live, binding.place, &root));
    assert(root.live && root.independent_root && root.type == p->aggregate &&
           root.fixed_field_count == 2 &&
           root.fixed_fields[0] != root.fixed_fields[1]);
    const size_t index = change->field.index;
    assert(index < 2 &&
           nl_semantic_place_view(live, root.fixed_fields[index], &child));
    assert(
        nl_semantic_place_view(live, root.fixed_fields[1 - index], &neighbor));
    assert(child.live && neighbor.live &&
           change->field.base == original->symbol &&
           change->field.parent == binding.place &&
           change->field.child == root.fixed_fields[index] &&
           change->field.parent_incarnation == root.incarnation &&
           change->field.child_incarnation == child.incarnation &&
           neighbor.parent_aggregate == binding.place &&
           neighbor.parent_field_index == 1 - index);
    assert(change->field.old_value == change->results[0].value &&
           change->field.parent_fact == root.current_fact &&
           change->field.child_fact == child.current_fact &&
           change->field.parent_fact != change->field.parent_post_fact &&
           change->field.child_fact != change->field.child_post_fact);
    assert(loan->loan.source == original->symbol &&
           loan->loan.place == change->field.child &&
           loan->loan.incarnation == child.incarnation &&
           loan->loan.access == NL_ACCESS_WRITE &&
           loan->loan.body_nonescape_proved &&
           loan->loan.normal_result_forwarded);
    assert(read->kind == NL_CHECKED_FIELD_READ &&
           other->kind == NL_CHECKED_FIELD_READ &&
           read->field.parent == binding.place &&
           other->field.parent == binding.place &&
           read->field.child == root.fixed_fields[index] &&
           other->field.child == root.fixed_fields[1 - index] &&
           read->field.parent_incarnation == root.incarnation &&
           other->field.parent_incarnation == root.incarnation &&
           read->field.child_incarnation == child.incarnation &&
           other->field.child_incarnation == neighbor.incarnation);
    assert(read->field.parent_fact == change->field.parent_post_fact &&
           read->field.child_fact == change->field.child_post_fact &&
           other->field.parent_fact == change->field.parent_post_fact &&
           other->field.child_fact == neighbor.current_fact);
    NLSemanticBindingView ended_binding;
    NLSemanticPlaceView ended_root;
    assert(nl_semantic_binding_view(a->owner->context, original->symbol,
                                    &ended_binding) &&
           ended_binding.availability == NL_CONSUMED &&
           nl_semantic_place_view(a->owner->context, ended_binding.place,
                                  &ended_root) &&
           !ended_root.live);
    assert(nl_semantic_binding_view(a->owner->context, old->symbol, &binding));
    NLSemanticValueView copy;
    assert(nl_semantic_value_view(a->owner->context, binding.value, &copy) &&
           copy.type == p->u8 && copy.dependencies == NL_DEPENDENCY_FREE &&
           !copy.value_dependency_count);
    const NLCheckedNodeView *call = nl_checked_node_view(a->owner, call_id);
    assert(call);
    NLSymbolId formal = 0;
    assert(v1_call_parameter_kind(a->owner, call_id, p, &formal, true));
    V0Program flags = *p;
    flags.scalar_loop = false;
    assert(predicate_original_frame(a->owner, call_id, &flags));
    flags = *p;
    flags.aggregate = p->aggregate + 1;
    assert(!predicate_original_frame(a->owner, call_id, &flags));
    printf("C226_SYMBOLS {\"original\":%zu,\"amount\":%zu,\"argument\":%zu,"
           "\"incoming\":%zu,\"old\":%zu,\"updated\":%zu,\"sibling\":%zu,"
           "\"old_again\":%zu,\"call\":%zu,\"function\":%zu,\"formal\":%zu,"
           "\"field\":%zu,\"ref\":%zu,\"entry\":%zu}\n",
           original->symbol, named_binding(a, "amount")->symbol,
           named_binding(a, "argument")->symbol, incoming->symbol, old->symbol,
           updated->symbol, sibling->symbol, old_again->symbol, call_id,
           call->function, formal, index, loan->loan.ref_symbol,
           nl_checked_node_view(a->root, nl_checked_root(a->root))->function);
    puts("C226 genuine pre-loop T/frame and postcall original/Change/old "
         "Copy/current/sibling PASS");
}
static void composed_refusals(Input *a, V0Program *p)
{
    NLCheckedNodeView *change = single_node(a, NL_CHECKED_REPLACE);
    NLCheckedNodeView saved = *change;
    for (size_t n = 0; n < 7; ++n) {
        if (n == 0)
            change->field.index = 1 - change->field.index;
        if (n == 1)
            change->field.parent = change->field.child;
        if (n == 2)
            ++change->field.child_incarnation;
        if (n == 3)
            change->field.access = NL_ACCESS_READ;
        if (n == 4)
            change->field.parent_post_fact = change->field.parent_fact;
        if (n == 5)
            change->field.child_post_fact = change->field.child_fact;
        if (n == 6)
            change->field.nominal = p->u8;
        assert(!valid(a));
        *change = saved;
        assert(valid(a));
    }
    NLCheckedNodeView *loan = single_node(a, NL_CHECKED_LOAN_HEADER);
    saved = *loan;
    loan->loan.body_nonescape_proved = false;
    assert(!valid(a));
    *loan = saved;
    loan->loan.access = NL_ACCESS_READ;
    assert(!valid(a));
    *loan = saved;
    NLCheckedNodeView *read =
        &a->owner->nodes[named_binding(a, "updated")->initializer - 1];
    saved = *read;
    read->field.index = 1 - read->field.index;
    assert(!valid(a));
    *read = saved;
    NLSemanticContext *world = (NLSemanticContext *)a->owner->context;
    NLCheckedNodeView *call = single_node(a, NL_CHECKED_REGISTERED_CALL);
    NLFunctionEntry *fn = &world->functions[call->function - 1], before = *fn;
    fn->caller_effects = true;
    assert(!valid(a));
    *fn = before;
    fn->result = p->unit;
    assert(!valid(a));
    *fn = before;
    fn->body = NULL;
    assert(!valid(a));
    *fn = before;
    size_t count = world->function_count;
    world->function_count = 0;
    assert(!valid(a));
    world->function_count = count;
    assert(valid(a));
    puts("C226 "
         "nominal/root/field/incarnation/current/mode/ending-scope/callable "
         "refusals PASS");
}

int main(int argc, char **argv)
{
    assert(argc == 2 || (argc == 3 && !strcmp(argv[2], "--describe")));
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
    assert(avs_shape(a.owner,
                     nl_checked_node_view(
                         a.owner, named_binding(&a, "original")->initializer)
                         ->type,
                     &p));
    size_t calls = 0;
    NLCheckedNodeId call_id = 0;
    NLCheckedNodeView *call = NULL;
    for (size_t id = 1; id <= a.owner->count; ++id)
        if (a.owner->nodes[id - 1].kind == NL_CHECKED_REGISTERED_CALL) {
            call_id = id;
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
    composed_current(&a, &p, call_id);
    if (argc == 3) {
        dispose(&a);
        return 0;
    }
    composed_refusals(&a, &p);
    puts("C226 authentic Unknown loop result -> Copy -> actual/formal -> "
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
    puts("C226 wrong-result/target/source/consumer/type/current/dependency "
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
    puts("C226 canonical A destruction / B survives PASS");
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
    assert(nl_source_create("main()", 6, "C226-OOM-entry", &i->entry_source) ==
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
            printf("C226 %s ordinal=%zu kind=%c pc=%p status=%d\n",
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
        "C226 reached %s OOM=%zu rollback/no-publication/current retry PASS\n",
        invoke ? "invocation" : "registration", reached);
}
static const char original_output[] = "P1146 immutable output\n";
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
    char directory[] = "/tmp/p1146-named-XXXXXX";
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
        int s = p1146_cli_main(4, args);
        injecting = false;
        if (f == SIZE_MAX) {
            assert(!s);
            reached = allocation;
            assert(reached);
        } else {
            const int expected = fault_phase ? 3 : 1;
            printf("C226 named ordinal=%zu kind=%c pc=%p phase=%d "
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
            assert(!p1146_cli_main(4, args));
        }
        if (f != SIZE_MAX && f + 1 == reached)
            break;
    }
    assert(!unlink(output) && !rmdir(directory));
    printf("C226 reached named CLI OOM=%zu exact bytes/inode/mtime/temp/retry "
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
