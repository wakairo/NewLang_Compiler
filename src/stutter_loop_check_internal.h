/* Included in semantic_check.c: a finite relational transfer, not a generic
 * fixed-point engine. H = the genuine exact persistent prefix E times one
 * dependency-free usize slot. The COMPLETE body grammar below admits no
 * operation on E on the Continue path, for ANY bits in that slot. Both IF
 * outcomes are checked from an Unknown slot. B is phase independent and is
 * applied once to the real caller transaction by the existing normal kernel.
 */
typedef struct {
    const NLSemanticContext *world;
    NLSemanticContext storage, *copy;
    NLCopyCallAnchor anchor;
} StutterWorld;
typedef struct {
    const NLCheckedFragment *fragment;
    const NLCheckedNodeView *base;
    NLCheckedNodeView *nodes;
    size_t count;
    NLCheckedNodeId root;
    const NLSource *source;
    StutterWorld world;
} StutterGraph;
typedef struct NLStutterProof {
    NLControlState *entry; /* private owned H prefix; never a public receipt */
    const NLControlTarget *target; /* canonical exits own it */
    NLCheckedFragment *graph;
    const NLSyntaxNode *break_source, *guard_source, *carry_source;
    const NLCheckedFragment *quantity_origin;
    NLCheckedNodeId quantity_node;
    NLSymbolId parameter;
    NLCheckedNodeId conditional, initial_node;
    size_t initial, next, limit, continue_arm;
    bool less, computed, subtract, scalar, graph_owned, sealed, definition;
    StutterGraph graphs[3];
    StutterWorld exits[4]; /* both leaf and both union-owned snapshots */
    NLControlExitView exit_views[4];
} NLStutterProof;

static bool stutter_scalar(const NLLinearBreakProof *p)
{
    return p && p->cycle && p->cycle->scalar;
}

