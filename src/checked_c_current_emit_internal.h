/* Included after the H serializer's existing Local/Env and writer helpers. */
static const char *physical_current_type(const Emit *e, NLTypeId type)
{
    return type == e->current->quantity ? "size_t"
           : type == e->u8              ? "uint8_t"
                                        : NULL;
}

static bool physical_current_scalar(Emit *, Env *, NLCheckedNodeId, Value *,
                                    size_t);

static bool physical_current_operand(Emit *e, const NLPhysicalHelper *pure,
                                     NLCheckedNodeId id, size_t actual)
{
    const NLTypeId quantity =
        nl_semantic_core_type(nl_checked_context(pure->body), NL_TYPE_USIZE);
    for (size_t count = 0; count < nl_checked_node_count(pure->body); ++count) {
        const NLCheckedNodeView *n = nl_checked_node_view(pure->body, id);
        if (!n || n->type != quantity || n->result_count != 1)
            return reject(e);
        if (n->kind == NL_CHECKED_USIZE_LITERAL)
            return n->has_scalar_result && n->scalar_result.known &&
                   put(e, "(size_t)UINTMAX_C(%zu)", n->scalar_result.value);
        if (n->kind != NL_CHECKED_IDENTIFIER || n->value_use != NL_VALUE_COPIED)
            return reject(e);
        if (n->symbol == pure->formal)
            return put(e, "nl_v_%zu", actual);
        NLCheckedNodeId initializer = 0;
        for (NLCheckedNodeId b = 1; b < id; ++b) {
            const NLCheckedNodeView *binding =
                nl_checked_node_view(pure->body, b);
            if (binding->kind == NL_CHECKED_BINDING &&
                binding->symbol == n->symbol) {
                if (initializer)
                    return reject(e);
                initializer = binding->initializer;
            }
        }
        if (!initializer)
            return reject(e);
        id = initializer;
    }
    return reject(e);
}

static bool physical_current_helper(Emit *e, Env *env, Value *out, size_t depth)
{
    const NLPhysicalCurrent *p = e->current;
    const NLCheckedNodeView *call = get(env, p->loop_view.incoming_call);
    const NLCheckedNodeView *choice =
        nl_checked_node_view(p->helper.body, p->helper.selection);
    const NLCheckedNodeView *test =
        choice ? nl_checked_node_view(p->helper.body, choice->initializer)
               : NULL;
    const NLCheckedNodeView *left =
        test ? nl_checked_node_view(p->helper.body, test->first_argument)
             : NULL;
    const NLCheckedNodeView *right =
        left ? nl_checked_node_view(p->helper.body, left->next_argument) : NULL;
    Value actual = {0};
    if (!call || !choice || !test || !left || !right || right->next_argument ||
        (test->kind != NL_CHECKED_USIZE_LESS_THAN &&
         test->kind != NL_CHECKED_USIZE_EQUALITY) ||
        !physical_current_scalar(e, env, call->first_argument, &actual,
                                 depth + 1))
        return reject(e);
    out->type = e->u8;
    out->temp = ++e->temp;
    if (!put(e, "uint8_t nl_v_%zu=(", out->temp) ||
        !physical_current_operand(e, &p->helper, test->first_argument,
                                  actual.temp) ||
        !put(e, test->kind == NL_CHECKED_USIZE_LESS_THAN ? " < " : " == ") ||
        !physical_current_operand(e, &p->helper, left->next_argument,
                                  actual.temp) ||
        !put(e, " ? "))
        return false;
    for (size_t arm = 0; arm < 2; ++arm) {
        const NLCheckedFragment *a =
            nl_checked_if_arm(p->helper.body, p->helper.selection, arm);
        /* The RETURN operand is sealed by full current validation. prepare
         * separately proved the actual normal-final Copy path ends at this
         * selection, so its normal arm literal is the emitted normal result.
         * No returning source arm is reclassified as normal. */
        const NLCheckedNodeView *root =
            nl_checked_node_view(a, nl_checked_root(a));
        const NLCheckedNodeView *block =
            root ? nl_checked_node_view(a, root->initializer) : NULL;
        const NLCheckedNodeView *ret =
            block ? nl_checked_node_view(a, block->first_item) : NULL;
        const NLCheckedNodeView *literal =
            block ? nl_checked_node_view(a, root->terminates && ret
                                                ? ret->initializer
                                                : block->tail)
                  : NULL;
        if (!literal || !physical_current_literal(a, literal) ||
            !put(e, "%zu%s", literal->scalar_result.value,
                 arm ? ");\n" : " : "))
            return reject(e);
    }
    return true;
}

