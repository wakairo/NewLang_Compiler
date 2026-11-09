/* P-owned actual source CHECK/owner proof, not a synthetic result issuer. */
#define main p1260_preserved_cli_main
#include "main.c"
#undef main
#include <assert.h>

typedef struct {
    NLSource *source, *call_source;
    NLParser *parser, *call_parser;
    NLSyntaxTree *tree, *call_tree;
    NLSemanticContext *context;
    NLCheckedFragment *owned, *owner;
    NLCheckedNodeId loop;
    NLLinearBreakView view;
} Input;
static const NLCheckedFragment *visited[256];
static size_t visited_count;
static void find_loop(Input *t, const NLCheckedFragment *f)
{
    if (!f)
        return;
    for (size_t i = 0; i < visited_count; ++i)
        if (visited[i] == f)
            return;
    assert(visited_count < 256);
    visited[visited_count++] = f;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        assert(n);
        if (n->kind == NL_CHECKED_LOOP) {
            assert(!t->owner);
            t->owner = (NLCheckedFragment *)f;
            t->loop = i;
        }
        find_loop(t, nl_checked_call_body(f, i));
        if (n->kind == NL_CHECKED_IF)
            for (size_t a = 0; a < 2; ++a)
                find_loop(t, nl_checked_if_arm(f, i, a));
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < n->item_count; ++a)
                find_loop(t, nl_checked_match_arm(f, i, a));
        if (n->kind == NL_CHECKED_LOOP)
            find_loop(t, nl_checked_loop_body(f, i));
    }
}
static void load(Input *t, const char *path)
{
    NLParseDiagnostic p = {0};
    NLCheckDiagnostic d = {0};
    NLFunctionUnitDiagnostic u = {0};
    assert(nl_source_load(path, &t->source) == NL_SOURCE_OK);
    assert(nl_parser_create(t->source, &t->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(t->parser, &t->tree, &p) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&t->context) == NL_CHECK_OK);
    const NLSyntaxTree *trees[] = {t->tree};
    assert(nl_semantic_register_function_unit(t->context, trees, 1, &u) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "P1260 genuine call",
                            &t->call_source) == NL_SOURCE_OK);
    assert(nl_parser_create(t->call_source, &t->call_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(t->call_parser, &t->call_tree,
                                               &p) == NL_PARSE_OK);
    NLCheckStatus status =
        nl_semantic_check_expression(t->context, t->call_tree, &t->owned, &d);
    printf("SOURCE %s CHECK=%d owned=%d\n", path, status, t->owned != NULL);
    assert(status == NL_CHECK_OK && t->owned);
    assert(nl_h_custody_validate(t->owned) && nl_h_origin_has_sites(t->owned) &&
           nl_h_origin_validate(t->owned));
    visited_count = 0;
    find_loop(t, t->owned);
    assert(t->owner && nl_linear_break_validate(t->owner, t->loop, &t->view) ==
                           NL_CHECK_OK);
}
static void dispose(Input *t)
{
    nl_checked_destroy(t->owned);
    nl_semantic_destroy(t->context);
    nl_syntax_tree_destroy(t->call_tree);
    nl_parser_destroy(t->call_parser);
    nl_source_destroy(t->call_source);
    nl_syntax_tree_destroy(t->tree);
    nl_parser_destroy(t->parser);
    nl_source_destroy(t->source);
}