static const NLCheckedFragment *stutter_graph(const NLLinearBreakProof *p)
{
    return p && p->cycle ? p->cycle->graph : NULL;
}
static void destroy_stutter_cycle(NLStutterProof *s)
{
    if (!s)
        return;
    for (size_t i = 0; i < 3; ++i) {
        free(s->graphs[i].nodes);
        nl_copy_state_destroy(&s->graphs[i].world.anchor);
        nl_semantic_destroy(s->graphs[i].world.copy);
    }
    for (size_t i = 0; i < 4; ++i) {
        nl_copy_state_destroy(&s->exits[i].anchor);
        nl_semantic_destroy(s->exits[i].copy);
    }
    nl_control_state_destroy(s->entry);
    if (!s->graph_owned)
        nl_checked_destroy(s->graph);
    free(s);
}
static bool save_stutter_graph(Check *check, NLCheckedNodeId id,
                               NLStutterProof *s, NLSourceSpan span)
{
    if (!save_arm(check, id, s->graph, span))
        return false;
    s->graph_owned = true;
    return true;
}
static bool stutter_name(Check *c, NLSourceSpan a, NLSourceSpan b)
{
    NLSourceView x, y;
    return nl_source_view(c->source, a, &x) &&
           nl_source_view(c->source, b, &y) && x.length == y.length &&
           x.length && !memcmp(x.bytes, y.bytes, x.length);
}
static const NLSyntaxNode *stutter_continue(const NLSyntaxNode *node)
{
    const NLSyntaxView *b = nl_syntax_node_view(node);
    if (!b || b->kind != NL_SYNTAX_BLOCK)
        return NULL;
    const NLSyntaxView *c = nl_syntax_node_view(b->data.block.items);
    if (b->data.block.tail || b->data.block.item_count != 1 || !c ||
        c->kind != NL_SYNTAX_CONTINUE || c->data.call.argument_count != 1 ||
        nl_syntax_next_argument(b->data.block.items))
        return NULL;
    const NLSyntaxView *v = nl_syntax_node_view(c->data.call.arguments);
    return v && (v->kind == NL_SYNTAX_USIZE_LITERAL ||
                 v->kind == NL_SYNTAX_USIZE_ADDITION ||
                 v->kind == NL_SYNTAX_USIZE_SUBTRACTION)
               ? c->data.call.arguments
               : NULL;
}
static bool stutter_scalar_source(Check *c, const NLSyntaxNode *node,
                                  NLSourceSpan parameter)
{
    const NLSyntaxView *b = nl_syntax_node_view(node);
    const NLSyntaxView *n = b && b->kind == NL_SYNTAX_BLOCK
                                ? nl_syntax_node_view(b->data.block.items)
                                : NULL;
    const NLSyntaxView *v =
        n && n->kind == NL_SYNTAX_BREAK
            ? nl_syntax_node_view(n->data.statement.expression)
            : NULL;
    return b && n && v && !b->data.block.tail &&
           b->data.block.item_count == 1 &&
           !nl_syntax_next_argument(b->data.block.items) &&
           v->kind == NL_SYNTAX_EXPR_NAME &&
           stutter_name(c, v->data.name, parameter);
}
static bool stutter_source_shape(Check *c, const NLSyntaxView *s)
{
    if (s->data.loop.count != 1 || c->loop || c->in_source_loan ||
        c->in_match_arm || c->capture_copy_body || c->allocated_slice ||
        c->artifact->if_parent || c->artifact->node_join_parent)
        return false;
    const NLSyntaxView *param = nl_syntax_node_view(s->data.loop.parameters);
    const NLSyntaxView *init =
        param ? nl_syntax_node_view(param->data.binding.initializer) : NULL;
    const NLSyntaxView *block = nl_syntax_node_view(s->data.loop.body);
    if (!param || !init || init->kind != NL_SYNTAX_USIZE_LITERAL || !block ||
        block->kind != NL_SYNTAX_BLOCK || block->data.block.items ||
        !block->data.block.tail)
        return false;
    const NLSyntaxView *cond = nl_syntax_node_view(block->data.block.tail);
    if (!cond || cond->kind != NL_SYNTAX_IF)
        return false;
    const NLSyntaxView *op =
        nl_syntax_node_view(cond->data.conditional.condition);
    if (!op || (op->kind != NL_SYNTAX_USIZE_EQUALITY &&
                op->kind != NL_SYNTAX_USIZE_LESS_THAN))
        return false;
    const NLSyntaxView *a = nl_syntax_node_view(op->data.equality.left),
                       *b = nl_syntax_node_view(op->data.equality.right);
    if (!a || !b || a->kind != NL_SYNTAX_EXPR_NAME ||
        !stutter_name(c, a->data.name, param->data.binding.name) ||
        b->kind != NL_SYNTAX_USIZE_LITERAL)
        return false;
    const NLSyntaxNode *arms[2] = {cond->data.conditional.then_block,
                                   cond->data.conditional.else_block};
    const bool first = stutter_continue(arms[0]) != NULL;
    if (first == (stutter_continue(arms[1]) != NULL))
        return false;
    const NLSyntaxView *carry =
        nl_syntax_node_view(stutter_continue(arms[first ? 0 : 1]));
    if (carry->kind != NL_SYNTAX_USIZE_LITERAL) {
        const NLSyntaxView *lhs =
            nl_syntax_node_view(carry->data.equality.left);
        const NLSyntaxView *rhs =
            nl_syntax_node_view(carry->data.equality.right);
        if (op->kind != NL_SYNTAX_USIZE_LESS_THAN || !lhs || !rhs ||
            lhs->kind != NL_SYNTAX_EXPR_NAME ||
            !stutter_name(c, lhs->data.name, param->data.binding.name) ||
            rhs->kind != NL_SYNTAX_USIZE_LITERAL)
            return false;
    }
    NLSyntaxView normal = *s;
    normal.data.loop.count = 0;
    normal.data.loop.body = arms[first ? 1 : 0];
    if (stutter_scalar_source(c, normal.data.loop.body,
                              param->data.binding.name))
        return true;
    if (!linear_break_source(c, &normal))
        return false;
    /* No phase capture or incoming-name shadow ambiguity in the normal arm.
     * Its only outer operation is a declared-field scoped Change. */
    const NLSyntaxView *bb = nl_syntax_node_view(normal.data.loop.body);
    const NLSyntaxView *item = nl_syntax_node_view(bb->data.block.items);
    const NLSyntaxView *loan = nl_syntax_node_view(
        item->kind == NL_SYNTAX_BINDING ? item->data.binding.initializer
                                        : item->data.statement.expression);
    const NLSyntaxView *inner = nl_syntax_node_view(loan->data.loan.body);
    const NLSyntaxView *call = nl_syntax_node_view(inner->data.block.tail);
    const NLSyntaxView *incoming =
        nl_syntax_node_view(nl_syntax_next_argument(call->data.call.arguments));
    return incoming && incoming->kind == NL_SYNTAX_U8_LITERAL;
}

/* Exact persistent semantic frame, not just bytes/scalar bits. The entry was
 * captured from this source transaction, and the projection retains that
 * origin. Extra iteration values must be flat free temporaries; no nonflat
 * package, loan or hidden dependency may be introduced on C. A fresh scalar
 * binding installs one lexical place and consumes one incarnation/fact ID;
 * projection ends that place but must not rewind either allocator. Require
 * exactly that finite delta, while every persistent place/fact stays exact. */
