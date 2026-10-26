/* P932 finite diagnostic relation, not an authority/proof issuer. Included
 * BEFORE P930 Expected: inputs are the original immutable AST and live owners.
 * No allocation, reparse, stale dereference, activeBlocks/history synthesis. */
typedef struct {
    char nominal[64], labels[2][64], bindings[2][64];
    NLSourceSpan body, binding_spans[2];
    size_t order[2][2], values[2][2], operands[2];
    bool less;
} P932Source;
typedef struct {
    uintptr_t key[11]; /* copied labels only, NEVER dereference after capture */
    size_t owners[2][2], raw_owners[2][2];
} P932Capture;
static bool p932_text(const NLSource *s, NLSourceSpan span, const char *text)
{
    NLSourceView v;
    return nl_source_view(s, span, &v) && v.length == strlen(text) &&
           !memcmp(v.bytes, text, v.length);
}
static void p932_name(const NLSource *s, NLSourceSpan span, char out[64])
{
    NLSourceView v;
    R(nl_source_view(s, span, &v) && v.length && v.length < 64);
    memcpy(out, v.bytes, v.length);
    out[v.length] = 0;
}
static size_t p932_decimal(const NLSource *s, const NLSyntaxNode *n,
                           bool quantity)
{
    const NLSyntaxView *v = nl_syntax_node_view(n);
    R(v &&
      v->kind == (quantity ? NL_SYNTAX_USIZE_LITERAL : NL_SYNTAX_U8_LITERAL));
    NLSourceView digits;
    R(nl_source_view(s, quantity ? v->data.usize_digits : v->data.u8_digits,
                     &digits) &&
      digits.length);
    size_t result = 0, maximum = quantity ? SIZE_MAX : 255;
    for (size_t i = 0; i < digits.length; ++i) {
        R(digits.bytes[i] >= '0' && digits.bytes[i] <= '9');
        size_t d = digits.bytes[i] - '0';
        R(result <= (maximum - d) / 10);
        result = result * 10 + d;
    }
    return result;
}
static P932Source p932_derive(const NLSyntaxTree *tree, const NLSource *s)
{
    R(nl_syntax_tree_source(tree) == s);
    const NLSyntaxView *u = nl_syntax_node_view(nl_syntax_tree_root(tree));
    R(u && u->kind == NL_SYNTAX_FUNCTION_UNIT &&
      u->data.function_unit.count == 2);
    const NLSyntaxNode *decl = u->data.function_unit.declarations;
    const NLSyntaxView *d = nl_syntax_node_view(decl);
    R(d && d->kind == NL_SYNTAX_AVS_STRUCT && d->data.avs_struct.count == 2);
    P932Source p = {0};
    p932_name(s, d->data.avs_struct.name, p.nominal);
    const NLSyntaxNode *field = d->data.avs_struct.fields;
    for (size_t j = 0; j < 2; ++j) {
        const NLSyntaxView *v = nl_syntax_node_view(field);
        R(v && v->kind == NL_SYNTAX_PARAMETER);
        const NLSyntaxView *t = nl_syntax_node_view(v->data.parameter.type);
        R(t && t->kind == NL_SYNTAX_TYPE_NAME &&
          p932_text(s, t->data.name, "u8"));
        p932_name(s, v->data.parameter.name, p.labels[j]);
        field = nl_syntax_next_argument(field);
    }
    R(!field && strcmp(p.labels[0], p.labels[1]));
    decl = nl_syntax_next_argument(decl);
    const NLSyntaxView *fn = nl_syntax_node_view(decl);
    R(fn && fn->kind == NL_SYNTAX_FUNCTION && !fn->data.function.count &&
      !nl_syntax_next_argument(decl) &&
      p932_text(s, fn->data.function.name, "main"));
    const NLSyntaxView *b = nl_syntax_node_view(fn->data.function.body);
    R(b && b->kind == NL_SYNTAX_BLOCK && b->data.block.item_count == 3);
    p.body = b->span;
    const NLSyntaxNode *item = b->data.block.items;
    for (size_t i = 0; i < 2; ++i) {
        const NLSyntaxView *v = nl_syntax_node_view(item);
        R(v && v->kind == NL_SYNTAX_BINDING);
        p932_name(s, v->data.binding.name, p.bindings[i]);
        p.binding_spans[i] = v->span;
        const NLSyntaxView *a =
            nl_syntax_node_view(v->data.binding.initializer);
        R(a && a->kind == NL_SYNTAX_AGGREGATE && a->data.aggregate.count == 2 &&
          p932_text(s, a->data.aggregate.type_name, p.nominal));
        field = a->data.aggregate.fields;
        for (size_t j = 0; j < 2; ++j) {
            const NLSyntaxView *f = nl_syntax_node_view(field);
            R(f && f->kind == NL_SYNTAX_FIELD);
            size_t k = p932_text(s, f->data.binding.name, p.labels[0]) ? 0 : 1;
            R(p932_text(s, f->data.binding.name, p.labels[k]) &&
              (!j || k != p.order[i][0]));
            p.order[i][j] = k;
            p.values[i][k] =
                p932_decimal(s, f->data.binding.initializer, false);
            field = nl_syntax_next_argument(field);
        }
        R(!field);
        item = nl_syntax_next_argument(item);
    }
    R(strcmp(p.bindings[0], p.bindings[1]));
    const NLSyntaxView *stmt = nl_syntax_node_view(item);
    R(stmt && stmt->kind == NL_SYNTAX_STATEMENT &&
      !nl_syntax_next_argument(item));
    const NLSyntaxView *choice =
        nl_syntax_node_view(stmt->data.statement.expression);
    R(choice && choice->kind == NL_SYNTAX_IF);
    const NLSyntaxView *condition =
        nl_syntax_node_view(choice->data.conditional.condition);
    R(condition && (condition->kind == NL_SYNTAX_USIZE_EQUALITY ||
                    condition->kind == NL_SYNTAX_USIZE_LESS_THAN));
    p.less = condition->kind == NL_SYNTAX_USIZE_LESS_THAN;
    p.operands[0] = p932_decimal(s, condition->data.equality.left, true);
    p.operands[1] = p932_decimal(s, condition->data.equality.right, true);
    return p;
}
/* Comparison returns false on a mismatched DIAGNOSTIC packet. It neither
 * validates a public certificate nor changes the graph/context. */
