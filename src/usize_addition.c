#include "newlang/usize_addition.h"
#include "c17_local_field_internal.h"
#include "copy_body_internal.h"
#include "division_usize_internal.h"
#include "guarded_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"
#include "newlang/usize_subtraction.h"
#include "pure_u8_internal.h"
#include "semantic_internal.h"
#include "syntax_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* One receipt owner, with a public view for each operation. */
typedef enum {
    NL_USIZE_ADD,
    NL_USIZE_SUBTRACT,
    NL_USIZE_DIVIDE,
    NL_USIZE_REMAINDER,
    NL_USIZE_MULTIPLY,
    NL_USIZE_ANY_FIXED_DIVISOR /* Caller matching only. Receipts name one op. */
} NLUsizeOperation;
static const struct {
    NLSyntaxKind syntax;
    NLCheckedKind checked;
    const char *token;
} arithmetic_operation[] = {
    [NL_USIZE_ADD] = {NL_SYNTAX_USIZE_ADDITION, NL_CHECKED_USIZE_ADDITION, "+"},
    [NL_USIZE_SUBTRACT] = {NL_SYNTAX_USIZE_SUBTRACTION,
                           NL_CHECKED_USIZE_SUBTRACTION, "-"},
    [NL_USIZE_DIVIDE] = {NL_SYNTAX_USIZE_DIVISION, NL_CHECKED_USIZE_DIVISION,
                         "/"},
    [NL_USIZE_REMAINDER] = {NL_SYNTAX_USIZE_REMAINDER,
                            NL_CHECKED_USIZE_REMAINDER, "%"},
    [NL_USIZE_MULTIPLY] = {NL_SYNTAX_USIZE_MULTIPLICATION,
                           NL_CHECKED_USIZE_MULTIPLICATION, "*"}};
typedef struct {
    const NLCheckedFragment *origin;
    NLCheckedNodeId node, operands[2];
    NLTypeId type;
    NLValueId values[2], result;
    NLSymbolId symbols[2];
    NLPlaceId places[2];
    NLIncarnationId incarnations[2];
    NLValueFactId facts[2];
    NLSourceSpan span, operator_span, operand_spans[2];
    size_t numbers[2], answer;
} NLUsizeArithmeticView;

/* One small immutable receipt per source operation; no world clone, hidden
 * authority registry, or alternate arithmetic interpreter. */
typedef struct NLUsizeAdditionProof {
    NLUsizeArithmeticView issued;
    NLUsizeOperation operation;
    const NLSyntaxNode *syntax;
    const NLSemanticContext *context;
    NLValueId binding_values[2];
    bool guarded;
    bool numerator_known, result_known;
    const NLCheckedFragment *guard_parent;
    NLCheckedNodeId conditional;
    size_t function, limit, step;
    NLSymbolId formal;
    NLSemanticBindingView guard_binding;
    NLSemanticPlaceView guard_place;
    struct NLUsizeAdditionProof *next;
} NLUsizeAdditionProof;

static bool span_equal(NLSourceSpan a, NLSourceSpan b)
{
    return a.start_byte == b.start_byte && a.end_byte == b.end_byte;
}

static bool quantity(const NLSemanticContext *, NLValueId, NLTypeId);
static bool node_value(const NLCheckedFragment *, const NLSemanticContext *,
                       NLCheckedNodeId, NLValueId *);

static bool fixed_divisor(NLUsizeOperation operation)
{
    return operation == NL_USIZE_DIVIDE || operation == NL_USIZE_REMAINDER;
}

static bool arithmetic_fact(const NLSemanticContext *c, NLValueId id,
                            NLTypeId type, bool known, size_t number)
{
    NLSemanticValueView v;
    return quantity(c, id, type) && nl_semantic_value_view(c, id, &v) &&
           v.scalar_known == known && v.scalar_value == number;
}

static bool plain(const NLSemanticContext *c, NLValueId id, NLTypeId type,
                  size_t number)
{
    NLSemanticValueView v;
    return nl_semantic_value_view(c, id, &v) && v.type == type &&
           v.scalar_known && v.scalar_value == number &&
           v.dependencies == NL_DEPENDENCY_FREE && !v.value_dependency_count &&
           !v.domain && !v.reference.place && !v.reference.incarnation &&
           !v.reference.scope &&
           v.reference.provenance == NL_PROVENANCE_UNKNOWN &&
           !v.reference.readable && !v.reference.writable &&
           !v.reference.occurrence_dependency && !v.reference_count &&
           !v.slot_place && !v.allocation_region && !v.occupancy.region &&
           !v.occupancy.start && !v.occupancy.length && !v.aggregate_owner &&
           !v.field_count && !v.sum_owner && !v.sum_payload && !v.variant;
}

static void destroy(NLCheckedFragment *f)
{
    while (f->usize_additions) {
        NLUsizeAdditionProof *p = f->usize_additions;
        f->usize_additions = p->next;
        free(p);
    }
}

NLCheckStatus nl_usize_arithmetic_capture(NLCheckedFragment *f,
                                          NLCheckedNodeId id,
                                          const NLSyntaxNode *syntax,
                                          const NLSemanticContext *working)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const NLSyntaxView *s = nl_syntax_node_view(syntax);
    if (!n || !s)
        return NL_CHECK_INTERNAL_ERROR;
    size_t op = 0;
    const size_t count =
        sizeof(arithmetic_operation) / sizeof(*arithmetic_operation);
    while (op < count && arithmetic_operation[op].checked != n->kind)
        ++op;
    if (op == count || arithmetic_operation[op].syntax != s->kind)
        return NL_CHECK_INTERNAL_ERROR;
    NLUsizeAdditionProof *p = calloc(1, sizeof(*p));
    if (!p)
        return NL_CHECK_OUT_OF_MEMORY;
    p->operation = (NLUsizeOperation)op;
    p->result_known = n->scalar_result.known;
    p->syntax = syntax;
    /* f->context is the publication destination; CHECK still operates on its
     * transaction clone. Read current facts from working, qualify saved IDs by
     * the destination that receives those exact tables only on commit. */
    p->context = f->context;
    NLUsizeArithmeticView *v = &p->issued;
    *v = (NLUsizeArithmeticView){.origin = f,
                                 .node = id,
                                 .type = n->type,
                                 .result = n->results[0].value,
                                 .span = n->span,
                                 .operator_span = n->name,
                                 .answer = n->scalar_result.value};
    v->operands[0] = n->first_argument;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *a = nl_checked_node_view(f, v->operands[i]);
        if (!a || a->result_count != 1)
            goto invalid;
        v->values[i] = a->results[0].value;
        v->operand_spans[i] = a->span;
        NLSemanticValueView value;
        if (!nl_semantic_value_view(working, v->values[i], &value))
            goto invalid;
        v->numbers[i] = value.scalar_value;
        if (!i)
            p->numerator_known = value.scalar_known;
        if (a->kind == NL_CHECKED_IDENTIFIER) {
            NLSemanticBindingView b;
            NLSemanticPlaceView place;
            if (a->value_use != NL_VALUE_COPIED ||
                !nl_semantic_binding_view(working, a->symbol, &b) ||
                b.availability != NL_AVAILABLE ||
                !nl_semantic_place_view(working, b.place, &place) ||
                !place.live || !place.current_fact ||
                place.current_value != b.value || place.type != n->type ||
                !(fixed_divisor(p->operation)
                      ? arithmetic_fact(working, b.value, n->type,
                                        value.scalar_known, value.scalar_value)
                      : plain(working, b.value, n->type, value.scalar_value)))
                goto invalid;
            v->symbols[i] = a->symbol;
            v->places[i] = b.place;
            v->incarnations[i] = place.incarnation;
            v->facts[i] = place.current_fact;
            p->binding_values[i] = b.value;
        } else if (a->kind != NL_CHECKED_USIZE_LITERAL)
            goto invalid;
        if (i == 0)
            v->operands[1] = a->next_argument;
    }
    p->next = f->usize_additions;
    f->usize_additions = p;
    f->destroy_usize_additions = destroy;
    return NL_CHECK_OK;
invalid:
    free(p);
    return NL_CHECK_INTERNAL_ERROR;
}

static const NLUsizeAdditionProof *find(const NLCheckedFragment *f,
                                        NLCheckedNodeId id)
{
    for (const NLUsizeAdditionProof *p = f ? f->usize_additions : NULL; p;
         p = p->next)
        if (p->issued.node == id)
            return p;
    return NULL;
}

static bool same(const NLUsizeArithmeticView *a, const NLUsizeArithmeticView *b)
{
    if (a->origin != b->origin || a->node != b->node || a->type != b->type ||
        a->result != b->result || a->answer != b->answer ||
        !span_equal(a->span, b->span) ||
        !span_equal(a->operator_span, b->operator_span))
        return false;
    for (size_t i = 0; i < 2; ++i)
        if (a->operands[i] != b->operands[i] || a->values[i] != b->values[i] ||
            a->symbols[i] != b->symbols[i] || a->places[i] != b->places[i] ||
            a->incarnations[i] != b->incarnations[i] ||
            a->facts[i] != b->facts[i] || a->numbers[i] != b->numbers[i] ||
            !span_equal(a->operand_spans[i], b->operand_spans[i]))
            return false;
    return true;
}

static bool token(const NLSource *s, NLSourceSpan span, const char *text)
{
    NLSourceView bytes;
    return text && nl_source_view(s, span, &bytes) &&
           strlen(text) == bytes.length &&
           memcmp(bytes.bytes, text, bytes.length) == 0;
}

bool nl_usize_product_fits(size_t lhs, size_t rhs)
{
    return lhs == 0 || rhs <= SIZE_MAX / lhs;
}

static bool arithmetic_result(NLUsizeOperation operation, size_t lhs,
                              size_t rhs, size_t answer)
{
    if (operation == NL_USIZE_ADD)
        return rhs <= SIZE_MAX - lhs && answer == lhs + rhs;
    if (operation == NL_USIZE_SUBTRACT)
        return lhs >= rhs && answer == lhs - rhs;
    if (operation == NL_USIZE_MULTIPLY)
        return nl_usize_product_fits(lhs, rhs) && answer == lhs * rhs;
    if (operation == NL_USIZE_DIVIDE)
        return rhs && answer == lhs / rhs;
    if (operation == NL_USIZE_REMAINDER)
        return rhs && answer < rhs && answer == lhs % rhs;
    return false;
}

/* Match a checked lexical block's tail or item's initializer to its exact
 * retained expression. Identical text in another item is a different event. */
static const NLSyntaxNode *block_expression_source(const NLCheckedFragment *f,
                                                   NLCheckedNodeId block,
                                                   const NLSyntaxNode *syntax,
                                                   NLCheckedNodeId id)
{
    const NLCheckedNodeView *root = nl_checked_node_view(f, block);
    const NLSyntaxView *src = nl_syntax_node_view(syntax);
    if (!root || root->kind != NL_CHECKED_BLOCK || !src ||
        src->kind != NL_SYNTAX_BLOCK || !span_equal(root->span, src->span))
        return NULL;
    if (root->tail == id)
        return src->data.block.tail;
    NLCheckedNodeId next = root->first_item;
    const NLSyntaxNode *item = src->data.block.items;
    for (size_t i = 0; item && i < root->item_count; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, next);
        const NLSyntaxView *s = nl_syntax_node_view(item);
        if (!s || !n || !span_equal(n->span, s->span))
            return NULL;
        if (n->initializer == id) {
            if (n->kind == NL_CHECKED_BINDING && s->kind == NL_SYNTAX_BINDING)
                return s->data.binding.initializer;
            if (n->kind == NL_CHECKED_RETURN && s->kind == NL_SYNTAX_RETURN)
                return s->data.statement.expression;
            return NULL;
        }
        next = n->next_item;
        item = nl_syntax_next_argument(item);
    }
    return NULL;
}

/* The retained tail is one call with this exact fixed-divisor expression as
 * its sole actual.
 * Its own callee seal is already committed when the enclosing body is planned.
 */
static bool arm_fixed_divisor_tail_actual(const NLCheckedFragment *arm,
                                          NLCheckedNodeId block_id,
                                          const NLSyntaxNode *block_source,
                                          const NLUsizeAdditionProof *proof)
{
    const NLCheckedNodeView *block = nl_checked_node_view(arm, block_id);
    const NLSyntaxView *src = nl_syntax_node_view(block_source);
    if (!block || block->kind != NL_CHECKED_BLOCK || block->terminates ||
        !src || src->kind != NL_SYNTAX_BLOCK ||
        !span_equal(block->span, src->span))
        return false;
    const NLCheckedNodeId tail = block->tail;
    const NLCheckedNodeId id = proof->issued.node;
    const NLCheckedNodeView *call = nl_checked_node_view(arm, tail);
    const NLSyntaxView *call_source = nl_syntax_node_view(src->data.block.tail);
    if (!call || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || call->argument_count != 1 ||
        call->first_argument != id || tail >= id || !call_source ||
        call_source->kind != NL_SYNTAX_EXPR_CALL ||
        call_source->data.call.arguments != proof->syntax ||
        nl_syntax_next_argument(proof->syntax) ||
        !span_equal(call->span, call_source->span) ||
        !span_equal(call->name, call_source->data.call.callee))
        return false;
    const NLSemanticContext *c = arm->context;
    const NLFunctionEntry *fn =
        nl_quantity_usize_function(c, call->function, 1);
    const NLCheckedNodeView *actual = nl_checked_node_view(arm, id);
    NLValueId actual_value, call_result, block_result, arm_result;
    if (!fn || !token(arm->source, call->name, fn->name) || !actual ||
        actual->kind != arithmetic_operation[proof->operation].checked ||
        actual->next_argument || actual->parameter_type != fn->parameters[0] ||
        !node_value(arm, c, id, &actual_value) ||
        actual_value != proof->issued.result ||
        !node_value(arm, c, tail, &call_result) ||
        !node_value(arm, c, block_id, &block_result) ||
        !node_value(arm, c, arm->root, &arm_result) ||
        call_result != block_result || block_result != arm_result ||
        !nl_h_custody_result_valid(arm, tail))
        return false;
    const NLCheckedFragment *callee = nl_h_custody_observe_body(arm, tail);
    return callee && callee->context == c && callee->body_owner == fn->body &&
           callee->source == fn->body->source;
}

static const NLFunctionBody *arm_arithmetic_owner(
    const NLCheckedFragment *arm, const NLUsizeAdditionProof *proof,
    const NLSemanticContext *c, const NLCheckedFragment *if_parent)
{
    if (c != arm->context || arm->body_owner || arm->arm_count ||
        (!if_parent && nl_control_exits_count(arm->exits)))
        return NULL;
    size_t function;
    const NLCheckedFragment *body =
        nl_h_custody_if_body(arm, if_parent, &function);
    if (!body || !body->body_owner || !body->body_owner->syntax ||
        function > c->function_count)
        return NULL;
    const NLFunctionEntry *fn = &c->functions[function - 1];
    if (fn->kind != NL_CHECKED_REGISTERED_CALL ||
        fn->body != body->body_owner || fn->body->source != arm->source)
        return NULL;
    const NLCheckedNodeId site = arm->if_parent_node;
    const NLCheckedNodeView *conditional = nl_checked_node_view(body, site);
    const NLSyntaxView *src = nl_syntax_node_view(block_expression_source(
        body, body->root, nl_syntax_tree_root(fn->body->syntax), site));
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    if (!conditional || conditional->kind != NL_CHECKED_IF ||
        (!if_parent && conditional->normal_arms != 2) ||
        (if_parent && conditional->normal_arms != 1 &&
         conditional->normal_arms != 2) ||
        conditional->item_count != 2 || conditional->terminates ||
        (conditional->normal_arms == 1) != (conditional->tail != 0) ||
        conditional->result_count != 1 || conditional->type != type || !src ||
        src->kind != NL_SYNTAX_IF || !span_equal(conditional->span, src->span))
        return NULL;
    const NLCheckedNodeView *condition =
        nl_checked_node_view(body, conditional->initializer);
    const NLSyntaxView *test =
        nl_syntax_node_view(src->data.conditional.condition);
    if (!test || !condition || condition->result_count != 1 ||
        condition->type != nl_semantic_core_type(c, NL_TYPE_BOOL) ||
        !span_equal(condition->span, test->span) ||
        arm->branch_value_prefix != condition->results[0].value)
        return NULL;
    const NLSyntaxNode *block = arm->if_arm_index == 0
                                    ? src->data.conditional.then_block
                                    : src->data.conditional.else_block;
    const NLSyntaxView *block_src = nl_syntax_node_view(block);
    const NLCheckedNodeView *root = nl_checked_node_view(arm, arm->root);
    if (!root || root->kind != NL_CHECKED_IF_ARM || !block_src ||
        !span_equal(root->span, block_src->span) ||
        (root->terminates
             ? !if_parent || conditional->normal_arms != 1 || root->type ||
                   root->result_count || !nl_control_exits_count(arm->exits)
             : root->type != type || root->result_count != 1 ||
                   nl_control_exits_count(arm->exits)))
        return NULL;
    const NLSyntaxNode *event = block_expression_source(
        arm, root->initializer, block, proof->issued.node);
    if (event != proof->syntax &&
        (event || conditional->normal_arms != 2 ||
         !fixed_divisor(proof->operation) ||
         !arm_fixed_divisor_tail_actual(arm, root->initializer, block, proof)))
        return NULL;
    return fn->body;
}

static bool operation_valid_at(const NLCheckedFragment *f, NLCheckedNodeId id,
                               NLUsizeOperation operation,
                               const NLUsizeArithmeticView *candidate,
                               const NLSemanticContext *c,
                               const NLCheckedFragment *if_parent)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!p || p->guarded || p->operation != operation ||
        p->context != f->context || !c || p->issued.origin != f ||
        (candidate && !same(candidate, &p->issued)))
        return false;
    const NLFunctionBody *owner = f->body_owner;
    if (fixed_divisor(operation) && (if_parent || !nl_h_custody_is_body(f)))
        owner = arm_arithmetic_owner(f, p, c, if_parent);
    if (!owner || f->source != owner->source)
        return false;
    bool registered = false, retained = false;
    for (size_t i = 0; i < c->function_count; ++i)
        if (c->functions[i].body == owner &&
            c->functions[i].kind == NL_CHECKED_REGISTERED_CALL)
            registered = true;
    for (const NLSyntaxNode *n = owner->syntax->nodes; n; n = n->owned_next)
        if (n == p->syntax)
            retained = true;
    if (!registered || !retained || nl_sem_validate(c) != NL_CHECK_OK)
        return false;
    const NLUsizeArithmeticView *v = &p->issued;
    const NLSyntaxView *s = nl_syntax_node_view(p->syntax);
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    NLSemanticTypeView t;
    if (v->type != nl_semantic_core_type(c, NL_TYPE_USIZE) ||
        !nl_semantic_type_view(c, v->type, &t) || !t.is_copy ||
        t.kind != NL_TYPE_USIZE ||
        s->kind != arithmetic_operation[operation].syntax || !n ||
        n->kind != arithmetic_operation[operation].checked || n->terminates ||
        !span_equal(v->span, (NLSourceSpan){v->operand_spans[0].start_byte,
                                            v->operand_spans[1].end_byte}) ||
        v->operand_spans[0].end_byte > v->operator_span.start_byte ||
        v->operator_span.end_byte > v->operand_spans[1].start_byte ||
        n->type != v->type || n->result_count != 1 || n->argument_count != 2 ||
        n->results[0].type != v->type || n->results[0].value != v->result ||
        n->first_argument != v->operands[0] || !span_equal(n->span, v->span) ||
        !span_equal(s->span, v->span) ||
        !span_equal(n->name, v->operator_span) ||
        !span_equal(s->data.equality.operator_span, v->operator_span) ||
        !token(f->source, v->operator_span,
               arithmetic_operation[operation].token) ||
        n->value_use != NL_VALUE_USE_NONE || !n->has_scalar_result ||
        n->scalar_result.type != v->type ||
        n->scalar_result.value != v->answer ||
        (fixed_divisor(operation)
             ? (p->result_known != p->numerator_known ||
                n->scalar_result.known != p->result_known || !v->numbers[1] ||
                (p->numerator_known
                     ? !arithmetic_result(operation, v->numbers[0],
                                          v->numbers[1], v->answer)
                     : (v->numbers[0] != 0 || v->answer != 0)) ||
                !arithmetic_fact(c, v->result, v->type, p->result_known,
                                 v->answer))
             : (!n->scalar_result.known || !p->numerator_known ||
                !p->result_known ||
                !arithmetic_result(operation, v->numbers[0], v->numbers[1],
                                   v->answer) ||
                !plain(c, v->result, v->type, v->answer))))
        return false;
    const NLSyntaxNode *atoms[] = {s->data.equality.left,
                                   s->data.equality.right};
    for (size_t i = 0; i < 2; ++i) {
        const NLSyntaxView *a = nl_syntax_node_view(atoms[i]);
        const NLCheckedNodeView *operand =
            nl_checked_node_view(f, v->operands[i]);
        if (!a || !operand || v->operands[i] >= id ||
            (i == 0 && v->operands[0] >= v->operands[1]) ||
            v->values[i] == v->result ||
            !span_equal(a->span, v->operand_spans[i]) ||
            !span_equal(operand->span, v->operand_spans[i]) ||
            operand->type != v->type || operand->result_count != 1 ||
            operand->results[0].type != v->type ||
            operand->results[0].value != v->values[i] ||
            operand->next_argument != (i == 0 ? v->operands[1] : 0) ||
            !arithmetic_fact(c, v->values[i], v->type,
                             !fixed_divisor(operation) || i ||
                                 p->numerator_known,
                             v->numbers[i]))
            return false;
        if (fixed_divisor(operation) &&
            a->kind != (i ? NL_SYNTAX_USIZE_LITERAL : NL_SYNTAX_EXPR_NAME))
            return false;
        if (a->kind == NL_SYNTAX_USIZE_LITERAL) {
            if (operand->kind != NL_CHECKED_USIZE_LITERAL || v->symbols[i] ||
                v->places[i] || v->facts[i] || v->incarnations[i] ||
                !operand->has_scalar_result || !operand->scalar_result.known ||
                operand->scalar_result.type != v->type ||
                operand->scalar_result.value != v->numbers[i])
                return false;
            NLSourceView bytes;
            size_t number = 0;
            if (!nl_source_view(f->source, a->data.usize_digits, &bytes) ||
                !bytes.length)
                return false;
            for (size_t k = 0; k < bytes.length; ++k) {
                const unsigned char digit = bytes.bytes[k];
                if (digit < '0' || digit > '9' ||
                    number > (SIZE_MAX - (digit - '0')) / 10)
                    return false;
                number = number * 10 + (digit - '0');
            }
            if (number != v->numbers[i])
                return false;
        } else if (a->kind == NL_SYNTAX_EXPR_NAME) {
            NLSemanticBindingView b;
            NLSemanticPlaceView place;
            if (operand->kind != NL_CHECKED_IDENTIFIER ||
                operand->value_use != NL_VALUE_COPIED ||
                operand->symbol != v->symbols[i] || !operand->symbol ||
                operand->symbol > c->binding_count ||
                !token(f->source, a->data.name,
                       c->bindings[operand->symbol - 1].name) ||
                !span_equal(operand->name, a->data.name) ||
                !nl_semantic_binding_view(c, operand->symbol, &b) ||
                b.type != v->type || b.place != v->places[i] ||
                b.value != p->binding_values[i] ||
                !nl_semantic_place_view(c, b.place, &place) ||
                place.type != v->type ||
                (!place.live && (place.current_fact || place.current_value)) ||
                (place.live && (!place.current_fact || !place.current_value ||
                                place.current_fact > c->last_value_fact)) ||
                place.incarnation != v->incarnations[i] || !v->facts[i] ||
                v->facts[i] > c->last_value_fact ||
                !arithmetic_fact(c, p->binding_values[i], v->type,
                                 !fixed_divisor(operation) || i ||
                                     p->numerator_known,
                                 v->numbers[i]))
                return false;
        } else
            return false;
    }
    return true;
}

