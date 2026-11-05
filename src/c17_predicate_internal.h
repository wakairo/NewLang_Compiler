#ifndef NEWLANG_C17_PREDICATE_INTERNAL_H
#define NEWLANG_C17_PREDICATE_INTERNAL_H

#include "pure_u8_internal.h"

/* Compose only with a T constructed before this actual scalar loop. The
 * existing loop validator seals the complete caller and exact persistent
 * frame; the pure callee below cannot receive or modify that T. This is a
 * borrowed source-origin check, not a new state bridge or stored receipt. */
static bool predicate_original_frame(const NLCheckedFragment *f,
                                     NLCheckedNodeId call_id,
                                     const V0Program *p)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, call_id);
    NLCheckedNodeId loop = 0;
    V0Program operands = *p;
    if (!call ||
        !quantity_scalar_impl(f, call->first_argument, &operands, true,
                              &loop) ||
        !loop)
        return false;
    const NLCheckedNodeView *origin = nl_checked_node_view(f, loop);
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f));
    if (!origin || !root || root->kind != NL_CHECKED_BLOCK ||
        origin->span.end_byte > call->span.start_byte)
        return false;
    bool original = false;
    size_t visited = 0;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (!n || ++visited > nl_checked_node_count(f))
            return false;
        if (n->kind == NL_CHECKED_BINDING && n->initializer == loop)
            return original;
        const NLCheckedNodeView *receiver =
            nl_checked_node_view(f, n->first_argument);
        if (n->kind == NL_CHECKED_BINDING && n->argument_count == 1 &&
            receiver && receiver->kind == NL_CHECKED_RECEIVER &&
            receiver->type == p->aggregate && receiver->symbol == n->symbol &&
            n->span.end_byte <= origin->span.start_byte &&
            avs_construction(f, n->initializer, &operands))
            original = true;
        id = n->next_item;
    }
    return false;
}

static bool predicate_call(const NLCheckedFragment *f, NLCheckedNodeId id,
                           V0Program *p)
{
    if (!nl_h_custody_result_valid(f, id))
        return false;
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (!call || !c || !body || !c->functions || call->type != p->u8 ||
        call->argument_count != 1 || call->result_count != 1 ||
        call->results[0].type != p->u8 || !call->function ||
        call->function > c->function_count || p->scalar_depth ||
        p->formal_body || p->unit_parameters || p->u8_export ||
        (p->aggregate && !predicate_original_frame(f, id, p)) ||
        p->linear_break)
        return false;
    NLPureU8Call pure;
    NLSymbolId formal;
    if (!nl_pure_u8_call(f, id, &pure) ||
        !v1_call_parameter_kind(f, id, p, &formal, true) ||
        formal != pure.formal)
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