static bool stutter_frame(const NLSemanticContext *a,
                          const NLSemanticContext *b, size_t fresh_bindings)
{
    if (!a || !b || a->binding_count != b->binding_count ||
        a->place_count != b->place_count || a->scope_count != b->scope_count ||
        a->value_count > b->value_count || a->domain_count || b->domain_count ||
        a->region_count || b->region_count || a->occurrence_count ||
        b->occurrence_count || a->function_count != b->function_count ||
        a->last_incarnation > b->last_incarnation ||
        b->last_incarnation - a->last_incarnation != fresh_bindings ||
        a->last_value_fact > b->last_value_fact ||
        b->last_value_fact - a->last_value_fact != fresh_bindings ||
        !nl_copy_same_types(a, b))
        return false;
    for (size_t i = 0; i < a->binding_count; ++i) {
        const NLBindingEntry *x = &a->bindings[i], *y = &b->bindings[i];
        if (x->hidden != y->hidden || strcmp(x->name, y->name) ||
            x->view.type != y->view.type || x->view.value != y->view.value ||
            x->view.place != y->view.place ||
            x->view.availability != y->view.availability)
            return false;
    }
    for (size_t i = 0; i < a->place_count; ++i)
        if (!same_place_frame(a->places[i], b->places[i]) ||
            a->places[i].current_fact != b->places[i].current_fact ||
            a->places[i].current_value != b->places[i].current_value ||
            a->places[i].payload_occurrence != b->places[i].payload_occurrence)
            return false;
    for (size_t i = 0; i < a->scope_count; ++i)
        if (!same_scope(a->scopes[i], b->scopes[i]))
            return false;
    for (size_t i = 0; i < a->value_count; ++i)
        if (!nl_node_value_same(a->values[i], b->values[i]))
            return false;
    for (size_t i = a->value_count; i < b->value_count; ++i) {
        const NLSemanticValueView *v = &b->values[i];
        if (v->carrier != NL_CARRIER_ENDED &&
            (!flat_copy(b, v->type) || v->dependencies != NL_DEPENDENCY_FREE ||
             v->value_dependency_count))
            return false;
    }
    return true;
}
static bool stutter_literal(const NLCheckedNodeView *n, NLTypeId type,
                            size_t value)
{
    return n && n->kind == NL_CHECKED_USIZE_LITERAL && n->type == type &&
           n->result_count == 1 && n->results[0].type == type &&
           n->has_scalar_result && n->scalar_result.known &&
           n->scalar_result.value == value;
}

static bool stutter_span(NLSourceSpan a, NLSourceSpan b)
{
    return a.start_byte == b.start_byte && a.end_byte == b.end_byte;
}
static bool stutter_free_quantity(const NLSemanticContext *c, NLValueId id,
                                  NLTypeId type)
{
    NLSemanticValueView v;
    return nl_semantic_value_view(c, id, &v) &&
           (v.carrier == NL_CARRIER_LOOSE || v.carrier == NL_CARRIER_ENDED) &&
           nl_node_value_same(
               v, (NLSemanticValueView){.type = type, .carrier = v.carrier});
}
/* This is a finite representability lemma, independent of the initial bits:
 * C=true  and x<L: x<=L-1, so k<=MAX-(L-1) proves x+k<=MAX.
 * C=false and x<L: x>=L,   so k<=L proves x-k>=0.
 * Each successor is again an arbitrary free usize in H. No range is granted
 * to another source/arm, and no known-number public receipt is issued. */
static bool stutter_quantity_bound(const NLStutterProof *s)
{
    return s->computed && s->less && s->limit &&
           (s->subtract
                ? s->continue_arm == 1 && s->next <= s->limit
                : s->continue_arm == 0 && finite_add_bound(s->limit, s->next));
}
static bool stutter_quantity(Check *c, const NLSyntaxNode *source,
                             NLCheckedNodeId left, NLCheckedNodeId right)
{
    NLStutterProof *s = c->loop ? c->loop->stutter : NULL;
    if (!s || !s->computed || source != s->carry_source || s->quantity_node ||
        c->loop->target != s->target || !c->loop->stuttering ||
        c->artifact->if_parent != s->graph ||
        c->artifact->if_arm_index != s->continue_arm ||
        c->artifact->source != s->graph->source ||
        c->source != s->graph->source || c->context == s->graph->context)
        return false;
    const NLCheckedNodeId branch = c->artifact->if_parent_node;
    const NLCheckedNodeView *choice = nl_checked_node_view(s->graph, branch);
    const NLCheckedNodeView *test =
        choice ? nl_checked_node_view(s->graph, choice->initializer) : NULL;
    const NLCheckedNodeView *guard_left =
        test ? nl_checked_node_view(s->graph, test->first_argument) : NULL;
    const NLCheckedNodeView *guard_right =
        guard_left ? nl_checked_node_view(s->graph, guard_left->next_argument)
                   : NULL;
    const NLSyntaxView *guard = nl_syntax_node_view(s->guard_source);
    const NLSyntaxView *syntax = nl_syntax_node_view(source);
    const NLCheckedNodeView *lhs = view(c, left), *rhs = view(c, right);
    const NLTypeId type = nl_semantic_core_type(c->context, NL_TYPE_USIZE);
    if (!choice || choice->kind != NL_CHECKED_IF || !test ||
        test->kind != NL_CHECKED_USIZE_LESS_THAN || !guard ||
        !stutter_span(test->span, guard->span) ||
        !stutter_span(test->name, guard->data.equality.operator_span) ||
        !guard_left || guard_left->kind != NL_CHECKED_IDENTIFIER ||
        guard_left->symbol != s->parameter || !guard_right ||
        !guard_right->has_scalar_result ||
        !stutter_literal(guard_right, type, guard_right->scalar_result.value) ||
        guard_right->next_argument || !lhs || !rhs ||
        lhs->kind != NL_CHECKED_IDENTIFIER || lhs->symbol != s->parameter ||
        lhs->value_use != NL_VALUE_COPIED || lhs->result_count != 1 ||
        !stutter_span(lhs->span,
                      nl_syntax_node_view(syntax->data.equality.left)->span) ||
        !stutter_free_quantity(c->context, lhs->results[0].value, type) ||
        !rhs->has_scalar_result ||
        !stutter_literal(rhs, type, rhs->scalar_result.value) ||
        !s->parameter || s->parameter > c->context->binding_count ||
        s->parameter > s->graph->context->binding_count)
        return false;
    const NLSemanticBindingView a =
        s->graph->context->bindings[s->parameter - 1].view;
    const NLSemanticBindingView b = c->context->bindings[s->parameter - 1].view;
    NLSemanticPlaceView before, now;
    if (a.type != type || b.type != type || a.value != b.value ||
        a.place != b.place || a.availability != NL_AVAILABLE ||
        b.availability != NL_AVAILABLE ||
        !nl_semantic_place_view(s->graph->context, a.place, &before) ||
        !nl_semantic_place_view(c->context, b.place, &now) || !before.live ||
        !now.live || !same_place_frame(before, now) ||
        before.current_value != a.value || now.current_value != b.value ||
        !before.current_fact || before.current_fact != now.current_fact ||
        !stutter_free_quantity(s->graph->context, guard_left->results[0].value,
                               type))
        return false;
    s->conditional = branch;
    s->limit = guard_right->scalar_result.value;
    s->next = rhs->scalar_result.value; /* step, never a known successor */
    return stutter_quantity_bound(s);
}
static void stutter_quantity_record(Check *c, NLCheckedNodeId node)
{
    NLStutterProof *s = c->loop->stutter;
    s->quantity_origin = c->artifact;
    s->quantity_node = node;
}

