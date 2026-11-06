/* Real issued A/B instances; the inherited value never becomes a writer
 * result or a new field-current capability. No forged instance is executed. */
#include "p1188_predecessor_safety.h"

static NLCheckedNodeView *declaration(NLCheckedFragment *f, const char *name)
{
    for (size_t i = 0; i < f->count; ++i) {
        NLCheckedNodeView *n = &f->nodes[i];
        if (n->kind == NL_CHECKED_BINDING && n->symbol &&
            !strcmp(f->context->bindings[n->symbol - 1].name, name))
            return n;
    }
    return NULL;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    Input a = {0}, b = {0};
    parse(&a, argv[1]);
    assert(registration(&a) == NL_CHECK_OK && invocation(&a) == NL_CHECK_OK);
    NLLinearBreakView proof;
    assert(valid(&a, &proof));
    NLCheckedFragment *caller = owner(&a);
    NLCheckedFragment *iteration = (NLCheckedFragment *)proof.iteration;
    NLCheckedNodeView *held = declaration(caller, "held"),
                      *same = declaration(caller, "same_bits"),
                      *local = declaration(iteration, "capture_before");
    assert(held && same && local);
    NLCheckedNodeView *use = &iteration->nodes[local->initializer - 1];
    assert(use->kind == NL_CHECKED_IDENTIFIER && use->symbol == held->symbol);
    const NLCheckedNodeView saved_use = *use, saved_held = *held;
    NLSemanticContext *world = (NLSemanticContext *)iteration->context;
    NLBindingEntry *binding = &world->bindings[held->symbol - 1];
    const NLBindingEntry saved_binding = *binding;
    NLSemanticValueView *package = &world->values[binding->view.value - 1];
    const NLSemanticValueView saved_package = *package;
    assert(package->scalar_known &&
           world->values[world->bindings[same->symbol - 1].view.value - 1]
                   .scalar_value == package->scalar_value);
    size_t trials = 0;
#define REJECT(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        NLLinearBreakView output, before;                                      \
        memset(&output, 0xB3, sizeof output);                                  \
        memcpy(&before, &output, sizeof before);                               \
        assert(nl_linear_break_validate(caller, a.retained_loop, &output) !=   \
               NL_CHECK_OK);                                                   \
        assert(!memcmp(&before, &output, sizeof before));                      \
        restore;                                                               \
        assert(valid(&a, &proof));                                             \
        ++trials;                                                              \
    } while (0)
    /* 1: Equal bits are not the actual source symbol/declaration/Copy. */
    REJECT(use->symbol = same->symbol, *use = saved_use);
    REJECT(held->initializer = same->initializer, *held = saved_held);
    REJECT(use->results[0].value = world->bindings[same->symbol - 1].view.value,
           *use = saved_use);
    REJECT(use->name = same->name, *use = saved_use);
    /* 2: A captured old Copy grants neither consume nor writer authority. */
    REJECT(use->value_use = NL_VALUE_CONSUMED, *use = saved_use);
    NLCheckedNodeView *writer = NULL;
    for (size_t i = 0; i < iteration->count; ++i)
        if (iteration->nodes[i].kind == NL_CHECKED_REPLACE)
            writer = &iteration->nodes[i];
    assert(writer);
    const NLCheckedNodeView saved_writer = *writer;
    REJECT(writer->field.new_value = binding->view.value,
           *writer = saved_writer);
    /* 3: Actual enclosing lexical ownership and declaration order. */
    REJECT(held->name.start_byte++, *held = saved_held);
    REJECT(held->initializer = (NLCheckedNodeId)(held - caller->nodes) + 1,
           *held = saved_held);
    const NLSource *saved_source = caller->source;
    REJECT(caller->source = a.entry, caller->source = saved_source);
    /* 4: Prefix availability and lexical visibility. */
    REJECT(binding->view.availability = NL_CONSUMED, *binding = saved_binding);
    REJECT(binding->hidden = true, *binding = saved_binding);
    /* 5: Preserved value package and dependency-free entry/header identity. */
    REJECT(binding->view.value = world->bindings[same->symbol - 1].view.value,
           *binding = saved_binding);
    REJECT(package->dependencies = NL_DEPENDENCIES_UNKNOWN,
           *package = saved_package);
    REJECT(package->scalar_value++, *package = saved_package);
#undef REJECT
    /* 6: Display substitution does not transfer B's original ownership to A. */
    parse(&b, argv[1]);
    assert(registration(&b) == NL_CHECK_OK && invocation(&b) == NL_CHECK_OK);
    NLLinearBreakView other;
    assert(valid(&b, &other));
    NLCheckedFragment *ga =
        (NLCheckedFragment *)nl_checked_loop_body(caller, loop_id(&a));
    NLCheckedFragment *gb =
        (NLCheckedFragment *)nl_checked_loop_body(owner(&b), loop_id(&b));
    assert(ga && gb && ga->arm_count == 2 && gb->arm_count == 2);
    ga->arms[0] = gb->arms[0];
    NLLinearBreakView output, before;
    memset(&output, 0xB3, sizeof output);
    memcpy(&before, &output, sizeof before);
    assert(nl_linear_break_validate(caller, a.retained_loop, &output) !=
           NL_CHECK_OK);
    assert(!memcmp(&before, &output, sizeof before));
    dispose(
        &a); /* Unrestored A dies according to its canonical child ledger. */
    assert(valid(&b, &other));
    dispose(&b);
    printf("C254 capture categories=6 restored=%zu foreign=1 "
           "out-no-clobber/B-survive/cleanup=PASS\n",
           trials);
    return p1188_previous_safety_main(argc, argv);
}
