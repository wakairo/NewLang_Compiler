#define main p1158_cli_main
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
    printf("C232 REGISTER=%d\n", status);
    if (status != NL_CHECK_OK)
        (void)nl_check_diagnostic_render(stderr, a->source,
                                         &diagnostic.diagnostic);
    assert(status == NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C232-actual-entry",
                            &a->entry_source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->entry_source, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(a->entry_parser, &a->entry_tree,
                                               NULL) == NL_PARSE_OK);
    status =
        nl_semantic_check_expression(a->world, a->entry_tree, &a->root, NULL);
    printf("C232 INVOKE=%d\n", status);
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
    const size_t id = conditional(a);
    NLIfReturnView v;
    assert(nl_checked_if_return_view(a->owner, id, &v) &&
           nl_checked_if_return_validate(a->owner, id, &v) == NL_CHECK_OK);
    assert(v.normal_arm < 2 && v.replay_events == 1 && v.replay_changes == 1 &&
           v.field.arms[v.normal_arm].event_count == 1 &&
           v.field.arms[1 - v.normal_arm].event_count == 0);
    assert(v.returned.type == p->unit && !v.returned.value &&
           v.return_live != v.field.post && v.return_ended != v.field.post);
    V0Program flags = *p;
    assert(!boolean_scalar_at(a->owner, v.field.condition, &flags, id));
    flags = *p;
    assert(runtime_boolean_scalar_at(a->owner, v.field.condition, &flags, id));
    assert(!quantity_scalar(a->owner, binding(a, "copied")->initializer, p));
    NLSemanticValueView condition;
    const NLCheckedNodeView *node =
        nl_checked_node_view(a->owner, v.field.condition);
    assert(nl_semantic_value_view(a->owner->context, node->results[0].value,
                                  &condition) &&
           !condition.scalar_known &&
           condition.dependencies == NL_DEPENDENCY_FREE);
    NLSemanticPlaceView entry, post, live, ended, child, after, sibling, other;
    assert(nl_semantic_place_view(v.field.entry, v.field.root, &entry) &&
           entry.live &&
           nl_semantic_place_view(v.field.post, v.field.root, &post) &&
           post.live &&
           nl_semantic_place_view(v.return_live, v.field.root, &live) &&
           live.live &&
           nl_semantic_place_view(v.return_ended, v.field.root, &ended) &&
           !ended.live);
    assert(entry.incarnation == post.incarnation &&
           entry.incarnation == live.incarnation &&
           entry.incarnation == ended.incarnation &&
           live.current_fact == entry.current_fact &&
           post.current_fact != live.current_fact &&
           entry.fixed_field_count == 2 &&
           entry.fixed_fields[v.field.index] == v.field.child &&
           entry.fixed_fields[1 - v.field.index] == v.field.sibling);
    assert(nl_semantic_place_view(v.field.entry, v.field.child, &child) &&
           nl_semantic_place_view(v.field.post, v.field.child, &after) &&
           nl_semantic_place_view(v.field.entry, v.field.sibling, &sibling) &&
           nl_semantic_place_view(v.field.post, v.field.sibling, &other) &&
           child.incarnation == after.incarnation && after.live && other.live &&
           child.current_fact != after.current_fact &&
           sibling.current_fact == other.current_fact &&
           sibling.current_value == other.current_value);
    NLSemanticValueView updated;
    assert(
        nl_semantic_value_view(v.field.post, after.current_value, &updated) &&
        updated.scalar_known && updated.type == p->u8);
    for (size_t stream = 0; stream < 3; ++stream) {
        NLIfFieldEvent e;
        const bool got =
            nl_checked_if_return_event(a->owner, id, stream, 0, &e);
        if (stream == 1 - v.normal_arm) {
            assert(!got);
            continue;
        }
        assert(got &&
               nl_checked_if_return_event_validate(a->owner, id, stream, 0,
                                                   &e) == NL_CHECK_OK &&
               e.change && e.field.parent == v.field.root &&
               e.field.child == v.field.child &&
               e.field.index == v.field.index &&
               e.field.access == NL_ACCESS_WRITE &&
               e.field.parent_incarnation == entry.incarnation &&
               e.field.child_incarnation == child.incarnation);
        NLSemanticValueView old, incoming;
        assert(nl_semantic_value_view(e.before, e.result, &old) &&
               old.scalar_known &&
               nl_semantic_value_view(e.before, e.incoming, &incoming) &&
               incoming.scalar_known && old.type == p->u8 &&
               incoming.type == p->u8 &&
               incoming.scalar_value == updated.scalar_value);
    }
    const NLCheckedNodeView *read =
        nl_checked_node_view(a->owner, binding(a, "updated")->initializer);
    const NLCheckedNodeView *peer =
        nl_checked_node_view(a->owner, binding(a, "sibling")->initializer);
    assert(read->field.parent == v.field.root &&
           read->field.child == v.field.child &&
           read->field.parent_fact == post.current_fact &&
           read->field.child_fact == after.current_fact &&
           peer->field.child == v.field.sibling &&
           peer->field.child_fact == other.current_fact);
    const NLControlExits *exits = nl_checked_control_exits(a->owner);
    const NLControlExitView *edge = nl_control_exit_view(exits, 0);
    assert(nl_control_exits_count(exits) == 1 && edge &&
           edge->kind == NL_EXIT_RETURN &&
           edge->target == a->owner->function_target);
    NLSemanticBindingView final_binding;
    assert(nl_semantic_binding_view(a->owner->context, v.field.base,
                                    &final_binding) &&
           final_binding.availability == NL_CONSUMED &&
           nl_semantic_place_view(a->owner->context, final_binding.place,
                                  &ended) &&
           !ended.live);
    assert(nl_semantic_binding_view(a->owner->context,
                                    binding(a, "old_copy")->symbol,
                                    &final_binding) &&
           nl_semantic_value_view(a->owner->context, final_binding.value,
                                  &updated) &&
           updated.type == p->u8 && updated.dependencies == NL_DEPENDENCY_FREE);
    printf("C232_SYMBOLS "
           "{\"original\":%zu,\"decision\":%zu,\"old\":%zu,\"old_copy\":%zu,"
           "\"field\":%zu,\"return_arm\":%zu,\"entry\":%zu}\n",
           v.field.base, binding(a, "decision")->symbol,
           binding(a, "old")->symbol, binding(a, "old_copy")->symbol,
           v.field.index, 1 - v.normal_arm,
           nl_checked_node_view(a->root, nl_checked_root(a->root))->function);
    puts("C232 genuine loop/condition/RETURN-ended and sole-normal "
         "replay/current/old/sibling PASS");
}
static void refusals(Input *a, Input *b, V0Program *p)
{
    const size_t id = conditional(a);
    NLIfReturnView v;
    assert(nl_checked_if_return_view(a->owner, id, &v));
    for (size_t t = 0; t < 9; ++t) {
        NLIfReturnView bad = v;
        if (t == 0)
            bad.normal_arm = 1 - v.normal_arm;
        if (t == 1)
            bad.return_live = v.field.post;
        if (t == 2)
            bad.return_ended = v.return_live;
        if (t == 3)
            bad.field.post = v.return_live;
        if (t == 4)
            bad.replay_root = 0;
        if (t == 5)
            bad.return_node = 0;
        if (t == 6)
            bad.field.child = v.field.sibling;
        if (t == 7)
            bad.field.entry = nl_checked_context(b->owner);
        if (t == 8)
            bad.returned.type = p->u8;
        assert(nl_checked_if_return_validate(a->owner, id, &bad) !=
               NL_CHECK_OK);
    }
    for (size_t stream = 0; stream < 3; ++stream) {
        if (stream == 1 - v.normal_arm)
            continue;
        NLIfFieldEvent e;
        assert(nl_checked_if_return_event(a->owner, id, stream, 0, &e));
        e.field.access = NL_ACCESS_READ;
        assert(nl_checked_if_return_event_validate(a->owner, id, stream, 0,
                                                   &e) != NL_CHECK_OK);
    }
    const NLControlExitView *edge = nl_control_exit_view(a->owner->exits, 0);
    const NLControlTarget *saved_target = edge->target;
    NLControlTarget *foreign = NULL;
    assert(nl_control_target_create(NL_TARGET_FUNCTION, &foreign) ==
           NL_CHECK_OK);
    ((NLControlExitView *)edge)->target = foreign;
    assert(!valid(a));
    ((NLControlExitView *)edge)->target = saved_target;
    nl_control_target_destroy(foreign);
    NLCheckedNodeView *comparison =
        &a->owner->nodes[binding(a, "decision")->initializer - 1];
    NLCheckedNodeView saved = *comparison;
    for (size_t t = 0; t < 5; ++t) {
        if (t == 0)
            comparison->first_argument = 0;
        if (t == 1)
            comparison->argument_count = 1;
        if (t == 2)
            comparison->scalar_result.known = true;
        if (t == 3)
            comparison->results[0].type = p->u8;
        if (t == 4)
            comparison->name.start_byte = comparison->name.end_byte;
        assert(!valid(a));
        *comparison = saved;
        assert(valid(a));
    }
    NLCheckedNodeView *loop = &a->owner->nodes[a->loop - 1];
    saved = *loop;
    loop->header_inductive = false;
    assert(!valid(a));
    *loop = saved;
    const NLSource *source = a->owner->source;
    a->owner->source = b->owner->source;
    assert(!valid(a));
    a->owner->source = source;
    const NLSemanticContext *context = a->owner->context;
    a->owner->context = b->owner->context;
    assert(!valid(a));
    a->owner->context = context;
    NLSemanticContext *world = (NLSemanticContext *)context;
    NLFunctionEntry *functions = world->functions;
    world->functions = NULL;
    assert(!valid(a));
    world->functions = functions;
    size_t count = world->function_count;
    world->function_count = 0;
    assert(!valid(a));
    world->function_count = count;
    assert(valid(a) && valid(b));
    puts("C232 RETURN/normal-world/replay/target/current/source/type/callable "
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
        NLCheckedFragment *fa = t < 3 ? a.graph : a.owner,
                          *fb = t < 3 ? b.graph : b.owner;
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
    puts("C232 canonical A disposal / B survives loop/RETURN arms/body "
         "mutations PASS");
    return 0;
}