static size_t refused, observations;
static size_t calls(Input *t, NLCheckedNodeId ids[2])
{
    size_t count = 0;
    for (NLCheckedNodeId id = 1; id <= t->owner->count; ++id) {
        NLCopyBodyParameters p;
        if (nl_checked_copy_body_parameters(t->owner, id, &p)) {
            assert(count < 2);
            ids[count++] = id;
        }
    }
    return count;
}
static void accepted(Input *t)
{
    assert(nl_linear_break_validate(t->owner, t->loop, &t->view) ==
           NL_CHECK_OK);
    assert(t->view.terminal_old && t->view.changing_current &&
           t->view.result.value != t->view.terminal_change.new_value);
    NLCheckedNodeId ids[2];
    const size_t count = calls(t, ids);
    assert(count);
    for (size_t i = 0; i < count; ++i) {
        NLCopyBodyView v;
        NLCopyBodyParameters p;
        NLCopyBodyEventView e;
        assert(nl_checked_copy_body_view(t->owner, ids[i], &v) &&
               nl_checked_copy_body_parameters(t->owner, ids[i], &p) &&
               nl_checked_copy_body_event(t->owner, ids[i], 0, &e));
        assert(nl_checked_copy_body_validate(t->owner, ids[i], &v) ==
                   NL_CHECK_OK &&
               nl_checked_copy_body_parameters_validate(t->owner, ids[i], &p) ==
                   NL_CHECK_OK &&
               nl_checked_copy_body_event_validate(t->owner, ids[i], 0, &e) ==
                   NL_CHECK_OK);
        const NLCheckedNodeView *loan = nl_checked_node_view(t->owner, p.loan);
        NLSemanticScopeView scope;
        NLSemanticPlaceView root, child;
        assert(loan && loan->loan.body_nonescape_proved &&
               loan->loan.normal_result_forwarded &&
               nl_semantic_scope_view(t->owner->context, loan->loan.scope,
                                      &scope) &&
               !scope.active && e.kind == NL_COPY_BODY_CHANGE &&
               v.event_count == 1 && e.result != e.incoming &&
               p.field.parent == t->view.root &&
               p.field.parent_incarnation == t->view.incarnation &&
               nl_semantic_place_view(p.handoff, p.field.parent, &root) &&
               nl_semantic_place_view(p.handoff, p.field.child, &child) &&
               root.fixed_fields[p.field.index] == p.field.child &&
               child.current_value == e.field.new_value);
        printf("POST call=%zu field=%zu actual-old=%zu incoming=%zu "
               "root=%zu incarnation=%zu closed=1 validators=0\n",
               ids[i], p.field.index, e.result, e.incoming, p.field.parent,
               p.field.parent_incarnation);
    }
    const NLCheckedNodeView *root =
        nl_checked_node_view(t->owner, t->owner->root);
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(t->owner, id);
        NLSourceView name;
        if (n->kind == NL_CHECKED_BINDING &&
            nl_source_view(t->owner->source, n->name, &name))
            printf("BIND %.*s %zu\n", (int)name.length,
                   (const char *)name.bytes, n->symbol);
        id = n->next_item;
    }
    char *text = NULL;
    size_t length = 0;
    assert(nl_checked_c_node(t->owned, &text, &length) == NL_NODE_C_OK &&
           text && length);
    free(text);
}
static void rejected(Input *t)
{
    char *text = NULL;
    size_t length = 98765;
    assert(nl_checked_c_node(t->owned, &text, &length) ==
               NL_NODE_C_UNSUPPORTED &&
           !text && length == 98765);
    ++refused;
}
static void candidate_controls(Input *t)
{
    NLCheckedNodeId ids[2];
    const size_t count = calls(t, ids);
    for (size_t i = 0; i < count; ++i) {
        NLCopyBodyParameters p, bad;
        NLCopyBodyView v;
        NLCopyBodyEventView e, wrong;
        assert(nl_checked_copy_body_parameters(t->owner, ids[i], &p) &&
               nl_checked_copy_body_view(t->owner, ids[i], &v) &&
               nl_checked_copy_body_event(t->owner, ids[i], 0, &e));
#define PARAMETER_CHANGE(statement)                                            \
    do {                                                                       \
        bad = p;                                                               \
        statement;                                                             \
        assert(nl_checked_copy_body_parameters_validate(t->owner, ids[i],      \
                                                        &bad) != NL_CHECK_OK); \
        ++observations;                                                        \
    } while (0)
        PARAMETER_CHANGE(bad.entry = p.handoff);
        PARAMETER_CHANGE(bad.handoff = p.entry);
        PARAMETER_CHANGE(bad.loan++);
        PARAMETER_CHANGE(bad.arguments[0] = p.arguments[1]);
        PARAMETER_CHANGE(bad.parameters[0] = p.parameters[1]);
        PARAMETER_CHANGE(bad.field.index = 1 - p.field.index);
        PARAMETER_CHANGE(bad.field.parent_incarnation++);
        PARAMETER_CHANGE(bad.field.old_value = e.incoming);
#undef PARAMETER_CHANGE
        wrong = e;
        wrong.result = e.incoming;
        assert(nl_checked_copy_body_event_validate(t->owner, ids[i], 0,
                                                   &wrong) != NL_CHECK_OK);
        wrong = e;
        wrong.field.child_incarnation++;
        assert(nl_checked_copy_body_event_validate(t->owner, ids[i], 0,
                                                   &wrong) != NL_CHECK_OK);
        v.result.value = e.incoming;
        assert(nl_checked_copy_body_validate(t->owner, ids[i], &v) !=
               NL_CHECK_OK);
        observations += 3;
    }
}
static void adversarial(const char *path)
{
    Input a = {0}, b = {0};
    load(&a, path);
    load(&b, path);
    NLCheckedNodeId ids[2], other_ids[2];
    assert(calls(&a, ids) == calls(&b, other_ids));
    NLCheckedNodeView *call = &a.owner->nodes[ids[0] - 1];
    NLCopyBodyParameters p;
    NLCopyBodyView view;
    assert(nl_checked_copy_body_parameters(a.owner, ids[0], &p) &&
           nl_checked_copy_body_view(a.owner, ids[0], &view));
    NLCheckedFragment *callee = (NLCheckedFragment *)view.body;
    NLCheckedNodeView *loan = &a.owner->nodes[p.loan - 1];
#define MUTATE(object, statement)                                              \
    do {                                                                       \
        unsigned char saved[sizeof(object)];                                   \
        memcpy(saved, &(object), sizeof(object));                              \
        statement;                                                             \
        rejected(&a);                                                          \
        memcpy(&(object), saved, sizeof(object));                              \
    } while (0)
    MUTATE(*loan, loan->loan.body_nonescape_proved = false);
    MUTATE(*loan, loan->loan.normal_result_forwarded = false);
    MUTATE(*loan, loan->loan.access = NL_ACCESS_READ);
    MUTATE(*loan, loan->loan.place = a.view.child);
    MUTATE(*loan, loan->field.index = 1 - loan->field.index);
    MUTATE(*call, call->function = 0);
    MUTATE(*call, call->function = a.owner->context->function_count + 1);
    MUTATE(*call, call->results[0].value = a.view.terminal_change.new_value);
    MUTATE(callee->context, callee->context = b.owner->context);
    MUTATE(callee->source, callee->source = b.owner->source);
    NLSemanticContext *world = (NLSemanticContext *)a.owner->context;
    /* current_signature_origin authenticates all allocation bases/counts
     * before profile walkers or signature content reads. */
    MUTATE(world->functions, world->functions = NULL);
    MUTATE(world->function_count, ++world->function_count);
    NLFunctionEntry *fn = &world->functions[call->function - 1];
    void *short_foreign = malloc(1);
    assert(short_foreign);
    MUTATE(fn->parameters, fn->parameters = short_foreign);
    free(short_foreign);
    MUTATE(fn->count, ++fn->count);
    MUTATE(fn->result, fn->result = nl_semantic_unit_type(world));
    MUTATE(fn->kind, fn->kind = NL_CHECKED_UNIT);
    MUTATE(fn->caller_effects, fn->caller_effects = true);
    MUTATE(fn->owner_receiver, fn->owner_receiver = true);
    MUTATE(fn->body, fn->body = b.owner->body_owner);
    NLSemanticContext *entry = (NLSemanticContext *)p.entry;
    NLSemanticPlaceView *root = &entry->places[p.field.parent - 1];
    MUTATE(root->incarnation, ++root->incarnation);
    MUTATE(root->current_fact, ++root->current_fact);
    const NLCheckedNodeView *l = nl_checked_node_view(a.owner, p.loan);
    MUTATE(world->scopes[l->loan.scope - 1].active,
           world->scopes[l->loan.scope - 1].active = true);
    NLCheckedFragment *arm = (NLCheckedFragment *)a.view.breaking;
    MUTATE(arm->context, arm->context = b.view.breaking->context);
    NLControlExitView *edge =
        (NLControlExitView *)nl_control_exit_view(arm->exits, 0);
    MUTATE(edge->source_target,
           edge->source_target =
               nl_control_exit_view(b.view.breaking->exits, 0)->source_target);
    MUTATE(a.owner->body_count, a.owner->body_count = 0);
    MUTATE(a.owner->body_count, ++a.owner->body_count);
    NLCheckedFragment *helper = (NLCheckedFragment *)a.view.helper_body;
    MUTATE(helper->arm_count, helper->arm_count = 0);
    MUTATE(helper->arm_count, ++helper->arm_count);
#undef MUTATE
    candidate_controls(&a);
    accepted(&a);
    /* Replace the complete genuine child with B, deliberately never restore
     * it. Destroy still follows A's private canonical ownership, not a view. */
    NLCheckedFragment *other_helper = (NLCheckedFragment *)b.view.helper_body;
    assert(helper->arm_count && other_helper->arm_count);
    helper->arms[0].artifact = other_helper->arms[0].artifact;
    rejected(&a);
    helper->arm_count++;
    a.owner->body_count++;
    dispose(&a);
    accepted(&b);
    dispose(&b);
}
int main(int argc, char **argv)
{
    assert(argc == 3);
    setvbuf(stdout, NULL, _IONBF, 0);
    for (int i = 1; i < argc; ++i) {
        Input t = {0};
        load(&t, argv[i]);
        accepted(&t);
        candidate_controls(&t);
        dispose(&t);
    }
    adversarial(argv[1]);
    printf("P1260 REJECT %zu observations %zu; A destroyed unrestored/B "
           "survives; genuine owned/current/typed Copy PASS\n",
           refused, observations);
    assert(refused >= 29 && observations >= 44);
    return 0;
}
