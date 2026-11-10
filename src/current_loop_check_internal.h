/* Included only by semantic_check.c. This relation is distinct from stutter:
 * H preserves the exact structural frame, and existentially quantifies the
 * dependency-free selected u8/current root package and one free usize slot.
 * A complete source transfer on this H, not the first concrete iteration,
 * establishes closure. Ghost projection never emits a runtime Change.
 */
typedef struct {
    const NLSyntaxNode *parameter, *conditional, *field, *second_field, *carry,
        *scalar, *terminal_scalar;
    bool terminal_old;
} CurrentSource;

/* A bounded, read-only provenance walk for the three ordinary usize Copy
 * boundaries used by the current loop profile. It follows actual checked
 * declarations in one owned fragment; equal scalar bits, inherited bindings,
 * and a name in another world are never an origin proof. The budget counts
 * identifier-to-identifier Copy edges. A literal seed binding is the root,
 * not another Copy edge. */
static bool current_copy_name(const NLCheckedFragment *f, NLSourceSpan span,
                              const char *name)
{
    NLSourceView text;
    return f && name && nl_source_view(nl_checked_source(f), span, &text) &&
           text.length && text.length == strlen(name) &&
           !memcmp(text.bytes, name, text.length);
}

static bool current_copy_value(const NLSemanticContext *c,
                               const NLCheckedNodeView *n, NLTypeId type,
                               NLSemanticValueView *out)
{
    return c && n && !n->terminates && n->type == type &&
           n->result_count == 1 && n->results[0].type == type &&
           nl_semantic_value_view(c, n->results[0].value, out) &&
           copy_if_plain(*out, type) &&
           (!n->has_scalar_result ||
            (n->scalar_result.type == type &&
             n->scalar_result.known == out->scalar_known &&
             n->scalar_result.value == out->scalar_value));
}

static const NLCheckedNodeView *
current_copy_binding(const NLCheckedFragment *f, const NLSemanticContext *c,
                     NLCheckedNodeId use, NLSymbolId symbol,
                     const NLCheckedNodeView **initializer)
{
    const NLCheckedNodeView *found = NULL;
    if (!f || !c || !symbol || symbol > c->binding_count ||
        symbol > NL_SEMANTIC_MAX_ENTRIES || !initializer)
        return NULL;
    for (NLCheckedNodeId id = 1; id < use; ++id) {
        const NLCheckedNodeView *b = nl_checked_node_view(f, id);
        if (!b || b->kind != NL_CHECKED_BINDING || b->symbol != symbol)
            continue;
        if (found)
            return NULL;
        const NLCheckedNodeView *receiver =
            nl_checked_node_view(f, b->first_argument);
        const NLCheckedNodeView *init = nl_checked_node_view(f, b->initializer);
        const NLCheckedNodeView *use_node = nl_checked_node_view(f, use);
        if (!receiver || !init || !use_node ||
            b->type != nl_semantic_unit_type(c) || b->result_count ||
            b->argument_count != 1 || b->initializer >= id ||
            b->first_argument <= id || b->first_argument >= use ||
            receiver->next_argument || receiver->kind != NL_CHECKED_RECEIVER ||
            receiver->value_use != NL_VALUE_RECEIVED ||
            receiver->symbol != symbol || receiver->type != init->type ||
            !current_copy_name(f, b->name, c->bindings[symbol - 1].name) ||
            !current_copy_name(f, receiver->name,
                               c->bindings[symbol - 1].name) ||
            b->span.end_byte > use_node->span.start_byte ||
            c->bindings[symbol - 1].view.type != init->type ||
            c->bindings[symbol - 1].view.value != init->results[0].value)
            return NULL;
        found = b;
        *initializer = init;
    }
    return found;
}

static bool current_copy_origin(const NLCheckedFragment *f,
                                const NLSemanticContext *c, NLCheckedNodeId id,
                                NLSymbolId root_symbol, bool literal_root,
                                size_t max_copies, size_t *literal)
{
    const NLTypeId type = c ? nl_semantic_core_type(c, NL_TYPE_USIZE) : 0;
    NLSemanticValueView expected = {0};
    bool first = true;
    size_t copies = 0;
    while (f && c && id && id <= nl_checked_node_count(f)) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        NLSemanticValueView value;
        if (!current_copy_value(c, n, type, &value) ||
            (!first && (value.scalar_known != expected.scalar_known ||
                        value.scalar_value != expected.scalar_value)))
            return false;
        if (first) {
            expected = value;
            first = false;
        }
        if (n->kind == NL_CHECKED_USIZE_LITERAL) {
            if (!literal_root || root_symbol || !value.scalar_known ||
                !n->has_scalar_result || !n->scalar_result.known ||
                n->scalar_result.value != value.scalar_value ||
                copies > max_copies)
                return false;
            if (literal)
                *literal = value.scalar_value;
            return true;
        }
        if (n->kind != NL_CHECKED_IDENTIFIER || !n->symbol ||
            n->symbol > c->binding_count ||
            n->symbol > NL_SEMANTIC_MAX_ENTRIES ||
            n->value_use != NL_VALUE_COPIED ||
            n->span.start_byte != n->name.start_byte ||
            n->span.end_byte != n->name.end_byte ||
            !current_copy_name(f, n->name, c->bindings[n->symbol - 1].name))
            return false;
        if (root_symbol && n->symbol == root_symbol) {
            NLSemanticValueView root;
            return !literal_root && copies <= max_copies &&
                   c->bindings[root_symbol - 1].view.type == type &&
                   nl_semantic_value_view(
                       c, c->bindings[root_symbol - 1].view.value, &root) &&
                   copy_if_plain(root, type) &&
                   root.scalar_known == value.scalar_known &&
                   root.scalar_value == value.scalar_value;
        }
        const NLCheckedNodeView *init = NULL;
        if (!current_copy_binding(f, c, id, n->symbol, &init))
            return false;
        NLSemanticValueView source;
        if (!current_copy_value(c, init, type, &source) ||
            source.scalar_known != value.scalar_known ||
            source.scalar_value != value.scalar_value)
            return false;
        if (init->kind == NL_CHECKED_IDENTIFIER && ++copies > max_copies)
            return false;
        if (init->kind != NL_CHECKED_IDENTIFIER &&
            init->kind != NL_CHECKED_USIZE_LITERAL)
            return false;
        id = (NLCheckedNodeId)(init - f->nodes) + 1;
    }
    return false;
}
/* At most two source RETURN leaves and their two existing body-union
 * records. These borrow core-owned states; only the authentication snapshots
 * below are owned here. No public RETURN issuer or graph is introduced. */
typedef struct {
    const NLCheckedFragment *owner, *source;
    const NLControlExits *set;
    size_t ordinal;
    NLCheckedNodeId node;
} CurrentReturnRef;
typedef struct {
    CurrentReturnRef ref;
    NLControlExitView view;
    size_t set_count;
    StutterWorld world;
} CurrentReturnSeal;
typedef struct {
    const NLTypeId *parameters;
    size_t count;
} CurrentSignatureOrigin;
typedef struct {
    const NLFunctionEntry *functions;
    size_t count;
    const CurrentSignatureOrigin *entries;
} CurrentSignatureSet;

enum {
    CURRENT_GRAPH_LIMIT = 9,
    CURRENT_CALLER_SLOT = 9,
    CURRENT_POST_SLOT = 10,
    CURRENT_EXIT_SLOT = 11,
    CURRENT_RETURN_SLOT = 15,
    CURRENT_CAPTURE_SLOT = 19,
    CURRENT_SIGNATURE_LIMIT = 21
};

/* One existing prefix call. These are borrowed custody members and allocation
 * origins, not graph/world clones or a second issuer. The two arm signature
 * origins use the same private table as all other current-cycle signatures. */
typedef struct {
    NLCheckedNodeId call;
    NLPureU8Call pure;
    const NLCheckedFragment *fragments[3];
    const NLSemanticContext *worlds[3];
    NLSemanticContext storage[3];
    const NLControlTarget *target;
    NLFunctionBody *plan;
    const NLSyntaxTree *syntax;
    const NLSource *source;
    const char *parameter_name;
} CurrentCapturedCall;

typedef struct NLCurrentLoopProof {
    NLControlState *entry, *header;
    NLCheckedFragment *graph;
    const NLCheckedFragment *parent;
    const NLControlTarget *target;
    const NLSyntaxNode *carry_source, *scalar_source, *terminal_source;
    NLSymbolId base, phase;
    NLPlaceId root, child;
    NLPlaceId second_child;
    size_t second_index;
    bool paired;
    size_t index, initial, limit, step, incoming;
    NLCheckedNodeId initial_node, conditional, quantity, terminal, scalar;
    StutterGraph graphs[CURRENT_GRAPH_LIMIT];
    bool helper;
    NLCheckedNodeId helper_choice;
    size_t helper_function;
    NLFunctionBody *helper_plan;
    const NLSyntaxTree *helper_syntax;
    const NLSource *helper_source;
    const char *helper_parameter_name;
    NLSourceSpan helper_name;
    /* Up to nine graphs, caller/post, four loop exits and four helper
     * exits. One private allocation contains every signature pointer/count
     * that the all-function comparator may consume. */
    CurrentSignatureSet signatures[CURRENT_SIGNATURE_LIMIT];
    CurrentSignatureOrigin *signature_entries;
    size_t signature_entry_count;
    const NLControlTarget *helper_target, *terminal_target;
    CurrentReturnSeal *returns;
    size_t return_count;
    StutterWorld exits[4];
    NLControlExitView exit_views[4];
    bool graph_owned, sealed;
    bool terminal_old;
    NLCheckedNodeId terminal_loan;
    NLCheckedNodeId terminal_call, terminal_choice;
    NLCheckedField terminal_change;
    CurrentCapturedCall capture;
} NLCurrentLoopProof;

static size_t current_graph_count(const NLCurrentLoopProof *s)
{
    return s->terminal_call ? CURRENT_GRAPH_LIMIT
           : s->helper      ? 6u
           : s->scalar      ? 5u
                            : 3u;
}

static void destroy_current_cycle(NLCurrentLoopProof *s)
{
    if (!s)
        return;
    for (size_t i = 0; i < CURRENT_GRAPH_LIMIT; ++i) {
        free(s->graphs[i].nodes);
        nl_copy_state_destroy(&s->graphs[i].world.anchor);
        nl_semantic_destroy(s->graphs[i].world.copy);
    }
    for (size_t i = 0; i < 4; ++i) {
        nl_copy_state_destroy(&s->exits[i].anchor);
        nl_semantic_destroy(s->exits[i].copy);
    }
    for (size_t i = 0; i < s->return_count; ++i) {
        nl_copy_state_destroy(&s->returns[i].world.anchor);
        nl_semantic_destroy(s->returns[i].world.copy);
    }
    free(s->signature_entries);
    free(s->returns);
    nl_control_state_destroy(s->entry);
    nl_control_state_destroy(s->header);
    if (!s->graph_owned)
        nl_checked_destroy(s->graph);
    free(s);
}

/* One phase predicate, two literal u8 normal results, no effects or exits.
 * This is a source envelope, never a substitute for the issued graph walk. */
static bool current_scalar_source(Check *c, const NLSyntaxNode *node,
                                  NLSourceSpan phase)
{
    const NLSyntaxView *v = nl_syntax_node_view(node);
    if (v && v->kind == NL_SYNTAX_EXPR_CALL) {
        const NLSyntaxView *arg = nl_syntax_node_view(v->data.call.arguments);
        /* This is only an envelope. The actual checked Copy chain must reach
         * the exact phase in current_pure_call, after checking the body. */
        return v->data.call.argument_count == 1 && arg &&
               arg->kind == NL_SYNTAX_EXPR_NAME;
    }
    if (!v || v->kind != NL_SYNTAX_IF)
        return false;
    const NLSyntaxView *test =
        nl_syntax_node_view(v->data.conditional.condition);
    if (!test || (test->kind != NL_SYNTAX_USIZE_LESS_THAN &&
                  test->kind != NL_SYNTAX_USIZE_EQUALITY))
        return false;
    const NLSyntaxView *left = nl_syntax_node_view(test->data.equality.left),
                       *right = nl_syntax_node_view(test->data.equality.right);
    if (!left || left->kind != NL_SYNTAX_EXPR_NAME ||
        !stutter_name(c, left->data.name, phase) || !right ||
        right->kind != NL_SYNTAX_USIZE_LITERAL)
        return false;
    const NLSyntaxNode *arms[] = {v->data.conditional.then_block,
                                  v->data.conditional.else_block};
    for (size_t i = 0; i < 2; ++i) {
        const NLSyntaxView *b = nl_syntax_node_view(arms[i]);
        const NLSyntaxView *tail = b && b->kind == NL_SYNTAX_BLOCK
                                       ? nl_syntax_node_view(b->data.block.tail)
                                       : NULL;
        if (!b || b->kind != NL_SYNTAX_BLOCK || b->data.block.items || !tail ||
            tail->kind != NL_SYNTAX_U8_LITERAL)
            return false;
    }
    return true;
}

/* The terminal result is independent of the Continue transfer. Keep stutter's
 * exhaustive source family unchanged; this alternative belongs only to the
 * changing-current relation and is checked again in each actual exit world. */
static bool current_break_source(Check *c, const NLSyntaxNode *node,
                                 NLSourceSpan phase)
{
    if (stutter_scalar_source(c, node, phase))
        return true;
    const NLSyntaxView *block = nl_syntax_node_view(node);
    const NLSyntaxView *terminal =
        block && block->kind == NL_SYNTAX_BLOCK
            ? nl_syntax_node_view(block->data.block.items)
            : NULL;
    const NLSyntaxView *value =
        terminal && terminal->kind == NL_SYNTAX_BREAK
            ? nl_syntax_node_view(terminal->data.statement.expression)
            : NULL;
    return block && !block->data.block.tail &&
           block->data.block.item_count == 1 && terminal &&
           !nl_syntax_next_argument(block->data.block.items) && value &&
           value->kind == NL_SYNTAX_USIZE_LITERAL;
}

