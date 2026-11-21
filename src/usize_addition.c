#include "newlang/usize_addition.h"
#include "division_usize_internal.h"
#include "guarded_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"
#include "newlang/usize_subtraction.h"
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
                            NL_CHECKED_USIZE_REMAINDER, "%"}};
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

static bool arithmetic_result(NLUsizeOperation operation, size_t lhs,
                              size_t rhs, size_t answer)
{
    if (operation == NL_USIZE_ADD)
        return rhs <= SIZE_MAX - lhs && answer == lhs + rhs;
    if (operation == NL_USIZE_SUBTRACT)
        return lhs >= rhs && answer == lhs - rhs;
    if (operation == NL_USIZE_DIVIDE)
        return rhs && answer == lhs / rhs;
    if (operation == NL_USIZE_REMAINDER)
        return rhs && answer < rhs && answer == lhs % rhs;
    return false;
}

static bool operation_valid_at(const NLCheckedFragment *f, NLCheckedNodeId id,
                               NLUsizeOperation operation,
                               const NLUsizeArithmeticView *candidate,
                               const NLSemanticContext *c)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!p || p->guarded || p->operation != operation || !f->body_owner ||
        f->source != f->body_owner->source || p->context != f->context || !c ||
        p->issued.origin != f || (candidate && !same(candidate, &p->issued)))
        return false;
    bool registered = false, retained = false;
    for (size_t i = 0; i < c->function_count; ++i)
        if (c->functions[i].body == f->body_owner &&
            c->functions[i].kind == NL_CHECKED_REGISTERED_CALL)
            registered = true;
    for (const NLSyntaxNode *n = f->body_owner->syntax->nodes; n;
         n = n->owned_next)
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
                              f ? f->context : NULL);
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
                              f ? f->context : NULL);
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
                              f->context);
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
static const NLFunctionEntry *quantity_function(const NLSemanticContext *c,
                                                size_t id, size_t count)
{
    if (!c || !c->functions || !id || id > c->function_count)
        return NULL;
    const NLFunctionEntry *fn = &c->functions[id - 1];
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    return fn->kind == NL_CHECKED_REGISTERED_CALL && fn->count == count &&
                   fn->result == type && fn->body && fn->body->syntax &&
                   fn->body->source && fn->body->count == count &&
                   (!count || (fn->parameters && fn->parameters[0] == type &&
                               fn->body->parameter_names[0])) &&
                   !fn->caller_effects && !fn->hidden_dependencies &&
                   !fn->owner_receiver && !fn->owner_producer &&
                   !fn->custody_recipient
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
static bool guard_shape(const NLCheckedFragment *f, const NLSemanticContext *c,
                        size_t function, NLSymbolId formal,
                        const NLSyntaxNode *syntax, NLCheckedNodeId left,
                        NLCheckedNodeId right, size_t *limit, size_t *step)
{
    const NLFunctionEntry *fn = quantity_function(c, function, 1);
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
    const NLSyntaxView *sum = nl_syntax_node_view(syntax);
    if (!fn || !f || !parent || !choice || !test || !a || !bound || !x || !k ||
        f->if_arm_index != 0 || f->source != parent->source ||
        f->source != fn->body->source || choice->kind != NL_CHECKED_IF ||
        test->kind != NL_CHECKED_USIZE_LESS_THAN ||
        choice->initializer >= f->if_parent_node || !sum ||
        sum->kind != NL_SYNTAX_USIZE_ADDITION ||
        source_node(fn->body, sum->kind, sum->span) != syntax || !formal ||
        formal > c->binding_count || a->kind != NL_CHECKED_IDENTIFIER ||
        a->symbol != formal || a->value_use != NL_VALUE_COPIED ||
        a->result_count != 1 || a->results[0].value > f->branch_value_prefix ||
        x->kind != NL_CHECKED_IDENTIFIER || x->symbol != formal ||
        x->value_use != NL_VALUE_COPIED || x->result_count != 1 ||
        !span_equal(x->span,
                    nl_syntax_node_view(sum->data.equality.left)->span) ||
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
        !literal(f, c, k, sum->data.equality.right, step) || !*limit ||
        *step > SIZE_MAX - (*limit - 1) ||
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
           guard_shape(f, c, function, formal, syntax, left, right, &limit,
                       &step) &&
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
    if (!n || !left || n->kind != NL_CHECKED_USIZE_ADDITION ||
        !nl_guarded_usize_permitted(f, c, entry, function, formal, syntax,
                                    n->first_argument, left->next_argument))
        return NL_CHECK_INTERNAL_ERROR;
    NLUsizeAdditionProof *p = calloc(1, sizeof(*p));
    if (!p)
        return NL_CHECK_OUT_OF_MEMORY;
    p->guarded = true;
    p->operation = NL_USIZE_ADD;
    p->syntax = syntax;
    p->context = f->context;
    p->guard_parent = f->if_parent;
    p->conditional = f->if_parent_node;
    p->function = function;
    p->formal = formal;
    p->guard_binding = c->bindings[formal - 1].view;
    p->guard_place = c->places[p->guard_binding.place - 1];
    (void)guard_shape(f, c, function, formal, syntax, n->first_argument,
                      left->next_argument, &p->limit, &p->step);
    p->issued = (NLUsizeArithmeticView){
        .origin = f,
        .node = id,
        .type = n->type,
        .result = n->results[0].value,
        .span = n->span,
        .operator_span = n->name,
        .operands = {n->first_argument, left->next_argument}};
    p->next = f->usize_additions;
    f->usize_additions = p;
    f->destroy_usize_additions = destroy;
    return NL_CHECK_OK;
}
static bool guarded_event(const NLCheckedFragment *f, NLCheckedNodeId id,
                          size_t function, NLSymbolId formal,
                          NLGuardedUsizePlan *out)
{
    const NLUsizeAdditionProof *p = find(f, id);
    const NLSemanticContext *c = f ? f->context : NULL;
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    size_t limit, step;
    NLSemanticValueView value;
    if (!p || !p->guarded || p->operation != NL_USIZE_ADD || !c || !n ||
        p->issued.origin != f || p->context != c || p->function != function ||
        p->formal != formal || p->guard_parent != f->if_parent ||
        p->conditional != f->if_parent_node ||
        nl_checked_if_arm(f->if_parent, p->conditional, 0) != f ||
        !guard_shape(f, c, function, formal, p->syntax, p->issued.operands[0],
                     p->issued.operands[1], &limit, &step) ||
        limit != p->limit || step != p->step ||
        !same_binding(p->guard_binding, c->bindings[formal - 1].view) ||
        !same_operand_place(p->guard_place,
                            c->places[p->guard_binding.place - 1]) ||
        n->kind != NL_CHECKED_USIZE_ADDITION || n->terminates ||
        n->argument_count != 2 || n->first_argument != p->issued.operands[0] ||
        n->result_count != 1 || n->type != p->issued.type ||
        n->results[0].value != p->issued.result ||
        !span_equal(n->span, p->issued.span) ||
        !span_equal(n->name, p->issued.operator_span) ||
        !token(f->source, n->name, "+") || !n->has_scalar_result ||
        n->scalar_result.known || n->scalar_result.value ||
        !quantity(c, n->results[0].value, n->type) ||
        !nl_semantic_value_view(c, n->results[0].value, &value) ||
        value.scalar_known || value.scalar_value)
        return false;
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
        n->results[0].type != type || !quantity(c, n->results[0].value, type))
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
static bool binding_source(const NLCheckedFragment *f,
                           const NLSemanticContext *c,
                           const NLCheckedNodeView *n, const NLSyntaxView *s)
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
           c->bindings[n->symbol - 1].view.type == init->type &&
           c->bindings[n->symbol - 1].view.value == init->results[0].value;
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
                        const NLFunctionEntry *fn, NLGuardedUsizePlan *p)
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
        NLValueId block_value, arm_value;
        if (!node_value(arm, arm->context, ar->initializer, &block_value) ||
            !node_value(arm, arm->context, arm->root, &arm_value) ||
            block_value != arm_value)
            return false;
        if (a == 0) {
            const NLSyntaxNode *item = block_source->data.block.items;
            const NLCheckedNodeView *b =
                nl_checked_node_view(arm, block->first_item);
            if (!item || nl_syntax_next_argument(item) || !b || b->next_item ||
                !binding_source(arm, arm->context, b,
                                nl_syntax_node_view(item)) ||
                !guarded_event(arm, b->initializer, p->function, p->formal,
                               p) ||
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
    const NLFunctionEntry *fn = quantity_function(c, function, 1);
    if (!fn)
        fn = quantity_function(c, function, 0);
    if (!f || !fn || !out || f->body_owner != fn->body ||
        f->source != fn->body->source)
        return false;
    NLGuardedUsizePlan plan = {.body = f, .function = function};
    if (fn->count) {
        if (!helper_body(f, c, fn, &plan))
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
                    quantity_function(c, init->function, 1);
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
                    !helper_body(body, c, target, &nested) || !arg ||
                    arg->next_argument ||
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
        if (!binding || binding->kind != NL_CHECKED_BINDING ||
            (binding->next_item && binding->next_item <= id))
            return 0;
        if (binding->symbol == symbol)
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
        !operation_valid_at(f, id, receipt->operation, NULL, c) ||
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
    const NLFunctionEntry *fn = quantity_function(c, function, 1);
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

static bool division_call_values(const NLCheckedFragment *f,
                                 const NLSemanticContext *c, NLCheckedNodeId id,
                                 const NLDivisionUsizePlan *helper)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f, call->first_argument);
    NLValueId actual, result, body_result;
    return call->kind == NL_CHECKED_REGISTERED_CALL && call->body_backed &&
           call->argument_count == 1 && arg && !arg->next_argument &&
           helper->formal && call->function == helper->function &&
           node_value(f, c, call->first_argument, &actual) &&
           c->bindings[helper->formal - 1].view.value == actual &&
           (!arg->symbol || arg->symbol < helper->formal) &&
           node_value(f, c, id, &result) &&
           node_value(helper->body, c, helper->result, &body_result) &&
           result == body_result;
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
           src->kind == NL_SYNTAX_EXPR_CALL && src->data.call.arguments &&
           !nl_syntax_next_argument(src->data.call.arguments) &&
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
    if (!n ||
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
        !operation_valid_at(f, id, operation, NULL, c))
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
    const NLCheckedFragment *body;
    const NLSemanticContext *context;
    const NLFunctionEntry *function;
    size_t depth, calls, arms, *remaining;
    NLSymbolId formal;
    bool in_arm;
    unsigned char origins[NL_SEMANTIC_MAX_ENTRIES];
} QuantityBody;

typedef struct {
    const NLSyntaxNode *syntax;
    NLCheckedNodeId id;
    bool returned;
} QuantityResult;

static bool division_unit_expression(const NLCheckedFragment *, NLCheckedNodeId,
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
    if (!b->function->count || (b->formal && b->formal != symbol) ||
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
        c->values[value - 1].scalar_value > 1 ||
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
        src->kind != NL_SYNTAX_EXPR_CALL || !src->data.call.arguments ||
        nl_syntax_next_argument(src->data.call.arguments) ||
        call->argument_count != 1 || call->first_argument <= id ||
        !f->body_calls || nesting >= f->body_count ||
        b->depth + nesting >= NL_SEMANTIC_MAX_DEPTH || !*b->remaining)
        return QUANTITY_UNSEEN;
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f, call->first_argument);
    if (!arg || arg->next_argument)
        return QUANTITY_UNSEEN;
    const NLSyntaxNode *actual_source = src->data.call.arguments;
    QuantityOrigin actual;
    if (arg->kind == NL_CHECKED_REGISTERED_CALL) {
        if (arg->parameter_type != nl_semantic_core_type(c, NL_TYPE_USIZE))
            return QUANTITY_UNSEEN;
        actual = quantity_call_chain_at(b, call->first_argument, actual_source,
                                        nesting + 1);
    } else {
        actual = quantity_atom_at(b, call->first_argument, actual_source);
    }
    /* Node allocation reserves the outer call first. Owned bodies complete
     * in evaluation order, and each inner result is used by the next call. */
    if (!actual || (b->function->count && actual < QUANTITY_FROM_FORMAL) ||
        b->calls >= f->body_count || f->body_calls[b->calls] != id ||
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
    const NLSemanticContext *c = b->context;
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
    const NLUsizeAdditionProof *proof = find(f, id);
    if (b->in_arm || !proof || proof->syntax != syntax ||
        !operation_valid_at(f, id, proof->operation, NULL, c))
        return QUANTITY_UNSEEN;
    const QuantityOrigin left =
        quantity_atom_at(b, proof->issued.operands[0], src->data.equality.left);
    if (!left)
        return QUANTITY_UNSEEN;
    if (fixed_divisor(proof->operation))
        return !b->function->count || left >= QUANTITY_FROM_FORMAL
                   ? QUANTITY_FROM_RESULT
                   : QUANTITY_UNSEEN;
    const QuantityOrigin right = quantity_atom_at(b, proof->issued.operands[1],
                                                  src->data.equality.right);
    if (!right)
        return QUANTITY_UNSEEN;
    return left >= QUANTITY_FROM_FORMAL || right >= QUANTITY_FROM_FORMAL
               ? QUANTITY_FROM_RESULT
               : QUANTITY_INDEPENDENT;
}

static bool quantity_bindings_at(QuantityBody *b, const NLCheckedNodeView *root,
                                 const NLSyntaxView *source,
                                 QuantityResult *result)
{
    const NLCheckedFragment *f = b->body;
    const NLSemanticContext *c = b->context;
    if (!source || source->kind != NL_SYNTAX_BLOCK || !root ||
        root->kind != NL_CHECKED_BLOCK || !span_equal(root->span, source->span))
        return false;
    *result =
        (QuantityResult){.syntax = source->data.block.tail, .id = root->tail};
    NLCheckedNodeId id = root->first_item;
    size_t count = 0;
    for (const NLSyntaxNode *item = source->data.block.items; item;
         item = nl_syntax_next_argument(item)) {
        const NLSyntaxView *src = nl_syntax_node_view(item);
        const NLCheckedNodeView *node = nl_checked_node_view(f, id);
        if (++count > NL_PARSER_MAX_NODES || !src || !node)
            return false;
        if (src->kind == NL_SYNTAX_RETURN) {
            if (b->in_arm || nl_syntax_next_argument(item) || result->syntax ||
                !(b->function->count
                      ? division_return_package(f, c, root, node, src)
                      : division_unit_return(f, root, node, src,
                                             nl_semantic_unit_type(c))))
                return false;
            *result = (QuantityResult){.syntax = src->data.statement.expression,
                                       .id = node->initializer,
                                       .returned = true};
            id = 0;
            break;
        }
        if (!binding_source(f, c, node, src) ||
            node->symbol > NL_SEMANTIC_MAX_ENTRIES ||
            node->symbol == b->formal || b->origins[node->symbol - 1] ||
            (node->next_item && node->next_item <= id))
            return false;
        const NLSyntaxNode *init_source = src->data.binding.initializer;
        const NLCheckedNodeView *init =
            nl_checked_node_view(f, node->initializer);
        const QuantityOrigin origin =
            f->arm_count && init->type == nl_semantic_core_type(c, NL_TYPE_BOOL)
                ? quantity_condition_at(b, node->initializer, init_source)
                : quantity_expression_at(b, node->initializer, init_source);
        if (!origin || node->symbol == b->formal)
            return false;
        b->origins[node->symbol - 1] = (unsigned char)origin;
        id = node->next_item;
    }
    return id == 0 && count == root->item_count;
}

static bool quantity_tail_at(const QuantityBody *b, NLCheckedNodeId block,
                             NLCheckedNodeId tail, NLValueId *value)
{
    NLValueId tail_value;
    return !nl_control_exits_count(b->body->exits) &&
           node_value(b->body, b->context, block, value) &&
           node_value(b->body, b->context, tail, &tail_value) &&
           *value == tail_value;
}

static QuantityOrigin quantity_if_at(QuantityBody *b, NLCheckedNodeId id,
                                     const NLSyntaxView *src)
{
    const NLCheckedFragment *f = b->body;
    const NLCheckedNodeView *node = nl_checked_node_view(f, id);
    NLValueId joined;
    if (b->in_arm || src->kind != NL_SYNTAX_IF ||
        !span_equal(node->span, src->span) || node->normal_arms != 2 ||
        node->item_count != 2 || node->tail ||
        !node_value(f, b->context, id, &joined) ||
        b->context->values[joined - 1].scalar_known || *b->remaining < 2)
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
        NLValueId arm_value, tail;
        if (!root || root->kind != NL_CHECKED_IF_ARM || !block ||
            !span_equal(root->span, block->span) ||
            !node_value(arm, arm->context, arm->root, &arm_value))
            return QUANTITY_UNSEEN;
        /* Reuse only the prefix locals in the fork. Each arm validates its
         * own bindings and calls, and exports its runtime value alone. */
        QuantityBody branch = *b;
        branch.body = arm;
        branch.context = arm->context;
        branch.calls = branch.arms = 0;
        branch.in_arm = true;
        QuantityResult result;
        if (!quantity_bindings_at(&branch,
                                  nl_checked_node_view(arm, root->initializer),
                                  block, &result) ||
            !quantity_tail_at(&branch, root->initializer, result.id, &tail) ||
            tail != arm_value)
            return QUANTITY_UNSEEN;
        const QuantityOrigin incoming =
            quantity_expression_at(&branch, result.id, result.syntax);
        if (!incoming || branch.calls != arm->body_count)
            return QUANTITY_UNSEEN;
        if (incoming > origin)
            origin = incoming;
        b->formal = branch.formal;
    }
    b->arms += 2;
    return origin >= QUANTITY_FROM_FORMAL ? QUANTITY_FROM_RESULT
                                          : QUANTITY_INDEPENDENT;
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
    const NLSyntaxView *source =
        nl_syntax_node_view(nl_syntax_tree_root(fn->body->syntax));
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    QuantityBody b = {.body = f,
                      .context = c,
                      .function = fn,
                      .depth = depth,
                      .remaining = remaining};
    QuantityResult result;
    if (!quantity_bindings_at(&b, root, source, &result))
        return false;
    if (fn->count) {
        NLValueId value;
        if ((!result.returned &&
             !quantity_tail_at(&b, f->root, result.id, &value)) ||
            quantity_expression_at(&b, result.id, result.syntax) !=
                QUANTITY_FROM_RESULT ||
            !b.formal)
            return false;
    } else {
        const NLTypeId unit = nl_semantic_unit_type(c);
        if (!result.returned &&
            (root->terminates || root->type != unit || root->result_count ||
             root->results[0].value || nl_control_exits_count(f->exits) ||
             !division_unit_expression(f, result.id, result.syntax, unit)))
            return false;
    }
    if (b.calls != f->body_count || b.arms != f->arm_count)
        return false;
    *out = (NLDivisionUsizePlan){.body = f,
                                 .function = function,
                                 .formal = b.formal,
                                 .result = result.id};
    return true;
}

static bool quantity_helper_at(const NLCheckedFragment *f,
                               const NLSemanticContext *c, size_t function,
                               NLUsizeOperation operation, size_t depth,
                               size_t *remaining, NLDivisionUsizePlan *out)
{
    if (!*remaining || depth >= NL_SEMANTIC_MAX_DEPTH)
        return false;
    --*remaining;
    return division_helper(f, c, function, operation, out) ||
           (operation == NL_USIZE_ANY_FIXED_DIVISOR &&
            composed_quantity_body(f, c, quantity_function(c, function, 1),
                                   function, depth, remaining, out));
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

static bool has_nested_actual(const NLCheckedFragment *f)
{
    for (size_t i = 0; i < f->body_count; ++i) {
        const NLCheckedNodeView *call =
            nl_checked_node_view(f, f->body_calls[i]);
        const NLCheckedNodeView *arg =
            call ? nl_checked_node_view(f, call->first_argument) : NULL;
        if (arg && arg->kind == NL_CHECKED_REGISTERED_CALL)
            return true;
    }
    return false;
}

static bool literal_divisor_body_at(const NLCheckedFragment *f,
                                    const NLSemanticContext *c, size_t function,
                                    NLUsizeOperation operation,
                                    NLDivisionUsizePlan *out)
{
    if (!f || !c || !out || !function || function > c->function_count ||
        f->body_count > NL_CHECKED_MAX_BODY_CALLS ||
        (f->body_count && !f->body_calls))
        return false;
    const NLFunctionEntry *fn = &c->functions[function - 1];
    size_t remaining = NL_SEMANTIC_MAX_ENTRIES;
    if (fn->count == 1)
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
    if (f->arm_count || has_nested_actual(f))
        return operation == NL_USIZE_ANY_FIXED_DIVISOR &&
               !strcmp(fn->name, "main") &&
               composed_quantity_body(f, c, fn, function, 0, &remaining, out);
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
        if (++count > NL_PARSER_MAX_NODES || !src || !n)
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
        if (!binding_source(f, c, n, src) ||
            (n->next_item && n->next_item <= id))
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
         !division_unit_expression(f, root->tail, s->data.block.tail, unit)))
        return false;
    *out = plan;
    return true;
}