/* Preserve the existing Known-only private entry point as well as its public
 * wrappers. Unknown events never enter this +/- validator. */
static bool arithmetic_valid(const NLCheckedFragment *f, NLCheckedNodeId id,
                             NLUsizeOperation operation,
                             const NLUsizeArithmeticView *candidate)
{
    return (operation == NL_USIZE_ADD || operation == NL_USIZE_SUBTRACT) &&
           operation_valid_at(f, id, operation, candidate,
                              f ? f->context : NULL, NULL);
}

bool nl_usize_multiplication_valid(const NLCheckedFragment *f,
                                   NLCheckedNodeId id)
{
    return operation_valid_at(f, id, NL_USIZE_MULTIPLY, NULL,
                              f ? f->context : NULL, NULL);
}

/* Conversion only: no public layout casts, union punning or API migration.
 * Arguments are plain local lvalues. Numeric result fields are deliberately
 * excluded; each wrapper maps .sum OR .difference explicitly. */
#define COPY_METADATA(out, in)                                                 \
    do {                                                                       \
        (out).origin = (in).origin;                                            \
        (out).node = (in).node;                                                \
        (out).type = (in).type;                                                \
        (out).result = (in).result;                                            \
        (out).span = (in).span;                                                \
        (out).operator_span = (in).operator_span;                              \
        for (size_t i = 0; i < 2; ++i) {                                       \
            (out).operands[i] = (in).operands[i];                              \
            (out).values[i] = (in).values[i];                                  \
            (out).symbols[i] = (in).symbols[i];                                \
            (out).places[i] = (in).places[i];                                  \
            (out).incarnations[i] = (in).incarnations[i];                      \
            (out).facts[i] = (in).facts[i];                                    \
            (out).operand_spans[i] = (in).operand_spans[i];                    \
            (out).numbers[i] = (in).numbers[i];                                \
        }                                                                      \
    } while (0)

bool nl_checked_usize_addition_view(const NLCheckedFragment *f,
                                    NLCheckedNodeId id,
                                    NLUsizeAdditionView *out)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!p || p->guarded || p->operation != NL_USIZE_ADD || !out)
        return false;
    NLUsizeAdditionView v = {0};
    COPY_METADATA(v, p->issued);
    v.sum = p->issued.answer;
    *out = v;
    return true;
}
bool nl_checked_usize_addition_valid(const NLCheckedFragment *f,
                                     NLCheckedNodeId id,
                                     const NLUsizeAdditionView *candidate)
{
    if (!candidate)
        return arithmetic_valid(f, id, NL_USIZE_ADD, NULL);
    NLUsizeArithmeticView v = {0};
    COPY_METADATA(v, *candidate);
    v.answer = candidate->sum;
    return arithmetic_valid(f, id, NL_USIZE_ADD, &v);
}
bool nl_checked_usize_subtraction_view(const NLCheckedFragment *f,
                                       NLCheckedNodeId id,
                                       NLUsizeSubtractionView *out)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!p || p->operation != NL_USIZE_SUBTRACT || !out)
        return false;
    NLUsizeSubtractionView v = {0};
    COPY_METADATA(v, p->issued);
    v.difference = p->issued.answer;
    *out = v;
    return true;
}
bool nl_checked_usize_subtraction_valid(const NLCheckedFragment *f,
                                        NLCheckedNodeId id,
                                        const NLUsizeSubtractionView *candidate)
{
    if (!candidate)
        return arithmetic_valid(f, id, NL_USIZE_SUBTRACT, NULL);
    NLUsizeArithmeticView v = {0};
    COPY_METADATA(v, *candidate);
    v.answer = candidate->difference;
    return arithmetic_valid(f, id, NL_USIZE_SUBTRACT, &v);
}
bool nl_checked_usize_division_view(const NLCheckedFragment *f,
                                    NLCheckedNodeId id,
                                    NLUsizeDivisionView *out)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!p || p->guarded || p->operation != NL_USIZE_DIVIDE || !out)
        return false;
    NLUsizeDivisionView v = {0};
    COPY_METADATA(v, p->issued);
    v.quotient = p->issued.answer;
    v.numerator_known = p->numerator_known;
    v.quotient_known = p->result_known;
    v.target_max = SIZE_MAX;
    *out = v;
    return true;
}
bool nl_checked_usize_division_valid(const NLCheckedFragment *f,
                                     NLCheckedNodeId id,
                                     const NLUsizeDivisionView *candidate)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!p || p->operation != NL_USIZE_DIVIDE)
        return false;
    NLUsizeArithmeticView v = {0};
    if (candidate) {
        if (candidate->numerator_known != p->numerator_known ||
            candidate->quotient_known != p->result_known ||
            candidate->target_max != SIZE_MAX)
            return false;
        COPY_METADATA(v, *candidate);
        v.answer = candidate->quotient;
    }
    return operation_valid_at(f, id, NL_USIZE_DIVIDE, candidate ? &v : NULL,
                              f->context, NULL);
}

bool nl_checked_usize_remainder_view(const NLCheckedFragment *f,
                                     NLCheckedNodeId id,
                                     NLUsizeRemainderView *out)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!out || !p || p->guarded || p->operation != NL_USIZE_REMAINDER ||
        p->issued.origin != f || !f->source ||
        (f->body_owner && f->source != f->body_owner->source) ||
        !nl_h_custody_borrow_valid(f))
        return false;
    NLUsizeRemainderView v = {0};
    COPY_METADATA(v, p->issued);
    v.source = f->source;
    v.remainder = p->issued.answer;
    v.numerator_known = p->numerator_known;
    v.remainder_known = p->result_known;
    v.target_max = SIZE_MAX;
    *out = v;
    return true;
}

bool nl_checked_usize_remainder_valid(const NLCheckedFragment *f,
                                      NLCheckedNodeId id,
                                      const NLUsizeRemainderView *candidate)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!p || p->operation != NL_USIZE_REMAINDER ||
        !nl_h_custody_borrow_valid(f))
        return false;
    NLUsizeArithmeticView v = {0};
    if (candidate) {
        if (candidate->source != f->source ||
            candidate->numerator_known != p->numerator_known ||
            candidate->remainder_known != p->result_known ||
            candidate->target_max != SIZE_MAX)
            return false;
        COPY_METADATA(v, *candidate);
        v.answer = candidate->remainder;
    }
    return operation_valid_at(f, id, NL_USIZE_REMAINDER, candidate ? &v : NULL,
                              f->context, NULL);
}
#undef COPY_METADATA

/* Conditional arithmetic is a private mode of this same source issuer. A
 * bound is not a numeric answer and never enters the public Known views. */
static bool quantity(const NLSemanticContext *c, NLValueId id, NLTypeId type)
{
    NLSemanticValueView v;
    return nl_semantic_value_view(c, id, &v) && v.type == type &&
           (v.scalar_known || v.scalar_value == 0) &&
           v.dependencies == NL_DEPENDENCY_FREE && !v.value_dependency_count &&
           !v.domain && !v.reference.place && !v.reference.incarnation &&
           !v.reference.scope &&
           v.reference.provenance == NL_PROVENANCE_UNKNOWN &&
           !v.reference.readable && !v.reference.writable &&
           !v.reference.occurrence_dependency && !v.reference_count &&
           !v.slot_place && !v.allocation_region && !v.occupancy.region &&
           !v.occupancy.start && !v.occupancy.length && !v.aggregate_owner &&
           !v.field_count && !v.sum_owner && !v.sum_payload && !v.variant;
}
static const NLFunctionEntry *
quantity_source_function(const NLSemanticContext *c, size_t id, size_t count)
{
    if (!c || !c->functions || !id || id > c->function_count ||
        count > NL_QUANTITY_MAX_ARGUMENTS)
        return NULL;
    const NLFunctionEntry *fn = &c->functions[id - 1];
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    if (fn->kind != NL_CHECKED_REGISTERED_CALL || fn->count != count ||
        fn->result != type || !fn->body || !fn->body->syntax ||
        !fn->body->source || fn->body->count != count || fn->caller_effects ||
        fn->hidden_dependencies || fn->owner_receiver || fn->owner_producer ||
        fn->custody_recipient)
        return NULL;
    return fn;
}

const NLFunctionEntry *nl_quantity_usize_function(const NLSemanticContext *c,
                                                  size_t id, size_t count)
{
    const NLFunctionEntry *fn = quantity_source_function(c, id, count);
    if (!fn)
        return NULL;
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    bool has_boolean = false;
    for (size_t i = 0; i < count; ++i) {
        if (!fn->parameters || !fn->body->parameter_names[i])
            return NULL;
        if (fn->parameters[i] == type)
            continue;
        if (count != NL_QUANTITY_MAX_ARGUMENTS || has_boolean ||
            fn->parameters[i] != boolean)
            return NULL;
        has_boolean = true;
    }
    return fn;
}

const NLFunctionEntry *nl_quantity_bool_function(const NLSemanticContext *c,
                                                 size_t id)
{
    const NLFunctionEntry *fn = quantity_source_function(c, id, 1);
    return fn && fn->parameters && fn->body->parameter_names[0] &&
                   fn->parameters[0] == nl_semantic_core_type(c, NL_TYPE_BOOL)
               ? fn
               : NULL;
}
static const NLSyntaxNode *source_node(const NLFunctionBody *body,
                                       NLSyntaxKind kind, NLSourceSpan span)
{
    size_t count = 0;
    for (const NLSyntaxNode *s = body->syntax->nodes; s; s = s->owned_next) {
        if (++count > body->syntax->node_count)
            return NULL;
        const NLSyntaxView *v = nl_syntax_node_view(s);
        if (v && v->kind == kind && span_equal(v->span, span))
            return s;
    }
    return NULL;
}
static bool literal(const NLCheckedFragment *f, const NLSemanticContext *c,
                    const NLCheckedNodeView *n, const NLSyntaxNode *syntax,
                    size_t *number)
{
    const NLSyntaxView *s = nl_syntax_node_view(syntax);
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    NLSourceView text;
    if (!n || !s || s->kind != NL_SYNTAX_USIZE_LITERAL ||
        n->kind != NL_CHECKED_USIZE_LITERAL || n->type != type ||
        n->terminates || n->result_count != 1 || n->results[0].type != type ||
        !n->has_scalar_result || !n->scalar_result.known ||
        n->scalar_result.type != type || !span_equal(n->span, s->span) ||
        !plain(c, n->results[0].value, type, n->scalar_result.value) ||
        !nl_source_view(f->source, s->data.usize_digits, &text) || !text.length)
        return false;
    size_t parsed = 0;
    for (size_t i = 0; i < text.length; ++i) {
        const unsigned char ch = text.bytes[i];
        if (ch < '0' || ch > '9' || parsed > (SIZE_MAX - (ch - '0')) / 10)
            return false;
        parsed = parsed * 10 + ch - '0';
    }
    if (parsed != n->scalar_result.value)
        return false;
    *number = parsed;
    return true;
}
static bool same_binding(NLSemanticBindingView a, NLSemanticBindingView b)
{
    return a.type == b.type && a.place == b.place && a.value == b.value &&
           a.availability == b.availability;
}
static bool same_operand_place(NLSemanticPlaceView a, NLSemanticPlaceView b)
{
    return a.type == b.type && a.live && b.live &&
           a.incarnation == b.incarnation &&
           a.current_value == b.current_value && a.current_fact &&
           a.current_fact == b.current_fact && !a.governing_domain &&
           !b.governing_domain && !a.parent_aggregate && !b.parent_aggregate &&
           !a.parent_sum && !b.parent_sum;
}
static NLUsizeOperation guarded_operation(const NLSyntaxNode *syntax)
{
    const NLSyntaxView *source = nl_syntax_node_view(syntax);
    return source && source->kind == NL_SYNTAX_USIZE_MULTIPLICATION
               ? NL_USIZE_MULTIPLY
               : NL_USIZE_ADD;
}

/* One body-tail IF, one product on its true tail, and a literal fallback. */
static bool guarded_product_source(const NLFunctionEntry *fn,
                                   const NLSyntaxNode *choice,
                                   const NLSyntaxView *branch,
                                   const NLSyntaxNode *product)
{
    const NLSyntaxView *body =
        nl_syntax_node_view(nl_syntax_tree_root(fn->body->syntax));
    if (!body || body->kind != NL_SYNTAX_BLOCK || body->data.block.items ||
        body->data.block.tail != choice)
        return false;
    const NLSyntaxView *yes =
        nl_syntax_node_view(branch->data.conditional.then_block);
    const NLSyntaxView *no =
        nl_syntax_node_view(branch->data.conditional.else_block);
    if (!yes || yes->kind != NL_SYNTAX_BLOCK || yes->data.block.items ||
        yes->data.block.tail != product || !no || no->kind != NL_SYNTAX_BLOCK ||
        no->data.block.items)
        return false;
    const NLSyntaxView *fallback = nl_syntax_node_view(no->data.block.tail);
    return fallback && fallback->kind == NL_SYNTAX_USIZE_LITERAL;
}

static bool guard_shape(const NLCheckedFragment *f, const NLSemanticContext *c,
                        size_t function, NLSymbolId formal,
                        NLUsizeOperation operation, const NLSyntaxNode *syntax,
                        NLCheckedNodeId left, NLCheckedNodeId right,
                        size_t *limit, size_t *step)
{
    const NLFunctionEntry *fn = nl_quantity_usize_function(c, function, 1);
    const NLCheckedFragment *parent = f ? f->if_parent : NULL;
    const NLCheckedNodeView *choice =
        parent ? nl_checked_node_view(parent, f->if_parent_node) : NULL;
    const NLCheckedNodeView *test =
        choice ? nl_checked_node_view(parent, choice->initializer) : NULL;
    const NLCheckedNodeView *a =
        test ? nl_checked_node_view(parent, test->first_argument) : NULL;
    const NLCheckedNodeView *bound =
        a ? nl_checked_node_view(parent, a->next_argument) : NULL;
    const NLCheckedNodeView *x = f ? nl_checked_node_view(f, left) : NULL;
    const NLCheckedNodeView *k = f ? nl_checked_node_view(f, right) : NULL;
    const NLSyntaxView *arithmetic = nl_syntax_node_view(syntax);
    if (!fn || !f || !parent || !choice || !test || !a || !bound || !x || !k ||
        f->if_arm_index != 0 || f->source != parent->source ||
        f->source != fn->body->source || choice->kind != NL_CHECKED_IF ||
        test->kind != NL_CHECKED_USIZE_LESS_THAN ||
        choice->initializer >= f->if_parent_node || !arithmetic ||
        arithmetic->kind != arithmetic_operation[operation].syntax ||
        source_node(fn->body, arithmetic->kind, arithmetic->span) != syntax ||
        !formal || formal > c->binding_count ||
        a->kind != NL_CHECKED_IDENTIFIER || a->symbol != formal ||
        a->value_use != NL_VALUE_COPIED || a->result_count != 1 ||
        a->results[0].value > f->branch_value_prefix ||
        x->kind != NL_CHECKED_IDENTIFIER || x->symbol != formal ||
        x->value_use != NL_VALUE_COPIED || x->result_count != 1 ||
        !span_equal(
            x->span,
            nl_syntax_node_view(arithmetic->data.equality.left)->span) ||
        !token(f->source, x->name, fn->body->parameter_names[0]) ||
        !token(parent->source, a->name, fn->body->parameter_names[0]) ||
        !c->bindings[formal - 1].name ||
        strcmp(c->bindings[formal - 1].name, fn->body->parameter_names[0]) ||
        bound->next_argument || k->next_argument)
        return false;
    const NLSyntaxNode *if_source =
        source_node(fn->body, NL_SYNTAX_IF, choice->span);
    const NLSyntaxView *branch = nl_syntax_node_view(if_source);
    const NLSyntaxView *guard =
        branch ? nl_syntax_node_view(branch->data.conditional.condition) : NULL;
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    NLSemanticBindingView binding;
    NLSemanticPlaceView place;
    if (!guard || guard->kind != NL_SYNTAX_USIZE_LESS_THAN ||
        !span_equal(guard->span, test->span) ||
        !span_equal(test->name, guard->data.equality.operator_span) ||
        !token(f->source, test->name, "<") ||
        !span_equal(a->span,
                    nl_syntax_node_view(guard->data.equality.left)->span) ||
        !literal(f, c, bound, guard->data.equality.right, limit) ||
        !literal(f, c, k, arithmetic->data.equality.right, step) || !*limit ||
        (operation == NL_USIZE_MULTIPLY
             ? !guarded_product_source(fn, if_source, branch, syntax) ||
                   !nl_usize_product_fits(*limit - 1, *step)
             : *step > SIZE_MAX - (*limit - 1)) ||
        !nl_semantic_binding_view(c, formal, &binding) ||
        binding.type != type || binding.availability != NL_AVAILABLE ||
        !nl_semantic_place_view(c, binding.place, &place) || !place.live ||
        !place.current_fact || place.current_value != binding.value ||
        !quantity(c, binding.value, type) ||
        !quantity(c, a->results[0].value, type) ||
        !quantity(c, x->results[0].value, type))
        return false;
    const NLSemanticValueView original = c->values[binding.value - 1];
    const NLSemanticValueView copied[] = {c->values[a->results[0].value - 1],
                                          c->values[x->results[0].value - 1]};
    for (size_t i = 0; i < 2; ++i)
        if (copied[i].scalar_known != original.scalar_known ||
            copied[i].scalar_value != original.scalar_value)
            return false;
    return true;
}
bool nl_guarded_usize_permitted(const NLCheckedFragment *f,
                                const NLSemanticContext *c,
                                const NLSemanticContext *entry, size_t function,
                                NLSymbolId formal, const NLSyntaxNode *syntax,
                                NLCheckedNodeId left, NLCheckedNodeId right)
{
    size_t limit, step;
    NLSemanticBindingView before, now;
    NLSemanticPlaceView a, b;
    return entry && entry != c &&
           guard_shape(f, c, function, formal, guarded_operation(syntax),
                       syntax, left, right, &limit, &step) &&
           nl_semantic_binding_view(entry, formal, &before) &&
           nl_semantic_binding_view(c, formal, &now) &&
           same_binding(before, now) &&
           nl_semantic_place_view(entry, before.place, &a) &&
           nl_semantic_place_view(c, now.place, &b) && same_operand_place(a, b);
}
NLCheckStatus nl_guarded_usize_capture(NLCheckedFragment *f, NLCheckedNodeId id,
                                       const NLSyntaxNode *syntax,
                                       const NLSemanticContext *c,
                                       const NLSemanticContext *entry,
                                       size_t function, NLSymbolId formal)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const NLCheckedNodeView *left =
        n ? nl_checked_node_view(f, n->first_argument) : NULL;
    const NLUsizeOperation operation = guarded_operation(syntax);
    if (!n || !left || n->kind != arithmetic_operation[operation].checked ||
        !nl_guarded_usize_permitted(f, c, entry, function, formal, syntax,
                                    n->first_argument, left->next_argument))
        return NL_CHECK_INTERNAL_ERROR;
    NLUsizeAdditionProof *p = calloc(1, sizeof(*p));
    if (!p)
        return NL_CHECK_OUT_OF_MEMORY;
    p->guarded = true;
    p->operation = operation;
    p->syntax = syntax;
    p->context = f->context;
    p->guard_parent = f->if_parent;
    p->conditional = f->if_parent_node;
    p->function = function;
    p->formal = formal;
    p->guard_binding = c->bindings[formal - 1].view;
    p->guard_place = c->places[p->guard_binding.place - 1];
    (void)guard_shape(f, c, function, formal, operation, syntax,
                      n->first_argument, left->next_argument, &p->limit,
                      &p->step);
    const NLCheckedNodeView *right =
        nl_checked_node_view(f, left->next_argument);
    p->issued = (NLUsizeArithmeticView){
        .origin = f,
        .node = id,
        .type = n->type,
        .result = n->results[0].value,
        .span = n->span,
        .operator_span = n->name,
        .values = {left->results[0].value, right->results[0].value},
        .operands = {n->first_argument, left->next_argument}};
    p->next = f->usize_additions;
    f->usize_additions = p;
    f->destroy_usize_additions = destroy;
    return NL_CHECK_OK;
}
static bool guarded_event(const NLCheckedFragment *f, NLCheckedNodeId id,
                          size_t function, NLSymbolId formal,
                          NLUsizeOperation operation, NLGuardedUsizePlan *out)
{
    const NLUsizeAdditionProof *p = find(f, id);
    const NLSemanticContext *c = f ? f->context : NULL;
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const NLCheckedNodeView *left =
        p ? nl_checked_node_view(f, p->issued.operands[0]) : NULL;
    const NLCheckedNodeView *right =
        p ? nl_checked_node_view(f, p->issued.operands[1]) : NULL;
    size_t limit, step;
    NLSemanticValueView value;
    if (!p || !p->guarded || p->operation != operation || !c || !n || !left ||
        !right || left->next_argument != p->issued.operands[1] ||
        left->results[0].value != p->issued.values[0] ||
        right->results[0].value != p->issued.values[1] ||
        p->issued.origin != f || p->context != c || p->function != function ||
        p->formal != formal || p->guard_parent != f->if_parent ||
        p->conditional != f->if_parent_node ||
        nl_checked_if_arm(f->if_parent, p->conditional, 0) != f ||
        !guard_shape(f, c, function, formal, operation, p->syntax,
                     p->issued.operands[0], p->issued.operands[1], &limit,
                     &step) ||
        limit != p->limit || step != p->step ||
        !same_binding(p->guard_binding, c->bindings[formal - 1].view) ||
        !same_operand_place(p->guard_place,
                            c->places[p->guard_binding.place - 1]) ||
        n->kind != arithmetic_operation[operation].checked || n->terminates ||
        n->argument_count != 2 || n->first_argument != p->issued.operands[0] ||
        n->result_count != 1 || n->type != p->issued.type ||
        n->results[0].value != p->issued.result ||
        !span_equal(n->span, p->issued.span) ||
        !span_equal(n->name, p->issued.operator_span) ||
        !token(f->source, n->name, arithmetic_operation[operation].token) ||
        !n->has_scalar_result || n->scalar_result.type != p->issued.type ||
        n->scalar_result.known || n->scalar_result.value ||
        !quantity(c, n->results[0].value, n->type) ||
        !nl_semantic_value_view(c, n->results[0].value, &value) ||
        value.scalar_known || value.scalar_value)
        return false;
    if (operation == NL_USIZE_ADD)
        out->addition = id;
    out->limit = limit;
    out->step = step;
    return true;
}

