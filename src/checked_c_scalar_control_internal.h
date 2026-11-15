#ifndef NEWLANG_CHECKED_C_SCALAR_CONTROL_INTERNAL_H
#define NEWLANG_CHECKED_C_SCALAR_CONTROL_INTERNAL_H

/* Transport only primitive values from the existing whole-root result seal.
 * This is not a branch/current/owner issuer. Ordinary IF arms below contain
 * no loans, fields, calls, allocation, mutation, or terminating edges. */
static bool scalar_control_scan(Emit *e, const NLCheckedFragment *f,
                                size_t depth, size_t *budget)
{
    if (!f || depth >= C_DEPTH || nl_checked_node_count(f) > *budget)
        return reject(e);
    *budget -= nl_checked_node_count(f);
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (!n)
            return reject(e);
        if (n->kind == NL_CHECKED_MATCH && n->type == e->boolean) {
            if (n->terminates || n->normal_arms != 2 || n->result_count != 1)
                return reject(e);
            e->scalar_control = true;
        }
        if (n->kind == NL_CHECKED_LINK_READ && n->type == e->option &&
            n->field.present && n->field.access == NL_ACCESS_READ) {
            NLSemanticValueView old;
            if (!nl_semantic_value_view(nl_checked_context(f),
                                        n->field.old_value, &old))
                return reject(e);
            if (old.variant == 1)
                ++e->scalar_none_expected;
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body && !scalar_control_scan(e, body, depth + 1, budget))
            return false;
        if (n->kind == NL_CHECKED_IF || n->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < n->item_count; ++a) {
                const NLCheckedFragment *arm =
                    n->kind == NL_CHECKED_IF ? nl_checked_if_arm(f, id, a)
                                             : nl_checked_match_arm(f, id, a);
                if (!scalar_control_scan(e, arm, depth + 1, budget))
                    return false;
            }
    }
    return true;
}

static bool scalar_control_prepare(Emit *e, const NLCheckedFragment *body)
{
    size_t budget = 4096;
    if (!scalar_control_scan(e, body, 0, &budget))
        return false;
    if (!e->scalar_control)
        return true;
    /* Keep the existing two-original producer/receiver route. An optional
     * effect-free None read is counted independently of its write protocol. */
    return nl_h_custody_result_world_present(e->origin_root,
                                             nl_checked_root(e->origin_root)) &&
                   e->allocated && e->two_heap && e->live_tail && !e->custody &&
                   !e->qualified_unchecked && e->scalar_none_expected <= 1
               ? true
               : reject(e);
}

static bool scalar_plain(const NLSemanticContext *c, NLValueId id,
                         NLTypeId type)
{
    NLSemanticValueView v;
    return id && nl_semantic_value_view(c, id, &v) && v.type == type &&
           v.dependencies == NL_DEPENDENCY_FREE && !v.value_dependency_count &&
           !v.reference_count && !v.reference.place && !v.reference.scope &&
           !v.reference.occurrence_dependency && !v.owner_place && !v.domain &&
           !v.allocation_region && !v.slot_place && !v.occupancy.region &&
           !v.aggregate_owner && !v.field_count && !v.variant &&
           !v.sum_payload && !v.sum_owner;
}

static bool scalar_if_body(const Emit *e, const NLCheckedFragment *f,
                           const NLCheckedNodeView *arm, NLCheckedNodeId id)
{
    const NLCheckedNodeView *b = nl_checked_node_view(f, id);
    if (!e->scalar_control || !arm || arm->kind != NL_CHECKED_IF_ARM ||
        arm->terminates || !scalar_result_type(e, arm->type) ||
        arm->result_count != 1 || !b || b->kind != NL_CHECKED_BLOCK ||
        b->terminates || b->type != arm->type || b->result_count != 1 ||
        arm->results[0].value != b->results[0].value ||
        !scalar_plain(nl_checked_context(f), b->results[0].value, b->type))
        return false;
    for (NLCheckedNodeId j = 1; j <= nl_checked_node_count(f); ++j) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, j);
        if (!n || n->terminates)
            return false;
        switch (n->kind) {
        case NL_CHECKED_IF_ARM:
        case NL_CHECKED_BLOCK:
        case NL_CHECKED_BINDING:
        case NL_CHECKED_RECEIVER:
        case NL_CHECKED_STATEMENT:
        case NL_CHECKED_UNIT:
        case NL_CHECKED_U8_LITERAL:
        case NL_CHECKED_USIZE_LITERAL:
        case NL_CHECKED_USIZE_LESS_THAN:
        case NL_CHECKED_USIZE_EQUALITY:
            break;
        case NL_CHECKED_IDENTIFIER:
            if (n->value_use != NL_VALUE_COPIED || n->result_count != 1 ||
                (n->type != e->u8 && n->type != e->boolean &&
                 n->type != e->usize) ||
                !scalar_plain(nl_checked_context(f), n->results[0].value,
                              n->type))
                return false;
            break;
        default:
            return false;
        }
    }
    return true;
}

