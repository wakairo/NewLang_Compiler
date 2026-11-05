/* Included only by semantic_check.c. This relation is distinct from stutter:
 * H preserves the exact structural frame, and existentially quantifies the
 * dependency-free selected u8/current root package and one free usize slot.
 * A complete source transfer on this H, not the first concrete iteration,
 * establishes closure. Ghost projection never emits a runtime Change.
 */
typedef struct {
    const NLSyntaxNode *parameter, *conditional, *field, *carry;
} CurrentSource;
typedef struct NLCurrentLoopProof {
    NLControlState *entry, *header;
    NLCheckedFragment *graph;
    const NLCheckedFragment *parent;
    const NLControlTarget *target;
    const NLSyntaxNode *carry_source;
    NLSymbolId base, phase;
    NLPlaceId root, child;
    size_t index, initial, limit, step, incoming;
    NLCheckedNodeId initial_node, conditional, quantity, terminal;
    StutterGraph graphs[3];
    StutterWorld exits[4];
    NLControlExitView exit_views[4];
    bool graph_owned, sealed;
} NLCurrentLoopProof;

static void destroy_current_cycle(NLCurrentLoopProof *s)
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
    nl_control_state_destroy(s->header);
    if (!s->graph_owned)
        nl_checked_destroy(s->graph);
    free(s);
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
        ? nl_syntax_node_view(b->data.block.tail) : NULL;
    const NLSyntaxView *guard =
        v && v->kind == NL_SYNTAX_IF
            ? nl_syntax_node_view(v->data.conditional.condition) : NULL;
    const NLSyntaxView *lhs = guard ? nl_syntax_node_view(guard->data.equality.left) : NULL;
    const NLSyntaxView *rhs = guard ? nl_syntax_node_view(guard->data.equality.right) : NULL;
    if (!p || !init || init->kind != NL_SYNTAX_USIZE_LITERAL || !b ||
        b->kind != NL_SYNTAX_BLOCK || b->data.block.items || !v ||
        !guard || guard->kind != NL_SYNTAX_USIZE_LESS_THAN || !lhs || !rhs ||
        lhs->kind != NL_SYNTAX_EXPR_NAME ||
        !stutter_name(c, lhs->data.name, p->data.binding.name) ||
        rhs->kind != NL_SYNTAX_USIZE_LITERAL ||
        !stutter_scalar_source(c, v->data.conditional.else_block,
                               p->data.binding.name))
        return false;
    const NLSyntaxView *arm = nl_syntax_node_view(v->data.conditional.then_block);
    if (!arm || arm->kind != NL_SYNTAX_BLOCK || arm->data.block.tail)
        return false;
    const NLSyntaxNode *field = NULL, *carry = NULL;
    size_t writers = 0, count = 0;
    for (const NLSyntaxNode *n = arm->data.block.items; n;
         n = nl_syntax_next_argument(n)) {
        if (++count > NL_SEMANTIC_MAX_ENTRIES)
            return false;
        const NLSyntaxView *item = nl_syntax_node_view(n), *expr = NULL;
        if (item->kind == NL_SYNTAX_CONTINUE) {
            if (nl_syntax_next_argument(n) || item->data.call.argument_count != 1)
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
        if (expr->kind == NL_SYNTAX_LOCAL_WRITE_LOAN &&
            item->kind == NL_SYNTAX_BINDING) {
            const NLSyntaxView *body = nl_syntax_node_view(expr->data.loan.body);
            const NLSyntaxView *call = body ? nl_syntax_node_view(body->data.block.tail) : NULL;
            const NLSyntaxView *target = nl_syntax_node_view(expr->data.loan.source);
            if (++writers != 1 || expr->data.loan.access != NL_ACCESS_WRITE ||
                expr->data.loan.from_ptr || expr->data.loan.is_exclusive ||
                !target || target->kind != NL_SYNTAX_FIELD_DESIGNATOR ||
                !body || body->kind != NL_SYNTAX_BLOCK || body->data.block.items ||
                !call || call->kind != NL_SYNTAX_EXPR_CALL || call->data.call.argument_count != 2)
                return false;
            field = expr->data.loan.source;
        } else if (expr->kind != NL_SYNTAX_EXPR_NAME &&
                   expr->kind != NL_SYNTAX_FIELD_DESIGNATOR &&
                   expr->kind != NL_SYNTAX_U8_LITERAL)
            return false;
    }
    const NLSyntaxView *add = nl_syntax_node_view(carry);
    lhs = add && add->kind == NL_SYNTAX_USIZE_ADDITION
        ? nl_syntax_node_view(add->data.equality.left) : NULL;
    rhs = add && add->kind == NL_SYNTAX_USIZE_ADDITION
        ? nl_syntax_node_view(add->data.equality.right) : NULL;
    if (writers != 1 || !add || add->kind != NL_SYNTAX_USIZE_ADDITION ||
        !lhs || lhs->kind != NL_SYNTAX_EXPR_NAME ||
        !stutter_name(c, lhs->data.name, p->data.binding.name) || !rhs ||
        rhs->kind != NL_SYNTAX_USIZE_LITERAL)
        return false;
    *out = (CurrentSource){s->data.loop.parameters, b->data.block.tail, field, carry};
    return true;
}

