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
static bool division_emit_atom(DivisionText *t, const NLCheckedFragment *f,
                               NLCheckedNodeId id)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (!n)
        return false;
    if (n->kind == NL_CHECKED_IDENTIFIER)
        return division_text_append(t, "nl_local_%zu", n->symbol);
    return n->kind == NL_CHECKED_USIZE_LITERAL &&
           division_text_append(t, "((size_t)UINTMAX_C(%zu))",
                                n->scalar_result.value);
}

/* The complete plan has already matched these operands, kinds, and child
 * calls to their source-owned graph. Render actual expressions in order. */
static bool division_emit_expression(DivisionText *t,
                                     const NLCheckedFragment *f,
                                     NLCheckedNodeId id)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const char *operator_text;
    switch (n->kind) {
    case NL_CHECKED_REGISTERED_CALL:
        return division_text_append(t, "nl_qty_fn_%zu(", n->function) &&
               division_emit_atom(t, f, n->first_argument) &&
               division_text_append(t, ")");
    case NL_CHECKED_USIZE_DIVISION:
        operator_text = " / ";
        break;
    case NL_CHECKED_USIZE_REMAINDER:
        operator_text = " % ";
        break;
    case NL_CHECKED_USIZE_ADDITION:
        operator_text = " + ";
        break;
    case NL_CHECKED_USIZE_SUBTRACTION:
        operator_text = " - ";
        break;
    default:
        return division_emit_atom(t, f, id);
    }
    const NLCheckedNodeView *left = nl_checked_node_view(f, n->first_argument);
    return division_emit_atom(t, f, n->first_argument) &&
           division_text_append(t, "%s", operator_text) &&
           division_emit_atom(t, f, left->next_argument);
}

static bool division_emit_bindings(DivisionText *t, const NLCheckedFragment *f)
{
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n->kind == NL_CHECKED_RETURN)
            break; /* The validated plan supplies the separate exit result. */
        if (!division_text_append(t, "    size_t nl_local_%zu = ", n->symbol) ||
            !division_emit_expression(t, f, n->initializer) ||
            !division_text_append(t, ";\n    (void)nl_local_%zu;\n", n->symbol))
            return false;
        id = n->next_item;
    }
    return true;
}

static NLCheckStatus literal_divisor_c17(const NLCheckedFragment *entry,
                                         bool remainder, char **out,
                                         size_t *length)
{
    bool (*call_valid)(const NLCheckedFragment *, NLCheckedNodeId,
                       NLDivisionUsizePlan *) =
        remainder ? nl_remainder_usize_call_valid
                  : nl_division_usize_call_valid;
    NLDivisionUsizePlan main, helper;
    if (!out || !length || !call_valid(entry, nl_checked_root(entry), &main) ||
        main.formal || !main.call ||
        !call_valid(main.body, main.call, &helper) || !helper.formal)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    const size_t node_limit = (SIZE_MAX - 2048) / 512;
    if (main.body->count > node_limit ||
        helper.body->count > node_limit - main.body->count)
        return NL_CHECK_RESOURCE_LIMIT;
    /* One existing staging buffer. The selected helper is emitted once;
     * every caller invocation uses its own actual value and checked result. */
    DivisionText text = {
        .capacity = (main.body->count + helper.body->count) * 512 + 2048};
    text.bytes = malloc(text.capacity);
    if (!text.bytes)
        return NL_CHECK_OUT_OF_MEMORY;
    if (!division_text_append(
            &text,
            "/* NewLang source-owned quantity %s C17. */\n"
            "#include <stddef.h>\n#include <stdint.h>\n"
            "_Static_assert(SIZE_MAX == UINTMAX_C(%zu), "
            "\"NewLang usize target mismatch\");\n\n"
            "static size_t nl_qty_fn_%zu(size_t nl_local_%zu)\n{\n"
            "    _Static_assert(UINTMAX_C(%zu) != 0, \"nonzero divisor\");\n",
            remainder ? "remainder" : "division", SIZE_MAX, helper.function,
            helper.formal, helper.divisor) ||
        !division_emit_bindings(&text, helper.body) ||
        !division_text_append(&text, "    return ") ||
        !division_emit_expression(&text, helper.body, helper.result) ||
        !division_text_append(&text,
                              ";\n}\n\nstatic void nl_qty_fn_%zu(void)\n{\n",
                              main.function) ||
        !division_emit_bindings(&text, main.body) ||
        !division_text_append(&text,
                              "    return;\n}\n\nint main(void)\n{\n"
                              "    nl_qty_fn_%zu();\n    return 0;\n}\n",
                              main.function)) {
        free(text.bytes);
        return NL_CHECK_RESOURCE_LIMIT;
    }
    *out = text.bytes;
    *length = text.length;
    return NL_CHECK_OK;
}

NLCheckStatus nl_division_usize_c17(const NLCheckedFragment *entry, char **out,
                                    size_t *length)
{
    return literal_divisor_c17(entry, false, out, length);
}

NLCheckStatus nl_remainder_usize_c17(const NLCheckedFragment *entry, char **out,
                                     size_t *length)
{
    return literal_divisor_c17(entry, true, out, length);
}
