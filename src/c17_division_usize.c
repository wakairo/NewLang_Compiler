#include "copy_body_internal.h"
#include "division_usize_internal.h"
#include "pure_u8_internal.h"

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
static const char *division_scalar_type(const NLSemanticContext *c,
                                        NLTypeId type)
{
    if (type == nl_semantic_core_type(c, NL_TYPE_USIZE))
        return "size_t";
    if (type == nl_semantic_core_type(c, NL_TYPE_U8))
        return "uint8_t";
    return type == nl_semantic_core_type(c, NL_TYPE_BOOL) ? "_Bool" : NULL;
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
        if (!n->argument_count)
            return !n->first_argument &&
                   division_text_append(t, "nl_qty_fn_%zu()", n->function);
        if (n->argument_count == 2)
            return division_text_append(t,
                                        "nl_qty_fn_%zu(nl_actual_%zu_0, "
                                        "nl_actual_%zu_1)",
                                        n->function, id, id);
        const NLCheckedNodeView *arg =
            nl_checked_node_view(f, n->first_argument);
        return n->argument_count == 1 && arg && n->first_argument > id &&
               division_text_append(t, "nl_qty_fn_%zu(", n->function) &&
               division_emit_expression(t, f, n->first_argument) &&
               division_text_append(t, ")");
    }
    case NL_CHECKED_IF:
        return division_text_append(t, "nl_if_%zu", id);
    case NL_CHECKED_USIZE_MULTIPLICATION:
        return division_text_append(t, "nl_product_left * nl_product_right");
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

static bool division_emit_items(DivisionText *, const NLCheckedFragment *,
                                NLCheckedNodeId, int);
static bool division_emit_evaluation(DivisionText *, const NLCheckedFragment *,
                                     NLCheckedNodeId, int);

/* The plan matches both owned arms. Evaluate the checked condition once and
 * initialize a single result from just the selected lexical block. */
static bool division_emit_if(DivisionText *t, const NLCheckedFragment *f,
                             NLCheckedNodeId id, int indent)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const char *type = n ? division_scalar_type(f->context, n->type) : NULL;
    if (!type ||
        !division_text_append(t,
                              "%*s%s nl_if_%zu;\n"
                              "%*s_Bool nl_condition_%zu = ",
                              indent, "", type, id, indent, "", id) ||
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
        if (!block ||
            !division_emit_items(t, arm, root->initializer, indent + 4))
            return false;
        if (root->terminates) {
            NLCheckedNodeId item = block->first_item;
            const NLCheckedNodeView *ret = NULL;
            while (item) {
                ret = nl_checked_node_view(arm, item);
                if (!ret)
                    return false;
                item = ret->next_item;
            }
            if (n->normal_arms != 1 || !ret || ret->kind != NL_CHECKED_RETURN ||
                !division_emit_evaluation(t, arm, ret->initializer,
                                          indent + 4) ||
                !division_text_append(t, "%*sreturn ", indent + 4, "") ||
                !division_emit_expression(t, arm, ret->initializer) ||
                !division_text_append(t, ";\n"))
                return false;
        } else if (!division_emit_evaluation(t, arm, block->tail, indent + 4) ||
                   !division_text_append(t, "%*snl_if_%zu = ", indent + 4, "",
                                         id) ||
                   !division_emit_expression(t, arm, block->tail) ||
                   !division_text_append(t, ";\n"))
            return false;
        if (!division_text_append(t, "%*s%s\n", indent, "",
                                  a == 0 ? "} else {" : "}"))
            return false;
    }
    return true;
}

/* Stage the already validated atoms in source order in the product's block.
 * A guarded product is staged only inside its true arm. */
static bool division_emit_product_operands(DivisionText *t,
                                           const NLCheckedFragment *f,
                                           const NLCheckedNodeView *product,
                                           int indent)
{
    const NLCheckedNodeView *left =
        nl_checked_node_view(f, product->first_argument);
    return left &&
           division_text_append(t, "%*sconst size_t nl_product_left = ", indent,
                                "") &&
           division_emit_atom(t, f, product->first_argument) &&
           division_text_append(
               t, ";\n%*sconst size_t nl_product_right = ", indent, "") &&
           division_emit_atom(t, f, left->next_argument) &&
           division_text_append(t, ";\n");
}