/* Recheck the actual private issued operation, not a caller's arithmetic
 * view. The parent/arm/world/node seals additionally bind every byte and the
 * original canonical child. The exhaustive grammar admits no phase writer
 * between the guard read and this read of the SAME fresh binding. */
static bool stutter_computed(const NLStutterProof *s,
                             const NLCheckedFragment *arm,
                             const NLCheckedNodeView *operation,
                             NLCheckedNodeId node, NLTypeId type,
                             NLCheckedResult result)
{
    const NLCheckedNodeView *lhs =
        operation ? nl_checked_node_view(arm, operation->first_argument) : NULL;
    const NLCheckedNodeView *rhs =
        lhs ? nl_checked_node_view(arm, lhs->next_argument) : NULL;
    const NLSyntaxView *source = nl_syntax_node_view(s->carry_source);
    return stutter_quantity_bound(s) && s->quantity_origin == arm &&
           s->quantity_node == node && operation && lhs && rhs && source &&
           operation->kind == (s->subtract ? NL_CHECKED_USIZE_SUBTRACTION
                                           : NL_CHECKED_USIZE_ADDITION) &&
           operation->type == type && operation->result_count == 1 &&
           operation->argument_count == 2 && !operation->terminates &&
           operation->has_scalar_result && !operation->scalar_result.known &&
           operation->scalar_result.type == type &&
           stutter_span(operation->span, source->span) &&
           stutter_span(operation->name, source->data.equality.operator_span) &&
           operation->results[0].type == type && result.type == type &&
           operation->results[0].value == result.value &&
           stutter_free_quantity(arm->context, result.value, type) &&
           lhs->kind == NL_CHECKED_IDENTIFIER && lhs->symbol == s->parameter &&
           lhs->value_use == NL_VALUE_COPIED && lhs->type == type &&
           lhs->result_count == 1 && lhs->results[0].type == type &&
           stutter_free_quantity(arm->context, lhs->results[0].value, type) &&
           stutter_literal(rhs, type, s->next) && !rhs->next_argument;
}

/* An exhaustive graph grammar is the all-H transfer argument. The only
 * predicate inputs are the fresh free usize binding and a literal; C only
 * constructs a literal, ends the fresh scope and transfers it. No operation
 * capable of changing any persistent component of E occurs on this path.
 * B is checked separately and never fed back to H. */
