#ifndef NEWLANG_CHECKED_C_QUALIFIED_INTERNAL_H
#define NEWLANG_CHECKED_C_QUALIFIED_INTERNAL_H

/* This private consumer authenticates one finite whole-root source graph.  It
 * does not convert the author's pointer premise into NLReferenceFacts.  The
 * generated program evaluates the authenticated Bool call and transports the
 * pointer selected by that call; the premise is used only to admit the exact
 * local read under its real source Domain and closed loan. */
static bool qualified_record_equal(NLCheckedLocalAssumptionView a,
                                   NLCheckedLocalAssumptionView b)
{
    return a.source == b.source && a.owner == b.owner &&
           a.span.start_byte == b.span.start_byte &&
           a.span.end_byte == b.span.end_byte && a.operation == b.operation &&
           a.pointer_argument == b.pointer_argument &&
           a.stability_argument == b.stability_argument &&
           a.pointer_if == b.pointer_if && a.projection == b.projection &&
           a.read == b.read && a.target_type == b.target_type &&
           a.pointer_value == b.pointer_value &&
           a.reference_value == b.reference_value &&
           a.read_value == b.read_value && a.domain == b.domain &&
           a.scope == b.scope && a.conditional_target == b.conditional_target &&
           a.conditional_incarnation == b.conditional_incarnation &&
           a.conditional_range.region == b.conditional_range.region &&
           a.conditional_range.start == b.conditional_range.start &&
           a.conditional_range.length == b.conditional_range.length &&
           a.assumed_conditions == b.assumed_conditions &&
           a.stable_at_operation == b.stable_at_operation &&
           a.original_live_at_operation == b.original_live_at_operation &&
           a.loan_closed == b.loan_closed;
}

static bool qualified_find(const NLCheckedFragment *f, size_t depth,
                           size_t *budget, const NLCheckedFragment **owner,
                           NLCheckedLocalAssumptionView *record, size_t *found)
{
    if (!f || depth >= C_DEPTH)
        return false;
    const size_t count = nl_checked_node_count(f);
    if (count > *budget)
        return false;
    *budget -= count;
    for (NLCheckedNodeId id = 1; id <= count; ++id) {
        NLCheckedLocalAssumptionView view;
        if (nl_checked_local_assumption_view(f, id, &view)) {
            if (++*found != 1)
                return false;
            *owner = f;
            *record = view;
        }
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (!n)
            return false;
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body && !qualified_find(body, depth + 1, budget, owner, record,
                                    found))
            return false;
        if (n->kind == NL_CHECKED_IF || n->kind == NL_CHECKED_MATCH) {
            for (size_t a = 0; a < n->item_count; ++a) {
                const NLCheckedFragment *arm =
                    n->kind == NL_CHECKED_IF ? nl_checked_if_arm(f, id, a)
                                             : nl_checked_match_arm(f, id, a);
                if (!arm || !qualified_find(arm, depth + 1, budget, owner,
                                            record, found))
                    return false;
            }
        }
    }
    return true;
}

static bool qualified_plain_value(const NLSemanticContext *c, NLValueId id,
                                  NLTypeId type, NLSemanticValueView *out)
{
    return id && nl_semantic_value_view(c, id, out) && out->type == type &&
           out->dependencies == NL_DEPENDENCY_FREE &&
           !out->value_dependency_count && !out->reference_count &&
           !out->owner_place && !out->domain && !out->allocation_region &&
           !out->slot_place && !out->occupancy.region && !out->field_count &&
           !out->sum_payload && !out->aggregate_owner;
}

