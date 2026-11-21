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
    if (!n)
        return false;
    const char *operator_text;
    switch (n->kind) {
    case NL_CHECKED_REGISTERED_CALL: {
        const NLCheckedNodeView *arg =
            nl_checked_node_view(f, n->first_argument);
        return arg && n->first_argument > id &&
               division_text_append(t, "nl_qty_fn_%zu(", n->function) &&
               division_emit_expression(t, f, n->first_argument) &&
               division_text_append(t, ")");
    }
    case NL_CHECKED_IF:
        return division_text_append(t, "nl_if_%zu", id);
    case NL_CHECKED_USIZE_LESS_THAN:
        operator_text = " < ";
        break;
    case NL_CHECKED_USIZE_EQUALITY:
        operator_text = " == ";
        break;
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

static bool division_emit_bindings(DivisionText *, const NLCheckedFragment *,
                                   NLCheckedNodeId, int);

/* The plan matches both owned arms. Evaluate the checked condition once and
 * initialize a single result from just the selected lexical block. */
static bool division_emit_if(DivisionText *t, const NLCheckedFragment *f,
                             NLCheckedNodeId id, int indent)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (n->kind != NL_CHECKED_IF)
        return true;
    if (!division_text_append(t,
                              "%*ssize_t nl_if_%zu;\n"
                              "%*s_Bool nl_condition_%zu = ",
                              indent, "", id, indent, "", id) ||
        !division_emit_expression(t, f, n->initializer) ||
        !division_text_append(t, ";\n%*sif (nl_condition_%zu) {\n", indent, "",
                              id))
        return false;
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, a);
        if (!arm)
            return false;
        const NLCheckedNodeView *root = nl_checked_node_view(arm, arm->root);
        const NLCheckedNodeView *block =
            nl_checked_node_view(arm, root->initializer);
        if (!division_emit_bindings(t, arm, root->initializer, indent + 4) ||
            !division_text_append(t, "%*snl_if_%zu = ", indent + 4, "", id) ||
            !division_emit_expression(t, arm, block->tail) ||
            !division_text_append(t, ";\n%*s%s\n", indent, "",
                                  a == 0 ? "} else {" : "}"))
            return false;
    }
    return true;
}

static bool division_emit_bindings(DivisionText *t, const NLCheckedFragment *f,
                                   NLCheckedNodeId block, int indent)
{
    const NLTypeId boolean = nl_semantic_core_type(f->context, NL_TYPE_BOOL);
    const NLCheckedNodeView *root = nl_checked_node_view(f, block);
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n->kind == NL_CHECKED_RETURN)
            break; /* The validated plan supplies the separate exit result. */
        const NLCheckedNodeView *init = nl_checked_node_view(f, n->initializer);
        const char *type = init->type == boolean ? "_Bool" : "size_t";
        if (!division_emit_if(t, f, n->initializer, indent) ||
            !division_text_append(t, "%*s%s nl_local_%zu = ", indent, "", type,
                                  n->symbol) ||
            !division_emit_expression(t, f, n->initializer) ||
            !division_text_append(t, ";\n%*s(void)nl_local_%zu;\n", indent, "",
                                  n->symbol))
            return false;
        id = n->next_item;
    }
    return true;
}

static bool division_emit_helper(DivisionText *text,
                                 const NLDivisionUsizePlan *helper)
{
    return division_text_append(
               text, "static size_t nl_qty_fn_%zu(size_t nl_local_%zu)\n{\n",
               helper->function, helper->formal) &&
           (!helper->division ||
            division_text_append(text,
                                 "    _Static_assert(UINTMAX_C(%zu) != 0, "
                                 "\"nonzero divisor\");\n",
                                 helper->divisor)) &&
           division_emit_bindings(text, helper->body, helper->body->root, 4) &&
           division_emit_if(text, helper->body, helper->result, 4) &&
           division_text_append(text, "    return ") &&
           division_emit_expression(text, helper->body, helper->result) &&
           division_text_append(text, ";\n}\n\n");
}

typedef bool (*QuantityCallValidator)(const NLCheckedFragment *,
                                      NLCheckedNodeId, NLDivisionUsizePlan *);

typedef struct {
    NLDivisionUsizePlan definitions[NL_SEMANTIC_MAX_ENTRIES];
    bool seen[NL_SEMANTIC_MAX_ENTRIES];
    size_t count, nodes, remaining;
} DivisionProgram;

static NLCheckStatus division_collect_helpers(const NLCheckedFragment *body,
                                              QuantityCallValidator valid,
                                              size_t depth, bool count_nodes,
                                              DivisionProgram *program)
{
    if (depth >= NL_SEMANTIC_MAX_DEPTH ||
        body->body_count > NL_CHECKED_MAX_BODY_CALLS)
        return NL_CHECK_RESOURCE_LIMIT;
    const size_t node_limit = (SIZE_MAX - 2048) / 512;
    if (count_nodes) {
        if (body->count > node_limit - program->nodes)
            return NL_CHECK_RESOURCE_LIMIT;
        program->nodes += body->count;
    }
    for (size_t i = 0; i < body->body_count; ++i) {
        if (!program->remaining)
            return NL_CHECK_RESOURCE_LIMIT;
        --program->remaining;
        NLDivisionUsizePlan helper;
        if (!valid(body, body->body_calls[i], &helper) || !helper.formal ||
            !helper.function || helper.function > NL_SEMANTIC_MAX_ENTRIES)
            return NL_CHECK_SEMANTIC_UNSUPPORTED;
        /* Walk each actual, including repeated calls. Share only the C
         * definition, after visiting its callees in source order. */
        const bool new_definition = !program->seen[helper.function - 1];
        const NLCheckStatus status = division_collect_helpers(
            helper.body, valid, depth + 1, new_definition, program);
        if (status != NL_CHECK_OK)
            return status;
        if (new_definition) {
            program->seen[helper.function - 1] = true;
            program->definitions[program->count++] = helper;
        }
    }
    for (size_t i = 0; i < body->arm_count; ++i) {
        if (!program->remaining)
            return NL_CHECK_RESOURCE_LIMIT;
        --program->remaining;
        const NLCheckStatus status = division_collect_helpers(
            body->arms[i].artifact, valid, depth + 1, count_nodes, program);
        if (status != NL_CHECK_OK)
            return status;
    }
    return NL_CHECK_OK;
}

static NLCheckStatus literal_divisor_c17(const NLCheckedFragment *entry,
                                         const char *profile,
                                         QuantityCallValidator call_valid,
                                         char **out, size_t *length)
{
    NLDivisionUsizePlan main;
    if (!out || !length || !call_valid(entry, nl_checked_root(entry), &main) ||
        main.formal || main.body->body_count > NL_CHECKED_MAX_BODY_CALLS)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    DivisionProgram program = {.remaining = NL_SEMANTIC_MAX_ENTRIES};
    const NLCheckStatus status =
        division_collect_helpers(main.body, call_valid, 0, true, &program);
    if (status != NL_CHECK_OK)
        return status;
    DivisionText text = {.capacity = program.nodes * 512 + 2048};
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
    for (size_t i = 0; emitted && i < program.count; ++i)
        emitted = division_emit_helper(&text, &program.definitions[i]);
    if (!emitted ||
        !division_text_append(&text, "static void nl_qty_fn_%zu(void)\n{\n",
                              main.function) ||
        !division_emit_bindings(&text, main.body, main.body->root, 4) ||
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
