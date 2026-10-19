/* Additional authorized ended-original-root controls. Real source only;
 * mutations are restored before original semantic/consumer revalidation. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

static bool valid(const TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}

static bool probe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n) && valid(&n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLCheckedNodeView *write = NULL;
    NLCheckedNodeId conditional = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(write == NULL);
            write = v;
        }
        if (v->kind == NL_CHECKED_IF)
            conditional = i;
    }
    CHECK(write != NULL && conditional != 0);
    NLSemanticContext *world = (NLSemanticContext *)nl_checked_context(f);
    CHECK(write->field.parent != 0 &&
          write->field.parent <= world->place_count);
    NLSemanticPlaceView *place = &world->places[write->field.parent - 1];
    const NLSemanticPlaceView saved = *place;
    CHECK(!saved.live && saved.independent_root && saved.implicit_local &&
          saved.current_fact == 0 && saved.current_value == 0);
    const NLCheckedFragment *arm = nl_checked_if_arm(f, conditional, 0);
    CHECK(arm != NULL && arm->context->last_value_fact != 0);
    TestState *before = malloc(sizeof(*before));
    CHECK(before != NULL && test_state(world, before));
    size_t rejected = 0;
#define REFUSE(change)                                                         \
    do {                                                                       \
        change;                                                                \
        const bool accepted = valid(&n);                                       \
        *place = saved;                                                        \
        CHECK(!accepted && test_unchanged(world, before) && valid(&n));        \
        ++rejected;                                                            \
    } while (0)
    REFUSE(place->current_fact = arm->context->last_value_fact);
    REFUSE(place->current_value = write->field.new_value);
    REFUSE(place->live = true);
#undef REFUSE
    printf("P634 ended_original_root=%zu fact=0 value=0 live=0 rejects=%zu "
           "genuine=PASS restored=UNCHANGED\n",
           write->field.parent, rejected);
    free(before);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
