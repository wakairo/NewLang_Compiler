#ifndef NEWLANG_PURE_U8_INTERNAL_H
#define NEWLANG_PURE_U8_INTERNAL_H

#include "guarded_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "numeric_conversion_internal.h"
#include "scalar_view_internal.h"

static inline bool nl_pure_widening_function(const NLSemanticContext *c,
                                             const NLFunctionEntry *fn)
{
    return fn && fn->kind == NL_CHECKED_REGISTERED_CALL && fn->count == 1 &&
           fn->parameters &&
           fn->parameters[0] == nl_semantic_core_type(c, NL_TYPE_U8) &&
           fn->result == nl_semantic_core_type(c, NL_TYPE_USIZE) && fn->body &&
           fn->body->syntax && !fn->caller_effects &&
           !fn->hidden_dependencies && !fn->owner_receiver &&
           !fn->owner_producer && !fn->custody_recipient;
}

/* Shared private numeric consumer. The existing source checker establishes
 * both IF arms; this walk admits only pure scalar syntax in the actual owned
 * callable. It imports no arm value/current/owner into the caller. */
typedef struct {
    NLCheckedNodeId selection, final_return;
    size_t normal_arm, body_returns;
} NLPureU8Plan; /* transient owned-graph borrow; no allocation or issuer */

typedef struct {
    const NLCheckedFragment *body;
    const NLSemanticContext *context;
    NLSymbolId formal, incoming; /* optional second, plain u8 formal */
    const NLCheckedFragment *declarations; /* original prefix for arm Copies */
    NLTypeId quantity, boolean, u8, parameter;
    NLCheckedNodeId bindings[NL_SEMANTIC_MAX_ENTRIES];
    bool seen[NL_PARSER_MAX_NODES];
    bool guarded_narrowing, allow_widening, widened;
    size_t depth, selections;
    NLCheckedNodeId selection;
} PredicateWalk;

static const NLCheckedNodeView *predicate_take(PredicateWalk *w,
                                               NLCheckedNodeId id)
{
    if (!id || id > nl_checked_node_count(w->body) ||
        id > NL_PARSER_MAX_NODES || w->seen[id - 1])
        return NULL;
    w->seen[id - 1] = true;
    return nl_checked_node_view(w->body, id);
}

static bool predicate_value_at(const NLSemanticContext *c,
                               const NLCheckedNodeView *n,
                               NLSemanticValueView *v)
{
    return n && !n->terminates && n->result_count == 1 &&
           n->results[0].type == n->type &&
           nl_semantic_value_view(c, n->results[0].value, v) &&
           copy_if_plain(*v, n->type) &&
           (!n->has_scalar_result ||
            (n->scalar_result.type == n->type &&
             n->scalar_result.known == v->scalar_known &&
             n->scalar_result.value == v->scalar_value));
}

static inline bool predicate_value(const NLCheckedFragment *f,
                                   const NLCheckedNodeView *n,
                                   NLSemanticValueView *v)
{
    return predicate_value_at(nl_checked_context(f), n, v);
}

static bool predicate_name(const NLCheckedFragment *f, NLSourceSpan span,
                           const char *name)
{
    NLSourceView text;
    return name && nl_source_view(nl_checked_source(f), span, &text) &&
           text.length && text.length == strlen(name) &&
           !memcmp(text.bytes, name, text.length);
}

static bool predicate_scalar(PredicateWalk *, NLCheckedNodeId);

/* This only reads an already checked arm. Its exit world must additionally
 * be authenticated by the enclosing current-loop consumer before publication
 * consumers use a returning arm. */
static const NLCheckedNodeView *
predicate_arm_value(const NLCheckedFragment *arm)
{
    const NLCheckedNodeView *root =
        nl_checked_node_view(arm, nl_checked_root(arm));
    const NLCheckedNodeView *block =
        root ? nl_checked_node_view(arm, root->initializer) : NULL;
    const NLCheckedNodeView *ret =
        block ? nl_checked_node_view(arm, block->first_item) : NULL;
    return block ? nl_checked_node_view(arm, root->terminates && ret
                                                 ? ret->initializer
                                                 : block->tail)
                 : NULL;
}

static bool predicate_return(const NLCheckedFragment *owner,
                             const NLCheckedNodeView *ret, size_t ordinal,
                             const NLControlTarget *target)
{
    const NLControlExitView *x = nl_control_exit_view(owner->exits, ordinal);
    const NLCheckedNodeView *value =
        ret ? nl_checked_node_view(owner, ret->initializer) : NULL;
    return ret && value && ret->kind == NL_CHECKED_RETURN && ret->terminates &&
           !ret->result_count && !ret->next_item &&
           ret->returned.type == value->type &&
           ret->returned.value == value->results[0].value && x &&
           x->kind == NL_EXIT_RETURN && x->target == target && x->count == 1 &&
           !x->source_owner && !x->source_node && !x->source_target &&
           x->values[0].type == ret->returned.type &&
           x->values[0].value == ret->returned.value &&
           copy_if_gap(owner->source, ret->span.start_byte,
                       value->span.start_byte, "return");
}

/* Same scalar/source walk in a genuine fork. Prefix declarations belong to
 * the original body, while actual copied values belong to this arm world. */
static bool predicate_selection_arm(PredicateWalk *w,
                                    const NLCheckedFragment *arm,
                                    const NLCheckedNodeView *tail)
{
    if (tail->kind != NL_CHECKED_IDENTIFIER &&
        tail->kind != NL_CHECKED_U8_LITERAL)
        return false;
    PredicateWalk branch = *w;
    branch.body = arm;
    branch.context = nl_checked_context(arm);
    branch.declarations = w->body;
    branch.depth = 0;
    memset(branch.seen, 0, sizeof(branch.seen));
    const NLCheckedNodeView *root = nl_checked_node_view(arm, arm->root);
    const NLCheckedNodeView *block =
        nl_checked_node_view(arm, root->initializer);
    return predicate_scalar(&branch, block->tail);
}