static bool scalar_if(Emit *e, Env *env, NLCheckedNodeId id,
                      const NLCheckedNodeView *v, Value *out)
{
    const NLCheckedFragment *f = env->artifact;
    const NLCheckedNodeView *condition = get(env, v->initializer);
    if (!e->scalar_control || !e->scalar_issued || v->terminates || v->tail ||
        v->normal_arms != 2 || v->item_count != 2 || v->result_count != 1 ||
        !scalar_result_type(e, v->type) || !condition ||
        condition->kind != NL_CHECKED_IDENTIFIER ||
        condition->type != e->boolean || condition->result_count != 1 ||
        condition->value_use != NL_VALUE_COPIED ||
        !scalar_plain(nl_checked_context(f), condition->results[0].value,
                      e->boolean) ||
        !scalar_plain(nl_checked_context(f), v->results[0].value, v->type))
        return reject(e);
    Value actual = {0};
    if (!expr(e, env, v->initializer, &actual) || !actual.temp ||
        actual.type != e->boolean)
        return reject(e);
    out->type = v->type;
    out->temp = ++e->temp;
    if (!put(e, "uint8_t nl_v_%zu;\nNL_SCALAR_IF_CONDITION(nl_v_%zu);\n",
             out->temp, actual.temp))
        return false;
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, a);
        const NLCheckedNodeView *root =
            arm ? nl_checked_node_view(arm, nl_checked_root(arm)) : NULL;
        if (!arm || arm->if_parent != f || arm->if_parent_node != id ||
            arm->if_arm_index != a ||
            nl_checked_source(arm) != nl_checked_source(f) ||
            arm->branch_value_prefix != condition->results[0].value || !root ||
            root->type != v->type ||
            !scalar_if_body(e, arm, root, root->initializer) ||
            env->depth + 1 >= C_DEPTH)
            return reject(e);
        Env branch = {.artifact = arm,
                      .parent = env,
                      .prefix = SIZE_MAX,
                      .scalar_prefix = arm->branch_value_prefix,
                      .frame = ++e->frames,
                      .depth = env->depth + 1};
        if (!(a == 0 ? put(e, "if(nl_v_%zu){\n", actual.temp)
                     : put(e, "}else{\n")))
            return false;
        Value result = {0};
        if (!block(e, &branch, root->initializer, &result) ||
            result.type != v->type || !result.temp ||
            !put(e, "nl_v_%zu=nl_v_%zu;\nNL_SCALAR_IF_RESULT(%zu,nl_v_%zu);\n",
                 out->temp, result.temp, a, out->temp))
            return reject(e);
    }
    return put(e, "}\n");
}

static bool scalar_comparison(Emit *e, Env *env, const NLCheckedNodeView *v,
                              Value *out)
{
    size_t left_value, right_value;
    if (v->kind == NL_CHECKED_USIZE_LITERAL) {
        if (!copy_if_literal(env->artifact, v, e->usize, SIZE_MAX, &left_value))
            return reject(e);
        out->temp = ++e->temp;
        return put(e, "size_t nl_v_%zu=(size_t)UINTMAX_C(%zu);\n", out->temp,
                   left_value);
    }
    const NLCheckedNodeView *left = get(env, v->first_argument),
                            *right =
                                left ? get(env, left->next_argument) : NULL;
    if (v->type != e->boolean || v->argument_count != 2 ||
        v->result_count != 1 || !left || !right || right->next_argument ||
        left->kind != NL_CHECKED_USIZE_LITERAL ||
        right->kind != NL_CHECKED_USIZE_LITERAL ||
        !copy_if_literal(env->artifact, left, e->usize, SIZE_MAX,
                         &left_value) ||
        !copy_if_literal(env->artifact, right, e->usize, SIZE_MAX,
                         &right_value))
        return reject(e);
    const size_t lhs = ++e->temp, rhs = ++e->temp;
    out->temp = ++e->temp;
    return put(e,
               "size_t nl_v_%zu=(size_t)UINTMAX_C(%zu);\n"
               "size_t nl_v_%zu=(size_t)UINTMAX_C(%zu);\n"
               "uint8_t nl_v_%zu=(uint8_t)(nl_v_%zu%s nl_v_%zu);\n",
               lhs, left_value, rhs, right_value, out->temp, lhs,
               v->kind == NL_CHECKED_USIZE_LESS_THAN ? "<" : "==", rhs);
}

static bool scalar_control_preamble(Emit *e)
{
    return !e->scalar_control ||
           put(e,
               "_Static_assert(SIZE_MAX==UINTMAX_C(%zu),\"usize width\");\n"
               "#ifndef NL_SCALAR_BOOL_RESULT\n"
               "#define NL_SCALAR_BOOL_RESULT(...) ((void)0)\n#endif\n"
               "#ifndef NL_SCALAR_IF_CONDITION\n"
               "#define NL_SCALAR_IF_CONDITION(...) ((void)0)\n#endif\n"
               "#ifndef NL_SCALAR_IF_RESULT\n"
               "#define NL_SCALAR_IF_RESULT(...) ((void)0)\n#endif\n",
               SIZE_MAX);
}

#endif
