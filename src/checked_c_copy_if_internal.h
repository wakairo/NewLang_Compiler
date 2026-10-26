#ifndef NEWLANG_CHECKED_C_COPY_IF_INTERNAL_H
#define NEWLANG_CHECKED_C_COPY_IF_INTERNAL_H
#include "semantic_internal.h"

/* A consumer of an existing validated Copy call entry, not an IF issuer.
 * Literal-only forks must retain its complete original prefix. No fork-local
 * place or authority is imported into the C carrier environment. */
static bool copy_if_prefix_same(const NLSemanticContext *arm,
                                const NLSemanticContext *entry, size_t prefix)
{
    NLSemanticSnapshot a, b;
    if (!nl_semantic_snapshot(arm, &a) || !nl_semantic_snapshot(entry, &b) ||
        a.values != prefix + 1 || b.values < a.values || a.types != b.types ||
        a.functions != b.functions || a.bindings > b.bindings ||
        a.places > b.places || a.scopes > b.scopes || a.domains != b.domains ||
        a.backing_regions != b.backing_regions ||
        a.raw_intervals != b.raw_intervals || a.occurrences != b.occurrences)
        return false;
#define SAME_VIEW(count, type, getter)                                         \
    for (size_t j = 1; j <= (count); ++j) {                                    \
        type x, y;                                                             \
        if (!getter(arm, j, &x) || !getter(entry, j, &y) ||                    \
            memcmp(&x, &y, sizeof(x)))                                         \
            return false;                                                      \
    }
    SAME_VIEW(a.types, NLSemanticTypeView, nl_semantic_type_view)
    SAME_VIEW(a.bindings, NLSemanticBindingView, nl_semantic_binding_view)
    SAME_VIEW(a.places, NLSemanticPlaceView, nl_semantic_place_view)
    SAME_VIEW(prefix, NLSemanticValueView, nl_semantic_value_view)
    SAME_VIEW(a.scopes, NLSemanticScopeView, nl_semantic_scope_view)
    SAME_VIEW(a.domains, NLSemanticDomainView, nl_semantic_domain_view)
    SAME_VIEW(a.occurrences, NLSemanticOccurrenceView,
              nl_semantic_occurrence_view)
    SAME_VIEW(a.backing_regions, NLSemanticBackingView,
              nl_semantic_backing_view)
#undef SAME_VIEW
    for (size_t j = 1; j <= a.backing_regions; ++j) {
        NLSemanticBackingView region;
        if (!nl_semantic_backing_view(arm, j, &region) || !region.live ||
            region.size != 24)
            return false;
        for (size_t offset = 0; offset < region.size; ++offset) {
            NLRawRepView x, y;
            if (!nl_semantic_raw_rep_view(arm, j, offset, &x) ||
                !nl_semantic_raw_rep_view(entry, j, offset, &y) ||
                memcmp(&x, &y, sizeof(x)))
                return false;
        }
    }
    return true;
}

static bool copy_if_literal(const NLCheckedFragment *f,
                            const NLCheckedNodeView *n, NLTypeId type,
                            size_t maximum, size_t *value)
{
    NLSemanticValueView v;
    if (!n || n->terminates || n->type != type || n->result_count != 1 ||
        n->results[0].type != type || !n->has_scalar_result ||
        n->scalar_result.type != type || !n->scalar_result.known ||
        n->scalar_result.value > maximum ||
        !nl_semantic_value_view(nl_checked_context(f), n->results[0].value,
                                &v) ||
        v.type != type || !v.scalar_known ||
        v.scalar_value != n->scalar_result.value ||
        v.dependencies != NL_DEPENDENCY_FREE || v.value_dependency_count ||
        v.reference_count || v.reference.place || v.reference.scope ||
        v.domain || v.allocation_region || v.slot_place || v.occupancy.region ||
        v.field_count || v.sum_payload || v.sum_owner)
        return false;
    *value = n->scalar_result.value;
    return true;
}