static bool scalar_node_value(const NLCheckedFragment *f,
                              const NLSemanticContext *c, NLCheckedNodeId id,
                              NLTypeId type, NLValueId *value)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (!n || n->terminates || n->result_count != 1 || n->type != type ||
        n->results[0].type != type || !quantity(c, n->results[0].value, type) ||
        (type == nl_semantic_core_type(c, NL_TYPE_BOOL) &&
         c->values[n->results[0].value - 1].scalar_value > 1))
        return false;
    *value = n->results[0].value;
    return true;
}
static bool node_value(const NLCheckedFragment *f, const NLSemanticContext *c,
                       NLCheckedNodeId id, NLValueId *value)
{
    return scalar_node_value(f, c, id, nl_semantic_core_type(c, NL_TYPE_USIZE),
                             value);
}
static bool binding_source_shape(const NLCheckedFragment *f,
                                 const NLSemanticContext *c,
                                 const NLCheckedNodeView *n,
                                 const NLSyntaxView *s)
{
    if (!s || s->kind != NL_SYNTAX_BINDING || !n ||
        n->kind != NL_CHECKED_BINDING || n->terminates || n->result_count ||
        n->argument_count != 1 || !n->symbol || n->symbol > c->binding_count ||
        !span_equal(n->span, s->span))
        return false;
    const NLCheckedNodeView *r = nl_checked_node_view(f, n->first_argument);
    const NLCheckedNodeView *init = nl_checked_node_view(f, n->initializer);
    return r && init && r->kind == NL_CHECKED_RECEIVER &&
           r->symbol == n->symbol && !r->next_argument &&
           !n->results[0].value && r->type == init->type &&
           init->result_count == 1 &&
           span_equal(init->span,
                      nl_syntax_node_view(s->data.binding.initializer)->span) &&
           token(f->source, s->data.binding.name,
                 c->bindings[n->symbol - 1].name) &&
           span_equal(r->name, s->data.binding.name) &&
           c->bindings[n->symbol - 1].view.type == init->type;
}
static bool binding_source(const NLCheckedFragment *f,
                           const NLSemanticContext *c,
                           const NLCheckedNodeView *n, const NLSyntaxView *s)
{
    return binding_source_shape(f, c, n, s) &&
           c->bindings[n->symbol - 1].view.value ==
               nl_checked_node_view(f, n->initializer)->results[0].value;
}
static bool scalar_name_copy(const NLCheckedFragment *f,
                             const NLSemanticContext *c, NLCheckedNodeId id,
                             const NLSyntaxNode *syntax, NLSymbolId symbol,
                             NLTypeId type)
{
    const NLSyntaxView *s = nl_syntax_node_view(syntax);
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    NLValueId value;
    if (!s || s->kind != NL_SYNTAX_EXPR_NAME || !n ||
        n->kind != NL_CHECKED_IDENTIFIER || n->value_use != NL_VALUE_COPIED ||
        !symbol || symbol > c->binding_count || n->symbol != symbol ||
        !span_equal(s->span, n->span) || !span_equal(s->data.name, n->name) ||
        !token(f->source, n->name, c->bindings[symbol - 1].name) ||
        !scalar_node_value(f, c, id, type, &value))
        return false;
    NLValueId original = c->bindings[symbol - 1].view.value;
    if (!quantity(c, original, n->type))
        return false;
    return c->values[value - 1].scalar_known ==
               c->values[original - 1].scalar_known &&
           c->values[value - 1].scalar_value ==
               c->values[original - 1].scalar_value;
}
static bool name_copy(const NLCheckedFragment *f, const NLSemanticContext *c,
                      NLCheckedNodeId id, const NLSyntaxNode *syntax,
                      NLSymbolId symbol)
{
    return scalar_name_copy(f, c, id, syntax, symbol,
                            nl_semantic_core_type(c, NL_TYPE_USIZE));
}
static bool helper_body(const NLCheckedFragment *f, const NLSemanticContext *c,
                        const NLFunctionEntry *fn, NLUsizeOperation operation,
                        NLGuardedUsizePlan *p)
{
    const NLSyntaxView *source =
        nl_syntax_node_view(nl_syntax_tree_root(fn->body->syntax));
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    const NLCheckedNodeView *choice =
        root ? nl_checked_node_view(f, root->tail) : NULL;
    const NLSyntaxView *s =
        source ? nl_syntax_node_view(source->data.block.tail) : NULL;
    if (!source || source->kind != NL_SYNTAX_BLOCK ||
        source->data.block.items || !root || root->kind != NL_CHECKED_BLOCK ||
        root->first_item || !choice || !s || s->kind != NL_SYNTAX_IF ||
        choice->kind != NL_CHECKED_IF || choice->normal_arms != 2 ||
        choice->terminates || !span_equal(root->span, source->span) ||
        !span_equal(choice->span, s->span))
        return false;
    NLValueId returned, selected;
    if (!node_value(f, c, f->root, &returned) ||
        !node_value(f, c, root->tail, &selected) || returned != selected ||
        c->values[selected - 1].scalar_known ||
        c->values[selected - 1].scalar_value)
        return false;
    const NLCheckedNodeView *test =
        nl_checked_node_view(f, choice->initializer);
    const NLCheckedNodeView *operand =
        test ? nl_checked_node_view(f, test->first_argument) : NULL;
    if (!operand || !operand->symbol)
        return false;
    p->formal = operand->symbol;
    p->conditional = root->tail;
    const NLSyntaxNode *syntax[] = {s->data.conditional.then_block,
                                    s->data.conditional.else_block};
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, root->tail, a);
        const NLSyntaxView *block_source = nl_syntax_node_view(syntax[a]);
        const NLCheckedNodeView *ar =
            arm ? nl_checked_node_view(arm, arm->root) : NULL;
        const NLCheckedNodeView *block =
            ar ? nl_checked_node_view(arm, ar->initializer) : NULL;
        if (!arm || arm->if_parent != f || arm->if_parent_node != root->tail ||
            arm->if_arm_index != a || arm->source != f->source || !ar ||
            ar->kind != NL_CHECKED_IF_ARM || ar->terminates || !block ||
            block->kind != NL_CHECKED_BLOCK || block->terminates ||
            !block_source || block_source->kind != NL_SYNTAX_BLOCK ||
            !span_equal(block->span, block_source->span))
            return false;
        NLValueId block_value, arm_value, tail_value;
        if (!node_value(arm, arm->context, ar->initializer, &block_value) ||
            !node_value(arm, arm->context, arm->root, &arm_value) ||
            !node_value(arm, arm->context, block->tail, &tail_value) ||
            block_value != arm_value || block_value != tail_value)
            return false;
        if (operation == NL_USIZE_MULTIPLY) {
            if (block->first_item || block_source->data.block.items)
                return false;
            if (a == 0) {
                if (!guarded_event(arm, block->tail, p->function, p->formal,
                                   operation, p))
                    return false;
            } else {
                size_t fallback;
                if (!literal(arm, arm->context,
                             nl_checked_node_view(arm, block->tail),
                             block_source->data.block.tail, &fallback))
                    return false;
            }
        } else if (a == 0) {
            const NLSyntaxNode *item = block_source->data.block.items;
            const NLCheckedNodeView *b =
                nl_checked_node_view(arm, block->first_item);
            if (!item || nl_syntax_next_argument(item) || !b || b->next_item ||
                !binding_source(arm, arm->context, b,
                                nl_syntax_node_view(item)) ||
                !guarded_event(arm, b->initializer, p->function, p->formal,
                               operation, p) ||
                !name_copy(arm, arm->context, block->tail,
                           block_source->data.block.tail, b->symbol))
                return false;
        } else if (block->first_item || block_source->data.block.items ||
                   !name_copy(arm, arm->context, block->tail,
                              block_source->data.block.tail, p->formal))
            return false;
    }
    return true;
}
bool nl_guarded_usize_body_at(const NLCheckedFragment *f,
                              const NLSemanticContext *c, size_t function,
                              NLGuardedUsizePlan *out)
{
    const NLFunctionEntry *fn = nl_quantity_usize_function(c, function, 1);
    if (!fn)
        fn = nl_quantity_usize_function(c, function, 0);
    if (!f || !fn || !out || f->body_owner != fn->body ||
        f->source != fn->body->source)
        return false;
    NLGuardedUsizePlan plan = {.body = f, .function = function};
    if (fn->count) {
        if (!helper_body(f, c, fn, NL_USIZE_ADD, &plan))
            return false;
    } else {
        const NLSyntaxView *s =
            nl_syntax_node_view(nl_syntax_tree_root(fn->body->syntax));
        const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
        if (!s || s->kind != NL_SYNTAX_BLOCK || !root ||
            root->kind != NL_CHECKED_BLOCK || root->terminates ||
            !span_equal(root->span, s->span))
            return false;
        NLCheckedNodeId id = root->first_item;
        NLSymbolId previous = 0;
        size_t count = 0;
        for (const NLSyntaxNode *item = s->data.block.items; item;
             item = nl_syntax_next_argument(item)) {
            const NLSyntaxView *b = nl_syntax_node_view(item);
            const NLCheckedNodeView *n = nl_checked_node_view(f, id);
            if (++count > NL_PARSER_MAX_NODES || !binding_source(f, c, n, b))
                return false;
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, n->initializer);
            if (init && init->kind == NL_CHECKED_REGISTERED_CALL) {
                const NLSyntaxView *call =
                    nl_syntax_node_view(b->data.binding.initializer);
                if (!call || call->kind != NL_SYNTAX_EXPR_CALL || !init ||
                    init->kind != NL_CHECKED_REGISTERED_CALL ||
                    !init->body_backed || init->argument_count != 1 ||
                    init->terminates)
                    return false;
                const NLFunctionEntry *target =
                    nl_quantity_usize_function(c, init->function, 1);
                const NLCheckedFragment *body =
                    nl_checked_call_body(f, n->initializer);
                NLGuardedUsizePlan nested = {.body = body,
                                             .function = init->function};
                const NLCheckedNodeView *arg =
                    nl_checked_node_view(f, init->first_argument);
                size_t actual;
                NLValueId result, inner;
                if (!target || !body || body->body_owner != target->body ||
                    body->source != target->body->source ||
                    !helper_body(body, c, target, NL_USIZE_ADD, &nested) ||
                    !arg || arg->next_argument ||
                    !(previous ? name_copy(f, c, init->first_argument,
                                           call->data.call.arguments, previous)
                               : literal(f, c, arg, call->data.call.arguments,
                                         &actual)) ||
                    !token(f->source, init->name, target->name) ||
                    !node_value(f, c, n->initializer, &result) ||
                    !node_value(body, c, body->root, &inner) ||
                    result != inner ||
                    c->bindings[nested.formal - 1].view.value !=
                        arg->results[0].value)
                    return false;
                if (!plan.call)
                    plan.call = n->initializer;
            } else if (!name_copy(f, c, n->initializer,
                                  b->data.binding.initializer, previous))
                return false;
            previous = n->symbol;
            id = n->next_item;
        }
        NLValueId result, tail;
        if (!previous || id ||
            !name_copy(f, c, root->tail, s->data.block.tail, previous) ||
            !node_value(f, c, f->root, &result) ||
            !node_value(f, c, root->tail, &tail) || result != tail)
            return false;
    }
    *out = plan;
    return true;
}
bool nl_guarded_usize_call_valid(const NLCheckedFragment *f,
                                 NLCheckedNodeId call, NLGuardedUsizePlan *out)
{
    if (!out || nl_checked_proof_class(f) != NL_CHECKED_MACHINE_PROVED ||
        !nl_h_custody_result_valid(f, call))
        return false;
    const NLCheckedNodeView *n = nl_checked_node_view(f, call);
    const NLCheckedFragment *body = nl_checked_call_body(f, call);
    NLGuardedUsizePlan plan;
    NLValueId actual, returned;
    if (!n || !body || n->kind != NL_CHECKED_REGISTERED_CALL || n->terminates ||
        !n->body_backed ||
        !nl_guarded_usize_body_at(body, f->context, n->function, &plan) ||
        !node_value(f, f->context, call, &actual) ||
        !node_value(body, f->context, body->root, &returned) ||
        actual != returned)
        return false;
    /* Each call has its own CHECK-owned body and result seal, including
     * repeated calls to the same declaration with fresh formal bindings.
     * Authenticate all of them, not just the first producer. The outer seal
     * covers the complete final caller world and Copy chain. */
    if (plan.call) {
        const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
        for (NLCheckedNodeId id = root->first_item; id;) {
            const NLCheckedNodeView *binding = nl_checked_node_view(body, id);
            const NLCheckedNodeView *init =
                nl_checked_node_view(body, binding->initializer);
            if (init->kind == NL_CHECKED_REGISTERED_CALL &&
                !nl_h_custody_result_valid(body, binding->initializer))
                return false;
            id = binding->next_item;
        }
    }
    *out = plan;
    return true;
}

static bool literal_divisor_operands(const NLCheckedFragment *f,
                                     const NLSemanticContext *c,
                                     const NLSyntaxNode *syntax,
                                     NLSyntaxKind kind, NLCheckedNodeId left,
                                     NLCheckedNodeId right)
{
    const NLSyntaxView *s = nl_syntax_node_view(syntax);
    const NLSyntaxView *a =
        s ? nl_syntax_node_view(s->data.equality.left) : NULL;
    const NLCheckedNodeView *x = nl_checked_node_view(f, left);
    const NLCheckedNodeView *k = nl_checked_node_view(f, right);
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    NLSemanticBindingView binding;
    NLSemanticPlaceView place;
    size_t divisor;
    if (!s || s->kind != kind || !a || a->kind != NL_SYNTAX_EXPR_NAME || !x ||
        !k || x->kind != NL_CHECKED_IDENTIFIER ||
        x->value_use != NL_VALUE_COPIED || x->type != type ||
        x->result_count != 1 || !quantity(c, x->results[0].value, type) ||
        !x->symbol || x->symbol > c->binding_count ||
        !nl_semantic_binding_view(c, x->symbol, &binding) ||
        binding.type != type || binding.availability != NL_AVAILABLE ||
        !nl_semantic_place_view(c, binding.place, &place) || !place.live ||
        !place.current_fact || place.current_value != binding.value ||
        place.type != type || !quantity(c, binding.value, type) ||
        !token(f->source, a->data.name, c->bindings[x->symbol - 1].name) ||
        !span_equal(x->span, a->span) || !span_equal(x->name, a->data.name) ||
        !literal(f, c, k, s->data.equality.right, &divisor))
        return false;
    const NLSemanticValueView original = c->values[binding.value - 1];
    return arithmetic_fact(c, x->results[0].value, type, original.scalar_known,
                           original.scalar_value);
}

bool nl_usize_division_operands(const NLCheckedFragment *f,
                                const NLSemanticContext *c,
                                const NLSyntaxNode *syntax,
                                NLCheckedNodeId left, NLCheckedNodeId right)
{
    return literal_divisor_operands(f, c, syntax, NL_SYNTAX_USIZE_DIVISION,
                                    left, right);
}

bool nl_usize_remainder_operands(const NLCheckedFragment *f,
                                 const NLSemanticContext *c,
                                 const NLSyntaxNode *syntax,
                                 NLCheckedNodeId left, NLCheckedNodeId right)
{
    return literal_divisor_operands(f, c, syntax, NL_SYNTAX_USIZE_REMAINDER,
                                    left, right);
}

/* These consumers borrow the checked lexical order and existing result/world
 * custody. A Copy's symbol and fresh checked value belong to that graph; the
 * scalar facts checked by name_copy are only a consistency condition. */
static NLCheckedNodeId division_local_before(const NLCheckedFragment *f,
                                             NLCheckedNodeId before,
                                             NLSymbolId symbol)
{
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    for (NLCheckedNodeId id = root->first_item; id && id < before;) {
        const NLCheckedNodeView *binding = nl_checked_node_view(f, id);
        if (!binding ||
            (binding->kind != NL_CHECKED_BINDING &&
             binding->kind != NL_CHECKED_STATEMENT) ||
            (binding->next_item && binding->next_item <= id))
            return 0;
        if (binding->kind == NL_CHECKED_BINDING && binding->symbol == symbol)
            return id;
        id = binding->next_item;
    }
    return 0;
}

static bool division_name_copy(const NLCheckedFragment *f,
                               const NLSemanticContext *c, NLCheckedNodeId id,
                               const NLSyntaxNode *syntax, NLSymbolId symbol)
{
    return name_copy(f, c, id, syntax, symbol) &&
           c->bindings[symbol - 1].view.value <
               nl_checked_node_view(f, id)->results[0].value;
}

static bool division_copy_at(const NLCheckedFragment *f,
                             const NLSemanticContext *c, NLCheckedNodeId id,
                             const NLSyntaxNode *syntax,
                             NLCheckedNodeId first_binding)
{
    const NLCheckedNodeView *use = nl_checked_node_view(f, id);
    const NLCheckedNodeId binding =
        use ? division_local_before(f, id, use->symbol) : 0;
    return binding && binding >= first_binding &&
           division_name_copy(f, c, id, syntax, use->symbol);
}

static const NLSyntaxNode *discarded_quantity_expression(
    const NLCheckedFragment *f, const NLSemanticContext *c, NLCheckedNodeId id,
    const NLCheckedNodeView *node, const NLSyntaxView *syntax, NLTypeId type,
    NLSyntaxKind kind)
{
    if (node->kind != NL_CHECKED_STATEMENT || node->terminates ||
        node->type != nl_semantic_unit_type(c) || node->result_count ||
        node->results[0].value || node->symbol || node->argument_count ||
        node->first_argument || !node->initializer || node->initializer >= id ||
        !span_equal(node->span, syntax->span))
        return NULL;
    const NLSyntaxNode *expression = syntax->data.statement.expression;
    const NLSyntaxView *source = nl_syntax_node_view(expression);
    NLValueId value;
    if (!source || source->kind != kind ||
        !scalar_node_value(f, c, node->initializer, type, &value))
        return NULL;
    const NLSemanticValueView *result = &c->values[value - 1];
    const NLSemanticTypeView *properties = &c->types[type - 1].view;
    /* The statement owns no result. CHECK discharged the expression's
     * package; the caller still authenticates the Copy or call itself. */
    return properties->is_copy && properties->is_discardable &&
                   result->carrier == NL_CARRIER_ENDED && !result->owner_place
               ? expression
               : NULL;
}

/* Before the operation, each previously accepted local is rooted in this
 * same formal. The selected operand can borrow one of those bindings. */
static bool division_input_copy(const NLCheckedFragment *f,
                                const NLSemanticContext *c,
                                const NLFunctionEntry *fn, NLCheckedNodeId id,
                                const NLSyntaxNode *syntax,
                                NLDivisionUsizePlan *p)
{
    const NLCheckedNodeView *use = nl_checked_node_view(f, id);
    if (!use)
        return false;
    if (p->formal && use->symbol != p->formal)
        return division_copy_at(f, c, id, syntax, 0);
    if (!token(f->source, use->name, fn->body->parameter_names[0]) ||
        !division_name_copy(f, c, id, syntax, use->symbol))
        return false;
    p->formal = use->symbol;
    return true;
}

static bool division_event_at(const NLCheckedFragment *f,
                              const NLSemanticContext *c,
                              const NLFunctionEntry *fn, NLCheckedNodeId id,
                              const NLSyntaxNode *src,
                              NLUsizeOperation operation,
                              NLDivisionUsizePlan *p)
{
    const NLUsizeAdditionProof *receipt = find(f, id);
    if (!receipt || receipt->syntax != src ||
        !fixed_divisor(receipt->operation) ||
        (operation != NL_USIZE_ANY_FIXED_DIVISOR &&
         operation != receipt->operation) ||
        !operation_valid_at(f, id, receipt->operation, NULL, c, NULL) ||
        !division_input_copy(f, c, fn, receipt->issued.operands[0],
                             nl_syntax_node_view(src)->data.equality.left, p))
        return false;
    p->division = id;
    p->divisor = receipt->issued.numbers[1];
    return true;
}

static bool division_return_package(const NLCheckedFragment *f,
                                    const NLSemanticContext *c,
                                    const NLCheckedNodeView *root,
                                    const NLCheckedNodeView *ret,
                                    const NLSyntaxView *syntax)
{
    NLValueId value;
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLControlExitView *exit = nl_control_exit_view(f->exits, 0);
    /* The exit owns its post-state and IDs. Match the returned package in the
     * committed world via the issued return and root nodes, not across worlds.
     */
    return root->terminates && !root->tail && !root->type &&
           !root->result_count && ret && ret->kind == NL_CHECKED_RETURN &&
           ret->terminates && !ret->next_item && !ret->result_count &&
           span_equal(ret->span, syntax->span) &&
           nl_control_exits_count(f->exits) == 1 && exit && exit->count == 1 &&
           exit->values[0].type == type &&
           nl_control_exits_only_return(f->exits, f->function_target) &&
           node_value(f, c, ret->initializer, &value) &&
           ret->returned.type == type && ret->returned.value == value &&
           root->returned.type == type && root->returned.value == value;
}

static bool division_helper(const NLCheckedFragment *f,
                            const NLSemanticContext *c, size_t function,
                            NLUsizeOperation operation,
                            NLDivisionUsizePlan *out)
{
    const NLFunctionEntry *fn = nl_quantity_usize_function(c, function, 1);
    if (!f || !fn || f->body_owner != fn->body ||
        f->source != fn->body->source || f->body_count || f->arm_count)
        return false;
    const NLSyntaxView *s =
        nl_syntax_node_view(nl_syntax_tree_root(fn->body->syntax));
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    if (!s || s->kind != NL_SYNTAX_BLOCK || !root ||
        root->kind != NL_CHECKED_BLOCK || !span_equal(root->span, s->span))
        return false;
    NLDivisionUsizePlan plan = {.body = f, .function = function};
    NLCheckedNodeId id = root->first_item;
    NLCheckedNodeId operation_binding = 0;
    size_t count = 0;
    for (const NLSyntaxNode *item = s->data.block.items; item;
         item = nl_syntax_next_argument(item)) {
        const NLSyntaxView *src = nl_syntax_node_view(item);
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (++count > NL_PARSER_MAX_NODES || !src || !n)
            return false;
        if (src->kind == NL_SYNTAX_RETURN) {
            if (!plan.division || nl_syntax_next_argument(item) ||
                s->data.block.tail || count != root->item_count ||
                !division_return_package(f, c, root, n, src) ||
                !division_copy_at(f, c, n->initializer,
                                  src->data.statement.expression,
                                  operation_binding))
                return false;
            plan.result = n->initializer;
            *out = plan;
            return true;
        }
        if (!binding_source(f, c, n, src) ||
            (n->next_item && n->next_item <= id))
            return false;
        const NLSyntaxNode *init = src->data.binding.initializer;
        if (operation_binding) {
            /* Result copies can only start at the operation's binding. */
            if (!division_copy_at(f, c, n->initializer, init,
                                  operation_binding))
                return false;
        } else if (nl_checked_node_view(f, n->initializer)->kind ==
                   NL_CHECKED_IDENTIFIER) {
            if (!division_input_copy(f, c, fn, n->initializer, init, &plan))
                return false;
        } else {
            if (!division_event_at(f, c, fn, n->initializer, init, operation,
                                   &plan))
                return false;
            operation_binding = id;
        }
        id = n->next_item;
    }
    const NLSyntaxNode *tail = s->data.block.tail;
    NLValueId body_result, tail_value;
    if (id || count != root->item_count || nl_control_exits_count(f->exits) ||
        !node_value(f, c, f->root, &body_result) ||
        !node_value(f, c, root->tail, &tail_value) || body_result != tail_value)
        return false;
    if (operation_binding
            ? !division_copy_at(f, c, root->tail, tail, operation_binding)
            : !division_event_at(f, c, fn, root->tail, tail, operation, &plan))
        return false;
    plan.result = root->tail;
    *out = plan;
    return true;
}

static bool quantity_main_body(const NLCheckedFragment *caller,
                               const NLSemanticContext *c)
{
    if (!caller->body_owner || caller->body_owner->count ||
        caller->source != caller->body_owner->source)
        return false;
    for (size_t i = 0; i < c->function_count; ++i) {
        const NLFunctionEntry *fn = &c->functions[i];
        if (fn->kind == NL_CHECKED_REGISTERED_CALL && !fn->count &&
            fn->body == caller->body_owner && !strcmp(fn->name, "main") &&
            fn->result == nl_semantic_unit_type(c))
            return true;
    }
    return false;
}

/* A completed lexical parameter has a historical row for its exact slot.
 * Read it only through the reserved or committed BODY of this invocation. */
