#ifndef NEWLANG_C17_GUARDED_USIZE_INTERNAL_H
#define NEWLANG_C17_GUARDED_USIZE_INTERNAL_H
#include "guarded_usize_internal.h"

/* Typed lowering of the arithmetic issuer's already authenticated plan.
 * No AST interpretation, expected answer, new source admission or issuer. */
static bool guarded_usize_helper_emit(FILE *out, const NLGuardedUsizePlan *p)
{
    const NLCheckedFragment *arm =
        nl_checked_if_arm(p->body, p->conditional, 0);
    const NLCheckedNodeView *ar = nl_checked_node_view(arm, arm->root);
    const NLCheckedNodeView *block = nl_checked_node_view(arm, ar->initializer);
    const NLCheckedNodeView *binding =
        nl_checked_node_view(arm, block->first_item);
    return fprintf(out,
                   "static size_t nl_qty_fn_%zu(size_t nl_local_%zu)\n{\n"
                   "    if (nl_local_%zu < ((size_t)UINTMAX_C(%zu))) {\n"
                   "        size_t nl_local_%zu = nl_local_%zu + "
                   "((size_t)UINTMAX_C(%zu));\n"
                   "        return nl_local_%zu;\n"
                   "    } else {\n        return nl_local_%zu;\n    }\n}\n\n",
                   p->function, p->formal, p->formal, p->limit, binding->symbol,
                   p->formal, p->step, binding->symbol, p->formal) >= 0;
}
static bool guarded_usize_emit(FILE *out, const NLCheckedFragment *entry)
{
    NLGuardedUsizePlan main;
    if (!nl_guarded_usize_call_valid(entry, nl_checked_root(entry), &main) ||
        main.formal || !main.call)
        return false;
    const NLCheckedNodeView *outer = nl_checked_node_view(entry, entry->root);
    const NLCheckedNodeView *root =
        nl_checked_node_view(main.body, main.body->root);
    if (!outer || outer->argument_count || !root)
        return false;
    /* Bounded by the existing 64 actual body custody slots. No persistent
     * receipt, new issuer or copied semantic world: borrow during emission. */
    NLGuardedUsizePlan helpers[64];
    size_t count = 0;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(main.body, id);
        const NLCheckedNodeView *init =
            nl_checked_node_view(main.body, n->initializer);
        if (init->kind == NL_CHECKED_REGISTERED_CALL) {
            NLGuardedUsizePlan helper;
            if (!nl_guarded_usize_call_valid(main.body, n->initializer,
                                             &helper) ||
                !helper.formal || !helper.conditional || helper.call)
                return false;
            size_t i = 0;
            while (i < count && helpers[i].function != helper.function)
                ++i;
            if (i == count) {
                if (count == 64)
                    return false;
                helpers[count++] = helper;
            }
        }
        id = n->next_item;
    }
    /* All actual calls, complete result/world seals and Copy operands are
     * authenticated before the first byte. CLI stages named output. */
    if (fprintf(out,
                "/* NewLang source-owned guarded quantity C17. */\n"
                "#include <stddef.h>\n#include <stdint.h>\n"
                "_Static_assert(SIZE_MAX == UINTMAX_C(%zu), "
                "\"NewLang usize target mismatch\");\n\n",
                SIZE_MAX) < 0)
        return false;
    for (size_t i = 0; i < count; ++i)
        if (!guarded_usize_helper_emit(out, &helpers[i]))
            return false;
    if (fprintf(out, "static size_t nl_qty_fn_%zu(void)\n{\n", main.function) <
        0)
        return false;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(main.body, id);
        const NLCheckedNodeView *init =
            nl_checked_node_view(main.body, n->initializer);
        if (fprintf(out, "    size_t nl_local_%zu = ", n->symbol) < 0)
            return false;
        if (init->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedNodeView *arg =
                nl_checked_node_view(main.body, init->first_argument);
            if (fprintf(out, "nl_qty_fn_%zu(", init->function) < 0)
                return false;
            if (arg->kind == NL_CHECKED_USIZE_LITERAL) {
                if (fprintf(out, "((size_t)UINTMAX_C(%zu))",
                            arg->scalar_result.value) < 0)
                    return false;
            } else if (fprintf(out, "nl_local_%zu", arg->symbol) < 0)
                return false;
            if (fputs(")", out) < 0)
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