static bool physical_current_field(Emit *e, Env *env,
                                   const NLCheckedNodeView *n, Value *out)
{
    const NLLinearBreakView *p = &e->current->loop_view;
    const Local *original = local(env, p->base, p->nominal);
    const NLCheckedField *f = &n->field;
    NLSemanticPlaceView root, child;
    if (!original || original->place != p->root ||
        original->incarnation != p->incarnation || !f->present ||
        !f->dependency_compatible || f->nominal != p->nominal ||
        f->parent != p->root || f->parent_incarnation != p->incarnation ||
        f->base != p->base || f->index > 1 || f->access != NL_ACCESS_READ ||
        !nl_semantic_place_view(p->post_world, p->root, &root) ||
        f->child != root.fixed_fields[f->index] ||
        !nl_semantic_place_view(p->post_world, f->child, &child) ||
        f->child_incarnation != child.incarnation)
        return reject(e);
    if (env->artifact == e->current->body &&
        (!e->current_emitted || f->parent_fact != root.current_fact ||
         f->child_fact != child.current_fact ||
         f->old_value != child.current_value))
        return reject(e);
    out->type = e->u8;
    out->temp = ++e->temp;
    if (!put(e, "uint8_t nl_v_%zu=nl_b_%zu_%zu.f%zu;\n", out->temp,
             original->frame, original->symbol, f->index))
        return false;
    return env->artifact != e->current->body ||
           put(e, "NL_LOOP_READ(&nl_b_%zu_%zu.f%zu,nl_v_%zu);\n",
               original->frame, original->symbol, f->index, out->temp);
}

static bool physical_current_loan(Emit *e, Env *env, const NLCheckedNodeView *n,
                                  Value *out, size_t depth)
{
    const NLLinearBreakView *p = &e->current->loop_view;
    const Local *original = local(env, p->base, p->nominal);
    const NLCheckedNodeView *body = get(env, n->initializer);
    const NLCheckedNodeView *change = body ? get(env, body->tail) : NULL;
    const NLCheckedNodeView *ref =
        change ? get(env, change->first_argument) : NULL;
    Value incoming = {0};
    if (!original || original->place != p->root ||
        original->incarnation != p->incarnation ||
        env->artifact != p->iteration || !body || body->item_count || !change ||
        change->kind != NL_CHECKED_REPLACE || !ref ||
        ref->symbol != n->loan.ref_symbol || n->loan.place != p->child ||
        n->loan.source != p->base || !n->loan.body_nonescape_proved ||
        !n->loan.normal_result_forwarded || !n->loan.implicit_local ||
        n->loan.from_ptr || n->loan.is_exclusive ||
        n->loan.access != NL_ACCESS_WRITE || change->field.parent != p->root ||
        change->field.child != p->child ||
        !physical_current_scalar(e, env, ref->next_argument, &incoming,
                                 depth + 1))
        return reject(e);
    out->type = e->u8;
    out->temp = ++e->temp;
    return put(e,
               "uint8_t nl_v_%zu;\n{\nuint8_t *const nl_current_write="
               "&nl_b_%zu_%zu.f%zu;\nnl_v_%zu=*nl_current_write;\n"
               "*nl_current_write=nl_v_%zu;\n"
               "NL_MIXED_COPY_STORE(nl_current_write,nl_v_%zu,nl_v_%zu);\n}\n",
               out->temp, original->frame, original->symbol, p->index,
               out->temp, incoming.temp, out->temp, incoming.temp);
}

