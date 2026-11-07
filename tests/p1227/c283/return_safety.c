/* Unchanged strict old safety oracle plus real independent RETURN instances. */
#define main p1227_original_safety_main
#include "../safety.c"
#undef main

typedef struct {
    NLCheckedFragment *body, *arm;
    NLCheckedNodeView *choice, *ret;
    size_t index;
} ReturnSite;
static ReturnSite return_site(Input *input)
{
    ReturnSite s = {.body = (NLCheckedFragment *)input->view.helper_body};
    s.choice = &s.body->nodes[input->view.helper_selection - 1];
    for (size_t i = 0; i < 2; ++i) {
        NLCheckedFragment *arm = (NLCheckedFragment *)nl_checked_if_arm(
            s.body, input->view.helper_selection, i);
        const NLCheckedNodeView *root =
            nl_checked_node_view(arm, nl_checked_root(arm));
        if (!root->terminates)
            continue;
        const NLCheckedNodeView *block =
            nl_checked_node_view(arm, root->initializer);
        assert(!s.arm && block && block->first_item);
        s.arm = arm;
        s.index = i;
        s.ret = &arm->nodes[block->first_item - 1];
        assert(s.ret->kind == NL_CHECKED_RETURN &&
               nl_control_exits_count(arm->exits) == 1);
    }
    assert(s.arm && s.ret);
    return s;
}
int main(int argc, char **argv)
{
    assert(p1227_original_safety_main(argc, argv) == 0);
    Input a = {0}, b = {0};
    load(&a, argv[1]);
    load(&b, argv[1]);
    accepted(&a);
    accepted(&b);
    ReturnSite x = return_site(&a), y = return_site(&b);
    assert(x.ret->returned.value == y.ret->returned.value &&
           x.ret->returned.type == y.ret->returned.type &&
           x.arm->context != y.arm->context &&
           x.body->function_target != y.body->function_target);
    const NLSemanticContext *world = x.arm->context;
    x.arm->context = y.arm->context; /* same IDs, genuine foreign world */
    rejected(&a);
    x.arm->context = world;
    NLControlExits *exits = x.arm->exits;
    x.arm->exits = y.arm->exits; /* genuine foreign RETURN package */
    rejected(&a);
    x.arm->exits = exits;
    NLControlTarget *target = x.body->function_target;
    x.body->function_target = y.body->function_target;
    rejected(&a);
    x.body->function_target = target;
    NLCheckedNodeView saved = *x.ret;
    x.ret->returned.value++;
    rejected(&a);
    *x.ret = saved;
    saved = *x.choice;
    x.choice->normal_arms = 2;
    rejected(&a);
    *x.choice = saved;
    accepted(&a);
    /* Destruction follows original canonical children, not display arms.
     * Leave this whole genuine foreign RETURN arm unrestored at A destroy. */
    bool replaced = false;
    for (size_t i = 0; i < x.body->arm_count; ++i)
        if (x.body->arms[i].artifact == x.arm) {
            x.body->arms[i].artifact = y.arm;
            replaced = true;
            break;
        }
    assert(replaced);
    rejected(&a);
    dispose(&a);
    accepted(&b);
    dispose(&b);
    puts("C283 six RETURN/source-world/target/node/foreign-arm refusals; "
         "same-number foreign RETURN never authenticates; canonical A "
         "destroy/B survive PASS");
    return 0;
}