static bool predicate_narrow_value(const PredicateWalk *w, NLCheckedNodeId id,
                                   const NLCheckedFragment *arm,
                                   NLCheckedNodeId value)
{
    const NLCheckedNodeView *cast = nl_checked_node_view(arm, value);
    if (!w->guarded_narrowing || w->incoming || w->parameter != w->quantity ||
        !cast || cast->kind != NL_CHECKED_NUMERIC_CONVERSION ||
        cast->type != w->u8)
        return false;
    const NLCheckedNodeView *body =
        nl_checked_node_view(w->body, nl_checked_root(w->body));
    const NLCheckedNodeView *operand =
        nl_checked_node_view(arm, cast->first_argument);
    const NLSemanticContext *c = nl_checked_context(arm);
    NLSemanticValueView result;
    return body && !body->item_count && !body->first_item && body->tail == id &&
           cast->argument_count == 1 && cast->first_argument < value &&
           cast->has_scalar_result && predicate_value_at(c, cast, &result) &&
           !result.scalar_known && !result.scalar_value && operand &&
           nl_guarded_u8_operand(arm, c, w->formal, cast->first_argument) &&
           copy_if_gap(arm->source, cast->span.start_byte,
                       operand->span.start_byte, "u8(") &&
           copy_if_gap(arm->source, operand->span.end_byte, cast->span.end_byte,
                       ")");
}

static bool predicate_binding_shape(PredicateWalk *, const NLCheckedNodeView *,
                                    NLTypeId);

static bool predicate_narrow_copy(PredicateWalk *w, NLCheckedNodeId id,
                                  NLSymbolId from)
{
    const NLCheckedNodeView *copy = predicate_take(w, id);
    NLSemanticValueView result;
    return copy && copy->kind == NL_CHECKED_IDENTIFIER && from &&
           copy->symbol == from && copy->type == w->u8 &&
           copy->value_use == NL_VALUE_COPIED && !copy->argument_count &&
           !copy->first_argument && !copy->next_argument &&
           copy->span.start_byte == copy->name.start_byte &&
           copy->span.end_byte == copy->name.end_byte &&
           predicate_name(w->body, copy->name,
                          w->context->bindings[from - 1].name) &&
           predicate_value_at(w->context, copy, &result) &&
           !result.scalar_known && !result.scalar_value;
}

/* The already authenticated arm owns these values and bindings. Account for
 * every node here and follow declaration symbols through the Copy tail. */
static bool predicate_narrow_copy_arm(const PredicateWalk *w,
                                      NLCheckedNodeId id,
                                      const NLCheckedFragment *arm,
                                      const NLCheckedNodeView *root,
                                      const NLCheckedNodeView *block)
{
    if (!w->guarded_narrowing || w->incoming || arm->if_arm_index != 0 ||
        block->terminates || nl_checked_node_count(arm) > NL_PARSER_MAX_NODES)
        return false;
    PredicateWalk local = {.body = arm,
                           .context = nl_checked_context(arm),
                           .formal = w->formal,
                           .u8 = w->u8};
    if (!predicate_take(&local, nl_checked_root(arm)) ||
        !predicate_take(&local, root->initializer))
        return false;
    const NLTypeId unit = nl_semantic_unit_type(local.context);
    NLSymbolId previous = 0;
    size_t count = 0, end = block->span.start_byte;
    for (NLCheckedNodeId item = block->first_item; item;) {
        const NLCheckedNodeView *binding = predicate_take(&local, item);
        if (!binding || binding->kind != NL_CHECKED_BINDING ||
            binding->initializer >= item ||
            !copy_if_gap(arm->source, end, binding->span.start_byte,
                         previous ? ";" : "{") ||
            !predicate_binding_shape(&local, binding, unit))
            return false;
        if (previous) {
            if (!predicate_narrow_copy(&local, binding->initializer, previous))
                return false;
        } else {
            const NLCheckedNodeView *cast =
                predicate_take(&local, binding->initializer);
            if (!cast ||
                !predicate_narrow_value(w, id, arm, binding->initializer) ||
                !predicate_take(&local, cast->first_argument))
                return false;
        }
        previous = binding->symbol;
        local.bindings[previous - 1] = item;
        end = binding->span.end_byte;
        item = binding->next_item;
        ++count;
    }
    const NLCheckedNodeView *tail = nl_checked_node_view(arm, block->tail);
    if (count != block->item_count ||
        !predicate_narrow_copy(&local, block->tail, previous) ||
        !copy_if_gap(arm->source, end, tail->span.start_byte, ";") ||
        !copy_if_gap(arm->source, tail->span.end_byte, block->span.end_byte,
                     "}"))
        return false;
    for (size_t i = 0; i < nl_checked_node_count(arm); ++i)
        if (!local.seen[i])
            return false;
    return true;
}

static bool predicate_arm(PredicateWalk *w, NLCheckedNodeId id, size_t index)
{
    const NLCheckedFragment *arm = nl_checked_if_arm(w->body, id, index);
    if (!arm || !copy_if_arm_origin(w->body, id, index, arm) ||
        arm->function_target != NULL)
        return false;
    const NLCheckedNodeView *root =
        nl_checked_node_view(arm, nl_checked_root(arm));
    const NLCheckedNodeView *block =
        root ? nl_checked_node_view(arm, root->initializer) : NULL;
    const NLCheckedNodeView *tail = predicate_arm_value(arm);
    if (!root || !block || !tail || root == block || root == tail ||
        block == tail || root->kind != NL_CHECKED_IF_ARM ||
        block->kind != NL_CHECKED_BLOCK || tail->type != w->u8)
        return false;
    const bool local = !root->terminates && block->first_item &&
                       predicate_narrow_copy_arm(w, id, arm, root, block);
    const bool narrow =
        !root->terminates && !block->terminates &&
        (local || (!block->first_item &&
                   predicate_narrow_value(w, id, arm, block->tail)));
    if (w->incoming
            ? (root->terminates || !predicate_selection_arm(w, arm, tail))
            : (!narrow && !copy_if_literal(arm, tail)))
        return false;
    if (root->terminates) {
        const NLCheckedNodeView *ret =
            nl_checked_node_view(arm, block->first_item);
        return nl_checked_node_count(arm) == 4 && block->terminates &&
               !root->result_count && !block->result_count && !block->tail &&
               block->item_count == 1 && ret && ret != root && ret != block &&
               ret != tail && root->returned.type == w->u8 &&
               block->returned.type == w->u8 &&
               root->returned.value == ret->returned.value &&
               block->returned.value == ret->returned.value &&
               nl_control_exits_count(arm->exits) == 1 &&
               predicate_return(arm, ret, 0, w->body->function_target) &&
               copy_if_gap(arm->source, block->span.start_byte,
                           ret->span.start_byte, "{") &&
               copy_if_gap(arm->source, tail->span.end_byte, ret->span.end_byte,
                           ";") &&
               copy_if_gap(arm->source, ret->span.end_byte,
                           block->span.end_byte, "}");
    }
    return !block->terminates && root->type == w->u8 && block->type == w->u8 &&
           root->result_count == 1 && block->result_count == 1 &&
           root->results[0].type == w->u8 && block->results[0].type == w->u8 &&
           root->results[0].value == tail->results[0].value &&
           block->results[0].value == tail->results[0].value &&
           !nl_control_exits_count(arm->exits) &&
           (local || (nl_checked_node_count(arm) == (narrow ? 4u : 3u) &&
                      !block->first_item && !block->item_count &&
                      copy_if_gap(arm->source, block->span.start_byte,
                                  tail->span.start_byte, "{") &&
                      copy_if_gap(arm->source, tail->span.end_byte,
                                  block->span.end_byte, "}")));
}

