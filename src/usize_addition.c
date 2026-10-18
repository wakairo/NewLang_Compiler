#include "newlang/usize_addition.h"
#include "semantic_internal.h"
#include "syntax_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* One small immutable receipt per source operation; no world clone, hidden
 * authority registry, or alternate arithmetic interpreter. */
typedef struct NLUsizeAdditionProof {
    NLUsizeAdditionView issued;
    const NLSyntaxNode *syntax;
    const NLSemanticContext *context;
    NLValueId binding_values[2];
    struct NLUsizeAdditionProof *next;
} NLUsizeAdditionProof;

static bool span_equal(NLSourceSpan a, NLSourceSpan b)
{
    return a.start_byte == b.start_byte && a.end_byte == b.end_byte;
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

NLCheckStatus nl_usize_addition_capture(NLCheckedFragment *f,
                                        NLCheckedNodeId id,
                                        const NLSyntaxNode *syntax,
                                        const NLSemanticContext *working)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (!f || !n || !syntax || n->kind != NL_CHECKED_USIZE_ADDITION)
        return NL_CHECK_INTERNAL_ERROR;
    NLUsizeAdditionProof *p = calloc(1, sizeof(*p));
    if (!p)
        return NL_CHECK_OUT_OF_MEMORY;
    p->syntax = syntax;
    /* f->context is the publication destination; CHECK still operates on its
     * transaction clone. Read current facts from working, qualify saved IDs by
     * the destination that receives those exact tables only on commit. */
    p->context = f->context;
    NLUsizeAdditionView *v = &p->issued;
    *v = (NLUsizeAdditionView){.origin = f,
                               .node = id,
                               .type = n->type,
                               .result = n->results[0].value,
                               .span = n->span,
                               .operator_span = n->name,
                               .sum = n->scalar_result.value};
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
        if (a->kind == NL_CHECKED_IDENTIFIER) {
            NLSemanticBindingView b;
            NLSemanticPlaceView place;
            if (a->value_use != NL_VALUE_COPIED ||
                !nl_semantic_binding_view(working, a->symbol, &b) ||
                b.availability != NL_AVAILABLE ||
                !nl_semantic_place_view(working, b.place, &place) ||
                !place.live || !place.current_fact ||
                place.current_value != b.value || place.type != n->type ||
                !plain(working, b.value, n->type, value.scalar_value))
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

bool nl_checked_usize_addition_view(const NLCheckedFragment *f,
                                    NLCheckedNodeId id,
                                    NLUsizeAdditionView *out)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!p || !out)
        return false;
    *out = p->issued;
    return true;
}

static bool same(const NLUsizeAdditionView *a, const NLUsizeAdditionView *b)
{
    if (a->origin != b->origin || a->node != b->node || a->type != b->type ||
        a->result != b->result || a->sum != b->sum ||
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

bool nl_checked_usize_addition_valid(const NLCheckedFragment *f,
                                     NLCheckedNodeId id,
                                     const NLUsizeAdditionView *candidate)
{
    const NLUsizeAdditionProof *p = find(f, id);
    if (!p || !f->body_owner || f->source != f->body_owner->source ||
        p->context != f->context || p->issued.origin != f ||
        (candidate && !same(candidate, &p->issued)))
        return false;
    bool registered = false, retained = false;
    for (size_t i = 0; i < f->context->function_count; ++i)
        if (f->context->functions[i].body == f->body_owner &&
            f->context->functions[i].kind == NL_CHECKED_REGISTERED_CALL)
            registered = true;
    for (const NLSyntaxNode *n = f->body_owner->syntax->nodes; n;
         n = n->owned_next)
        if (n == p->syntax)
            retained = true;
    if (!registered || !retained || nl_sem_validate(f->context) != NL_CHECK_OK)
        return false;
    const NLUsizeAdditionView *v = &p->issued;
    const NLSyntaxView *s = nl_syntax_node_view(p->syntax);
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    NLSemanticTypeView t;
    if (v->type != nl_semantic_core_type(f->context, NL_TYPE_USIZE) ||
        !nl_semantic_type_view(f->context, v->type, &t) || !t.is_copy ||
        t.kind != NL_TYPE_USIZE || s->kind != NL_SYNTAX_USIZE_ADDITION || !n ||
        n->kind != NL_CHECKED_USIZE_ADDITION || n->terminates ||
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
        !token(f->source, v->operator_span, "+") ||
        n->value_use != NL_VALUE_USE_NONE || !n->has_scalar_result ||
        !n->scalar_result.known || n->scalar_result.type != v->type ||
        n->scalar_result.value != v->sum ||
        v->numbers[1] > SIZE_MAX - v->numbers[0] ||
        v->sum != v->numbers[0] + v->numbers[1] ||
        !plain(f->context, v->result, v->type, v->sum))
        return false;
    const NLSyntaxNode *atoms[] = {s->data.equality.left,
                                   s->data.equality.right};
    for (size_t i = 0; i < 2; ++i) {
        const NLSyntaxView *a = nl_syntax_node_view(atoms[i]);
        const NLCheckedNodeView *c = nl_checked_node_view(f, v->operands[i]);
        if (!a || !c || v->operands[i] >= id ||
            (i == 0 && v->operands[0] >= v->operands[1]) ||
            v->values[i] == v->result ||
            !span_equal(a->span, v->operand_spans[i]) ||
            !span_equal(c->span, v->operand_spans[i]) || c->type != v->type ||
            c->result_count != 1 || c->results[0].type != v->type ||
            c->results[0].value != v->values[i] ||
            c->next_argument != (i == 0 ? v->operands[1] : 0) ||
            !plain(f->context, v->values[i], v->type, v->numbers[i]))
            return false;
        if (a->kind == NL_SYNTAX_USIZE_LITERAL) {
            if (c->kind != NL_CHECKED_USIZE_LITERAL || v->symbols[i] ||
                v->places[i] || v->facts[i] || v->incarnations[i] ||
                !c->has_scalar_result || !c->scalar_result.known ||
                c->scalar_result.type != v->type ||
                c->scalar_result.value != v->numbers[i])
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
            if (c->kind != NL_CHECKED_IDENTIFIER ||
                c->value_use != NL_VALUE_COPIED || c->symbol != v->symbols[i] ||
                !c->symbol || c->symbol > f->context->binding_count ||
                !token(f->source, a->data.name,
                       f->context->bindings[c->symbol - 1].name) ||
                !span_equal(c->name, a->data.name) ||
                !nl_semantic_binding_view(f->context, c->symbol, &b) ||
                b.type != v->type || b.place != v->places[i] ||
                b.value != p->binding_values[i] ||
                !nl_semantic_place_view(f->context, b.place, &place) ||
                place.type != v->type ||
                (!place.live && (place.current_fact || place.current_value)) ||
                (place.live &&
                 (!place.current_fact || !place.current_value ||
                  place.current_fact > f->context->last_value_fact)) ||
                place.incarnation != v->incarnations[i] || !v->facts[i] ||
                v->facts[i] > f->context->last_value_fact ||
                !plain(f->context, p->binding_values[i], v->type,
                       v->numbers[i]))
                return false;
        } else
            return false;
    }
    return true;
}