static bool literal_divisor_call_valid(const NLCheckedFragment *f,
                                       NLCheckedNodeId call,
                                       NLUsizeOperation operation,
                                       NLDivisionUsizePlan *out)
{
    if (!f || !out || nl_checked_proof_class(f) != NL_CHECKED_MACHINE_PROVED ||
        !nl_h_custody_result_valid(f, call))
        return false;
    const NLCheckedNodeView *n = nl_checked_node_view(f, call);
    const NLCheckedFragment *body = nl_checked_call_body(f, call);
    NLDivisionUsizePlan plan;
    if (!n || !body || n->kind != NL_CHECKED_REGISTERED_CALL ||
        !n->body_backed || n->terminates ||
        !literal_divisor_body_at(body, f->context, n->function, operation,
                                 &plan))
        return false;
    if (plan.formal) {
        if (!division_call_values(f, f->context, call, &plan))
            return false;
        if (plan.division) {
            const NLCheckedNodeView *event =
                nl_checked_node_view(body, plan.division);
            if (!event || !((event->kind == NL_CHECKED_USIZE_DIVISION &&
                             nl_checked_usize_division_valid(
                                 body, plan.division, NULL)) ||
                            (event->kind == NL_CHECKED_USIZE_REMAINDER &&
                             nl_checked_usize_remainder_valid(
                                 body, plan.division, NULL))))
                return false;
        }
    } else {
        if (n->argument_count || n->first_argument ||
            n->type != nl_semantic_unit_type(f->context) || n->result_count ||
            n->results[0].value || !nl_h_custody_result_world_present(f, call))
            return false;
        /* Each actual invocation owns its result even when the emitter uses
         * one shared C definition for the same source declaration. */
        for (size_t i = 0; i < body->body_count; ++i)
            if (!nl_h_custody_result_valid(body, body->body_calls[i]))
                return false;
    }
    *out = plan;
    return true;
}

