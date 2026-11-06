/* P-owned A/B mutation cohort on the new mixed continuation paths. */
#include "semantic_check.c"
#include <assert.h>

typedef struct {
    NLSource *source, *entry;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *call;
    NLSemanticContext *world;
    NLCheckedFragment *root, *body;
    NLLinearBreakProof *proof;
} Input;

static void open_input(Input *a, const char *path)
{
    assert(nl_source_load(path, &a->source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->source, &a->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(a->parser, &a->unit, NULL) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&a->world) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {a->unit};
    assert(nl_semantic_register_function_unit(a->world, units, 1, NULL) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C263 owned mutation", &a->entry) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(a->entry, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(a->entry_parser, &a->call,
                                               NULL) == NL_PARSE_OK);
    assert(nl_semantic_check_expression(a->world, a->call, &a->root, NULL) ==
           NL_CHECK_OK);
    a->body = (NLCheckedFragment *)nl_checked_call_body(a->root, a->root->root);
    assert(a->body && a->body->linear_breaks);
    a->proof = a->body->linear_breaks;
    assert(a->proof->current_cycle);
}

static void valid(Input *a)
{
    NLLinearBreakView out;
    assert(nl_h_custody_validate(a->root));
    assert(nl_linear_break_validate(a->body, a->proof->loop, &out) ==
           NL_CHECK_OK);
    assert(out.changing_current && out.incoming_call);
}