/* C17 does not order call arguments. Materialize the checked expressions
 * once, left to right, in the lexical block that evaluates the call. */
static bool division_emit_evaluation(DivisionText *t,
                                     const NLCheckedFragment *f,
                                     NLCheckedNodeId id, int indent)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (!n)
        return false;
    if (n->kind == NL_CHECKED_USIZE_MULTIPLICATION)
        return division_emit_product_operands(t, f, n, indent);
    if (n->kind == NL_CHECKED_IF)
        return division_emit_if(t, f, id, indent);
    if (n->kind != NL_CHECKED_REGISTERED_CALL)
        return true;
    NLCheckedNodeId arg = n->first_argument;
    if (!n->argument_count)
        return !arg;
    if (n->argument_count == 1)
        return division_emit_evaluation(t, f, arg, indent);
    if (n->argument_count != 2)
        return false;
    for (size_t i = 0; i < n->argument_count; ++i) {
        const NLCheckedNodeView *actual = nl_checked_node_view(f, arg);
        const char *type =
            actual ? division_scalar_type(f->context, actual->type) : NULL;
        if (!type || !division_emit_evaluation(t, f, arg, indent) ||
            !division_text_append(t, "%*s%s nl_actual_%zu_%zu = ", indent, "",
                                  type, id, i) ||
            !division_emit_expression(t, f, arg) ||
            !division_text_append(t, ";\n"))
            return false;
        arg = actual->next_argument;
    }
    return !arg;
}

static bool division_emit_byte_constructor(DivisionText *text,
                                           const NLCheckedFragment *f,
                                           const NLCheckedNodeView *original)
{
    const NLCheckedNodeView *ctor =
        nl_checked_node_view(f, original->initializer);
    if (!division_text_append(text,
                              "    struct { uint8_t f0; uint8_t f1; } "
                              "nl_local_%zu = {",
                              original->symbol))
        return false;
    for (NLCheckedNodeId id = ctor->first_argument; id;) {
        const NLCheckedNodeView *field = nl_checked_node_view(f, id);
        const NLCheckedNodeView *literal =
            nl_checked_node_view(f, field->initializer);
        if (!division_text_append(text, " .f%zu = ((uint8_t)UINT8_C(%zu)),",
                                  field->field_index,
                                  literal->scalar_result.value))
            return false;
        id = field->next_argument;
    }
    return division_text_append(text, " };\n");
}

static bool division_emit_binding(DivisionText *t, const NLCheckedFragment *f,
                                  const NLCheckedNodeView *binding, int indent)
{
    const NLCheckedNodeView *init =
        nl_checked_node_view(f, binding->initializer);
    const char *type =
        init ? division_scalar_type(f->context, init->type) : NULL;
    return type &&
           division_emit_evaluation(t, f, binding->initializer, indent) &&
           division_text_append(t, "%*s%s nl_local_%zu = ", indent, "", type,
                                binding->symbol) &&
           division_emit_expression(t, f, binding->initializer) &&
           division_text_append(t, ";\n%*s(void)nl_local_%zu;\n", indent, "",
                                binding->symbol);
}

static bool division_emit_items_before(DivisionText *t,
                                       const NLCheckedFragment *f,
                                       NLCheckedNodeId block, int indent,
                                       NLCheckedNodeId descriptor,
                                       NLCheckedNodeId second_descriptor,
                                       NLCheckedNodeId stop)
{
    const NLCheckedNodeView *root = nl_checked_node_view(f, block);
    for (NLCheckedNodeId id = root->first_item; id && id != stop;) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (id == descriptor || id == second_descriptor) {
            if (!division_emit_byte_constructor(t, f, n))
                return false;
            id = n->next_item;
            continue;
        }
        if (n->kind == NL_CHECKED_RETURN)
            break; /* The validated plan supplies the separate exit result. */
        if (n->kind == NL_CHECKED_STATEMENT) {
            if (!division_text_append(t, "%*s(void)(", indent, "") ||
                !division_emit_expression(t, f, n->initializer) ||
                !division_text_append(t, ");\n"))
                return false;
            id = n->next_item;
            continue;
        }
        if (n->kind != NL_CHECKED_BINDING ||
            !division_emit_binding(t, f, n, indent))
            return false;
        id = n->next_item;
    }
    return true;
}