bool nl_division_usize_call_valid(const NLCheckedFragment *f,
                                  NLCheckedNodeId call,
                                  NLDivisionUsizePlan *out)
{
    return literal_divisor_call_valid(f, call, NL_USIZE_DIVIDE, out);
}

bool nl_remainder_usize_call_valid(const NLCheckedFragment *f,
                                   NLCheckedNodeId call,
                                   NLDivisionUsizePlan *out)
{
    return literal_divisor_call_valid(f, call, NL_USIZE_REMAINDER, out);
}

bool nl_quantity_usize_body_at(const NLCheckedFragment *f,
                               const NLSemanticContext *c, size_t function,
                               bool entry, NLDivisionUsizePlan *out)
{
    if (entry || quantity_function(c, function, 1))
        return literal_divisor_body_at(f, c, function,
                                       NL_USIZE_ANY_FIXED_DIVISOR, out);
    return literal_divisor_body_at(f, c, function, NL_USIZE_DIVIDE, out) ||
           literal_divisor_body_at(f, c, function, NL_USIZE_REMAINDER, out);
}

bool nl_quantity_usize_call_valid(const NLCheckedFragment *f,
                                  NLCheckedNodeId call,
                                  NLDivisionUsizePlan *out)
{
    return literal_divisor_call_valid(f, call, NL_USIZE_ANY_FIXED_DIVISOR, out);
}