static bool predicate_selection(PredicateWalk *w, NLCheckedNodeId id,
                                const NLCheckedNodeView *n)
{
    const NLCheckedNodeView *cond =
        nl_checked_node_view(w->body, n->initializer);
    if (++w->selections != 1 || n->item_count != 2 || n->has_scalar_result ||
        !cond || cond->type != w->boolean ||
        !predicate_scalar(w, n->initializer) || !predicate_arm(w, id, 0) ||
        !predicate_arm(w, id, 1))
        return false;
    const NLCheckedFragment *a = nl_checked_if_arm(w->body, id, 0),
                            *b = nl_checked_if_arm(w->body, id, 1);
    const NLCheckedNodeView *x = nl_checked_node_view(a, a->root),
                            *y = nl_checked_node_view(b, b->root);
    const size_t normal = (size_t)!x->terminates + (size_t)!y->terminates;
    NLSemanticValueView value;
    if (n->normal_arms != normal || n->terminates != (normal == 0) ||
        !copy_if_gap(w->body->source, n->span.start_byte, cond->span.start_byte,
                     "if(") ||
        !copy_if_gap(w->body->source, cond->span.end_byte, x->span.start_byte,
                     ")") ||
        !copy_if_gap(w->body->source, x->span.end_byte, y->span.start_byte,
                     "else") ||
        y->span.end_byte != n->span.end_byte)
        return false;
    if (normal) {
        if (!predicate_value_at(w->context, n, &value) || n->type != w->u8)
            return false;
        if (normal == 2) {
            if (value.scalar_known || value.scalar_value || n->tail)
                return false;
        } else {
            /* CHECK reapplies the sole normal block in the parent world.
             * Consume every replay node and match its source and literal to
             * that genuine arm; it is not a second possible normal arm. */
            const NLCheckedFragment *arm = x->terminates ? b : a;
            const NLCheckedNodeView *ar = nl_checked_node_view(arm, arm->root),
                                    *ab = nl_checked_node_view(arm,
                                                               ar->initializer),
                                    *av = predicate_arm_value(arm);
            const NLCheckedNodeView *block = predicate_take(w, n->tail),
                                    *tail = block
                                                ? predicate_take(w, block->tail)
                                                : NULL;
            if (!block || !tail || block->kind != NL_CHECKED_BLOCK ||
                block->terminates || block->first_item || block->item_count ||
                block->type != w->u8 || block->result_count != 1 ||
                block->results[0].type != w->u8 ||
                block->results[0].value != tail->results[0].value ||
                n->results[0].value != tail->results[0].value ||
                block->span.start_byte != ab->span.start_byte ||
                block->span.end_byte != ab->span.end_byte ||
                tail->span.start_byte != av->span.start_byte ||
                tail->span.end_byte != av->span.end_byte ||
                !copy_if_literal(w->body, tail) ||
                tail->scalar_result.value != av->scalar_result.value)
                return false;
        }
    } else {
        if (n->result_count || n->tail || n->returned.type != w->u8 ||
            !nl_semantic_value_view(w->context, n->returned.value, &value) ||
            !copy_if_plain(value, w->u8) || value.scalar_known ||
            value.scalar_value)
            return false;
    }
    w->selection = id;
    return true;
}

static bool predicate_widening(PredicateWalk *w, NLCheckedNodeId id,
                               const NLCheckedNodeView *n,
                               NLSemanticValueView value)
{
    const NLCheckedNodeView *operand =
        nl_checked_node_view(w->body, n->first_argument);
    NLSemanticValueView input;
    if (!w->allow_widening || w->widened || n->type != w->quantity ||
        n->argument_count != 1 || n->first_argument >= id ||
        !n->has_scalar_result || !operand ||
        operand->kind != NL_CHECKED_IDENTIFIER || operand->type != w->u8 ||
        operand->symbol != w->formal || operand->next_argument ||
        !copy_if_gap(w->body->source, n->span.start_byte,
                     operand->span.start_byte, "usize(") ||
        !copy_if_gap(w->body->source, operand->span.end_byte, n->span.end_byte,
                     ")") ||
        !predicate_scalar(w, n->first_argument) ||
        !predicate_value_at(w->context, operand, &input) ||
        !nl_numeric_runtime_widening(NL_TYPE_U8, NL_TYPE_USIZE) ||
        (input.scalar_known ? input.scalar_value > UINT8_MAX
                            : input.scalar_value != 0) ||
        value.scalar_known != input.scalar_known ||
        value.scalar_value != input.scalar_value)
        return false;
    w->widened = true;
    return true;
}