static bool copy_value_if(Emit *e, const Env *env, NLCheckedNodeId id,
                          const NLCheckedNodeView *v, Value *out)
{
    const NLCheckedFragment *f = env->artifact;
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE),
                   boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    const NLCheckedNodeView *condition = get(env, v->initializer);
    if (!e->copy_anchor || f != e->copy_issuer || v->terminates ||
        v->normal_arms != 2 || v->item_count != 2 || v->tail ||
        v->has_scalar_result || v->type != e->u8 || v->result_count != 1 ||
        v->results[0].type != e->u8 || !condition ||
        (condition->kind != NL_CHECKED_USIZE_EQUALITY &&
         condition->kind != NL_CHECKED_USIZE_LESS_THAN) ||
        condition->terminates || condition->type != boolean ||
        condition->result_count != 1 || condition->results[0].type != boolean ||
        condition->argument_count != 2 || v->initializer >= id)
        return reject(e);
    const NLCheckedNodeView *left = get(env, condition->first_argument),
                            *right =
                                left ? get(env, left->next_argument) : NULL;
    size_t lhs, rhs;
    NLSemanticValueView b, result;
    if (!left || !right || left->kind != NL_CHECKED_USIZE_LITERAL ||
        right->kind != NL_CHECKED_USIZE_LITERAL || right->next_argument ||
        !copy_if_literal(f, left, usize, SIZE_MAX, &lhs) ||
        !copy_if_literal(f, right, usize, SIZE_MAX, &rhs) ||
        !nl_semantic_value_view(c, condition->results[0].value, &b) ||
        b.type != boolean || !b.scalar_known ||
        b.scalar_value != (size_t)(condition->kind == NL_CHECKED_USIZE_EQUALITY
                                       ? lhs == rhs
                                       : lhs < rhs) ||
        b.dependencies != NL_DEPENDENCY_FREE || b.value_dependency_count ||
        b.reference_count || b.reference.place || b.domain ||
        b.allocation_region || b.slot_place || b.occupancy.region ||
        b.field_count || b.sum_payload ||
        !nl_semantic_value_view(c, v->results[0].value, &result) ||
        result.type != e->u8 || result.scalar_known ||
        result.dependencies != NL_DEPENDENCY_FREE ||
        result.value_dependency_count || result.reference_count ||
        result.reference.place || result.domain || result.allocation_region ||
        result.slot_place || result.occupancy.region || result.field_count ||
        result.sum_payload)
        return reject(e);
    const NLCheckedFragment *arms[2] = {nl_checked_if_arm(f, id, 0),
                                        nl_checked_if_arm(f, id, 1)};
    size_t choices[2];
    NLSemanticSnapshot first = {0};
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = arms[a];
        if (!arm || arm == f || nl_checked_context(arm) == c ||
            (a && nl_checked_context(arm) == nl_checked_context(arms[0])) ||
            arm->if_parent != f || arm->if_parent_node != id ||
            arm->if_arm_index != a ||
            nl_checked_source(arm) != nl_checked_source(f) || arm->count != 3 ||
            arm->arm_count || arm->body_count || arm->exits ||
            arm->loop_returns ||
            arm->branch_value_prefix != condition->results[0].value ||
            v->results[0].value != arm->branch_value_prefix + 1)
            return reject(e);
        size_t edges = 0;
        for (size_t j = 0; j < f->arm_count; ++j) {
            const NLCheckedArm *edge = &f->arms[j];
            if (edge->match == id) {
                if (edges >= 2 || edge->artifact != arms[edges] ||
                    edge->context_origin != nl_checked_context(arms[edges]))
                    return reject(e);
                ++edges;
            }
        }
        NLSemanticSnapshot state;
        if (edges != 2 ||
            !nl_semantic_snapshot(nl_checked_context(arm), &state) ||
            state.last_value_fact != arm->branch_fact_prefix ||
            !copy_if_prefix_same(nl_checked_context(arm), e->copy_anchor,
                                 arm->branch_value_prefix))
            return reject(e);
        if (a && memcmp(&state, &first, sizeof(state)))
            return reject(e);
        first = state;
        const NLCheckedNodeView *root =
            nl_checked_node_view(arm, nl_checked_root(arm));
        const NLCheckedNodeView *block =
            root ? nl_checked_node_view(arm, root->initializer) : NULL;
        const NLCheckedNodeView *literal =
            block ? nl_checked_node_view(arm, block->tail) : NULL;
        if (!root || root->kind != NL_CHECKED_IF_ARM || root->terminates ||
            root->type != e->u8 || root->result_count != 1 || !block ||
            block->kind != NL_CHECKED_BLOCK || block->terminates ||
            block->item_count || block->first_item || block->type != e->u8 ||
            block->result_count != 1 || !literal ||
            literal->kind != NL_CHECKED_U8_LITERAL || root == block ||
            root == literal || block == literal ||
            !copy_if_literal(arm, literal, e->u8, 255, &choices[a]) ||
            root->results[0].value != literal->results[0].value ||
            block->results[0].value != literal->results[0].value ||
            literal->results[0].value != arm->branch_value_prefix + 1 ||
            root->span.start_byte <= v->span.start_byte ||
            root->span.end_byte > v->span.end_byte)
            return reject(e);
    }
    const NLCheckedNodeView *then = nl_checked_node_view(
                                arms[0], nl_checked_root(arms[0])),
                            *otherwise = nl_checked_node_view(
                                arms[1], nl_checked_root(arms[1]));
    if (then->span.end_byte >= otherwise->span.start_byte)
        return reject(e);
    const size_t l = ++e->temp, r = ++e->temp;
    out->type = e->u8;
    out->temp = ++e->temp;
    return put(e,
               "_Static_assert(SIZE_MAX == UINTMAX_C(%zu), "
               "\"checked usize target width\");\n"
               "const size_t nl_v_%zu=(size_t)UINTMAX_C(%zu);\n"
               "const size_t nl_v_%zu=(size_t)UINTMAX_C(%zu);\n"
               "uint8_t nl_v_%zu;\n"
               "if(nl_v_%zu %s nl_v_%zu){nl_v_%zu=%zu;}"
               "else{nl_v_%zu=%zu;}\n",
               SIZE_MAX, l, lhs, r, rhs, out->temp, l,
               condition->kind == NL_CHECKED_USIZE_EQUALITY ? "==" : "<", r,
               out->temp, choices[0], out->temp, choices[1]);
}
#endif
