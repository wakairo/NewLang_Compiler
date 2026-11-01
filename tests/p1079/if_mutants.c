#include "../p1062/support.h"
#include "newlang/if_field.h"
static NLCheckedNodeId conditional(const NLCheckedFragment *f)
{
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id)
        if (nl_checked_node_view(f, id)->kind == NL_CHECKED_IF)
            return id;
    return 0;
}
static bool refused(TestNode *a, NLCheckedFragment *f, NLCheckedNodeId id)
{
    NLIfFieldView historical;
    CHECK(nl_checked_if_field_view(f, id, &historical) &&
          nl_checked_if_field_validate(f, id, &historical) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          p1062_unpublished(a));
    return true;
}
static bool run(const char *path)
{
    NLSemanticContext *ba = NULL, *bb = NULL;
    TestNode a = {0}, b = {0};
    P1062Call x[2], y[2];
    CHECK(registry(path, &ba) && checked(ba, &a) && p1062_calls(&a, x) &&
          p1062_valid(x) && registry(path, &bb) && checked(bb, &b) &&
          p1062_calls(&b, y) && p1062_valid(y));
    NLCheckedFragment *f = x[0].caller, *g = y[0].caller;
    NLCheckedNodeId id = conditional(f), other = conditional(g);
    NLIfFieldView v, w;
    CHECK(id && other && nl_checked_if_field_view(f, id, &v) &&
          nl_checked_if_field_view(g, other, &w) &&
          nl_checked_if_field_validate(f, id, &v) == NL_CHECK_OK &&
          nl_checked_if_field_validate(g, other, &w) == NL_CHECK_OK);
    char *expected = NULL;
    size_t bytes = 777;
    CHECK(nl_checked_c_node(a.entry, &expected, &bytes) == NL_NODE_C_OK &&
          expected);
    NLCheckedNodeView *choice = &f->nodes[id - 1], saved = *choice;
    for (size_t fault = 0; fault < 5; ++fault) {
        if (fault == 0)
            choice->normal_arms = 1;
        else if (fault == 1)
            choice->initializer = 0;
        else if (fault == 2)
            choice->results[0].value = v.result.value + 1;
        else if (fault == 3)
            choice->item_count = 1;
        else
            choice->terminates = true;
        CHECK(refused(&a, f, id));
        *choice = saved;
        CHECK(p1062_exact(&a, expected, bytes));
        printf(
            "P1079 IF REJECT parent=%zu historical_GET=YES exact_retry=YES\n",
            fault);
    }
    for (size_t arm = 0; arm < 2; ++arm) {
        NLIfFieldEvent event;
        CHECK(nl_checked_if_field_event(f, id, arm, 0, &event));
        NLCheckedFragment *af = (NLCheckedFragment *)v.arms[arm].artifact;
        NLCheckedNodeView *change = &af->nodes[event.node - 1], old = *change;
        const NLSemanticContext *world = af->context;
        const NLSource *source = af->source;
        for (size_t fault = 0; fault < 7; ++fault) {
            if (fault == 0)
                change->field.child = v.sibling;
            else if (fault == 1)
                change->field.index ^= 1;
            else if (fault == 2)
                ++change->field.child_fact;
            else if (fault == 3)
                ++change->field.child_incarnation;
            else if (fault == 4)
                change->field.new_value = event.result;
            else if (fault == 5)
                af->context = w.arms[arm].world;
            else
                af->source = w.arms[arm].artifact->source;
            CHECK(refused(&a, f, id));
            *change = old;
            af->context = world;
            af->source = source;
            CHECK(p1062_exact(&a, expected, bytes));
            printf("P1079 IF REJECT arm=%zu fault=%zu exact_retry=YES\n", arm,
                   fault);
        }
    }
    for (size_t fault = 0; fault < 4; ++fault) {
        NLIfFieldView fake = v;
        if (fault == 0)
            fake.child = v.sibling;
        else if (fault == 1)
            fake.root = x[0].parameters.field.parent;
        else if (fault == 2)
            fake.post = w.post;
        else
            fake.arms[0] = w.arms[0];
        CHECK(nl_checked_if_field_validate(f, id, &fake) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
              p1062_exact(&a, expected, bytes));
        printf("P1079 IF VIEW REJECT fault=%zu original_current=YES\n", fault);
    }
    const size_t count = f->arm_count;
    for (size_t fault = 0; fault < 2; ++fault) {
        f->arm_count = fault ? count + 1 : 0;
        CHECK(p1062_unpublished(&a));
        f->arm_count = count;
        CHECK(p1062_exact(&a, expected, bytes));
        printf("P1079 IF COUNT REJECT fault=%zu exact_retry=YES\n", fault);
    }
    size_t ai = 0, bi = 0;
    while (ai < f->arm_count && f->arms[ai].match != id)
        ++ai;
    while (bi < g->arm_count && g->arms[bi].match != other)
        ++bi;
    CHECK(ai + 1 < f->arm_count && bi + 1 < g->arm_count);
    f->arms[ai] = g->arms[bi];
    f->arms[ai + 1] = g->arms[bi + 1];
    CHECK(p1062_unpublished(&a));
    node_checked_destroy(&a);
    CHECK(nl_checked_if_field_validate(g, other, &w) == NL_CHECK_OK &&
          p1062_valid(y) && p1062_exact(&b, expected, bytes));
    node_checked_destroy(&b);
    nl_semantic_destroy(ba);
    nl_semantic_destroy(bb);
    free(expected);
    puts("P1079 IF OWNER foreign_B_arms_A_destroyed_unrestored=YES "
         "B_current_exact_destroy=YES");
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    return argc == 2 && run(argv[1]) ? 0 : 1;
}