static void rejected(Input *a)
{
    NLLinearBreakView out, before;
    memset(&out, 0xA5, sizeof(out));
    memcpy(&before, &out, sizeof(out));
    assert(nl_linear_break_validate(a->body, a->proof->loop, &out) ==
           NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    assert(!memcmp(&out, &before, sizeof(out)));
}

static void close_input(Input *a)
{
    nl_checked_destroy(a->root);
    nl_semantic_destroy(a->world);
    nl_syntax_tree_destroy(a->call);
    nl_parser_destroy(a->entry_parser);
    nl_source_destroy(a->entry);
    nl_syntax_tree_destroy(a->unit);
    nl_parser_destroy(a->parser);
    nl_source_destroy(a->source);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    setvbuf(stdout, NULL, _IONBF, 0);
    for (size_t trial = 0; trial < 20; ++trial) {
        Input a = {0}, b = {0};
        open_input(&a, argv[1]);
        open_input(&b, argv[1]);
        valid(&a);
        valid(&b);
        NLCurrentLoopProof *s = a.proof->current_cycle,
                           *t = b.proof->current_cycle;
        assert(s->return_count && t->return_count);
        CurrentReturnSeal *r = &s->returns[s->return_count - 1];
        NLControlExitView *e = (NLControlExitView *)nl_control_exit_view(
            r->ref.set, r->ref.ordinal);
        const NLControlExitView saved = *e;
        const NLControlExitView *foreign = nl_control_exit_view(
            t->returns[0].ref.set, t->returns[0].ref.ordinal);
        NLCheckedFragment *helper = (NLCheckedFragment *)s->graphs[3].fragment;
        NLCheckedFragment *iteration =
            (NLCheckedFragment *)s->graphs[1].fragment;
        NLSemanticContext *world = (NLSemanticContext *)r->world.world;
        bool unrestored = false;
        switch (trial) {
        case 0:
            e->target = foreign->target;
            rejected(&a);
            *e = saved;
            break;
        case 1:
            e->state = foreign->state;
            rejected(&a);
            *e = saved;
            break;
        case 2:
            e->count = 0;
            rejected(&a);
            *e = saved;
            break;
        case 3:
            e->values[0].value = 0;
            rejected(&a);
            *e = saved;
            break;
        case 4:
            e->values[0].type = nl_semantic_core_type(world, NL_TYPE_USIZE);
            rejected(&a);
            *e = saved;
            break;
        case 5:
            e->kind = NL_EXIT_BREAK;
            rejected(&a);
            *e = saved;
            break;
        case 6: {
            NLControlTarget *old = helper->function_target;
            helper->function_target = (NLControlTarget *)t->helper_target;
            rejected(&a);
            helper->function_target = old;
            break;
        }
        case 7: {
            NLValueId old = world->places[s->child - 1].current_value;
            world->places[s->child - 1].current_value = 0;
            rejected(&a);
            world->places[s->child - 1].current_value = old;
            break;
        }
        case 8: {
            NLFunctionEntry *fn = &world->functions[s->helper_function - 1];
            NLTypeId *old = fn->parameters;
            void *short_allocation = malloc(1);
            assert(short_allocation);
            fn->parameters = short_allocation;
            rejected(&a);
            fn->parameters = old;
            free(short_allocation);
            break;
        }
        case 9: {
            NLFunctionEntry *old = world->functions;
            world->functions = NULL;
            rejected(&a);
            world->functions = old;
            break;
        }
        case 10: {
            size_t old = world->function_count;
            world->function_count = s->helper_function - 1;
            rejected(&a);
            world->function_count = old;
            break;
        }
        case 11: {
            NLCheckedNodeView *call = &iteration->nodes[s->scalar - 1];
            NLCheckedNodeView *arg =
                &iteration->nodes[call->first_argument - 1];
            NLSymbolId old = arg->symbol;
            arg->symbol = 0;
            rejected(&a);
            arg->symbol = old;
            break;
        }
        case 12: {
            const NLSource *old = helper->source;
            helper->source = t->graphs[3].fragment->source;
            rejected(&a);
            helper->source = old;
            break;
        }
        case 13: {
            NLFunctionBody *old = helper->body_owner;
            helper->body_owner = t->graphs[3].fragment->body_owner;
            rejected(&a);
            helper->body_owner = old;
            break;
        }
        case 14: {
            NLPureU8Call pure;
            assert(nl_pure_u8_contents(iteration, s->scalar, &pure));
            NLCheckedNodeView *n = &helper->nodes[pure.selection - 1];
            size_t old = n->normal_arms;
            n->normal_arms = 2;
            rejected(&a);
            n->normal_arms = old;
            break;
        }
        case 15: {
            NLPureU8Call pure;
            assert(nl_pure_u8_contents(iteration, s->scalar, &pure));
            NLCheckedNodeView *n = &helper->nodes[pure.selection - 1];
            NLCheckedNodeView *replay = &helper->nodes[n->tail - 1];
            NLValueId old = replay->results[0].value;
            replay->results[0].value = 0;
            rejected(&a);
            replay->results[0].value = old;
            break;
        }
        case 16: {
            NLCheckedNodeView *loan = NULL;
            for (size_t i = 0; i < iteration->count; ++i)
                if (iteration->nodes[i].kind == NL_CHECKED_LOAN_HEADER)
                    loan = &iteration->nodes[i];
            assert(loan);
            NLAccessSyntax old = loan->loan.access;
            loan->loan.access = NL_ACCESS_READ;
            rejected(&a);
            loan->loan.access = old;
            break;
        }
        case 17: {
            size_t old = r->ref.ordinal;
            r->ref.ordinal = old ? 0 : 1;
            rejected(&a);
            r->ref.ordinal = old;
            break;
        }
        case 18:
            helper->arms[0].artifact =
                (NLCheckedFragment *)t->graphs[4].fragment;
            rejected(&a);
            unrestored = true;
            break;
        case 19:
            e->state = foreign->state;
            rejected(&a);
            unrestored = true;
            break;
        default:
            assert(0);
        }
        if (!unrestored)
            valid(&a);
        close_input(&a);
        valid(&b);
        close_input(&b);
        printf("C263 mutation=%zu "
               "reject/out-unchanged/restore-retry/A-destroy-B-survive=PASS "
               "restore=%d\n",
               trial, !unrestored);
    }
    return 0;
}