/* Selection only: the checked source edge, loan, exact old-result path and
 * complete exit world are independently proved below before publication. */
static bool current_terminal_source(const NLSyntaxNode *node,
                                    const NLSyntaxNode **scalar)
{
    *scalar = NULL;
    const NLSyntaxView *block = nl_syntax_node_view(node);
    if (!block || block->kind != NL_SYNTAX_BLOCK || block->data.block.tail)
        return false;
    size_t count = 0, loans = 0;
    bool terminal = false;
    for (const NLSyntaxNode *n = block->data.block.items; n;
         n = nl_syntax_next_argument(n)) {
        const NLSyntaxView *v = nl_syntax_node_view(n), *x = NULL;
        if (!v || ++count > NL_SEMANTIC_MAX_ENTRIES || terminal)
            return false;
        if (v->kind == NL_SYNTAX_BREAK) {
            if (nl_syntax_next_argument(n))
                return false;
            terminal = true;
            x = nl_syntax_node_view(v->data.statement.expression);
        } else if (v->kind == NL_SYNTAX_BINDING)
            x = nl_syntax_node_view(v->data.binding.initializer);
        else
            return false;
        if (!x)
            return false;
        if (x->kind == NL_SYNTAX_LOCAL_WRITE_LOAN) {
            const NLSyntaxView *body = nl_syntax_node_view(x->data.loan.body);
            const NLSyntaxView *call =
                body && body->kind == NL_SYNTAX_BLOCK
                    ? nl_syntax_node_view(body->data.block.tail)
                    : NULL;
            if (++loans != 1 || x->data.loan.from_ptr ||
                x->data.loan.is_exclusive ||
                x->data.loan.access != NL_ACCESS_WRITE || !body ||
                body->data.block.items || !call ||
                call->kind != NL_SYNTAX_EXPR_CALL ||
                call->data.call.argument_count != 2)
                return false;
        } else if (x->kind == NL_SYNTAX_EXPR_CALL) {
            const NLSyntaxView *arg =
                nl_syntax_node_view(x->data.call.arguments);
            if (v->kind != NL_SYNTAX_BINDING || *scalar || loans ||
                x->data.call.argument_count != 1 || !arg ||
                arg->kind != NL_SYNTAX_EXPR_NAME)
                return false;
            *scalar = v->data.binding.initializer;
        } else if (x->kind != NL_SYNTAX_EXPR_NAME &&
                   x->kind != NL_SYNTAX_FIELD_DESIGNATOR &&
                   x->kind != NL_SYNTAX_U8_LITERAL)
            return false;
    }
    return terminal && loans == 1 && count == block->data.block.item_count;
}

static bool current_source_shape(Check *c, const NLSyntaxView *s,
                                 CurrentSource *out)
{
    if (s->data.loop.count != 1 || c->loop || c->in_source_loan ||
        c->in_match_arm || c->capture_copy_body || c->allocated_slice ||
        c->artifact->if_parent || c->artifact->node_join_parent)
        return false;
    for (size_t i = 0; i < c->artifact->count; ++i)
        if (c->artifact->nodes[i].kind == NL_CHECKED_LOOP)
            return false;
    const NLSyntaxView *p = nl_syntax_node_view(s->data.loop.parameters);
    const NLSyntaxView *init =
        p ? nl_syntax_node_view(p->data.binding.initializer) : NULL;
    const NLSyntaxView *b = nl_syntax_node_view(s->data.loop.body);
    const NLSyntaxView *v = b && b->kind == NL_SYNTAX_BLOCK
                                ? nl_syntax_node_view(b->data.block.tail)
                                : NULL;
    const NLSyntaxView *guard =
        v && v->kind == NL_SYNTAX_IF
            ? nl_syntax_node_view(v->data.conditional.condition)
            : NULL;
    const NLSyntaxView *lhs =
        guard && guard->kind == NL_SYNTAX_USIZE_LESS_THAN
            ? nl_syntax_node_view(guard->data.equality.left)
            : NULL;
    const NLSyntaxView *rhs =
        guard && guard->kind == NL_SYNTAX_USIZE_LESS_THAN
            ? nl_syntax_node_view(guard->data.equality.right)
            : NULL;
    if (!p || !init ||
        (init->kind != NL_SYNTAX_USIZE_LITERAL &&
         init->kind != NL_SYNTAX_EXPR_NAME) ||
        !b || b->kind != NL_SYNTAX_BLOCK || b->data.block.items || !v ||
        !guard || guard->kind != NL_SYNTAX_USIZE_LESS_THAN || !lhs || !rhs ||
        lhs->kind != NL_SYNTAX_EXPR_NAME ||
        !stutter_name(c, lhs->data.name, p->data.binding.name) ||
        rhs->kind != NL_SYNTAX_USIZE_LITERAL)
        return false;
    const bool terminal_old = !current_break_source(
        c, v->data.conditional.else_block, p->data.binding.name);
    const NLSyntaxNode *terminal_scalar = NULL;
    if (terminal_old && !current_terminal_source(v->data.conditional.else_block,
                                                 &terminal_scalar))
        return false;
    const NLSyntaxView *arm =
        nl_syntax_node_view(v->data.conditional.then_block);
    if (!arm || arm->kind != NL_SYNTAX_BLOCK || arm->data.block.tail)
        return false;
    const NLSyntaxNode *field = NULL, *second_field = NULL, *carry = NULL,
                       *scalar = NULL;
    size_t writers = 0, count = 0;
    for (const NLSyntaxNode *n = arm->data.block.items; n;
         n = nl_syntax_next_argument(n)) {
        if (++count > NL_SEMANTIC_MAX_ENTRIES)
            return false;
        const NLSyntaxView *item = nl_syntax_node_view(n), *expr = NULL;
        if (item->kind == NL_SYNTAX_CONTINUE) {
            if (nl_syntax_next_argument(n) ||
                item->data.call.argument_count != 1)
                return false;
            carry = item->data.call.arguments;
            continue;
        }
        if (item->kind == NL_SYNTAX_BINDING) {
            if (stutter_name(c, item->data.binding.name, p->data.binding.name))
                return false;
            expr = nl_syntax_node_view(item->data.binding.initializer);
        } else if (item->kind == NL_SYNTAX_STATEMENT) {
            expr = nl_syntax_node_view(item->data.statement.expression);
        }
        if (!expr)
            return false;
        if (item->kind == NL_SYNTAX_BINDING &&
            (expr->kind == NL_SYNTAX_IF || expr->kind == NL_SYNTAX_EXPR_CALL)) {
            if (scalar || writers ||
                !current_scalar_source(c, item->data.binding.initializer,
                                       p->data.binding.name))
                return false;
            scalar = item->data.binding.initializer;
            continue;
        }
        if (expr->kind == NL_SYNTAX_LOCAL_WRITE_LOAN &&
            item->kind == NL_SYNTAX_BINDING) {
            const NLSyntaxView *body =
                nl_syntax_node_view(expr->data.loan.body);
            const NLSyntaxView *call =
                body ? nl_syntax_node_view(body->data.block.tail) : NULL;
            const NLSyntaxView *target =
                nl_syntax_node_view(expr->data.loan.source);
            if (++writers > 2 || expr->data.loan.access != NL_ACCESS_WRITE ||
                expr->data.loan.from_ptr || expr->data.loan.is_exclusive ||
                !target || target->kind != NL_SYNTAX_FIELD_DESIGNATOR ||
                !body || body->kind != NL_SYNTAX_BLOCK ||
                body->data.block.items || !call ||
                call->kind != NL_SYNTAX_EXPR_CALL ||
                call->data.call.argument_count != 2)
                return false;
            if (writers == 1)
                field = expr->data.loan.source;
            else
                second_field = expr->data.loan.source;
        } else if (expr->kind != NL_SYNTAX_EXPR_NAME &&
                   expr->kind != NL_SYNTAX_FIELD_DESIGNATOR &&
                   expr->kind != NL_SYNTAX_U8_LITERAL)
            return false;
    }
    const NLSyntaxView *add = nl_syntax_node_view(carry);
    lhs = add && add->kind == NL_SYNTAX_USIZE_ADDITION
              ? nl_syntax_node_view(add->data.equality.left)
              : NULL;
    rhs = add && add->kind == NL_SYNTAX_USIZE_ADDITION
              ? nl_syntax_node_view(add->data.equality.right)
              : NULL;
    if (!writers || !add || add->kind != NL_SYNTAX_USIZE_ADDITION || !lhs ||
        lhs->kind != NL_SYNTAX_EXPR_NAME ||
        !stutter_name(c, lhs->data.name, p->data.binding.name) || !rhs ||
        rhs->kind != NL_SYNTAX_USIZE_LITERAL)
        return false;
    *out = (CurrentSource){.parameter = s->data.loop.parameters,
                           .conditional = b->data.block.tail,
                           .field = field,
                           .second_field = second_field,
                           .carry = carry,
                           .scalar = scalar,
                           .terminal_scalar = terminal_scalar,
                           .terminal_old = terminal_old};
    return true;
}

/* This slice contains no live capability/dependency which forgetting scalar
 * bits could launder. Check EVERY surviving package, not just the child.
 * Ended lexical loan packages may remain as history, never as a blocker.
 */
static bool current_packages(const NLSemanticContext *c, NLPlaceId root)
{
    if (!c || !root || root > c->place_count || c->domain_count ||
        c->region_count || c->occurrence_count ||
        nl_fixed_validate(c) != NL_CHECK_OK)
        return false;
    const NLValueId aggregate = c->places[root - 1].current_value;
    NLSemanticPlaceView trio[3];
    if (!nl_copy_trio(c, root, trio))
        return false;
    for (size_t i = 0; i < c->value_count; ++i) {
        const NLSemanticValueView v = c->values[i];
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        if (!v.type || v.type > c->type_count ||
            v.dependencies != NL_DEPENDENCY_FREE || v.value_dependency_count ||
            v.reference_count || v.reference.place)
            return false;
        if (i + 1 == aggregate)
            continue;
        if (!flat_copy(c, v.type) ||
            c->types[v.type - 1].view.kind == NL_TYPE_REF)
            return false;
    }
    return true;
}
static bool current_frame(const NLCurrentLoopProof *s,
                          const NLSemanticContext *a,
                          const NLSemanticContext *b)
{
    if (!current_packages(a, s->root) || !current_packages(b, s->root) ||
        a->binding_count != b->binding_count ||
        a->place_count != b->place_count || a->scope_count != b->scope_count ||
        !nl_copy_same_types(a, b) || a->function_count != b->function_count ||
        a->value_count > b->value_count ||
        a->last_incarnation > b->last_incarnation ||
        a->last_value_fact > b->last_value_fact)
        return false;
    for (size_t i = 0; i < a->place_count; ++i)
        if (!same_place_frame(a->places[i], b->places[i]) ||
            a->places[i].payload_occurrence !=
                b->places[i].payload_occurrence ||
            (i + 1 != s->root && i + 1 != s->child &&
             (!s->paired || i + 1 != s->second_child) &&
             (a->places[i].current_fact != b->places[i].current_fact ||
              a->places[i].current_value != b->places[i].current_value)))
            return false;
    for (size_t i = 0; i < a->binding_count; ++i) {
        const NLBindingEntry x = a->bindings[i], y = b->bindings[i];
        if (x.hidden != y.hidden || strcmp(x.name, y.name) ||
            x.view.type != y.view.type || x.view.place != y.view.place ||
            x.view.availability != y.view.availability ||
            y.view.value != (x.view.place == s->root
                                 ? b->places[s->root - 1].current_value
                                 : x.view.value))
            return false;
    }
    for (size_t i = 0; i < a->scope_count; ++i)
        if (!same_scope(a->scopes[i], b->scopes[i]))
            return false;
    const NLValueId old = a->places[s->root - 1].current_value;
    const NLPlaceId sibling = a->places[s->root - 1].fixed_fields[1 - s->index];
    const NLValueId peer = a->places[sibling - 1].current_value;
    NLSemanticValueView peer_after = b->values[peer - 1];
    /* current_packages(b)/nl_fixed_validate already proved that this exact
     * sibling belongs to b's canonical current root. Map only that checked
     * owner link back; every other sibling semantic member remains exact. */
    peer_after.aggregate_owner = old;
    if (!s->paired && !nl_node_value_same(a->values[peer - 1], peer_after))
        return false;
    for (size_t i = 0; i < a->value_count; ++i) {
        if (i + 1 == old || i + 1 == a->values[old - 1].fields[0] ||
            i + 1 == a->values[old - 1].fields[1])
            continue;
        if (!nl_node_value_same(a->values[i], b->values[i]))
            return false;
    }
    return true;
}

/* A private structural phi representation. It is used only after proving
 * inclusion/closure, or to construct the symbolic input to that proof.
 * It is NOT a source operation/receipt, and is never emitted by C17.
 */
static NLCheckStatus current_abstract(const NLCurrentLoopProof *s,
                                      NLSemanticContext *c)
{
    const NLPlaceId children[] = {s->child, s->second_child};
    for (size_t i = 0; i < (s->paired ? 2u : 1u); ++i) {
        const NLPlaceId child = children[i];
        NLCheckStatus status = nl_fixed_change_dependencies(c, child, 0);
        NLValueId value = 0, old = c->places[child - 1].current_value;
        if (status == NL_CHECK_OK)
            status = nl_sem_new_value(
                c,
                (NLSemanticValueView){.type =
                                          nl_semantic_core_type(c, NL_TYPE_U8)},
                &value);
        if (status == NL_CHECK_OK)
            status = nl_fixed_change(c, child, value, false);
        if (status != NL_CHECK_OK)
            return status;
        nl_sem_end_value(c, old);
    }
    for (size_t i = 0; i < c->binding_count; ++i)
        if (c->bindings[i].view.place == s->root)
            c->bindings[i].view.value = c->places[s->root - 1].current_value;
    return NL_CHECK_OK;
}
static bool current_projection(const NLCurrentLoopProof *s,
                               const NLSemanticContext *a,
                               const NLSemanticContext *b)
{
    if (!current_frame(s, a, b))
        return false;
    const NLSemanticPlaceView r = b->places[s->root - 1],
                              c = b->places[s->child - 1];
    const NLSemanticValueView v = b->values[c.current_value - 1];
    bool second = true;
    if (s->paired) {
        const NLSemanticPlaceView d = b->places[s->second_child - 1];
        second = !b->values[d.current_value - 1].scalar_known &&
                 d.current_fact == a->last_value_fact + 3 &&
                 d.current_value > a->value_count &&
                 b->values[a->places[s->second_child - 1].current_value - 1]
                         .carrier == NL_CARRIER_ENDED;
    }
    return second && !v.scalar_known &&
           b->last_incarnation == a->last_incarnation &&
           b->last_value_fact - a->last_value_fact == (s->paired ? 4u : 2u) &&
           r.current_fact > a->last_value_fact &&
           c.current_fact > a->last_value_fact &&
           r.current_fact != c.current_fact &&
           r.current_value > a->value_count &&
           c.current_value > a->value_count &&
           b->values[a->places[s->root - 1].current_value - 1].carrier ==
               NL_CARRIER_ENDED &&
           b->values[a->places[s->child - 1].current_value - 1].carrier ==
               NL_CARRIER_ENDED;
}