static NLCheckStatus stutter_relation(const NLStutterProof *s,
                                      const NLCheckedFragment *parent,
                                      LinearBreakEffect *effect)
{
    const NLCheckStatus bad = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLCheckedFragment *g = s->graph;
    const NLSemanticContext *e = nl_control_state_context(s->entry);
    const NLTypeId usize = nl_semantic_core_type(e, NL_TYPE_USIZE);
    const NLCheckedNodeView *block = nl_checked_node_view(g, g->root);
    const NLCheckedNodeView *conditional =
        block ? nl_checked_node_view(g, block->tail) : NULL;
    const NLCheckedNodeView *test =
        conditional ? nl_checked_node_view(g, conditional->initializer) : NULL;
    const NLCheckedNodeView *lhs =
        test ? nl_checked_node_view(g, test->first_argument) : NULL;
    const NLCheckedNodeView *rhs =
        lhs ? nl_checked_node_view(g, lhs->next_argument) : NULL;
    if (!block || block->kind != NL_CHECKED_BLOCK || block->item_count ||
        !conditional || conditional->kind != NL_CHECKED_IF ||
        block->tail != s->conditional || conditional->normal_arms ||
        !conditional->terminates || !test || test->argument_count != 2 ||
        test->kind != (s->less ? NL_CHECKED_USIZE_LESS_THAN
                               : NL_CHECKED_USIZE_EQUALITY) ||
        !lhs || lhs->kind != NL_CHECKED_IDENTIFIER ||
        lhs->symbol != s->parameter || lhs->value_use != NL_VALUE_COPIED ||
        lhs->type != usize || lhs->result_count != 1 ||
        !stutter_literal(rhs, usize, s->limit) || rhs->next_argument ||
        nl_control_exits_count(g->exits) != 2 ||
        s->parameter != e->binding_count + 1 ||
        g->context->binding_count != e->binding_count + 1)
        return bad;
    NLSemanticValueView phase;
    const NLValueId phase_id =
        g->context->bindings[s->parameter - 1].view.value;
    if (!nl_semantic_value_view(g->context, phase_id, &phase) ||
        phase.type != usize || phase.scalar_known ||
        phase.dependencies != NL_DEPENDENCY_FREE ||
        phase.value_dependency_count)
        return bad;
    size_t kinds[5] = {0};
    for (size_t i = 1; i <= g->count; ++i) {
        switch (nl_checked_node_view(g, i)->kind) {
        case NL_CHECKED_BLOCK:
            ++kinds[0];
            break;
        case NL_CHECKED_IF:
            ++kinds[1];
            break;
        case NL_CHECKED_IDENTIFIER:
            ++kinds[2];
            break;
        case NL_CHECKED_USIZE_LITERAL:
            ++kinds[3];
            break;
        case NL_CHECKED_USIZE_EQUALITY:
        case NL_CHECKED_USIZE_LESS_THAN:
            ++kinds[4];
            break;
        default:
            return bad;
        }
    }
    for (size_t i = 0; i < 5; ++i)
        if (kinds[i] != 1)
            return bad;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *a = nl_checked_if_arm(g, s->conditional, i);
        const NLControlExitView *x =
            a ? nl_control_exit_view(a->exits, 0) : NULL;
        const NLControlExitView *u = nl_control_exit_view(g->exits, i);
        const NLControlExitKind kind =
            i == s->continue_arm ? NL_EXIT_CONTINUE : NL_EXIT_BREAK;
        if (!a || a->source != parent->source || a->if_parent != g ||
            a->if_parent_node != s->conditional || a->if_arm_index != i ||
            !nl_control_source_terminal_valid(a, s->target, kind, parent) ||
            !x || !u || x->target != s->target || u->target != s->target ||
            u->kind != kind || u->count != 1 || u->source_owner != a ||
            u->source_node != x->source_node || u->source_target != s->target ||
            u->values[0].type != x->values[0].type ||
            u->values[0].value != x->values[0].value)
            return bad;
        if (kind == NL_EXIT_CONTINUE) {
            const NLCheckedNodeView *n =
                nl_checked_node_view(a, x->source_node);
            const NLCheckedNodeView *v =
                nl_checked_node_view(a, n->initializer);
            if (!(s->computed ? stutter_computed(s, a, v, n->initializer, usize,
                                                 x->values[0])
                              : stutter_literal(v, usize, s->next)) ||
                !stutter_frame(
                    e, nl_control_state_context((NLControlState *)x->state),
                    1) ||
                !stutter_frame(
                    e, nl_control_state_context((NLControlState *)u->state), 1))
                return bad;
            size_t nc = 0, nl = 0, nb = 0, na = 0, ni = 0, nq = 0;
            for (size_t j = 1; j <= a->count; ++j)
                switch (nl_checked_node_view(a, j)->kind) {
                case NL_CHECKED_CONTINUE:
                    ++nc;
                    break;
                case NL_CHECKED_USIZE_LITERAL:
                    ++nl;
                    break;
                case NL_CHECKED_BLOCK:
                    ++nb;
                    break;
                case NL_CHECKED_IF_ARM:
                    ++na;
                    break;
                case NL_CHECKED_IDENTIFIER:
                    ++ni;
                    break;
                case NL_CHECKED_USIZE_ADDITION:
                case NL_CHECKED_USIZE_SUBTRACTION:
                    ++nq;
                    break;
                default:
                    return bad;
                }
            if (nc != 1 || nl != 1 || nb != 1 || na != 1 ||
                ni != (size_t)s->computed || nq != (size_t)s->computed)
                return bad;
        } else if (s->scalar) {
            const NLCheckedNodeView *n =
                nl_checked_node_view(a, x->source_node);
            const NLCheckedNodeView *v =
                n ? nl_checked_node_view(a, n->initializer) : NULL;
            if (!n || n->kind != NL_CHECKED_BREAK || !v ||
                v->kind != NL_CHECKED_IDENTIFIER || v->symbol != s->parameter ||
                v->value_use != NL_VALUE_COPIED || v->type != usize ||
                v->result_count != 1 || v->results[0].type != usize ||
                x->values[0].type != usize ||
                v->results[0].value != x->values[0].value ||
                !stutter_free_quantity(a->context, v->results[0].value,
                                       usize) ||
                !stutter_free_quantity(
                    nl_control_state_context((NLControlState *)x->state),
                    x->values[0].value, usize) ||
                !stutter_frame(
                    e, nl_control_state_context((NLControlState *)x->state),
                    1) ||
                !stutter_frame(
                    e, nl_control_state_context((NLControlState *)u->state), 1))
                return bad;
            size_t nb = 0, na = 0, ni = 0, nx = 0;
            for (size_t j = 1; j <= a->count; ++j)
                switch (nl_checked_node_view(a, j)->kind) {
                case NL_CHECKED_BLOCK:
                    ++nb;
                    break;
                case NL_CHECKED_IF_ARM:
                    ++na;
                    break;
                case NL_CHECKED_IDENTIFIER:
                    ++ni;
                    break;
                case NL_CHECKED_BREAK:
                    ++nx;
                    break;
                default:
                    return bad;
                }
            if (nb != 1 || na != 1 || ni != 1 || nx != 1)
                return bad;
        } else {
            NLCheckStatus r = linear_break_relation(
                a, a->context, s->entry, s->target, x->values[0], s->definition,
                effect, parent);
            if (r != NL_CHECK_OK)
                return r;
        }
    }
    return NL_CHECK_OK;
}

