#define main p1152_cli_main
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
    printf("C228 REGISTER=%d\n", status);
    if (status != NL_CHECK_OK)
        (void)nl_check_diagnostic_render(stderr, a->source,
                                         &diagnostic.diagnostic);
    assert(status == NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C228-actual-entry",
                            &a->entry_source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->entry_source, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(a->entry_parser, &a->entry_tree,
                                               NULL) == NL_PARSE_OK);
    status =
        nl_semantic_check_expression(a->world, a->entry_tree, &a->root, NULL);
    printf("C228 INVOKE=%d\n", status);
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
#ifndef P1152_OOM
static NLCheckedNodeView *binding(Input *a, const char *name)
{
    NLCheckedNodeView *found = NULL;
    for (size_t i = 0; i < a->owner->count; ++i) {
        NLCheckedNodeView *n = &a->owner->nodes[i];
        if (n->kind == NL_CHECKED_BINDING && n->symbol &&
            n->symbol <= a->owner->context->binding_count &&
            !strcmp(a->owner->context->bindings[n->symbol - 1].name, name)) {
            assert(!found);
            found = n;
        }
    }
    assert(found);
    return found;
}
static NLCheckedNodeId conditional(Input *a)
{
    NLCheckedNodeId result = 0;
    for (size_t id = 1; id <= a->owner->count; ++id)
        if (a->owner->nodes[id - 1].kind == NL_CHECKED_IF) {
            assert(!result);
            result = id;
        }
    assert(result);
    return result;
}
static V0Program program(Input *a)
{
    V0Program p = {.unit = nl_semantic_unit_type(a->world),
                   .u8 = nl_semantic_core_type(a->world, NL_TYPE_U8)};
    assert(avs_shape(
        a->owner,
        nl_checked_node_view(a->owner, binding(a, "original")->initializer)
            ->type,
        &p));
    return p;
}
static void current(Input *a, V0Program *p)
{
    const NLCheckedNodeId id = conditional(a);
    NLIfFieldView proof = {0};
    assert(nl_checked_if_field_view(a->owner, id, &proof) &&
           nl_checked_if_field_validate(a->owner, id, &proof) == NL_CHECK_OK);
    NLCheckedNodeView *decision = binding(a, "decision");
    NLCheckedNodeView *comparison = &a->owner->nodes[decision->initializer - 1];
    NLSemanticValueView v;
    assert(nl_semantic_value_view(a->owner->context,
                                  comparison->results[0].value, &v) &&
           !v.scalar_known && v.dependencies == NL_DEPENDENCY_FREE &&
           !v.value_dependency_count);
    V0Program flags = *p;
    assert(!boolean_scalar_at(a->owner, proof.condition, &flags, id));
    flags = *p;
    flags.scalar_loop = false;
    assert(runtime_boolean_scalar_at(a->owner, proof.condition, &flags, id));
    assert(!quantity_scalar(a->owner, binding(a, "copied")->initializer, p));
    for (size_t route = 0; route < 4; ++route) {
        flags = *p;
        if (route == 0)
            flags.u8_export = true;
        if (route == 1)
            flags.scalar_depth = 1;
        if (route == 2)
            flags.formal_body = a->owner;
        if (route == 3)
            flags.unit_parameters = (const NLCopyBodyParameters *)p;
        assert(
            !runtime_boolean_scalar_at(a->owner, proof.condition, &flags, id));
    }
    NLSemanticBindingView b;
    NLSemanticPlaceView root, child, sibling, post_root, post_child, post_other;
    assert(nl_semantic_binding_view(proof.entry, proof.base, &b) &&
           b.availability == NL_AVAILABLE && b.place == proof.root &&
           nl_semantic_place_view(proof.entry, proof.root, &root) &&
           root.live && root.independent_root && root.fixed_field_count == 2 &&
           root.type == p->aggregate && root.incarnation == proof.incarnation &&
           proof.index < 2 && root.fixed_fields[proof.index] == proof.child &&
           root.fixed_fields[1 - proof.index] == proof.sibling &&
           nl_semantic_place_view(proof.entry, proof.child, &child) &&
           nl_semantic_place_view(proof.entry, proof.sibling, &sibling) &&
           child.live && sibling.live &&
           child.incarnation == proof.child_incarnation);
    assert(nl_semantic_place_view(proof.post, proof.root, &post_root) &&
           nl_semantic_place_view(proof.post, proof.child, &post_child) &&
           nl_semantic_place_view(proof.post, proof.sibling, &post_other) &&
           post_root.live && post_child.live && post_other.live &&
           post_root.incarnation == root.incarnation &&
           post_child.incarnation == child.incarnation &&
           post_root.fixed_fields[proof.index] == proof.child &&
           post_root.fixed_fields[1 - proof.index] == proof.sibling &&
           post_other.current_fact == sibling.current_fact &&
           post_other.current_value == sibling.current_value &&
           post_child.current_fact != child.current_fact &&
           nl_semantic_value_view(proof.post, post_child.current_value, &v) &&
           !v.scalar_known && v.type == p->u8 &&
           v.dependencies == NL_DEPENDENCY_FREE);
    for (size_t arm = 0; arm < 2; ++arm) {
        assert(proof.arms[arm].event_count == 1 &&
               proof.arms[arm].change_count == 1);
        NLIfFieldEvent e;
        assert(nl_checked_if_field_event(a->owner, id, arm, 0, &e) &&
               nl_checked_if_field_event_validate(a->owner, id, arm, 0, &e) ==
                   NL_CHECK_OK &&
               e.change && e.field.parent == proof.root &&
               e.field.child == proof.child && e.field.index == proof.index &&
               e.field.parent_incarnation == root.incarnation &&
               e.field.child_incarnation == child.incarnation &&
               e.field.access == NL_ACCESS_WRITE &&
               e.field.parent_fact != e.field.parent_post_fact &&
               e.field.child_fact != e.field.child_post_fact);
        NLSemanticValueView old, incoming;
        assert(nl_semantic_value_view(e.before, e.result, &old) &&
               nl_semantic_value_view(e.before, e.incoming, &incoming) &&
               old.scalar_known && incoming.scalar_known && old.type == p->u8 &&
               incoming.type == p->u8 &&
               old.dependencies == NL_DEPENDENCY_FREE);
    }
    NLCheckedNodeView *read =
        &a->owner->nodes[binding(a, "updated")->initializer - 1];
    NLCheckedNodeView *other =
        &a->owner->nodes[binding(a, "sibling")->initializer - 1];
    assert(read->field.parent == proof.root &&
           read->field.child == proof.child &&
           read->field.child_fact == post_child.current_fact &&
           read->field.parent_fact == post_root.current_fact &&
           other->field.child == proof.sibling &&
           other->field.child_fact == post_other.current_fact);
    assert(nl_semantic_binding_view(a->owner->context,
                                    binding(a, "old_copy")->symbol, &b) &&
           nl_semantic_value_view(a->owner->context, b.value, &v) &&
           v.type == p->u8 && v.dependencies == NL_DEPENDENCY_FREE &&
           !v.value_dependency_count);
    assert(nl_semantic_binding_view(a->owner->context, proof.base, &b) &&
           b.availability == NL_CONSUMED &&
           nl_semantic_place_view(a->owner->context, b.place, &root) &&
           !root.live);
    printf("C228_SYMBOLS {\"original\":%zu,\"result\":%zu,\"copied\":%zu,"
           "\"decision\":%zu,\"old\":%zu,\"old_copy\":%zu,"
           "\"conditional\":%zu,\"field\":%zu,\"entry\":%zu}\n",
           proof.base, binding(a, "result")->symbol,
           binding(a, "copied")->symbol, decision->symbol,
           binding(a, "old")->symbol, binding(a, "old_copy")->symbol, id,
           proof.index,
           nl_checked_node_view(a->root, nl_checked_root(a->root))->function);
    puts("C228 authentic Unknown "
         "loop/Copy/comparison/current/both-arm/join/old/sibling PASS");
}
static void refusals(Input *a, Input *b, V0Program *p)
{
    NLCheckedNodeView *cmp =
        &a->owner->nodes[binding(a, "decision")->initializer - 1];
    NLCheckedNodeView saved = *cmp;
    for (size_t t = 0; t < 9; ++t) {
        if (t == 0)
            cmp->kind = cmp->kind == NL_CHECKED_USIZE_EQUALITY
                            ? NL_CHECKED_USIZE_LESS_THAN
                            : NL_CHECKED_USIZE_EQUALITY;
        if (t == 1)
            cmp->first_argument = 0;
        if (t == 2)
            cmp->argument_count = 1;
        if (t == 3)
            cmp->scalar_result.known = true;
        if (t == 4)
            cmp->results[0].type = p->u8;
        if (t == 5)
            cmp->name.start_byte = cmp->name.end_byte;
        if (t == 6)
            cmp->span.end_byte = cmp->span.start_byte;
        if (t == 7)
            cmp->results[0].value = 0;
        if (t == 8)
            cmp->type = p->unit;
        assert(!valid(a));
        *cmp = saved;
        assert(valid(a));
    }
    NLSemanticContext *world = (NLSemanticContext *)a->owner->context;
    NLSemanticValueView *value = &world->values[cmp->results[0].value - 1];
    const NLSemanticValueView before = *value;
    value->scalar_known = true;
    assert(!valid(a));
    *value = before;
    value->dependencies = NL_DEPENDENCIES_UNKNOWN;
    assert(!valid(a));
    *value = before;
    NLCheckedNodeView *loop = &a->owner->nodes[a->loop - 1];
    saved = *loop;
    for (size_t t = 0; t < 4; ++t) {
        if (t == 0)
            loop->header_inductive = false;
        if (t == 1)
            loop->continue_edges = 0;
        if (t == 2)
            loop->results[0].value = 0;
        if (t == 3)
            loop->first_argument = 0;
        assert(!valid(a));
        *loop = saved;
        assert(valid(a));
    }
    const NLCheckedNodeId id = conditional(a);
    NLIfFieldView proof;
    assert(nl_checked_if_field_view(a->owner, id, &proof));
    for (size_t t = 0; t < 7; ++t) {
        NLIfFieldView bad = proof;
        if (t == 0)
            bad.child = proof.sibling;
        if (t == 1)
            bad.index = 1 - proof.index;
        if (t == 2)
            ++bad.child_incarnation;
        if (t == 3)
            bad.entry = nl_checked_context(b->owner);
        if (t == 4)
            bad.post = proof.entry;
        if (t == 5)
            bad.condition = binding(a, "result")->initializer;
        if (t == 6)
            bad.arms[1] = bad.arms[0];
        assert(nl_checked_if_field_validate(a->owner, id, &bad) != NL_CHECK_OK);
    }
    for (size_t arm = 0; arm < 2; ++arm) {
        NLIfFieldEvent e;
        assert(nl_checked_if_field_event(a->owner, id, arm, 0, &e));
        e.field.access = NL_ACCESS_READ;
        assert(nl_checked_if_field_event_validate(a->owner, id, arm, 0, &e) !=
               NL_CHECK_OK);
        NLCheckedFragment *f = (NLCheckedFragment *)proof.arms[arm].artifact;
        for (size_t i = 0; i < f->count; ++i) {
            NLCheckedNodeView *n = &f->nodes[i];
            if (n->kind == NL_CHECKED_LOAN_HEADER) {
                saved = *n;
                n->loan.body_nonescape_proved = false;
                assert(!valid(a));
                *n = saved;
                n->loan.access = NL_ACCESS_READ;
                assert(!valid(a));
                *n = saved;
                assert(valid(a));
            }
        }
    }
    const NLSource *source = a->owner->source;
    a->owner->source = b->owner->source;
    assert(!valid(a));
    a->owner->source = source;
    const NLSemanticContext *context = a->owner->context;
    a->owner->context = b->owner->context;
    assert(!valid(a));
    a->owner->context = context;
    NLFunctionEntry *functions = world->functions;
    world->functions = NULL;
    assert(!valid(a));
    world->functions = functions;
    const size_t count = world->function_count;
    world->function_count = 0;
    assert(!valid(a));
    world->function_count = count;
    const size_t fid =
        nl_checked_node_view(a->root, nl_checked_root(a->root))->function;
    NLFunctionEntry *fn = &world->functions[fid - 1], original = *fn;
    fn->body = NULL;
    assert(!valid(a));
    *fn = original;
    fn->result = p->u8;
    assert(!valid(a));
    *fn = original;
    assert(valid(a) && valid(b));
    puts("C228 source/operand/knownness/type/world/callable/current/arm "
         "refusals PASS");
}
int main(int argc, char **argv)
{
    assert(argc == 2 || (argc == 3 && !strcmp(argv[2], "--describe")));
    setvbuf(stdout, NULL, _IONBF, 0);
    Input a = {0}, b = {0};
    prepare(&a, argv[1]);
    assert(valid(&a));
    V0Program p = program(&a);
    current(&a, &p);
    if (argc == 3) {
        dispose(&a);
        return 0;
    }
    prepare(&b, argv[1]);
    assert(valid(&b));
    refusals(&a, &b, &p);
    dispose(&a);
    dispose(&b);
    for (size_t t = 0; t < 9; ++t) {
        a = (Input){0};
        b = (Input){0};
        prepare(&a, argv[1]);
        prepare(&b, argv[1]);
        assert(valid(&a) && valid(&b));
        NLCheckedFragment *fa = t < 3 ? a.graph : a.owner;
        NLCheckedFragment *fb = t < 3 ? b.graph : b.owner;
        if (t == 0 || t == 3)
            fa->arm_count = 0;
        if (t == 1 || t == 4)
            fa->arm_count = SIZE_MAX;
        if (t == 2 || t == 5) {
            fa->arms = fb->arms;
            fa->arm_count = fb->arm_count;
        }
        if (t == 6)
            a.root->body_count = 0;
        if (t == 7)
            a.root->body_count = SIZE_MAX;
        if (t == 8) {
            a.root->bodies = b.root->bodies;
            a.root->body_count = b.root->body_count;
        }
        assert(!valid(&a) && valid(&b));
        dispose(&a);
        assert(valid(&b));
        dispose(&b);
    }
    puts("C228 canonical A destruction / B survives loop-and-T-arm/body-count "
         "PASS");
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
    assert(nl_source_create("main()", 6, "C228-OOM-entry", &i->entry_source) ==
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
            printf("C228 %s ordinal=%zu kind=%c pc=%p status=%d\n",
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
        "C228 reached %s OOM=%zu rollback/no-publication/current retry PASS\n",
        invoke ? "invocation" : "registration", reached);
}
static const char original_output[] = "P1152 immutable output\n";
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
    char directory[] = "/tmp/p1152-named-XXXXXX";
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
        int s = p1152_cli_main(4, args);
        injecting = false;
        if (f == SIZE_MAX) {
            assert(!s);
            reached = allocation;
            assert(reached);
        } else {
            const int expected = fault_phase ? 3 : 1;
            printf("C228 named ordinal=%zu kind=%c pc=%p phase=%d "
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
            assert(!p1152_cli_main(4, args));
        }
        if (f != SIZE_MAX && f + 1 == reached)
            break;
    }
    assert(!unlink(output) && !rmdir(directory));
    printf("C228 reached named CLI OOM=%zu exact bytes/inode/mtime/temp/retry "
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