static bool quantity_copy_formals(const NLCheckedFragment *body,
                                  const NLSemanticContext *c,
                                  const NLFunctionEntry *fn, size_t function,
                                  NLSymbolId formals[NL_QUANTITY_MAX_ARGUMENTS])
{
    const size_t count = fn->count;
    const bool single = count == 1;
    if (count != NL_QUANTITY_MAX_ARGUMENTS &&
        !(single &&
          fn->parameters[0] == nl_semantic_core_type(c, NL_TYPE_BOOL)))
        return false;
    NLCheckedNodeId id = 0;
    const NLCheckedFragment *caller = nl_h_custody_body_caller(body, c, &id);
    const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
    if (!call || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || call->terminates || call->function != function ||
        call->argument_count != count || call->first_argument <= id ||
        (single && !quantity_main_body(caller, c)) ||
        c->binding_count > NL_SEMANTIC_MAX_ENTRIES)
        return false;
    NLValueId values[NL_QUANTITY_MAX_ARGUMENTS];
    NLCheckedNodeId actual = call->first_argument;
    for (size_t i = 0; i < count; ++i) {
        const NLCheckedNodeView *arg = nl_checked_node_view(caller, actual);
        const NLTypeId type = fn->parameters[i];
        if (!arg || (arg->next_argument && arg->next_argument <= actual) ||
            (single && (arg->kind != NL_CHECKED_IDENTIFIER ||
                        arg->value_use != NL_VALUE_COPIED)) ||
            arg->parameter_type != type ||
            !scalar_node_value(caller, c, actual, type, &values[i]) ||
            c->values[values[i] - 1].carrier != NL_CARRIER_ENDED ||
            c->values[values[i] - 1].owner_place)
            return false;
        actual = arg->next_argument;
    }
    if (actual || (!single && values[0] >= values[1]))
        return false;
    NLPlaceId places[NL_QUANTITY_MAX_ARGUMENTS];
    NLIncarnationId incarnations[NL_QUANTITY_MAX_ARGUMENTS];
    for (size_t i = 0; i < c->binding_count; ++i) {
        const NLBindingEntry *binding = &c->bindings[i];
        for (size_t j = 0; j < count; ++j) {
            if (binding->view.value != values[j])
                continue;
            const NLTypeId type = fn->parameters[j];
            NLSemanticPlaceView place;
            if (formals[j] || !binding->hidden ||
                binding->view.availability != NL_CONSUMED ||
                binding->view.type != type ||
                strcmp(binding->name, fn->body->parameter_names[j]) ||
                !nl_semantic_place_view(c, binding->view.place, &place) ||
                place.type != type || !place.independent_root ||
                !place.incarnation || place.live || place.current_value ||
                place.current_fact)
                return false;
            formals[j] = i + 1;
            places[j] = binding->view.place;
            incarnations[j] = place.incarnation;
        }
    }
    return formals[0] && (single || (formals[0] + 1 == formals[1] &&
                                     places[0] + 1 == places[1] &&
                                     incarnations[0] + 1 == incarnations[1]));
}

static bool division_call_values(const NLCheckedFragment *f,
                                 const NLSemanticContext *c, NLCheckedNodeId id,
                                 const NLDivisionUsizePlan *helper)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLSymbolId formals[] = {helper->formal, helper->second_formal};
    const size_t count = helper->second_formal ? 2 : helper->formal != 0;
    const NLFunctionEntry *fn =
        nl_quantity_usize_function(c, helper->function, count);
    if (!fn && count == 1)
        fn = nl_quantity_bool_function(c, helper->function);
    const bool boolean =
        fn && count == 1 &&
        fn->parameters[0] == nl_semantic_core_type(c, NL_TYPE_BOOL);
    if (!fn || !call || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || call->argument_count != count ||
        call->function != helper->function ||
        ((!count || helper->second_formal || boolean) &&
         f->context != helper->body->context))
        return false;
    NLCheckedNodeId argument = call->first_argument;
    for (size_t i = 0; i < count; ++i) {
        const NLCheckedNodeView *arg = nl_checked_node_view(f, argument);
        NLValueId actual;
        if (!arg || !formals[i] || formals[i] > c->binding_count ||
            c->bindings[formals[i] - 1].view.type != fn->parameters[i] ||
            !scalar_node_value(f, c, argument, fn->parameters[i], &actual) ||
            c->bindings[formals[i] - 1].view.value != actual ||
            (arg->symbol && arg->symbol >= helper->formal))
            return false;
        argument = arg->next_argument;
    }
    NLValueId result, body_result;
    const NLCheckedNodeId body_node =
        helper->joined_result ? helper->joined_result : helper->result;
    return !argument && node_value(f, c, id, &result) &&
           node_value(helper->body, c, body_node, &body_result) &&
           result == body_result;
}

/* Select a direct continuation of the call's actual result. The ordinary
 * caller walker still authenticates the item source, Copy and both arms. */
static bool guarded_addition_decision(const NLCheckedFragment *caller,
                                      const NLSemanticContext *c,
                                      NLCheckedNodeId call)
{
    NLValueId result;
    const NLCheckedNodeView *root = nl_checked_node_view(caller, caller->root);
    if (!root || root->kind != NL_CHECKED_BLOCK ||
        !node_value(caller, c, call, &result))
        return false;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *item = nl_checked_node_view(caller, id);
        if (!item || (item->next_item && item->next_item <= id))
            return false;
        if (item->kind == NL_CHECKED_BINDING && item->initializer == call) {
            const NLCheckedNodeView *next =
                nl_checked_node_view(caller, item->next_item);
            const NLCheckedNodeView *choice =
                next && next->kind == NL_CHECKED_BINDING
                    ? nl_checked_node_view(caller, next->initializer)
                    : NULL;
            const NLCheckedNodeView *test =
                choice && choice->kind == NL_CHECKED_IF &&
                        choice->normal_arms == 2
                    ? nl_checked_node_view(caller, choice->initializer)
                    : NULL;
            const NLCheckedNodeView *left =
                test && test->kind == NL_CHECKED_USIZE_LESS_THAN
                    ? nl_checked_node_view(caller, test->first_argument)
                    : NULL;
            const NLCheckedNodeView *right =
                left ? nl_checked_node_view(caller, left->next_argument) : NULL;
            return right && right->kind == NL_CHECKED_USIZE_LITERAL &&
                   left->kind == NL_CHECKED_IDENTIFIER &&
                   left->value_use == NL_VALUE_COPIED && item->symbol &&
                   left->symbol == item->symbol &&
                   item->symbol <= c->binding_count &&
                   c->bindings[item->symbol - 1].view.value == result;
        }
        id = item->next_item;
    }
    return false;
}

static const NLCheckedNodeView *
guarded_named_actual(const NLCheckedFragment *caller,
                     const NLSemanticContext *c, NLCheckedNodeId id)
{
    const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
    const NLCheckedNodeView *actual =
        call ? nl_checked_node_view(caller, call->first_argument) : NULL;
    NLValueId value;
    return call && call->kind == NL_CHECKED_REGISTERED_CALL &&
                   call->body_backed && call->argument_count == 1 &&
                   call->first_argument > id && actual &&
                   actual->kind == NL_CHECKED_IDENTIFIER &&
                   actual->value_use == NL_VALUE_COPIED &&
                   !actual->next_argument &&
                   node_value(caller, c, call->first_argument, &value) &&
                   !c->values[value - 1].scalar_known
               ? actual
               : NULL;
}

static bool literal_usize_arm(const NLSyntaxNode *syntax)
{
    const NLSyntaxView *block = nl_syntax_node_view(syntax);
    const NLSyntaxView *tail = block && block->kind == NL_SYNTAX_BLOCK
                                   ? nl_syntax_node_view(block->data.block.tail)
                                   : NULL;
    return tail && !block->data.block.items &&
           tail->kind == NL_SYNTAX_USIZE_LITERAL;
}

/* Stage the existing inner result seal while this selector is still being
 * checked. Only the retained single-let/IF source may request it. A complete
 * selector must also have the checked binding and actual unit-main entry. */
static bool guarded_product_selector(const NLCheckedFragment *f,
                                     const NLSemanticContext *c,
                                     NLCheckedNodeId id,
                                     const NLCheckedNodeView *actual)
{
    const NLFunctionBody *owner = f->body_owner;
    if (!owner || owner->count != 1 || f->source != owner->source)
        return false;
    size_t function = 0;
    for (size_t i = 0; i < c->function_count; ++i)
        if (c->functions[i].body == owner) {
            function = i + 1;
            break;
        }
    if (!nl_quantity_usize_function(c, function, 1))
        return false;

    const NLSyntaxView *source =
        nl_syntax_node_view(nl_syntax_tree_root(owner->syntax));
    if (!source || source->kind != NL_SYNTAX_BLOCK)
        return false;
    const NLSyntaxNode *item = source->data.block.items;
    const NLSyntaxView *binding = nl_syntax_node_view(item);
    const NLSyntaxView *tail = nl_syntax_node_view(source->data.block.tail);
    if (!binding || binding->kind != NL_SYNTAX_BINDING ||
        nl_syntax_next_argument(item) || !tail || tail->kind != NL_SYNTAX_IF ||
        !literal_usize_arm(tail->data.conditional.then_block) ||
        !literal_usize_arm(tail->data.conditional.else_block))
        return false;
    const NLSyntaxView *call_source =
        nl_syntax_node_view(binding->data.binding.initializer);
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    if (!call_source || call_source->kind != NL_SYNTAX_EXPR_CALL ||
        !span_equal(call->span, call_source->span) ||
        !span_equal(call->name, call_source->data.call.callee) ||
        nl_syntax_next_argument(call_source->data.call.arguments) ||
        !token(f->source, actual->name, owner->parameter_names[0]) ||
        !name_copy(f, c, call->first_argument, call_source->data.call.arguments,
                   actual->symbol))
        return false;
    const NLSyntaxView *condition =
        nl_syntax_node_view(tail->data.conditional.condition);
    if (!condition || condition->kind != NL_SYNTAX_USIZE_LESS_THAN)
        return false;
    const NLSyntaxView *left =
        nl_syntax_node_view(condition->data.equality.left);
    const NLSyntaxView *right =
        nl_syntax_node_view(condition->data.equality.right);
    NLSourceView name, comparison;
    if (!left || left->kind != NL_SYNTAX_EXPR_NAME || !right ||
        right->kind != NL_SYNTAX_USIZE_LITERAL ||
        !nl_source_view(f->source, binding->data.binding.name, &name) ||
        !nl_source_view(f->source, left->data.name, &comparison) ||
        name.length != comparison.length ||
        memcmp(name.bytes, comparison.bytes, name.length))
        return false;
    if (!f->root)
        /* The custody lookup already required a valid seal for a committed
         * inner call. Here only the pending, unsealed call can proceed. */
        return !nl_h_custody_result_valid(f, id);

    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    const NLCheckedNodeView *local =
        root ? nl_checked_node_view(f, root->first_item) : NULL;
    const NLCheckedNodeView *choice =
        root ? nl_checked_node_view(f, root->tail) : NULL;
    const NLCheckedNodeView *test =
        choice ? nl_checked_node_view(f, choice->initializer) : NULL;
    if (!root || root->kind != NL_CHECKED_BLOCK || root->item_count != 1 ||
        !binding_source(f, c, local, binding) || local->initializer != id ||
        !choice || choice->kind != NL_CHECKED_IF || choice->normal_arms != 2 ||
        !test || test->kind != NL_CHECKED_USIZE_LESS_THAN ||
        !name_copy(f, c, test->first_argument, condition->data.equality.left,
                   local->symbol))
        return false;
    NLCheckedNodeId invocation = 0;
    const NLCheckedFragment *entry =
        nl_h_custody_body_caller(f, c, &invocation);
    const NLCheckedNodeView *input = guarded_named_actual(entry, c, invocation);
    const NLCheckedNodeView *call_selector =
        nl_checked_node_view(entry, invocation);
    return input && entry->context == f->context &&
           quantity_main_body(entry, c) &&
           call_selector->function == function &&
           c->bindings[actual->symbol - 1].view.value ==
               input->results[0].value;
}

/* The unit composition planner borrows the authenticated guarded result.
 * The legacy scalar guarded-addition caller keeps its own admission path. */
static bool guarded_unit_plan(const NLCheckedFragment *f,
                              const NLSemanticContext *c,
                              const NLFunctionEntry *fn, size_t function,
                              NLDivisionUsizePlan *out)
{
    if (fn != nl_quantity_usize_function(c, function, 1) || f->body_count ||
        f->arm_count != 2)
        return false;
    NLCheckedNodeId id = 0;
    const NLCheckedFragment *caller = nl_h_custody_body_caller(f, c, &id);
    const NLCheckedNodeView *actual = guarded_named_actual(caller, c, id);
    if (!actual || caller->context != f->context)
        return false;
    const bool unit_main = quantity_main_body(caller, c);
    if (!unit_main && !guarded_product_selector(caller, c, id, actual))
        return false;

    NLGuardedUsizePlan guard = {.body = f, .function = function};
    if (!helper_body(f, c, fn, NL_USIZE_MULTIPLY, &guard) &&
        (!unit_main || !guarded_addition_decision(caller, c, id) ||
         !helper_body(f, c, fn, NL_USIZE_ADD, &guard)))
        return false;
    const NLDivisionUsizePlan plan = {.body = f,
                                      .function = function,
                                      .formal = guard.formal,
                                      .result = guard.conditional};
    if (!division_call_values(caller, c, id, &plan))
        return false;
    *out = plan;
    return true;
}

static bool quantity_helper_at(const NLCheckedFragment *,
                               const NLSemanticContext *, size_t function,
                               NLUsizeOperation, size_t depth,
                               size_t *remaining, NLDivisionUsizePlan *);

static bool quantity_invocation_at(const NLCheckedFragment *f,
                                   const NLSemanticContext *c,
                                   NLCheckedNodeId id,
                                   const NLSyntaxNode *syntax,
                                   NLUsizeOperation operation, size_t depth,
                                   size_t *remaining)
{
    const NLSyntaxView *src = nl_syntax_node_view(syntax);
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    NLDivisionUsizePlan helper;
    return n && n->kind == NL_CHECKED_REGISTERED_CALL && src &&
           src->kind == NL_SYNTAX_EXPR_CALL &&
           (n->argument_count == 0) == (src->data.call.arguments == NULL) &&
           span_equal(n->span, src->span) &&
           span_equal(n->name, src->data.call.callee) && n->function &&
           n->function <= c->function_count &&
           token(f->source, n->name, c->functions[n->function - 1].name) &&
           nl_h_custody_result_valid(f, id) &&
           quantity_helper_at(nl_checked_call_body(f, id), c, n->function,
                              operation, depth, remaining, &helper) &&
           division_call_values(f, c, id, &helper);
}

static bool division_call_at(const NLCheckedFragment *f,
                             const NLSemanticContext *c, NLCheckedNodeId id,
                             const NLSyntaxNode *syntax,
                             NLCheckedNodeId first_call,
                             NLUsizeOperation operation, size_t *remaining)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const NLCheckedNodeView *first = nl_checked_node_view(f, first_call);
    const NLSyntaxView *src = nl_syntax_node_view(syntax);
    if (!n || n->argument_count != 1 || !src ||
        src->kind != NL_SYNTAX_EXPR_CALL ||
        nl_syntax_next_argument(src->data.call.arguments) ||
        (first && operation != NL_USIZE_ANY_FIXED_DIVISOR &&
         first->function != n->function) ||
        !quantity_invocation_at(f, c, id, syntax, operation, 1, remaining))
        return false;
    const NLCheckedNodeView *arg = nl_checked_node_view(f, n->first_argument);
    const NLSyntaxNode *actual =
        nl_syntax_node_view(syntax)->data.call.arguments;
    size_t literal_value;
    return arg->kind == NL_CHECKED_IDENTIFIER
               ? division_copy_at(f, c, n->first_argument, actual, 0)
               : literal(f, c, arg, actual, &literal_value);
}

static bool division_caller_arithmetic(const NLCheckedFragment *f,
                                       const NLSemanticContext *c,
                                       NLCheckedNodeId id,
                                       const NLSyntaxNode *syntax,
                                       NLUsizeOperation operation)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!p || p->syntax != syntax ||
        !operation_valid_at(f, id, operation, NULL, c, NULL))
        return false;
    if (fixed_divisor(operation))
        return division_copy_at(f, c, p->issued.operands[0],
                                nl_syntax_node_view(syntax)->data.equality.left,
                                0);
    for (size_t i = 0; i < 2; ++i)
        if (p->issued.symbols[i] &&
            !division_local_before(f, p->issued.operands[i],
                                   p->issued.symbols[i]))
            return false;
    return true;
}

typedef enum {
    QUANTITY_UNSEEN,
    QUANTITY_INDEPENDENT,
    QUANTITY_FROM_FORMAL,
    QUANTITY_FROM_RESULT
} QuantityOrigin;

/* A source-order working set for this invocation. The checked packages and
 * their existing arithmetic receipts and custody remain the evidence. */
typedef struct {
    const NLCheckedFragment *body, *if_parent;
    const NLSemanticContext *context;
    const NLFunctionEntry *function;
    size_t depth, calls, arms, returns, *remaining;
    NLSymbolId formal, local_numerator;
    NLValueId local_numerator_value;
    NLPlaceId local_numerator_place;
    unsigned char origins[NL_SEMANTIC_MAX_ENTRIES];
} QuantityBody;

typedef struct {
    const NLSyntaxNode *syntax;
    NLCheckedNodeId id;
    bool returned, formal_copies_only;
} QuantityResult;

static bool division_unit_expression(const NLCheckedFragment *, NLCheckedNodeId,
                                     const NLSyntaxNode *, NLTypeId);
static bool division_unit_completion(const NLCheckedFragment *, NLCheckedNodeId,
                                     const NLSyntaxNode *, NLTypeId);
static bool division_unit_return(const NLCheckedFragment *,
                                 const NLCheckedNodeView *,
                                 const NLCheckedNodeView *,
                                 const NLSyntaxView *, NLTypeId);
static QuantityOrigin quantity_if_at(QuantityBody *, NLCheckedNodeId,
                                     const NLSyntaxView *);

static QuantityOrigin quantity_atom_at(QuantityBody *b, NLCheckedNodeId id,
                                       const NLSyntaxNode *syntax)
{
    const NLCheckedFragment *f = b->body;
    const NLCheckedNodeView *atom = nl_checked_node_view(f, id);
    if (!atom)
        return QUANTITY_UNSEEN;
    if (atom->kind == NL_CHECKED_USIZE_LITERAL) {
        size_t number;
        return literal(f, b->context, atom, syntax, &number)
                   ? QUANTITY_INDEPENDENT
                   : QUANTITY_UNSEEN;
    }
    const NLSymbolId symbol = atom->symbol;
    if (!symbol || symbol > NL_SEMANTIC_MAX_ENTRIES ||
        !division_name_copy(f, b->context, id, syntax, symbol))
        return QUANTITY_UNSEEN;
    if (b->origins[symbol - 1])
        return (QuantityOrigin)b->origins[symbol - 1];
    if (b->function->count != 1 || (b->formal && b->formal != symbol) ||
        !token(f->source, atom->name, b->function->body->parameter_names[0]))
        return QUANTITY_UNSEEN;
    b->formal = symbol;
    return QUANTITY_FROM_FORMAL;
}

static QuantityOrigin quantity_condition_at(QuantityBody *b, NLCheckedNodeId id,
                                            const NLSyntaxNode *syntax)
{
    const NLCheckedFragment *f = b->body;
    const NLSemanticContext *c = b->context;
    const NLCheckedNodeView *node = nl_checked_node_view(f, id);
    const NLSyntaxView *src = nl_syntax_node_view(syntax);
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    NLValueId value;
    if (!src || !scalar_node_value(f, c, id, boolean, &value) ||
        !span_equal(node->span, src->span))
        return QUANTITY_UNSEEN;
    if (node->kind == NL_CHECKED_IDENTIFIER) {
        const NLSymbolId symbol = node->symbol;
        return symbol && symbol <= NL_SEMANTIC_MAX_ENTRIES &&
                       scalar_name_copy(f, c, id, syntax, symbol, boolean) &&
                       c->bindings[symbol - 1].view.value < value
                   ? (QuantityOrigin)b->origins[symbol - 1]
                   : QUANTITY_UNSEEN;
    }
    const bool less = src->kind == NL_SYNTAX_USIZE_LESS_THAN;
    if ((!less && src->kind != NL_SYNTAX_USIZE_EQUALITY) ||
        node->kind !=
            (less ? NL_CHECKED_USIZE_LESS_THAN : NL_CHECKED_USIZE_EQUALITY) ||
        node->argument_count != 2 ||
        !span_equal(node->name, src->data.equality.operator_span))
        return QUANTITY_UNSEEN;
    const NLCheckedNodeView *left =
        nl_checked_node_view(f, node->first_argument);
    const NLCheckedNodeView *right =
        left ? nl_checked_node_view(f, left->next_argument) : NULL;
    if (!right || right->next_argument)
        return QUANTITY_UNSEEN;
    const QuantityOrigin left_origin =
        quantity_atom_at(b, node->first_argument, src->data.equality.left);
    const QuantityOrigin right_origin =
        quantity_atom_at(b, left->next_argument, src->data.equality.right);
    if (!left_origin || !right_origin)
        return QUANTITY_UNSEEN;
    return left_origin > right_origin ? left_origin : right_origin;
}

static QuantityOrigin quantity_arithmetic_at(QuantityBody *b,
                                             NLCheckedNodeId id,
                                             const NLSyntaxNode *syntax)
{
    const NLUsizeAdditionProof *proof = find(b->body, id);
    if (!proof || proof->syntax != syntax ||
        (proof->operation == NL_USIZE_MULTIPLY &&
         (b->function->count ||
          b->function->result !=
              nl_semantic_core_type(b->context, NL_TYPE_USIZE))) ||
        (b->if_parent && !fixed_divisor(proof->operation)) ||
        !operation_valid_at(b->body, id, proof->operation, NULL, b->context,
                            b->if_parent))
        return QUANTITY_UNSEEN;
    const NLSyntaxView *src = nl_syntax_node_view(syntax);
    const QuantityOrigin left =
        quantity_atom_at(b, proof->issued.operands[0], src->data.equality.left);
    if (!left)
        return QUANTITY_UNSEEN;
    if (fixed_divisor(proof->operation)) {
        if (!b->function->count)
            return QUANTITY_FROM_RESULT;
        if (!b->if_parent || !b->local_numerator)
            return left >= QUANTITY_FROM_FORMAL ? QUANTITY_FROM_RESULT
                                                : QUANTITY_UNSEEN;
        /* The checked binding in the parent supplied these exact slot and
         * place IDs. The operation and atom verified this ARM's Copy. */
        const NLCheckedNodeView *numerator =
            nl_checked_node_view(b->body, proof->issued.operands[0]);
        if (numerator->kind != NL_CHECKED_IDENTIFIER ||
            numerator->symbol != b->local_numerator ||
            !b->local_numerator_value || !b->local_numerator_place)
            return QUANTITY_UNSEEN;
        const NLSemanticBindingView binding =
            b->context->bindings[b->local_numerator - 1].view;
        return binding.value == b->local_numerator_value &&
                       binding.place == b->local_numerator_place
                   ? QUANTITY_FROM_RESULT
                   : QUANTITY_UNSEEN;
    }
    const QuantityOrigin right = quantity_atom_at(b, proof->issued.operands[1],
                                                  src->data.equality.right);
    if (!right)
        return QUANTITY_UNSEEN;
    return left >= QUANTITY_FROM_FORMAL || right >= QUANTITY_FROM_FORMAL
               ? QUANTITY_FROM_RESULT
               : QUANTITY_INDEPENDENT;
}