static bool predicate_scalar(PredicateWalk *w, NLCheckedNodeId id)
{
    if (++w->depth > NL_SEMANTIC_MAX_DEPTH)
        return false;
    const NLCheckedNodeView *n = predicate_take(w, id);
    NLSemanticValueView value;
    bool valid = false;
    /* The readonly-u8 alternative is an actual formal/Copy chain, not the
     * phase predicate family with its type guard removed. */
    if ((w->parameter == w->u8 ||
         (w->incoming && n && n->type == w->boolean)) &&
        (!n ||
         (n->kind != NL_CHECKED_IDENTIFIER &&
          !(w->allow_widening && n->kind == NL_CHECKED_NUMERIC_CONVERSION))))
        goto done;
    if (n && n->kind == NL_CHECKED_IF) {
        valid = predicate_selection(w, id, n);
        goto done;
    }
    if (!predicate_value_at(w->context, n, &value) ||
        (n->type != w->quantity && n->type != w->boolean && n->type != w->u8))
        goto done;
    switch (n->kind) {
    case NL_CHECKED_NUMERIC_CONVERSION:
        valid = predicate_widening(w, id, n, value);
        break;
    case NL_CHECKED_U8_LITERAL:
        valid = n->type == w->u8 && copy_if_literal(w->body, n);
        break;
    case NL_CHECKED_USIZE_LITERAL: {
        char text[64];
        const int length =
            snprintf(text, sizeof(text), "usize(%zu)", value.scalar_value);
        valid = n->type == w->quantity && value.scalar_known &&
                n->has_scalar_result && length > 0 &&
                (size_t)length < sizeof(text) &&
                copy_if_gap(nl_checked_source(w->body), n->span.start_byte,
                            n->span.end_byte, text);
        break;
    }
    case NL_CHECKED_IDENTIFIER: {
        const NLSemanticContext *c = w->context;
        if (!n->symbol || n->symbol > c->binding_count ||
            n->symbol > NL_SEMANTIC_MAX_ENTRIES ||
            n->value_use != NL_VALUE_COPIED ||
            n->span.start_byte != n->name.start_byte ||
            n->span.end_byte != n->name.end_byte ||
            !predicate_name(w->body, n->name, c->bindings[n->symbol - 1].name))
            break;
        NLValueId origin;
        if ((n->symbol == w->formal && n->type == w->parameter) ||
            (w->incoming && n->symbol == w->incoming && n->type == w->u8))
            origin = c->bindings[n->symbol - 1].view.value;
        else {
            const NLCheckedFragment *declaration_owner =
                w->declarations ? w->declarations : w->body;
            const NLCheckedNodeView *declaration = nl_checked_node_view(
                declaration_owner, w->bindings[n->symbol - 1]);
            const NLCheckedNodeView *init =
                declaration ? nl_checked_node_view(declaration_owner,
                                                   declaration->initializer)
                            : NULL;
            const bool scalar_initializer =
                init && (init->kind == NL_CHECKED_IDENTIFIER ||
                         (w->allow_widening &&
                          init->kind == NL_CHECKED_NUMERIC_CONVERSION) ||
                         init->kind == NL_CHECKED_U8_LITERAL ||
                         init->kind == NL_CHECKED_USIZE_LITERAL ||
                         init->kind == NL_CHECKED_USIZE_EQUALITY ||
                         init->kind == NL_CHECKED_USIZE_LESS_THAN ||
                         init->kind == NL_CHECKED_IF);
            if (!declaration || declaration->kind != NL_CHECKED_BINDING ||
                declaration->symbol != n->symbol || declaration->terminates ||
                declaration->result_count ||
                declaration->type != nl_semantic_unit_type(c) ||
                !scalar_initializer || init->terminates ||
                init->result_count != 1 || init->type != n->type ||
                init->results[0].type != n->type ||
                nl_checked_source(declaration_owner) !=
                    nl_checked_source(w->body) ||
                declaration->span.end_byte != init->span.end_byte ||
                declaration->span.end_byte > n->span.start_byte ||
                (declaration_owner != w->body &&
                 (w->body->if_parent != declaration_owner ||
                  init->results[0].value > w->body->branch_value_prefix)))
                break;
            origin = init->results[0].value;
        }
        NLSemanticValueView original;
        valid = nl_semantic_value_view(c, origin, &original) &&
                copy_if_plain(original, n->type) &&
                value.scalar_known == original.scalar_known &&
                value.scalar_value == original.scalar_value;
        break;
    }
    case NL_CHECKED_USIZE_EQUALITY:
    case NL_CHECKED_USIZE_LESS_THAN: {
        const NLCheckedNodeView *left =
            nl_checked_node_view(w->body, n->first_argument);
        const NLCheckedNodeView *right =
            left ? nl_checked_node_view(w->body, left->next_argument) : NULL;
        const bool less = n->kind == NL_CHECKED_USIZE_LESS_THAN;
        NLSemanticValueView a, b;
        if (n->type != w->boolean || n->argument_count != 2 || !left ||
            !right || left->type != w->quantity || right->type != w->quantity ||
            right->next_argument || !n->has_scalar_result ||
            n->first_argument >= left->next_argument ||
            left->next_argument >= id ||
            n->span.start_byte != left->span.start_byte ||
            n->span.end_byte != right->span.end_byte ||
            left->span.end_byte > n->name.start_byte ||
            n->name.end_byte > right->span.start_byte ||
            !copy_if_gap(nl_checked_source(w->body), n->name.start_byte,
                         n->name.end_byte, less ? "<" : "==") ||
            !predicate_scalar(w, n->first_argument) ||
            !predicate_scalar(w, left->next_argument) ||
            !predicate_value_at(w->context, left, &a) ||
            !predicate_value_at(w->context, right, &b))
            break;
        const bool known = a.scalar_known && b.scalar_known;
        valid = value.scalar_known == known &&
                value.scalar_value ==
                    (known ? (less ? a.scalar_value < b.scalar_value
                                   : a.scalar_value == b.scalar_value)
                           : 0);
        break;
    }
    default:
        break;
    }
done:
    --w->depth;
    return valid;
}

static bool predicate_binding_shape(PredicateWalk *w,
                                    const NLCheckedNodeView *n, NLTypeId unit)
{
    const NLCheckedFragment *f = w->body;
    const NLSemanticContext *c = w->context;
    const NLCheckedNodeView *receiver = predicate_take(w, n->first_argument);
    const NLCheckedNodeView *init = nl_checked_node_view(f, n->initializer);
    if (!receiver || !init || init->result_count != 1 ||
        receiver->kind != NL_CHECKED_RECEIVER || receiver->next_argument ||
        n->argument_count != 1 || !n->symbol || n->symbol > c->binding_count ||
        n->symbol > NL_SEMANTIC_MAX_ENTRIES || n->symbol == w->formal ||
        receiver->symbol != n->symbol || n->result_count || n->type != unit ||
        n->terminates || receiver->type != init->type ||
        w->bindings[n->symbol - 1] ||
        !predicate_name(f, receiver->name, c->bindings[n->symbol - 1].name) ||
        c->bindings[n->symbol - 1].view.value != init->results[0].value ||
        !copy_if_gap(nl_checked_source(f), n->span.start_byte,
                     receiver->name.start_byte, "let") ||
        !copy_if_gap(nl_checked_source(f), receiver->name.end_byte,
                     init->span.start_byte, "=") ||
        n->span.end_byte != init->span.end_byte)
        return false;
    return true;
}