static bool qualified_pointer_lineage(const NLCheckedFragment *owner,
                                      const NLUncheckedProof *p,
                                      NLValueId destination, NLTypeId ptr)
{
    if (!owner || !p || !p->has_pointer || !p->pointer_result ||
        p->copy_count > NL_UNCHECKED_COPY_LIMIT)
        return false;
    const NLSemanticContext *c = nl_checked_context(owner);
    NLValueId current = p->pointer_result;
    bool used[NL_UNCHECKED_COPY_LIMIT] = {false};
    for (size_t step = 0; step < p->copy_count; ++step) {
        size_t selected = p->copy_count;
        for (size_t i = 0; i < p->copy_count; ++i)
            if (!used[i] && p->copies[i].from == current) {
                if (selected != p->copy_count)
                    return false;
                selected = i;
            }
        if (selected == p->copy_count)
            return false;
        const NLUncheckedCopy copy = p->copies[selected];
        const NLCheckedNodeView *node =
            nl_checked_node_view(owner, copy.node);
        NLSemanticBindingView binding;
        NLSemanticValueView from, to;
        if (!node || node->kind != NL_CHECKED_IDENTIFIER ||
            node->symbol != copy.binding || node->type != ptr ||
            node->value_use != NL_VALUE_COPIED || node->result_count != 1 ||
            node->results[0].type != ptr ||
            node->results[0].value != copy.to ||
            !nl_semantic_binding_view(c, copy.binding, &binding) ||
            binding.type != ptr || binding.value != copy.from ||
            !qualified_plain_value(c, copy.from, ptr, &from) ||
            !qualified_plain_value(c, copy.to, ptr, &to) ||
            from.reference.provenance != NL_PROVENANCE_UNKNOWN ||
            to.reference.provenance != NL_PROVENANCE_UNKNOWN ||
            from.reference.place || to.reference.place ||
            from.reference.scope || to.reference.scope)
            return false;
        used[selected] = true;
        current = copy.to;
    }
    return current == destination;
}