static QuantityOrigin quantity_call_chain_at(QuantityBody *b,
                                             NLCheckedNodeId id,
                                             const NLSyntaxNode *syntax,
                                             size_t nesting)
{
    const NLCheckedFragment *f = b->body;
    const NLSemanticContext *c = b->context;
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLSyntaxView *src = nl_syntax_node_view(syntax);
    if (!call || call->kind != NL_CHECKED_REGISTERED_CALL || !src ||
        src->kind != NL_SYNTAX_EXPR_CALL ||
        call->argument_count > NL_QUANTITY_MAX_ARGUMENTS || !f->body_calls ||
        nesting >= f->body_count ||
        b->depth + nesting >= NL_SEMANTIC_MAX_DEPTH || !*b->remaining)
        return QUANTITY_UNSEEN;
    if (!call->argument_count &&
        (b->depth || b->if_parent || nesting ||
         strcmp(b->function->name, "main") ||
         b->function->result != nl_semantic_unit_type(c)))
        return QUANTITY_UNSEEN;
    if ((b->function->count || b->if_parent || nesting) &&
        call->argument_count == 1 &&
        nl_quantity_bool_function(c, call->function))
        return QUANTITY_UNSEEN;
    const NLSyntaxNode *actual_source = src->data.call.arguments;
    NLCheckedNodeId argument = call->first_argument, previous = id;
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    for (size_t i = 0; i < call->argument_count; ++i) {
        const NLCheckedNodeView *arg = nl_checked_node_view(f, argument);
        if (!arg || !actual_source || argument <= previous)
            return QUANTITY_UNSEEN;
        QuantityOrigin actual;
        if (arg->parameter_type == boolean) {
            actual = quantity_condition_at(b, argument, actual_source);
        } else if (arg->kind == NL_CHECKED_IDENTIFIER ||
                   arg->kind == NL_CHECKED_USIZE_LITERAL) {
            actual = quantity_atom_at(b, argument, actual_source);
        } else {
            if (arg->parameter_type != nl_semantic_core_type(c, NL_TYPE_USIZE))
                return QUANTITY_UNSEEN;
            actual = arg->kind == NL_CHECKED_REGISTERED_CALL
                         ? quantity_call_chain_at(b, argument, actual_source,
                                                  nesting + 1)
                         : quantity_arithmetic_at(b, argument, actual_source);
        }
        if (!actual || (b->function->count && actual < QUANTITY_FROM_FORMAL))
            return QUANTITY_UNSEEN;
        previous = argument;
        argument = arg->next_argument;
        actual_source = nl_syntax_next_argument(actual_source);
    }
    /* Node allocation reserves the outer call first. Owned bodies complete
     * in evaluation order, and each inner result is used by the next call. */
    if (argument || actual_source || b->calls >= f->body_count ||
        f->body_calls[b->calls] != id ||
        !quantity_invocation_at(f, c, id, syntax, NL_USIZE_ANY_FIXED_DIVISOR,
                                b->depth + nesting + 1, b->remaining))
        return QUANTITY_UNSEEN;
    ++b->calls;
    return QUANTITY_FROM_RESULT;
}

static QuantityOrigin quantity_expression_at(QuantityBody *b,
                                             NLCheckedNodeId id,
                                             const NLSyntaxNode *syntax)
{
    const NLCheckedFragment *f = b->body;
    const NLCheckedNodeView *node = nl_checked_node_view(f, id);
    const NLSyntaxView *src = nl_syntax_node_view(syntax);
    if (!node || !src)
        return QUANTITY_UNSEEN;
    if (node->kind == NL_CHECKED_IDENTIFIER ||
        node->kind == NL_CHECKED_USIZE_LITERAL)
        return quantity_atom_at(b, id, syntax);
    if (node->kind == NL_CHECKED_IF)
        return quantity_if_at(b, id, src);
    if (node->kind == NL_CHECKED_REGISTERED_CALL)
        return quantity_call_chain_at(b, id, syntax, 0);
    return quantity_arithmetic_at(b, id, syntax);
}

static bool quantity_same_state(const NLSemanticContext *a,
                                const NLSemanticContext *b)
{
    if (!a || !b || a->domain_count || b->domain_count || a->region_count ||
        b->region_count || a->type_count != b->type_count ||
        a->place_count != b->place_count ||
        a->binding_count != b->binding_count ||
        a->value_count != b->value_count || a->scope_count != b->scope_count ||
        a->function_count != b->function_count || !nl_copy_same_types(a, b) ||
        nl_sem_validate(a) != NL_CHECK_OK || nl_sem_validate(b) != NL_CHECK_OK)
        return false;
    if ((a->place_count &&
         memcmp(a->places, b->places, a->place_count * sizeof(*a->places))) ||
        (a->value_count &&
         memcmp(a->values, b->values, a->value_count * sizeof(*a->values))) ||
        (a->scope_count &&
         memcmp(a->scopes, b->scopes, a->scope_count * sizeof(*a->scopes))))
        return false;
    for (size_t i = 0; i < a->binding_count; ++i)
        if (!same_binding(a->bindings[i].view, b->bindings[i].view))
            return false;
    return true;
}

static bool quantity_same_scalar(const NLSemanticContext *a, NLValueId x,
                                 const NLSemanticContext *b, NLValueId y)
{
    const NLTypeId a_type = nl_semantic_core_type(a, NL_TYPE_USIZE);
    const NLTypeId b_type = nl_semantic_core_type(b, NL_TYPE_USIZE);
    NLSemanticValueView first, second;
    return quantity(a, x, a_type) && quantity(b, y, b_type) &&
           nl_semantic_value_view(a, x, &first) &&
           nl_semantic_value_view(b, y, &second) &&
           first.scalar_known == second.scalar_known &&
           first.scalar_value == second.scalar_value;
}

static bool quantity_return_arm_package(const NLCheckedFragment *arm,
                                        const NLSemanticContext *c,
                                        const NLCheckedNodeView *root,
                                        const NLCheckedNodeView *ret,
                                        const NLSyntaxView *source,
                                        const NLCheckedFragment *parent,
                                        const NLFunctionEntry *expected)
{
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    const NLControlExitView *edge = nl_control_exit_view(arm->exits, 0);
    const NLControlExitView *outer =
        parent ? nl_control_exit_view(parent->exits, 0) : NULL;
    const NLSemanticContext *edge_world =
        edge ? nl_control_state_context((NLControlState *)edge->state) : NULL;
    const NLSemanticContext *outer_world =
        outer ? nl_control_state_context((NLControlState *)outer->state) : NULL;
    NLValueId value;
    size_t function = 0;
    const NLCheckedFragment *owner =
        nl_h_custody_if_body(arm, parent, &function);
    const NLFunctionEntry *local =
        owner ? nl_quantity_usize_function(c, function, 2) : NULL;
    /* Only this ordered signature admits the one-normal Return path. */
    return parent && parent->context && parent->function_target &&
           owner == parent && local && expected &&
           local->parameters[0] == type && local->parameters[1] == boolean &&
           local->body == parent->body_owner && local->body == expected->body &&
           local->count == expected->count && parent->source == arm->source &&
           local->body->source == arm->source && arm->if_parent == parent &&
           root->terminates && !root->tail && !root->type &&
           !root->result_count && ret && ret->kind == NL_CHECKED_RETURN &&
           ret->terminates && !ret->next_item && !ret->result_count &&
           span_equal(ret->span, source->span) &&
           nl_control_quantity_return_receipt_valid(parent, c, arm, root,
                                                    ret) &&
           nl_control_exits_count(arm->exits) == 1 &&
           nl_control_exits_count(parent->exits) == 1 && edge && outer &&
           edge->kind == NL_EXIT_RETURN && outer->kind == NL_EXIT_RETURN &&
           nl_control_target_same(edge->target, parent->function_target) &&
           nl_control_target_same(outer->target, parent->function_target) &&
           edge->count == 1 && outer->count == 1 && edge_world && outer_world &&
           edge->values[0].type ==
               nl_semantic_core_type(edge_world, NL_TYPE_USIZE) &&
           outer->values[0].type ==
               nl_semantic_core_type(outer_world, NL_TYPE_USIZE) &&
           node_value(arm, c, ret->initializer, &value) &&
           ret->returned.type == type && ret->returned.value == value &&
           root->returned.type == type && root->returned.value == value &&
           quantity_same_state(c, edge_world) &&
           quantity_same_state(edge_world, outer_world) &&
           quantity_same_scalar(c, value, edge_world, edge->values[0].value) &&
           quantity_same_scalar(edge_world, edge->values[0].value, outer_world,
                                outer->values[0].value);
}

static bool quantity_items_at(QuantityBody *b, const NLCheckedNodeView *root,
                              const NLSyntaxView *source,
                              QuantityResult *result)
{
    const NLCheckedFragment *f = b->body;
    const NLSemanticContext *c = b->context;
    if (!source || source->kind != NL_SYNTAX_BLOCK || !root ||
        root->kind != NL_CHECKED_BLOCK || !span_equal(root->span, source->span))
        return false;
    *result = (QuantityResult){.syntax = source->data.block.tail,
                               .id = root->tail,
                               .formal_copies_only = true};
    NLCheckedNodeId id = root->first_item;
    size_t count = 0;
    for (const NLSyntaxNode *item = source->data.block.items; item;
         item = nl_syntax_next_argument(item)) {
        const NLSyntaxView *src = nl_syntax_node_view(item);
        const NLCheckedNodeView *node = nl_checked_node_view(f, id);
        if (++count > NL_PARSER_MAX_NODES || !src || !node ||
            (node->next_item && node->next_item <= id))
            return false;
        if (src->kind == NL_SYNTAX_RETURN) {
            if (nl_syntax_next_argument(item) || result->syntax ||
                !(b->if_parent
                      ? quantity_return_arm_package(f, c, root, node, src,
                                                    b->if_parent, b->function)
                  : b->function->count
                      ? division_return_package(f, c, root, node, src)
                      : division_unit_return(f, root, node, src,
                                             nl_semantic_unit_type(c))))
                return false;
            result->syntax = src->data.statement.expression;
            result->id = node->initializer;
            result->returned = true;
            id = 0;
            break;
        }
        if (src->kind == NL_SYNTAX_STATEMENT) {
            const NLCheckedNodeView *use =
                nl_checked_node_view(f, node->initializer);
            const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
            const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE);
            if (!use || (use->type != boolean && use->type != usize))
                return false;
            const bool call = use->kind == NL_CHECKED_REGISTERED_CALL;
            if (call &&
                (b->function->count || b->if_parent || use->type != usize ||
                 (use->argument_count &&
                  !nl_quantity_bool_function(c, use->function))))
                return false;
            const NLSyntaxNode *expression = discarded_quantity_expression(
                f, c, id, node, src, use->type,
                call ? NL_SYNTAX_EXPR_CALL : NL_SYNTAX_EXPR_NAME);
            if (!expression)
                return false;
            const QuantityOrigin origin =
                call ? quantity_call_chain_at(b, node->initializer, expression,
                                              0)
                : use->type == boolean
                    ? quantity_condition_at(b, node->initializer, expression)
                    : quantity_atom_at(b, node->initializer, expression);
            if (!origin)
                return false;
            id = node->next_item;
            continue;
        }
        if (!binding_source(f, c, node, src) ||
            node->symbol > NL_SEMANTIC_MAX_ENTRIES ||
            node->symbol == b->formal || b->origins[node->symbol - 1])
            return false;
        const NLSyntaxNode *init_source = src->data.binding.initializer;
        const NLCheckedNodeView *init =
            nl_checked_node_view(f, node->initializer);
        const QuantityOrigin origin =
            init->type == nl_semantic_core_type(c, NL_TYPE_BOOL)
                ? quantity_condition_at(b, node->initializer, init_source)
                : quantity_expression_at(b, node->initializer, init_source);
        if (!origin || node->symbol == b->formal)
            return false;
        if (node->symbol == b->local_numerator) {
            const NLSemanticBindingView binding =
                c->bindings[node->symbol - 1].view;
            b->local_numerator_value = binding.value;
            b->local_numerator_place = binding.place;
        }
        b->origins[node->symbol - 1] = (unsigned char)origin;
        if (init->kind != NL_CHECKED_IDENTIFIER ||
            origin != QUANTITY_FROM_FORMAL)
            result->formal_copies_only = false;
        id = node->next_item;
    }
    return id == 0 && count == root->item_count;
}

static bool quantity_tail_at(const QuantityBody *b, NLCheckedNodeId block,
                             NLCheckedNodeId tail, NLValueId *value)
{
    NLValueId tail_value;
    if (nl_control_exits_count(b->body->exits) != b->returns ||
        !node_value(b->body, b->context, block, value) ||
        !node_value(b->body, b->context, tail, &tail_value))
        return false;
    if (!b->returns)
        return *value == tail_value;
    NLSemanticValueView joined, normal;
    const NLTypeId type = nl_semantic_core_type(b->context, NL_TYPE_USIZE);
    return b->returns == 1 && tail_value != SIZE_MAX &&
           *value == tail_value + 1 && quantity(b->context, *value, type) &&
           quantity(b->context, tail_value, type) &&
           nl_semantic_value_view(b->context, *value, &joined) &&
           nl_semantic_value_view(b->context, tail_value, &normal) &&
           (joined.carrier == NL_CARRIER_LOOSE ||
            joined.carrier == NL_CARRIER_ENDED) &&
           !joined.scalar_known && !joined.scalar_value &&
           normal.carrier == NL_CARRIER_ENDED;
}

static QuantityOrigin quantity_if_at(QuantityBody *b, NLCheckedNodeId id,
                                     const NLSyntaxView *src)
{
    const NLCheckedFragment *f = b->body;
    const NLCheckedNodeView *node = nl_checked_node_view(f, id);
    NLValueId joined;
    if (b->if_parent || src->kind != NL_SYNTAX_IF ||
        !span_equal(node->span, src->span) ||
        (node->normal_arms != 1 && node->normal_arms != 2) ||
        node->item_count != 2 ||
        (node->normal_arms == 1) != (node->tail != 0) ||
        (node->normal_arms == 1 && b->returns) ||
        !node_value(f, b->context, id, &joined) ||
        (node->normal_arms == 2 &&
         b->context->values[joined - 1].scalar_known) ||
        *b->remaining < 2)
        return QUANTITY_UNSEEN;
    QuantityOrigin origin = quantity_condition_at(
        b, node->initializer, src->data.conditional.condition);
    if (!origin)
        return QUANTITY_UNSEEN;
    const NLCheckedNodeView *condition =
        nl_checked_node_view(f, node->initializer);
    const NLSyntaxNode *blocks[] = {src->data.conditional.then_block,
                                    src->data.conditional.else_block};
    *b->remaining -= 2;
    size_t returned = 0, normal = 0, normal_index = 0;
    for (size_t index = 0; index < 2; ++index) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, index);
        if (!arm || arm->if_parent != f || arm->if_parent_node != id ||
            arm->if_arm_index != index || arm->source != f->source ||
            arm->context == b->context || arm->arm_count ||
            arm->branch_value_prefix != condition->results[0].value ||
            arm->body_count > NL_CHECKED_MAX_BODY_CALLS)
            return QUANTITY_UNSEEN;
        const NLCheckedNodeView *root = nl_checked_node_view(arm, arm->root);
        const NLSyntaxView *block = nl_syntax_node_view(blocks[index]);
        NLValueId arm_value = 0, tail;
        if (!root || root->kind != NL_CHECKED_IF_ARM || !block ||
            !span_equal(root->span, block->span))
            return QUANTITY_UNSEEN;
        /* Reuse only the prefix locals in the fork. Each arm validates its
         * own bindings and calls, and exports its runtime value alone. */
        QuantityBody branch = *b;
        branch.body = arm;
        branch.context = arm->context;
        branch.calls = branch.arms = 0;
        branch.if_parent = f;
        QuantityResult result;
        if (!quantity_items_at(&branch,
                               nl_checked_node_view(arm, root->initializer),
                               block, &result))
            return QUANTITY_UNSEEN;
        if (result.returned) {
            if (!root->terminates || ++returned != 1)
                return QUANTITY_UNSEEN;
        } else {
            if (root->terminates ||
                !node_value(arm, arm->context, arm->root, &arm_value) ||
                !quantity_tail_at(&branch, root->initializer, result.id,
                                  &tail) ||
                tail != arm_value)
                return QUANTITY_UNSEEN;
            ++normal;
            normal_index = index;
        }
        const QuantityOrigin incoming =
            quantity_expression_at(&branch, result.id, result.syntax);
        if (!incoming || branch.calls != arm->body_count)
            return QUANTITY_UNSEEN;
        if (incoming > origin)
            origin = incoming;
        b->formal = branch.formal;
    }
    if ((node->normal_arms == 2 && (normal != 2 || returned)) ||
        (node->normal_arms == 1 && (normal != 1 || returned != 1)))
        return QUANTITY_UNSEEN;
    if (node->normal_arms == 1) {
        const NLSyntaxNode *block = blocks[normal_index];
        const NLCheckedNodeView *replay_root =
            nl_checked_node_view(f, node->tail);
        QuantityBody replay = *b;
        replay.calls = replay.arms = 0;
        replay.if_parent = NULL;
        QuantityResult result;
        NLValueId replay_value, result_value;
        if (!replay_root || replay_root->kind != NL_CHECKED_BLOCK ||
            !quantity_items_at(&replay, replay_root, nl_syntax_node_view(block),
                               &result) ||
            result.returned ||
            !node_value(f, b->context, node->tail, &replay_value) ||
            !node_value(f, b->context, result.id, &result_value) ||
            replay_value != result_value || replay_value != joined ||
            quantity_expression_at(&replay, result.id, result.syntax) ==
                QUANTITY_UNSEEN ||
            replay.calls || replay.arms)
            return QUANTITY_UNSEEN;
        ++b->returns;
    }
    b->arms += 2;
    return origin >= QUANTITY_FROM_FORMAL ? QUANTITY_FROM_RESULT
                                          : QUANTITY_INDEPENDENT;
}

typedef enum {
    QUANTITY_BOOL_LITERAL_TAILS,
    QUANTITY_BOOL_MATCHING_ARITHMETIC,
    QUANTITY_BOOL_JOINED_NUMERATOR_ARITHMETIC
} QuantityBoolArmProfile;

/* A private shape check. The ordinary IF planner still validates each
 * source operation, operands, branch world, condition, and the joined result.
 */
static bool quantity_bool_arms(const NLCheckedFragment *f, NLCheckedNodeId id,
                               NLSymbolId formal,
                               QuantityBoolArmProfile profile)
{
    const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
    if (!choice || choice->kind != NL_CHECKED_IF || choice->normal_arms != 2)
        return false;
    const NLCheckedNodeView *condition =
        nl_checked_node_view(f, choice->initializer);
    if (!condition || condition->kind != NL_CHECKED_IDENTIFIER ||
        condition->symbol != formal)
        return false;
    NLCheckedKind tail_kind = NL_CHECKED_USIZE_LITERAL;
    for (size_t i = 0; i != 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, i);
        const NLCheckedNodeView *arm_root =
            nl_checked_node_view(arm, nl_checked_root(arm));
        const NLCheckedNodeView *block =
            arm_root ? nl_checked_node_view(arm, arm_root->initializer) : NULL;
        const NLCheckedNodeView *tail =
            block ? nl_checked_node_view(arm, block->tail) : NULL;
        if (!block || block->item_count || !tail)
            return false;
        if (profile == QUANTITY_BOOL_LITERAL_TAILS) {
            if (tail->kind != NL_CHECKED_USIZE_LITERAL)
                return false;
        } else {
            if (tail->kind != NL_CHECKED_USIZE_DIVISION &&
                tail->kind != NL_CHECKED_USIZE_REMAINDER)
                return false;
            if (profile == QUANTITY_BOOL_MATCHING_ARITHMETIC && i != 0 &&
                tail->kind != tail_kind)
                return false;
            tail_kind = tail->kind;
        }
    }
    return true;
}

static bool quantity_bool_choice(const NLCheckedFragment *f,
                                 const NLCheckedNodeView *root,
                                 NLSymbolId formal, NLSymbolId *local_numerator)
{
    const bool joined_numerator = f->arm_count == 4;
    if (!root || root->kind != NL_CHECKED_BLOCK || root->terminates ||
        (!joined_numerator && f->arm_count != 2) || f->body_count ||
        root->item_count > 1 || nl_control_exits_count(f->exits))
        return false;
    NLCheckedNodeId id = root->tail;
    if (root->first_item) {
        const NLCheckedNodeView *item =
            nl_checked_node_view(f, root->first_item);
        const NLCheckedNodeView *tail = nl_checked_node_view(f, id);
        if (!item || item->kind != NL_CHECKED_BINDING || !tail)
            return false;
        const NLCheckedNodeView *init =
            nl_checked_node_view(f, item->initializer);
        if (joined_numerator) {
            if (!quantity_bool_arms(f, item->initializer, formal,
                                    QUANTITY_BOOL_LITERAL_TAILS))
                return false;
            *local_numerator = item->symbol;
        } else if (init && init->kind == NL_CHECKED_USIZE_LITERAL) {
            *local_numerator = item->symbol;
        } else if (tail->kind == NL_CHECKED_IDENTIFIER &&
                   tail->symbol == item->symbol) {
            id = item->initializer;
        } else {
            return false;
        }
    }
    if (!*local_numerator)
        return !joined_numerator &&
               quantity_bool_arms(f, id, formal, QUANTITY_BOOL_LITERAL_TAILS);
    return quantity_bool_arms(f, id, formal,
                              joined_numerator
                                  ? QUANTITY_BOOL_JOINED_NUMERATOR_ARITHMETIC
                                  : QUANTITY_BOOL_MATCHING_ARITHMETIC);
}

static bool quantity_fixed_divisor_syntax(NLSyntaxKind kind)
{
    return kind == NL_SYNTAX_USIZE_DIVISION ||
           kind == NL_SYNTAX_USIZE_REMAINDER;
}

/* A zero-formal helper has one product and optionally one fixed-divisor
 * operation. Other initializers are ordinary atoms. */
static bool quantity_product_shape(const NLCheckedFragment *f,
                                   const NLCheckedNodeView *root,
                                   const NLSyntaxView *source,
                                   bool *with_fixed_divisor)
{
    if (f->body_count || f->arm_count || !root || root->terminates || !source ||
        source->kind != NL_SYNTAX_BLOCK)
        return false;
    const NLSyntaxView *tail = nl_syntax_node_view(source->data.block.tail);
    if (!tail)
        return false;
    bool product = tail->kind == NL_SYNTAX_USIZE_MULTIPLICATION;
    bool divisor = quantity_fixed_divisor_syntax(tail->kind);
    if (!product && !divisor && tail->kind != NL_SYNTAX_EXPR_NAME)
        return false;
    size_t count = 0;
    for (const NLSyntaxNode *item = source->data.block.items; item;
         item = nl_syntax_next_argument(item)) {
        const NLSyntaxView *binding = nl_syntax_node_view(item);
        if (++count > root->item_count || !binding ||
            binding->kind != NL_SYNTAX_BINDING)
            return false;
        const NLSyntaxView *init =
            nl_syntax_node_view(binding->data.binding.initializer);
        if (!init)
            return false;
        if (init->kind == NL_SYNTAX_USIZE_MULTIPLICATION && !product)
            product = true;
        else if (quantity_fixed_divisor_syntax(init->kind) && !divisor)
            divisor = true;
        else if (init->kind != NL_SYNTAX_USIZE_LITERAL &&
                 init->kind != NL_SYNTAX_EXPR_NAME)
            return false;
    }
    *with_fixed_divisor = divisor;
    return product;
}

/* The item walker has matched each initializer to its actual binding and
 * validated its Copy or arithmetic receipt in this invocation. Follow those
 * bindings by identity, with checked node IDs decreasing at every step. */
static const NLCheckedNodeView *
quantity_local_origin(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    const NLCheckedNodeView *node = nl_checked_node_view(f, id);
    while (node && node->kind == NL_CHECKED_IDENTIFIER) {
        const NLCheckedNodeId binding =
            division_local_before(f, id, node->symbol);
        const NLCheckedNodeView *item = nl_checked_node_view(f, binding);
        if (!item || item->initializer >= binding)
            return NULL;
        id = item->initializer;
        node = nl_checked_node_view(f, id);
    }
    return node;
}