static bool predicate_binding(PredicateWalk *w, NLCheckedNodeId id,
                              const NLCheckedNodeView *n, NLTypeId unit)
{
    if (!predicate_binding_shape(w, n, unit) ||
        !predicate_scalar(w, n->initializer))
        return false;
    w->bindings[n->symbol - 1] = id;
    return true;
}

static bool predicate_discard(PredicateWalk *w, const NLCheckedNodeView *n,
                              NLTypeId unit)
{
    const NLCheckedNodeView *value =
        nl_checked_node_view(w->body, n->initializer);
    /* Generic statement lowering has unit RETURNs. Discarding a typed IF
     * with an exit needs its separate binding/tail consumer. */
    return value && (value->kind != NL_CHECKED_IF || value->normal_arms == 2) &&
           value->type == w->u8 && n->type == unit && !n->result_count &&
           !n->terminates && n->span.start_byte == value->span.start_byte &&
           copy_if_gap(w->body->source, value->span.end_byte, n->span.end_byte,
                       ";") &&
           predicate_scalar(w, n->initializer);
}

static bool predicate_unit_value(PredicateWalk *w, NLCheckedNodeId id,
                                 NLTypeId unit)
{
    const NLCheckedNodeView *value = predicate_take(w, id);
    return value && value->kind == NL_CHECKED_UNIT && value->type == unit &&
           !value->terminates && !value->result_count &&
           copy_if_gap(w->body->source, value->span.start_byte,
                       value->span.end_byte, "unit");
}

static inline bool nl_pure_u8_unit_function(const NLSemanticContext *c,
                                            const NLFunctionEntry *fn)
{
    return fn && fn->kind == NL_CHECKED_REGISTERED_CALL && fn->count == 1 &&
           fn->parameters &&
           fn->parameters[0] == nl_semantic_core_type(c, NL_TYPE_U8) &&
           fn->result == nl_semantic_unit_type(c) && fn->body &&
           fn->body->syntax && !fn->caller_effects &&
           !fn->hidden_dependencies && !fn->owner_receiver &&
           !fn->owner_producer && !fn->custody_recipient;
}

/* Use the ordinary scalar Copy walk on an exact non-owning body. A unit
 * return has the canonical exit slot but carries no caller result value. */
static inline bool nl_pure_u8_unit_body_at(const NLCheckedFragment *f,
                                           NLSymbolId formal,
                                           const NLSemanticContext *c)
{
    const NLTypeId unit = nl_semantic_unit_type(c);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    if (!f || !formal || !u8 || f->body_count || f->arm_count ||
        nl_checked_node_count(f) > NL_PARSER_MAX_NODES)
        return false;
    PredicateWalk w = {
        .body = f, .context = c, .formal = formal, .u8 = u8, .parameter = u8};
    const NLCheckedNodeView *root = predicate_take(&w, nl_checked_root(f));
    if (!root || root->kind != NL_CHECKED_BLOCK || root->result_count)
        return false;
    size_t items = 0;
    const NLCheckedNodeView *ret = NULL;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = predicate_take(&w, id);
        if (!n || ++items > root->item_count)
            return false;
        if (n->kind == NL_CHECKED_BINDING) {
            if (!predicate_binding(&w, id, n, unit))
                return false;
        } else if (n->kind == NL_CHECKED_STATEMENT) {
            if (!predicate_discard(&w, n, unit))
                return false;
        } else if (n->kind == NL_CHECKED_RETURN) {
            if (n->next_item || !predicate_unit_value(&w, n->initializer, unit))
                return false;
            ret = n;
        } else
            return false;
        id = n->next_item;
    }
    if (items != root->item_count)
        return false;
    const size_t exits = nl_control_exits_count(nl_checked_control_exits(f));
    if (ret) {
        if (!root->terminates || root->tail || root->returned.type != unit ||
            root->returned.value || exits != 1 ||
            !predicate_return(f, ret, 0, f->function_target))
            return false;
    } else if (root->terminates || root->type != unit || exits ||
               (root->tail && !predicate_unit_value(&w, root->tail, unit)))
        return false;
    for (size_t i = 0; i < nl_checked_node_count(f); ++i)
        if (!w.seen[i])
            return false;
    return true;
}