static bool qualified_prepare(Emit *e, const NLCheckedFragment *entry)
{
    const NLCheckedFragment *owner = NULL;
    NLCheckedLocalAssumptionView read = {0};
    size_t budget = 4096, found = 0;
    if (!e || !entry || nl_checked_proof_class(entry) !=
                            NL_CHECKED_AUTHOR_PREMISE ||
        !nl_h_unchecked_present(entry) ||
        !qualified_find(entry, 0, &budget, &owner, &read, &found) ||
        found != 1 || !owner || read.owner != owner ||
        read.source != nl_checked_source(owner) ||
        read.assumed_conditions != NL_AUTHOR_PTR_TARGET_DOMAIN ||
        !read.stable_at_operation || !read.original_live_at_operation ||
        !read.loan_closed || !read.operation || !read.pointer_argument ||
        !read.stability_argument || !read.pointer_if || !read.projection ||
        !read.read || !read.pointer_value || !read.reference_value ||
        !read.read_value || !read.domain || !read.scope ||
        !read.conditional_target || !read.conditional_incarnation ||
        read.conditional_range.length != 24 ||
        nl_checked_local_assumption_origin(owner, read.operation) !=
            NL_CHECK_OK)
        return false;
    const NLUncheckedProof *p = nl_h_unchecked_data(owner);
    if (!p || !p->has_pointer || !p->has_read ||
        !qualified_record_equal(read, p->read) ||
        p->pointer_if != read.pointer_if || p->pointer_type != e->ptr ||
        p->pointer_result == 0 || p->condition == 0)
        return false;

    const NLSemanticContext *c = nl_checked_context(owner);
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE);
    NLSemanticPlaceView target;
    NLSemanticDomainView domain;
    NLSemanticBackingView backing;
    NLPlaceId unique = 0;
    if (!boolean || !usize || read.target_type != e->node ||
        nl_unchecked_domain_target(c, e->node, read.domain, &unique) !=
            NL_CHECK_OK ||
        unique != read.conditional_target ||
        !nl_semantic_place_view(c, unique, &target) || !target.live ||
        !target.independent_root || target.type != e->node ||
        target.incarnation != read.conditional_incarnation ||
        target.governing_domain != read.domain ||
        target.placement.region != read.conditional_range.region ||
        target.placement.start != read.conditional_range.start ||
        target.placement.length != read.conditional_range.length ||
        !nl_semantic_domain_view(c, read.domain, &domain) || !domain.live ||
        !nl_semantic_backing_view(c, target.placement.region, &backing) ||
        !backing.live || !backing.ordinary_read || backing.size != 24)
        return false;

    const NLCheckedNodeView *operation =
        nl_checked_node_view(owner, read.operation);
    const NLCheckedNodeView *pointer =
        nl_checked_node_view(owner, read.pointer_argument);
    const NLCheckedNodeView *stable =
        nl_checked_node_view(owner, read.stability_argument);
    NLSemanticTypeView operation_type, stable_type;
    NLSemanticValueView pointer_value, reference_value;
    if (!operation || operation->kind != NL_CHECKED_REF_FROM_PTR ||
        operation->span.start_byte != read.span.start_byte ||
        operation->span.end_byte != read.span.end_byte ||
        operation->first_argument != read.pointer_argument ||
        operation->argument_count != 2 || operation->result_count != 1 ||
        operation->results[0].value != read.reference_value || !pointer ||
        pointer->kind != NL_CHECKED_IDENTIFIER ||
        pointer->next_argument != read.stability_argument ||
        pointer->type != e->ptr || pointer->value_use != NL_VALUE_COPIED ||
        pointer->result_count != 1 ||
        pointer->results[0].value != read.pointer_value || !stable ||
        stable->kind != NL_CHECKED_IDENTIFIER || stable->next_argument ||
        stable->value_use != NL_VALUE_COPIED || stable->result_count != 1 ||
        !nl_semantic_type_view(c, stable->type, &stable_type) ||
        stable_type.kind != NL_TYPE_REF || stable_type.is_exclusive ||
        stable_type.access != NL_ACCESS_READ ||
        stable_type.target != nl_semantic_domain_type(c) ||
        !nl_semantic_type_view(c, operation->type, &operation_type) ||
        operation_type.kind != NL_TYPE_REF || operation_type.is_exclusive ||
        operation_type.access != NL_ACCESS_READ ||
        operation_type.target != e->node ||
        !qualified_plain_value(c, read.pointer_value, e->ptr,
                               &pointer_value) ||
        pointer_value.reference.provenance != NL_PROVENANCE_UNKNOWN ||
        pointer_value.reference.place || pointer_value.reference.scope ||
        !qualified_plain_value(c, read.reference_value, operation->type,
                               &reference_value) ||
        reference_value.reference.provenance != NL_PROVENANCE_UNKNOWN ||
        reference_value.reference.place ||
        reference_value.reference.scope != read.scope ||
        !qualified_pointer_lineage(owner, p, read.pointer_value, e->ptr))
        return false;

    const NLCheckedNodeView *projection =
        nl_checked_node_view(owner, read.projection);
    const NLCheckedNodeView *load = nl_checked_node_view(owner, read.read);
    const NLCheckedNodeView *projection_arg =
        projection ? nl_checked_node_view(owner, projection->first_argument)
                   : NULL;
    if (!projection || projection->kind != NL_CHECKED_FIELD_REF ||
        projection->argument_count != 1 || projection->result_count != 1 ||
        projection->field.present || projection->field.nominal != e->node ||
        projection->field.type != e->option || projection->field.index != 0 ||
        projection->field.access != NL_ACCESS_READ || !projection_arg ||
        projection_arg->kind != NL_CHECKED_IDENTIFIER ||
        projection_arg->symbol != projection->field.base ||
        projection_arg->value_use != NL_VALUE_COPIED ||
        projection_arg->result_count != 1 || !load ||
        load->kind != NL_CHECKED_LINK_READ || load->type != e->option ||
        load->first_argument != read.projection || load->argument_count != 1 ||
        load->result_count != 1 || load->results[0].value != read.read_value ||
        load->field.present || load->field.nominal != e->node ||
        load->field.type != e->option || load->field.index != 0 ||
        load->field.access != NL_ACCESS_READ ||
        load->field.base != projection->field.base)
        return false;

    const NLCheckedNodeView *condition =
        nl_checked_node_view(owner, p->condition);
    const NLCheckedNodeView *choice =
        nl_checked_node_view(owner, p->pointer_if);
    if (!condition || condition->kind != NL_CHECKED_IDENTIFIER ||
        condition->type != boolean || condition->value_use != NL_VALUE_COPIED ||
        condition->result_count != 1 || !choice ||
        choice->kind != NL_CHECKED_IF || choice->initializer != p->condition ||
        choice->type != e->ptr || choice->terminates ||
        choice->normal_arms != 2 || choice->item_count != 2 || choice->tail ||
        choice->result_count != 1 ||
        choice->results[0].value != p->pointer_result)
        return false;
    NLSemanticValueView pointer_result;
    if (!qualified_plain_value(c, p->pointer_result, e->ptr,
                               &pointer_result) ||
        pointer_result.reference.provenance != NL_PROVENANCE_UNKNOWN ||
        pointer_result.reference.place || pointer_result.reference.scope)
        return false;

    NLCheckedNodeId binding_id = 0;
    const NLCheckedNodeView *binding = NULL;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(owner); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(owner, id);
        if (node->kind == NL_CHECKED_BINDING &&
            node->symbol == condition->symbol) {
            if (binding)
                return false;
            binding = node;
            binding_id = id;
        }
    }
    const NLCheckedNodeView *call =
        binding ? nl_checked_node_view(owner, binding->initializer) : NULL;
    const NLCheckedFragment *body =
        call ? nl_checked_call_body(owner, binding->initializer) : NULL;
    const NLCheckedNodeView *argument =
        call ? nl_checked_node_view(owner, call->first_argument) : NULL;
    if (!binding || binding->argument_count != 1 || !call || !body ||
        call->kind != NL_CHECKED_REGISTERED_CALL || !call->body_backed ||
        call->terminates || call->type != boolean || call->result_count != 1 ||
        call->results[0].type != boolean || !call->function ||
        call->function > c->function_count || !c->functions ||
        call->argument_count != 1 || !argument || argument->next_argument ||
        argument->kind != NL_CHECKED_USIZE_LITERAL || argument->type != usize ||
        argument->parameter_type != usize || argument->result_count != 1 ||
        !argument->has_scalar_result || !argument->scalar_result.known ||
        argument->scalar_result.type != usize)
        return false;
    const NLFunctionEntry *function = &c->functions[call->function - 1];
    if (function->kind != NL_CHECKED_REGISTERED_CALL ||
        function->result != boolean || function->count != 1 ||
        !function->parameters || function->parameters[0] != usize ||
        function->caller_effects || function->hidden_dependencies ||
        function->owner_receiver || function->owner_producer ||
        function->custody_recipient || !function->body ||
        function->body->count != 1 || !function->body->parameter_names[0] ||
        body->body_owner != function->body ||
        body->source != function->body->source ||
        !function->body->syntax ||
        nl_syntax_tree_source(function->body->syntax) != body->source)
        return false;
    const NLCheckedNodeView *body_root =
        nl_checked_node_view(body, nl_checked_root(body));
    const NLCheckedNodeView *tail =
        body_root ? nl_checked_node_view(body, body_root->tail) : NULL;
    const NLCheckedNodeView *left =
        tail ? nl_checked_node_view(body, tail->first_argument) : NULL;
    const NLCheckedNodeView *right =
        left ? nl_checked_node_view(body, left->next_argument) : NULL;
    if (!body_root || body_root->kind != NL_CHECKED_BLOCK ||
        body_root->type != boolean || body_root->result_count != 1 || !tail ||
        (tail->kind != NL_CHECKED_USIZE_EQUALITY &&
         tail->kind != NL_CHECKED_USIZE_LESS_THAN) ||
        tail->type != boolean || tail->argument_count != 2 ||
        tail->result_count != 1 || !left || !right || right->next_argument ||
        left->kind != NL_CHECKED_IDENTIFIER ||
        right->kind != NL_CHECKED_IDENTIFIER || left->type != usize ||
        right->type != usize ||
        body_root->results[0].value != tail->results[0].value ||
        tail->results[0].value != call->results[0].value)
        return false;
    NLSymbolId formal = 0;
    for (size_t i = 0; i < c->binding_count; ++i) {
        const NLBindingEntry *candidate = &c->bindings[i];
        if (candidate->hidden && candidate->name &&
            candidate->view.type == usize &&
            candidate->view.value == argument->results[0].value &&
            !strcmp(candidate->name, function->body->parameter_names[0])) {
            if (formal)
                return false;
            formal = i + 1;
        }
    }
    NLSemanticBindingView decision_binding;
    NLSemanticValueView decision;
    if (!formal || left->symbol != formal || right->symbol != formal ||
        !nl_semantic_binding_view(c, condition->symbol, &decision_binding) ||
        decision_binding.type != boolean ||
        decision_binding.value != call->results[0].value ||
        !nl_semantic_value_view(c, condition->results[0].value, &decision) ||
        decision.type != boolean || !decision.scalar_known ||
        decision.scalar_value !=
            (tail->kind == NL_CHECKED_USIZE_EQUALITY ? 1u : 0u))
        return false;

    const size_t selected =
        tail->kind == NL_CHECKED_USIZE_EQUALITY ? 0u : 1u;
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(owner, p->pointer_if, a);
        const NLCheckedNodeView *root =
            arm ? nl_checked_node_view(arm, nl_checked_root(arm)) : NULL;
        const NLCheckedNodeView *block =
            root ? nl_checked_node_view(arm, root->initializer) : NULL;
        const NLCheckedNodeView *value =
            block ? nl_checked_node_view(arm, block->tail) : NULL;
        NLSemanticValueView prefix, copied;
        if (!arm || p->arms[a] != arm || !root ||
            root->kind != NL_CHECKED_IF_ARM || root->terminates ||
            root->result_count != 1 || !block ||
            block->kind != NL_CHECKED_BLOCK || block->terminates ||
            block->item_count || block->result_count != 1 ||
            p->tails[a] != block->tail || !value ||
            value->kind != NL_CHECKED_IDENTIFIER || value->type != e->ptr ||
            value->value_use != NL_VALUE_COPIED || value->result_count != 1 ||
            root->results[0].value != value->results[0].value ||
            block->results[0].value != value->results[0].value ||
            !qualified_plain_value(nl_checked_context(arm),
                                   p->prefix_values[a], e->ptr, &prefix) ||
            !qualified_plain_value(nl_checked_context(arm),
                                   value->results[0].value, e->ptr, &copied) ||
            prefix.reference.provenance != NL_PROVENANCE_VALID ||
            copied.reference.provenance != NL_PROVENANCE_VALID ||
            prefix.reference.place != copied.reference.place ||
            prefix.reference.incarnation != copied.reference.incarnation ||
            !prefix.reference.readable || !copied.reference.readable ||
            (a == selected &&
             (prefix.reference.place != read.conditional_target ||
              prefix.reference.incarnation !=
                  read.conditional_incarnation)))
            return false;
    }

    e->qualified_unchecked = true;
    e->qualified_owner = owner;
    e->qualified_body = body;
    e->qualified_recipe = p;
    e->qualified_read = read;
    e->qualified_binding = binding_id;
    e->qualified_call = binding->initializer;
    e->qualified_condition = p->condition;
    e->qualified_if = p->pointer_if;
    e->qualified_predicate = tail->kind;
    e->qualified_bool = boolean;
    e->qualified_usize = usize;
    return true;
}