static bool p932_correlate(const P932Source *p, NLTypeId type,
                           const NLCheckedFragment *f, NLCheckedNodeId block,
                           NLCheckedNodeId id, const NLSemanticContext *c,
                           P932Capture *out)
{
    const NLCheckedNodeView *b = nl_checked_node_view(f, block);
    if (!type || type > c->type_count || !c->types[type - 1].name ||
        strcmp(c->types[type - 1].name, p->nominal) || !b ||
        b->kind != NL_CHECKED_BLOCK || b->item_count != 2)
        return false;
    NLCheckedNodeId item = b->first_item;
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticBindingView binding;
        NLSemanticPlaceView root;
        NLSemanticValueView package;
        NLSymbolId symbol = nl_semantic_find_binding(c, p->bindings[i]);
        const NLCheckedNodeView *v = nl_checked_node_view(f, item);
        const NLCheckedNodeView *a =
            v ? nl_checked_node_view(f, v->initializer) : NULL;
        if (!nl_semantic_binding_view(c, symbol, &binding) ||
            binding.type != type || binding.availability != NL_AVAILABLE ||
            !nl_semantic_place_view(c, binding.place, &root) ||
            root.type != type || !root.live || !root.independent_root ||
            root.fixed_field_count != 2 ||
            !nl_semantic_value_view(c, root.current_value, &package) ||
            package.carrier != NL_CARRIER_PLACE ||
            package.owner_place != binding.place || package.type != type ||
            package.field_count != 2 || !v || v->kind != NL_CHECKED_BINDING ||
            v->symbol != symbol ||
            v->span.start_byte + p->body.start_byte !=
                p->binding_spans[i].start_byte ||
            v->span.end_byte + p->body.start_byte !=
                p->binding_spans[i].end_byte ||
            !a || a->kind != NL_CHECKED_AGGREGATE || a->type != type ||
            a->argument_count != 2)
            return false;
        NLCheckedNodeId arg = a->first_argument;
        for (size_t j = 0; j < 2; ++j) {
            size_t k = p->order[i][j];
            if (k >= 2)
                return false;
            NLAggregateField registered;
            NLSemanticPlaceView child;
            NLSemanticValueView member;
            const NLCheckedNodeView *q = nl_checked_node_view(f, arg);
            const NLCheckedNodeView *literal =
                q ? nl_checked_node_view(f, q->initializer) : NULL;
            if (!nl_semantic_aggregate_field_view(c, type, k, &registered) ||
                strcmp(registered.name, p->labels[k]) ||
                registered.type != nl_semantic_core_type(c, NL_TYPE_U8) ||
                !nl_semantic_place_view(c, root.fixed_fields[k], &child) ||
                !child.live || child.parent_aggregate != binding.place ||
                child.parent_field_index != k ||
                child.parent_incarnation != root.incarnation ||
                !nl_semantic_value_view(c, child.current_value, &member) ||
                member.type != registered.type ||
                member.carrier != NL_CARRIER_AGGREGATE ||
                member.aggregate_owner != root.current_value ||
                package.fields[k] != child.current_value ||
                !member.scalar_known ||
                member.scalar_value != p->values[i][k] || !q ||
                q->kind != NL_CHECKED_AGGREGATE_FIELD || q->field_index != k ||
                !p932_text(f->source, q->name, p->labels[k]) || !literal ||
                literal->kind != NL_CHECKED_U8_LITERAL ||
                !literal->has_scalar_result || !literal->scalar_result.known ||
                literal->scalar_result.type != registered.type ||
                literal->scalar_result.value != p->values[i][k])
                return false;
            /* ValueId -> actual package -> PlaceId, never a numeric cast. */
            out->raw_owners[i][k] = member.aggregate_owner;
            out->owners[i][k] = package.owner_place;
            arg = q->next_argument;
        }
        if (arg)
            return false;
        item = v->next_item;
    }
    const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
    const NLCheckedNodeView *cond =
        choice ? nl_checked_node_view(f, choice->initializer) : NULL;
    NLSemanticValueView boolean;
    bool truth = p->less ? p->operands[0] < p->operands[1]
                         : p->operands[0] == p->operands[1];
    if (item || !cond ||
        cond->kind != (p->less ? NL_CHECKED_USIZE_LESS_THAN
                               : NL_CHECKED_USIZE_EQUALITY) ||
        cond->result_count != 1 ||
        !nl_semantic_value_view(c, cond->results[0].value, &boolean) ||
        boolean.type != nl_semantic_core_type(c, NL_TYPE_BOOL) ||
        !boolean.scalar_known || boolean.scalar_value != (size_t)truth ||
        boolean.carrier != NL_CARRIER_ENDED)
        return false;
    NLCheckedNodeId arg = cond->first_argument;
    for (size_t k = 0; k < 2; ++k) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, arg);
        NLSemanticValueView value;
        if (!v || v->kind != NL_CHECKED_USIZE_LITERAL || v->result_count != 1 ||
            !nl_semantic_value_view(c, v->results[0].value, &value) ||
            value.type != nl_semantic_core_type(c, NL_TYPE_USIZE) ||
            !value.scalar_known || value.scalar_value != p->operands[k] ||
            value.carrier != NL_CARRIER_ENDED)
            return false;
        arg = v->next_argument;
    }
    return !arg;
}
static bool p932_capture_equal(const P932Capture *a, const P932Capture *b)
{
    return !memcmp(a->key, b->key, sizeof(a->key)) &&
           !memcmp(a->owners, b->owners, sizeof(a->owners));
}
static void p932_observe(const NLSyntaxTree *tree, const NLSource *unit,
                         const NLFunctionBody *plan, NLSourceSpan span,
                         const NLCheckedFragment *f, NLCheckedNodeId id,
                         const NLSemanticContext *actual, size_t session,
                         size_t phase, bool negative)
{
    /* Compute from SOURCE before obtaining entry/registered values. */
    P932Source source = p932_derive(tree, unit);
    R(source.body.start_byte == span.start_byte &&
      source.body.end_byte == span.end_byte);
    const NLSemanticContext *c = f->if_fields->entry;
    NLTypeId type = 0;
    for (size_t i = 0; i < c->type_count; ++i)
        if (c->types[i].name && !strcmp(c->types[i].name, source.nominal)) {
            R(!type);
            type = i + 1;
        }
    NLCheckedNodeId block = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v && v->kind == NL_CHECKED_BLOCK && !v->span.start_byte &&
            v->span.end_byte == nl_source_length(plan->source)) {
            R(!block);
            block = i;
        }
    }
    R(block && f->function_target);
    P932Capture capture = {
        .key = {(uintptr_t)tree, (uintptr_t)unit, (uintptr_t)plan, session,
                phase, (uintptr_t)f, block, (uintptr_t)f->function_target,
                (uintptr_t)c, (uintptr_t)actual, (uintptr_t)f->context}};
    R(p932_correlate(&source, type, f, block, id, c, &capture));
    static P932Capture definition;
    static P932Source first_source;
    if (phase == 1)
        definition = capture;
    if (session == 1 && phase == 1)
        first_source = source;
    size_t refusals = 0;
    if (negative) {
        P932Capture wrong = capture;
        R(!p932_correlate(&source, nl_semantic_core_type(c, NL_TYPE_U8), f,
                          block, id, c, &wrong));
        ++refusals;
        P932Source swapped = source;
        swapped.order[0][0] = source.order[0][1];
        swapped.order[0][1] = source.order[0][0];
        R(!p932_correlate(&swapped, type, f, block, id, c, &wrong));
        ++refusals;
        wrong = capture;
        memcpy(wrong.owners, capture.raw_owners, sizeof(wrong.owners));
        R(!p932_capture_equal(&wrong, &capture));
        ++refusals;
        if (phase == 2) {
            R(!p932_capture_equal(&definition, &capture));
            ++refusals;
        }
        if (memcmp(first_source.order, source.order, sizeof(source.order)) &&
            !memcmp(first_source.values, source.values,
                    sizeof(source.values))) {
            R(!p932_correlate(&first_source, type, f, block, id, c, &wrong));
            ++refusals;
        }
    }
    size_t bits = 0;
    for (size_t n = SIZE_MAX; n; n >>= 1)
        ++bits;
    printf("P932 source-AST/type%zu/block%zu session%zu phase%zu: "
           "typed source->checked->normalized entry; target-value-bits%zu "
           "new mismatches%zu rejected; history/activeBlocks UNPROVED\n",
           type, block, session, phase, bits, refusals);
}
