/* P-owned real CHECK instances; no foreign graph is executed as a guest. */
#include "h_origin_internal.h"
#include "linear_break_internal.h"
#include "newlang/checked_c_node.h"
#include "newlang/parser.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *syntax, *entry_syntax;
    NLSemanticContext *context;
    NLCheckedFragment *root, *body;
    NLCheckedNodeId loop;
    NLLinearBreakView view;
} Input;
static void load(Input *a, const char *path)
{
    assert(nl_source_load(path, &a->source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->source, &a->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(a->parser, &a->syntax, NULL) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&a->context) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {a->syntax};
    assert(nl_semantic_register_function_unit(a->context, units, 1, NULL) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "P1217 real main", &a->entry_source) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(a->entry_source, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(
               a->entry_parser, &a->entry_syntax, NULL) == NL_PARSE_OK);
    assert(nl_semantic_check_expression(a->context, a->entry_syntax, &a->root,
                                        NULL) == NL_CHECK_OK);
    a->body = (NLCheckedFragment *)nl_checked_call_body(
        a->root, nl_checked_root(a->root));
    assert(a->body);
    for (NLCheckedNodeId i = 1; i <= a->body->count; ++i)
        if (a->body->nodes[i - 1].kind == NL_CHECKED_LOOP) {
            assert(!a->loop);
            a->loop = i;
        }
    assert(a->loop &&
           nl_linear_break_validate(a->body, a->loop, &a->view) == NL_CHECK_OK);
    assert(a->view.changing_current && a->view.entry_world &&
           a->view.post_world);
}
static void dispose(Input *a)
{
    nl_checked_destroy(a->root);
    nl_semantic_destroy(a->context);
    nl_syntax_tree_destroy(a->entry_syntax);
    nl_parser_destroy(a->entry_parser);
    nl_source_destroy(a->entry_source);
    nl_syntax_tree_destroy(a->syntax);
    nl_parser_destroy(a->parser);
    nl_source_destroy(a->source);
}
static void accepted(Input *a)
{
    char *text = NULL;
    size_t length = 0;
    assert(nl_checked_c_node(a->root, &text, &length) == NL_NODE_C_OK && text &&
           length && strlen(text) == length);
    free(text);
}
static void rejected(Input *a)
{
    char *text = NULL;
    size_t length = 12345;
    assert(nl_checked_c_node(a->root, &text, &length) == NL_NODE_C_UNSUPPORTED);
    assert(!text && length == 12345);
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    Input a = {0}, b = {0};
    load(&a, argv[1]);
    load(&b, argv[1]);
    accepted(&a);
    accepted(&b);
    NLCheckedFragment *iteration = (NLCheckedFragment *)a.view.iteration;
    NLCheckedNodeView *loan = NULL, *read = NULL, *constructor = NULL;
    for (size_t i = 0; i < iteration->count; ++i) {
        if (iteration->nodes[i].kind == NL_CHECKED_LOAN_HEADER)
            loan = &iteration->nodes[i];
        if (iteration->nodes[i].kind == NL_CHECKED_FIELD_READ)
            read = &iteration->nodes[i];
    }
    for (size_t i = 0; i < a.body->count; ++i)
        if (a.body->nodes[i].kind == NL_CHECKED_AGGREGATE)
            constructor = &a.body->nodes[i];
    assert(loan && read && constructor);
    const NLSource *old_source = a.body->source;
    a.body->source = b.source;
    rejected(&a);
    a.body->source = old_source;
    const NLSemanticContext *old_world = a.body->context;
    a.body->context = b.body->context;
    rejected(&a);
    a.body->context = old_world;
    NLCheckedNodeView saved = *constructor;
    constructor->type = nl_semantic_core_type(a.context, NL_TYPE_U8);
    rejected(&a);
    *constructor = saved;
    saved = *loan;
    loan->loan.access = NL_ACCESS_READ;
    rejected(&a);
    *loan = saved;
    saved = *read;
    read->field.parent_incarnation++;
    rejected(&a);
    *read = saved;
    saved = *read;
    read->field.child = read->field.parent;
    rejected(&a);
    *read = saved;
    saved = *read;
    read->field.child_fact++;
    rejected(&a);
    *read = saved;
    saved = *read;
    read->reference_result.provenance = NL_PROVENANCE_VALID;
    rejected(&a);
    *read = saved;
    NLSemanticContext *world = (NLSemanticContext *)a.view.post_world;
    NLSemanticPlaceView place = world->places[a.view.child - 1];
    world->places[a.view.child - 1].current_fact++;
    rejected(&a);
    world->places[a.view.child - 1] = place;
    NLCheckedNodeView *loop = &a.body->nodes[a.loop - 1];
    saved = *loop;
    loop->continue_edges++;
    rejected(&a);
    *loop = saved;
    NLFunctionEntry *functions = a.context->functions;
    a.context->functions = NULL;
    rejected(&a);
    a.context->functions = functions;
    const NLCheckedNodeView *call =
        nl_checked_node_view(iteration, a.view.incoming_call);
    world = (NLSemanticContext *)iteration->context;
    NLFunctionEntry *fn = &world->functions[call->function - 1];
    NLTypeId *parameters = fn->parameters;
    void *short_allocation = malloc(1);
    assert(short_allocation);
    fn->parameters = short_allocation;
    rejected(&a);
    fn->parameters = parameters;
    free(short_allocation);
    /* Whole genuine foreign arm replacement, then unrestored A destruction.
     * Canonical ownership must never consume the independent B instance. */
    NLCheckedFragment *foreign = (NLCheckedFragment *)b.view.iteration;
    bool replaced = false;
    for (size_t i = 0; i < a.body->arm_count; ++i)
        if (a.body->arms[i].match == a.loop) {
            a.body->arms[i].artifact = foreign;
            replaced = true;
            break;
        }
    assert(replaced);
    rejected(&a);
    dispose(&a);
    accepted(&b);
    dispose(&b);
    puts("P1217 13 original/source/world/current/loan/foreign-allocation "
         "rejection and canonical A/B destruction PASS");
    return 0;
}