static bool qualified_bool_call(Emit *e, Env *env, NLCheckedNodeId id,
                                const NLCheckedNodeView *call, Value *out)
{
    if (!e->qualified_unchecked || env->artifact != e->qualified_owner ||
        id != e->qualified_call || ++e->qualified_calls != 1 ||
        call != get(env, id))
        return reject(e);
    const NLCheckedNodeView *argument = get(env, call->first_argument);
    if (!argument || argument->kind != NL_CHECKED_USIZE_LITERAL ||
        !argument->has_scalar_result || !argument->scalar_result.known ||
        argument->scalar_result.type != e->qualified_usize)
        return reject(e);
    const size_t actual = ++e->temp;
    out->type = e->qualified_bool;
    out->temp = ++e->temp;
    return put(e,
               "const size_t nl_v_%zu=(size_t)UINTMAX_C(%zu);\n"
               "const uint8_t nl_v_%zu=nl_qualified_predicate(nl_v_%zu);\n"
               "NL_QUALIFIED_CONDITION(nl_v_%zu,nl_v_%zu);\n",
               actual, argument->scalar_result.value, out->temp, actual,
               out->temp, actual);
}

static bool qualified_ptr_if(Emit *e, Env *env, NLCheckedNodeId id,
                             const NLCheckedNodeView *choice, Value *out)
{
    if (!e->qualified_unchecked || env->artifact != e->qualified_owner ||
        id != e->qualified_if || choice != get(env, id) ||
        ++e->ptr_ifs != 1 || ++e->qualified_conditions != 1)
        return reject(e);
    Value condition = {0};
    if (!expr(e, env, choice->initializer, &condition) ||
        condition.type != e->qualified_bool || !condition.temp)
        return reject(e);
    const Local *carriers[2] = {0};
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm =
            nl_checked_if_arm(env->artifact, id, a);
        const NLCheckedNodeView *root =
            arm ? nl_checked_node_view(arm, nl_checked_root(arm)) : NULL;
        const NLCheckedNodeView *block =
            root ? nl_checked_node_view(arm, root->initializer) : NULL;
        const NLCheckedNodeView *value =
            block ? nl_checked_node_view(arm, block->tail) : NULL;
        if (!value || !(carriers[a] = local(env, value->symbol, e->ptr)))
            return reject(e);
    }
    out->type = e->ptr;
    out->temp = ++e->temp;
    return put(e,
               "const nl_node *nl_v_%zu;\n"
               "if(nl_v_%zu){nl_v_%zu=nl_b_%zu_%zu;"
               "NL_NODE_ARM(0,nl_v_%zu);}\n"
               "else{nl_v_%zu=nl_b_%zu_%zu;"
               "NL_NODE_ARM(1,nl_v_%zu);}\n",
               out->temp, condition.temp, out->temp, carriers[0]->frame,
               carriers[0]->symbol, out->temp, out->temp, carriers[1]->frame,
               carriers[1]->symbol, out->temp);
}