static bool division_emit_items(DivisionText *t, const NLCheckedFragment *f,
                                NLCheckedNodeId block, int indent)
{
    return division_emit_items_before(t, f, block, indent, 0, 0, 0);
}

/* The entry validator matches every binding in this exact continuation.
 * Names come from the original checked carriers, and indices are the
 * resolved semantic fields of the selected original. */
static bool division_emit_byte_loan(DivisionText *text,
                                    const NLCheckedFragment *f,
                                    NLSymbolId object,
                                    const NLCheckedNodeView *old,
                                    bool registered)
{
    const NLCheckedNodeView *loan = nl_checked_node_view(f, old->initializer);
    const NLCheckedNodeView *body = nl_checked_node_view(f, loan->initializer);
    const NLCheckedNodeView *change = nl_checked_node_view(f, body->tail);
    const NLCheckedNodeView *ref =
        nl_checked_node_view(f, change->first_argument);
    const NLCheckedNodeView *selected =
        nl_checked_node_view(f, ref->next_argument);
    const NLSymbolId writer = ref->symbol;
    if (!division_text_append(
            text,
            "    uint8_t nl_local_%zu;\n"
            "    {\n"
            "        uint8_t *const nl_local_%zu = &nl_local_%zu.f%zu;\n",
            old->symbol, writer, object, loan->field.index))
        return false;
    if (registered) {
        if (!division_text_append(
                text,
                "        nl_local_%zu = nl_qty_fn_%zu(nl_local_%zu, "
                "nl_local_%zu);\n",
                old->symbol, change->function, writer, selected->symbol))
            return false;
    } else if (!division_text_append(text,
                                     "        nl_local_%zu = *nl_local_%zu;\n"
                                     "        *nl_local_%zu = nl_local_%zu;\n",
                                     old->symbol, writer, writer,
                                     selected->symbol))
        return false;
    return division_text_append(text, "    }\n");
}

static bool division_emit_byte_read(DivisionText *text,
                                    const NLCheckedFragment *f,
                                    const NLCheckedNodeView *binding,
                                    NLSymbolId object)
{
    const NLCheckedNodeView *read =
        nl_checked_node_view(f, binding->initializer);
    return division_text_append(
        text,
        "    const uint8_t nl_local_%zu = nl_local_%zu.f%zu;\n"
        "    (void)nl_local_%zu;\n",
        binding->symbol, object, read->field.index, binding->symbol);
}

static bool division_emit_byte_copy(DivisionText *text,
                                    const NLCheckedNodeView *copy,
                                    const NLCheckedNodeView *source)
{
    return division_text_append(
        text,
        "    const uint8_t nl_local_%zu = nl_local_%zu;\n"
        "    (void)nl_local_%zu;\n",
        copy->symbol, source->symbol, copy->symbol);
}