static NLCheckStatus stutter_anchor_world(StutterWorld *a,
                                          const NLSemanticContext *world)
{
    a->world = world;
    a->storage = *world;
    NLCheckStatus r = nl_sem_clone(world, &a->copy);
    return r == NL_CHECK_OK ? nl_copy_state_anchor(world, &a->anchor) : r;
}
static NLCheckStatus seal_stutter_cycle(NLLinearBreakProof *p)
{
    NLStutterProof *s = p->cycle;
    for (size_t i = 0; i < 3; ++i) {
        StutterGraph *a = &s->graphs[i];
        a->fragment =
            i ? nl_checked_if_arm(s->graph, s->conditional, i - 1) : s->graph;
        if (!a->fragment)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        a->base = a->fragment->nodes;
        a->count = a->fragment->count;
        a->root = a->fragment->root;
        a->source = a->fragment->source;
        NLCheckStatus r = linear_seal_graph(a->fragment, &a->nodes);
        if (r == NL_CHECK_OK)
            r = stutter_anchor_world(&a->world, a->fragment->context);
        if (r != NL_CHECK_OK)
            return r;
    }
    for (size_t i = 0; i < 4; ++i) {
        const NLControlExitView *x =
            i < 2 ? nl_control_exit_view(s->graphs[i + 1].fragment->exits, 0)
                  : nl_control_exit_view(s->graph->exits, i - 2);
        if (!x)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        memcpy(&s->exit_views[i], x, sizeof(*x));
        NLCheckStatus r = stutter_anchor_world(
            &s->exits[i], nl_control_state_context((NLControlState *)x->state));
        if (r != NL_CHECK_OK)
            return r;
    }
    s->sealed = true;
    return NL_CHECK_OK;
}
static NLCheckStatus validate_stutter_cycle(const NLLinearBreakProof *p,
                                            NLLinearBreakView *out)
{
    const NLCheckStatus bad = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLStutterProof *s = p->cycle;
    const NLCheckedNodeView *loop = nl_checked_node_view(p->owner, p->loop);
    const NLCheckedNodeView *initial =
        nl_checked_node_view(p->owner, s->initial_node);
    if (!s->sealed || !s->graph_owned || s->definition || !loop ||
        loop->kind != NL_CHECKED_LOOP || loop->argument_count != 1 ||
        loop->continue_edges != 1 || loop->break_edges != 1 ||
        loop->return_edges || !loop->header_inductive ||
        loop->first_argument != s->initial_node || p->target != s->target ||
        nl_checked_loop_body(p->owner, p->loop) != s->graph ||
        (s->scalar ? (p->body != nl_checked_if_arm(s->graph, s->conditional,
                                                   1 - s->continue_arm) ||
                      nl_checked_match_arm(p->owner, p->loop, 1))
                   : (nl_checked_match_arm(p->owner, p->loop, 1) != p->body ||
                      nl_checked_match_arm(p->owner, p->loop, 2))) ||
        !stutter_literal(initial,
                         nl_semantic_core_type(p->world, NL_TYPE_USIZE),
                         s->initial) ||
        !stutter_frame(nl_control_state_context(s->entry),
                       nl_control_state_context(p->entry), 0))
        return bad;
    for (size_t i = 0; i < 3; ++i) {
        const StutterGraph *a = &s->graphs[i];
        if ((i && nl_checked_if_arm(s->graph, s->conditional, i - 1) !=
                      a->fragment) ||
            a->fragment->source != a->source ||
            !linear_graph_matches(a->fragment, a->base, a->nodes, a->count,
                                  a->root) ||
            a->fragment->context != a->world.world ||
            !linear_world_matches(a->world.world, &a->world.storage,
                                  a->world.copy, &a->world.anchor))
            return bad;
    }
    for (size_t i = 0; i < 4; ++i) {
        const NLControlExitView *x =
            i < 2 ? nl_control_exit_view(s->graphs[i + 1].fragment->exits, 0)
                  : nl_control_exit_view(s->graph->exits, i - 2);
        if (!x || memcmp(x, &s->exit_views[i], sizeof(*x)) ||
            nl_control_state_context((NLControlState *)x->state) !=
                s->exits[i].world ||
            !linear_world_matches(s->exits[i].world, &s->exits[i].storage,
                                  s->exits[i].copy, &s->exits[i].anchor))
            return bad;
    }
    LinearBreakEffect effect;
    NLCheckStatus r = stutter_relation(s, p->owner, &effect);
    if (r != NL_CHECK_OK)
        return r;
    const NLCheckedField *f = &effect.field;
    if (!s->scalar &&
        (f->parent != out->root || f->child != out->child ||
         f->nominal != out->nominal || f->index != out->index ||
         f->parent_incarnation != out->incarnation ||
         f->child_incarnation != out->child_incarnation || !effect.incoming ||
         effect.incoming->kind != NL_CHECKED_U8_LITERAL ||
         effect.incoming->scalar_result.value != out->incoming))
        return bad;
    out->stuttering = true;
    out->less = s->less;
    out->continue_on_true = s->continue_arm == 0;
    out->computed_carry = s->computed;
    out->subtract_carry = s->subtract;
    out->initial_phase = s->initial;
    out->next_phase = s->next;
    out->limit = s->limit;
    return NL_CHECK_OK;
}