static bool current_quantity(Check *c, const NLSyntaxNode *source,
                             NLCheckedNodeId left, NLCheckedNodeId right)
{
    NLCurrentLoopProof *s = c->loop ? c->loop->current : NULL;
    if (!s || s->carry_source != source || s->quantity ||
        c->loop->target != s->target || c->artifact->if_parent != s->graph ||
        c->artifact->if_arm_index != 0 ||
        c->artifact->source != s->graph->source)
        return false;
    const NLCheckedNodeView *choice =
        nl_checked_node_view(s->graph, c->artifact->if_parent_node);
    const NLCheckedNodeView *guard =
        choice ? nl_checked_node_view(s->graph, choice->initializer) : NULL;
    const NLCheckedNodeView *a =
        guard ? nl_checked_node_view(s->graph, guard->first_argument) : NULL;
    const NLCheckedNodeView *b =
        a ? nl_checked_node_view(s->graph, a->next_argument) : NULL;
    const NLCheckedNodeView *x = view(c, left), *y = view(c, right);
    const NLTypeId type = nl_semantic_core_type(c->context, NL_TYPE_USIZE);
    if (!guard || guard->kind != NL_CHECKED_USIZE_LESS_THAN || !a || !b ||
        a->kind != NL_CHECKED_IDENTIFIER || a->symbol != s->phase ||
        x->kind != NL_CHECKED_IDENTIFIER || x->symbol != s->phase ||
        !b->has_scalar_result || !y->has_scalar_result ||
        !stutter_literal(b, type, b->scalar_result.value) ||
        !stutter_literal(y, type, y->scalar_result.value) || !s->phase ||
        s->phase > c->context->binding_count ||
        s->phase > s->graph->context->binding_count ||
        !stutter_free_quantity(c->context, x->results[0].value, type))
        return false;
    const NLSemanticBindingView before =
                                    s->graph->context->bindings[s->phase - 1]
                                        .view,
                                now = c->context->bindings[s->phase - 1].view;
    if (before.value != now.value || before.place != now.place ||
        before.availability != NL_AVAILABLE ||
        now.availability != NL_AVAILABLE || !before.place ||
        before.place > c->context->place_count ||
        before.place > s->graph->context->place_count ||
        s->graph->context->places[before.place - 1].current_fact !=
            c->context->places[now.place - 1].current_fact)
        return false;
    s->conditional = c->artifact->if_parent_node;
    s->limit = b->scalar_result.value;
    s->step = y->scalar_result.value;
    return s->step && finite_add_bound(s->limit, s->step);
}
static void current_quantity_record(Check *c, NLCheckedNodeId node)
{
    c->loop->current->quantity = node;
}

typedef struct {
    const NLCurrentLoopProof *proof;
    const NLCheckedFragment *arm;
    const NLSemanticContext *header;
    bool seen[NL_SEMANTIC_MAX_ENTRIES];
    NLValueId bound[NL_SEMANTIC_MAX_ENTRIES];
    bool incoming[NL_SEMANTIC_MAX_ENTRIES];
    NLCheckedNodeId scalar;
    NLCheckedField change;
    NLCheckedField changes[2];
    size_t change_count;
    bool changed, terminal;
} CurrentWalk;
static const NLCheckedNodeView *current_take(CurrentWalk *, NLCheckedNodeId);

/* Every persistent prefix member, including the actual phase package/fact,
 * is unchanged before the writer. Extra scalar locals are checked by the
 * enclosing exhaustive walk; no dependency/capability is forgotten here. */
static bool current_pure_prefix(const NLCurrentLoopProof *s,
                                const NLSemanticContext *b)
{
    const NLSemanticContext *a = s->graph->context;
    if (!current_packages(b, s->root) || !nl_copy_same_types(a, b) ||
        a->function_count != b->function_count ||
        a->binding_count > b->binding_count ||
        a->place_count > b->place_count || a->scope_count > b->scope_count ||
        a->value_count > b->value_count)
        return false;
    for (size_t i = 0; i < a->binding_count; ++i) {
        const NLBindingEntry x = a->bindings[i], y = b->bindings[i];
        if (!x.name || !y.name || strcmp(x.name, y.name) ||
            x.hidden != y.hidden || x.view.type != y.view.type ||
            x.view.value != y.view.value || x.view.place != y.view.place ||
            x.view.availability != y.view.availability)
            return false;
    }
    for (size_t i = 0; i < a->place_count; ++i)
        if (!same_place_frame(a->places[i], b->places[i]) ||
            a->places[i].current_value != b->places[i].current_value ||
            a->places[i].current_fact != b->places[i].current_fact ||
            a->places[i].payload_occurrence != b->places[i].payload_occurrence)
            return false;
    for (size_t i = 0; i < a->scope_count; ++i)
        if (!same_scope(a->scopes[i], b->scopes[i]))
            return false;
    for (size_t i = 0; i < a->value_count; ++i)
        if (!nl_node_value_same(a->values[i], b->values[i]))
            return false;
    return true;
}

/* Source RETURN nodes are already owned by CHECK, while generic return
 * records deliberately have no public source issuer. Bind the finite leaf /
 * body-union order to those actual nodes privately, and validate each result
 * only in its own captured world. Equal branch-local numeric IDs do not
 * identify a value in another branch. */
static bool current_return_ref(const NLCurrentLoopProof *s,
                               const CurrentReturnRef *r,
                               const NLControlTarget *target)
{
    const NLControlExitView *x = nl_control_exit_view(r->set, r->ordinal);
    const NLCheckedNodeView *ret = nl_checked_node_view(r->source, r->node),
                            *value = ret ? nl_checked_node_view(
                                               r->source, ret->initializer)
                                         : NULL;
    const NLSemanticContext *world =
        x ? nl_control_state_context((NLControlState *)x->state) : NULL;
    NLSemanticValueView actual, original;
    return r->owner->exits == r->set &&
           r->owner->function_target == (r->owner->if_parent ? NULL : target) &&
           r->source->function_target ==
               (r->source->if_parent ? NULL : target) &&
           ret && value && ret->kind == NL_CHECKED_RETURN && ret->terminates &&
           x && x->kind == NL_EXIT_RETURN && x->target == target &&
           x->count == 1 && !x->source_owner && !x->source_node &&
           !x->source_target && x->values[0].type == ret->returned.type &&
           x->values[0].value == ret->returned.value &&
           predicate_value(r->source, value, &original) &&
           nl_semantic_value_view(world, x->values[0].value, &actual) &&
           copy_if_plain(actual, value->type) &&
           actual.scalar_known == original.scalar_known &&
           actual.scalar_value == original.scalar_value &&
           current_pure_prefix(s, world);
}

static bool current_return_refs(const NLCurrentLoopProof *s,
                                const NLPureU8Call *pure,
                                CurrentReturnRef refs[4], size_t *count)
{
    const NLCheckedFragment *body = pure->body;
    const NLCheckedNodeView *choice =
                                nl_checked_node_view(body, pure->selection),
                            *root = nl_checked_node_view(body, body->root);
    *count = 0;
    size_t joined = 0;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm =
            nl_checked_if_arm(body, pure->selection, i);
        const NLCheckedNodeView *ar = nl_checked_node_view(arm, arm->root);
        if (!ar->terminates)
            continue;
        const NLCheckedNodeView *block =
            nl_checked_node_view(arm, ar->initializer);
        refs[(*count)++] =
            (CurrentReturnRef){arm, arm, arm->exits, 0, block->first_item};
        refs[(*count)++] = (CurrentReturnRef){body, arm, body->exits, joined++,
                                              block->first_item};
    }
    if (pure->plan.final_return) {
        if (!choice->normal_arms || !root->terminates || *count == 4)
            return false;
        refs[(*count)++] = (CurrentReturnRef){body, body, body->exits, joined,
                                              pure->plan.final_return};
        ++joined;
    }
    if (nl_control_exits_count(body->exits) != joined ||
        pure->plan.body_returns != joined || *count > 4)
        return false;
    for (size_t i = 0; i < *count; ++i)
        if (!current_return_ref(s, &refs[i], body->function_target))
            return false;
    return true;
}

/* Authenticate the actual normal scalar join and both complete, effect-free
 * arm graphs. Equal numeric IDs in different worlds are never an identity
 * proof: each result is checked only in its own owned world. */
static bool current_pure_if(CurrentWalk *r, NLCheckedNodeId id,
                            const NLCheckedNodeView *v)
{
    const NLCurrentLoopProof *s = r->proof;
    const NLSemanticContext *c = r->arm->context;
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8),
                   usize = nl_semantic_core_type(c, NL_TYPE_USIZE);
    const NLSyntaxView *source = nl_syntax_node_view(s->scalar_source);
    if (!source || r->scalar || r->changed || !v || v->kind != NL_CHECKED_IF ||
        !stutter_span(v->span, source->span) || v->terminates ||
        v->normal_arms != 2 || v->item_count != 2 || v->type != u8 ||
        v->result_count != 1 || v->has_scalar_result ||
        !nl_copy_plain(c, v->results[0].value, u8) ||
        c->values[v->results[0].value - 1].scalar_known)
        return false;
    const NLCheckedNodeView *test = current_take(r, v->initializer);
    const NLCheckedNodeView *left =
        test ? current_take(r, test->first_argument) : NULL;
    const NLCheckedNodeView *right =
        left ? current_take(r, left->next_argument) : NULL;
    const NLSyntaxView *guard =
        nl_syntax_node_view(source->data.conditional.condition);
    if (!test || !left || !right || !guard || test->argument_count != 2 ||
        !stutter_span(test->span, guard->span) ||
        !stutter_span(test->name, guard->data.equality.operator_span) ||
        test->kind != (guard->kind == NL_SYNTAX_USIZE_LESS_THAN
                           ? NL_CHECKED_USIZE_LESS_THAN
                           : NL_CHECKED_USIZE_EQUALITY) ||
        test->type != nl_semantic_core_type(c, NL_TYPE_BOOL) ||
        test->result_count != 1 || !test->has_scalar_result ||
        !nl_copy_plain(c, test->results[0].value, test->type) ||
        c->values[test->results[0].value - 1].scalar_known ||
        test->scalar_result.known || left->kind != NL_CHECKED_IDENTIFIER ||
        left->symbol != s->phase || left->value_use != NL_VALUE_COPIED ||
        left->result_count != 1 || left->type != usize ||
        !stutter_free_quantity(c, left->results[0].value, usize) ||
        !right->has_scalar_result ||
        !stutter_literal(right, usize, right->scalar_result.value) ||
        right->next_argument ||
        !nl_copy_plain(c, right->results[0].value, usize) ||
        !c->values[right->results[0].value - 1].scalar_known ||
        c->values[right->results[0].value - 1].scalar_value !=
            right->scalar_result.value)
        return false;
    const NLCheckedFragment *arms[2];
    for (size_t i = 0; i < 2; ++i) {
        arms[i] = nl_checked_if_arm(r->arm, id, i);
        const NLCheckedFragment *a = arms[i];
        if (!a || a->if_parent != r->arm || a->if_parent_node != id ||
            a->if_arm_index != i || a->source != r->arm->source ||
            a->count != 3 || nl_control_exits_count(a->exits) || a->arm_count ||
            a->body_count || !current_pure_prefix(s, a->context))
            return false;
        const NLCheckedNodeView *root = nl_checked_node_view(a, a->root);
        const NLCheckedNodeView *block =
            root ? nl_checked_node_view(a, root->initializer) : NULL;
        const NLCheckedNodeView *tail =
            block ? nl_checked_node_view(a, block->tail) : NULL;
        const NLSyntaxView *source_block =
            nl_syntax_node_view(i ? source->data.conditional.else_block
                                  : source->data.conditional.then_block);
        const NLSyntaxView *source_tail =
            source_block ? nl_syntax_node_view(source_block->data.block.tail)
                         : NULL;
        if (!root || root->kind != NL_CHECKED_IF_ARM || root->terminates ||
            root->result_count != 1 || !block ||
            block->kind != NL_CHECKED_BLOCK || block->first_item ||
            block->item_count || block->result_count != 1 || !tail ||
            tail->kind != NL_CHECKED_U8_LITERAL || tail->result_count != 1 ||
            tail->type != u8 || !tail->has_scalar_result ||
            !tail->scalar_result.known || tail->scalar_result.value > 255 ||
            root->results[0].value != tail->results[0].value ||
            block->results[0].value != tail->results[0].value || !source_tail ||
            !stutter_span(block->span, source_block->span) ||
            !stutter_span(tail->span, source_tail->span) ||
            !nl_copy_plain(a->context, tail->results[0].value, u8) ||
            !a->context->values[tail->results[0].value - 1].scalar_known ||
            a->context->values[tail->results[0].value - 1].scalar_value !=
                tail->scalar_result.value ||
            a->context->value_count != a->branch_value_prefix + 1 ||
            tail->results[0].value != a->context->value_count ||
            v->results[0].value != a->branch_value_prefix + 1 ||
            a->context->last_value_fact != a->branch_fact_prefix)
            return false;
    }
    const NLSemanticContext *a = arms[0]->context, *b = arms[1]->context;
    if (arms[0]->branch_value_prefix != arms[1]->branch_value_prefix ||
        arms[0]->branch_fact_prefix != arms[1]->branch_fact_prefix ||
        a->binding_count != b->binding_count ||
        a->place_count != b->place_count || a->scope_count != b->scope_count ||
        a->last_incarnation != b->last_incarnation)
        return false;
    for (size_t i = 0; i < a->binding_count; ++i)
        if (strcmp(a->bindings[i].name, b->bindings[i].name) ||
            a->bindings[i].hidden != b->bindings[i].hidden ||
            a->bindings[i].view.type != b->bindings[i].view.type ||
            a->bindings[i].view.value != b->bindings[i].view.value ||
            a->bindings[i].view.place != b->bindings[i].view.place ||
            a->bindings[i].view.availability !=
                b->bindings[i].view.availability)
            return false;
    for (size_t i = 0; i < a->place_count; ++i)
        if (memcmp(&a->places[i], &b->places[i], sizeof(NLSemanticPlaceView)))
            return false;
    for (size_t i = 0; i < a->scope_count; ++i)
        if (!same_scope(a->scopes[i], b->scopes[i]))
            return false;
    for (size_t i = 0; i < arms[0]->branch_value_prefix; ++i)
        if (!nl_node_value_same(a->values[i], b->values[i]))
            return false;
    r->scalar = id;
    return true;
}
/* The call's complete pure body is shared with the ordinary consumer.
 * Its IF cuts precede the sole writer, while body/context itself shares the
 * caller's final world. Never mistake that final world for an entry cut. */
