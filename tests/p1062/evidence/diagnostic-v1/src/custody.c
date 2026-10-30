#include "semantic_internal.h"

#include <string.h>

/* This is a finite conditional definition checker, not a concrete semantic
 * world. The two formal origins are intentionally uncorrelated. In particular
 * recognizing this body cannot establish Current(sink)==None at a call. */
bool nl_custody_signature(const NLSemanticContext *c, const NLTypeId *types,
                          size_t count, NLTypeId result)
{
    if (count != 2 || result != nl_semantic_unit_type(c))
        return false;
    const NLSemanticTypeView sink = c->types[types[0] - 1].view;
    return sink.kind == NL_TYPE_REF && sink.target != 0 &&
           c->types[types[1] - 1].live_tail_target != 0 &&
           c->types[sink.target - 1].option_target == types[1];
}

static bool text(const NLFunctionBody *b, NLSourceSpan span, const char *name)
{
    NLSourceView v;
    return nl_source_view(b->source, span, &v) && v.length == strlen(name) &&
           memcmp(v.bytes, name, v.length) == 0;
}

static bool same(const NLFunctionBody *b, NLSourceSpan a, NLSourceSpan z)
{
    NLSourceView x, y;
    return nl_source_view(b->source, a, &x) &&
           nl_source_view(b->source, z, &y) && x.length == y.length &&
           memcmp(x.bytes, y.bytes, x.length) == 0;
}

static bool name(const NLFunctionBody *b, const NLSyntaxNode *n,
                 const char *spelling)
{
    const NLSyntaxView *v = nl_syntax_node_view(n);
    return v != NULL && v->kind == NL_SYNTAX_EXPR_NAME &&
           text(b, v->data.name, spelling);
}

static bool unit_block(const NLFunctionBody *b, const NLSyntaxNode *n)
{
    const NLSyntaxView *v = nl_syntax_node_view(n);
    return v != NULL && v->kind == NL_SYNTAX_BLOCK &&
           v->data.block.items == NULL && name(b, v->data.block.tail, "unit");
}

static NLCheckStatus reject(NLCheckDiagnostic *out, NLSourceSpan span,
                            const char *code, const char *message)
{
    if (out != NULL)
        *out = (NLCheckDiagnostic){
            {NL_DIAG_ERROR, "semantic", code, message, NULL, NULL, 0}, span};
    return NL_CHECK_SEMANTIC_ERROR;
}