static bool physical_current_scalar(Emit *e, Env *env, NLCheckedNodeId id,
                                    Value *out, size_t depth)
{
    const NLCheckedNodeView *n = get(env, id);
    NLSemanticValueView value;
    if (!n || depth >= C_DEPTH || n->terminates || n->result_count != 1 ||
        !physical_current_type(e, n->type) ||
        !nl_semantic_value_view(nl_checked_context(env->artifact),
                                n->results[0].value, &value) ||
        !nl_copy_plain(nl_checked_context(env->artifact), n->results[0].value,
                       n->type))
        return reject(e);
    out->type = n->type;
    if (n->kind == NL_CHECKED_FIELD_READ)
        return physical_current_field(e, env, n, out);
    if (n->kind == NL_CHECKED_LOAN_HEADER)
        return physical_current_loan(e, env, n, out, depth);
    if (n->kind == NL_CHECKED_REGISTERED_CALL)
        return env->artifact == e->current->loop_view.iteration &&
                       id == e->current->loop_view.incoming_call
                   ? physical_current_helper(e, env, out, depth)
                   : reject(e);
    out->temp = ++e->temp;
    if (n->kind == NL_CHECKED_U8_LITERAL || n->kind == NL_CHECKED_USIZE_LITERAL)
        return n->has_scalar_result && n->scalar_result.known &&
               n->scalar_result.value == value.scalar_value &&
               value.scalar_known &&
               put(e, "%s nl_v_%zu=(%s)UINTMAX_C(%zu);\n",
                   physical_current_type(e, n->type), out->temp,
                   physical_current_type(e, n->type), value.scalar_value);
    if (n->kind != NL_CHECKED_IDENTIFIER || n->value_use != NL_VALUE_COPIED)
        return reject(e);
    if (env->artifact == e->current->loop_view.iteration &&
        n->symbol == e->current->loop_view.phase_symbol)
        return put(e, "size_t nl_v_%zu=nl_current_phase;\n", out->temp);
    const Local *l = local(env, n->symbol, n->type);
    return l ? put(e, "%s nl_v_%zu=nl_b_%zu_%zu;\n",
                   physical_current_type(e, n->type), out->temp, l->frame,
                   l->symbol)
             : reject(e);
}

static bool physical_current_item(Emit *e, Env *env, NLCheckedNodeId id)
{
    const NLCheckedNodeView *n = get(env, id);
    Value value = {0};
    if (!n ||
        (n->kind != NL_CHECKED_BINDING && n->kind != NL_CHECKED_STATEMENT) ||
        !physical_current_scalar(e, env, n->initializer, &value, 0))
        return reject(e);
    if (n->kind == NL_CHECKED_STATEMENT)
        return put(e, "(void)nl_v_%zu;\n", value.temp);
    const NLCheckedNodeView *receiver = get(env, n->first_argument);
    if (!receiver || receiver->kind != NL_CHECKED_RECEIVER ||
        n->argument_count != 1 || receiver->next_argument ||
        receiver->symbol != n->symbol || receiver->type != value.type ||
        !add_local(e, env, n->symbol, value.type, false))
        return reject(e);
    return put(e,
               "%s nl_b_%zu_%zu=nl_v_%zu;\n(void)nl_b_%zu_%zu;\n"
               "NL_CURRENT_LOCAL(%zu,nl_b_%zu_%zu);\n",
               physical_current_type(e, value.type), env->frame, n->symbol,
               value.temp, env->frame, n->symbol, n->symbol, env->frame,
               n->symbol);
}