static bool current_pure_call(CurrentWalk *r, NLCheckedNodeId id,
                              const NLCheckedNodeView *v)
{
    const NLCurrentLoopProof *s = r->proof;
    const NLSyntaxView *source = nl_syntax_node_view(
        r->terminal ? s->terminal_source : s->scalar_source);
    NLPureU8Call pure;
    if (!s->helper || !source || source->kind != NL_SYNTAX_EXPR_CALL ||
        r->scalar || r->changed || !v || !stutter_span(v->span, source->span) ||
        !stutter_span(v->name, source->data.call.callee) ||
        (s->sealed && !nl_h_custody_result_valid(r->arm, id)) ||
        !nl_pure_u8_contents(r->arm, id, &pure))
        return false;
    if (r->terminal) {
        const NLCheckedNodeView *root =
            nl_checked_node_view(pure.body, pure.body->root);
        const NLCheckedNodeView *choice =
            nl_checked_node_view(pure.body, pure.selection);
        /* One already-selected pure function, at its actual call boundary.
         * The new terminal family has two normal arms and the IF is the
         * actual final value; no RETURN/postlude is silently normalized. */
        if (pure.function != s->helper_function || !root || !choice ||
            root->terminates || root->tail != pure.selection ||
            choice->normal_arms != 2 ||
            nl_control_exits_count(pure.body->exits))
            return false;
    }
    const NLCheckedNodeView *arg = current_take(r, v->first_argument);
    const NLSyntaxView *actual =
        nl_syntax_node_view(source->data.call.arguments);
    const NLTypeId quantity =
        nl_semantic_core_type(r->arm->context, NL_TYPE_USIZE);
    const NLCheckedNodeView *root =
        nl_checked_node_view(pure.body, pure.body->root);
    const NLCheckedNodeView *choice =
        nl_checked_node_view(pure.body, pure.selection);
    const NLCheckedNodeView *test =
        choice ? nl_checked_node_view(pure.body, choice->initializer) : NULL;
    const NLCheckedNodeView *left =
        test ? nl_checked_node_view(pure.body, test->first_argument) : NULL;
    const NLCheckedNodeView *right =
        left ? nl_checked_node_view(pure.body, left->next_argument) : NULL;
    if (!arg || !actual || !stutter_span(arg->span, actual->span) ||
        arg->next_argument ||
        !current_copy_origin(r->arm, r->arm->context, v->first_argument,
                             s->phase, false, 2, NULL) ||
        !stutter_free_quantity(r->arm->context, arg->results[0].value,
                               quantity) ||
        !root || pure.body->body_count || pure.body->arm_count != 2 ||
        !choice || !test || !left || !right ||
        (test->kind != NL_CHECKED_USIZE_LESS_THAN &&
         test->kind != NL_CHECKED_USIZE_EQUALITY) ||
        !current_copy_origin(pure.body, pure.body->context,
                             test->first_argument, pure.formal, false, 2,
                             NULL) ||
        !stutter_literal(right, quantity, right->scalar_result.value) ||
        right->next_argument)
        return false;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm =
            nl_checked_if_arm(pure.body, pure.selection, i);
        if (!arm || arm->arm_count || arm->body_count ||
            !current_pure_prefix(s, arm->context))
            return false;
    }
    CurrentReturnRef refs[4];
    size_t count;
    if (!r->terminal && !current_return_refs(s, &pure, refs, &count))
        return false;
    r->scalar = id;
    return true;
}
static bool current_intrinsic(NLSemanticValueView a, NLSemanticValueView b)
{
    return a.type == b.type && a.scalar_known == b.scalar_known &&
           (!a.scalar_known || a.scalar_value == b.scalar_value);
}

/* A captured scalar keeps its original entry package. It is not the selected
 * child's newer current value, and gives no access/owner authority. The full
 * frame/exit relation still checks every other prefix member independently. */
static bool current_capture_binding(const NLSemanticContext *entry,
                                    const NLSemanticContext *world,
                                    NLSymbolId symbol, NLTypeId type)
{
    if (!entry || !world || !symbol || symbol > NL_SEMANTIC_MAX_ENTRIES ||
        symbol > entry->binding_count || symbol > world->binding_count)
        return false;
    const NLBindingEntry a = entry->bindings[symbol - 1],
                         b = world->bindings[symbol - 1];
    const NLValueId value = a.view.value;
    return a.name && b.name && !a.hidden && !b.hidden &&
           !strcmp(a.name, b.name) && a.view.type == type &&
           b.view.type == type && a.view.availability == NL_AVAILABLE &&
           b.view.availability == NL_AVAILABLE && value &&
           b.view.value == value && b.view.place == a.view.place &&
           value <= entry->value_count && value <= world->value_count &&
           entry->values[value - 1].carrier != NL_CARRIER_ENDED &&
           copy_if_plain(entry->values[value - 1], type) &&
           copy_if_plain(world->values[value - 1], type) &&
           nl_node_value_same(entry->values[value - 1],
                              world->values[value - 1]);
}

/* Follow actual declarations in the owning enclosing prefix, bounded by the
 * already-issued caller initializer node. Reuse the Copy declaration/source
 * checks; never seed the arm's local binding table from a live world. */
static bool current_prefix_contents(const NLCurrentLoopProof *s,
                                    NLCheckedNodeId id, NLPureU8Call *out)
{
    const NLCheckedFragment *parent = s->parent;
    const NLSemanticContext *entry = nl_control_state_context(s->entry);
    const NLCheckedNodeView *call = nl_checked_node_view(parent, id);
    NLPureU8Call pure;
    size_t seed = 0;
    if (!entry || !id || id >= s->initial_node || !call ||
        (s->sealed && !nl_h_custody_result_valid(parent, id)) ||
        !nl_pure_u8_contents_at(parent, id,
                                s->sealed ? nl_checked_context(parent) : entry,
                                &pure) ||
        !current_copy_origin(parent, entry, call->first_argument, 0, true, 2,
                             &seed))
        return false;
    const NLCheckedNodeView *root =
        nl_checked_node_view(pure.body, pure.body->root);
    const NLCheckedNodeView *choice =
        nl_checked_node_view(pure.body, pure.selection);
    if (!root || !choice || root->terminates || root->tail != pure.selection ||
        choice->normal_arms != 2 || nl_control_exits_count(pure.body->exits))
        return false;
    *out = pure;
    return true;
}

/* The real declaration must precede the loop and remain in its entry frame.
 * One independent ordinary call suffices for this family. Unrelated or more
 * general prefixes keep their old CHECK behavior and can still decline C17. */
static void current_collect_capture(NLCurrentLoopProof *s)
{
    if (!s->helper || !s->terminal_old)
        return;
    const NLSemanticContext *entry = nl_control_state_context(s->entry);
    const NLTypeId u8 = nl_semantic_core_type(entry, NL_TYPE_U8);
    for (NLCheckedNodeId id = 1; id < s->initial_node; ++id) {
        const NLCheckedNodeView *binding = nl_checked_node_view(s->parent, id);
        const NLCheckedNodeView *init = NULL;
        NLPureU8Call pure;
        if (!binding || binding->kind != NL_CHECKED_BINDING ||
            !current_capture_binding(entry, entry, binding->symbol, u8) ||
            current_copy_binding(s->parent, entry, s->initial_node,
                                 binding->symbol, &init) != binding ||
            init->kind != NL_CHECKED_REGISTERED_CALL ||
            !current_prefix_contents(s, binding->initializer, &pure))
            continue;
        if (s->capture.call) {
            s->capture = (CurrentCapturedCall){0};
            return;
        }
        s->capture.call = binding->initializer;
        s->capture.pure = pure;
    }
}

/* Borrow the already checked, closed allocation result, never an H owner.
 * The parent declaration walk below binds this exact package to the loop E.
 * Published validation additionally authenticates the existing sealed main
 * call and its complete H subtree before observing any arm view. */
static bool current_closed_h_result(const NLCurrentLoopProof *s,
                                    const NLCheckedNodeView *match,
                                    NLTypeId type)
{
    const NLCheckedFragment *parent = s->parent;
    const NLCheckedNodeId id = (NLCheckedNodeId)(match - parent->nodes) + 1;
    if (s->sealed) {
        const NLLinearBreakProof *proof = parent->linear_breaks;
        while (proof && proof->current_cycle != s)
            proof = proof->next;
        if (!proof || !proof->sealed || proof->owner != parent ||
            !nl_h_origin_validate(proof->root) ||
            !nl_h_custody_result_valid(proof->root, proof->call))
            return false;
    }
    const NLCheckedNodeView *trial =
        nl_checked_node_view(parent, match->initializer);
    if (id >= s->initial_node || match->type != type || match->terminates ||
        match->borrowed_match || !match->normal_frame_unchanged ||
        match->captured_frame_closed || match->normal_arms != 2 ||
        match->item_count != 2 || !trial ||
        trial->kind != NL_CHECKED_TRY_ALLOCATE_ONE || !trial->allocation_trial)
        return false;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_match_arm(parent, id, i);
        const NLCheckedNodeView *result =
            arm ? nl_checked_node_view(arm, nl_checked_root(arm)) : NULL;
        NLSemanticValueView value;
        if (!arm || arm->source != parent->source || !result ||
            result->kind != NL_CHECKED_MATCH_ARM || result->variant != i + 1 ||
            !result->normal_frame_unchanged ||
            !current_copy_value(arm->context, result, type, &value))
            return false;
    }
    return true;
}

static bool current_capture_origin(const NLCurrentLoopProof *s,
                                   NLSymbolId symbol, NLTypeId type)
{
    const NLCheckedFragment *parent = s->parent;
    const NLSemanticContext *entry = nl_control_state_context(s->entry);
    NLCheckedNodeId use = s->initial_node;
    size_t copies = 0;
    while (parent && entry && use && use <= nl_checked_node_count(parent)) {
        const NLCheckedNodeView *init = NULL;
        if (!current_capture_binding(entry, entry, symbol, type) ||
            !current_copy_binding(parent, entry, use, symbol, &init))
            return false;
        NLSemanticValueView value;
        if (!current_copy_value(entry, init, type, &value))
            return false;
        if (init->kind == NL_CHECKED_U8_LITERAL)
            return init->has_scalar_result && value.scalar_known &&
                   value.scalar_value <= 255;
        if (init->kind == NL_CHECKED_MATCH)
            return current_closed_h_result(s, init, type);
        if (init->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedNodeId id =
                (NLCheckedNodeId)(init - parent->nodes) + 1;
            NLPureU8Call pure;
            return id == s->capture.call &&
                   current_prefix_contents(s, id, &pure) &&
                   pure.body == s->capture.pure.body &&
                   pure.function == s->capture.pure.function &&
                   pure.formal == s->capture.pure.formal &&
                   pure.selection == s->capture.pure.selection;
        }
        if (init->kind == NL_CHECKED_FIELD_READ) {
            const NLCheckedField f = init->field;
            if (!s->root || s->root > entry->place_count || !f.present ||
                f.base != s->base || init->symbol != s->base ||
                f.parent != s->root || f.index >= 2 || f.type != type ||
                f.access != NL_ACCESS_READ ||
                init->value_use != NL_VALUE_COPIED)
                return false;
            const NLSemanticPlaceView root = entry->places[s->root - 1];
            if (!f.child || f.child > entry->place_count ||
                f.child != root.fixed_fields[f.index] ||
                f.nominal != root.type ||
                f.parent_incarnation != root.incarnation ||
                f.parent_fact != root.current_fact)
                return false;
            const NLSemanticPlaceView child = entry->places[f.child - 1];
            return f.child_incarnation == child.incarnation &&
                   f.child_fact == child.current_fact &&
                   f.old_value == child.current_value && f.old_value &&
                   f.old_value <= entry->value_count && !f.new_value &&
                   !f.parent_post_fact && !f.child_post_fact &&
                   current_intrinsic(entry->values[f.old_value - 1], value);
        }
        if (init->kind != NL_CHECKED_IDENTIFIER || ++copies > 2 ||
            init->value_use != NL_VALUE_COPIED ||
            init->span.start_byte != init->name.start_byte ||
            init->span.end_byte != init->name.end_byte || !init->symbol ||
            init->symbol > entry->binding_count ||
            !current_copy_name(parent, init->name,
                               entry->bindings[init->symbol - 1].name) ||
            !current_capture_binding(entry, entry, init->symbol, type) ||
            !current_intrinsic(
                entry->values[entry->bindings[init->symbol - 1].view.value - 1],
                value))
            return false;
        use = (NLCheckedNodeId)(init - parent->nodes) + 1;
        symbol = init->symbol;
    }
    return false;
}