static bool nl_scalar_formals_at(const NLCheckedFragment *f, NLSymbolId formal,
                                 NLTypeId unit, NLTypeId u8, NLTypeId result,
                                 const NLSemanticContext *c, bool readonly_u8,
                                 NLTypeId parameter, NLSymbolId incoming,
                                 bool guarded_narrowing, NLPureU8Plan *plan)
{
    PredicateWalk w = {.body = f,
                       .context = c,
                       .formal = formal,
                       .incoming = incoming,
                       .quantity = nl_semantic_core_type(c, NL_TYPE_USIZE),
                       .boolean = nl_semantic_core_type(c, NL_TYPE_BOOL),
                       .u8 = u8,
                       .parameter = parameter,
                       .guarded_narrowing = guarded_narrowing,
                       .allow_widening = readonly_u8 && result != u8};
    const NLCheckedNodeView *root = predicate_take(&w, nl_checked_root(f));
    if (!root || root->kind != NL_CHECKED_BLOCK ||
        nl_checked_node_count(f) > NL_PARSER_MAX_NODES)
        return false;
    size_t items = 0;
    NLCheckedNodeId returned = 0, final_return = 0;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = predicate_take(&w, id);
        if (!n || ++items > nl_checked_node_count(f))
            return false;
        if (n->kind == NL_CHECKED_BINDING) {
            if (!predicate_binding(&w, id, n, unit))
                return false;
        } else if (n->kind == NL_CHECKED_STATEMENT && !readonly_u8 &&
                   !incoming) {
            if (!predicate_discard(&w, n, unit))
                return false;
        } else if (n->kind == NL_CHECKED_RETURN && !readonly_u8 && !incoming) {
            const NLCheckedNodeView *value =
                nl_checked_node_view(f, n->initializer);
            if (n->next_item || !n->terminates || !value || value->type != u8 ||
                n->returned.type != u8 ||
                n->returned.value != value->results[0].value ||
                !copy_if_gap(nl_checked_source(f), n->span.start_byte,
                             value->span.start_byte, "return") ||
                !predicate_scalar(&w, n->initializer))
                return false;
            returned = n->initializer;
            final_return = id;
        } else
            return false;
        id = n->next_item;
    }
    if (items != root->item_count)
        return false;
    const NLCheckedNodeView *tail = NULL;
    if (!returned) {
        tail = nl_checked_node_view(f, root->tail);
        if (!tail || !predicate_scalar(&w, root->tail))
            return false;
    }
    if (readonly_u8) {
        if (w.selections || f->arm_count || f->body_count ||
            nl_control_exits_count(f->exits) || root->terminates ||
            w.allow_widening != w.widened || root->type != result ||
            tail->type != result || root->result_count != 1 ||
            root->results[0].type != result ||
            root->results[0].value != tail->results[0].value)
            return false;
        for (size_t i = 0; i < nl_checked_node_count(f); ++i)
            if (!w.seen[i])
                return false;
        if (plan)
            *plan = (NLPureU8Plan){0, 0, 2, 0};
        return true;
    }
    const NLCheckedNodeView *choice = nl_checked_node_view(f, w.selection);
    if (!choice || w.selections != 1 || choice->normal_arms > 2 ||
        (incoming && (choice->normal_arms != 2 || f->body_count ||
                      f->arm_count != 2 || root->terminates)))
        return false;
    const size_t early = 2 - choice->normal_arms;
    const size_t returns = early + (size_t)(final_return != 0);
    const NLControlExits *exits = nl_checked_control_exits(f);
    if (nl_control_exits_count(exits) != returns ||
        (returns && !nl_control_exits_only_return(exits, f->function_target)))
        return false;
    if (returned) {
        const NLCheckedNodeView *value = nl_checked_node_view(f, returned);
        const NLCheckedNodeView *ret = nl_checked_node_view(f, final_return);
        if (!choice->normal_arms || !value || !root->terminates || root->tail ||
            root->returned.type != u8 ||
            (!early && root->returned.value != value->results[0].value) ||
            !predicate_return(f, ret, early, f->function_target))
            return false;
    } else {
        /* A zero-normal IF cannot supply a binding/continuation. One normal
         * arm already has CHECK's source-matched replay, so its pure result
         * may flow through ordinary bindings/Copy/discard nodes above. */
        if (!choice->normal_arms && root->tail != w.selection)
            return false;
        if (!early) {
            if (root->terminates || tail->type != u8 ||
                root->result_count != 1 || root->results[0].type != u8 ||
                root->results[0].value != tail->results[0].value)
                return false;
        } else if (root->terminates != tail->terminates ||
                   root->result_count != (size_t)!root->terminates)
            return false;
    }
    if (early) {
        /* Core joins all real function exits into a fresh Unknown summary.
         * It is distinct from the sole-normal operand and from each RETURN
         * package, which the current consumer checks in its own exit world. */
        const NLCheckedResult summary =
            root->terminates ? root->returned : root->results[0];
        NLSemanticValueView v;
        if (summary.type != u8 ||
            !nl_semantic_value_view(c, summary.value, &v) ||
            !copy_if_plain(v, u8) || v.scalar_known || v.scalar_value)
            return false;
    }
    for (size_t i = 0; i < nl_checked_node_count(f); ++i)
        if (!w.seen[i])
            return false;
    if (plan) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, w.selection, 0);
        const NLCheckedNodeView *a = nl_checked_node_view(arm, arm->root);
        *plan = (NLPureU8Plan){
            w.selection, final_return,
            choice->normal_arms == 1 ? (size_t)a->terminates : 2, returns};
    }
    return true;
}

static bool nl_scalar_u8_body_at(const NLCheckedFragment *f, NLSymbolId formal,
                                 NLTypeId unit, NLTypeId u8,
                                 const NLSemanticContext *c, bool readonly_u8,
                                 bool guarded_narrowing, NLPureU8Plan *plan)
{
    return nl_scalar_formals_at(
        f, formal, unit, u8, u8, c, readonly_u8,
        readonly_u8 ? u8 : nl_semantic_core_type(c, NL_TYPE_USIZE), 0,
        guarded_narrowing, plan);
}

static inline bool nl_pure_widening_body_at(const NLCheckedFragment *f,
                                            NLSymbolId formal,
                                            const NLSemanticContext *c)
{
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    return nl_scalar_formals_at(f, formal, nl_semantic_unit_type(c), u8,
                                nl_semantic_core_type(c, NL_TYPE_USIZE), c,
                                true, u8, 0, false, NULL);
}

/* The ordinary scalar emitter handles the guarded conversion tail. */
static inline bool nl_pure_u8_body(const NLCheckedFragment *f,
                                   NLSymbolId formal, NLTypeId unit,
                                   NLTypeId u8, NLPureU8Plan *plan)
{
    return nl_scalar_u8_body_at(f, formal, unit, u8, nl_checked_context(f),
                                false, true, plan);
}

typedef struct {
    const NLCheckedFragment *body;
    NLSymbolId formal;
    NLCheckedNodeId selection;
    size_t function;
    NLPureU8Plan plan;
} NLPureU8Call;

/* An actual CHECK-owned synchronous call, not a host-supplied summary.
 * Caller placement/quantity-expression policy belongs to each consumer. */
/* CHECK may read the genuine precommit transaction; published consumers use
 * the wrapper below and the current public caller. This changes no child
 * context/ownership and never turns a shallow foreign-world view into proof. */