/* This slice contains no live capability/dependency which forgetting scalar
 * bits could launder. Check EVERY surviving package, not just the child.
 * Ended lexical loan packages may remain as history, never as a blocker.
 */
static bool current_packages(const NLSemanticContext *c, NLPlaceId root)
{
    if (!c || !root || root > c->place_count || c->domain_count ||
        c->region_count || c->occurrence_count || nl_fixed_validate(c) != NL_CHECK_OK)
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
        a->binding_count != b->binding_count || a->place_count != b->place_count ||
        a->scope_count != b->scope_count || !nl_copy_same_types(a, b) ||
        a->function_count != b->function_count || a->value_count > b->value_count ||
        a->last_incarnation > b->last_incarnation || a->last_value_fact > b->last_value_fact)
        return false;
    for (size_t i = 0; i < a->place_count; ++i)
        if (!same_place_frame(a->places[i], b->places[i]) ||
            a->places[i].payload_occurrence != b->places[i].payload_occurrence ||
            (i + 1 != s->root && i + 1 != s->child &&
             (a->places[i].current_fact != b->places[i].current_fact ||
              a->places[i].current_value != b->places[i].current_value)))
            return false;
    for (size_t i = 0; i < a->binding_count; ++i) {
        const NLBindingEntry x = a->bindings[i], y = b->bindings[i];
        if (x.hidden != y.hidden || strcmp(x.name, y.name) ||
            x.view.type != y.view.type || x.view.place != y.view.place ||
            x.view.availability != y.view.availability ||
            y.view.value != (x.view.place == s->root
                ? b->places[s->root - 1].current_value : x.view.value))
            return false;
    }
    for (size_t i = 0; i < a->scope_count; ++i)
        if (!same_scope(a->scopes[i], b->scopes[i]))
            return false;
    const NLValueId old = a->places[s->root - 1].current_value;
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
    NLCheckStatus status = nl_fixed_change_dependencies(c, s->child, 0);
    NLValueId value = 0, old = c->places[s->child - 1].current_value;
    if (status == NL_CHECK_OK)
        status = nl_sem_new_value(c,
            (NLSemanticValueView){.type = nl_semantic_core_type(c, NL_TYPE_U8)}, &value);
    if (status == NL_CHECK_OK)
        status = nl_fixed_change(c, s->child, value, false);
    if (status != NL_CHECK_OK)
        return status;
    nl_sem_end_value(c, old);
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
    const NLSemanticPlaceView r = b->places[s->root - 1], c = b->places[s->child - 1];
    const NLSemanticValueView v = b->values[c.current_value - 1];
    return !v.scalar_known && r.current_fact > a->last_value_fact &&
           c.current_fact > a->last_value_fact && r.current_fact != c.current_fact &&
           r.current_value > a->value_count && c.current_value > a->value_count &&
           b->values[a->places[s->root - 1].current_value - 1].carrier == NL_CARRIER_ENDED &&
           b->values[a->places[s->child - 1].current_value - 1].carrier == NL_CARRIER_ENDED;
}

