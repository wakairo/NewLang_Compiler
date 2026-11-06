/* Twelve P-owned challenges, using genuine independently checked A/B roots. */
#include "semantic_check.c"
#include <assert.h>

typedef struct {
    NLSource *source, *entry;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *call;
    NLSemanticContext *world;
    NLCheckedFragment *root;
    NLCheckedFragment *body;
    NLLinearBreakProof *proof;
} P256Input;
static void p256_open(P256Input *a, const char *path)
{
    assert(nl_source_load(path, &a->source) == NL_SOURCE_OK);
    assert(nl_parser_create(a->source, &a->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(a->parser, &a->unit, NULL) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&a->world) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {a->unit};
    assert(nl_semantic_register_function_unit(a->world, units, 1, NULL) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "C256 challenge genuine main",
                            &a->entry) == NL_SOURCE_OK);
    assert(nl_parser_create(a->entry, &a->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(a->entry_parser, &a->call,
                                               NULL) == NL_PARSE_OK);
    assert(nl_semantic_check_expression(a->world, a->call, &a->root, NULL) ==
           NL_CHECK_OK);
    a->body = (NLCheckedFragment *)nl_checked_call_body(a->root, a->root->root);
    assert(a->body);
    a->proof = a->body->linear_breaks;
    assert(a->proof && a->proof->current_cycle);
    NLLinearBreakView v;
    assert(nl_linear_break_validate(a->body, a->proof->loop, &v) ==
           NL_CHECK_OK);
    assert(nl_h_custody_validate(a->root));
}
static void p256_close(P256Input *a)
{
    nl_checked_destroy(a->root);
    nl_semantic_destroy(a->world);
    nl_syntax_tree_destroy(a->call);
    nl_parser_destroy(a->entry_parser);
    nl_source_destroy(a->entry);
    nl_syntax_tree_destroy(a->unit);
    nl_parser_destroy(a->parser);
    nl_source_destroy(a->source);
    memset(a, 0, sizeof(*a));
}
static void p256_rejected(P256Input *a)
{
    NLLinearBreakView v, before;
    memset(&v, 0xA5, sizeof(v));
    memcpy(&before, &v, sizeof(v));
    NLCheckStatus status =
        nl_linear_break_validate(a->body, a->proof->loop, &v);
    assert(status == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
           !memcmp(&v, &before, sizeof(v)));
}
static void p256_valid(P256Input *a)
{
    NLLinearBreakView v;
    assert(nl_h_custody_validate(a->root));
    assert(nl_linear_break_validate(a->body, a->proof->loop, &v) ==
           NL_CHECK_OK);
    assert(v.changing_current && v.incoming_call);
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *cases[] = {
        "01-return-left", "01-return-left", "01-return-left", "02-return-right",
        "03-both-return", "03-both-return", "03-both-return", "03-both-return",
        "04-return-if",   "04-return-if",   "03-both-return", "03-both-return"};
    for (size_t trial = 0; trial < 12; ++trial) {
        char path[1024];
        assert(snprintf(path, sizeof(path), "%s/%s.nl", argv[1], cases[trial]) >
               0);
        P256Input a = {0}, b = {0};
        p256_open(&a, path);
        p256_open(&b, path);
        NLCurrentLoopProof *s = a.proof->current_cycle,
                           *t = b.proof->current_cycle;
        assert(s->return_count && t->return_count);
        CurrentReturnSeal *r = &s->returns[trial == 2 || trial == 3 ? 1 : 0];
        NLControlExitView *e = (NLControlExitView *)nl_control_exit_view(
            r->ref.set, r->ref.ordinal);
        const NLControlExitView saved = *e;
        NLControlExitView *foreign = (NLControlExitView *)nl_control_exit_view(
            t->returns[0].ref.set, t->returns[0].ref.ordinal);
        bool unrestored = false;
        switch (trial) {
        case 0:
            e->target = foreign->target;
            p256_rejected(&a);
            *e = saved;
            break;
        case 1:
            e->state = foreign->state;
            p256_rejected(&a);
            *e = saved;
            break;
        case 2:
            e->count = 0;
            p256_rejected(&a);
            *e = saved;
            break;
        case 3:
            e->values[0].value = 0;
            p256_rejected(&a);
            *e = saved;
            break;
        case 4:
            e->kind = NL_EXIT_BREAK;
            p256_rejected(&a);
            *e = saved;
            break;
        case 5: {
            NLCheckedFragment *h = (NLCheckedFragment *)s->graphs[3].fragment;
            NLControlTarget *old = h->function_target;
            h->function_target = (NLControlTarget *)t->helper_target;
            p256_rejected(&a);
            h->function_target = old;
            break;
        }
        case 6: {
            NLSemanticContext *w = (NLSemanticContext *)r->world.world;
            NLValueId old = w->places[s->child - 1].current_value;
            w->places[s->child - 1].current_value = 0;
            p256_rejected(&a);
            w->places[s->child - 1].current_value = old;
            break;
        }
        case 7: {
            NLSemanticContext *w = (NLSemanticContext *)r->world.world;
            NLFunctionEntry *fn = &w->functions[s->helper_function - 1];
            NLTypeId *old = fn->parameters;
            void *short_allocation = malloc(1);
            assert(short_allocation);
            fn->parameters = short_allocation;
            p256_rejected(&a);
            fn->parameters = old;
            free(short_allocation);
            break;
        }
        case 8: {
            NLSemanticContext *w = (NLSemanticContext *)r->world.world;
            NLFunctionEntry *old = w->functions;
            w->functions = NULL;
            p256_rejected(&a);
            w->functions = old;
            break;
        }
        case 9: {
            NLCheckedFragment *iteration =
                (NLCheckedFragment *)s->graphs[1].fragment;
            NLCheckedNodeView *call = &iteration->nodes[s->scalar - 1];
            NLCheckedNodeView *arg =
                &iteration->nodes[call->first_argument - 1];
            NLSymbolId old = arg->symbol;
            arg->symbol = 0;
            p256_rejected(&a);
            arg->symbol = old;
            break;
        }
        case 10: {
            NLCheckedFragment *h = (NLCheckedFragment *)s->graphs[3].fragment;
            h->arms[0].artifact = (NLCheckedFragment *)t->graphs[4].fragment;
            p256_rejected(&a);
            unrestored = true;
            break;
        }
        case 11:
            e->state = foreign->state;
            p256_rejected(&a);
            unrestored = true;
            break;
        default:
            assert(0);
        }
        if (!unrestored)
            p256_valid(&a);
        p256_close(&a);
        p256_valid(&b);
        p256_close(&b);
        printf("C256 CHALLENGE trial=%zu current_reject/no_clobber=PASS "
               "restore=%s A_destroy/B_survive=PASS\n",
               trial + 1, unrestored ? "NO" : "YES");
    }
    puts("C256 twelve focused authority/state/ownership challenges PASS");
    return 0;
}