static bool qualified_ref(Emit *e, Env *env, const NLCheckedNodeView *op,
                          Value *out)
{
    if (!e->qualified_unchecked || env->artifact != e->qualified_owner ||
        op != get(env, e->qualified_read.operation) ||
        env->ptr_observation != op || ++e->qualified_refs != 1)
        return reject(e);
    const NLCheckedNodeView *pointer = get(env, op->first_argument);
    const NLCheckedNodeView *stable =
        pointer ? get(env, pointer->next_argument) : NULL;
    const Env *target = domain_heap(env, e->qualified_read.domain);
    Value actual = {0}, stability = {0};
    if (!pointer || !stable ||
        pointer != get(env, e->qualified_read.pointer_argument) ||
        stable != get(env, e->qualified_read.stability_argument) || !target ||
        target->region_slot >= 2 ||
        target->heap_place != e->qualified_read.conditional_target ||
        target->heap_incarnation !=
            e->qualified_read.conditional_incarnation ||
        target->backing != e->qualified_read.conditional_range.region ||
        e->lifecycle[target->region_slot].stage != 4 ||
        env->stability_scope != e->qualified_read.scope ||
        env->stability_domain != e->qualified_read.domain ||
        !expr(e, env, op->first_argument, &actual) ||
        !expr(e, env, pointer->next_argument, &stability) ||
        actual.type != e->ptr || !actual.temp || !stability.temp)
        return reject(e);
    out->type = op->type;
    out->temp = ++e->temp;
    return put(e,
               "const nl_node *nl_v_%zu=nl_v_%zu;\n"
               "NL_HEAP_ROOT(nl_v_%zu,nl_v_%zu->token,0,%zu,%zu,%zu);\n"
               "(void)nl_v_%zu;\n",
               out->temp, actual.temp, out->temp, stability.temp,
               e->qualified_read.conditional_target,
               e->qualified_read.conditional_incarnation,
               e->qualified_read.scope, stability.temp);
}