NLCheckStatus nl_custody_definition(const NLSemanticContext *c,
                                    NLFunctionBody *b, NLCustodyDefinition *out,
                                    NLCheckDiagnostic *diagnostic)
{
    const NLSyntaxView *root =
        nl_syntax_node_view(nl_syntax_tree_root(b->syntax));
    const NLSyntaxView *binding =
        root == NULL ? NULL : nl_syntax_node_view(root->data.block.items);
    if (root == NULL || root->kind != NL_SYNTAX_BLOCK || b->count != 2 ||
        binding == NULL || binding->kind != NL_SYNTAX_BINDING)
        return reject(diagnostic, root == NULL ? (NLSourceSpan){0} : root->span,
                      "CUSTODY-DEFINITION-PROFILE",
                      "recipient requires its bounded replace/consume body");
    const NLSyntaxView *replace =
        nl_syntax_node_view(binding->data.binding.initializer);
    if (replace == NULL || replace->kind != NL_SYNTAX_EXPR_CALL ||
        !text(b, replace->data.call.callee, "replace") ||
        replace->data.call.argument_count != 2 ||
        !name(b, replace->data.call.arguments, b->parameter_names[0]))
        return reject(diagnostic, binding->span, "CUSTODY-DEFINITION-REPLACE",
                      "recipient must replace the original sink, preserving "
                      "the non-Discardable old value");
    const NLSyntaxView *some = nl_syntax_node_view(
        nl_syntax_next_argument(replace->data.call.arguments));
    const NLSyntaxView *type =
        some == NULL || some->kind != NL_SYNTAX_SUM_CONSTRUCTOR
            ? NULL
            : nl_syntax_node_view(some->data.constructor.type);
    if (type == NULL || type->kind != NL_SYNTAX_OPTION_LIVE_TAIL ||
        !text(b, some->data.constructor.variant, "Some") ||
        !some->data.constructor.parentheses ||
        some->data.constructor.argument_count != 1 ||
        !name(b, some->data.constructor.arguments, b->parameter_names[1]))
        return reject(diagnostic, replace->span, "CUSTODY-DEFINITION-PACKET",
                      "Some must transfer the one complete original packet");
    NLSourceView binder;
    if (!nl_source_view(b->source, binding->data.binding.name, &binder) ||
        !nl_sem_lexical_name_admissible(binder.bytes, binder.length) ||
        text(b, binding->data.binding.name, b->parameter_names[0]) ||
        text(b, binding->data.binding.name, b->parameter_names[1]))
        return reject(diagnostic, binding->span, "CUSTODY-DEFINITION-NAME",
                      "old-value receiver must be a fresh ordinary binding");
    const NLSyntaxNode *second =
        nl_syntax_next_argument(root->data.block.items);
    const NLSyntaxView *statement = nl_syntax_node_view(second);
    const NLSyntaxView *match =
        statement == NULL || statement->kind != NL_SYNTAX_STATEMENT
            ? NULL
            : nl_syntax_node_view(statement->data.statement.expression);
    const NLSyntaxView *scrutinee =
        match == NULL || match->kind != NL_SYNTAX_MATCH
            ? NULL
            : nl_syntax_node_view(match->data.match.scrutinee);
    const NLSyntaxNode *arm = scrutinee == NULL ? NULL : match->data.match.arms;
    const NLSyntaxView *none = nl_syntax_node_view(arm);
    if (scrutinee == NULL || scrutinee->kind != NL_SYNTAX_EXPR_NAME ||
        !same(b, scrutinee->data.name, binding->data.binding.name) ||
        none == NULL || !text(b, none->data.arm.variant, "None") ||
        none->data.arm.payload || none->data.arm.wildcard ||
        nl_syntax_next_argument(arm) != NULL ||
        !unit_block(b, none->data.arm.body))
        return reject(diagnostic,
                      statement == NULL ? root->span : statement->span,
                      "CUSTODY-DEFINITION-OLD-NONE",
                      "exact old None must be consumed once by the first "
                      "bounded consuming match");
    if (nl_syntax_next_argument(second) != NULL ||
        !name(b, root->data.block.tail, "unit"))
        return reject(diagnostic, root->span, "CUSTODY-DEFINITION-EXIT",
                      "recipient must exit unit without any extra effects, "
                      "authority, borrow or unused packet");
    /* Locate the immutable signature, not a favorable caller's state. */
    for (size_t i = 0; i < c->function_count; ++i) {
        const NLFunctionEntry f = c->functions[i];
        if (f.body != b)
            continue;
        const NLSemanticTypeView sink = c->types[f.parameters[0] - 1].view;
        if (sink.is_exclusive || sink.access != NL_ACCESS_WRITE)
            return reject(diagnostic, root->span, "CUSTODY-DEFINITION-WRITE",
                          "recipient requires an ordinary write ref");
        *out = (NLCustodyDefinition){
            .definition_checked = true,
            .target = c->types[f.parameters[1] - 1].live_tail_target,
            .packet = f.parameters[1],
            .option = sink.target,
            .requirements = NL_CUSTODY_ALL_REQUIREMENTS};
        return NL_CHECK_OK;
    }
    return NL_CHECK_INTERNAL_ERROR;
}

bool nl_semantic_function_custody_applicability(const NLSemanticContext *c,
                                                size_t function,
                                                NLCustodyDefinition *out)
{
    if (c == NULL || out == NULL || function == 0 ||
        function > c->function_count)
        return false;
    const NLFunctionEntry f = c->functions[function - 1];
    if (!f.custody_recipient || f.body == NULL ||
        !f.body->custody_definition.definition_checked)
        return false;
    *out = f.body->custody_definition;
    return true;
}