static bool division_emit_byte_update(DivisionText *text,
                                      const NLDivisionUsizePlan *plan)
{
    const NLCheckedFragment *f = plan->body;
    const NLSymbolId object =
        nl_checked_node_view(f, plan->byte_descriptor)->symbol;
    const NLSymbolId second_object =
        plan->byte_second_descriptor
            ? nl_checked_node_view(f, plan->byte_second_descriptor)->symbol
            : object;
    const NLCheckedNodeView *first = nl_checked_node_view(f, plan->byte_update);
    if (!division_emit_byte_loan(text, f, object, first,
                                 plan->byte_update_call != 0))
        return false;
    const NLCheckedNodeView *item = nl_checked_node_view(f, first->next_item);
    const size_t early_reads = plan->byte_snapshot_first ? 2 : 0;
    for (size_t i = 0; i < early_reads; ++i) {
        if (!division_emit_byte_read(text, f, item, object))
            return false;
        item = nl_checked_node_view(f, item->next_item);
    }
    const NLCheckedNodeView *second = NULL;
    if (plan->byte_second_update) {
        const NLCheckedNodeView *init =
            nl_checked_node_view(f, item->initializer);
        if (init->kind == NL_CHECKED_FIELD_READ) {
            if (!division_emit_byte_read(text, f, item, object))
                return false;
            item = nl_checked_node_view(f, item->next_item);
            init = nl_checked_node_view(f, item->initializer);
        }
        second = nl_checked_node_view(f, plan->byte_second_update);
        /* The field read can itself bind the second writer's actual. */
        if (item != second) {
            /* The entry plan ties this Copy to the first exchange's result. */
            if (init->kind == NL_CHECKED_IDENTIFIER) {
                if (!division_emit_byte_copy(text, item, first))
                    return false;
            } else if (!division_text_append(text,
                                             "    const uint8_t nl_local_%zu = "
                                             "((uint8_t)UINT8_C(%zu));\n",
                                             item->symbol,
                                             init->scalar_result.value)) {
                return false;
            }
        }
        if (!division_emit_byte_loan(text, f, second_object, second, true))
            return false;
        item = nl_checked_node_view(f, second->next_item);
    }
    const size_t first_reads =
        plan->byte_second_descriptor ? 2 - early_reads : 2;
    const size_t reads = first_reads + (plan->byte_second_descriptor ? 2 : 0);
    for (size_t i = 0; i < reads; ++i) {
        if (!division_emit_byte_read(text, f, item,
                                     i < first_reads ? object : second_object))
            return false;
        item = nl_checked_node_view(f, item->next_item);
    }
    if (!division_emit_byte_copy(text, item, first))
        return false;
    if (!second)
        return true;
    const NLCheckedNodeView *second_old =
        nl_checked_node_view(f, item->next_item);
    if (!division_emit_byte_copy(text, second_old, second))
        return false;
    if (!plan->byte_snapshot_first || plan->byte_second_descriptor)
        return true;
    const NLCheckedNodeView *report =
        nl_checked_node_view(f, second_old->next_item);
    if (!report || report->kind != NL_CHECKED_BINDING)
        return true;
    /* The entry plan matched the two report Copies to these earlier reads. */
    const NLCheckedNodeView *mode = nl_checked_node_view(f, first->next_item);
    const NLCheckedNodeView *peer = nl_checked_node_view(f, mode->next_item);
    const NLCheckedNodeView *tag = nl_checked_node_view(f, report->next_item);
    if (!division_emit_byte_copy(text, report, mode) ||
        !division_emit_byte_copy(text, tag, peer))
        return false;
    const NLCheckedNodeView *route = nl_checked_node_view(f, tag->next_item);
    if (!route || route->kind != NL_CHECKED_BINDING)
        return true;
    if (!division_emit_binding(text, f, route, 4))
        return false;
    const NLCheckedNodeView *payload =
        nl_checked_node_view(f, route->next_item);
    return !payload || payload->kind != NL_CHECKED_BINDING ||
           division_emit_binding(text, f, payload, 4);
}

