#ifndef NEWLANG_PURE_U8_INTERNAL_H
#define NEWLANG_PURE_U8_INTERNAL_H

#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "scalar_view_internal.h"

/* Shared private numeric consumer. The existing source checker establishes
 * both IF arms; this walk admits only pure scalar syntax in the actual owned
 * callable. It imports no arm value/current/owner into the caller. */
typedef struct {
    const NLCheckedFragment *body;
    NLSymbolId formal;
    NLTypeId quantity, boolean, u8;
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

static bool predicate_value(const NLCheckedFragment *f,
                            const NLCheckedNodeView *n, NLSemanticValueView *v)
{
    return n && !n->terminates && n->result_count == 1 &&
           n->results[0].type == n->type &&
           nl_semantic_value_view(nl_checked_context(f), n->results[0].value,
                                  v) &&
           copy_if_plain(*v, n->type) &&
           (!n->has_scalar_result ||
            (n->scalar_result.type == n->type &&
             n->scalar_result.known == v->scalar_known &&
             n->scalar_result.value == v->scalar_value));
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

static bool predicate_arm(PredicateWalk *w, NLCheckedNodeId id, size_t index)
{
    const NLCheckedFragment *arm = nl_checked_if_arm(w->body, id, index);
    if (!arm || !copy_if_arm_origin(w->body, id, index, arm) ||
        nl_checked_node_count(arm) != 3)
        return false;
    const NLCheckedNodeView *root =
        nl_checked_node_view(arm, nl_checked_root(arm));
    const NLCheckedNodeView *block =
        root ? nl_checked_node_view(arm, root->initializer) : NULL;
    const NLCheckedNodeView *tail =
        block ? nl_checked_node_view(arm, block->tail) : NULL;
    return root && block && tail && root != block && root != tail &&
           block != tail && root->kind == NL_CHECKED_IF_ARM &&
           block->kind == NL_CHECKED_BLOCK && !root->terminates &&
           !block->terminates && !block->first_item && !block->item_count &&
           root->type == w->u8 && block->type == w->u8 &&
           root->result_count == 1 && block->result_count == 1 &&
           root->results[0].type == w->u8 && block->results[0].type == w->u8 &&
           root->results[0].value == tail->results[0].value &&
           block->results[0].value == tail->results[0].value &&
           tail->type == w->u8 && copy_if_literal(arm, tail) &&
           !nl_control_exits_count(nl_checked_control_exits(arm)) &&
           copy_if_gap(nl_checked_source(arm), block->span.start_byte,
                       tail->span.start_byte, "{") &&
           copy_if_gap(nl_checked_source(arm), tail->span.end_byte,
                       block->span.end_byte, "}");
}

static bool predicate_scalar(PredicateWalk *w, NLCheckedNodeId id)
{
    if (++w->depth > NL_SEMANTIC_MAX_DEPTH)
        return false;
    const NLCheckedNodeView *n = predicate_take(w, id);
    NLSemanticValueView value;
    bool valid = false;
    if (!predicate_value(w->body, n, &value) ||
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
        const NLSemanticContext *c = nl_checked_context(w->body);
        if (!n->symbol || n->symbol > c->binding_count ||
            n->symbol > NL_SEMANTIC_MAX_ENTRIES ||
            n->value_use != NL_VALUE_COPIED ||
            n->span.start_byte != n->name.start_byte ||
            n->span.end_byte != n->name.end_byte ||
            !predicate_name(w->body, n->name, c->bindings[n->symbol - 1].name))
            break;
        NLValueId origin;
        if (n->symbol == w->formal && n->type == w->quantity)
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
            !predicate_value(w->body, left, &a) ||
            !predicate_value(w->body, right, &b))
            break;
        const bool known = a.scalar_known && b.scalar_known;
        valid = value.scalar_known == known &&
                value.scalar_value ==
                    (known ? (less ? a.scalar_value < b.scalar_value
                                   : a.scalar_value == b.scalar_value)
                           : 0);
        break;
    }
    case NL_CHECKED_IF: {
        const NLCheckedNodeView *cond =
            nl_checked_node_view(w->body, n->initializer);
        valid = ++w->selections == 1 && n->type == w->u8 &&
                n->normal_arms == 2 && n->item_count == 2 &&
                !value.scalar_known && !value.scalar_value &&
                !n->has_scalar_result && cond && cond->type == w->boolean &&
                predicate_scalar(w, n->initializer) &&
                predicate_arm(w, id, 0) && predicate_arm(w, id, 1);
        if (valid) {
            w->selection = id;
            const NLCheckedFragment *a = nl_checked_if_arm(w->body, id, 0);
            const NLCheckedFragment *b = nl_checked_if_arm(w->body, id, 1);
            const NLCheckedNodeView *x =
                nl_checked_node_view(a, nl_checked_root(a));
            const NLCheckedNodeView *y =
                nl_checked_node_view(b, nl_checked_root(b));
            valid = copy_if_gap(nl_checked_source(w->body), n->span.start_byte,
                                cond->span.start_byte, "if(") &&
                    copy_if_gap(nl_checked_source(w->body), cond->span.end_byte,
                                x->span.start_byte, ")") &&
                    copy_if_gap(nl_checked_source(w->body), x->span.end_byte,
                                y->span.start_byte, "else") &&
                    y->span.end_byte == n->span.end_byte;
        }
        break;
    }
    default:
        break;
    }
done:
    --w->depth;
    return valid;
}