static NLSymbolId nl_scalar_invocation_formal_at(const NLCheckedFragment *f,
                                                 NLCheckedNodeId id,
                                                 const NLSemanticContext *c,
                                                 NLTypeId parameter,
                                                 NLTypeId result_type)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (!c || !c->functions || !call || !body ||
        body->context != nl_checked_context(f) ||
        call->kind != NL_CHECKED_REGISTERED_CALL || !call->function ||
        call->function > c->function_count || call->argument_count != 1 ||
        call->result_count != 1)
        return 0;
    const NLFunctionEntry *fn = &c->functions[call->function - 1];
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f, call->first_argument);
    if (call->type != result_type || call->results[0].type != result_type ||
        fn->kind != NL_CHECKED_REGISTERED_CALL || fn->count != 1 ||
        !fn->parameters || fn->parameters[0] != parameter ||
        fn->result != result_type || !fn->body || fn->body->count != 1 ||
        !fn->body->parameter_names[0] || !fn->body->syntax ||
        body->body_owner != fn->body || body->source != fn->body->source ||
        nl_syntax_tree_source(fn->body->syntax) != fn->body->source ||
        fn->caller_effects || fn->hidden_dependencies || fn->owner_receiver ||
        fn->owner_producer || fn->custody_recipient || !arg ||
        arg->next_argument || arg->result_count != 1 ||
        arg->type != parameter || arg->parameter_type != parameter ||
        !predicate_name(f, call->name, fn->name))
        return 0;
    NLSymbolId formal = 0;
    for (size_t i = 0; i < c->binding_count; ++i) {
        const NLBindingEntry *b = &c->bindings[i];
        if (b->hidden && b->view.type == parameter &&
            b->view.value == arg->results[0].value && b->name &&
            !strcmp(b->name, fn->body->parameter_names[0])) {
            if (formal)
                return 0;
            formal = i + 1;
        }
    }
    const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
    if (!formal || !root)
        return 0;
    const NLCheckedResult result =
        root->terminates ? root->returned : root->results[0];
    if (result.type != call->results[0].type ||
        result.value != call->results[0].value)
        return 0;
    return formal;
}

static bool nl_scalar_u8_invocation_at(const NLCheckedFragment *f,
                                       NLCheckedNodeId id,
                                       const NLSemanticContext *c,
                                       bool readonly_u8, NLPureU8Call *out)
{
    if (!c)
        return false;
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    const NLTypeId parameter =
        readonly_u8 ? u8 : nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLSymbolId formal =
        nl_scalar_invocation_formal_at(f, id, c, parameter, u8);
    if (!formal)
        return false;
    *out = (NLPureU8Call){.body = nl_checked_call_body(f, id),
                          .formal = formal,
                          .function = nl_checked_node_view(f, id)->function};
    return true;
}

static bool nl_scalar_u8_contents_at(const NLCheckedFragment *f,
                                     NLCheckedNodeId id,
                                     const NLSemanticContext *c,
                                     bool readonly_u8, bool guarded_narrowing,
                                     NLPureU8Call *out)
{
    NLPureU8Call call;
    if (!nl_scalar_u8_invocation_at(f, id, c, readonly_u8, &call) ||
        !nl_scalar_u8_body_at(call.body, call.formal, nl_semantic_unit_type(c),
                              nl_semantic_core_type(c, NL_TYPE_U8), c,
                              readonly_u8, guarded_narrowing, &call.plan))
        return false;
    call.selection = call.plan.selection;
    *out = call;
    return true;
}

static inline bool nl_pure_u8_contents_at(const NLCheckedFragment *f,
                                          NLCheckedNodeId id,
                                          const NLSemanticContext *c,
                                          NLPureU8Call *out)
{
    return nl_scalar_u8_contents_at(f, id, c, false, false, out);
}

/* Both byte writers use this exact finite threshold policy. The real child
 * result seal is required during entry checking as well as publication. */
static inline bool nl_pure_u8_threshold_at(const NLCheckedFragment *f,
                                           const NLSemanticContext *c,
                                           NLCheckedNodeId id,
                                           NLPureU8Call *pure)
{
    if (!nl_h_custody_result_valid(f, id) ||
        !nl_pure_u8_contents_at(f, id, c, pure))
        return false;
    const NLCheckedNodeView *root =
        nl_checked_node_view(pure->body, pure->body->root);
    const NLCheckedNodeView *choice =
        nl_checked_node_view(pure->body, pure->selection);
    if (!root || !choice || choice->normal_arms != 2 || root->item_count ||
        root->tail != pure->selection || pure->body->body_count ||
        pure->body->arm_count != 2)
        return false;
    const NLCheckedNodeView *test =
        nl_checked_node_view(pure->body, choice->initializer);
    const NLCheckedNodeView *left =
        test ? nl_checked_node_view(pure->body, test->first_argument) : NULL;
    const NLCheckedNodeView *right =
        left ? nl_checked_node_view(pure->body, left->next_argument) : NULL;
    return test && test->kind == NL_CHECKED_USIZE_LESS_THAN && right &&
           left->kind == NL_CHECKED_IDENTIFIER &&
           left->symbol == pure->formal &&
           right->kind == NL_CHECKED_USIZE_LITERAL;
}

typedef struct {
    NLPureU8Call threshold, wrapper;
    NLCheckedNodeId delegate;
} NLPureU8WriterPolicy;

/* A writer may use the direct threshold or a single pure tail delegation.
 * Both registered calls have their existing result seals. The leaf is
 * validated by the direct threshold predicate. */