static bool current_quantity(Check *c, const NLSyntaxNode *source,
                              NLCheckedNodeId left, NLCheckedNodeId right)
{
    NLCurrentLoopProof *s = c->loop ? c->loop->current : NULL;
    if (!s || s->carry_source != source || s->quantity ||
        c->loop->target != s->target || c->artifact->if_parent != s->graph ||
        c->artifact->if_arm_index != 0 || c->artifact->source != s->graph->source)
        return false;
    const NLCheckedNodeView *choice = nl_checked_node_view(s->graph, c->artifact->if_parent_node);
    const NLCheckedNodeView *guard = choice ? nl_checked_node_view(s->graph, choice->initializer) : NULL;
    const NLCheckedNodeView *a = guard ? nl_checked_node_view(s->graph, guard->first_argument) : NULL;
    const NLCheckedNodeView *b = a ? nl_checked_node_view(s->graph, a->next_argument) : NULL;
    const NLCheckedNodeView *x = view(c, left), *y = view(c, right);
    const NLTypeId type = nl_semantic_core_type(c->context, NL_TYPE_USIZE);
    if (!guard || guard->kind != NL_CHECKED_USIZE_LESS_THAN || !a || !b ||
        a->kind != NL_CHECKED_IDENTIFIER || a->symbol != s->phase ||
        x->kind != NL_CHECKED_IDENTIFIER || x->symbol != s->phase ||
        !b->has_scalar_result || !y->has_scalar_result ||
        !stutter_literal(b, type, b->scalar_result.value) ||
        !stutter_literal(y, type, y->scalar_result.value) ||
        !s->phase || s->phase > c->context->binding_count ||
        s->phase > s->graph->context->binding_count ||
        !stutter_free_quantity(c->context, x->results[0].value, type))
        return false;
    const NLSemanticBindingView before = s->graph->context->bindings[s->phase - 1].view,
                                now = c->context->bindings[s->phase - 1].view;
    if (before.value != now.value || before.place != now.place ||
        before.availability != NL_AVAILABLE || now.availability != NL_AVAILABLE ||
        !before.place || before.place > c->context->place_count ||
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
    NLCheckedField change;
    bool changed;
} CurrentWalk;
static bool current_intrinsic(NLSemanticValueView a, NLSemanticValueView b)
{
    return a.type == b.type && a.scalar_known == b.scalar_known &&
           (!a.scalar_known || a.scalar_value == b.scalar_value);
}
static const NLCheckedNodeView *current_take(CurrentWalk *r, NLCheckedNodeId id)
{
    if (!id || id > r->arm->count || id > NL_SEMANTIC_MAX_ENTRIES || r->seen[id - 1])
        return NULL;
    r->seen[id - 1] = true;
    return nl_checked_node_view(r->arm, id);
}
static bool current_scalar(CurrentWalk *r, const NLCheckedNodeView *v)
{
    if (!v || v->result_count != 1 || v->type != nl_semantic_core_type(r->arm->context, NL_TYPE_U8) ||
        !nl_copy_plain(r->arm->context, v->results[0].value, v->type))
        return false;
    if (v->kind == NL_CHECKED_U8_LITERAL)
        return v->has_scalar_result && v->scalar_result.known && v->scalar_result.value <= 255;
    if (v->kind == NL_CHECKED_IDENTIFIER)
        return v->symbol && v->symbol <= NL_SEMANTIC_MAX_ENTRIES &&
               r->bound[v->symbol - 1] && v->value_use == NL_VALUE_COPIED &&
               current_intrinsic(r->arm->context->values[v->results[0].value - 1],
                         r->arm->context->values[r->bound[v->symbol - 1] - 1]);
    if (v->kind != NL_CHECKED_FIELD_READ)
        return false;
    const NLCurrentLoopProof *s = r->proof;
    const NLCheckedField f = v->field;
    const NLSemanticPlaceView root = r->header->places[s->root - 1];
    if (!f.present || f.base != s->base || f.parent != s->root || f.index >= 2 ||
        f.child != root.fixed_fields[f.index] || f.parent_incarnation != root.incarnation)
        return false;
    const NLSemanticPlaceView child = r->header->places[f.child - 1];
    const bool changed = r->changed && f.child == s->child;
    return f.child_incarnation == child.incarnation &&
           f.parent_fact == (r->changed ? r->change.parent_post_fact : root.current_fact) &&
           f.child_fact == (changed ? r->change.child_post_fact : child.current_fact) &&
           f.old_value == (changed ? r->change.new_value : child.current_value) &&
           current_intrinsic(r->arm->context->values[v->results[0].value - 1],
                     r->arm->context->values[f.old_value - 1]);
}

static bool current_loan(CurrentWalk *r, const NLCheckedNodeView *loan)
{
    const NLCurrentLoopProof *s = r->proof;
    const NLCheckedNodeView *body = current_take(r, loan->initializer);
    const NLCheckedNodeView *op = body ? current_take(r, body->tail) : NULL;
    const NLCheckedNodeView *ref = op ? current_take(r, op->first_argument) : NULL;
    const NLCheckedNodeView *value = ref ? current_take(r, ref->next_argument) : NULL;
    const NLSemanticContext *c = r->arm->context;
    if (r->changed || !body || body->kind != NL_CHECKED_BLOCK || body->item_count ||
        !op || op->kind != NL_CHECKED_REPLACE || op->argument_count != 2 ||
        op->result_count != 1 || !ref || !value || value->next_argument ||
        ref->kind != NL_CHECKED_IDENTIFIER || ref->symbol != loan->loan.ref_symbol ||
        !current_scalar(r, value) || value->kind != NL_CHECKED_U8_LITERAL ||
        !loan->loan.implicit_local || loan->loan.from_ptr || loan->loan.is_exclusive ||
        loan->loan.access != NL_ACCESS_WRITE || loan->loan.source != s->base ||
        !loan->loan.body_nonescape_proved || !loan->loan.normal_result_forwarded ||
        !loan->loan.scope || loan->loan.scope > c->scope_count ||
        c->scopes[loan->loan.scope - 1].active ||
        !op->field.present || !op->field.dependency_compatible ||
        op->field.child != s->child || op->field.parent != s->root ||
        op->field.index != s->index || loan->loan.place != s->child ||
        op->field.parent_fact != r->header->places[s->root - 1].current_fact ||
        op->field.child_fact != r->header->places[s->child - 1].current_fact ||
        op->field.old_value != r->header->places[s->child - 1].current_value ||
        op->results[0].value != op->field.old_value ||
        value->results[0].value != op->field.new_value ||
        loan->results[0].value != op->results[0].value)
        return false;
    r->changed = true;
    r->change = op->field;
    return true;
}

static bool current_arm(const NLCurrentLoopProof *s,
                         const NLCheckedFragment *arm, bool continuing,
                         NLCheckedField *field)
{
    CurrentWalk r = {.proof = s, .arm = arm,
                    .header = nl_control_state_context(s->header)};
    if (!arm || arm->count > NL_SEMANTIC_MAX_ENTRIES ||
        !nl_control_source_terminal_valid(arm, s->target,
            continuing ? NL_EXIT_CONTINUE : NL_EXIT_BREAK, s->parent))
        return false;
    const NLCheckedNodeView *wrapper = current_take(&r, arm->root);
    const NLCheckedNodeView *block = wrapper ? current_take(&r, wrapper->initializer) : NULL;
    if (!block || block->kind != NL_CHECKED_BLOCK || block->tail)
        return false;
    size_t count = 0;
    for (NLCheckedNodeId id = block->first_item; id;) {
        const NLCheckedNodeView *v = current_take(&r, id);
        if (!v || ++count > block->item_count)
            return false;
        if (v->kind == NL_CHECKED_CONTINUE || v->kind == NL_CHECKED_BREAK) {
            const NLCheckedNodeView *x = current_take(&r, v->initializer);
            if (v->next_item || !x || v->kind != (continuing ? NL_CHECKED_CONTINUE : NL_CHECKED_BREAK))
                return false;
            if (continuing) {
                const NLCheckedNodeView *a = current_take(&r, x->first_argument);
                const NLCheckedNodeView *b = a ? current_take(&r, a->next_argument) : NULL;
                if (v->initializer != s->quantity || x->kind != NL_CHECKED_USIZE_ADDITION ||
                    !a || a->kind != NL_CHECKED_IDENTIFIER || a->symbol != s->phase ||
                    !b || b->next_argument || !stutter_literal(b, x->type, s->step) ||
                    !x->has_scalar_result || x->scalar_result.known ||
                    !stutter_free_quantity(arm->context, x->results[0].value, x->type) ||
                    !r.changed)
                    return false;
            } else if (block->item_count != 1 || x->kind != NL_CHECKED_IDENTIFIER ||
                       x->symbol != s->phase || x->value_use != NL_VALUE_COPIED ||
                       !stutter_free_quantity(arm->context, x->results[0].value, x->type))
                return false;
        } else if (continuing && (v->kind == NL_CHECKED_BINDING || v->kind == NL_CHECKED_STATEMENT)) {
            const NLCheckedNodeView *x = current_take(&r, v->initializer);
            if (!x)
                return false;
            if (v->kind == NL_CHECKED_BINDING) {
                const NLCheckedNodeView *receiver = current_take(&r, v->first_argument);
                if (!receiver || receiver->kind != NL_CHECKED_RECEIVER || !v->symbol ||
                    v->symbol > NL_SEMANTIC_MAX_ENTRIES || r.bound[v->symbol - 1] ||
                    v->symbol == s->phase || receiver->symbol != v->symbol ||
                    receiver->type != nl_semantic_core_type(arm->context, NL_TYPE_U8) ||
                    !(x->kind == NL_CHECKED_LOAN_HEADER ? current_loan(&r, x) : current_scalar(&r, x)))
                    return false;
                r.bound[v->symbol - 1] = x->results[0].value;
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
        *field = r.change;
    return count == block->item_count && r.changed == continuing;
}

static NLCheckStatus current_relation(NLCurrentLoopProof *s)
{
    const NLCheckStatus bad = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLSemanticContext *e = nl_control_state_context(s->entry),
                            *h = nl_control_state_context(s->header);
    const NLCheckedFragment *g = s->graph;
    const NLCheckedNodeView *root = nl_checked_node_view(g, g->root);
    const NLCheckedNodeView *choice = root ? nl_checked_node_view(g, root->tail) : NULL;
    const NLCheckedNodeView *guard = choice ? nl_checked_node_view(g, choice->initializer) : NULL;
    const NLCheckedNodeView *lhs = guard ? nl_checked_node_view(g, guard->first_argument) : NULL;
    const NLCheckedNodeView *rhs = lhs ? nl_checked_node_view(g, lhs->next_argument) : NULL;
    const NLTypeId type = nl_semantic_core_type(h, NL_TYPE_USIZE);
    if (!current_projection(s, e, h) || !s->step || !finite_add_bound(s->limit, s->step) ||
        !root || root->kind != NL_CHECKED_BLOCK || root->item_count ||
        root->tail != s->conditional || !choice || choice->kind != NL_CHECKED_IF ||
        !guard || guard->kind != NL_CHECKED_USIZE_LESS_THAN || !lhs || !rhs ||
        lhs->kind != NL_CHECKED_IDENTIFIER || lhs->symbol != s->phase ||
        !stutter_literal(rhs, type, s->limit) || rhs->next_argument ||
        !stutter_free_quantity(g->context, lhs->results[0].value, type) ||
        g->count != 5 || !current_packages(g->context, s->root) ||
        nl_control_exits_count(g->exits) != 2)
        return bad;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_if_arm(g, s->conditional, i);
        NLCheckedField change = {0};
        if (!current_arm(s, arm, i == 0, &change) || !arm ||
            arm->if_parent != g || arm->if_parent_node != s->conditional ||
            arm->if_arm_index != i || arm->source != g->source)
            return bad;
        const NLControlExitView *leaf = nl_control_exit_view(arm->exits, 0);
        const NLControlExitView *joined = nl_control_exit_view(g->exits, i);
        const NLSemanticContext *a = leaf ? nl_control_state_context((NLControlState *)leaf->state) : NULL;
        const NLSemanticContext *b = joined ? nl_control_state_context((NLControlState *)joined->state) : NULL;
        if (!leaf || !joined || leaf->target != s->target || joined->target != s->target ||
            joined->kind != leaf->kind || joined->count != 1 ||
            joined->source_owner != arm || joined->source_node != leaf->source_node ||
            joined->values[0].type != leaf->values[0].type ||
            joined->values[0].value != leaf->values[0].value ||
            !current_frame(s, h, a) || !current_frame(s, h, b) ||
            !stutter_free_quantity(a, leaf->values[0].value, type) ||
            !stutter_free_quantity(b, joined->values[0].value, type))
            return bad;
        if (i == 0) {
            if (!linear_break_frame(h, arm->context, &change) ||
                !linear_break_frame(h, a, &change) || !linear_break_frame(h, b, &change))
                return bad;
            const NLSemanticValueView incoming = a->values[change.new_value - 1];
            if (!incoming.scalar_known || incoming.scalar_value > 255)
                return bad;
            if (s->sealed && (s->incoming != incoming.scalar_value || s->terminal != leaf->source_node))
                return bad;
            if (!s->sealed) {
                s->incoming = incoming.scalar_value;
                s->terminal = leaf->source_node;
            }
        } else if (!stutter_frame(h, a, 1) || !stutter_frame(h, b, 1))
            return bad;
    }
    return NL_CHECK_OK;
}

static NLCheckStatus seal_current_cycle(NLLinearBreakProof *p)
{
    NLCurrentLoopProof *s = p->current_cycle;
    for (size_t i = 0; i < 3; ++i) {
        StutterGraph *a = &s->graphs[i];
        a->fragment = i ? nl_checked_if_arm(s->graph, s->conditional, i - 1) : s->graph;
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
    for (size_t i = 0; i < 4; ++i) {
        const NLControlExitView *x = i < 2
            ? nl_control_exit_view(s->graphs[i + 1].fragment->exits, 0)
            : nl_control_exit_view(s->graph->exits, i - 2);
        if (!x)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        memcpy(&s->exit_views[i], x, sizeof(*x));
        NLCheckStatus status = stutter_anchor_world(&s->exits[i],
            nl_control_state_context((NLControlState *)x->state));
        if (status != NL_CHECK_OK)
            return status;
    }
    s->sealed = true;
    return NL_CHECK_OK;
}

static NLCheckStatus validate_current_cycle(const NLLinearBreakProof *p,
                                            const NLCheckedFragment *f,
                                            NLLinearBreakView *out)
{
    const NLCheckStatus bad = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLCurrentLoopProof *s = p->current_cycle;
    if (!p->sealed || !s->sealed || !s->graph_owned || p->owner != f ||
        f->context != p->world || f->source != p->source || f->body_owner != p->plan ||
        !nl_h_custody_validate(p->root) ||
        !linear_graph_matches(p->root, p->root_origin, p->root_nodes, p->root_count, p->root_root) ||
        !linear_graph_matches(f, p->parent_origin, p->parent_nodes, p->parent_count, p->parent_root) ||
        !linear_world_matches(f->context, &p->final_storage, p->final, &p->final_anchor) ||
        !linear_callable(p, f->context) || !linear_callable(p, p->post) ||
        !nl_copy_state_matches(p->post, &p->post_anchor) ||
        nl_checked_loop_body(f, p->loop) != s->graph)
        return bad;
    for (size_t i = 0; i < 3; ++i) {
        const StutterGraph *a = &s->graphs[i];
        if ((i && nl_checked_if_arm(s->graph, s->conditional, i - 1) != a->fragment) ||
            !linear_graph_matches(a->fragment, a->base, a->nodes, a->count, a->root) ||
            a->fragment->source != a->source || a->fragment->context != a->world.world ||
            !linear_world_matches(a->world.world, &a->world.storage, a->world.copy, &a->world.anchor))
            return bad;
    }
    for (size_t i = 0; i < 4; ++i) {
        const NLControlExitView *x = i < 2
            ? nl_control_exit_view(s->graphs[i + 1].fragment->exits, 0)
            : nl_control_exit_view(s->graph->exits, i - 2);
        if (!x || memcmp(x, &s->exit_views[i], sizeof(*x)) ||
            nl_control_state_context((NLControlState *)x->state) != s->exits[i].world ||
            !linear_world_matches(s->exits[i].world, &s->exits[i].storage,
                                   s->exits[i].copy, &s->exits[i].anchor))
            return bad;
    }
    const NLCheckedNodeView *n = nl_checked_node_view(f, p->loop),
                            *call = nl_checked_node_view(p->root, p->call);
    const NLSemanticContext *e = nl_control_state_context(s->entry),
                            *h = nl_control_state_context(s->header);
    if (!n || !call || n->kind != NL_CHECKED_LOOP || !n->header_inductive ||
        n->continue_edges != 1 || n->break_edges != 1 || n->return_edges ||
        n->first_argument != s->initial_node || n->argument_count != 1 ||
        n->result_count != 1 || n->results[0].value != p->result.value ||
        n->results[0].type != p->result.type || p->target != s->target ||
        call->function != p->function || call->argument_count ||
        nl_checked_call_body(p->root, p->call) != f ||
        !stutter_literal(nl_checked_node_view(f, s->initial_node), p->result.type, s->initial) ||
        !current_projection(s, e, p->post) ||
        !stutter_free_quantity(p->post, p->result.value, p->result.type) ||
        p->result.value != p->post->value_count ||
        !stutter_frame(e, nl_control_state_context(p->entry), 0) ||
        current_relation(s) != NL_CHECK_OK)
        return bad;
    const NLSemanticPlaceView r = h->places[s->root - 1], c = h->places[s->child - 1];
    *out = (NLLinearBreakView){.base = s->base, .nominal = r.type,
        .root = s->root, .child = s->child, .incarnation = r.incarnation,
        .child_incarnation = c.incarnation, .index = s->index, .incoming = s->incoming,
        .result = p->result, .changing_current = true, .scalar_loop = true,
        .iteration = s->graphs[1].fragment, .continue_node = s->terminal,
        .phase_symbol = s->phase, .initial_phase = s->initial,
        .limit = s->limit, .next_phase = s->step};
    return NL_CHECK_OK;
}

static NLCheckedNodeId source_current_loop(Check *check, const NLSyntaxView *syntax,
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
    if (!fixed_selection(check, nl_syntax_node_view(source->field), NL_ACCESS_WRITE, &field))
        goto cleanup;
    s->base = field.base; s->root = field.parent; s->child = field.child; s->index = field.index;
    s->parent = check->artifact; s->carry_source = source->carry;
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
    s->initial = view(check, s->initial_node)->scalar_result.value;
    if (!host(check, nl_control_target_create(NL_TARGET_LOOP, &target), syntax->span) ||
        !host(check, nl_control_state_create(check->context, &s->entry), syntax->span) ||
        !host(check, nl_sem_clone(check->context, &working), syntax->span) ||
        !host(check, current_abstract(s, working), syntax->span) ||
        !host(check, nl_control_state_capture(s->entry, working, &s->header), syntax->span))
        goto cleanup;
    s->target = target;
    s->graph = calloc(1, sizeof(*s->graph));
    if (!s->graph) {
        (void)host(check, NL_CHECK_OUT_OF_MEMORY, syntax->span);
        goto cleanup;
    }
    *s->graph = (NLCheckedFragment){.source = check->source, .context = working,
                                  .destroy_context = nl_semantic_destroy};
    working = NULL;
    if (!host(check, nl_h_custody_attach(check->artifact, s->graph), syntax->span))
        goto cleanup;
    NLSemanticContext *world = (NLSemanticContext *)s->graph->context;
    LoopControl loop = {.source_parent = check->artifact, .target = target,
        .entry = s->header, .current = s, .count = 1,
        .bindings = world->binding_count, .places = world->place_count, .scopes = world->scope_count};
    loop.types[0] = nl_semantic_core_type(world, NL_TYPE_USIZE);
    Check body = *check;
    body.context = world; body.artifact = s->graph; body.loop = &loop;
    body.binding_floor = body.arm_floor = loop.bindings; body.has_arm_floor = true;
    char *name = source_name_copy(&body, param->data.binding.name);
    const NLValueId phase = new_value(&body, (NLSemanticValueView){.type = loop.types[0]}, syntax->span);
    if (!name || !phase || !host(&body,
        nl_sem_bind_in_scope(world, name, phase, loop.bindings, &s->phase), syntax->span)) {
        free(name); goto body_failure;
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
    if (!host(check, nl_control_state_fork(s->entry, &normal.entry), syntax->span))
        goto cleanup;
    if (host(check, current_abstract(s, check->context), syntax->span)) {
        const NLValueId result = new_value(check, (NLSemanticValueView){.type = loop.types[0]}, syntax->span);
        normal.normal_result = (NLCheckedResult){loop.types[0], result};
        if (result)
            id = add(check, (NLCheckedNodeView){.kind = NL_CHECKED_LOOP, .span = syntax->span,
                .type = loop.types[0], .result_count = 1, .results = {{loop.types[0], result}},
                .header_inductive = true, .continue_edges = 1, .break_edges = 1,
                .argument_count = 1, .first_argument = s->initial_node});
    }
    if (id && save_arm(check, id, s->graph, syntax->span)) {
        s->graph_owned = true;
        const NLCheckedFragment *breaking = nl_checked_if_arm(s->graph, s->conditional, 1);
        if (!host(check, retain_linear_break(check, &normal, id, breaking, NULL), syntax->span))
            id = 0;
        /* Even a later failure belongs to the enclosing transaction. */
        for (NLLinearBreakProof *p = check->artifact->linear_breaks; p; p = p->next)
            if (p->loop == id || p->body == breaking) {
                p->current_cycle = s;
                s = NULL;
                break;
            }
    } else
        id = 0;
    nl_control_state_destroy(normal.entry);
    goto cleanup;
body_failure:
    check->status = body.status; check->diagnostic = body.diagnostic;
    if (check->status == NL_CHECK_OK)
        loop_precision(check, syntax->span);
cleanup:
    nl_semantic_destroy(working);
    destroy_current_cycle(s);
    nl_control_target_destroy(target);
    return check->status == NL_CHECK_OK ? id : 0;
}