static bool division_emit_helper(DivisionText *text,
                                 const NLDivisionUsizePlan *helper)
{
    const NLSemanticContext *c = helper->body->context;
    const size_t count = helper->second_formal ? 2 : helper->formal != 0;
    const NLFunctionEntry *fn =
        nl_quantity_usize_function(c, helper->function, count);
    if (!fn && count == 1)
        fn = nl_quantity_bool_function(c, helper->function);
    if (!fn)
        return false;
    if (!count)
        return division_text_append(text,
                                    "static size_t nl_qty_fn_%zu(void)\n{\n",
                                    helper->function) &&
               division_emit_items(text, helper->body, helper->body->root, 4) &&
               division_emit_evaluation(text, helper->body, helper->result,
                                        4) &&
               division_text_append(text, "    return ") &&
               division_emit_expression(text, helper->body, helper->result) &&
               division_text_append(text, ";\n}\n\n");
    const char *first = division_scalar_type(c, fn->parameters[0]);
    const char *second =
        count == 2 ? division_scalar_type(c, fn->parameters[1]) : NULL;
    if (!first || (count == 2 && !second))
        return false;
    return division_text_append(text,
                                "static size_t nl_qty_fn_%zu(%s nl_local_%zu",
                                helper->function, first, helper->formal) &&
           (!helper->second_formal ||
            division_text_append(text, ", %s nl_local_%zu", second,
                                 helper->second_formal)) &&
           division_text_append(text, ")\n{\n") &&
           (!helper->second_formal ||
            division_text_append(text,
                                 "    (void)nl_local_%zu;\n"
                                 "    (void)nl_local_%zu;\n",
                                 helper->formal, helper->second_formal)) &&
           (!helper->division ||
            division_text_append(text,
                                 "    _Static_assert(UINTMAX_C(%zu) != 0, "
                                 "\"nonzero divisor\");\n",
                                 helper->divisor)) &&
           division_emit_items(text, helper->body, helper->body->root, 4) &&
           division_emit_evaluation(text, helper->body, helper->result, 4) &&
           division_text_append(text, "    return ") &&
           division_emit_expression(text, helper->body, helper->result) &&
           division_text_append(text, ";\n}\n\n");
}

static bool division_emit_byte_choice(DivisionText *t,
                                      const NLCheckedFragment *body,
                                      NLCheckedNodeId id)
{
    const NLCheckedNodeView *choice = nl_checked_node_view(body, id);
    const NLCheckedNodeView *yes =
        predicate_arm_value(nl_checked_if_arm(body, id, 0));
    const NLCheckedNodeView *no =
        predicate_arm_value(nl_checked_if_arm(body, id, 1));
    return choice && yes && no &&
           division_emit_expression(t, body, choice->initializer) &&
           division_text_append(t,
                                " ? ((uint8_t)UINT8_C(%zu)) "
                                ": ((uint8_t)UINT8_C(%zu))",
                                yes->scalar_result.value,
                                no->scalar_result.value);
}

static bool division_emit_byte_selector_header(DivisionText *t,
                                               const NLPureU8Call *pure)
{
    return division_text_append(t,
                                "static uint8_t nl_qty_fn_%zu(size_t "
                                "nl_local_%zu)\n{\n    return ",
                                pure->function, pure->formal);
}

static bool division_emit_byte_threshold(DivisionText *t,
                                         const NLPureU8Call *pure)
{
    return division_emit_byte_selector_header(t, pure) &&
           division_emit_byte_choice(t, pure->body, pure->selection) &&
           division_text_append(t, ";\n}\n\n");
}

static bool division_emit_byte_selector(DivisionText *t,
                                        const NLCheckedFragment *caller,
                                        NLCheckedNodeId id)
{
    NLPureU8Call pure;
    return nl_pure_u8_call(caller, id, &pure) &&
           division_emit_byte_threshold(t, &pure);
}

static bool division_emit_writer_selector(DivisionText *t,
                                          const NLCheckedFragment *caller,
                                          NLCheckedNodeId id)
{
    NLPureU8WriterPolicy policy;
    return nl_pure_u8_writer_policy_at(caller, nl_checked_context(caller), id,
                                       &policy) &&
           division_emit_byte_threshold(t, &policy.threshold) &&
           (!policy.delegate ||
            (division_emit_byte_selector_header(t, &policy.wrapper) &&
             division_emit_expression(t, policy.wrapper.body,
                                      policy.delegate) &&
             division_text_append(t, ";\n}\n\n")));
}

static bool division_emit_byte_exchange(DivisionText *t, NLSymbolId writer,
                                        NLSymbolId mode)
{
    return division_text_append(
        t,
        "    const uint8_t nl_previous = *nl_local_%zu;\n"
        "    *nl_local_%zu = nl_local_%zu;\n"
        "    return nl_previous;\n}\n\n",
        writer, writer, mode);
}