static bool qualified_projection(Emit *e, Env *env,
                                 const NLCheckedNodeView *projection,
                                 Value *out)
{
    if (!e->qualified_unchecked || env->artifact != e->qualified_owner ||
        projection != get(env, e->qualified_read.projection) ||
        ++e->qualified_projections != 1)
        return reject(e);
    Value base = {0};
    if (!expr(e, env, projection->first_argument, &base) || !base.temp)
        return reject(e);
    out->type = projection->type;
    out->temp = ++e->temp;
    return put(e,
               "const nl_node_option *nl_v_%zu=&nl_v_%zu->f0;\n"
               "NL_HEAP_FIELD(nl_v_%zu,nl_v_%zu,0,%zu,0,%zu,%zu,%zu);\n",
               out->temp, base.temp, base.temp, out->temp, e->node,
               e->qualified_read.conditional_target,
               e->qualified_read.conditional_incarnation,
               e->qualified_read.scope);
}

static bool qualified_load(Emit *e, Env *env, const NLCheckedNodeView *load,
                           Value *out)
{
    if (!e->qualified_unchecked || env->artifact != e->qualified_owner ||
        load != get(env, e->qualified_read.read) ||
        ++e->qualified_loads != 1 || ++e->ptr_reads != 1)
        return reject(e);
    Value projected = {0};
    if (!expr(e, env, load->first_argument, &projected) || !projected.temp)
        return reject(e);
    out->type = e->option;
    out->temp = ++e->temp;
    return put(e,
               "nl_node_option nl_v_%zu=*nl_v_%zu;\n"
               "NL_HEAP_COPY(nl_v_%zu,&nl_v_%zu);\n",
               out->temp, projected.temp, projected.temp, out->temp);
}

static bool qualified_preamble(Emit *e)
{
    if (!e->qualified_unchecked)
        return true;
    const char *operator_text =
        e->qualified_predicate == NL_CHECKED_USIZE_EQUALITY ? "==" : "<";
    return put(e,
               "_Static_assert(SIZE_MAX==UINTMAX_C(%zu),\"usize width\");\n"
               "#ifndef NL_QUALIFIED_CONDITION\n"
               "#define NL_QUALIFIED_CONDITION(...) ((void)0)\n#endif\n"
               "static uint8_t nl_qualified_predicate(size_t n){"
               "return (uint8_t)(n%s n);}\n",
               SIZE_MAX, operator_text);
}

static bool qualified_complete(const Emit *e)
{
    return !e->qualified_unchecked ||
           (e->qualified_calls == 1 && e->qualified_conditions == 1 &&
            e->qualified_refs == 1 && e->qualified_projections == 1 &&
            e->qualified_loads == 1 && e->ptr_ifs == 1 &&
            e->ptr_roundtrips == 0 && e->ptr_reads == 1);
}

#endif