static bool current_inherited_scalar(CurrentWalk *r,
                                     const NLCheckedNodeView *value)
{
    const NLCurrentLoopProof *s = r->proof;
    const NLSemanticContext *entry = nl_control_state_context(s->entry),
                            *world = r->arm->context;
    const NLTypeId type = nl_semantic_core_type(world, NL_TYPE_U8);
    if (!s->helper || !s->parent ||
        nl_checked_source(s->parent) != nl_checked_source(r->arm) ||
        nl_checked_source(s->parent) != nl_checked_source(s->graph) || !value ||
        value->kind != NL_CHECKED_IDENTIFIER ||
        value->value_use != NL_VALUE_COPIED ||
        value->span.start_byte != value->name.start_byte ||
        value->span.end_byte != value->name.end_byte ||
        !current_capture_binding(entry, r->header, value->symbol, type) ||
        !current_capture_binding(entry, world, value->symbol, type) ||
        !current_copy_name(r->arm, value->name,
                           entry->bindings[value->symbol - 1].name) ||
        !current_capture_origin(s, value->symbol, type))
        return false;
    NLSemanticValueView copy;
    if (!current_copy_value(world, value, type, &copy)) {
        /* This operand is observed in the checked POST-Change world. Its
         * fresh Copy package may already be the selected current member.
         * Authenticate that exact attachment; inspect only a local content
         * view, without detaching anything or changing the semantic world. */
        if (!r->terminal || value->type != type || value->result_count != 1 ||
            value->results[0].type != type || !s->root || !s->child ||
            s->root > world->place_count || s->child > world->place_count ||
            !nl_semantic_value_view(world, value->results[0].value, &copy))
            return false;
        const NLSemanticPlaceView root = world->places[s->root - 1],
                                  child = world->places[s->child - 1];
        if (!root.live || !child.live || root.fixed_field_count != 2 ||
            s->index >= 2 || root.fixed_fields[s->index] != s->child ||
            child.parent_aggregate != s->root ||
            child.parent_field_index != s->index ||
            child.parent_incarnation != root.incarnation ||
            child.current_value != value->results[0].value ||
            !copy.aggregate_owner || copy.aggregate_owner != root.current_value)
            return false;
        copy.aggregate_owner = 0;
        if (!copy_if_plain(copy, type) ||
            (value->has_scalar_result &&
             (value->scalar_result.type != type ||
              value->scalar_result.known != copy.scalar_known ||
              value->scalar_result.value != copy.scalar_value)))
            return false;
    }
    return current_intrinsic(
        entry->values[entry->bindings[value->symbol - 1].view.value - 1], copy);
}

static const NLCheckedNodeView *current_take(CurrentWalk *r, NLCheckedNodeId id)
{
    if (!id || id > r->arm->count || id > NL_SEMANTIC_MAX_ENTRIES ||
        r->seen[id - 1])
        return NULL;
    r->seen[id - 1] = true;
    return nl_checked_node_view(r->arm, id);
}
static bool current_scalar(CurrentWalk *r, const NLCheckedNodeView *v)
{
    if (v && v->type == nl_semantic_core_type(r->arm->context, NL_TYPE_USIZE)) {
        const NLCheckedNodeId id = (NLCheckedNodeId)(v - r->arm->nodes) + 1;
        return current_copy_origin(r->arm, r->arm->context, id, r->proof->phase,
                                   false, 2, NULL);
    }
    if (!v || v->result_count != 1 ||
        v->type != nl_semantic_core_type(r->arm->context, NL_TYPE_U8) ||
        !nl_copy_plain(r->arm->context, v->results[0].value, v->type))
        return false;
    if (v->kind == NL_CHECKED_U8_LITERAL)
        return v->has_scalar_result && v->scalar_result.known &&
               v->scalar_result.value <= 255;
    if (v->kind == NL_CHECKED_IDENTIFIER) {
        if (current_inherited_scalar(r, v))
            return true;
        return v->symbol && v->symbol <= NL_SEMANTIC_MAX_ENTRIES &&
               v->symbol <= r->arm->context->binding_count &&
               r->bound[v->symbol - 1] && v->value_use == NL_VALUE_COPIED &&
               r->arm->context->bindings[v->symbol - 1].view.value ==
                   r->bound[v->symbol - 1] &&
               current_intrinsic(
                   r->arm->context->values[v->results[0].value - 1],
                   r->arm->context->values[r->bound[v->symbol - 1] - 1]);
    }
    if (v->kind != NL_CHECKED_FIELD_READ)
        return false;
    const NLCurrentLoopProof *s = r->proof;
    const NLCheckedField f = v->field;
    const NLSemanticPlaceView root = r->header->places[s->root - 1];
    if (!f.present || f.base != s->base || f.parent != s->root ||
        f.index >= 2 || f.child != root.fixed_fields[f.index] ||
        f.parent_incarnation != root.incarnation)
        return false;
    const NLSemanticPlaceView child = r->header->places[f.child - 1];
    const NLCheckedField *changed = NULL;
    for (size_t i = 0; i < r->change_count; ++i)
        if (r->changes[i].child == f.child)
            changed = &r->changes[i];
    return f.child_incarnation == child.incarnation &&
           f.parent_fact ==
               (r->changed ? r->change.parent_post_fact : root.current_fact) &&
           f.child_fact ==
               (changed ? changed->child_post_fact : child.current_fact) &&
           f.old_value ==
               (changed ? changed->new_value : child.current_value) &&
           current_intrinsic(r->arm->context->values[v->results[0].value - 1],
                             r->arm->context->values[f.old_value - 1]);
}

static bool current_loan(CurrentWalk *r, const NLCheckedNodeView *loan,
                         bool terminal)
{
    const NLCurrentLoopProof *s = r->proof;
    const bool second = !terminal && s->paired && r->change_count == 1;
    const NLPlaceId child = second ? s->second_child : s->child;
    const size_t index = second ? s->second_index : s->index;
    const NLCheckedNodeView *body = current_take(r, loan->initializer);
    const NLCheckedNodeView *op = body ? current_take(r, body->tail) : NULL;
    const NLCheckedNodeView *ref =
        op ? current_take(r, op->first_argument) : NULL;
    const NLCheckedNodeView *value =
        ref ? current_take(r, ref->next_argument) : NULL;
    const NLSemanticContext *c = r->arm->context;
    if (r->change_count >= (terminal || !s->paired ? 1u : 2u) || !body ||
        body->kind != NL_CHECKED_BLOCK || body->item_count || !op ||
        op->kind != NL_CHECKED_REPLACE || op->argument_count != 2 ||
        op->result_count != 1 || !ref || !value || value->next_argument ||
        ref->kind != NL_CHECKED_IDENTIFIER ||
        ref->symbol != loan->loan.ref_symbol || !current_scalar(r, value) ||
        (terminal
             ? (value->kind != NL_CHECKED_U8_LITERAL &&
                !current_inherited_scalar(r, value) &&
                (!r->scalar || value->kind != NL_CHECKED_IDENTIFIER ||
                 !value->symbol || value->symbol > NL_SEMANTIC_MAX_ENTRIES ||
                 !r->incoming[value->symbol - 1]))
         : second ? false
         : s->scalar_source
             ? (!r->scalar || value->kind != NL_CHECKED_IDENTIFIER ||
                !value->symbol || value->symbol > NL_SEMANTIC_MAX_ENTRIES ||
                !r->incoming[value->symbol - 1])
             : value->kind != NL_CHECKED_U8_LITERAL) ||
        !loan->loan.implicit_local || loan->loan.from_ptr ||
        loan->loan.is_exclusive || loan->loan.access != NL_ACCESS_WRITE ||
        loan->loan.source != s->base || !loan->loan.body_nonescape_proved ||
        !loan->loan.normal_result_forwarded || !loan->loan.scope ||
        loan->loan.scope > c->scope_count ||
        c->scopes[loan->loan.scope - 1].active || !op->field.present ||
        !op->field.dependency_compatible || op->field.child != child ||
        op->field.parent != s->root || op->field.index != index ||
        loan->loan.place != child ||
        op->field.parent_fact !=
            (r->changed ? r->change.parent_post_fact
                        : r->header->places[s->root - 1].current_fact) ||
        op->field.child_fact != r->header->places[child - 1].current_fact ||
        op->field.old_value != r->header->places[child - 1].current_value ||
        op->field.parent_post_fact == op->field.child_post_fact ||
        op->results[0].value != op->field.old_value ||
        value->results[0].value != op->field.new_value ||
        loan->results[0].value != op->results[0].value)
        return false;
    r->changed = true;
    r->change = op->field;
    r->changes[r->change_count++] = op->field;
    return true;
}

/* Follow real checked Copy declarations to this very loan. Equal scalar bits
 * and a copied numeric ValueId are never a replacement for source origin. */
static bool current_terminal_old_path(const NLCheckedFragment *f,
                                      NLCheckedNodeId use, NLCheckedNodeId loan)
{
    const NLTypeId u8 = nl_semantic_core_type(f->context, NL_TYPE_U8);
    size_t steps = 0;
    while (use && use <= nl_checked_node_count(f) &&
           ++steps <= nl_checked_node_count(f)) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, use);
        if (!n ||
            !current_copy_value(f->context, n, u8, &(NLSemanticValueView){0}))
            return false;
        if (use == loan)
            return n->kind == NL_CHECKED_LOAN_HEADER;
        if (n->kind != NL_CHECKED_IDENTIFIER || !n->symbol ||
            n->value_use != NL_VALUE_COPIED)
            return false;
        const NLCheckedNodeView *initializer = NULL;
        const NLCheckedNodeView *binding =
            current_copy_binding(f, f->context, use, n->symbol, &initializer);
        if (!binding || !initializer || binding->initializer >= use)
            return false;
        use = binding->initializer;
    }
    return false;
}

static bool current_terminal_change_same(NLCheckedField a, NLCheckedField b)
{
#define SAME(member) a.member == b.member
    return SAME(present) && SAME(dependency_compatible) && SAME(base) &&
           SAME(nominal) && SAME(type) && SAME(index) && SAME(parent) &&
           SAME(child) && SAME(parent_incarnation) && SAME(child_incarnation) &&
           SAME(parent_fact) && SAME(child_fact) && SAME(parent_post_fact) &&
           SAME(child_post_fact) && SAME(access) && SAME(old_value) &&
           SAME(new_value) && SAME(payload_occurrence) &&
           SAME(post_payload_occurrence);
#undef SAME
}

/* A source literal remains Known in its own leaf/union worlds. The phase is
 * Unknown. Neither package grants memory authority, and the caller summary
 * may conservatively forget the literal bits. Do not relax Continue's free
 * Unknown quantity check to admit this separate terminal result. */
static bool current_break_value(const NLCurrentLoopProof *s,
                                const NLSemanticContext *world,
                                const NLCheckedNodeView *source,
                                NLCheckedResult result)
{
    const NLTypeId type = nl_semantic_core_type(world, NL_TYPE_USIZE);
    if (!source || source->result_count != 1 || source->type != type ||
        result.type != type || source->results[0].type != type ||
        source->results[0].value != result.value)
        return false;
    if (source->kind == NL_CHECKED_IDENTIFIER)
        return source->symbol == s->phase &&
               source->value_use == NL_VALUE_COPIED &&
               stutter_free_quantity(world, result.value, type);
    NLSemanticValueView value;
    return stutter_literal(source, type, source->scalar_result.value) &&
           nl_semantic_value_view(world, result.value, &value) &&
           copy_if_plain(value, type) && value.scalar_known &&
           value.scalar_value == source->scalar_result.value;
}