/* The entry plan validated one replace and its forwarded old Copy. */
static bool division_emit_byte_writer(DivisionText *t,
                                      const NLCheckedFragment *caller,
                                      NLCheckedNodeId id, bool local_policy)
{
    const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
    if (local_policy) {
        NLCopyPolicyWriter policy;
        if (!call || !nl_copy_policy_writer(caller, id, &policy) ||
            (policy.selector_call && !division_emit_writer_selector(
                                         t, policy.body, policy.selector_call)))
            return false;
        return division_text_append(
                   t,
                   "static uint8_t nl_qty_fn_%zu(uint8_t *nl_local_%zu, "
                   "size_t nl_local_%zu)\n{\n"
                   "    const uint8_t nl_local_%zu = ",
                   call->function, policy.writer, policy.budget,
                   policy.selected) &&
               (policy.selector_call
                    ? division_emit_expression(t, policy.body,
                                               policy.selector_call)
                    : division_emit_byte_choice(t, policy.selection_body,
                                                policy.selection)) &&
               division_text_append(t, ";\n") &&
               division_emit_byte_exchange(t, policy.writer, policy.selected);
    }
    NLCopyBodyParameters formals;
    return call && nl_checked_copy_body_parameters(caller, id, &formals) &&
           division_text_append(
               t,
               "static uint8_t nl_qty_fn_%zu(uint8_t *nl_local_%zu, "
               "uint8_t nl_local_%zu)\n{\n",
               call->function, formals.parameters[0], formals.parameters[1]) &&
           division_emit_byte_exchange(t, formals.parameters[0],
                                       formals.parameters[1]);
}

typedef bool (*QuantityCallValidator)(const NLCheckedFragment *,
                                      NLCheckedNodeId, NLDivisionUsizePlan *);

typedef struct {
    NLDivisionUsizePlan definitions[NL_SEMANTIC_MAX_ENTRIES];
    bool seen[NL_SEMANTIC_MAX_ENTRIES];
    size_t count, nodes, remaining;
} DivisionProgram;

static NLCheckStatus division_collect_helpers(
    const NLCheckedFragment *body, QuantityCallValidator valid, size_t depth,
    bool count_nodes, NLCheckedNodeId byte_selector,
    NLCheckedNodeId byte_writer, NLCheckedNodeId second_byte_writer,
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
        const NLCheckedNodeId id = body->body_calls[i];
        if ((byte_selector && id == byte_selector) ||
            (byte_writer && id == byte_writer) ||
            (second_byte_writer && id == second_byte_writer)) {
            const NLCheckedFragment *typed = nl_checked_call_body(body, id);
            if (!typed)
                return NL_CHECK_SEMANTIC_UNSUPPORTED;
            NLCheckedNodeId nested_selector = 0;
            if (id == byte_writer && !byte_selector) {
                NLCopyPolicyWriter policy;
                if (!nl_copy_policy_writer(body, id, &policy) ||
                    policy.body != typed)
                    return NL_CHECK_SEMANTIC_UNSUPPORTED;
                nested_selector = policy.selector_call;
            } else if (id == byte_selector && typed->body_count) {
                NLPureU8WriterPolicy policy;
                if (!nl_pure_u8_writer_policy_at(body, nl_checked_context(body),
                                                 id, &policy) ||
                    policy.wrapper.body != typed || !policy.delegate)
                    return NL_CHECK_SEMANTIC_UNSUPPORTED;
                nested_selector = policy.delegate;
            }
            const NLCheckStatus status = division_collect_helpers(
                typed, valid, depth + 1, true, nested_selector, 0, 0, program);
            if (status != NL_CHECK_OK)
                return status;
            continue; /* The whole-entry plan selected its typed definition. */
        }
        NLDivisionUsizePlan helper;
        if (!valid(body, id, &helper) || !helper.function ||
            helper.function > NL_SEMANTIC_MAX_ENTRIES ||
            (!helper.formal && !nl_quantity_usize_function(helper.body->context,
                                                           helper.function, 0)))
            return NL_CHECK_SEMANTIC_UNSUPPORTED;
        /* Walk each actual, including repeated calls. Share only the C
         * definition, after visiting its callees in source order. */
        const bool new_definition = !program->seen[helper.function - 1];
        const NLCheckStatus status = division_collect_helpers(
            helper.body, valid, depth + 1, new_definition, 0, 0, 0, program);
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
        const NLCheckStatus status =
            division_collect_helpers(body->arms[i].artifact, valid, depth + 1,
                                     count_nodes, 0, 0, 0, program);
        if (status != NL_CHECK_OK)
            return status;
    }
    return NL_CHECK_OK;
}