/* A new free Copy package represents the actual unknown normal result.
 * No arm-world value ID or memory authority is copied into the caller. */
static NLCheckStatus validate_scalar_cycle(const NLLinearBreakProof *p,
                                           const NLCheckedFragment *f,
                                           NLLinearBreakView *out)
{
    const NLCheckStatus bad = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    if (!p->sealed || p->owner != f || f->context != p->world ||
        f->source != p->source || f->body_owner != p->plan ||
        !nl_h_custody_validate(p->root) ||
        !linear_graph_matches(p->root, p->root_origin, p->root_nodes,
                              p->root_count, p->root_root) ||
        !linear_graph_matches(f, p->parent_origin, p->parent_nodes,
                              p->parent_count, p->parent_root) ||
        !linear_graph_matches(p->body, p->body_origin, p->body_nodes,
                              p->body_count, p->body_root) ||
        !linear_world_matches(f->context, &p->final_storage, p->final,
                              &p->final_anchor) ||
        !linear_callable(p, f->context) || !linear_callable(p, p->post) ||
        nl_checked_loop_body(f, p->loop) != p->cycle->graph)
        return bad;
    const NLCheckedNodeView *n = nl_checked_node_view(f, p->loop);
    const NLTypeId type = nl_semantic_core_type(p->world, NL_TYPE_USIZE);
    const NLSemanticContext *entry = nl_control_state_context(p->entry);
    if (!n || n->type != type || n->result_count != 1 ||
        n->results[0].type != type || n->results[0].value != p->result.value ||
        p->result.type != type || p->result.value != entry->value_count + 1 ||
        p->post->value_count != entry->value_count + 1 ||
        !stutter_frame(entry, p->post, 0) ||
        !stutter_free_quantity(p->post, p->result.value, type) ||
        !stutter_free_quantity(f->context, p->result.value, type))
        return bad;
    *out = (NLLinearBreakView){.scalar_loop = true, .result = p->result};
    return validate_stutter_cycle(p, out);
}