static bool current_arm(const NLCurrentLoopProof *s,
                        const NLCheckedFragment *arm, bool continuing,
                        NLCheckedField *field, NLCheckedNodeId *scalar)
{
    CurrentWalk r = {.proof = s,
                     .arm = arm,
                     .header = nl_control_state_context(s->header),
                     .terminal = !continuing};
    if (!arm || arm->count > NL_SEMANTIC_MAX_ENTRIES ||
        !nl_control_source_terminal_valid(
            arm, s->target, continuing ? NL_EXIT_CONTINUE : NL_EXIT_BREAK,
            s->parent))
        return false;
    const NLCheckedNodeView *wrapper = current_take(&r, arm->root);
    const NLCheckedNodeView *block =
        wrapper ? current_take(&r, wrapper->initializer) : NULL;
    if (!block || block->kind != NL_CHECKED_BLOCK || block->tail)
        return false;
    size_t count = 0;
    NLCheckedNodeId old_loan = 0;
    for (NLCheckedNodeId id = block->first_item; id;) {
        const NLCheckedNodeView *v = current_take(&r, id);
        if (!v || ++count > block->item_count)
            return false;
        if (v->kind == NL_CHECKED_CONTINUE || v->kind == NL_CHECKED_BREAK) {
            const NLCheckedNodeView *x = current_take(&r, v->initializer);
            if (v->next_item || !x ||
                v->kind !=
                    (continuing ? NL_CHECKED_CONTINUE : NL_CHECKED_BREAK))
                return false;
            if (continuing) {
                const NLCheckedNodeView *a =
                    current_take(&r, x->first_argument);
                const NLCheckedNodeView *b =
                    a ? current_take(&r, a->next_argument) : NULL;
                if (v->initializer != s->quantity ||
                    x->kind != NL_CHECKED_USIZE_ADDITION || !a ||
                    a->kind != NL_CHECKED_IDENTIFIER || a->symbol != s->phase ||
                    !b || b->next_argument ||
                    !stutter_literal(b, x->type, s->step) ||
                    !x->has_scalar_result || x->scalar_result.known ||
                    !stutter_free_quantity(arm->context, x->results[0].value,
                                           x->type) ||
                    !r.changed)
                    return false;
            } else if (s->terminal_old) {
                if (x->kind == NL_CHECKED_LOAN_HEADER) {
                    if (!current_loan(&r, x, true))
                        return false;
                    old_loan = v->initializer;
                }
                if (!r.changed || !old_loan ||
                    !current_terminal_old_path(arm, v->initializer, old_loan))
                    return false;
            } else if (block->item_count != 1 ||
                       !current_break_value(s, arm->context, x, x->results[0]))
                return false;
        } else if ((continuing || s->terminal_old) &&
                   (v->kind == NL_CHECKED_BINDING ||
                    v->kind == NL_CHECKED_STATEMENT)) {
            const NLCheckedNodeView *x = current_take(&r, v->initializer);
            if (!x)
                return false;
            if (v->kind == NL_CHECKED_BINDING) {
                const NLCheckedNodeView *receiver =
                    current_take(&r, v->first_argument);
                if (!receiver || receiver->kind != NL_CHECKED_RECEIVER ||
                    !v->symbol || v->symbol > NL_SEMANTIC_MAX_ENTRIES ||
                    v->symbol > arm->context->binding_count ||
                    r.bound[v->symbol - 1] || v->symbol == s->phase ||
                    receiver->symbol != v->symbol ||
                    receiver->type != x->type ||
                    !(x->kind == NL_CHECKED_LOAN_HEADER
                          ? current_loan(&r, x, !continuing)
                      : x->kind == NL_CHECKED_IF
                          ? current_pure_if(&r, v->initializer, x)
                      : x->kind == NL_CHECKED_REGISTERED_CALL
                          ? current_pure_call(&r, v->initializer, x)
                          : current_scalar(&r, x)) ||
                    arm->context->bindings[v->symbol - 1].view.value !=
                        x->results[0].value)
                    return false;
                r.bound[v->symbol - 1] = x->results[0].value;
                if (!continuing && x->kind == NL_CHECKED_LOAN_HEADER)
                    old_loan = v->initializer;
                r.incoming[v->symbol - 1] =
                    (x->kind == NL_CHECKED_IF ||
                     x->kind == NL_CHECKED_REGISTERED_CALL) ||
                    (x->kind == NL_CHECKED_IDENTIFIER && x->symbol &&
                     x->symbol <= NL_SEMANTIC_MAX_ENTRIES &&
                     r.incoming[x->symbol - 1]);
            } else if (!current_scalar(&r, x))
                return false;
        } else
            return false;
        id = v->next_item;
    }
    for (size_t i = 0; i < arm->count; ++i)
        if (!r.seen[i])
            return false;
    if (field)
        for (size_t i = 0; i < r.change_count; ++i)
            field[i] = r.changes[i];
    if (scalar)
        *scalar = r.scalar;
    return count == block->item_count &&
           r.changed == (continuing || s->terminal_old) &&
           r.change_count == (continuing ? (s->paired ? 2u : 1u)
                                         : (s->terminal_old ? 1u : 0u)) &&
           (!continuing || !!r.scalar == !!s->scalar_source);
}

static NLCheckStatus current_relation(NLCurrentLoopProof *s)
{
    const NLCheckStatus bad = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLSemanticContext *e = nl_control_state_context(s->entry),
                            *h = nl_control_state_context(s->header);
    const NLCheckedFragment *g = s->graph;
    const NLCheckedNodeView *root = nl_checked_node_view(g, g->root);
    const NLCheckedNodeView *choice =
        root ? nl_checked_node_view(g, root->tail) : NULL;
    const NLCheckedNodeView *guard =
        choice ? nl_checked_node_view(g, choice->initializer) : NULL;
    const NLCheckedNodeView *lhs =
        guard ? nl_checked_node_view(g, guard->first_argument) : NULL;
    const NLCheckedNodeView *rhs =
        lhs ? nl_checked_node_view(g, lhs->next_argument) : NULL;
    const NLTypeId type = nl_semantic_core_type(h, NL_TYPE_USIZE);
    if (!e || !h || !s->root || s->root > h->place_count || !s->child ||
        s->child > h->place_count || s->index >= 2 ||
        h->places[s->root - 1].fixed_fields[s->index] != s->child ||
        (s->paired && (!s->second_child || s->second_child > h->place_count ||
                       s->second_index >= 2 || s->second_index == s->index ||
                       s->second_child == s->child ||
                       h->places[s->root - 1].fixed_fields[s->second_index] !=
                           s->second_child)) ||
        !current_projection(s, e, h) || !s->step ||
        !finite_add_bound(s->limit, s->step) || !root ||
        root->kind != NL_CHECKED_BLOCK || root->item_count ||
        root->tail != s->conditional || !choice ||
        choice->kind != NL_CHECKED_IF || !guard ||
        guard->kind != NL_CHECKED_USIZE_LESS_THAN || !lhs || !rhs ||
        lhs->kind != NL_CHECKED_IDENTIFIER || lhs->symbol != s->phase ||
        !stutter_literal(rhs, type, s->limit) || rhs->next_argument ||
        !stutter_free_quantity(g->context, lhs->results[0].value, type) ||
        g->count != 5 || !current_packages(g->context, s->root) ||
        nl_control_exits_count(g->exits) != 2)
        return bad;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_if_arm(g, s->conditional, i);
        NLCheckedField changes[2] = {0};
        NLCheckedNodeId scalar = 0;
        if (!current_arm(s, arm, i == 0, changes, &scalar) || !arm ||
            arm->if_parent != g || arm->if_parent_node != s->conditional ||
            arm->if_arm_index != i || arm->source != g->source)
            return bad;
        const NLControlExitView *leaf = nl_control_exit_view(arm->exits, 0);
        const NLControlExitView *joined = nl_control_exit_view(g->exits, i);
        const NLSemanticContext *a =
            leaf ? nl_control_state_context((NLControlState *)leaf->state)
                 : NULL;
        const NLSemanticContext *b =
            joined ? nl_control_state_context((NLControlState *)joined->state)
                   : NULL;
        if (!leaf || !joined || leaf->target != s->target ||
            joined->target != s->target || joined->kind != leaf->kind ||
            joined->count != 1 || joined->source_owner != arm ||
            joined->source_node != leaf->source_node ||
            joined->values[0].type != leaf->values[0].type ||
            joined->values[0].value != leaf->values[0].value ||
            !current_frame(s, h, a) || !current_frame(s, h, b))
            return bad;
        if (i == 0) {
            if (!stutter_free_quantity(a, leaf->values[0].value, type) ||
                !stutter_free_quantity(b, joined->values[0].value, type) ||
                !linear_break_frame_changes(h, arm->context, changes,
                                            s->paired ? 2u : 1u) ||
                !linear_break_frame_changes(h, a, changes,
                                            s->paired ? 2u : 1u) ||
                !linear_break_frame_changes(h, b, changes, s->paired ? 2u : 1u))
                return bad;
            const NLSemanticValueView incoming =
                a->values[changes[0].new_value - 1];
            if (s->scalar_source
                    ? (!scalar || incoming.scalar_known)
                    : (!incoming.scalar_known || incoming.scalar_value > 255))
                return bad;
            if (s->sealed &&
                (s->scalar != scalar ||
                 (!s->scalar_source && s->incoming != incoming.scalar_value) ||
                 s->terminal != leaf->source_node))
                return bad;
            if (s->helper) {
                NLPureU8Call pure;
                if ((s->sealed && !nl_h_custody_result_valid(arm, scalar)) ||
                    !nl_pure_u8_contents(arm, scalar, &pure) ||
                    (s->sealed && s->helper_choice != pure.selection))
                    return bad;
                if (!s->sealed) {
                    s->helper_choice = pure.selection;
                    s->helper_function = pure.function;
                } else if (s->helper_function != pure.function)
                    return bad;
            }
            if (!s->sealed) {
                s->scalar = scalar;
                s->incoming = s->scalar_source ? 0 : incoming.scalar_value;
                s->terminal = leaf->source_node;
            }
        } else {
            const NLCheckedField change = changes[0];
            const NLCheckedNodeView *terminal =
                nl_checked_node_view(arm, leaf->source_node);
            const NLCheckedNodeView *value =
                terminal ? nl_checked_node_view(arm, terminal->initializer)
                         : NULL;
            if (s->terminal_old) {
                const NLTypeId u8 = nl_semantic_core_type(h, NL_TYPE_U8);
                if (!!scalar != !!s->terminal_source)
                    return bad;
                if (scalar) {
                    NLPureU8Call terminal_pure;
                    if (!s->helper ||
                        !nl_pure_u8_contents(arm, scalar, &terminal_pure) ||
                        terminal_pure.function != s->helper_function ||
                        (s->sealed &&
                         (s->terminal_call != scalar ||
                          s->terminal_choice != terminal_pure.selection)))
                        return bad;
                    if (!s->sealed) {
                        s->terminal_call = scalar;
                        s->terminal_choice = terminal_pure.selection;
                    }
                }
                if (!value || leaf->values[0].type != u8 ||
                    !leaf->values[0].value ||
                    leaf->values[0].value > a->value_count ||
                    !joined->values[0].value ||
                    joined->values[0].value > b->value_count ||
                    !change.old_value || change.old_value > h->value_count ||
                    a->values[leaf->values[0].value - 1].carrier !=
                        NL_CARRIER_LOOSE ||
                    b->values[joined->values[0].value - 1].carrier !=
                        NL_CARRIER_LOOSE ||
                    !copy_if_plain(a->values[leaf->values[0].value - 1], u8) ||
                    !copy_if_plain(b->values[joined->values[0].value - 1],
                                   u8) ||
                    !linear_break_frame(h, arm->context, &change) ||
                    !linear_break_frame(h, a, &change) ||
                    !linear_break_frame(h, b, &change) ||
                    !current_intrinsic(a->values[leaf->values[0].value - 1],
                                       h->values[change.old_value - 1]) ||
                    !current_intrinsic(b->values[joined->values[0].value - 1],
                                       h->values[change.old_value - 1]))
                    return bad;
                NLCheckedNodeId loan = 0;
                for (NLCheckedNodeId q = 1; q <= arm->count; ++q)
                    if (nl_checked_node_view(arm, q)->kind ==
                        NL_CHECKED_LOAN_HEADER) {
                        if (loan)
                            return bad;
                        loan = q;
                    }
                if (!loan ||
                    !current_terminal_old_path(arm, terminal->initializer,
                                               loan) ||
                    (s->sealed && (s->terminal_loan != loan ||
                                   !current_terminal_change_same(
                                       s->terminal_change, change))))
                    return bad;
                if (!s->sealed) {
                    s->terminal_loan = loan;
                    s->terminal_change = change;
                }
            } else if (!current_break_value(s, a, value, leaf->values[0]) ||
                       !current_break_value(s, b, value, joined->values[0]) ||
                       !stutter_frame(h, a, 1) || !stutter_frame(h, b, 1))
                return bad;
        }
    }
    return NL_CHECK_OK;
}

/* The caller is a clone/projection of the authenticated existing BREAK exit,
 * not a fresh result typed from an oracle. The source edge proof above must
 * precede this comparison. No new state-origin or issuer is introduced. */
static bool current_terminal_post(const NLCurrentLoopProof *s,
                                  const NLSemanticContext *post,
                                  NLCheckedResult result)
{
    const NLCheckedFragment *arm =
        nl_checked_if_arm(s->graph, s->conditional, 1);
    const NLControlExitView *exit =
        arm ? nl_control_exit_view(arm->exits, 0) : NULL;
    const NLSemanticContext *a =
        exit ? nl_control_state_context((NLControlState *)exit->state) : NULL;
    const NLSemanticContext *h = nl_control_state_context(s->header);
    if (!exit || !a || !post || result.type != exit->values[0].type ||
        result.value != exit->values[0].value ||
        result.type != nl_semantic_core_type(post, NL_TYPE_U8) ||
        !current_frame(s, h, post) || !nl_copy_same_types(a, post) ||
        a->binding_count != post->binding_count ||
        a->place_count != post->place_count ||
        a->scope_count != post->scope_count ||
        a->value_count != post->value_count ||
        a->last_incarnation != post->last_incarnation ||
        a->last_value_fact != post->last_value_fact)
        return false;
    for (size_t i = 0; i < a->value_count; ++i)
        if (!nl_node_value_same(a->values[i], post->values[i]))
            return false;
    for (size_t i = 0; i < a->place_count; ++i)
        if (!same_place_frame(a->places[i], post->places[i]) ||
            a->places[i].current_value != post->places[i].current_value ||
            a->places[i].current_fact != post->places[i].current_fact ||
            a->places[i].payload_occurrence !=
                post->places[i].payload_occurrence)
            return false;
    for (size_t i = 0; i < a->binding_count; ++i) {
        const NLBindingEntry x = a->bindings[i], y = post->bindings[i];
        if (x.hidden != y.hidden || strcmp(x.name, y.name) ||
            x.view.type != y.view.type || x.view.place != y.view.place ||
            x.view.value != y.view.value ||
            x.view.availability != y.view.availability)
            return false;
    }
    for (size_t i = 0; i < a->scope_count; ++i)
        if (!same_scope(a->scopes[i], post->scopes[i]))
            return false;
    return true;
}

static const NLCheckedFragment *current_graph_at(const NLCurrentLoopProof *s,
                                                 size_t i)
{
    if (!i)
        return s->graph;
    if (i < 3)
        return nl_checked_if_arm(s->graph, s->conditional, i - 1);
    if (i >= 6) {
        const NLCheckedFragment *breaking =
            nl_checked_if_arm(s->graph, s->conditional, 1);
        const NLCheckedFragment *body =
            breaking ? nl_checked_call_body(breaking, s->terminal_call) : NULL;
        return i == 6 ? body
               : body ? nl_checked_if_arm(body, s->terminal_choice, i - 7)
                      : NULL;
    }
    const NLCheckedFragment *iteration =
        nl_checked_if_arm(s->graph, s->conditional, 0);
    if (s->helper) {
        const NLCheckedFragment *body =
            iteration ? nl_checked_call_body(iteration, s->scalar) : NULL;
        return i == 3 ? body
               : body ? nl_checked_if_arm(body, s->helper_choice, i - 4)
                      : NULL;
    }
    return iteration ? nl_checked_if_arm(iteration, s->scalar, i - 3) : NULL;
}

