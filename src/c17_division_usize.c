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

static bool division_emit_helper(DivisionText *text,
                                 const NLDivisionUsizePlan *helper)
{
    return division_text_append(
               text,
               "static size_t nl_qty_fn_%zu(size_t nl_local_%zu)\n{\n"
               "    _Static_assert(UINTMAX_C(%zu) != 0, \"nonzero "
               "divisor\");\n",
               helper->function, helper->formal, helper->divisor) &&
           division_emit_bindings(text, helper->body) &&
           division_text_append(text, "    return ") &&
           division_emit_expression(text, helper->body, helper->result) &&
           division_text_append(text, ";\n}\n\n");
}

typedef bool (*QuantityCallValidator)(const NLCheckedFragment *,
                                      NLCheckedNodeId, NLDivisionUsizePlan *);

static NLCheckStatus literal_divisor_c17(const NLCheckedFragment *entry,
                                         const char *profile,
                                         QuantityCallValidator call_valid,
                                         char **out, size_t *length)
{
    NLDivisionUsizePlan main;
    if (!out || !length || !call_valid(entry, nl_checked_root(entry), &main) ||
        main.formal || main.body->body_count > NL_CHECKED_MAX_BODY_CALLS)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    const size_t node_limit = (SIZE_MAX - 2048) / 512;
    size_t nodes = main.body->count;
    if (nodes > node_limit)
        return NL_CHECK_RESOURCE_LIMIT;

    /* Authenticate every invocation. One representative of each registered
     * declaration supplies code shape only, in first-use order. */
    NLDivisionUsizePlan helpers[NL_CHECKED_MAX_BODY_CALLS];
    size_t helper_count = 0;
    for (size_t i = 0; i < main.body->body_count; ++i) {
        NLDivisionUsizePlan helper;
        if (!call_valid(main.body, main.body->body_calls[i], &helper) ||
            !helper.formal)
            return NL_CHECK_SEMANTIC_UNSUPPORTED;
        size_t j = 0;
        while (j < helper_count && helpers[j].function != helper.function)
            ++j;
        if (j == helper_count) {
            if (helper.body->count > node_limit - nodes)
                return NL_CHECK_RESOURCE_LIMIT;
            nodes += helper.body->count;
            helpers[helper_count++] = helper;
        }
    }

    DivisionText text = {.capacity = nodes * 512 + 2048};
    text.bytes = malloc(text.capacity);
    if (!text.bytes)
        return NL_CHECK_OUT_OF_MEMORY;
    bool emitted =
        division_text_append(&text,
                             "/* NewLang source-owned quantity %s C17. */\n"
                             "#include <stddef.h>\n#include <stdint.h>\n"
                             "_Static_assert(SIZE_MAX == UINTMAX_C(%zu), "
                             "\"NewLang usize target mismatch\");\n\n",
                             profile, SIZE_MAX);
    for (size_t i = 0; emitted && i < helper_count; ++i)
        emitted = division_emit_helper(&text, &helpers[i]);
    if (!emitted ||
        !division_text_append(&text, "static void nl_qty_fn_%zu(void)\n{\n",
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
    return literal_divisor_c17(entry, "division", nl_division_usize_call_valid,
                               out, length);
}

NLCheckStatus nl_remainder_usize_c17(const NLCheckedFragment *entry, char **out,
                                     size_t *length)
{
    return literal_divisor_c17(entry, "remainder",
                               nl_remainder_usize_call_valid, out, length);
}

NLCheckStatus nl_quantity_usize_c17(const NLCheckedFragment *entry, char **out,
                                    size_t *length)
{
    return literal_divisor_c17(entry, "composition",
                               nl_quantity_usize_call_valid, out, length);
}
