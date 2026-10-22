/* P-owned post-finding diagnostic, genuine source only; never edits R views. */
#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include "newlang/if_field.h"
static bool examine(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *caller =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    size_t found = 0;
    for (size_t id = 1; id <= caller->count; ++id) {
        NLCopyUnitView v;
        if (!nl_checked_copy_unit_view(caller, id, &v))
            continue;
        NLIfReturnView ir;
        CHECK(nl_checked_if_return_view(v.body, v.conditional, &ir));
        const NLCheckedNodeView *loan =
            nl_checked_node_view(caller, v.parameters.loan);
        const NLCheckedNodeView *block =
            nl_checked_node_view(caller, loan->initializer);
        const NLCopyBodyProof *p = v.body->copy_body;
        printf("call=%zu function=%zu root=%zu child=%zu field=%zu scope=%zu "
               "normal_arm=%zu worlds entry=%p handoff=%p IF_entry=%p "
               "IF_post=%p return_live=%p ended=%p\n",
               id, v.function, v.parameters.field.parent,
               v.parameters.field.child, v.parameters.field.index,
               loan->loan.scope, ir.normal_arm, (void *)v.parameters.entry,
               (void *)v.postcall, (void *)ir.field.entry,
               (void *)ir.field.post, (void *)ir.return_live,
               (void *)ir.return_ended);
        printf("loan block=%zu tail=%zu first_item=%zu item_count=%zu "
               "result_count=%zu type=%zu terminates=%d call_unit=%d\n",
               loan->initializer, block->tail, block->first_item,
               block->item_count, block->result_count, block->type,
               block->terminates, p->unit_call);
        for (size_t item = block->first_item; item;) {
            const NLCheckedNodeView *it = nl_checked_node_view(caller, item);
            printf("loan item=%zu kind=%d initializer=%zu next=%zu\n", item,
                   it->kind, it->initializer, it->next_item);
            item = it->next_item;
        }
        printf("unit_status=%d if_status=%d parameters_status=%d\n",
               nl_checked_copy_unit_validate(caller, id, &v),
               nl_checked_if_return_validate(v.body, v.conditional, &ir),
               nl_checked_copy_body_parameters_validate(caller, id,
                                                        &v.parameters));
        ++found;
    }
    CHECK(found == 1);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && examine(argv[1]) ? 0 : 1;
}