static const NLFunctionEntry *
current_helper_function(const NLCurrentLoopProof *s, const NLSemanticContext *c)
{
    return c && c->functions && s->helper_function &&
                   s->helper_function <= c->function_count
               ? &c->functions[s->helper_function - 1]
               : NULL;
}

static const NLSemanticContext *
current_signature_world(const NLCurrentLoopProof *s,
                        const NLLinearBreakProof *p, size_t slot)
{
    if (slot < current_graph_count(s))
        return s->graphs[slot].world.world;
    if (slot == CURRENT_CALLER_SLOT)
        return &p->final_storage;
    if (slot == CURRENT_POST_SLOT)
        return p->post;
    if (slot >= CURRENT_EXIT_SLOT && slot < CURRENT_RETURN_SLOT)
        return s->exits[slot - CURRENT_EXIT_SLOT].world;
    if (slot >= CURRENT_RETURN_SLOT &&
        slot - CURRENT_RETURN_SLOT < s->return_count)
        return s->returns[slot - CURRENT_RETURN_SLOT].world.world;
    if (slot >= CURRENT_CAPTURE_SLOT && slot < CURRENT_SIGNATURE_LIMIT &&
        s->capture.call)
        return s->capture.worlds[slot - CURRENT_CAPTURE_SLOT + 1];
    return NULL;
}

/* Retain the complete dependency set of the all-function signature-content
 * comparison. The ledger borrows the CHECK-owned allocations; it never owns
 * or extends their lifetime. One allocation keeps OOM rollback local. */
static NLCheckStatus current_signature_seal(NLCurrentLoopProof *s,
                                            const NLLinearBreakProof *p)
{
    size_t total = 0;
    for (size_t slot = 0; slot < CURRENT_SIGNATURE_LIMIT; ++slot) {
        const NLSemanticContext *c = current_signature_world(s, p, slot);
        if (!c)
            continue;
        if (c->function_count > NL_SEMANTIC_MAX_ENTRIES ||
            (c->function_count && !c->functions) ||
            c->function_count > SIZE_MAX - total)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        total += c->function_count;
    }
    if (total > SIZE_MAX / sizeof(*s->signature_entries))
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    if (total) {
        s->signature_entries = calloc(total, sizeof(*s->signature_entries));
        if (!s->signature_entries)
            return NL_CHECK_OUT_OF_MEMORY;
    }
    s->signature_entry_count = total;
    size_t offset = 0;
    for (size_t slot = 0; slot < CURRENT_SIGNATURE_LIMIT; ++slot) {
        const NLSemanticContext *c = current_signature_world(s, p, slot);
        if (!c)
            continue;
        CurrentSignatureSet *set = &s->signatures[slot];
        set->functions = c->functions;
        set->count = c->function_count;
        set->entries = c->function_count ? s->signature_entries + offset : NULL;
        for (size_t i = 0; i < c->function_count; ++i) {
            s->signature_entries[offset + i] = (CurrentSignatureOrigin){
                .parameters = c->functions[i].parameters,
                .count = c->functions[i].count};
        }
        offset += c->function_count;
    }
    return NL_CHECK_OK;
}

/* Authenticate every signature allocation and extent before the comparator
 * can read any signature contents. No parameter element is read here. */
static bool current_signature_origin(const NLCurrentLoopProof *s,
                                     const NLSemanticContext *c, size_t slot)
{
    if (!c || slot >= CURRENT_SIGNATURE_LIMIT)
        return false;
    const CurrentSignatureSet *set = &s->signatures[slot];
    if (c->functions != set->functions || c->function_count != set->count ||
        (set->count && (!set->functions || !set->entries)))
        return false;
    for (size_t i = 0; i < set->count; ++i)
        if (set->functions[i].parameters != set->entries[i].parameters ||
            set->functions[i].count != set->entries[i].count)
            return false;
    return true;
}

/* Current helper admission authenticates the actual retained plan and
 * each world's original signature allocation before indexing it. Plan owner
 * counts may change; syntax/source/parameter identity may not. */
static bool current_helper_entry(const NLCurrentLoopProof *s,
                                 const NLSemanticContext *c, size_t slot)
{
    if (!current_signature_origin(s, c, slot) ||
        s->helper_function > s->signatures[slot].count)
        return false;
    const NLFunctionEntry *fn = current_helper_function(s, c);
    if (!fn)
        return false;
    const CurrentSignatureOrigin *origin =
        &s->signatures[slot].entries[s->helper_function - 1];
    return fn->kind == NL_CHECKED_REGISTERED_CALL &&
           fn->body == s->helper_plan && fn->count == 1 &&
           fn->parameters == origin->parameters && origin->count == 1 &&
           fn->parameters &&
           fn->parameters[0] == nl_semantic_core_type(c, NL_TYPE_USIZE) &&
           fn->result == nl_semantic_core_type(c, NL_TYPE_U8) &&
           !fn->caller_effects && !fn->hidden_dependencies &&
           !fn->owner_receiver && !fn->owner_producer &&
           !fn->custody_recipient &&
           predicate_name(s->graphs[1].fragment, s->helper_name, fn->name) &&
           fn->body->syntax == s->helper_syntax &&
           fn->body->source == s->helper_source && fn->body->count == 1 &&
           fn->body->parameter_names[0] == s->helper_parameter_name &&
           !fn->body->owner_definition.definition_checked &&
           !fn->body->custody_definition.definition_checked;
}