static NLCheckStatus literal_divisor_c17(const NLDivisionUsizePlan *main,
                                         const char *profile,
                                         QuantityCallValidator call_valid,
                                         char **out, size_t *length)
{
    if (main->formal || main->body->body_count > NL_CHECKED_MAX_BODY_CALLS)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    DivisionProgram program = {.remaining = NL_SEMANTIC_MAX_ENTRIES};
    const NLCheckStatus status = division_collect_helpers(
        main->body, call_valid, 0, true, main->byte_selector,
        main->byte_update_call, main->byte_second_update_call, &program);
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
    if (emitted && main->byte_selector)
        emitted =
            division_emit_byte_selector(&text, main->body, main->byte_selector);
    if (emitted && main->byte_update_call)
        emitted = division_emit_byte_writer(
            &text, main->body, main->byte_update_call, !main->byte_selector);
    if (emitted && main->byte_second_update_call) {
        const NLCheckedNodeView *first =
            nl_checked_node_view(main->body, main->byte_update_call);
        const NLCheckedNodeView *second =
            nl_checked_node_view(main->body, main->byte_second_update_call);
        emitted =
            first && second &&
            (first->function == second->function ||
             division_emit_byte_writer(&text, main->body,
                                       main->byte_second_update_call, false));
    }
    if (emitted && main->byte_descriptor)
        emitted = division_text_append(
            &text, "_Static_assert(UINT8_MAX == 255 && sizeof(uint8_t) == 1, "
                   "\"NewLang C17 requires exact 8-bit uint8_t\");\n\n");
    if (!emitted ||
        !division_text_append(&text, "static void nl_qty_fn_%zu(void)\n{\n",
                              main->function) ||
        !division_emit_items_before(
            &text, main->body, main->body->root, 4, main->byte_descriptor,
            main->byte_second_descriptor, main->byte_update) ||
        (main->byte_update && !division_emit_byte_update(&text, main)) ||
        !division_text_append(&text,
                              "    return;\n}\n\nint main(void)\n{\n"
                              "    nl_qty_fn_%zu();\n    return 0;\n}\n",
                              main->function)) {
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
    NLDivisionUsizePlan plan;
    if (!out || !length ||
        !nl_division_usize_call_valid(entry, nl_checked_root(entry), &plan))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    return literal_divisor_c17(&plan, "division", nl_division_usize_call_valid,
                               out, length);
}

NLCheckStatus nl_remainder_usize_c17(const NLCheckedFragment *entry, char **out,
                                     size_t *length)
{
    NLDivisionUsizePlan plan;
    if (!out || !length ||
        !nl_remainder_usize_call_valid(entry, nl_checked_root(entry), &plan))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    return literal_divisor_c17(&plan, "remainder",
                               nl_remainder_usize_call_valid, out, length);
}

NLCheckStatus nl_quantity_usize_c17(const NLCheckedFragment *entry, char **out,
                                    size_t *length)
{
    if (!out || !length)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    NLDivisionUsizePlan plan;
    const NLCheckStatus status =
        nl_quantity_usize_call_plan(entry, nl_checked_root(entry), &plan);
    if (status != NL_CHECK_OK)
        return status;
    return literal_divisor_c17(&plan, "composition",
                               nl_quantity_usize_call_valid, out, length);
}