static bool quantity_product_tail(QuantityBody *b, const QuantityResult *result,
                                  bool with_fixed_divisor)
{
    const NLCheckedFragment *f = b->body;
    const QuantityOrigin expected =
        with_fixed_divisor ? QUANTITY_FROM_RESULT : QUANTITY_INDEPENDENT;
    NLValueId value;
    if (!quantity_tail_at(b, f->root, result->id, &value) ||
        quantity_expression_at(b, result->id, result->syntax) != expected)
        return false;

    const NLCheckedNodeView *origin = quantity_local_origin(f, result->id);
    if (with_fixed_divisor) {
        if (!origin || (origin->kind != NL_CHECKED_USIZE_DIVISION &&
                        origin->kind != NL_CHECKED_USIZE_REMAINDER))
            return false;
        origin = quantity_local_origin(f, origin->first_argument);
    }
    return origin && origin->kind == NL_CHECKED_USIZE_MULTIPLICATION;
}

static bool composed_quantity_body(const NLCheckedFragment *f,
                                   const NLSemanticContext *c,
                                   const NLFunctionEntry *fn, size_t function,
                                   size_t depth, size_t *remaining,
                                   NLDivisionUsizePlan *out)
{
    if (!f || !fn || f->body_owner != fn->body ||
        f->source != fn->body->source ||
        f->body_count > NL_CHECKED_MAX_BODY_CALLS)
        return false;
    if (guarded_unit_plan(f, c, fn, function, out))
        return true;
    const NLSyntaxView *source =
        nl_syntax_node_view(nl_syntax_tree_root(fn->body->syntax));
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    QuantityBody b = {.body = f,
                      .context = c,
                      .function = fn,
                      .depth = depth,
                      .remaining = remaining};
    NLSymbolId formals[NL_QUANTITY_MAX_ARGUMENTS] = {0};
    const bool boolean = fn == nl_quantity_bool_function(c, function);
    if (boolean || fn->count == NL_QUANTITY_MAX_ARGUMENTS) {
        if (!quantity_copy_formals(f, c, fn, function, formals) ||
            (boolean &&
             !quantity_bool_choice(f, root, formals[0], &b.local_numerator)))
            return false;
        b.formal = formals[0];
        for (size_t i = 0; i < fn->count; ++i)
            b.origins[formals[i] - 1] = QUANTITY_FROM_FORMAL;
    }
    const bool product =
        !fn->count && fn->result == nl_semantic_core_type(c, NL_TYPE_USIZE);
    bool with_fixed_divisor = false;
    QuantityResult result;
    if ((product &&
         !quantity_product_shape(f, root, source, &with_fixed_divisor)) ||
        !quantity_items_at(&b, root, source, &result))
        return false;
    if (product) {
        if (!quantity_product_tail(&b, &result, with_fixed_divisor))
            return false;
    } else if (fn->count) {
        NLValueId value;
        if (!result.returned &&
            !quantity_tail_at(&b, f->root, result.id, &value))
            return false;
        const QuantityOrigin origin =
            quantity_expression_at(&b, result.id, result.syntax);
        /* A Copy-only helper may return its own formal through checked
         * lets. Arithmetic and composed helpers keep their existing
         * computed-result requirement. */
        const bool formal_copy =
            result.formal_copies_only && origin == QUANTITY_FROM_FORMAL;
        if (!b.formal || (origin != QUANTITY_FROM_RESULT && !formal_copy))
            return false;
    } else {
        const NLTypeId unit = nl_semantic_unit_type(c);
        if (!result.returned &&
            (root->terminates || root->type != unit || root->result_count ||
             root->results[0].value || nl_control_exits_count(f->exits) ||
             !division_unit_completion(f, result.id, result.syntax, unit)))
            return false;
    }
    if (b.calls != f->body_count || b.arms != f->arm_count)
        return false;
    *out = (NLDivisionUsizePlan){.body = f,
                                 .function = function,
                                 .formal = b.formal,
                                 .second_formal = formals[1],
                                 .result = result.id,
                                 .joined_result = b.returns ? f->root : 0};
    return true;
}

static bool quantity_helper_at(const NLCheckedFragment *f,
                               const NLSemanticContext *c, size_t function,
                               NLUsizeOperation operation, size_t depth,
                               size_t *remaining, NLDivisionUsizePlan *out)
{
    if (!*remaining || depth >= NL_SEMANTIC_MAX_DEPTH || !c || !c->functions ||
        !function || function > c->function_count)
        return false;
    --*remaining;
    if (division_helper(f, c, function, operation, out))
        return true;
    if (operation != NL_USIZE_ANY_FIXED_DIVISOR)
        return false;
    const NLFunctionEntry *fn = nl_quantity_usize_function(
        c, function, c->functions[function - 1].count);
    if (!fn)
        fn = nl_quantity_bool_function(c, function);
    return composed_quantity_body(f, c, fn, function, depth, remaining, out);
}

static bool division_unit_expression(const NLCheckedFragment *f,
                                     NLCheckedNodeId id,
                                     const NLSyntaxNode *syntax, NLTypeId unit)
{
    const NLCheckedNodeView *node = nl_checked_node_view(f, id);
    const NLSyntaxView *src = nl_syntax_node_view(syntax);
    return node && node->kind == NL_CHECKED_UNIT && node->type == unit &&
           !node->result_count && !node->results[0].value &&
           !node->terminates && src && src->kind == NL_SYNTAX_EXPR_NAME &&
           token(f->source, src->data.name, "unit") &&
           span_equal(src->span, node->span);
}

/* Draft17.38 §19.1 gives a normally completed block without a tail
 * expression the unit result. Keep that absence exact on both the source
 * and checked sides. An explicit unit tail remains authenticated by its
 * existing source event. */
static bool division_unit_completion(const NLCheckedFragment *f,
                                     NLCheckedNodeId id,
                                     const NLSyntaxNode *syntax, NLTypeId unit)
{
    return (!id && !syntax) || division_unit_expression(f, id, syntax, unit);
}

static bool division_unit_return(const NLCheckedFragment *f,
                                 const NLCheckedNodeView *root,
                                 const NLCheckedNodeView *ret,
                                 const NLSyntaxView *src, NLTypeId unit)
{
    const NLControlExitView *exit = nl_control_exit_view(f->exits, 0);
    return root->terminates && !root->type && !root->tail &&
           !root->result_count && root->returned.type == unit &&
           !root->returned.value && ret->kind == NL_CHECKED_RETURN &&
           ret->terminates && !ret->next_item && !ret->result_count &&
           ret->returned.type == unit && !ret->returned.value &&
           span_equal(ret->span, src->span) &&
           division_unit_expression(f, ret->initializer,
                                    src->data.statement.expression, unit) &&
           nl_control_exits_count(f->exits) == 1 && exit && exit->state &&
           nl_control_exits_only_return(f->exits, f->function_target) &&
           exit->count == 1 && exit->values[0].type == unit &&
           !exit->values[0].value;
}

static bool has_composed_actual(const NLCheckedFragment *f,
                                const NLSemanticContext *c)
{
    for (size_t i = 0; i < f->body_count; ++i) {
        const NLCheckedNodeView *call =
            nl_checked_node_view(f, f->body_calls[i]);
        if (call && ((!call->argument_count &&
                      nl_quantity_usize_function(c, call->function, 0)) ||
                     call->argument_count > 1 ||
                     (call->argument_count == 1 &&
                      nl_quantity_bool_function(c, call->function))))
            return true;
        const NLCheckedNodeView *arg =
            call ? nl_checked_node_view(f, call->first_argument) : NULL;
        if (arg && (arg->kind == NL_CHECKED_REGISTERED_CALL ||
                    find(f, call->first_argument)))
            return true;
    }
    return false;
}

static bool guarded_byte_count(QuantityBody *b, NLCheckedNodeId id,
                               const NLSyntaxNode *syntax, NLSymbolId key)
{
    const NLSyntaxView *choice = nl_syntax_node_view(syntax);
    if (!choice || choice->kind != NL_SYNTAX_IF ||
        !literal_usize_arm(choice->data.conditional.then_block) ||
        !literal_usize_arm(choice->data.conditional.else_block))
        return false;
    const NLSyntaxView *test =
        nl_syntax_node_view(choice->data.conditional.condition);
    const NLSyntaxView *bound =
        test && test->kind == NL_SYNTAX_USIZE_LESS_THAN
            ? nl_syntax_node_view(test->data.equality.right)
            : NULL;
    const NLCheckedNodeView *n = nl_checked_node_view(b->body, id);
    const NLCheckedNodeView *condition =
        n ? nl_checked_node_view(b->body, n->initializer) : NULL;
    return bound && bound->kind == NL_SYNTAX_USIZE_LITERAL && condition &&
           n->normal_arms == 2 &&
           name_copy(b->body, b->context, condition->first_argument,
                     test->data.equality.left, key) &&
           quantity_expression_at(b, id, syntax) == QUANTITY_INDEPENDENT;
}

static bool guarded_byte_actual(const NLCheckedFragment *f,
                                const NLSemanticContext *c, NLCheckedNodeId id,
                                const NLSyntaxNode *syntax, NLSymbolId binding)
{
    const NLSyntaxView *src = nl_syntax_node_view(syntax);
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    return call && src && src->kind == NL_SYNTAX_EXPR_CALL &&
           span_equal(src->span, call->span) &&
           span_equal(src->data.call.callee, call->name) &&
           !nl_syntax_next_argument(src->data.call.arguments) &&
           guarded_named_actual(f, c, id) &&
           name_copy(f, c, call->first_argument, src->data.call.arguments,
                     binding);
}

static bool guarded_byte_product(QuantityBody *b, NLCheckedNodeId id,
                                 const NLSyntaxNode *syntax)
{
    /* This is the same child invocation walk used during whole-quantity
     * CHECK. The public consumer will later require the entry's world seal. */
    if (!quantity_invocation_at(b->body, b->context, id, syntax,
                                NL_USIZE_ANY_FIXED_DIVISOR, 1, b->remaining))
        return false;
    const NLCheckedFragment *product = nl_checked_call_body(b->body, id);
    const NLCheckedNodeView *root =
        product ? nl_checked_node_view(product, product->root) : NULL;
    if (!root)
        return false;
    const NLCheckedNodeView *yes =
        predicate_arm_value(nl_checked_if_arm(product, root->tail, 0));
    const NLCheckedNodeView *no =
        predicate_arm_value(nl_checked_if_arm(product, root->tail, 1));
    return yes && yes->kind == NL_CHECKED_USIZE_MULTIPLICATION && no &&
           no->kind == NL_CHECKED_USIZE_LITERAL && no->scalar_result.known &&
           no->scalar_result.value == 0;
}

static bool descriptor_field_source(const NLCheckedFragment *f,
                                    const NLSemanticContext *c,
                                    const NLSyntaxView *source,
                                    const NLCheckedField *field,
                                    const NLCheckedNodeView *original,
                                    NLTypeId nominal, NLTypeId u8)
{
    NLAggregateField declared;
    return source && source->kind == NL_SYNTAX_FIELD_DESIGNATOR &&
           field->base == original->symbol && !field->payload_occurrence &&
           !field->post_payload_occurrence &&
           nl_c17_resolved_field(f, c, field, nominal, u8) &&
           nl_semantic_aggregate_field_view(c, nominal, field->index,
                                            &declared) &&
           token(f->source, source->data.field_designator.base,
                 c->bindings[original->symbol - 1].name) &&
           token(f->source, source->data.field_designator.field, declared.name);
}

static bool descriptor_constructor(const NLCheckedFragment *f,
                                   const NLSemanticContext *c,
                                   const NLCheckedNodeView *node,
                                   const NLSyntaxView *source, NLTypeId u8,
                                   NLSemanticValueView *initial)
{
    if (node->kind != NL_CHECKED_AGGREGATE || node->terminates ||
        node->argument_count != 2 || !source ||
        source->kind != NL_SYNTAX_AGGREGATE ||
        source->data.aggregate.count != 2 ||
        !nl_c17_pair_type(c, node->type, u8) ||
        !span_equal(node->name, source->data.aggregate.type_name) ||
        !token(f->source, node->name, c->types[node->type - 1].name) ||
        node->results[0].type != node->type ||
        !nl_semantic_value_view(c, node->results[0].value, initial) ||
        initial->type != node->type || initial->field_count != 2)
        return false;

    bool seen[2] = {false, false};
    NLCheckedNodeId id = node->first_argument;
    const NLSyntaxNode *item = source->data.aggregate.fields;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *field = nl_checked_node_view(f, id);
        const NLSyntaxView *member = nl_syntax_node_view(item);
        const NLCheckedNodeView *value =
            field ? nl_checked_node_view(f, field->initializer) : NULL;
        NLAggregateField declared;
        if (!field || field->kind != NL_CHECKED_AGGREGATE_FIELD ||
            field->field_index >= 2 || seen[field->field_index] ||
            field->type != u8 || !member || member->kind != NL_SYNTAX_FIELD ||
            !span_equal(field->span, member->span) ||
            !span_equal(field->name, member->data.binding.name) ||
            !nl_semantic_aggregate_field_view(c, node->type, field->field_index,
                                              &declared) ||
            !token(f->source, field->name, declared.name) || !value ||
            value->type != u8 || value->results[0].type != u8 ||
            !span_equal(
                value->span,
                nl_syntax_node_view(member->data.binding.initializer)->span) ||
            !copy_if_literal_at(f, c, value) ||
            initial->fields[field->field_index] != value->results[0].value)
            return false;
        seen[field->field_index] = true;
        id = field->next_argument;
        item = nl_syntax_next_argument(item);
    }
    return !id && !item;
}

/* Classify the actual checked writer before the entry world is sealed. The
 * published entry validator also replays the completed caller/Change proof. */
static bool descriptor_writer(const NLCheckedFragment *f, NLCheckedNodeId id,
                              const NLCheckedNodeView *loan, NLTypeId u8,
                              NLCheckedField *change)
{
    NLCopyBodyParameters formals;
    NLCopyBodyView copy;
    NLCopyBodyEventView event;
    if (!nl_checked_copy_body_parameters(f, id, &formals) ||
        nl_checked_node_view(f, formals.loan) != loan ||
        !nl_checked_copy_body_view(f, id, &copy) || copy.event_count != 1 ||
        !nl_checked_copy_body_event(f, id, 0, &event) ||
        event.kind != NL_COPY_BODY_CHANGE)
        return false;
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedNodeView *root = nl_checked_node_view(copy.body, copy.root);
    const NLCheckedNodeView *tail =
        root ? nl_checked_node_view(copy.body, root->tail) : NULL;
    const NLCheckedNodeView *store =
        nl_checked_node_view(copy.body, event.node);
    const NLCheckedNodeView *ref =
        store ? nl_checked_node_view(copy.body, store->first_argument) : NULL;
    const NLCheckedNodeView *incoming =
        ref ? nl_checked_node_view(copy.body, ref->next_argument) : NULL;
    if (!call || call->first_argument != formals.arguments[0] || !root ||
        root->kind != NL_CHECKED_BLOCK || root->terminates ||
        copy.body->arm_count || copy.body->body_count || !tail ||
        tail->result_count != 1 || tail->type != u8 ||
        tail->results[0].value != copy.result.value || !store ||
        store->kind != NL_CHECKED_REPLACE || !ref ||
        ref->kind != NL_CHECKED_IDENTIFIER ||
        ref->value_use != NL_VALUE_COPIED ||
        ref->symbol != formals.parameters[0] || !incoming ||
        incoming->kind != NL_CHECKED_IDENTIFIER ||
        incoming->value_use != NL_VALUE_COPIED ||
        incoming->symbol != formals.parameters[1] || incoming->next_argument)
        return false;
    if (root->tail == event.node) {
        /* The body directly forwards the recorded result of its one Change. */
        if (root->item_count || root->first_item || root->result_count != 1 ||
            root->results[0].type != u8 || copy.result.type != u8 ||
            root->results[0].value != event.result ||
            copy.result.value != event.result)
            return false;
    } else {
        const NLCheckedNodeView *local =
            nl_checked_node_view(copy.body, root->first_item);
        if (root->item_count != 1 || !local ||
            local->kind != NL_CHECKED_BINDING || local->next_item ||
            local->initializer != event.node ||
            tail->kind != NL_CHECKED_IDENTIFIER ||
            tail->value_use != NL_VALUE_COPIED || tail->symbol != local->symbol)
            return false;
    }
    *change = event.field;
    return true;
}

static bool
descriptor_replace(const NLCheckedFragment *f, const NLSemanticContext *c,
                   const NLCheckedNodeView *loan, const NLSyntaxView *source,
                   const NLCheckedNodeView *original, NLSymbolId selected,
                   NLTypeId nominal, NLTypeId u8, NLCheckedField *field,
                   NLCheckedNodeId *writer, NLCopyPolicyWriter *policy)
{
    if (!loan)
        return false;
    const NLCheckedNodeView *body = nl_checked_node_view(f, loan->initializer);
    if (loan->kind != NL_CHECKED_LOAN_HEADER || loan->type != u8 ||
        loan->result_count != 1 || loan->results[0].type != u8 ||
        !nl_c17_completed_loan(loan, body) || body->item_count ||
        body->first_item || loan->loan.access != NL_ACCESS_WRITE ||
        !loan->loan.prevent_lifetime_end || !source ||
        source->kind != NL_SYNTAX_LOCAL_WRITE_LOAN ||
        source->data.loan.from_ptr ||
        source->data.loan.access != NL_ACCESS_WRITE ||
        source->data.loan.is_exclusive || source->data.loan.stability ||
        !descriptor_field_source(f, c,
                                 nl_syntax_node_view(source->data.loan.source),
                                 &loan->field, original, nominal, u8))
        return false;

    const NLSyntaxView *src_body = nl_syntax_node_view(source->data.loan.body);
    const NLSyntaxView *src =
        src_body && src_body->kind == NL_SYNTAX_BLOCK &&
                !src_body->data.block.items
            ? nl_syntax_node_view(src_body->data.block.tail)
            : NULL;
    const NLCheckedNodeView *store = nl_checked_node_view(f, body->tail);
    NLValueId old_value;
    if (!src || src->kind != NL_SYNTAX_EXPR_CALL ||
        src->data.call.argument_count != 2 ||
        !span_equal(body->span, src_body->span) || !store ||
        store->argument_count != 2 || !span_equal(store->span, src->span) ||
        !scalar_node_value(f, c, body->tail, u8, &old_value) ||
        old_value != body->results[0].value ||
        old_value != loan->results[0].value)
        return false;

    const bool delegated = store->kind == NL_CHECKED_REGISTERED_CALL;
    if (delegated) {
        if (!store->function || store->function > c->function_count ||
            !token(f->source, src->data.call.callee,
                   c->functions[store->function - 1].name))
            return false;
        if (policy) {
            if (!nl_copy_policy_writer(f, body->tail, policy) ||
                nl_checked_node_view(f, policy->loan) != loan)
                return false;
            *field = policy->change;
        } else if (!descriptor_writer(f, body->tail, loan, u8, field)) {
            return false;
        }
    } else {
        if (policy || store->kind != NL_CHECKED_REPLACE ||
            !token(f->source, src->data.call.callee, "replace") ||
            !nl_c17_field_change(f, c, store, nominal, u8))
            return false;
        *field = store->field;
    }

    const NLCheckedField *before = &loan->field;
    const NLCheckedField *change = field;
    if (!change->present || !change->dependency_compatible ||
        change->access != NL_ACCESS_WRITE || change->nominal != nominal ||
        change->type != u8 || !change->parent_post_fact ||
        !change->child_post_fact ||
        change->parent_post_fact == change->parent_fact ||
        change->child_post_fact == change->child_fact ||
        before->access != NL_ACCESS_WRITE ||
        loan->loan.source != before->base || before->base != change->base ||
        before->index != change->index || before->parent != change->parent ||
        before->child != change->child ||
        before->parent_incarnation != change->parent_incarnation ||
        before->child_incarnation != change->child_incarnation ||
        before->parent_fact != change->parent_fact ||
        before->child_fact != change->child_fact ||
        before->old_value != change->old_value || change->payload_occurrence ||
        change->post_payload_occurrence || loan->loan.place != change->child ||
        loan->loan.incarnation != change->child_incarnation)
        return false;

    const NLCheckedNodeView *ref =
        nl_checked_node_view(f, store->first_argument);
    const NLSyntaxNode *first = src->data.call.arguments;
    const NLSyntaxView *ref_source = nl_syntax_node_view(first);
    const NLCheckedNodeView *arg =
        ref ? nl_checked_node_view(f, ref->next_argument) : NULL;
    NLSemanticValueView value;
    const NLTypeId scalar =
        policy ? nl_semantic_core_type(c, NL_TYPE_USIZE) : u8;
    if (!ref || ref->kind != NL_CHECKED_IDENTIFIER ||
        ref->value_use != NL_VALUE_COPIED || ref->result_count != 1 ||
        ref->symbol != loan->loan.ref_symbol ||
        ref->symbol > c->binding_count || !ref_source ||
        ref_source->kind != NL_SYNTAX_EXPR_NAME ||
        !span_equal(ref->span, ref_source->span) ||
        !token(f->source, ref->name, c->bindings[ref->symbol - 1].name) ||
        !span_equal(ref_source->data.name, ref->name) ||
        !token(f->source, source->data.loan.binding,
               c->bindings[ref->symbol - 1].name) ||
        !nl_c17_scalar_ref(c, ref->type, u8, NL_ACCESS_WRITE) ||
        !nl_semantic_value_view(c, ref->results[0].value, &value) ||
        value.type != ref->type || value.reference_count ||
        value.reference.provenance != NL_PROVENANCE_VALID ||
        !value.reference.readable || !value.reference.writable ||
        value.reference.place != change->child ||
        value.reference.incarnation != change->child_incarnation ||
        value.reference.scope != loan->loan.scope || !arg ||
        arg->next_argument ||
        !scalar_name_copy(f, c, ref->next_argument,
                          nl_syntax_next_argument(first), selected, scalar) ||
        (!delegated && change->new_value != arg->results[0].value))
        return false;
    *writer = delegated ? body->tail : 0;
    return true;
}

static bool descriptor_read(const NLCheckedFragment *f,
                            const NLSemanticContext *c, NLCheckedNodeId id,
                            const NLSyntaxView *source,
                            const NLCheckedNodeView *original,
                            const NLCheckedField *change, NLTypeId u8)
{
    const NLCheckedNodeView *read = nl_checked_node_view(f, id);
    NLValueId value;
    return read && read->kind == NL_CHECKED_FIELD_READ &&
           read->value_use == NL_VALUE_COPIED &&
           read->field.access == NL_ACCESS_READ &&
           descriptor_field_source(f, c, source, &read->field, original,
                                   change->nominal, u8) &&
           read->field.parent == change->parent &&
           read->field.parent_incarnation == change->parent_incarnation &&
           read->field.parent_fact == change->parent_post_fact &&
           scalar_node_value(f, c, id, u8, &value) &&
           quantity(c, read->field.old_value, u8) &&
           c->values[value - 1].scalar_known ==
               c->values[read->field.old_value - 1].scalar_known &&
           c->values[value - 1].scalar_value ==
               c->values[read->field.old_value - 1].scalar_value;
}

typedef struct {
    NLValueId constructor;
    NLCheckedField change;
    const NLCheckedField *sibling;
    NLCheckedNodeId writer, second_update, second_writer;
    NLCopyPolicyWriter policy;
} GuardedByteDescriptor;

static bool descriptor_target_read(const NLCheckedField *read,
                                   const NLCheckedField *change)
{
    return read->index == change->index && read->child == change->child &&
           read->child_incarnation == change->child_incarnation &&
           read->child_fact == change->child_post_fact &&
           read->old_value == change->new_value;
}

/* The actual second call must begin with the first loan already closed and
 * the same field at the first Change's post-state. No caller world is joined.
 */
