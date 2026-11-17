#ifndef NEWLANG_C17_GUARDED_USIZE_INTERNAL_H
#define NEWLANG_C17_GUARDED_USIZE_INTERNAL_H
#include "guarded_usize_internal.h"

/* Typed lowering of the arithmetic issuer's already authenticated plan.
 * No AST interpretation, expected answer, new source admission or issuer. */
static bool guarded_usize_emit(FILE *out, const NLCheckedFragment *entry)
{
    NLGuardedUsizePlan main, helper;
    if (!nl_guarded_usize_call_valid(entry, nl_checked_root(entry), &main) ||
        main.formal || !main.call ||
        !nl_guarded_usize_call_valid(main.body, main.call, &helper) ||
        !helper.formal || !helper.conditional || helper.call)
        return false;
    const NLCheckedNodeView *outer = nl_checked_node_view(entry, entry->root);
    const NLCheckedNodeView *call = nl_checked_node_view(main.body, main.call);
    if (!outer || outer->argument_count || !call)
        return false;
    const NLCheckedNodeView *arg =
        nl_checked_node_view(main.body, call->first_argument);
    const NLCheckedNodeView *root =
        nl_checked_node_view(main.body, main.body->root);
    const NLCheckedFragment *arm =
        nl_checked_if_arm(helper.body, helper.conditional, 0);
    if (!arm)
        return false;
    const NLCheckedNodeView *ar = nl_checked_node_view(arm, arm->root);
    if (!ar)
        return false;
    const NLCheckedNodeView *block = nl_checked_node_view(arm, ar->initializer);
    if (!block)
        return false;
    const NLCheckedNodeView *binding =
        nl_checked_node_view(arm, block->first_item);
    if (!arg || !root || !binding)
        return false;
    /* Admission and both complete result seals precede the first byte. The
     * ordinary CLI owns named-output staging/rename and error cleanup. */
    if (fprintf(out,
                "/* NewLang source-owned guarded quantity C17. */\n"
                "#include <stddef.h>\n#include <stdint.h>\n"
                "_Static_assert(SIZE_MAX == UINTMAX_C(%zu), "
                "\"NewLang usize target mismatch\");\n\n"
                "static size_t nl_qty_fn_%zu(size_t nl_local_%zu)\n{\n"
                "    if (nl_local_%zu < ((size_t)UINTMAX_C(%zu))) {\n"
                "        size_t nl_local_%zu = nl_local_%zu + "
                "((size_t)UINTMAX_C(%zu));\n"
                "        return nl_local_%zu;\n"
                "    } else {\n        return nl_local_%zu;\n    }\n}\n\n"
                "static size_t nl_qty_fn_%zu(void)\n{\n",
                SIZE_MAX, helper.function, helper.formal, helper.formal,
                helper.limit, binding->symbol, helper.formal, helper.step,
                binding->symbol, helper.formal, main.function) < 0)
        return false;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(main.body, id);
        const NLCheckedNodeView *init =
            nl_checked_node_view(main.body, n->initializer);
        if (fprintf(out, "    size_t nl_local_%zu = ", n->symbol) < 0)
            return false;
        if (n->initializer == main.call) {
            if (fprintf(out, "nl_qty_fn_%zu(((size_t)UINTMAX_C(%zu)))",
                        helper.function, arg->scalar_result.value) < 0)
                return false;
        } else if (fprintf(out, "nl_local_%zu", init->symbol) < 0)
            return false;
        if (fputs(";\n", out) < 0)
            return false;
        id = n->next_item;
    }
    const NLCheckedNodeView *tail = nl_checked_node_view(main.body, root->tail);
    return fprintf(out,
                   "    return nl_local_%zu;\n}\n\n"
                   "int main(void)\n{\n    (void)nl_qty_fn_%zu();\n    return "
                   "0;\n}\n",
                   tail->symbol, main.function) >= 0;
}
#endif
