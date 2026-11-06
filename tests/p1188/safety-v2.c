/* Reuse the existing P-owned ownership challenges on the new frozen sources.
 * The additional matrix targets each actual usize Copy declaration/use. */
#include "p1188_predecessor_safety.h"

int main(int argc, char **argv)
{
    assert(argc == 2);
    Input a = {0};
    parse(&a, argv[1]);
    assert(registration(&a) == NL_CHECK_OK && invocation(&a) == NL_CHECK_OK);
    NLLinearBreakView proof;
    assert(valid(&a, &proof));
    NLCheckedFragment *caller = owner(&a);
    NLCheckedFragment *iteration = (NLCheckedFragment *)proof.iteration;
    NLPureU8Call pure;
    assert(nl_pure_u8_call(iteration, proof.incoming_call, &pure));
    NLCheckedFragment *fragments[] = {caller, iteration,
                                      (NLCheckedFragment *)pure.body};
    size_t trials = 0, declarations = 0;
    for (size_t f = 0; f < 3; ++f) {
        NLCheckedFragment *fragment = fragments[f];
        const NLTypeId quantity =
            nl_semantic_core_type(fragment->context, NL_TYPE_USIZE);
        for (size_t i = 0; i < fragment->count; ++i) {
            NLCheckedNodeView *node = &fragment->nodes[i];
            if (node->kind != NL_CHECKED_BINDING)
                continue;
            const NLCheckedNodeView *receiver =
                nl_checked_node_view(fragment, node->first_argument);
            NLCheckedNodeView *init = &fragment->nodes[node->initializer - 1];
            if (!receiver || receiver->type != quantity ||
                (init->kind != NL_CHECKED_IDENTIFIER &&
                 init->kind != NL_CHECKED_USIZE_LITERAL))
                continue;
            ++declarations;
            NLCheckedNodeView saved = *node, saved_init = *init;
            NLBindingEntry *binding = &((NLSemanticContext *)fragment->context)
                                           ->bindings[node->symbol - 1];
            NLBindingEntry saved_binding = *binding;
#define COPY_BAD(mutation, restoration)                                        \
    do {                                                                       \
        mutation;                                                              \
        NLLinearBreakView untouched, before;                                   \
        memset(&untouched, 0xA7, sizeof untouched);                            \
        memcpy(&before, &untouched, sizeof before);                            \
        assert(nl_linear_break_validate(caller, loop_id(&a), &untouched) !=    \
               NL_CHECK_OK);                                                   \
        assert(!memcmp(&before, &untouched, sizeof before));                   \
        restoration;                                                           \
        assert(valid(&a, &proof));                                             \
        ++trials;                                                              \
    } while (0)
            COPY_BAD(node->initializer = i + 1, *node = saved);
            COPY_BAD(node->symbol = 0, *node = saved);
            COPY_BAD(node->name.start_byte++, *node = saved);
            COPY_BAD(init->type = proof.nominal, *init = saved_init);
            COPY_BAD(init->results[0].value = 0, *init = saved_init);
            COPY_BAD(binding->view.value = 0, *binding = saved_binding);
            if (init->kind == NL_CHECKED_IDENTIFIER) {
                COPY_BAD(init->symbol = 0, *init = saved_init);
                COPY_BAD(init->value_use = NL_VALUE_CONSUMED,
                         *init = saved_init);
            }
            NLSemanticContext *world = (NLSemanticContext *)fragment->context;
            NLSemanticValueView *value =
                &world->values[init->results[0].value - 1];
            NLSemanticValueView saved_value = *value;
            COPY_BAD(value->dependencies = NL_DEPENDENCIES_UNKNOWN,
                     *value = saved_value);
            COPY_BAD(value->scalar_known = !value->scalar_known,
                     *value = saved_value);
        }
    }
    printf(
        "C247 Copy-declarations=%zu restored-tamper=%zu view-no-clobber=PASS\n",
        declarations, trials);
    dispose(&a);
    return p1188_previous_safety_main(argc, argv);
}