static bool descriptor_reloan(
    const NLCheckedFragment *f, const NLCheckedNodeView *first_loan,
    const NLCheckedNodeView *second_loan, NLCheckedNodeId writer,
    NLCheckedNodeId second_writer, const NLCheckedField *first,
    const NLCheckedField *second, NLCopyBodyParameters *parameters)
{
    NLSemanticScopeView ended;
    const NLCheckedNodeView *first_call = nl_checked_node_view(f, writer);
    const NLCheckedNodeView *second_call =
        nl_checked_node_view(f, second_writer);
    return first_call && second_call &&
           first_call->function == second_call->function &&
           first_loan->loan.scope != second_loan->loan.scope &&
           first_loan->span.end_byte < second_loan->span.start_byte &&
           nl_checked_copy_body_parameters(f, second_writer, parameters) &&
           nl_semantic_scope_view(parameters->entry, first_loan->loan.scope,
                                  &ended) &&
           !ended.active && second->parent == first->parent &&
           second->parent_incarnation == first->parent_incarnation &&
           second->index == first->index && second->child == first->child &&
           second->child_incarnation == first->child_incarnation &&
           second->parent_fact == first->parent_post_fact &&
           second->child_fact == first->child_post_fact &&
           second->old_value == first->new_value;
}

/* Verify the two post-loan field reads against this original's packages.
 * Registered writer routes may observe either field first. */
static bool
descriptor_observations(const NLCheckedFragment *f, const NLSemanticContext *c,
                        const NLCheckedNodeView *original,
                        const NLSemanticValueView *initial,
                        NLValueId initial_target, const NLCheckedField *change,
                        const NLCheckedNodeView *const bindings[2],
                        const NLSyntaxNode *const sources[2], bool registered,
                        const NLCheckedField **sibling_out)
{
    const NLSemanticBindingView local = c->bindings[original->symbol - 1].view;
    NLSemanticPlaceView place;
    NLSemanticValueView final;
    if (local.place != change->parent ||
        !nl_semantic_place_view(c, local.place, &place) ||
        place.incarnation != change->parent_incarnation ||
        place.type != change->nominal ||
        !nl_semantic_value_view(c, local.value, &final) ||
        final.type != change->nominal || final.field_count != 2)
        return false;
    const NLCheckedNodeView *reads[2];
    for (size_t i = 0; i < 2; ++i) {
        if (!descriptor_read(f, c, bindings[i]->initializer,
                             nl_syntax_node_view(sources[i]), original, change,
                             change->type))
            return false;
        reads[i] = nl_checked_node_view(f, bindings[i]->initializer);
    }
    const size_t target =
        registered && reads[0]->field.index != change->index ? 1 : 0;
    const NLCheckedField *updated = &reads[target]->field;
    const NLCheckedField *sibling = &reads[1 - target]->field;
    if (!descriptor_target_read(updated, change) ||
        sibling->index == change->index || sibling->child == change->child ||
        initial->fields[change->index] != initial_target ||
        final.fields[change->index] != change->new_value ||
        initial->fields[sibling->index] != sibling->old_value ||
        final.fields[sibling->index] != sibling->old_value)
        return false;
    *sibling_out = sibling;
    return true;
}

static bool guarded_byte_completion(const NLCheckedFragment *f,
                                    const NLCheckedNodeView *root,
                                    const NLSyntaxView *source,
                                    NLCheckedNodeId id,
                                    const NLSyntaxNode *item, NLTypeId unit)
{
    if (root->terminates) {
        const NLSyntaxView *ret = nl_syntax_node_view(item);
        const NLCheckedNodeView *end = nl_checked_node_view(f, id);
        return ret && ret->kind == NL_SYNTAX_RETURN && end &&
               !nl_syntax_next_argument(item) && !source->data.block.tail &&
               division_unit_return(f, root, end, ret, unit);
    }
    return !id && !item && root->type == unit && !root->result_count &&
           !root->results[0].value && !nl_control_exits_count(f->exits) &&
           division_unit_completion(f, root->tail, source->data.block.tail,
                                    unit);
}

static bool guarded_byte_report_route(QuantityBody *body, NLCheckedNodeId id,
                                      const NLSyntaxNode *syntax,
                                      NLSymbolId bytes)
{
    const NLCheckedFragment *f = body->body;
    const NLCheckedNodeView *test = nl_checked_node_view(f, id);
    const NLCheckedNodeView *limit =
        test ? nl_checked_node_view(f, test->first_argument) : NULL;
    const NLCheckedNodeView *actual =
        limit ? nl_checked_node_view(f, limit->next_argument) : NULL;
    if (!test || test->kind != NL_CHECKED_USIZE_LESS_THAN || !limit ||
        limit->kind != NL_CHECKED_USIZE_LITERAL || !actual ||
        actual->kind != NL_CHECKED_IDENTIFIER || actual->symbol != bytes)
        return false;
    /* The prefix has already validated the producer of this exact binding. */
    body->origins[bytes - 1] = QUANTITY_FROM_RESULT;
    return quantity_condition_at(body, id, syntax) == QUANTITY_FROM_RESULT;
}

static bool guarded_byte_report_choice(const NLCheckedFragment *f,
                                       const NLSemanticContext *c,
                                       NLCheckedNodeId id,
                                       const NLSyntaxView *source,
                                       NLSymbolId route,
                                       const NLSymbolId samples[2], NLTypeId u8)
{
    const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
    NLValueId joined;
    if (!source || source->kind != NL_SYNTAX_IF || !choice ||
        choice->kind != NL_CHECKED_IF || choice->normal_arms != 2 ||
        choice->item_count != 2 || choice->tail ||
        !span_equal(choice->span, source->span) ||
        !scalar_node_value(f, c, id, u8, &joined) ||
        !scalar_name_copy(f, c, choice->initializer,
                          source->data.conditional.condition, route,
                          nl_semantic_core_type(c, NL_TYPE_BOOL)))
        return false;
    const NLCheckedNodeView *condition =
        nl_checked_node_view(f, choice->initializer);
    const NLSyntaxNode *syntax[] = {source->data.conditional.then_block,
                                    source->data.conditional.else_block};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, i);
        if (!copy_if_arm_origin(f, id, i, arm) || arm->context == c ||
            arm->branch_value_prefix != condition->results[0].value ||
            arm->arm_count || arm->body_count ||
            nl_control_exits_count(arm->exits) ||
            nl_checked_node_count(arm) != 3)
            return false;
        const NLCheckedNodeView *root = nl_checked_node_view(arm, arm->root);
        const NLCheckedNodeView *block =
            root ? nl_checked_node_view(arm, root->initializer) : NULL;
        const NLSyntaxView *shape = nl_syntax_node_view(syntax[i]);
        NLValueId exported, body_value, leaf;
        if (!root || root->kind != NL_CHECKED_IF_ARM || !block ||
            block->kind != NL_CHECKED_BLOCK || block->item_count || !shape ||
            shape->kind != NL_SYNTAX_BLOCK || shape->data.block.items ||
            !span_equal(shape->span, root->span) ||
            !span_equal(shape->span, block->span) ||
            !scalar_node_value(arm, arm->context, arm->root, u8, &exported) ||
            !scalar_node_value(arm, arm->context, root->initializer, u8,
                               &body_value) ||
            !scalar_node_value(arm, arm->context, block->tail, u8, &leaf) ||
            exported != body_value || body_value != leaf)
            return false;
        if (samples[i]) {
            NLSemanticBindingView sample;
            if (!nl_semantic_binding_view(arm->context, samples[i], &sample) ||
                sample.availability != NL_AVAILABLE || sample.type != u8 ||
                sample.value != c->bindings[samples[i] - 1].view.value ||
                !scalar_name_copy(arm, arm->context, block->tail,
                                  shape->data.block.tail, samples[i], u8))
                return false;
        } else {
            const NLSyntaxView *source_tail =
                nl_syntax_node_view(shape->data.block.tail);
            const NLCheckedNodeView *literal =
                nl_checked_node_view(arm, block->tail);
            if (!source_tail || source_tail->kind != NL_SYNTAX_U8_LITERAL ||
                !literal || !span_equal(source_tail->span, literal->span) ||
                !copy_if_literal_at(arm, arm->context, literal))
                return false;
        }
    }
    return true;
}

/* Match the fixed continuation, ending with the original field observations
 * and exact displaced Copies. The two-call form reuses the same writer. */
static bool guarded_byte_descriptor(
    QuantityBody *body, const NLCheckedNodeView *root,
    const NLSyntaxView *source, NLCheckedNodeId id, const NLSyntaxNode *item,
    NLSymbolId selected, NLSymbolId bytes, const NLCheckedNodeView *original,
    const NLSyntaxView *original_source, bool local_policy, bool sequential,
    bool midpoint, bool report, bool route, bool payload, bool handoff,
    bool delivery, GuardedByteDescriptor *out)
{
    const NLCheckedFragment *f = body->body;
    const NLSemanticContext *c = body->context;
    const bool intermediate =
        sequential && (root->item_count == 14 || midpoint);
    const size_t last_exchange =
        sequential ? 2 + (size_t)intermediate + (size_t)midpoint : 0;
    const size_t observation = last_exchange + 1;
    const size_t binding_count = last_exchange + 4 + (size_t)sequential +
                                 (report ? 2u : 0u) + (size_t)route +
                                 (size_t)payload + (size_t)handoff +
                                 (delivery ? 2u : 0u);
    const NLCheckedNodeView *bindings[16];
    const NLSyntaxNode *initializer_sources[16];
    NLCheckedNodeId second_update = 0;
    NLSymbolId previous =
        selected > original->symbol ? selected : original->symbol;
    for (size_t i = 0; i < binding_count; ++i) {
        const NLSyntaxView *s = nl_syntax_node_view(item);
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (!binding_source(f, c, n, s) ||
            (n->next_item && n->next_item <= id) || n->symbol <= previous)
            return false;
        if (sequential && i == last_exchange)
            second_update = id;
        bindings[i] = n;
        initializer_sources[i] = s->data.binding.initializer;
        previous = n->symbol;
        id = n->next_item;
        item = nl_syntax_next_argument(item);
    }
    const NLTypeId unit = nl_semantic_unit_type(c);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    if (!guarded_byte_completion(f, root, source, id, item, unit))
        return false;

    const NLCheckedNodeView *ctor =
        nl_checked_node_view(f, original->initializer);
    const NLCheckedNodeView *loan =
        nl_checked_node_view(f, bindings[0]->initializer);
    NLSemanticValueView initial;
    if (!descriptor_constructor(
            f, c, ctor,
            nl_syntax_node_view(original_source->data.binding.initializer), u8,
            &initial))
        return false;
    NLCheckedField field;
    NLCheckedNodeId writer;
    NLCopyPolicyWriter policy = {0};
    if (!descriptor_replace(f, c, loan,
                            nl_syntax_node_view(initializer_sources[0]),
                            original, selected, ctor->type, u8, &field, &writer,
                            local_policy ? &policy : NULL))
        return false;
    const NLCheckedField *change = &field;
    NLCheckedField second_field;
    NLCheckedNodeId second_writer = 0;
    if (sequential) {
        const size_t mode_index = last_exchange - 1;
        const NLCheckedNodeView *mode = bindings[mode_index];
        const NLCheckedNodeView *value =
            nl_checked_node_view(f, mode->initializer);
        const bool saved_copy =
            intermediate && scalar_name_copy(f, c, mode->initializer,
                                             initializer_sources[mode_index],
                                             bindings[0]->symbol, u8);
        const NLCheckedNodeView *second_loan =
            nl_checked_node_view(f, bindings[last_exchange]->initializer);
        NLCopyBodyParameters second_call;
        if (!writer || !value || value->type != u8 ||
            (!saved_copy && !copy_if_literal_at(f, c, value)) ||
            !descriptor_replace(
                f, c, second_loan,
                nl_syntax_node_view(initializer_sources[last_exchange]),
                original, mode->symbol, ctor->type, u8, &second_field,
                &second_writer, NULL) ||
            !descriptor_reloan(f, loan, second_loan, writer, second_writer,
                               &field, &second_field, &second_call))
            return false;
        if (intermediate) {
            const NLCheckedNodeId read = bindings[1]->initializer;
            if (!descriptor_read(f, c, read,
                                 nl_syntax_node_view(initializer_sources[1]),
                                 original, &field, u8) ||
                !descriptor_target_read(&nl_checked_node_view(f, read)->field,
                                        &field))
                return false;
        }
        if (saved_copy) {
            NLSemanticBindingView first_old, recovery;
            if (!nl_semantic_binding_view(second_call.entry,
                                          bindings[0]->symbol, &first_old) ||
                first_old.availability != NL_AVAILABLE ||
                first_old.type != u8 ||
                first_old.value != loan->results[0].value ||
                !nl_semantic_binding_view(second_call.entry, mode->symbol,
                                          &recovery) ||
                recovery.availability != NL_AVAILABLE || recovery.type != u8 ||
                recovery.value != value->results[0].value)
                return false;
        }
        change = &second_field;
    }
    const NLCheckedField *sibling_field;
    if (!descriptor_observations(f, c, original, &initial, field.old_value,
                                 change, bindings + observation,
                                 initializer_sources + observation, writer != 0,
                                 &sibling_field) ||
        !scalar_name_copy(f, c, bindings[observation + 2]->initializer,
                          initializer_sources[observation + 2],
                          bindings[0]->symbol, u8))
        return false;
    if (sequential &&
        !scalar_name_copy(f, c, bindings[observation + 3]->initializer,
                          initializer_sources[observation + 3],
                          bindings[last_exchange]->symbol, u8))
        return false;
    if (midpoint) {
        const NLCheckedNodeId read = bindings[2]->initializer;
        if (!descriptor_read(f, c, read,
                             nl_syntax_node_view(initializer_sources[2]),
                             original, &field, u8))
            return false;
        const NLCheckedField *peer = &nl_checked_node_view(f, read)->field;
        if (peer->index != sibling_field->index ||
            peer->child != sibling_field->child ||
            peer->child_incarnation != sibling_field->child_incarnation ||
            peer->child_fact != sibling_field->child_fact ||
            peer->old_value != sibling_field->old_value)
            return false;
    }
    if (report) {
        NLCopyBodyParameters second_call;
        if (!nl_checked_copy_body_parameters(f, second_writer, &second_call))
            return false;
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedNodeView *sample = bindings[1 + i];
            const NLCheckedNodeView *read =
                nl_checked_node_view(f, sample->initializer);
            const size_t result = observation + 4 + i;
            NLSemanticBindingView at_entry;
            if (!nl_semantic_binding_view(second_call.entry, sample->symbol,
                                          &at_entry) ||
                at_entry.availability != NL_AVAILABLE || at_entry.type != u8 ||
                at_entry.value != read->results[0].value ||
                !scalar_name_copy(f, c, bindings[result]->initializer,
                                  initializer_sources[result], sample->symbol,
                                  u8))
                return false;
        }
    }
    if (route &&
        !guarded_byte_report_route(body, bindings[observation + 6]->initializer,
                                   initializer_sources[observation + 6], bytes))
        return false;
    if (payload) {
        const NLSymbolId samples[] = {bindings[observation + 5]->symbol,
                                      bindings[observation + 4]->symbol};
        if (!guarded_byte_report_choice(
                f, c, bindings[observation + 7]->initializer,
                nl_syntax_node_view(initializer_sources[observation + 7]),
                bindings[observation + 6]->symbol, samples, u8))
            return false;
    }
    if (handoff &&
        !scalar_name_copy(f, c, bindings[observation + 8]->initializer,
                          initializer_sources[observation + 8],
                          bindings[observation + 7]->symbol, u8))
        return false;
    if (delivery) {
        const size_t ready = observation + 9;
        /* The false arm has an exact u8 literal instead of a named binding. */
        const NLSymbolId forwarded[] = {bindings[observation + 8]->symbol, 0};
        if (!guarded_byte_report_route(body, bindings[ready]->initializer,
                                       initializer_sources[ready], bytes) ||
            !guarded_byte_report_choice(
                f, c, bindings[ready + 1]->initializer,
                nl_syntax_node_view(initializer_sources[ready + 1]),
                bindings[ready]->symbol, forwarded, u8))
            return false;
    }
    *out = (GuardedByteDescriptor){.constructor = ctor->results[0].value,
                                   .change = field,
                                   .sibling = sibling_field,
                                   .writer = writer,
                                   .second_update = second_update,
                                   .second_writer = second_writer,
                                   .policy = policy};
    return true;
}

/* The first original must still be present at its exact post-Change state
 * when the second original is handed to the writer. */
static bool descriptor_after_change(const NLSemanticContext *context,
                                    const GuardedByteDescriptor *original,
                                    NLValueId value)
{
    const NLCheckedField *change = &original->change;
    const NLCheckedField *peer = original->sibling;
    NLSemanticBindingView binding;
    NLSemanticPlaceView root, field, sibling;
    return nl_semantic_binding_view(context, change->base, &binding) &&
           binding.availability == NL_AVAILABLE && binding.value == value &&
           binding.place == change->parent && binding.type == change->nominal &&
           nl_semantic_place_view(context, binding.place, &root) && root.live &&
           root.independent_root && root.implicit_local &&
           root.incarnation == change->parent_incarnation &&
           root.current_fact == change->parent_post_fact &&
           root.current_value == value && root.fixed_field_count == 2 &&
           root.fixed_fields[change->index] == change->child &&
           root.fixed_fields[peer->index] == peer->child &&
           nl_semantic_place_view(context, change->child, &field) &&
           field.live && field.incarnation == change->child_incarnation &&
           field.current_fact == change->child_post_fact &&
           field.current_value == change->new_value &&
           nl_semantic_place_view(context, peer->child, &sibling) &&
           sibling.live && sibling.incarnation == peer->child_incarnation &&
           sibling.current_fact == peer->child_fact &&
           sibling.current_value == peer->old_value;
}

static bool
descriptor_ordered_originals(const NLCheckedFragment *f,
                             const NLSemanticContext *c,
                             const NLCheckedNodeView *const loans[2],
                             const GuardedByteDescriptor fields[2],
                             const NLCheckedNodeView *const mode_bindings[2])
{
    const NLCheckedField *left = &fields[0].change, *right = &fields[1].change;
    if (left->nominal != right->nominal)
        return false;
    const NLPlaceId first_places[] = {left->parent, left->child,
                                      fields[0].sibling->child};
    const NLPlaceId second_places[] = {right->parent, right->child,
                                       fields[1].sibling->child};
    for (size_t i = 0; i < 3; ++i)
        for (size_t j = 0; j < 3; ++j)
            if (first_places[i] == second_places[j])
                return false;
    NLSemanticScopeView closed;
    NLSemanticBindingView final;
    NLCopyBodyParameters first_call, second_call;
    if (loans[0]->loan.scope == loans[1]->loan.scope ||
        loans[0]->span.end_byte >= loans[1]->span.start_byte ||
        !nl_checked_copy_body_parameters(f, fields[0].writer, &first_call) ||
        !nl_checked_copy_body_parameters(f, fields[1].writer, &second_call) ||
        !nl_semantic_scope_view(second_call.entry, loans[0]->loan.scope,
                                &closed) ||
        closed.active || !nl_semantic_binding_view(c, left->base, &final) ||
        !descriptor_after_change(second_call.entry, &fields[0], final.value) ||
        !nl_c17_descriptor_frame(first_call.entry, right->base,
                                 fields[1].constructor, right,
                                 fields[1].sibling))
        return false;
    if (mode_bindings) {
        NLSemanticBindingView input;
        const NLCheckedNodeView *copy =
            nl_checked_node_view(f, mode_bindings[1]->initializer);
        /* The second call must still hold the exact checked Copy. */
        if (!copy ||
            !nl_semantic_binding_view(second_call.entry,
                                      mode_bindings[1]->symbol, &input) ||
            input.availability != NL_AVAILABLE || input.type != left->type ||
            input.value != copy->results[0].value)
            return false;
        if (copy->kind == NL_CHECKED_IDENTIFIER) {
            NLSemanticBindingView returned;
            if (!nl_semantic_binding_view(
                    second_call.entry, mode_bindings[0]->symbol, &returned) ||
                returned.availability != NL_AVAILABLE ||
                returned.type != left->type ||
                returned.value != loans[0]->results[0].value)
                return false;
        }
    }
    const NLCopyBodyParameters calls[] = {first_call, second_call};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedField *change = &fields[i].change;
        if (!nl_c17_descriptor_frame(calls[i].entry, change->base,
                                     fields[i].constructor, change,
                                     fields[i].sibling) ||
            !nl_semantic_scope_view(c, loans[i]->loan.scope, &closed) ||
            closed.active)
            return false;
        const NLPlaceId places[] = {change->parent, change->child,
                                    fields[i].sibling->child};
        const NLIncarnationId incarnations[] = {
            change->parent_incarnation, change->child_incarnation,
            fields[i].sibling->child_incarnation};
        for (size_t j = 0; j < 3; ++j) {
            NLSemanticPlaceView ended;
            if (!nl_semantic_place_view(c, places[j], &ended) || ended.live ||
                ended.incarnation != incarnations[j])
                return false;
        }
    }
    return true;
}

/* Two early originals and two one-Change calls. The first original's reads
 * can precede the second loan when its target Copy also supplies the mode. */
static bool guarded_byte_distinct_descriptors(
    const NLCheckedFragment *f, const NLSemanticContext *c,
    const NLCheckedNodeView *root, const NLSyntaxView *source,
    NLCheckedNodeId id, const NLSyntaxNode *item, NLSymbolId selected,
    const NLCheckedNodeView *const originals[2],
    const NLSyntaxView *const original_sources[2], bool snapshot_first,
    GuardedByteDescriptor fields[2], NLCheckedNodeId *second_update)
{
    const NLCheckedNodeView *bindings[9];
    const NLSyntaxNode *initializers[9];
    const size_t count = snapshot_first ? 8 : 9;
    const size_t updates[] = {0, snapshot_first ? 3u : 2u};
    const size_t reads[] = {snapshot_first ? 1u : 3u, count - 4};
    NLSymbolId previous = selected;
    for (size_t i = 0; i < count; ++i) {
        const NLSyntaxView *syntax = nl_syntax_node_view(item);
        const NLCheckedNodeView *binding = nl_checked_node_view(f, id);
        if (!binding_source(f, c, binding, syntax) ||
            (binding->next_item && binding->next_item <= id) ||
            binding->symbol <= previous)
            return false;
        if (i == updates[1])
            *second_update = id;
        bindings[i] = binding;
        initializers[i] = syntax->data.binding.initializer;
        previous = binding->symbol;
        id = binding->next_item;
        item = nl_syntax_next_argument(item);
    }
    if (!guarded_byte_completion(f, root, source, id, item,
                                 nl_semantic_unit_type(c)))
        return false;
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    const NLCheckedNodeView *mode =
        nl_checked_node_view(f, bindings[1]->initializer);
    if (!mode || mode->type != u8)
        return false;
    const bool observed = mode->kind == NL_CHECKED_FIELD_READ;
    if (snapshot_first && !observed)
        return false;
    const bool displaced = mode->kind == NL_CHECKED_IDENTIFIER;
    const bool copied = observed || displaced;
    if (!copied && !copy_if_literal_at(f, c, mode))
        return false;
    if (displaced &&
        !scalar_name_copy(f, c, bindings[1]->initializer, initializers[1],
                          bindings[0]->symbol, u8))
        return false;
    const NLSymbolId actuals[] = {selected, bindings[1]->symbol};
    const NLCheckedNodeView *loans[2];
    for (size_t i = 0; i < 2; ++i) {
        const size_t update = updates[i], read = reads[i];
        const size_t copy = count - 2 + i;
        const NLCheckedNodeView *original = originals[i];
        const NLCheckedNodeView *ctor =
            nl_checked_node_view(f, original->initializer);
        NLSemanticValueView initial;
        loans[i] = nl_checked_node_view(f, bindings[update]->initializer);
        GuardedByteDescriptor *field = &fields[i];
        if (!ctor ||
            !descriptor_constructor(
                f, c, ctor,
                nl_syntax_node_view(
                    original_sources[i]->data.binding.initializer),
                u8, &initial) ||
            !descriptor_replace(f, c, loans[i],
                                nl_syntax_node_view(initializers[update]),
                                original, actuals[i], ctor->type, u8,
                                &field->change, &field->writer, NULL) ||
            field->writer != f->body_calls[2 + i] ||
            !descriptor_observations(f, c, original, &initial,
                                     field->change.old_value, &field->change,
                                     bindings + read, initializers + read, true,
                                     &field->sibling) ||
            !scalar_name_copy(f, c, bindings[copy]->initializer,
                              initializers[copy], bindings[update]->symbol, u8))
            return false;
        field->constructor = ctor->results[0].value;
    }
    /* A read feeding the second writer must observe the first Change. */
    const NLCheckedField *active = &fields[0].change;
    if (copied && (loans[0]->span.end_byte >= mode->span.start_byte ||
                   mode->span.end_byte >= loans[1]->span.start_byte))
        return false;
    if (observed && (!descriptor_read(f, c, bindings[1]->initializer,
                                      nl_syntax_node_view(initializers[1]),
                                      originals[0], active, u8) ||
                     !descriptor_target_read(&mode->field, active)))
        return false;
    /* Displaced handoffs and normally completing snapshots require
     * matching declared positions. */
    if ((displaced || (snapshot_first && !root->terminates)) &&
        active->index != fields[1].change.index)
        return false;
    return descriptor_ordered_originals(f, c, loans, fields,
                                        copied ? bindings : NULL);
}

