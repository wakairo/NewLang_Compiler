#include "newlang/parser.h"
#include "semantic_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

NLCheckStatus nl_body_retain(NLFunctionBody *body)
{
    if (body->owners == SIZE_MAX)
        return NL_CHECK_RESOURCE_LIMIT;
    ++body->owners;
    return NL_CHECK_OK;
}
void nl_body_release(NLFunctionBody *body)
{
    if (body == NULL || --body->owners != 0)
        return;
    for (size_t i = 0; i < body->count; ++i)
        free(body->parameter_names[i]);
    nl_syntax_tree_destroy(body->syntax);
    nl_source_destroy(body->source);
    free(body);
}
NLCheckStatus nl_body_create_span(const NLSource *source, NLSourceSpan span,
                                  const NLFunctionParameter *parameters,
                                  size_t count, NLFunctionBody **out)
{
    NLFunctionBody *body = malloc(sizeof(*body));
    if (body == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    *body = (NLFunctionBody){.owners = 1, .count = count};
    NLCheckStatus status = NL_CHECK_OK;
    for (size_t i = 0; i < count; ++i) {
        const size_t length = strlen(parameters[i].name);
        body->parameter_names[i] = malloc(length + 1);
        if (body->parameter_names[i] == NULL) {
            status = NL_CHECK_OUT_OF_MEMORY;
            goto failure;
        }
        memcpy(body->parameter_names[i], parameters[i].name, length + 1);
    }
    NLSourceView bytes;
    if (!nl_source_view(source, span, &bytes)) {
        status = NL_CHECK_INTERNAL_ERROR;
        goto failure;
    }
    NLSourceStatus source_status = nl_source_create(
        bytes.bytes, bytes.length, nl_source_name(source), &body->source);
    if (source_status != NL_SOURCE_OK) {
        status = source_status == NL_SOURCE_OUT_OF_MEMORY
                     ? NL_CHECK_OUT_OF_MEMORY
                     : NL_CHECK_RESOURCE_LIMIT;
        goto failure;
    }
    NLParser *parser = NULL;
    NLParseStatus parse_status = nl_parser_create(body->source, &parser);
    if (parse_status == NL_PARSE_OK)
        parse_status =
            nl_parser_parse_source_fragment(parser, &body->syntax, NULL);
    nl_parser_destroy(parser);
    if (parse_status != NL_PARSE_OK) {
        status = parse_status == NL_PARSE_OUT_OF_MEMORY ? NL_CHECK_OUT_OF_MEMORY
                 : parse_status == NL_PARSE_RESOURCE_LIMIT
                     ? NL_CHECK_RESOURCE_LIMIT
                     : NL_CHECK_SEMANTIC_UNSUPPORTED;
        goto failure;
    }
    if (nl_syntax_node_view(nl_syntax_tree_root(body->syntax))->kind !=
        NL_SYNTAX_BLOCK) {
        status = NL_CHECK_SEMANTIC_UNSUPPORTED;
        goto failure;
    }
    *out = body;
    return NL_CHECK_OK;
failure:
    nl_body_release(body);
    return status;
}

NLCheckStatus nl_body_create(const NLSyntaxTree *syntax,
                             const NLFunctionParameter *parameters,
                             size_t count, NLFunctionBody **out)
{
    const NLSource *source = nl_syntax_tree_source(syntax);
    return nl_body_create_span(source,
                               (NLSourceSpan){0, nl_source_length(source)},
                               parameters, count, out);
}

NLCheckStatus nl_sem_function_exit(const NLSemanticContext *c,
                                   size_t scope_floor, size_t place_floor)
{
    for (size_t i = 0; i < c->value_count; ++i) {
        const NLSemanticValueView v = c->values[i];
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        if (v.dependencies != NL_DEPENDENCY_FREE &&
            v.dependencies != NL_EXACT_VALUE_DEPENDENCIES)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        if (v.dependencies == NL_EXACT_VALUE_DEPENDENCIES) {
            if (v.value_dependency_count == 0 ||
                v.value_dependency_count > NL_SEMANTIC_MAX_VALUE_DEPENDENCIES)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            for (size_t d = 0; d < v.value_dependency_count; ++d)
                if (v.value_dependencies[d].place > place_floor ||
                    !nl_fixed_live(c, v.value_dependencies[d].place) ||
                    c->places[v.value_dependencies[d].place - 1].current_fact !=
                        v.value_dependencies[d].fact)
                    return NL_CHECK_SEMANTIC_ERROR;
        }
        if (c->types[v.type - 1].view.kind != NL_TYPE_REF)
            continue;
        for (size_t j = 0; j < nl_sem_ref_count(v); ++j) {
            const NLReferenceFacts fact = nl_sem_ref_fact(v, j);
            if (fact.scope == 0 || fact.place == 0)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            if (fact.place > place_floor)
                return NL_CHECK_SEMANTIC_ERROR;
            NLScopeId scope = fact.scope;
            size_t remaining = c->scope_count;
            while (scope != 0 && remaining-- != 0) {
                if (scope > scope_floor)
                    return NL_CHECK_SEMANTIC_ERROR;
                scope = c->scopes[scope - 1].parent;
            }
            if (scope != 0)
                return NL_CHECK_INTERNAL_ERROR;
        }
    }
    return NL_CHECK_OK;
}
