#ifndef NEWLANG_C17_PREDICATE_INTERNAL_H
#define NEWLANG_C17_PREDICATE_INTERNAL_H

/* Driver-private numeric consumer. The existing source checker establishes
 * both IF arms; this walk admits only pure scalar syntax in the actual owned
 * callable. It imports no arm value/current/owner into the caller. */
typedef struct {
    const NLCheckedFragment *body;
    NLSymbolId formal;
    NLTypeId quantity, boolean, u8;
    NLCheckedNodeId bindings[NL_SEMANTIC_MAX_ENTRIES];
    bool seen[NL_PARSER_MAX_NODES];
    size_t depth, selections;
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

static bool predicate_body(const NLCheckedFragment *f, NLSymbolId formal,
                           const V0Program *p)
{
    const NLSemanticContext *c = nl_checked_context(f);
    PredicateWalk w = {.body = f,
                       .formal = formal,
                       .quantity = nl_semantic_core_type(c, NL_TYPE_USIZE),
                       .boolean = nl_semantic_core_type(c, NL_TYPE_BOOL),
                       .u8 = p->u8};
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
                n->type != p->unit || n->terminates ||
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
            if (n->next_item || !n->terminates || !value ||
                value->type != p->u8 || n->returned.type != p->u8 ||
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
            root->returned.type != p->u8 ||
            root->returned.value != value->results[0].value ||
            nl_control_exits_count(exits) != 1 ||
            !nl_control_exits_only_return(exits, f->function_target))
            return false;
    } else {
        const NLCheckedNodeView *tail = nl_checked_node_view(f, root->tail);
        if (root->terminates || !tail || tail->type != p->u8 ||
            root->result_count != 1 || root->results[0].type != p->u8 ||
            root->results[0].value != tail->results[0].value ||
            nl_control_exits_count(exits) || !predicate_scalar(&w, root->tail))
            return false;
    }
    if (w.selections != 1)
        return false;
    for (size_t i = 0; i < nl_checked_node_count(f); ++i)
        if (!w.seen[i])
            return false;
    return true;
}

static bool predicate_call(const NLCheckedFragment *f, NLCheckedNodeId id,
                           V0Program *p)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (!call || !c || !body || !c->functions || call->type != p->u8 ||
        call->argument_count != 1 || call->result_count != 1 ||
        call->results[0].type != p->u8 || !call->function ||
        call->function > c->function_count || p->scalar_depth ||
        p->formal_body || p->unit_parameters || p->u8_export || p->aggregate ||
        p->linear_break)
        return false;
    const NLTypeId quantity = nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLFunctionEntry *fn = &c->functions[call->function - 1];
    NLSymbolId formal;
    if (fn->count != 1 || !fn->parameters || fn->parameters[0] != quantity ||
        !fn->body || !fn->body->syntax ||
        nl_syntax_tree_source(fn->body->syntax) != fn->body->source ||
        !predicate_name(f, call->name, fn->name) ||
        !v1_call_parameter_kind(f, id, p, &formal, true) ||
        !predicate_body(body, formal, p))
        return false;
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    const NLCheckedResult result =
        root->terminates ? root->returned : root->results[0];
    if (call->results[0].type != result.type ||
        call->results[0].value != result.value)
        return false;
    V0Function *prior = v0_find_function(p, call->function);
    if (prior)
        return prior->quantity_parameter && prior->result == p->u8;
    if (p->count == NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS)
        return false;
    p->functions[p->count++] =
        (V0Function){call->function, body, p->u8, formal, 0, true};
    p->uses_usize = true;
    return true;
}
#endif