static NLCheckedNodeId source_stutter_loop(Check *check,
                                           const NLSyntaxView *syntax)
{
    NLStutterProof *s = calloc(1, sizeof(*s));
    if (!s) {
        (void)host(check, NL_CHECK_OUT_OF_MEMORY, syntax->span);
        return 0;
    }
    NLControlTarget *target = NULL;
    NLCheckedNodeId id = 0;
    const NLSyntaxView *param =
        nl_syntax_node_view(syntax->data.loop.parameters);
    const NLSyntaxView *block = nl_syntax_node_view(syntax->data.loop.body);
    const NLSyntaxView *conditional =
        nl_syntax_node_view(block->data.block.tail);
    const NLSyntaxView *op =
        nl_syntax_node_view(conditional->data.conditional.condition);
    const NLSyntaxNode *arms[2] = {conditional->data.conditional.then_block,
                                   conditional->data.conditional.else_block};
    s->continue_arm = stutter_continue(arms[0]) ? 0 : 1;
    s->break_source = arms[1 - s->continue_arm];
    s->scalar =
        stutter_scalar_source(check, s->break_source, param->data.binding.name);
    s->guard_source = conditional->data.conditional.condition;
    s->carry_source = stutter_continue(arms[s->continue_arm]);
    const NLSyntaxView *carry = nl_syntax_node_view(s->carry_source);
    s->computed = carry->kind != NL_SYNTAX_USIZE_LITERAL;
    s->subtract = carry->kind == NL_SYNTAX_USIZE_SUBTRACTION;
    s->less = op->kind == NL_SYNTAX_USIZE_LESS_THAN;
    s->definition = check->definition;
    s->initial_node = expression(check, param->data.binding.initializer);
    if (!s->initial_node)
        goto cleanup;
    s->initial = view(check, s->initial_node)->scalar_result.value;
    if (!host(check, nl_control_target_create(NL_TARGET_LOOP, &target),
              syntax->span) ||
        !host(check, nl_control_state_create(check->context, &s->entry),
              syntax->span))
        goto cleanup;
    s->target = target;
    s->graph = calloc(1, sizeof(*s->graph));
    if (!s->graph) {
        (void)host(check, NL_CHECK_OUT_OF_MEMORY, syntax->span);
        goto cleanup;
    }
    NLSemanticContext *world = NULL;
    if (!host(check, nl_sem_clone(check->context, &world), syntax->span))
        goto cleanup;
    *s->graph = (NLCheckedFragment){.source = check->source,
                                    .context = world,
                                    .destroy_context = nl_semantic_destroy};
    if (!host(check, nl_h_custody_attach(check->artifact, s->graph),
              syntax->span))
        goto cleanup;
    LoopControl loop = {.source_parent = check->artifact,
                        .target = target,
                        .entry = s->entry,
                        .count = 1,
                        .bindings = world->binding_count,
                        .places = world->place_count,
                        .scopes = world->scope_count,
                        .stuttering = true,
                        .stutter = s};
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
        !host(&body,
              nl_sem_bind_in_scope(world, name, phase, loop.bindings,
                                   &s->parameter),
              syntax->span)) {
        free(name);
        goto body_failure;
    }
    free(name);
    s->graph->root = expression(&body, syntax->data.loop.body);
    if (!s->graph->root || !body.terminated || body.status != NL_CHECK_OK)
        goto body_failure;
    s->conditional = nl_checked_node_view(s->graph, s->graph->root)->tail;
    const NLCheckedNodeView *test = nl_checked_node_view(
        s->graph, nl_checked_node_view(s->graph, s->conditional)->initializer);
    const NLCheckedNodeView *left =
        nl_checked_node_view(s->graph, test->first_argument);
    const NLCheckedNodeView *right =
        nl_checked_node_view(s->graph, left->next_argument);
    s->limit = right->scalar_result.value;
    const NLCheckedFragment *continuing =
        nl_checked_if_arm(s->graph, s->conditional, s->continue_arm);
    const NLControlExitView *edge = nl_control_exit_view(continuing->exits, 0);
    NLSemanticValueView next;
    if (!edge || !nl_semantic_value_view(
                     nl_control_state_context((NLControlState *)edge->state),
                     edge->values[0].value, &next)) {
        (void)host(check, NL_CHECK_INTERNAL_ERROR, syntax->span);
        goto cleanup;
    }
    if (!s->computed)
        s->next = next.scalar_value;
    LinearBreakEffect effect;
    NLCheckStatus relation = stutter_relation(s, check->artifact, &effect);
    if (relation != NL_CHECK_OK) {
        (void)host(check, relation, syntax->span);
        goto cleanup;
    }
    /* The source grammar proves B does not mention the phase. Every H state
     * reaching B therefore executes this SAME transfer on exact E. This
     * operation establishes real caller IDs; abstract arm IDs are not copied.
     * Runtime lowering emits the transfer only at the actual loop exit. */
    if (s->scalar) {
        LoopControl normal = {.source_parent = check->artifact,
                              .target = target};
        if (!host(check, nl_control_state_create(check->context, &normal.entry),
                  syntax->span))
            goto cleanup;
        const NLTypeId type =
            nl_semantic_core_type(check->context, NL_TYPE_USIZE);
        const NLValueId result =
            new_value(check, (NLSemanticValueView){.type = type}, syntax->span);
        if (result) {
            normal.normal_result = (NLCheckedResult){type, result};
            id = add(check, (NLCheckedNodeView){.kind = NL_CHECKED_LOOP,
                                                .span = syntax->span,
                                                .type = type,
                                                .result_count = 1,
                                                .results = {{type, result}},
                                                .header_inductive = true,
                                                .break_edges = 1});
        }
        const NLCheckedFragment *breaking =
            nl_checked_if_arm(s->graph, s->conditional, 1 - s->continue_arm);
        if (id &&
            (!save_stutter_graph(check, id, s, syntax->span) ||
             !host(check, retain_linear_break(check, &normal, id, breaking, s),
                   syntax->span)))
            id = 0;
        nl_control_state_destroy(normal.entry);
    } else {
        NLSyntaxView normal = *syntax;
        normal.data.loop.count = 0;
        normal.data.loop.body = s->break_source;
        id = source_linear_break(check, &normal, s, target);
    }
    if (id) {
        view(check, id)->argument_count = 1;
        view(check, id)->first_argument = s->initial_node;
        view(check, id)->continue_edges = 1;
    }
    for (NLLinearBreakProof *p = check->artifact->linear_breaks; p; p = p->next)
        if (p->cycle == s) {
            s = NULL; /* retained even on failure; outer rollback owns it */
            break;
        }
    goto cleanup;
body_failure:
    check->status = body.status;
    check->diagnostic = body.diagnostic;
    if (check->status == NL_CHECK_OK)
        loop_precision(check, syntax->span);
cleanup:
    destroy_stutter_cycle(s);
    nl_control_target_destroy(target);
    return check->status == NL_CHECK_OK ? id : 0;
}
