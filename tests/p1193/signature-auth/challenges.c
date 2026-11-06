/* P-owned F-1198-01 regression. Never execute generated guests on mutations.
 * Reuse only P's already frozen genuine-source setup; its old tests stay exact.
 */
#define main c256_preserved_challenges_main
#include "../production/challenges.c"
#undef main

static NLSemanticContext *auth_world(P256Input *a, size_t slot)
{
    NLCurrentLoopProof *s = a->proof->current_cycle;
    if (slot < (s->helper ? 6u : s->scalar ? 5u : 3u))
        return (NLSemanticContext *)s->graphs[slot].world.world;
    if (slot == 6)
        return (NLSemanticContext *)a->body->context;
    if (slot == 7)
        return a->proof->post;
    if (slot >= 8 && slot < 12)
        return (NLSemanticContext *)s->exits[slot - 8].world;
    if (slot >= 12 && slot - 12 < s->return_count)
        return (NLSemanticContext *)s->returns[slot - 12].world.world;
    return NULL;
}

static size_t auth_extra(const NLSemanticContext *w)
{
    for (size_t i = 0; i < w->function_count; ++i)
        if (!strcmp(w->functions[i].name, "audit_unused")) {
            assert(w->functions[i].count == 2);
            return i;
        }
    assert(0);
    return 0;
}

static void auth_valid(P256Input *a)
{
    NLLinearBreakView v;
    assert(nl_h_custody_validate(a->root));
    assert(nl_linear_break_validate(a->body, a->proof->loop, &v) ==
           NL_CHECK_OK);
    assert(v.changing_current &&
           (a->proof->current_cycle->helper ? v.incoming_call : v.incoming_if));
}

static void auth_trial(const char *path, size_t slot, size_t family)
{
    P256Input a = {0}, b = {0};
    p256_open(&a, path);
    p256_open(&b, path);
    NLSemanticContext *w = auth_world(&a, slot),
                      *foreign = auth_world(&b, slot);
    assert(w && foreign);
    size_t extra = auth_extra(w), other = auth_extra(foreign);
    NLFunctionEntry *fn = &w->functions[extra];
    NLTypeId *parameters = fn->parameters;
    size_t count = fn->count, functions = w->function_count;
    NLFunctionEntry *table = w->functions;
    NLTypeId first = parameters[0];
    void *short_allocation = NULL;
    printf("AUTH BEFORE slot=%zu family=%zu functions=%zu extra_index=%zu "
           "original_elements=%zu element_bytes=%zu helper_index=%zu\n",
           slot, family, functions, extra, count, sizeof(NLTypeId),
           a.proof->current_cycle->helper_function);
    switch (family) {
    case 1: {
        assert(a.proof->current_cycle->helper);
        NLFunctionEntry *helper =
            &w->functions[a.proof->current_cycle->helper_function - 1];
        assert(helper->count == 1 && helper->parameters &&
               helper->parameters[0] == parameters[0] &&
               parameters[0] == parameters[1]);
        printf("AUTH FOREIGN live_helper_elements=1 requested_elements=2\n");
        fn->parameters = helper->parameters;
        break;
    }
    case 2:
        short_allocation = malloc(1);
        assert(short_allocation);
        fn->parameters = short_allocation;
        break;
    case 3:
        fn->parameters = NULL;
        break;
    case 4:
        fn->count = 1;
        break;
    case 5:
        fn->count = SIZE_MAX;
        break;
    case 6:
        fn->parameters = foreign->functions[other].parameters;
        break;
    case 7:
        w->functions = NULL;
        break;
    case 8:
        w->functions = foreign->functions;
        break;
    case 9:
        parameters[0] = nl_semantic_core_type(w, NL_TYPE_U8);
        break;
    case 10:
        w->function_count = 1;
        break;
    case 11:
        w->function_count = SIZE_MAX;
        break;
    default:
        assert(0);
    }
    p256_rejected(&a);
    w->functions = table;
    w->function_count = functions;
    fn->parameters = parameters;
    fn->count = count;
    parameters[0] = first;
    free(short_allocation);
    auth_valid(&a);
    p256_close(&a);
    auth_valid(&b);
    p256_close(&b);
    printf("AUTH PASS slot=%zu family=%zu reject/no_clobber/restore/retry/"
           "A_destroy/B_survive=YES\n",
           slot, family);
}

int main(int argc, char **argv)
{
    assert(argc == 2 || argc == 4);
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc == 4) {
        auth_trial(argv[1], (size_t)strtoul(argv[2], NULL, 10),
                   (size_t)strtoul(argv[3], NULL, 10));
        return 0;
    }
    P256Input sample = {0};
    p256_open(&sample, argv[1]);
    NLCurrentLoopProof *sealed = sample.proof->current_cycle;
    size_t slots = 0, entries = 0;
    for (size_t slot = 0; slot < 16; ++slot) {
        NLSemanticContext *w = auth_world(&sample, slot);
        if (!w)
            continue;
        ++slots;
        entries += w->function_count;
    }
    assert(sealed->signature_entries &&
           sealed->signature_entry_count == entries);
    printf(
        "AUTH LEDGER slots=%zu entries=%zu dynamic_bytes=%zu proof_size=%zu\n",
        slots, entries, entries * sizeof(*sealed->signature_entries),
        sizeof(*sealed));
    p256_close(&sample);
    size_t trials = 0;
    for (size_t slot = 0; slot < 16; ++slot) {
        P256Input a = {0};
        p256_open(&a, argv[1]);
        bool present = auth_world(&a, slot) != NULL;
        bool helper = a.proof->current_cycle->helper;
        p256_close(&a);
        if (!present)
            continue;
        for (size_t family = helper ? 1 : 2; family <= 11; ++family) {
            /* The post clone has no all-function content comparison. Its
             * selected helper is authenticated separately; an unrelated
             * same-allocation content mutation is outside this read set. */
            if (slot == 7 && family == 9)
                continue;
            auth_trial(argv[1], slot, family);
            ++trials;
        }
    }
    printf("AUTH COMPLETE trials=%zu families=11\n", trials);
    return 0;
}
