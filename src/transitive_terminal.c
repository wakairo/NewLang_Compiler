#include "semantic_internal.h"
#include <string.h>

/* Issue 278 finite source adapter. Only paths are correlated at definition
 * time. No abstract H world, Storage, pointer provenance or grant is seeded. */
static bool spelling(const NLFunctionBody *b, NLSourceSpan s, const char *name)
{
    NLSourceView v;
    return nl_source_view(b->source, s, &v) && strlen(name) == v.length &&
           memcmp(name, v.bytes, v.length) == 0;
}
static bool same(const NLFunctionBody *b, NLSourceSpan a, NLSourceSpan z)
{
    NLSourceView x, y;
    return nl_source_view(b->source, a, &x) &&
           nl_source_view(b->source, z, &y) && x.length == y.length &&
           memcmp(x.bytes, y.bytes, x.length) == 0;
}
static NLCheckStatus error(NLCheckDiagnostic *d, NLSourceSpan s,
                           NLCheckStatus status, const char *code,
                           const char *message)
{
    if (d)
        *d = (NLCheckDiagnostic){{NL_DIAG_ERROR,
                                  status == NL_CHECK_ANALYSIS_PRECISION_LIMIT
                                      ? "precision"
                                      : "semantic",
                                  code, message, NULL, NULL, 0},
                                 s};
    return status;
}
NLCheckStatus nl_two_root_definition(const NLSemanticContext *c,
                                     const NLFunctionBody *b, NLTypeId type,
                                     NLTwoRootDefinition *out,
                                     NLCheckDiagnostic *diag)
{
    if (!c || !b || !b->syntax || !out || !type || type > c->type_count)
        return NL_CHECK_INTERNAL_ERROR;
    const NLSyntaxView *block =
        nl_syntax_node_view(nl_syntax_tree_root(b->syntax));
    if (!block || block->kind != NL_SYNTAX_BLOCK)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLSyntaxNode *item = block->data.block.items;
    const NLSyntaxView *split = nl_syntax_node_view(item);
    NLTwoRootDefinition result = {.type = type};
    NLSourceSpan names[2];
    bool available[2] = {true, true};
    if (!nl_experimental_nested_type(c, type) || b->count != 1 ||
        block->kind != NL_SYNTAX_BLOCK || split == NULL ||
        split->kind != NL_SYNTAX_AGGREGATE_BINDING)
        goto profile;
    const NLTypeEntry *record = &c->types[type - 1];
    const NLSyntaxView *input =
        nl_syntax_node_view(split->data.aggregate.initializer);
    if (!spelling(b, split->data.aggregate.type_name, record->name) ||
        split->data.aggregate.count != 2 || input == NULL ||
        input->kind != NL_SYNTAX_EXPR_NAME ||
        !spelling(b, input->data.name, b->parameter_names[0]))
        return error(diag, split->span, NL_CHECK_SEMANTIC_ERROR,
                     "P278-DEFINITION-WHOLE",
                     "requires complete formal decomposition");
    bool seen[2] = {false, false};
    for (const NLSyntaxNode *field = split->data.aggregate.fields; field;
         field = nl_syntax_next_argument(field)) {
        const NLSourceSpan name = nl_syntax_node_view(field)->data.name;
        NLSourceView lexical;
        if (!nl_source_view(b->source, name, &lexical) ||
            !nl_sem_lexical_name_admissible(lexical.bytes, lexical.length) ||
            spelling(b, name, b->parameter_names[0]))
            return error(diag, name, NL_CHECK_SEMANTIC_ERROR,
                         "P278-DEFINITION-NAME",
                         "duplicate or inadmissible lexical binder");
        size_t index = 2;
        for (size_t j = 0; j < 2; ++j)
            if (spelling(b, name, record->field_names[j]))
                index = j;
        if (index == 2 || seen[index])
            return error(diag, name, NL_CHECK_SEMANTIC_ERROR,
                         "P278-DEFINITION-WHOLE",
                         "duplicate or unknown member path");
        seen[index] = true;
        names[index] = name;
    }
    if (!seen[0] || !seen[1])
        goto profile;
    for (item = nl_syntax_next_argument(item); item;
         item = nl_syntax_next_argument(item)) {
        const NLSyntaxView *statement = nl_syntax_node_view(item);
        if (statement->kind != NL_SYNTAX_STATEMENT || result.count == 2)
            goto profile;
        const NLSyntaxView *call =
            nl_syntax_node_view(statement->data.statement.expression);
        if (call == NULL || call->kind != NL_SYNTAX_EXPR_CALL ||
            call->data.call.argument_count != 1)
            goto profile;
        const NLSyntaxView *argument =
            nl_syntax_node_view(call->data.call.arguments);
        if (argument == NULL || argument->kind != NL_SYNTAX_EXPR_NAME)
            goto profile;
        size_t member = 2, function = 0;
        for (size_t j = 0; j < 2; ++j)
            if (same(b, argument->data.name, names[j]))
                member = j;
        for (size_t j = 0; j < c->function_count; ++j)
            if (spelling(b, call->data.call.callee, c->functions[j].name))
                function = j + 1;
        if (member == 2 || function == 0)
            goto profile;
        if (!available[member])
            return error(diag, argument->span, NL_CHECK_SEMANTIC_ERROR,
                         "P278-DEFINITION-CONSUMED",
                         "member responsibility already consumed");
        const NLFunctionEntry *callee = &c->functions[function - 1];
        if (!callee->experimental_root_receiver || callee->body == NULL ||
            callee->parameters[0] != record->field_types[member])
            goto profile;
        const NLTypedOwnerDefinition demand = callee->body->owner_definition;
        if (!demand.definition_checked || demand.live_return ||
            demand.head_link_required ||
            demand.requirements != NL_OWNER_ALL_REQUIREMENTS ||
            demand.step_count != 4)
            goto profile;
        for (size_t j = 0; j < 4; ++j)
            if (demand.steps[j] != (NLTypedOwnerStep)(j + 1))
                goto profile;
        result.calls[result.count].member = member;
        result.calls[result.count].function = function;
        result.calls[result.count].span = call->span;
        result.calls[result.count++].definition = demand;
        available[member] = false;
    }
    const NLSyntaxView *tail = nl_syntax_node_view(block->data.block.tail);
    if (!tail || tail->kind != NL_SYNTAX_EXPR_NAME ||
        !spelling(b, tail->data.name, "unit"))
        goto profile;
    if (available[0] || available[1])
        return error(diag, block->span, NL_CHECK_SEMANTIC_ERROR,
                     "P278-DEFINITION-OBLIGATION",
                     "normal exit loses a non-Discardable member");
    result.definition_checked = true;
    *out = result;
    return NL_CHECK_OK;
profile:
    return error(diag, block->span, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                 "P278-MEMBER-PATH-PRECISION",
                 "outside two direct independent member terminal calls");
}
bool nl_semantic_two_root_definition(const NLSemanticContext *c, size_t id,
                                     NLTwoRootDefinition *out)
{
    if (!c || !out || !id || id > c->function_count ||
        !c->functions[id - 1].body ||
        !c->functions[id - 1].body->two_root_definition.definition_checked)
        return false;
    *out = c->functions[id - 1].body->two_root_definition;
    return true;
}
