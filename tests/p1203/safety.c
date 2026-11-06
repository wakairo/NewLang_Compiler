/* P-owned local continuation of existing genuine A/B ownership machinery. */
#define main p1193_original_challenges
#include "../p1193/production/challenges.c"
#undef main

int main(int argc, char **argv)
{
    assert(argc == 2);
    setvbuf(stdout, NULL, _IONBF, 0);
    for (size_t trial = 0; trial < 12; ++trial) {
        P256Input a = {0}, b = {0};
        p256_open(&a, argv[1]);
        p256_open(&b, argv[1]);
        NLCurrentLoopProof *s = a.proof->current_cycle;
        NLCurrentLoopProof *t = b.proof->current_cycle;
        NLCheckedFragment *arm = (NLCheckedFragment *)s->graphs[2].fragment;
        NLCheckedFragment *other = (NLCheckedFragment *)t->graphs[2].fragment;
        NLControlExitView *edge =
            (NLControlExitView *)nl_control_exit_view(arm->exits, 0);
        const NLControlExitView saved = *edge;
        const NLControlExitView *foreign =
            nl_control_exit_view(other->exits, 0);
        NLCheckedNodeView *terminal = &arm->nodes[edge->source_node - 1];
        NLCheckedNodeView *value = &arm->nodes[terminal->initializer - 1];
        assert(value->kind == NL_CHECKED_USIZE_LITERAL);
        bool unrestored = false;
        switch (trial) {
        case 0:
            edge->target = foreign->target;
            p256_rejected(&a);
            *edge = saved;
            break;
        case 1:
            edge->state = foreign->state;
            p256_rejected(&a);
            *edge = saved;
            break;
        case 2:
            edge->count = 0;
            p256_rejected(&a);
            *edge = saved;
            break;
        case 3:
            edge->values[0].type = nl_semantic_core_type(a.world, NL_TYPE_U8);
            p256_rejected(&a);
            *edge = saved;
            break;
        case 4: {
            const NLCheckedNodeView original = *value;
            ++value->span.start_byte;
            p256_rejected(&a);
            *value = original;
            break;
        }
        case 5: {
            NLSemanticContext *world = (NLSemanticContext *)s->exits[1].world;
            const NLValueId id = edge->values[0].value;
            const NLSemanticValueView original = world->values[id - 1];
            ++world->values[id - 1].scalar_value;
            p256_rejected(&a);
            world->values[id - 1] = original;
            break;
        }
        case 6: {
            NLSemanticContext *world = (NLSemanticContext *)s->exits[1].world;
            const NLValueId original =
                world->places[s->child - 1].current_value;
            world->places[s->child - 1].current_value = 0;
            p256_rejected(&a);
            world->places[s->child - 1].current_value = original;
            break;
        }
        case 7: {
            NLCheckedFragment *iteration =
                (NLCheckedFragment *)s->graphs[1].fragment;
            NLCheckedNodeView *loan = NULL;
            for (size_t i = 0; i < iteration->count; ++i)
                if (iteration->nodes[i].kind == NL_CHECKED_LOAN_HEADER)
                    loan = &iteration->nodes[i];
            assert(loan);
            const NLCheckedNodeView original = *loan;
            loan->loan.access = NL_ACCESS_READ;
            p256_rejected(&a);
            *loan = original;
            break;
        }
        case 8: {
            NLSemanticContext *world = (NLSemanticContext *)s->exits[1].world;
            const size_t original = world->places[s->root - 1].current_fact;
            ++world->places[s->root - 1].current_fact;
            p256_rejected(&a);
            world->places[s->root - 1].current_fact = original;
            break;
        }
        case 9: {
            NLSemanticContext *world =
                (NLSemanticContext *)s->graphs[3].fragment->context;
            NLFunctionEntry *fn = &world->functions[s->helper_function - 1];
            NLTypeId *original = fn->parameters;
            void *short_array = malloc(1);
            assert(short_array);
            fn->parameters = short_array;
            p256_rejected(&a);
            fn->parameters = original;
            free(short_array);
            break;
        }
        case 10:
            /* Exact canonical child custody, even without restoring A's
             * display arm. A must not own or destroy B's genuine child. */
            for (size_t i = 0; i < s->graph->arm_count; ++i)
                if (s->graph->arms[i].artifact == arm)
                    s->graph->arms[i].artifact = other;
            p256_rejected(&a);
            unrestored = true;
            break;
        case 11:
            edge->source_owner = other;
            p256_rejected(&a);
            *edge = saved;
            break;
        default:
            assert(0);
        }
        if (!unrestored)
            p256_valid(&a);
        p256_close(&a);
        p256_valid(&b);
        p256_close(&b);
        printf("P1203 case=%zu "
               "rejection/out-no-clobber/retry/A-destroy/B-survive PASS\n",
               trial);
    }
    return 0;
}