static bool nl_pure_u8_body(const NLCheckedFragment *f, NLSymbolId formal,
                            NLTypeId unit, NLTypeId u8,
                            NLCheckedNodeId *selection)
{
    const NLSemanticContext *c = nl_checked_context(f);
    PredicateWalk w = {.body = f,
                       .formal = formal,
                       .quantity = nl_semantic_core_type(c, NL_TYPE_USIZE),
                       .boolean = nl_semantic_core_type(c, NL_TYPE_BOOL),
                       .u8 = u8};
    const NLCheckedNodeView *root = predicate_take(&w, nl_checked_root(f));
    if (!root || root->kind != NL_CHECKED_BLOCK ||
        nl_checked_node_count(f) > NL_PARSER_MAX_NODES)
        return false;
    size_t items = 0;
    NLCheckedNodeId returned = 0;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = predicate_take(&w, id);
        if (!n || ++items > nl_checked_node_count(f))
            return false;
        if (n->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *receiver =
                predicate_take(&w, n->first_argument);
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, n->initializer);
            if (!receiver || !init || receiver->kind != NL_CHECKED_RECEIVER ||
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
        } else if (n->kind == NL_CHECKED_RETURN) {
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
        } else
            return false;
        id = n->next_item;
    }
    if (items != root->item_count)
        return false;
    const NLControlExits *exits = nl_checked_control_exits(f);
    if (returned) {
        const NLCheckedNodeView *value = nl_checked_node_view(f, returned);
        if (!value || !root->terminates || root->tail ||
            root->returned.type != u8 ||
            root->returned.value != value->results[0].value ||
            nl_control_exits_count(exits) != 1 ||
            !nl_control_exits_only_return(exits, f->function_target))
            return false;
    } else {
        const NLCheckedNodeView *tail = nl_checked_node_view(f, root->tail);
        if (root->terminates || !tail || tail->type != u8 ||
            root->result_count != 1 || root->results[0].type != u8 ||
            root->results[0].value != tail->results[0].value ||
            nl_control_exits_count(exits) || !predicate_scalar(&w, root->tail))
            return false;
    }
    if (w.selections != 1)
        return false;
    for (size_t i = 0; i < nl_checked_node_count(f); ++i)
        if (!w.seen[i])
            return false;
    if (selection)
        *selection = w.selection;
    return true;
}

typedef struct {
    const NLCheckedFragment *body;
    NLSymbolId formal;
    NLCheckedNodeId selection;
    size_t function;
} NLPureU8Call;

/* An actual CHECK-owned synchronous call, not a host-supplied summary.
 * Caller placement/quantity-expression policy belongs to each consumer. */
static bool nl_pure_u8_call(const NLCheckedFragment *f, NLCheckedNodeId id,
                            NLPureU8Call *out)
{
    if (!nl_h_custody_result_valid(f, id))
        return false;
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (!c || !c->functions || !call || !body || body->context != c ||
        call->kind != NL_CHECKED_REGISTERED_CALL || !call->function ||
        call->function > c->function_count || call->argument_count != 1 ||
        call->result_count != 1)
        return false;
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8),
                   quantity = nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLFunctionEntry *fn = &c->functions[call->function - 1];
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f, call->first_argument);
    if (call->type != u8 || call->results[0].type != u8 ||
        fn->kind != NL_CHECKED_REGISTERED_CALL || fn->count != 1 ||
        !fn->parameters || fn->parameters[0] != quantity || fn->result != u8 ||
        !fn->body || fn->body->count != 1 || !fn->body->parameter_names[0] ||
        !fn->body->syntax || body->body_owner != fn->body ||
        body->source != fn->body->source ||
        nl_syntax_tree_source(fn->body->syntax) != fn->body->source ||
        fn->caller_effects || fn->hidden_dependencies || fn->owner_receiver ||
        fn->owner_producer || fn->custody_recipient || !arg ||
        arg->next_argument || arg->result_count != 1 || arg->type != quantity ||
        arg->parameter_type != quantity ||
        !predicate_name(f, call->name, fn->name))
        return false;
    NLSymbolId formal = 0;
    for (size_t i = 0; i < c->binding_count; ++i) {
        const NLBindingEntry *b = &c->bindings[i];
        if (b->hidden && b->view.type == quantity &&
            b->view.value == arg->results[0].value && b->name &&
            !strcmp(b->name, fn->body->parameter_names[0])) {
            if (formal)
                return false;
            formal = i + 1;
        }
    }
    NLCheckedNodeId selection = 0;
    if (!formal || !nl_pure_u8_body(body, formal, nl_semantic_unit_type(c), u8,
                                    &selection))
        return false;
    const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
    const NLCheckedResult result =
        root->terminates ? root->returned : root->results[0];
    if (result.type != call->results[0].type ||
        result.value != call->results[0].value)
        return false;
    *out = (NLPureU8Call){body, formal, selection, call->function};
    return true;
}

#endif