/* The unchanged scalar validators identify these three choices. Read the
 * original and its unchanged pre-Change fields in both arms of each. */
static bool guarded_byte_live_original(const NLCheckedFragment *f,
                                       NLCheckedNodeId count,
                                       NLCheckedNodeId product,
                                       const NLCheckedFragment *policy_body,
                                       NLCheckedNodeId selection,
                                       const GuardedByteDescriptor *original)
{
    const NLCheckedFragment *product_body = nl_checked_call_body(f, product);
    const NLCheckedNodeView *product_root =
        product_body ? nl_checked_node_view(product_body, product_body->root)
                     : NULL;
    if (!product_root)
        return false;
    const NLCheckedFragment *owners[] = {f, product_body, policy_body};
    const NLCheckedNodeId choices[] = {count, product_root->tail, selection};
    for (size_t choice = 0; choice < 3; ++choice)
        for (size_t arm = 0; arm < 2; ++arm) {
            const NLCheckedFragment *branch =
                nl_checked_if_arm(owners[choice], choices[choice], arm);
            if (!nl_c17_descriptor_frame(nl_checked_context(branch),
                                         original->change.base,
                                         original->constructor,
                                         &original->change, original->sibling))
                return false;
        }
    return true;
}

/* This whole entry is also walked before CHECK seals its result. The normal
 * published call validator still requires the original complete entry world. */
static bool guarded_byte_mode_entry(const NLCheckedFragment *f,
                                    const NLSemanticContext *c,
                                    const NLFunctionEntry *fn, size_t function,
                                    bool entry, NLDivisionUsizePlan *out)
{
    if (f->body_count < 2 || f->body_count > 4)
        return false;
    const NLSyntaxView *source =
        nl_syntax_node_view(nl_syntax_tree_root(fn->body->syntax));
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    if (!source || source->kind != NL_SYNTAX_BLOCK || !root ||
        root->kind != NL_CHECKED_BLOCK ||
        !span_equal(root->span, source->span) || root->item_count < 4)
        return false;

    /* The in-writer route has eight bindings and an optional Return item. */
    const bool local_policy = root->item_count == (root->terminates ? 9u : 8u);
    const size_t binding_count = root->item_count - (size_t)root->terminates;
    const NLCheckedNodeView *first = nl_checked_node_view(f, root->first_item);
    const NLCheckedNodeView *first_value =
        first ? nl_checked_node_view(f, first->initializer) : NULL;
    const NLCheckedNodeView *next =
        first ? nl_checked_node_view(f, first->next_item) : NULL;
    const NLCheckedNodeView *next_value =
        next ? nl_checked_node_view(f, next->initializer) : NULL;
    /* One original is followed by the rank, not another constructor. */
    const bool one_original =
        f->body_count == 4 && root->terminates && first_value &&
        first_value->kind == NL_CHECKED_AGGREGATE && next_value &&
        next_value->kind == NL_CHECKED_USIZE_LITERAL;
    const bool delivery = one_original && binding_count == 21;
    const bool handoff = one_original && (binding_count == 19 || delivery);
    const bool payload = one_original && (binding_count == 18 || handoff);
    const bool route = one_original && (binding_count == 17 || payload);
    const size_t expected_arms = 2 + (payload ? 2u : 0u) + (delivery ? 2u : 0u);
    if (f->arm_count != expected_arms)
        return false;
    const bool report = one_original && (binding_count == 16 || route);
    const bool midpoint = one_original && (binding_count == 14 || report);
    const bool snapshot_first =
        f->body_count == 4 && binding_count == 14 && !midpoint;
    const bool distinct =
        f->body_count == 4 && (binding_count == 15 || snapshot_first);
    const bool sequential = f->body_count == 4 && !distinct;
    if (sequential &&
        (!root->terminates ||
         (root->item_count != 13 && root->item_count != 14 && !midpoint)))
        return false;
    const bool descriptor_entry = local_policy || sequential || distinct ||
                                  (root->terminates && root->item_count == 10);
    if (root->item_count > 6 && (!entry || !descriptor_entry))
        return false;
    const size_t prefix = local_policy ? 3 : 4;
    const NLCheckedKind kinds[] = {NL_CHECKED_USIZE_LITERAL, NL_CHECKED_IF,
                                   NL_CHECKED_REGISTERED_CALL,
                                   NL_CHECKED_REGISTERED_CALL};
    const NLCheckedNodeView *bindings[4];
    const NLSyntaxNode *initializers[4];
    const NLSyntaxNode *item = source->data.block.items;
    NLCheckedNodeId id = root->first_item;
    NLCheckedNodeId descriptor = 0;
    const NLSyntaxNode *descriptor_item = NULL;
    NLCheckedNodeId second_descriptor = 0;
    const NLCheckedNodeView *standby = NULL;
    const NLSyntaxView *standby_source = NULL;
    const bool early = descriptor_entry && first_value &&
                       first_value->kind == NL_CHECKED_AGGREGATE;
    if ((local_policy && (!early || f->body_count != 2)) ||
        ((sequential || distinct) && !early))
        return false;
    if (early) {
        if (first->next_item <= id)
            return false;
        descriptor = id;
        descriptor_item = item;
        id = first->next_item;
        item = nl_syntax_next_argument(item);
    }
    if (distinct) {
        second_descriptor = id;
        standby = nl_checked_node_view(f, id);
        standby_source = nl_syntax_node_view(item);
        if (!binding_source_shape(f, c, standby, standby_source) ||
            standby->symbol <= first->symbol || standby->next_item <= id)
            return false;
        id = standby->next_item;
        item = nl_syntax_next_argument(item);
    }
    for (size_t i = 0; i < prefix; ++i) {
        const NLSyntaxView *src = nl_syntax_node_view(item);
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (!binding_source(f, c, n, src) ||
            (n->next_item && n->next_item <= id) ||
            n->symbol > NL_SEMANTIC_MAX_ENTRIES ||
            (i && bindings[i - 1]->symbol >= n->symbol) ||
            nl_checked_node_view(f, n->initializer)->kind != kinds[i])
            return false;
        bindings[i] = n;
        initializers[i] = src->data.binding.initializer;
        id = n->next_item;
        item = nl_syntax_next_argument(item);
    }
    const NLCheckedNodeId product = bindings[2]->initializer;
    const NLCheckedNodeId selector =
        local_policy ? 0 : bindings[3]->initializer;
    size_t remaining = NL_SEMANTIC_MAX_ENTRIES;
    QuantityBody b = {
        .body = f, .context = c, .function = fn, .remaining = &remaining};
    NLPureU8Call pure = {0};
    const NLSymbolId key = bindings[0]->symbol;
    if (quantity_expression_at(&b, bindings[0]->initializer, initializers[0]) !=
        QUANTITY_INDEPENDENT)
        return false;
    b.origins[key - 1] = QUANTITY_INDEPENDENT;
    if (!guarded_byte_count(&b, bindings[1]->initializer, initializers[1],
                            key) ||
        b.arms != 2 || b.calls || f->body_calls[0] != product ||
        !guarded_byte_actual(f, c, product, initializers[2],
                             bindings[1]->symbol) ||
        !guarded_byte_product(&b, product, initializers[2]))
        return false;
    if (!local_policy && (f->body_calls[1] != selector ||
                          !guarded_byte_actual(f, c, selector, initializers[3],
                                               bindings[2]->symbol) ||
                          !nl_pure_u8_threshold_at(f, c, selector, &pure)))
        return false;

    if (descriptor_entry) {
        if (!early) {
            descriptor = id;
            descriptor_item = item;
        }
        const NLCheckedNodeView *original = nl_checked_node_view(f, descriptor);
        const NLSyntaxView *original_source =
            nl_syntax_node_view(descriptor_item);
        if (!binding_source_shape(f, c, original, original_source) ||
            (early ? original->symbol >= key
                   : original->symbol <= bindings[3]->symbol))
            return false;
        if (!early) {
            if (original->next_item <= id)
                return false;
            id = original->next_item;
            item = nl_syntax_next_argument(item);
        }
        if (distinct) {
            if (standby->symbol >= key)
                return false;
            const NLCheckedNodeView *originals[] = {original, standby};
            const NLSyntaxView *sources[] = {original_source, standby_source};
            GuardedByteDescriptor originals_fields[2] = {0};
            NLCheckedNodeId second_update = 0;
            if (!guarded_byte_distinct_descriptors(
                    f, c, root, source, id, item, bindings[3]->symbol,
                    originals, sources, snapshot_first, originals_fields,
                    &second_update))
                return false;
            for (size_t i = 0; i < 2; ++i)
                if (!guarded_byte_live_original(
                        f, bindings[1]->initializer, product, pure.body,
                        pure.selection, &originals_fields[i]))
                    return false;
            *out = (NLDivisionUsizePlan){
                .body = f,
                .function = function,
                .byte_selector = selector,
                .byte_descriptor = descriptor,
                .byte_update = id,
                .byte_update_call = originals_fields[0].writer,
                .byte_second_descriptor = second_descriptor,
                .byte_second_update = second_update,
                .byte_second_update_call = originals_fields[1].writer,
                .byte_snapshot_first = snapshot_first};
            return true;
        }
        GuardedByteDescriptor fields;
        if (!guarded_byte_descriptor(
                &b, root, source, id, item, bindings[prefix - 1]->symbol,
                bindings[2]->symbol, original, original_source, local_policy,
                sequential, midpoint, report, route, payload, handoff, delivery,
                &fields))
            return false;
        if (sequential) {
            if (f->body_calls[2] != fields.writer ||
                f->body_calls[3] != fields.second_writer)
                return false;
        } else if (local_policy) {
            if (f->body_calls[1] != fields.writer)
                return false;
        } else if (fields.writer ? (!early || f->body_count != 3 ||
                                    f->body_calls[2] != fields.writer)
                                 : f->body_count != 2) {
            return false;
        }
        if (early &&
            !guarded_byte_live_original(
                f, bindings[1]->initializer, product,
                local_policy ? fields.policy.selection_body : pure.body,
                local_policy ? fields.policy.selection : pure.selection,
                &fields))
            return false;
        *out = (NLDivisionUsizePlan){.body = f,
                                     .function = function,
                                     .byte_selector = selector,
                                     .byte_descriptor = descriptor,
                                     .byte_update = id,
                                     .byte_update_call = fields.writer,
                                     .byte_second_update = fields.second_update,
                                     .byte_second_update_call =
                                         fields.second_writer,
                                     .byte_snapshot_first = midpoint};
        return true;
    }

    if (f->body_count != 2)
        return false;
    size_t count = 4;
    const NLTypeId unit = nl_semantic_unit_type(c);
    const NLSyntaxView *end = nl_syntax_node_view(item);
    if (end && end->kind == NL_SYNTAX_BINDING) {
        const NLCheckedNodeView *copy = nl_checked_node_view(f, id);
        if (!binding_source(f, c, copy, end) ||
            (copy->next_item && copy->next_item <= id) ||
            !scalar_name_copy(
                f, c, copy->initializer, end->data.binding.initializer,
                bindings[3]->symbol, nl_semantic_core_type(c, NL_TYPE_U8)))
            return false;
        ++count;
        id = copy->next_item;
        item = nl_syntax_next_argument(item);
        end = nl_syntax_node_view(item);
    }
    if (item) {
        const NLCheckedNodeView *ret = nl_checked_node_view(f, id);
        if (!end || end->kind != NL_SYNTAX_RETURN || !ret ||
            nl_syntax_next_argument(item) || source->data.block.tail ||
            !division_unit_return(f, root, ret, end, unit))
            return false;
        ++count;
    } else if (id || root->terminates || root->type != unit ||
               root->result_count || root->results[0].value ||
               nl_control_exits_count(f->exits) ||
               !division_unit_completion(f, root->tail, source->data.block.tail,
                                         unit))
        return false;
    if (count != root->item_count)
        return false;
    *out = (NLDivisionUsizePlan){
        .body = f, .function = function, .byte_selector = selector};
    return true;
}

static bool literal_divisor_body_at(const NLCheckedFragment *f,
                                    const NLSemanticContext *c, size_t function,
                                    NLUsizeOperation operation, bool entry,
                                    NLDivisionUsizePlan *out)
{
    if (!f || !c || !out || !function || function > c->function_count ||
        f->body_count > NL_CHECKED_MAX_BODY_CALLS ||
        (f->body_count && !f->body_calls))
        return false;
    const NLFunctionEntry *fn = &c->functions[function - 1];
    size_t remaining = NL_SEMANTIC_MAX_ENTRIES;
    if (fn->count || fn->result == nl_semantic_core_type(c, NL_TYPE_USIZE))
        return quantity_helper_at(f, c, function, operation, 0, &remaining,
                                  out);
    const NLTypeId unit = nl_semantic_unit_type(c);
    if (fn->kind != NL_CHECKED_REGISTERED_CALL || fn->count ||
        fn->result != unit || !fn->body || fn->body->count ||
        !fn->body->syntax || f->body_owner != fn->body ||
        f->source != fn->body->source || fn->caller_effects ||
        fn->hidden_dependencies || fn->owner_receiver || fn->owner_producer ||
        fn->custody_recipient)
        return false;
    if (f->arm_count || has_composed_actual(f, c))
        return operation == NL_USIZE_ANY_FIXED_DIVISOR &&
               !strcmp(fn->name, "main") &&
               (composed_quantity_body(f, c, fn, function, 0, &remaining,
                                       out) ||
                guarded_byte_mode_entry(f, c, fn, function, entry, out));
    const NLSyntaxView *s =
        nl_syntax_node_view(nl_syntax_tree_root(fn->body->syntax));
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    if (!s || s->kind != NL_SYNTAX_BLOCK || !root ||
        root->kind != NL_CHECKED_BLOCK || !span_equal(s->span, root->span))
        return false;

    /* Direct arithmetic and unit return belong to the actual scalar entry.
     * Keep the existing specialized helper-only profiles as their own paths. */
    const bool scalar_entry =
        operation == NL_USIZE_ANY_FIXED_DIVISOR && !strcmp(fn->name, "main");
    bool has_direct_divisor = false, returned = false;
    NLDivisionUsizePlan plan = {.body = f, .function = function};
    NLCheckedNodeId id = root->first_item;
    size_t count = 0, calls = 0;
    for (const NLSyntaxNode *item = s->data.block.items; item;
         item = nl_syntax_next_argument(item)) {
        const NLSyntaxView *src = nl_syntax_node_view(item);
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (++count > NL_PARSER_MAX_NODES || !src || !n ||
            (n->next_item && n->next_item <= id))
            return false;
        if (src->kind == NL_SYNTAX_RETURN) {
            if (!scalar_entry || nl_syntax_next_argument(item) ||
                s->data.block.tail ||
                !division_unit_return(f, root, n, src, unit))
                return false;
            returned = true;
            id = 0;
            break;
        }
        if (src->kind == NL_SYNTAX_STATEMENT) {
            const NLSyntaxNode *copy = discarded_quantity_expression(
                f, c, id, n, src, nl_semantic_core_type(c, NL_TYPE_USIZE),
                NL_SYNTAX_EXPR_NAME);
            if (!scalar_entry || !copy ||
                !division_copy_at(f, c, n->initializer, copy, 0))
                return false;
            id = n->next_item;
            continue;
        }
        if (!binding_source(f, c, n, src))
            return false;
        const NLSyntaxNode *syntax = src->data.binding.initializer;
        const NLCheckedNodeView *init = nl_checked_node_view(f, n->initializer);
        if (init->kind == NL_CHECKED_REGISTERED_CALL) {
            if (calls >= f->body_count ||
                f->body_calls[calls] != n->initializer ||
                !division_call_at(f, c, n->initializer, syntax, plan.call,
                                  operation, &remaining))
                return false;
            if (++calls == 1)
                plan.call = n->initializer;
        } else if (init->kind == NL_CHECKED_USIZE_ADDITION ||
                   init->kind == NL_CHECKED_USIZE_SUBTRACTION) {
            if (!division_caller_arithmetic(f, c, n->initializer, syntax,
                                            init->kind ==
                                                    NL_CHECKED_USIZE_ADDITION
                                                ? NL_USIZE_ADD
                                                : NL_USIZE_SUBTRACT))
                return false;
        } else if (init->kind == NL_CHECKED_USIZE_DIVISION ||
                   init->kind == NL_CHECKED_USIZE_REMAINDER) {
            const NLUsizeOperation direct =
                init->kind == NL_CHECKED_USIZE_DIVISION ? NL_USIZE_DIVIDE
                                                        : NL_USIZE_REMAINDER;
            if (!scalar_entry || !division_caller_arithmetic(
                                     f, c, n->initializer, syntax, direct))
                return false;
            has_direct_divisor = true;
        } else if (init->kind == NL_CHECKED_USIZE_LITERAL) {
            size_t number;
            if (!literal(f, c, init, syntax, &number))
                return false;
        } else if (!division_copy_at(f, c, n->initializer, syntax, 0))
            return false;
        id = n->next_item;
    }
    if (id || count != root->item_count || calls != f->body_count ||
        (!calls && !has_direct_divisor))
        return false;
    if (!returned &&
        (root->terminates || root->type != unit || root->result_count ||
         root->results[0].value || nl_control_exits_count(f->exits) ||
         !division_unit_completion(f, root->tail, s->data.block.tail, unit)))
        return false;
    *out = plan;
    return true;
}

static NLCheckStatus literal_divisor_call_plan(const NLCheckedFragment *f,
                                               NLCheckedNodeId call,
                                               NLUsizeOperation operation,
                                               NLDivisionUsizePlan *out)
{
    if (!f || !out || nl_checked_proof_class(f) != NL_CHECKED_MACHINE_PROVED ||
        !nl_h_custody_result_valid(f, call))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    const NLCheckedNodeView *n = nl_checked_node_view(f, call);
    const NLCheckedFragment *body = nl_checked_call_body(f, call);
    NLDivisionUsizePlan plan;
    if (!n || !body || n->kind != NL_CHECKED_REGISTERED_CALL ||
        !n->body_backed || n->terminates ||
        !literal_divisor_body_at(body, f->context, n->function, operation,
                                 !f->body_owner && call == nl_checked_root(f),
                                 &plan))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    if (plan.formal ||
        n->type == nl_semantic_core_type(f->context, NL_TYPE_USIZE)) {
        if (!division_call_values(f, f->context, call, &plan))
            return NL_CHECK_SEMANTIC_UNSUPPORTED;
        if (plan.division) {
            const NLCheckedNodeView *event =
                nl_checked_node_view(body, plan.division);
            if (!event || !((event->kind == NL_CHECKED_USIZE_DIVISION &&
                             nl_checked_usize_division_valid(
                                 body, plan.division, NULL)) ||
                            (event->kind == NL_CHECKED_USIZE_REMAINDER &&
                             nl_checked_usize_remainder_valid(
                                 body, plan.division, NULL))))
                return NL_CHECK_SEMANTIC_UNSUPPORTED;
        }
    } else {
        if (n->argument_count || n->first_argument ||
            n->type != nl_semantic_unit_type(f->context) || n->result_count ||
            n->results[0].value || !nl_h_custody_result_world_present(f, call))
            return NL_CHECK_SEMANTIC_UNSUPPORTED;
        /* Each actual invocation owns its result even when the emitter uses
         * one shared C definition for the same source declaration. */
        for (size_t i = 0; i < body->body_count; ++i) {
            const NLCheckedNodeId child = body->body_calls[i];
            if ((plan.byte_update_call && child == plan.byte_update_call) ||
                (plan.byte_second_update_call &&
                 child == plan.byte_second_update_call)) {
                /* The scoped writer has its own existing replay proof. The
                 * complete entry seal above already covers its owned graph. */
                const NLCheckStatus status =
                    plan.byte_selector
                        ? nl_checked_copy_body_parameters_validate(body, child,
                                                                   NULL)
                        : nl_copy_policy_writer_validate(body, child);
                if (status == NL_CHECK_OUT_OF_MEMORY ||
                    status == NL_CHECK_RESOURCE_LIMIT)
                    return status;
                if (status != NL_CHECK_OK)
                    return NL_CHECK_SEMANTIC_UNSUPPORTED;
            } else if (!nl_h_custody_result_valid(body, child))
                return NL_CHECK_SEMANTIC_UNSUPPORTED;
        }
    }
    *out = plan;
    return NL_CHECK_OK;
}

bool nl_division_usize_call_valid(const NLCheckedFragment *f,
                                  NLCheckedNodeId call,
                                  NLDivisionUsizePlan *out)
{
    return literal_divisor_call_plan(f, call, NL_USIZE_DIVIDE, out) ==
           NL_CHECK_OK;
}

bool nl_remainder_usize_call_valid(const NLCheckedFragment *f,
                                   NLCheckedNodeId call,
                                   NLDivisionUsizePlan *out)
{
    return literal_divisor_call_plan(f, call, NL_USIZE_REMAINDER, out) ==
           NL_CHECK_OK;
}

bool nl_quantity_usize_body_at(const NLCheckedFragment *f,
                               const NLSemanticContext *c, size_t function,
                               bool entry, NLDivisionUsizePlan *out)
{
    if (entry || nl_quantity_usize_function(c, function, 0) ||
        nl_quantity_usize_function(c, function, 1) ||
        nl_quantity_bool_function(c, function) ||
        nl_quantity_usize_function(c, function, NL_QUANTITY_MAX_ARGUMENTS))
        return literal_divisor_body_at(f, c, function,
                                       NL_USIZE_ANY_FIXED_DIVISOR, entry, out);
    return literal_divisor_body_at(f, c, function, NL_USIZE_DIVIDE, false,
                                   out) ||
           literal_divisor_body_at(f, c, function, NL_USIZE_REMAINDER, false,
                                   out);
}

bool nl_quantity_usize_call_valid(const NLCheckedFragment *f,
                                  NLCheckedNodeId call,
                                  NLDivisionUsizePlan *out)
{
    return nl_quantity_usize_call_plan(f, call, out) == NL_CHECK_OK;
}

NLCheckStatus nl_quantity_usize_call_plan(const NLCheckedFragment *f,
                                          NLCheckedNodeId call,
                                          NLDivisionUsizePlan *out)
{
    return literal_divisor_call_plan(f, call, NL_USIZE_ANY_FIXED_DIVISOR, out);
}