static NLCheckStatus seal_current_cycle(NLLinearBreakProof *p)
{
    NLCurrentLoopProof *s = p->current_cycle;
    if (s->capture.call) {
        CurrentCapturedCall *c = &s->capture;
        if (!current_prefix_contents(s, c->call, &c->pure))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        c->fragments[0] = c->pure.body;
        for (size_t i = 0; i < 2; ++i)
            c->fragments[i + 1] =
                nl_checked_if_arm(c->pure.body, c->pure.selection, i);
        for (size_t i = 0; i < 3; ++i) {
            if (!c->fragments[i] || !c->fragments[i]->context)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            c->worlds[i] = c->fragments[i]->context;
            c->storage[i] = i ? *c->worlds[i] : p->final_storage;
        }
        c->target = c->pure.body->function_target;
        c->plan = p->final_storage.functions[c->pure.function - 1].body;
        c->syntax = c->plan->syntax;
        c->source = c->plan->source;
        c->parameter_name = c->plan->parameter_names[0];
    }
    if (s->helper) {
        const NLCheckedFragment *iteration =
            nl_checked_if_arm(s->graph, s->conditional, 0);
        NLPureU8Call pure;
        if (!nl_pure_u8_contents(iteration, s->scalar, &pure))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        CurrentReturnRef refs[4];
        size_t count;
        if (!current_return_refs(s, &pure, refs, &count))
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        s->helper_target = pure.body->function_target;
        if (count) {
            s->returns = calloc(count, sizeof(*s->returns));
            if (!s->returns)
                return NL_CHECK_OUT_OF_MEMORY;
            s->return_count = count;
            for (size_t i = 0; i < count; ++i) {
                CurrentReturnSeal *r = &s->returns[i];
                r->ref = refs[i];
                r->set_count = nl_control_exits_count(r->ref.set);
                const NLControlExitView *x =
                    nl_control_exit_view(r->ref.set, r->ref.ordinal);
                memcpy(&r->view, x, sizeof(*x));
                NLCheckStatus status = stutter_anchor_world(
                    &r->world,
                    nl_control_state_context((NLControlState *)x->state));
                if (status != NL_CHECK_OK)
                    return status;
            }
        }
        s->helper_plan = pure.body->body_owner;
        s->helper_syntax = s->helper_plan->syntax;
        s->helper_source = s->helper_plan->source;
        s->helper_parameter_name = s->helper_plan->parameter_names[0];
        s->helper_name = nl_checked_node_view(iteration, s->scalar)->name;
        const NLFunctionEntry *actual =
                                  current_helper_function(s, &p->final_storage),
                              *post = current_helper_function(s, p->post);
        if (!actual || !post || !actual->parameters || !post->parameters)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    for (size_t i = 0; i < current_graph_count(s); ++i) {
        StutterGraph *a = &s->graphs[i];
        a->fragment = current_graph_at(s, i);
        if (!a->fragment)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        a->base = a->fragment->nodes;
        a->count = a->fragment->count;
        a->root = a->fragment->root;
        a->source = a->fragment->source;
        NLCheckStatus status = linear_seal_graph(a->fragment, &a->nodes);
        if (status == NL_CHECK_OK)
            status = stutter_anchor_world(&a->world, a->fragment->context);
        if (status != NL_CHECK_OK)
            return status;
    }
    if (s->terminal_call)
        s->terminal_target = s->graphs[6].fragment->function_target;
    for (size_t i = 0; i < 4; ++i) {
        const NLControlExitView *x =
            i < 2 ? nl_control_exit_view(s->graphs[i + 1].fragment->exits, 0)
                  : nl_control_exit_view(s->graph->exits, i - 2);
        if (!x)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        memcpy(&s->exit_views[i], x, sizeof(*x));
        NLCheckStatus status = stutter_anchor_world(
            &s->exits[i], nl_control_state_context((NLControlState *)x->state));
        if (status != NL_CHECK_OK)
            return status;
    }
    NLCheckStatus signature_status = current_signature_seal(s, p);
    if (signature_status != NL_CHECK_OK)
        return signature_status;
    s->sealed = true;
    return NL_CHECK_OK;
}

static NLCheckStatus validate_current_cycle(const NLLinearBreakProof *p,
                                            const NLCheckedFragment *f,
                                            NLLinearBreakView *out)
{
    const NLCheckStatus bad = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLCurrentLoopProof *s = p->current_cycle;
    NLPureU8Call pure = {0};
    if (!p->sealed || !s->sealed || !s->graph_owned || p->owner != f ||
        s->parent != f || f->context != p->world || f->source != p->source ||
        f->body_owner != p->plan || !nl_h_custody_validate(p->root) ||
        !linear_graph_matches(p->root, p->root_origin, p->root_nodes,
                              p->root_count, p->root_root) ||
        !linear_graph_matches(f, p->parent_origin, p->parent_nodes,
                              p->parent_count, p->parent_root) ||
        !current_signature_origin(s, f->context, CURRENT_CALLER_SLOT) ||
        !current_signature_origin(s, p->post, CURRENT_POST_SLOT) ||
        !linear_world_matches(f->context, &p->final_storage, p->final,
                              &p->final_anchor) ||
        !linear_callable(p, f->context) || !linear_callable(p, p->post) ||
        !nl_copy_state_matches(p->post, &p->post_anchor) ||
        nl_checked_loop_body(f, p->loop) != s->graph)
        return bad;
    for (size_t i = 0; i < current_graph_count(s); ++i) {
        const StutterGraph *a = &s->graphs[i];
        if (current_graph_at(s, i) != a->fragment ||
            !linear_graph_matches(a->fragment, a->base, a->nodes, a->count,
                                  a->root) ||
            a->fragment->source != a->source ||
            a->fragment->context != a->world.world ||
            !current_signature_origin(s, a->world.world, i) ||
            !linear_world_matches(a->world.world, &a->world.storage,
                                  a->world.copy, &a->world.anchor) ||
            (s->helper && !current_helper_entry(s, a->world.world, i)))
            return bad;
    }
    if (s->terminal_call &&
        s->graphs[6].fragment->function_target != s->terminal_target)
        return bad;
    if (s->capture.call) {
        const CurrentCapturedCall *c = &s->capture;
        if (!nl_h_custody_result_valid(f, c->call) ||
            nl_checked_call_body(f, c->call) != c->fragments[0] ||
            c->fragments[0]->function_target != c->target ||
            c->worlds[0] != f->context)
            return bad;
        for (size_t i = 0; i < 3; ++i) {
            const NLCheckedFragment *fragment =
                i ? nl_checked_if_arm(c->fragments[0], c->pure.selection, i - 1)
                  : c->fragments[0];
            const size_t slot =
                i ? CURRENT_CAPTURE_SLOT + i - 1 : CURRENT_CALLER_SLOT;
            if (fragment != c->fragments[i] ||
                fragment->context != c->worlds[i] ||
                !linear_world_origin(c->worlds[i], &c->storage[i]) ||
                !current_signature_origin(s, c->worlds[i], slot) ||
                (i && fragment->function_target) || !c->pure.function ||
                c->pure.function > c->worlds[i]->function_count)
                return bad;
            const NLFunctionEntry *fn =
                &c->worlds[i]->functions[c->pure.function - 1];
            if (fn->kind != NL_CHECKED_REGISTERED_CALL || fn->count != 1 ||
                !fn->parameters ||
                fn->parameters[0] !=
                    nl_semantic_core_type(c->worlds[i], NL_TYPE_USIZE) ||
                fn->result != nl_semantic_core_type(c->worlds[i], NL_TYPE_U8) ||
                fn->caller_effects || fn->hidden_dependencies ||
                fn->owner_receiver || fn->owner_producer ||
                fn->custody_recipient || fn->body != c->plan ||
                c->plan->count != 1 || c->plan->syntax != c->syntax ||
                c->plan->source != c->source ||
                c->plan->parameter_names[0] != c->parameter_name ||
                c->plan->owner_definition.definition_checked ||
                c->plan->custody_definition.definition_checked)
                return bad;
        }
        NLPureU8Call pure_capture;
        if (!current_prefix_contents(s, c->call, &pure_capture) ||
            pure_capture.body != c->pure.body ||
            pure_capture.function != c->pure.function ||
            pure_capture.formal != c->pure.formal ||
            pure_capture.selection != c->pure.selection)
            return bad;
    }
    if (s->helper &&
        (!current_helper_entry(s, f->context, CURRENT_CALLER_SLOT) ||
         !current_helper_entry(s, p->post, CURRENT_POST_SLOT)))
        return bad;
    for (size_t i = 0; i < 4; ++i) {
        const NLControlExitView *x =
            i < 2 ? nl_control_exit_view(s->graphs[i + 1].fragment->exits, 0)
                  : nl_control_exit_view(s->graph->exits, i - 2);
        if (!x || memcmp(x, &s->exit_views[i], sizeof(*x)) ||
            nl_control_state_context((NLControlState *)x->state) !=
                s->exits[i].world ||
            !current_signature_origin(s, s->exits[i].world,
                                      CURRENT_EXIT_SLOT + i) ||
            !linear_world_matches(s->exits[i].world, &s->exits[i].storage,
                                  s->exits[i].copy, &s->exits[i].anchor))
            return bad;
    }
    if (s->helper) {
        const NLCheckedFragment *helper = current_graph_at(s, 3);
        if (!helper || helper->function_target != s->helper_target)
            return bad;
        for (size_t i = 0; i < s->return_count; ++i) {
            const CurrentReturnSeal *r = &s->returns[i];
            if (r->ref.owner->exits != r->ref.set ||
                r->ref.owner->function_target !=
                    (r->ref.owner == helper ? s->helper_target : NULL) ||
                nl_control_exits_count(r->ref.set) != r->set_count)
                return bad;
            const NLControlExitView *x =
                nl_control_exit_view(r->ref.set, r->ref.ordinal);
            if (!x || memcmp(x, &r->view, sizeof(*x)) ||
                nl_control_state_context((NLControlState *)x->state) !=
                    r->world.world ||
                !current_signature_origin(s, r->world.world,
                                          CURRENT_RETURN_SLOT + i) ||
                !linear_world_matches(r->world.world, &r->world.storage,
                                      r->world.copy, &r->world.anchor) ||
                !current_helper_entry(s, r->world.world,
                                      CURRENT_RETURN_SLOT + i))
                return bad;
        }
        CurrentReturnRef refs[4];
        size_t count;
        if (!nl_pure_u8_contents(s->graphs[1].fragment, s->scalar, &pure) ||
            !current_return_refs(s, &pure, refs, &count) ||
            count != s->return_count)
            return bad;
        for (size_t i = 0; i < count; ++i)
            if (refs[i].owner != s->returns[i].ref.owner ||
                refs[i].source != s->returns[i].ref.source ||
                refs[i].set != s->returns[i].ref.set ||
                refs[i].ordinal != s->returns[i].ref.ordinal ||
                refs[i].node != s->returns[i].ref.node)
                return bad;
    }
    const NLCheckedNodeView *n = nl_checked_node_view(f, p->loop),
                            *call = nl_checked_node_view(p->root, p->call);
    const NLSemanticContext *e = nl_control_state_context(s->entry),
                            *h = nl_control_state_context(s->header);
    size_t initial = 0;
    if (!n || !call || n->kind != NL_CHECKED_LOOP || !n->header_inductive ||
        n->continue_edges != 1 || n->break_edges != 1 || n->return_edges ||
        n->first_argument != s->initial_node || n->argument_count != 1 ||
        n->result_count != 1 || n->results[0].value != p->result.value ||
        n->results[0].type != p->result.type || p->target != s->target ||
        call->function != p->function || call->argument_count ||
        nl_checked_call_body(p->root, p->call) != f ||
        !current_copy_origin(f, f->context, s->initial_node, 0, true, 2,
                             &initial) ||
        initial != s->initial ||
        (s->terminal_old ? !current_terminal_post(s, p->post, p->result)
                         : (!current_projection(s, e, p->post) ||
                            !stutter_free_quantity(p->post, p->result.value,
                                                   p->result.type) ||
                            p->result.value != p->post->value_count)) ||
        !stutter_frame(e, nl_control_state_context(p->entry), 0) ||
        current_relation(s) != NL_CHECK_OK)
        return bad;
    const NLSemanticPlaceView r = h->places[s->root - 1],
                              c = h->places[s->child - 1];
    const NLCheckedFragment *breaking = s->graphs[2].fragment;
    NLPureU8Call terminal_pure = {0};
    if (s->terminal_call &&
        !nl_pure_u8_contents(breaking, s->terminal_call, &terminal_pure))
        return bad;
    const NLControlExitView *exit = nl_control_exit_view(breaking->exits, 0);
    const NLCheckedNodeView *terminal =
        nl_checked_node_view(breaking, exit->source_node);
    *out = (NLLinearBreakView){
        .base = s->base,
        .nominal = r.type,
        .root = s->root,
        .child = s->child,
        .incarnation = r.incarnation,
        .child_incarnation = c.incarnation,
        .index = s->index,
        .incoming = s->incoming,
        .result = p->result,
        .changing_current = true,
        .paired_current = s->paired,
        .terminal_old = s->terminal_old,
        .terminal_loan = s->terminal_loan,
        .terminal_call = s->terminal_call,
        .terminal_change = s->terminal_change,
        .incoming_if = s->helper ? 0 : s->scalar,
        .incoming_call = s->helper ? s->scalar : 0,
        .scalar_loop = !s->terminal_old,
        .iteration = s->graphs[1].fragment,
        .breaking = breaking,
        .break_value = terminal->initializer,
        .continue_node = s->terminal,
        .phase_symbol = s->phase,
        .initial_node = s->initial_node,
        .initial_phase = s->initial,
        .limit = s->limit,
        .next_phase = s->step,
        .entry_world = e,
        .post_world = p->post,
        .helper_body = pure.body,
        .helper_formal = pure.formal,
        .helper_selection = pure.selection,
        .terminal_helper_body = terminal_pure.body,
        .terminal_helper_formal = terminal_pure.formal,
        .terminal_helper_selection = terminal_pure.selection,
        .preloop_call = s->capture.call,
        .preloop_helper_body = s->capture.pure.body,
        .preloop_helper_formal = s->capture.pure.formal,
        .preloop_helper_selection = s->capture.pure.selection};
    return NL_CHECK_OK;
}

static NLCheckedNodeId source_current_loop(Check *check,
                                           const NLSyntaxView *syntax,
                                           const CurrentSource *source)
{
    NLCurrentLoopProof *s = calloc(1, sizeof(*s));
    if (!s) {
        (void)host(check, NL_CHECK_OUT_OF_MEMORY, syntax->span);
        return 0;
    }
    NLCheckedNodeId id = 0;
    NLControlTarget *target = NULL;
    NLSemanticContext *working = NULL;
    NLCheckedField field = {0};
    if (!fixed_selection(check, nl_syntax_node_view(source->field),
                         NL_ACCESS_WRITE, &field))
        goto cleanup;
    s->base = field.base;
    s->root = field.parent;
    s->child = field.child;
    s->index = field.index;
    if (source->second_field) {
        NLCheckedField second = {0};
        if (!fixed_selection(check, nl_syntax_node_view(source->second_field),
                             NL_ACCESS_WRITE, &second))
            goto cleanup;
        if (second.base != field.base || second.parent != field.parent ||
            second.nominal != field.nominal || second.index == field.index ||
            second.child == field.child || second.index >= 2 ||
            second.parent_incarnation != field.parent_incarnation) {
            loop_precision(check, syntax->span);
            goto cleanup;
        }
        s->paired = true;
        s->second_child = second.child;
        s->second_index = second.index;
    }
    s->parent = check->artifact;
    s->carry_source = source->carry;
    s->scalar_source = source->scalar;
    s->terminal_source = source->terminal_scalar;
    s->terminal_old = source->terminal_old;
    s->helper = source->scalar && nl_syntax_node_view(source->scalar)->kind ==
                                      NL_SYNTAX_EXPR_CALL;
    if (!current_packages(check->context, s->root) ||
        !check->context->types[field.nominal - 1].view.is_copy ||
        check->context->places[s->root - 1].fixed_field_count != 2) {
        loop_precision(check, syntax->span);
        goto cleanup;
    }
    const NLSyntaxView *param = nl_syntax_node_view(source->parameter);
    s->initial_node = expression(check, param->data.binding.initializer);
    if (!s->initial_node)
        goto cleanup;
    if (!current_copy_origin(check->artifact, check->context, s->initial_node,
                             0, true, 2, &s->initial)) {
        loop_precision(check, syntax->span);
        goto cleanup;
    }
    if (!host(check, nl_control_target_create(NL_TARGET_LOOP, &target),
              syntax->span) ||
        !host(check, nl_control_state_create(check->context, &s->entry),
              syntax->span) ||
        !host(check, nl_sem_clone(check->context, &working), syntax->span) ||
        !host(check, current_abstract(s, working), syntax->span) ||
        !host(check, nl_control_state_capture(s->entry, working, &s->header),
              syntax->span))
        goto cleanup;
    s->target = target;
    current_collect_capture(s);
    s->graph = calloc(1, sizeof(*s->graph));
    if (!s->graph) {
        (void)host(check, NL_CHECK_OUT_OF_MEMORY, syntax->span);
        goto cleanup;
    }
    *s->graph = (NLCheckedFragment){.source = check->source,
                                    .context = working,
                                    .destroy_context = nl_semantic_destroy};
    working = NULL;
    if (!host(check, nl_h_custody_attach(check->artifact, s->graph),
              syntax->span))
        goto cleanup;
    NLSemanticContext *world = (NLSemanticContext *)s->graph->context;
    LoopControl loop = {.source_parent = check->artifact,
                        .target = target,
                        .entry = s->header,
                        .current = s,
                        .count = 1,
                        .bindings = world->binding_count,
                        .places = world->place_count,
                        .scopes = world->scope_count};
    loop.types[0] = nl_semantic_core_type(world, NL_TYPE_USIZE);
    Check body = *check;
    body.context = world;
    body.artifact = s->graph;
    body.loop = &loop;
    body.binding_floor = body.arm_floor = loop.bindings;
    body.has_arm_floor = true;
    char *name = source_name_copy(&body, param->data.binding.name);
    const NLValueId phase = new_value(
        &body, (NLSemanticValueView){.type = loop.types[0]}, syntax->span);
    if (!name || !phase ||
        !host(
            &body,
            nl_sem_bind_in_scope(world, name, phase, loop.bindings, &s->phase),
            syntax->span)) {
        free(name);
        goto body_failure;
    }
    free(name);
    s->graph->root = expression(&body, syntax->data.loop.body);
    if (!s->graph->root || body.status != NL_CHECK_OK)
        goto body_failure;
    if (!body.terminated || current_relation(s) != NL_CHECK_OK) {
        loop_precision(check, syntax->span);
        goto cleanup;
    }
    LoopControl normal = {.source_parent = check->artifact, .target = target};
    if (!host(check, nl_control_state_fork(s->entry, &normal.entry),
              syntax->span))
        goto cleanup;
    if (s->terminal_old) {
        const NLCheckedFragment *breaking =
            nl_checked_if_arm(s->graph, s->conditional, 1);
        const NLControlExitView *exit =
            nl_control_exit_view(breaking->exits, 0);
        const NLSemanticContext *post =
            nl_control_state_context((NLControlState *)exit->state);
        NLSemanticContext *completed = NULL;
        if (!host(check, nl_sem_validate(post), syntax->span) ||
            !host(check, nl_fixed_dependencies(post), syntax->span) ||
            !host(check, nl_raw_validate(post), syntax->span) ||
            !host(check, nl_sem_clone(post, &completed), syntax->span))
            goto normal_cleanup;
        normal.normal_result = exit->values[0];
        if (!current_terminal_post(s, completed, normal.normal_result)) {
            nl_semantic_destroy(completed);
            loop_precision(check, syntax->span);
            goto normal_cleanup;
        }
        nl_sem_commit(check->context, completed);
        id = add(check, (NLCheckedNodeView){.kind = NL_CHECKED_LOOP,
                                            .span = syntax->span,
                                            .type = normal.normal_result.type,
                                            .result_count = 1,
                                            .results = {normal.normal_result},
                                            .header_inductive = true,
                                            .continue_edges = 1,
                                            .break_edges = 1,
                                            .argument_count = 1,
                                            .first_argument = s->initial_node});
    } else if (host(check, current_abstract(s, check->context), syntax->span)) {
        const NLValueId result = new_value(
            check, (NLSemanticValueView){.type = loop.types[0]}, syntax->span);
        normal.normal_result = (NLCheckedResult){loop.types[0], result};
        /* Recheck the real caller summary before even an unpublished
         * header_inductive node is built. The consumer checks it again. */
        if (result && !current_projection(s, nl_control_state_context(s->entry),
                                          check->context))
            loop_precision(check, syntax->span);
        if (result && check->status == NL_CHECK_OK)
            id = add(check,
                     (NLCheckedNodeView){.kind = NL_CHECKED_LOOP,
                                         .span = syntax->span,
                                         .type = loop.types[0],
                                         .result_count = 1,
                                         .results = {{loop.types[0], result}},
                                         .header_inductive = true,
                                         .continue_edges = 1,
                                         .break_edges = 1,
                                         .argument_count = 1,
                                         .first_argument = s->initial_node});
    }
    if (id && save_arm(check, id, s->graph, syntax->span)) {
        s->graph_owned = true;
        const NLCheckedFragment *breaking =
            nl_checked_if_arm(s->graph, s->conditional, 1);
        if (!host(check,
                  retain_current_break(check, &normal, id, breaking, NULL,
                                       s->terminal_call),
                  syntax->span))
            id = 0;
        /* Even a later failure belongs to the enclosing transaction. */
        for (NLLinearBreakProof *p = check->artifact->linear_breaks; p;
             p = p->next)
            if (p->loop == id || p->body == breaking) {
                p->current_cycle = s;
                s = NULL;
                break;
            }
    } else
        id = 0;
normal_cleanup:
    nl_control_state_destroy(normal.entry);
    goto cleanup;
body_failure:
    check->status = body.status;
    check->diagnostic = body.diagnostic;
    if (check->status == NL_CHECK_OK)
        loop_precision(check, syntax->span);
cleanup:
    nl_semantic_destroy(working);
    destroy_current_cycle(s);
    nl_control_target_destroy(target);
    return check->status == NL_CHECK_OK ? id : 0;
}