static inline bool nl_pure_u8_writer_policy_at(const NLCheckedFragment *f,
                                               const NLSemanticContext *c,
                                               NLCheckedNodeId id,
                                               NLPureU8WriterPolicy *out)
{
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (!body)
        return false;
    NLPureU8WriterPolicy policy = {0};
    if (!body->body_count) {
        if (!nl_pure_u8_threshold_at(f, c, id, &policy.threshold))
            return false;
        *out = policy;
        return true;
    }
    if (body->body_count != 1 || !body->body_calls || body->arm_count ||
        nl_control_exits_count(body->exits) ||
        !nl_h_custody_result_valid(f, id) ||
        !nl_scalar_u8_invocation_at(f, id, c, false, &policy.wrapper))
        return false;

    policy.delegate = body->body_calls[0];
    const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
    const NLCheckedNodeView *delegate =
        nl_checked_node_view(body, policy.delegate);
    const NLCheckedNodeView *actual =
        delegate ? nl_checked_node_view(body, delegate->first_argument) : NULL;
    const NLSyntaxView *block =
        nl_syntax_node_view(nl_syntax_tree_root(body->body_owner->syntax));
    const NLSyntaxView *tail =
        block && block->kind == NL_SYNTAX_BLOCK && !block->data.block.items
            ? nl_syntax_node_view(block->data.block.tail)
            : NULL;
    const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    PredicateWalk walk = {.body = body,
                          .context = c,
                          .formal = policy.wrapper.formal,
                          .quantity = usize,
                          .u8 = u8,
                          .parameter = usize};
    if (!root || root->kind != NL_CHECKED_BLOCK || root->terminates ||
        root->first_item || root->item_count || root->tail != policy.delegate ||
        root->type != u8 || root->result_count != 1 || !delegate || !tail ||
        tail->kind != NL_SYNTAX_EXPR_CALL ||
        tail->span.start_byte != delegate->span.start_byte ||
        tail->span.end_byte != delegate->span.end_byte ||
        root->results[0].value != delegate->results[0].value || !actual ||
        actual->kind != NL_CHECKED_IDENTIFIER ||
        actual->symbol != policy.wrapper.formal ||
        !predicate_scalar(&walk, delegate->first_argument) ||
        !nl_pure_u8_threshold_at(body, c, policy.delegate, &policy.threshold))
        return false;
    *out = policy;
    return true;
}

/* Same owned graph walk and finished-result seal, distinct operand contract.
 * Current consumers authenticate their signature allocations before this
 * read, then prove the u8 actual comes from the exact current FIELD_READ. */
static inline bool nl_readonly_u8_contents(const NLCheckedFragment *f,
                                           NLCheckedNodeId id,
                                           NLPureU8Call *out)
{
    return nl_scalar_u8_contents_at(f, id, nl_checked_context(f), true, false,
                                    out);
}

static inline bool nl_pure_u8_contents(const NLCheckedFragment *f,
                                       NLCheckedNodeId id, NLPureU8Call *out)
{
    return nl_pure_u8_contents_at(f, id, nl_checked_context(f), out);
}

/* An extension of the same owned scalar walk, not a new result issuer.
 * Public consumers require the mature whole-world bytes before touching
 * current signatures or formal/Copy metadata. */
static inline bool
nl_readonly_selection_contents(const NLCheckedFragment *origin,
                               const NLCheckedFragment *f, NLCheckedNodeId id,
                               const NLCheckedFragment **body_out,
                               NLSymbolId formals[2])
{
    /* The actual enclosing original-H invocation freezes every owned child
     * at final completion. A nested scalar receipt need not freeze its
     * caller at the earlier call point. Authenticate that real root first. */
    if (!origin || !f ||
        nl_checked_proof_class(origin) != NL_CHECKED_MACHINE_PROVED ||
        !nl_h_custody_result_world_present(origin, nl_checked_root(origin)) ||
        !nl_h_origin_validate(origin) || origin->h_origin != f->h_origin ||
        !nl_h_custody_result_valid(f, id))
        return false;
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (!c || !c->functions || !call || !body || body->context != c ||
        call->kind != NL_CHECKED_REGISTERED_CALL || !call->function ||
        call->function > c->function_count || call->argument_count != 2 ||
        call->result_count != 1)
        return false;
    const NLTypeId types[2] = {nl_semantic_core_type(c, NL_TYPE_BOOL),
                               nl_semantic_core_type(c, NL_TYPE_U8)};
    const NLFunctionEntry *fn = &c->functions[call->function - 1];
    if (call->type != types[1] || fn->kind != NL_CHECKED_REGISTERED_CALL ||
        fn->count != 2 || !fn->parameters || fn->parameters[0] != types[0] ||
        fn->parameters[1] != types[1] || fn->result != types[1] || !fn->body ||
        fn->body->count != 2 || !fn->body->syntax ||
        body->body_owner != fn->body || body->source != fn->body->source ||
        nl_syntax_tree_source(fn->body->syntax) != body->source ||
        fn->caller_effects || fn->hidden_dependencies || fn->owner_receiver ||
        fn->owner_producer || fn->custody_recipient ||
        !predicate_name(f, call->name, fn->name))
        return false;
    NLCheckedNodeId argument = call->first_argument;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *arg = nl_checked_node_view(f, argument);
        NLSemanticValueView value;
        if (!arg || !fn->body->parameter_names[i] || arg->result_count != 1 ||
            arg->type != types[i] || arg->parameter_type != types[i] ||
            !predicate_value(f, arg, &value) || value.owner_place)
            return false;
        formals[i] = 0;
        for (size_t b = 0; b < c->binding_count; ++b) {
            const NLBindingEntry *entry = &c->bindings[b];
            if (entry->hidden && entry->view.type == types[i] &&
                entry->view.value == arg->results[0].value && entry->name &&
                !strcmp(entry->name, fn->body->parameter_names[i])) {
                if (formals[i])
                    return false;
                formals[i] = b + 1;
            }
        }
        if (!formals[i])
            return false;
        argument = arg->next_argument;
    }
    NLPureU8Plan plan;
    const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
    if (argument || !root || root->type != types[1] ||
        root->result_count != 1 ||
        root->results[0].type != call->results[0].type ||
        root->results[0].value != call->results[0].value ||
        !nl_scalar_formals_at(body, formals[0], nl_semantic_unit_type(c),
                              types[1], types[1], c, false, types[0],
                              formals[1], false, &plan))
        return false;
    *body_out = body;
    return true;
}

/* Published consumers require the original finished-result receipt. The
 * contents walk is also used by CHECK on its own unpublished definition
 * prefix, where source_call deliberately has not issued that receipt. */
static inline bool nl_pure_u8_call_mode(const NLCheckedFragment *f,
                                        NLCheckedNodeId id,
                                        bool guarded_narrowing,
                                        NLPureU8Call *out)
{
    if (!nl_h_custody_result_valid(f, id) ||
        !nl_scalar_u8_contents_at(f, id, nl_checked_context(f), false,
                                  guarded_narrowing, out))
        return false;
    const NLCheckedNodeView *choice =
        nl_checked_node_view(out->body, out->selection);
    return choice && choice->normal_arms == 2;
}

static inline bool nl_pure_u8_call(const NLCheckedFragment *f,
                                   NLCheckedNodeId id, NLPureU8Call *out)
{
    return nl_pure_u8_call_mode(f, id, false, out);
}

#endif
