#ifndef NEWLANG_PURE_U8_INTERNAL_H
#define NEWLANG_PURE_U8_INTERNAL_H

#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "scalar_view_internal.h"

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
    NLSymbolId formal;
    NLTypeId quantity, boolean, u8, parameter;
    NLCheckedNodeId bindings[NL_SEMANTIC_MAX_ENTRIES];
    bool seen[NL_PARSER_MAX_NODES];
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
        block->kind != NL_CHECKED_BLOCK || tail->type != w->u8 ||
        !copy_if_literal(arm, tail))
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
    return nl_checked_node_count(arm) == 3 && !block->terminates &&
           !block->first_item && !block->item_count && root->type == w->u8 &&
           block->type == w->u8 && root->result_count == 1 &&
           block->result_count == 1 && root->results[0].type == w->u8 &&
           block->results[0].type == w->u8 &&
           root->results[0].value == tail->results[0].value &&
           block->results[0].value == tail->results[0].value &&
           !nl_control_exits_count(arm->exits) &&
           copy_if_gap(arm->source, block->span.start_byte,
                       tail->span.start_byte, "{") &&
           copy_if_gap(arm->source, tail->span.end_byte, block->span.end_byte,
                       "}");
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

static bool predicate_scalar(PredicateWalk *w, NLCheckedNodeId id)
{
    if (++w->depth > NL_SEMANTIC_MAX_DEPTH)
        return false;
    const NLCheckedNodeView *n = predicate_take(w, id);
    NLSemanticValueView value;
    bool valid = false;
    /* The readonly-u8 alternative is an actual formal/Copy chain, not the
     * phase predicate family with its type guard removed. */
    if (w->parameter == w->u8 && (!n || n->kind != NL_CHECKED_IDENTIFIER))
        goto done;
    if (n && n->kind == NL_CHECKED_IF) {
        valid = predicate_selection(w, id, n);
        goto done;
    }
    if (!predicate_value_at(w->context, n, &value) ||
        (n->type != w->quantity && n->type != w->boolean && n->type != w->u8))
        goto done;
    switch (n->kind) {
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
        if (n->symbol == w->formal && n->type == w->parameter)
            origin = c->bindings[n->symbol - 1].view.value;
        else {
            const NLCheckedNodeView *declaration =
                nl_checked_node_view(w->body, w->bindings[n->symbol - 1]);
            const NLCheckedNodeView *init =
                declaration
                    ? nl_checked_node_view(w->body, declaration->initializer)
                    : NULL;
            if (!init || init->type != n->type ||
                declaration->span.end_byte > n->span.start_byte)
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

static bool nl_scalar_u8_body_at(const NLCheckedFragment *f, NLSymbolId formal,
                                 NLTypeId unit, NLTypeId u8,
                                 const NLSemanticContext *c, bool readonly_u8,
                                 NLPureU8Plan *plan)
{
    PredicateWalk w = {
        .body = f,
        .context = c,
        .formal = formal,
        .quantity = nl_semantic_core_type(c, NL_TYPE_USIZE),
        .boolean = nl_semantic_core_type(c, NL_TYPE_BOOL),
        .u8 = u8,
        .parameter =
            readonly_u8 ? u8 : nl_semantic_core_type(c, NL_TYPE_USIZE)};
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
            const NLCheckedNodeView *receiver =
                predicate_take(&w, n->first_argument);
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, n->initializer);
            if (!receiver || !init || init->result_count != 1 ||
                receiver->kind != NL_CHECKED_RECEIVER ||
                receiver->next_argument || n->argument_count != 1 ||
                !n->symbol || n->symbol > c->binding_count ||
                n->symbol > NL_SEMANTIC_MAX_ENTRIES || n->symbol == formal ||
                receiver->symbol != n->symbol || n->result_count ||
                n->type != unit || n->terminates ||
                receiver->type != init->type || w.bindings[n->symbol - 1] ||
                !predicate_name(f, receiver->name,
                                c->bindings[n->symbol - 1].name) ||
                c->bindings[n->symbol - 1].view.value !=
                    init->results[0].value ||
                !copy_if_gap(nl_checked_source(f), n->span.start_byte,
                             receiver->name.start_byte, "let") ||
                !copy_if_gap(nl_checked_source(f), receiver->name.end_byte,
                             init->span.start_byte, "=") ||
                n->span.end_byte != init->span.end_byte ||
                !predicate_scalar(&w, n->initializer))
                return false;
            w.bindings[n->symbol - 1] = id;
        } else if (n->kind == NL_CHECKED_STATEMENT && !readonly_u8) {
            const NLCheckedNodeView *value =
                nl_checked_node_view(f, n->initializer);
            /* Generic statement lowering has unit RETURNs. A discarded IF
             * with typed exits needs binding/tail lowering, not that path. */
            if (!value ||
                (value->kind == NL_CHECKED_IF && value->normal_arms != 2) ||
                value->type != u8 || n->type != unit || n->result_count ||
                n->terminates || n->span.start_byte != value->span.start_byte ||
                !copy_if_gap(f->source, value->span.end_byte, n->span.end_byte,
                             ";") ||
                !predicate_scalar(&w, n->initializer))
                return false;
        } else if (n->kind == NL_CHECKED_RETURN && !readonly_u8) {
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
            root->type != u8 || root->result_count != 1 ||
            root->results[0].type != u8 ||
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
    if (!choice || w.selections != 1 || choice->normal_arms > 2)
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

static inline bool nl_pure_u8_body_at(const NLCheckedFragment *f,
                                      NLSymbolId formal, NLTypeId unit,
                                      NLTypeId u8, const NLSemanticContext *c,
                                      NLPureU8Plan *plan)
{
    return nl_scalar_u8_body_at(f, formal, unit, u8, c, false, plan);
}

static inline bool nl_pure_u8_body(const NLCheckedFragment *f,
                                   NLSymbolId formal, NLTypeId unit,
                                   NLTypeId u8, NLPureU8Plan *plan)
{
    return nl_pure_u8_body_at(f, formal, unit, u8, nl_checked_context(f), plan);
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
static bool nl_scalar_u8_contents_at(const NLCheckedFragment *f,
                                     NLCheckedNodeId id,
                                     const NLSemanticContext *c,
                                     bool readonly_u8, NLPureU8Call *out)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (!c || !c->functions || !call || !body ||
        body->context != nl_checked_context(f) ||
        call->kind != NL_CHECKED_REGISTERED_CALL || !call->function ||
        call->function > c->function_count || call->argument_count != 1 ||
        call->result_count != 1)
        return false;
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8),
                   quantity = nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLTypeId parameter = readonly_u8 ? u8 : quantity;
    const NLFunctionEntry *fn = &c->functions[call->function - 1];
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f, call->first_argument);
    if (call->type != u8 || call->results[0].type != u8 ||
        fn->kind != NL_CHECKED_REGISTERED_CALL || fn->count != 1 ||
        !fn->parameters || fn->parameters[0] != parameter || fn->result != u8 ||
        !fn->body || fn->body->count != 1 || !fn->body->parameter_names[0] ||
        !fn->body->syntax || body->body_owner != fn->body ||
        body->source != fn->body->source ||
        nl_syntax_tree_source(fn->body->syntax) != fn->body->source ||
        fn->caller_effects || fn->hidden_dependencies || fn->owner_receiver ||
        fn->owner_producer || fn->custody_recipient || !arg ||
        arg->next_argument || arg->result_count != 1 ||
        arg->type != parameter || arg->parameter_type != parameter ||
        !predicate_name(f, call->name, fn->name))
        return false;
    NLSymbolId formal = 0;
    for (size_t i = 0; i < c->binding_count; ++i) {
        const NLBindingEntry *b = &c->bindings[i];
        if (b->hidden && b->view.type == parameter &&
            b->view.value == arg->results[0].value && b->name &&
            !strcmp(b->name, fn->body->parameter_names[0])) {
            if (formal)
                return false;
            formal = i + 1;
        }
    }
    NLPureU8Plan plan;
    if (!formal || !nl_scalar_u8_body_at(body, formal, nl_semantic_unit_type(c),
                                         u8, c, readonly_u8, &plan))
        return false;
    const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
    const NLCheckedResult result =
        root->terminates ? root->returned : root->results[0];
    if (result.type != call->results[0].type ||
        result.value != call->results[0].value)
        return false;
    *out = (NLPureU8Call){body, formal, plan.selection, call->function, plan};
    return true;
}

static inline bool nl_pure_u8_contents_at(const NLCheckedFragment *f,
                                          NLCheckedNodeId id,
                                          const NLSemanticContext *c,
                                          NLPureU8Call *out)
{
    return nl_scalar_u8_contents_at(f, id, c, false, out);
}

/* Same owned graph walk and finished-result seal, distinct operand contract.
 * Current consumers authenticate their signature allocations before this
 * read, then prove the u8 actual comes from the exact current FIELD_READ. */
static inline bool nl_readonly_u8_contents(const NLCheckedFragment *f,
                                           NLCheckedNodeId id,
                                           NLPureU8Call *out)
{
    return nl_scalar_u8_contents_at(f, id, nl_checked_context(f), true, out);
}

static bool nl_pure_u8_contents(const NLCheckedFragment *f, NLCheckedNodeId id,
                                NLPureU8Call *out)
{
    return nl_pure_u8_contents_at(f, id, nl_checked_context(f), out);
}

/* Published consumers require the original finished-result receipt. The
 * contents walk is also used by CHECK on its own unpublished definition
 * prefix, where source_call deliberately has not issued that receipt. */
static inline bool nl_pure_u8_call(const NLCheckedFragment *f,
                                   NLCheckedNodeId id, NLPureU8Call *out)
{
    if (!nl_h_custody_result_valid(f, id) || !nl_pure_u8_contents(f, id, out))
        return false;
    const NLCheckedNodeView *choice =
        nl_checked_node_view(out->body, out->selection);
    return choice && choice->normal_arms == 2;
}

#endif
