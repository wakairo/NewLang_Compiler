#include "semantic_internal.h"

/* Issue289 UNSELECTED finite source call. Shape admits an independent body
 * check; neither this predicate nor ordinary assembly grants matchedness. */
bool nl_three_owner_signature(const NLSemanticContext *c, const NLTypeId *p,
                              size_t count, NLTypeId result)
{
#ifdef NEWLANG_EXPERIMENTAL_THREE_ARG_OWNER_CALL
    if (!c || !p || count != 3 || !result || result > c->type_count ||
        !nl_experimental_nested_type(c, result) || !p[0] ||
        p[0] > c->type_count || p[1] != p[0] || !p[2] || p[2] > c->type_count ||
        !nl_experimental_root_record_type(c, p[0]))
        return false;
    const NLTypeEntry root = c->types[p[0] - 1], pair = c->types[result - 1];
    const NLSemanticTypeView token = c->types[p[2] - 1].view;
    return pair.field_types[0] == p[0] && pair.field_types[1] == p[1] &&
           token.kind == NL_TYPE_PTR && token.is_copy && token.is_discardable &&
           token.target == c->types[root.field_types[0] - 1].view.target;
#else
    (void)c;
    (void)p;
    (void)count;
    (void)result;
    return false;
#endif
}

/* Called on both the independent unmatched-formal definition artifact and
 * owned actual body. Copy token is intentionally inert: pure assembly does
 * not use it as root/ref/Allocation/Domain authority. No signature-only grant.
 * Closed body: return Pair{..., ...}, optionally let result first. */
bool nl_three_owner_body(const NLCheckedFragment *f,
                         const NLSymbolId parameters[3], NLValueId result)
{
#ifdef NEWLANG_EXPERIMENTAL_THREE_ARG_OWNER_CALL
    if (!f || !f->context || !parameters || !result ||
        result > f->context->value_count)
        return false;
    const NLSemanticContext *c = f->context;
    NLTypeId types[3];
    NLValueId inputs[3];
    for (size_t i = 0; i < 3; ++i) {
        if (!parameters[i] || parameters[i] > c->binding_count)
            return false;
        const NLSemanticBindingView b = c->bindings[parameters[i] - 1].view;
        if (!b.value || b.value > c->value_count ||
            b.availability != NL_CONSUMED)
            return false;
        types[i] = b.type;
        inputs[i] = b.value;
    }
    const NLSemanticValueView v = c->values[result - 1];
    if (!nl_three_owner_signature(c, types, 3, v.type) || v.field_count != 2 ||
        inputs[0] == inputs[1] ||
        !((v.fields[0] == inputs[0] && v.fields[1] == inputs[1]) ||
          (v.fields[1] == inputs[0] && v.fields[0] == inputs[1])))
        return false;
    const NLCheckedNodeView *block = nl_checked_node_view(f, f->root);
    if (!block || block->kind != NL_CHECKED_BLOCK || !block->terminates ||
        block->returned.value != result || block->returned.type != v.type ||
        block->tail || !block->item_count || block->item_count > 2)
        return false;
    NLCheckedNodeId item = block->first_item;
    for (size_t i = 0; i < block->item_count; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, item);
        if (!n || n->kind != (i + 1 == block->item_count ? NL_CHECKED_RETURN
                                                         : NL_CHECKED_BINDING))
            return false;
        item = n->next_item;
    }
    if (item)
        return false;
    size_t returns = 0, assemblies = 0, formal_uses[2] = {0};
    NLSymbolId local = 0;
    for (size_t i = 1; i <= f->count; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        switch (n->kind) {
        case NL_CHECKED_BLOCK:
            if (i != f->root)
                return false;
            break;
        case NL_CHECKED_AGGREGATE: {
            if (n->type != v.type || n->argument_count != 2 ||
                n->result_count != 1 || n->results[0].value != result)
                return false;
            bool seen[2] = {false};
            NLCheckedNodeId edge = n->first_argument;
            for (size_t j = 0; j < 2; ++j) {
                const NLCheckedNodeView *field = nl_checked_node_view(f, edge);
                if (!field || field->kind != NL_CHECKED_AGGREGATE_FIELD ||
                    field->field_index >= 2 || seen[field->field_index])
                    return false;
                const NLCheckedNodeView *input =
                    nl_checked_node_view(f, field->initializer);
                if (!input || input->kind != NL_CHECKED_IDENTIFIER ||
                    input->result_count != 1 ||
                    input->results[0].value != v.fields[field->field_index] ||
                    input->value_use != NL_VALUE_CONSUMED ||
                    field->type != input->type)
                    return false;
                seen[field->field_index] = true;
                edge = field->next_argument;
            }
            if (edge)
                return false;
            ++assemblies;
            break;
        }
        case NL_CHECKED_AGGREGATE_FIELD:
            if (n->field_index >= 2 || n->terminates)
                return false;
            break;
        case NL_CHECKED_BINDING: {
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, n->initializer);
            const NLCheckedNodeView *receiver =
                nl_checked_node_view(f, n->first_argument);
            if (!init || init->kind != NL_CHECKED_AGGREGATE ||
                init->result_count != 1 || init->results[0].value != result ||
                n->argument_count != 1 || !receiver ||
                receiver->kind != NL_CHECKED_RECEIVER || local)
                return false;
            local = receiver->symbol;
            break;
        }
        case NL_CHECKED_RECEIVER:
            if (n->symbol != local || n->type != v.type ||
                n->value_use != NL_VALUE_RECEIVED)
                return false;
            break;
        case NL_CHECKED_IDENTIFIER:
            if (n->value_use != NL_VALUE_CONSUMED || n->result_count != 1 ||
                !n->symbol || n->symbol > c->binding_count ||
                n->results[0].value != c->bindings[n->symbol - 1].view.value ||
                n->type != c->bindings[n->symbol - 1].view.type)
                return false;
            if (n->symbol == parameters[0])
                ++formal_uses[0];
            else if (n->symbol == parameters[1])
                ++formal_uses[1];
            else if (n->symbol != local || n->results[0].value != result)
                return false;
            break;
        case NL_CHECKED_RETURN: {
            const NLCheckedNodeView *input =
                nl_checked_node_view(f, n->initializer);
            if (!n->terminates || n->returned.value != result ||
                n->returned.type != v.type || !input ||
                input->result_count != 1 || input->results[0].value != result)
                return false;
            ++returns;
            break;
        }
        default:
            return false;
        }
    }
    return returns == 1 && assemblies == 1 && formal_uses[0] == 1 &&
           formal_uses[1] == 1;
#else
    (void)f;
    (void)parameters;
    (void)result;
    return false;
#endif
}
