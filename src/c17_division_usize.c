#include "division_usize_internal.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char *bytes;
    size_t length, capacity;
} DivisionText;
static bool division_text_append(DivisionText *t, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    const int count =
        vsnprintf(t->bytes + t->length, t->capacity - t->length, format, args);
    va_end(args);
    if (count < 0 || (size_t)count >= t->capacity - t->length)
        return false;
    t->length += (size_t)count;
    return true;
}
static bool atom(DivisionText *t, const NLCheckedFragment *f,
                 NLCheckedNodeId id)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (n->kind == NL_CHECKED_IDENTIFIER)
        return division_text_append(t, "nl_local_%zu", n->symbol);
    return n->kind == NL_CHECKED_USIZE_LITERAL &&
           division_text_append(t, "((size_t)UINTMAX_C(%zu))",
                                n->scalar_result.value);
}
NLCheckStatus nl_division_usize_c17(const NLCheckedFragment *entry, char **out,
                                    size_t *length)
{
    NLDivisionUsizePlan main, helper;
    if (!out || !length ||
        !nl_division_usize_call_valid(entry, nl_checked_root(entry), &main) ||
        main.formal || !main.call ||
        !nl_division_usize_call_valid(main.body, main.call, &helper) ||
        !helper.formal)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    const NLCheckedNodeView *root =
        nl_checked_node_view(main.body, main.body->root);
    if (main.body->count > (SIZE_MAX - 2048) / 512)
        return NL_CHECK_RESOURCE_LIMIT;
    /* A bounded rendering buffer, not another plan/world/authority copy.
     * All source events and the complete committed world are validated before
     * allocating or producing output. Both caller outputs commit together. */
    DivisionText text = {.capacity = main.body->count * 512 + 2048};
    text.bytes = malloc(text.capacity);
    if (!text.bytes)
        return NL_CHECK_OUT_OF_MEMORY;
    if (!division_text_append(
            &text,
            "/* NewLang source-owned quantity division C17. */\n"
            "#include <stddef.h>\n#include <stdint.h>\n"
            "_Static_assert(SIZE_MAX == UINTMAX_C(%zu), "
            "\"NewLang usize target mismatch\");\n\n"
            "static size_t nl_qty_fn_%zu(size_t nl_local_%zu)\n{\n"
            "    _Static_assert(UINTMAX_C(%zu) != 0, \"nonzero divisor\");\n"
            "    return nl_local_%zu / ((size_t)UINTMAX_C(%zu));\n}\n\n"
            "static void nl_qty_fn_%zu(void)\n{\n",
            SIZE_MAX, helper.function, helper.formal, helper.divisor,
            helper.formal, helper.divisor, main.function))
        goto failure;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(main.body, id);
        const NLCheckedNodeView *init =
            nl_checked_node_view(main.body, n->initializer);
        if (!division_text_append(&text,
                                  "    size_t nl_local_%zu = ", n->symbol))
            goto failure;
        if (init->kind == NL_CHECKED_REGISTERED_CALL) {
            if (!division_text_append(&text, "nl_qty_fn_%zu(",
                                      init->function) ||
                !atom(&text, main.body, init->first_argument) ||
                !division_text_append(&text, ")"))
                goto failure;
        } else if (init->kind == NL_CHECKED_USIZE_ADDITION ||
                   init->kind == NL_CHECKED_USIZE_SUBTRACTION) {
            const NLCheckedNodeView *left =
                nl_checked_node_view(main.body, init->first_argument);
            if (!atom(&text, main.body, init->first_argument) ||
                !division_text_append(
                    &text,
                    init->kind == NL_CHECKED_USIZE_ADDITION ? " + " : " - ") ||
                !atom(&text, main.body, left->next_argument))
                goto failure;
        } else if (!atom(&text, main.body, n->initializer))
            goto failure;
        if (!division_text_append(&text, ";\n    (void)nl_local_%zu;\n",
                                  n->symbol))
            goto failure;
        id = n->next_item;
    }
    if (!division_text_append(&text,
                              "    return;\n}\n\nint main(void)\n{\n"
                              "    nl_qty_fn_%zu();\n    return 0;\n}\n",
                              main.function))
        goto failure;
    *out = text.bytes;
    *length = text.length;
    return NL_CHECK_OK;
failure:
    free(text.bytes);
    return NL_CHECK_RESOURCE_LIMIT;
}