static bool physical_current_loop(Emit *e, Env *env, Value *out)
{
    const NLPhysicalCurrent *p = e->current;
    const NLLinearBreakView *v = &p->loop_view;
    const Local *original = local(env, v->base, v->nominal);
    const NLCheckedNodeView *wrapper =
        nl_checked_node_view(v->iteration, nl_checked_root(v->iteration));
    const NLCheckedNodeView *body =
        wrapper ? nl_checked_node_view(v->iteration, wrapper->initializer)
                : NULL;
    const NLCheckedNodeView *result =
        nl_checked_node_view(v->breaking, v->break_value);
    Value initial = {0};
    if (!e->current_h_closed || e->current_emitted || !original ||
        original->place != v->root || original->incarnation != v->incarnation ||
        !body || !result ||
        !physical_current_scalar(e, env, v->initial_node, &initial, 0))
        return reject(e);
    out->type = p->quantity;
    out->temp = ++e->temp;
    if (!put(e,
             "size_t nl_v_%zu;\n{\nsize_t nl_current_carry=nl_v_%zu;\n"
             "for (;;) {\nconst size_t nl_current_phase=nl_current_carry;\n"
             "NL_CURRENT_PHASE(nl_current_phase,&nl_b_%zu_%zu.f0,"
             "&nl_b_%zu_%zu.f1);\n"
             "if (nl_current_phase < (size_t)UINTMAX_C(%zu)) {\n",
             out->temp, initial.temp, original->frame, original->symbol,
             original->frame, original->symbol, v->limit))
        return false;
    /* The validated current relation, not raw ID coincidence, licenses this
     * exact persistent prefix in its hypothetical iteration world. */
    Env iteration = {.artifact = v->iteration,
                     .parent = env,
                     .prefix = v->entry_world->binding_count,
                     .frame = ++e->frames,
                     .depth = env->depth + 1};
    NLCheckedNodeId item = body->first_item;
    size_t count = 0;
    while (item && item != v->continue_node) {
        const NLCheckedNodeView *n = get(&iteration, item);
        if (!n || ++count > body->item_count ||
            !physical_current_item(e, &iteration, item))
            return reject(e);
        item = n->next_item;
    }
    if (item != v->continue_node || count + 1 != body->item_count ||
        !put(e,
             "nl_current_carry=nl_current_phase+(size_t)UINTMAX_C(%zu);\n"
             "continue;\n}\nnl_v_%zu=",
             v->next_phase, out->temp))
        return reject(e);
    if (result->kind == NL_CHECKED_USIZE_LITERAL) {
        if (!result->has_scalar_result || !result->scalar_result.known ||
            !put(e, "(size_t)UINTMAX_C(%zu)", result->scalar_result.value))
            return reject(e);
    } else if (result->kind != NL_CHECKED_IDENTIFIER ||
               result->symbol != v->phase_symbol || !put(e, "nl_current_phase"))
        return reject(e);
    e->current_emitted = true;
    return put(e, ";\nNL_CURRENT_RESULT(nl_v_%zu);\nbreak;\n}\n}\n", out->temp);
}

static bool physical_current_main(Emit *e, Env *env, NLCheckedNodeId id,
                                  Value *out)
{
    const NLPhysicalCurrent *p = e->current;
    const NLLinearBreakView *v = &p->loop_view;
    const NLCheckedNodeView *n = get(env, id);
    if (!n || env->artifact != p->body || env->parent)
        return reject(e);
    if (n->kind == NL_CHECKED_STATEMENT) {
        return physical_current_scalar(e, env, n->initializer, out, 0);
    }
    const NLCheckedNodeView *init = get(env, n->initializer);
    if (n->kind != NL_CHECKED_BINDING || !init)
        return reject(e);
    if (n->initializer == p->constructor) {
        if ((!p->original_before_heap && !e->current_h_closed) ||
            e->current_emitted || !add_local(e, env, v->base, v->nominal, true))
            return reject(e);
        const Local *l = local(env, v->base, v->nominal);
        if (!l || l->place != v->root || l->incarnation != v->incarnation ||
            !put(e, "nl_current_record nl_b_%zu_%zu={", l->frame, l->symbol))
            return reject(e);
        for (NLCheckedNodeId field = init->first_argument; field;) {
            const NLCheckedNodeView *f = get(env, field);
            const NLCheckedNodeView *x = f ? get(env, f->initializer) : NULL;
            if (!f || !x)
                return reject(e);
            if (!put(e, ".f%zu=%zu,", f->field_index, x->scalar_result.value))
                return false;
            field = f->next_argument;
        }
        out->type = e->unit;
        out->temp = 0;
        return put(e,
                   "};\nNL_MIXED_COPY_BIND(%zu,%zu,&nl_b_%zu_%zu.f0,"
                   "&nl_b_%zu_%zu.f1);\n",
                   l->frame, l->symbol, l->frame, l->symbol, l->frame,
                   l->symbol);
    }
    Value value = {0};
    if (!(id == p->binding
              ? physical_current_loop(e, env, &value)
              : physical_current_scalar(e, env, n->initializer, &value, 0)) ||
        !add_local(e, env, n->symbol, value.type, false) ||
        !put(e, "%s nl_b_%zu_%zu=nl_v_%zu;\n(void)nl_b_%zu_%zu;\n",
             physical_current_type(e, value.type), env->frame, n->symbol,
             value.temp, env->frame, n->symbol))
        return reject(e);
    out->type = e->unit;
    out->temp = 0;
    return true;
}
