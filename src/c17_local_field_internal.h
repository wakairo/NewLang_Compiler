#ifndef NEWLANG_C17_LOCAL_FIELD_INTERNAL_H
#define NEWLANG_C17_LOCAL_FIELD_INTERNAL_H
#include "newlang/checked.h"

/* Read completed CHECK evidence in the working or authenticated context
 * supplied by the caller. Point-in-time facts survive lexical completion. */
static inline bool nl_c17_pair_type(const NLSemanticContext *context,
                                    NLTypeId type, NLTypeId u8)
{
    NLSemanticTypeView t;
    if (!nl_semantic_type_view(context, type, &t) ||
        t.kind != NL_TYPE_NOMINAL || t.field_count != 2 || !t.is_copy ||
        !t.is_discardable)
        return false;
    for (size_t i = 0; i < 2; ++i) {
        NLAggregateField field;
        if (!nl_semantic_aggregate_field_view(context, type, i, &field) ||
            field.type != u8)
            return false;
    }
    return true;
}

static inline bool nl_c17_resolved_field(const NLCheckedFragment *f,
                                         const NLSemanticContext *context,
                                         const NLCheckedField *e,
                                         NLTypeId aggregate, NLTypeId u8)
{
    if (!e->present || !e->dependency_compatible || e->base == 0 ||
        e->type != u8 || e->nominal != aggregate || e->index >= 2 ||
        e->parent == 0 || e->child == 0 || e->parent == e->child ||
        e->parent_incarnation == 0 || e->child_incarnation == 0 ||
        e->parent_fact == 0 || e->child_fact == 0)
        return false;
    NLAggregateField field;
    if (!nl_semantic_aggregate_field_view(context, e->nominal, e->index,
                                          &field) ||
        field.type != u8)
        return false;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_RECEIVER && v->symbol == e->base &&
            v->type == e->nominal)
            return true;
    }
    return false;
}

static inline bool nl_c17_scalar_ref(const NLSemanticContext *context,
                                     NLTypeId id, NLTypeId u8,
                                     NLAccessSyntax access)
{
    NLSemanticTypeView t;
    return nl_semantic_type_view(context, id, &t) && t.kind == NL_TYPE_REF &&
           t.target == u8 && !t.is_exclusive && t.access == access;
}

static inline bool nl_c17_completed_loan(const NLCheckedNodeView *v,
                                         const NLCheckedNodeView *body)
{
    const bool write = v->loan.access == NL_ACCESS_WRITE;
    return v->loan.implicit_local && v->loan.body_nonescape_proved &&
           v->loan.normal_result_forwarded && v->loan.ref_symbol &&
           v->loan.scope && v->loan.place && v->loan.incarnation &&
           (v->loan.access == NL_ACCESS_READ || write) &&
           !v->loan.is_exclusive && (!write || !v->loan.from_ptr) && body &&
           body->kind == NL_CHECKED_BLOCK && !body->terminates &&
           body->result_count == v->result_count && v->result_count <= 1;
}

static inline bool nl_c17_field_change(const NLCheckedFragment *f,
                                       const NLSemanticContext *context,
                                       const NLCheckedNodeView *v,
                                       NLTypeId aggregate, NLTypeId u8)
{
    return nl_c17_resolved_field(f, context, &v->field, aggregate, u8) &&
           v->field.access == NL_ACCESS_WRITE &&
           v->field.old_value == v->results[0].value &&
           v->field.parent_post_fact && v->field.child_post_fact &&
           v->field.parent_post_fact != v->field.parent_fact &&
           v->field.child_post_fact != v->field.child_fact;
}
#endif
