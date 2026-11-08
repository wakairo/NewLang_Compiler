/* P-owned actual source CHECK/owner proof, not a synthetic result issuer. */
#define main p1248_preserved_cli_main
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
    assert(nl_source_create("main()", 6, "P1248 genuine call",
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
static void accepted(Input *t)
{
    NLLinearBreakView v = {0};
    assert(nl_linear_break_validate(t->owner, t->loop, &v) == NL_CHECK_OK);
    assert(v.terminal_old && v.changing_current && !v.scalar_loop &&
           v.breaking && v.terminal_loan);
    const NLCheckedField c = v.terminal_change;
    const NLControlExitView *x = nl_control_exit_view(v.breaking->exits, 0);
    const NLSemanticContext *exit =
        nl_control_state_context((NLControlState *)x->state);
    const NLCheckedNodeView *br =
        nl_checked_node_view(v.breaking, x->source_node);
    const NLCheckedNodeView *loan =
        nl_checked_node_view(v.breaking, v.terminal_loan);
    assert(x && x->kind == NL_EXIT_BREAK && x->count == 1 &&
           x->source_owner == v.breaking && br &&
           br->kind == NL_CHECKED_BREAK && br->initializer == v.break_value &&
           loan && loan->kind == NL_CHECKED_LOAN_HEADER && loan->loan.scope &&
           !v.breaking->context->scopes[loan->loan.scope - 1].active);
    /* Registration owns an immutable body-source slice, not the input unit
     * NLSource object. Authenticate that exact registered plan and owner. */
    size_t registered = 0;
    for (size_t i = 0; i < t->context->function_count; ++i) {
        const NLFunctionEntry *fn = &t->context->functions[i];
        if (fn->name && !strcmp(fn->name, "main")) {
            assert(fn->body == t->owner->body_owner &&
                   fn->body->source == v.breaking->source);
            ++registered;
        }
    }
    assert(registered == 1 && v.breaking->source == t->owner->source &&
           c.present && c.dependency_compatible && c.parent == v.root &&
           c.child == v.child && c.index == v.index &&
           c.parent_incarnation == v.incarnation &&
           c.child_incarnation == v.child_incarnation);
    const NLSemanticContext *post = v.post_world;
    const NLTypeId u8 = nl_semantic_core_type(post, NL_TYPE_U8);
    assert(v.result.type == u8 && v.result.type == x->values[0].type &&
           v.result.value == x->values[0].value &&
           nl_copy_plain(post, v.result.value, u8));
    assert(post->values[v.result.value - 1].scalar_known == false);
    assert(post->places[v.child - 1].current_value == c.new_value &&
           post->places[v.root - 1].incarnation == v.incarnation &&
           post->places[v.child - 1].incarnation == v.child_incarnation &&
           nl_copy_plain(post, c.new_value, u8) &&
           (v.terminal_call ? !post->values[c.new_value - 1].scalar_known
                            : (post->values[c.new_value - 1].scalar_known &&
                               post->values[c.new_value - 1].scalar_value == 211)));
    if (v.terminal_call) {
        const NLCheckedNodeView *call = nl_checked_node_view(v.breaking, v.terminal_call);
        assert(call && call->kind == NL_CHECKED_REGISTERED_CALL &&
               nl_h_custody_result_valid(v.breaking, v.terminal_call) &&
               v.terminal_helper_body == nl_checked_call_body(v.breaking, v.terminal_call) &&
               v.terminal_helper_formal && v.terminal_helper_selection);
    }
    const NLPlaceId peer = post->places[v.root - 1].fixed_fields[1 - v.index];
    assert(post->values[post->places[peer - 1].current_value - 1].scalar_known);
    assert(exit->values[v.result.value - 1].type == u8 &&
           exit->places[v.child - 1].current_value ==
               post->places[v.child - 1].current_value);
    printf("OWNED current=1 terminal_old=1 scalar_loop=0 result_type=u8 "
           "old_known=0 post=%zu sibling=%zu field=%zu closed_loan=1 "
           "source_break=%zu\n",
           post->values[c.new_value - 1].scalar_value,
           post->values[post->places[peer - 1].current_value - 1].scalar_value,
           v.index, x->source_node);
    NLSymbolId roles[5] = {0}, caller[4] = {0};
    const NLCheckedNodeView *iw =
        nl_checked_node_view(v.iteration, nl_checked_root(v.iteration));
    const NLCheckedNodeView *ib =
        nl_checked_node_view(v.iteration, iw->initializer);
    for (NLCheckedNodeId q = ib->first_item; q && q != v.continue_node;) {
        const NLCheckedNodeView *n = nl_checked_node_view(v.iteration, q);
        const NLCheckedNodeView *a =
            nl_checked_node_view(v.iteration, n->initializer);
        if (n->kind == NL_CHECKED_BINDING) {
            if (a->kind == NL_CHECKED_REGISTERED_CALL)
                roles[0] = n->symbol;
            if (a->kind == NL_CHECKED_LOAN_HEADER)
                roles[1] = n->symbol;
            if (a->kind == NL_CHECKED_IDENTIFIER && a->symbol == roles[1])
                roles[2] = n->symbol;
            if (a->kind == NL_CHECKED_FIELD_READ)
                roles[a->field.index == v.index ? 3 : 4] = n->symbol;
        }
        q = n->next_item;
    }
    const char *names[] = {"answer", "saved_answer", "post_value",
                           "post_witness"};
    const NLCheckedNodeView *main_body =
        nl_checked_node_view(t->owner, nl_checked_root(t->owner));
    for (NLCheckedNodeId q = main_body->first_item; q;) {
        const NLCheckedNodeView *n = nl_checked_node_view(t->owner, q);
        NLSourceView name;
        if (n->kind == NL_CHECKED_BINDING &&
            nl_source_view(t->owner->source, n->name, &name))
            for (size_t j = 0; j < 4; ++j)
                if (name.length == strlen(names[j]) &&
                    !memcmp(name.bytes, names[j], name.length))
                    caller[j] = n->symbol;
        q = n->next_item;
    }
    assert(roles[0] && roles[1] && roles[2] && roles[3] && roles[4] &&
           caller[0] && caller[1] && caller[2] && caller[3]);
    printf("ROLES %zu %zu %zu %zu %zu CALLER %zu %zu %zu %zu\n", roles[0],
           roles[1], roles[2], roles[3], roles[4], caller[0], caller[1],
           caller[2], caller[3]);
    t->view = v;
}
static void rejected(Input *t)
{
    NLLinearBreakView out = {.root = 987654};
    assert(nl_linear_break_validate(t->owner, t->loop, &out) != NL_CHECK_OK &&
           out.root == 987654);
#ifdef P1248_EMIT_SAFETY
    char *text = NULL;
    size_t length = 98765;
    assert(nl_checked_c_node(t->owned, &text, &length) ==
               NL_NODE_C_UNSUPPORTED &&
           text == NULL && length == 98765);
#endif
}
static void adversarial(const char *path)
{
    Input a = {0}, b = {0};
    load(&a, path);
    load(&b, path);
    accepted(&a);
    accepted(&b);
    assert(a.view.result.value == b.view.result.value &&
           a.view.post_world != b.view.post_world);
    NLCheckedFragment *arm = (NLCheckedFragment *)a.view.breaking;
    const NLSemanticContext *world = arm->context;
    arm->context = b.view.breaking->context;
    rejected(&a);
    arm->context = world;
    const NLSource *source = arm->source;
    arm->source = b.view.breaking->source;
    rejected(&a);
    arm->source = source;
    NLControlExits *exits = arm->exits;
    arm->exits = b.view.breaking->exits;
    rejected(&a);
    arm->exits = exits;
    NLControlExitView *edge =
        (NLControlExitView *)nl_control_exit_view(arm->exits, 0);
    NLControlExitView old_edge = *edge;
    edge->source_node = old_edge.source_node + 1;
    rejected(&a);
    *edge = old_edge;
    NLCheckedNodeView *loop = &a.owner->nodes[a.loop - 1];
    NLCheckedNodeView old_loop = *loop;
    loop->results[0].value = a.view.terminal_change.new_value;
    rejected(&a);
    *loop = old_loop;
    loop->type = nl_semantic_core_type(a.owner->context, NL_TYPE_USIZE);
    rejected(&a);
    *loop = old_loop;
    NLSemanticContext *post = (NLSemanticContext *)a.view.post_world;
    NLValueFactId fact = post->places[a.view.child - 1].current_fact;
    post->places[a.view.child - 1].current_fact =
        a.view.terminal_change.child_fact;
    rejected(&a);
    post->places[a.view.child - 1].current_fact = fact;
    NLPlaceId sibling =
        post->places[a.view.root - 1].fixed_fields[1 - a.view.index];
    NLSemanticValueView peer =
        post->values[post->places[sibling - 1].current_value - 1];
    post->values[post->places[sibling - 1].current_value - 1].scalar_value ^= 1;
    rejected(&a);
    post->values[post->places[sibling - 1].current_value - 1] = peer;
    NLSemanticValueView old_value = post->values[a.view.result.value - 1];
    post->values[a.view.result.value - 1].reference_count = 1;
    rejected(&a);
    post->values[a.view.result.value - 1] = old_value;
    edge->source_target =
        nl_control_exit_view(b.view.breaking->exits, 0)->source_target;
    rejected(&a);
    *edge = old_edge;
    NLCheckedNodeView *loan = &arm->nodes[a.view.terminal_loan - 1];
    NLSemanticContext *mutable_world = (NLSemanticContext *)world;
    assert(!mutable_world->scopes[loan->loan.scope - 1].active);
    mutable_world->scopes[loan->loan.scope - 1].active = true;
    rejected(&a);
    mutable_world->scopes[loan->loan.scope - 1].active = false;
    NLCheckedNodeView *body = &arm->nodes[loan->initializer - 1];
    NLCheckedNodeView *change = &arm->nodes[body->tail - 1];
    NLCheckedNodeView old_change = *change;
    change->field.index = 1 - change->field.index;
    rejected(&a);
    *change = old_change;
    change->field.child_incarnation++;
    rejected(&a);
    *change = old_change;
    NLCheckedFragment shallow = *a.owner;
    NLLinearBreakView bad = {.root = 987654};
    assert(nl_linear_break_validate(&shallow, a.loop, &bad) != NL_CHECK_OK &&
           bad.root == 987654);
    accepted(&a);
    NLCheckedFragment *graph =
        (NLCheckedFragment *)nl_checked_loop_body(a.owner, a.loop);
    NLControlExitView *one =
        (NLControlExitView *)nl_control_exit_view(graph->exits, 0);
    NLControlExitView *two =
        (NLControlExitView *)nl_control_exit_view(graph->exits, 1);
    NLControlExitView first = *one, second = *two;
    *one = second;
    *two = first;
    rejected(&a);
    *one = first;
    *two = second;
    NLCheckedFragment *iteration = (NLCheckedFragment *)a.view.iteration;
    NLCheckedNodeView *continuation =
        &iteration->nodes[a.view.continue_node - 1];
    NLCheckedNodeId quantity = continuation->initializer;
    NLCheckedNodeView old_quantity = iteration->nodes[quantity - 1];
    iteration->nodes[quantity - 1].kind = NL_CHECKED_USIZE_SUBTRACTION;
    rejected(&a);
    iteration->nodes[quantity - 1] = old_quantity;
    bool substituted = false;
    for (size_t i = 0; i < graph->arm_count; ++i)
        if (graph->arms[i].artifact == arm) {
            graph->arms[i].artifact = (NLCheckedFragment *)b.view.breaking;
            substituted = true;
            break;
        }
    assert(substituted);
    rejected(&a);
    dispose(
        &a); /* Canonical child ownership, foreign B deliberately unrestored. */
    accepted(&b);
    dispose(&b);
    puts("NEGATIVE 17 genuine "
         "foreign-source/world/edge/result/loan/field/incarnation/shallow/"
         "whole-arm controls PASS; A destroyed/B survives");
}

/* New terminal callable controls. Each mutation is a read-only view attack;
 * neither equal scalar bits nor foreign display children gain authority. */
static void terminal_adversarial(const char *path)
{
    Input a = {0}, b = {0};
    load(&a, path); load(&b, path);
    accepted(&a); accepted(&b);
    assert(a.view.terminal_call && b.view.terminal_call);
    NLCheckedFragment *arm = (NLCheckedFragment *)a.view.breaking;
    NLCheckedFragment *body = (NLCheckedFragment *)a.view.terminal_helper_body;
    NLCheckedFragment *other = (NLCheckedFragment *)b.view.terminal_helper_body;
    NLCheckedNodeView *call = &arm->nodes[a.view.terminal_call - 1];
    NLCheckedNodeView saved_call = *call;
    call->function = 0; rejected(&a); *call = saved_call;
    call->results[0].value = a.view.terminal_change.old_value;
    rejected(&a); *call = saved_call;
    NLCheckedNodeView *arg = &arm->nodes[call->first_argument - 1];
    NLCheckedNodeView saved_arg = *arg;
    arg->symbol++; rejected(&a); *arg = saved_arg;
    NLCheckedNodeView *choice = &body->nodes[a.view.terminal_helper_selection - 1];
    NLCheckedNodeView saved_choice = *choice;
    choice->normal_arms = 1; rejected(&a); *choice = saved_choice;
    NLCheckedNodeView *root = &body->nodes[body->root - 1];
    NLCheckedNodeView saved_root = *root;
    root->tail = 0; rejected(&a); *root = saved_root;
    const NLSource *source = body->source;
    body->source = other->source; rejected(&a); body->source = source;
    NLControlTarget *target = body->function_target;
    assert(target && other->function_target && target != other->function_target);
    body->function_target = other->function_target;
    rejected(&a); body->function_target = target;
    size_t count = body->arm_count;
    body->arm_count = 0; rejected(&a); body->arm_count = count;
    body->arm_count = count + 1; rejected(&a); body->arm_count = count;
    NLSemanticContext *world = (NLSemanticContext *)body->context;
    NLFunctionEntry *functions = world->functions;
    world->functions = NULL; rejected(&a); world->functions = functions;
    count = world->function_count;
    world->function_count = count + 1; rejected(&a); world->function_count = count;
    NLFunctionEntry *fn = &functions[call->function - 1];
    NLTypeId *parameters = fn->parameters;
    void *short_foreign = malloc(1);
    assert(short_foreign);
    fn->parameters = short_foreign;
    rejected(&a); fn->parameters = parameters; free(short_foreign);
    count = fn->count;
    fn->count = count + 1; rejected(&a); fn->count = count;
    NLCheckedNodeView *loan = &arm->nodes[a.view.terminal_loan - 1];
    NLCheckedNodeView *block = &arm->nodes[loan->initializer - 1];
    NLCheckedNodeView *change = &arm->nodes[block->tail - 1];
    NLCheckedNodeView saved_change = *change;
    change->field.new_value = change->field.old_value;
    rejected(&a); *change = saved_change;
    accepted(&a);
    assert(body->arm_count == 2 && other->arm_count == 2);
    body->arms[0].artifact = other->arms[0].artifact;
    rejected(&a);
    dispose(&a); /* Deliberately unrestored foreign B pure IF arm. */
    accepted(&b); dispose(&b);
    load(&a, path); load(&b, path);
    arm = (NLCheckedFragment *)a.view.breaking;
    bool found = false;
    for (size_t i = 0; i < arm->body_count; ++i)
        if (arm->body_calls[i] == a.view.terminal_call) {
            arm->bodies[i] = (NLCheckedFragment *)b.view.terminal_helper_body;
            found = true;
            break;
        }
    assert(found); rejected(&a);
    dispose(&a); /* Canonical ownership must keep B's entire body alive. */
    accepted(&b); dispose(&b);
    puts("P1248 terminal NEGATIVE 16 PASS: short foreign signature, callable/source/current/graph and A destroyed/B alive");
}

int main(int argc, char **argv)
{
    assert(argc > 1);
    setvbuf(stdout, NULL, _IONBF, 0);
    for (int i = 1; i < argc; ++i) {
        Input t = {0};
        load(&t, argv[i]);
        accepted(&t);
        dispose(&t);
    }
    adversarial(argv[1]);
    terminal_adversarial(argv[1]);
    puts("P1248 genuine source CHECK/owned typed terminal transfer and "
         "foreign-issuer refusal PASS");
    return 0;
}
