#include "newlang/array4.h"
#include "newlang/parser.h"
#include "newlang/source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse(NLSource *source, int function_unit, NLSyntaxTree **tree)
{
    NLParser *parser = NULL;
    NLParseDiagnostic diagnostic = {0};
    if (nl_parser_create(source, &parser) != NL_PARSE_OK)
        return 0;
    NLParseStatus status =
        function_unit
            ? nl_parser_parse_function_unit(parser, tree, &diagnostic)
            : nl_parser_parse_expression_fragment(parser, tree, &diagnostic);
    nl_parser_destroy(parser);
    return status == NL_PARSE_OK && *tree != NULL;
}

static const NLCheckedFragment *registered_body(const NLCheckedFragment *call)
{
    size_t total = nl_checked_node_count(call);
    for (size_t i = 1; i <= total; ++i) {
        const NLCheckedNodeView *node = nl_checked_node_view(call, i);
        if (node != NULL && node->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(call, i);
            if (body != NULL)
                return body;
        }
    }
    return NULL;
}

static int validate(const NLCheckedFragment *body, size_t expected)
{
    if (nl_checked_array4_count(body) != expected)
        return 0;
    NLArray4Construction records[2] = {0};
    for (size_t i = 0; i < expected; ++i) {
        if (!nl_checked_array4_view_at(body, i, &records[i]) ||
            nl_checked_array4_world_at(body, i) == NULL ||
            !nl_checked_array4_valid_at(body, i, &records[i]))
            return 0;
        const NLArray4Construction *record = &records[i];
        if (record->root == 0 || record->incarnation == 0 ||
            record->array_type == 0 || record->element_type == 0 ||
            record->array_type == record->element_type)
            return 0;
        for (size_t j = 0; j < 4; ++j) {
            const NLArray4Element *element = &record->elements[j];
            if (element->index != j || element->initializer == 0 ||
                element->place == 0 || element->incarnation == 0 ||
                element->fact == 0 || element->value == 0)
                return 0;
            for (size_t k = 0; k < j; ++k) {
                if (element->place == record->elements[k].place)
                    return 0;
            }
        }
        const NLCheckedNodeView *binding =
            nl_checked_node_view(body, record->binding);
        NLSourceView name = {0};
        NLSemanticTypeView array = {0};
        NLSemanticTypeView element_type = {0};
        const NLSemanticContext *world = nl_checked_array4_world_at(body, i);
        if (binding == NULL || binding->kind != NL_CHECKED_BINDING ||
            binding->symbol != record->symbol || record->symbol == 0 ||
            !nl_source_view(nl_checked_source(body), binding->name, &name) ||
            name.length == 0 || name.length > 120 ||
            !nl_semantic_type_view(world, record->array_type, &array) ||
            !nl_semantic_type_view(world, record->element_type, &element_type) ||
            array.kind != NL_TYPE_ARRAY || array.element_count != 4 ||
            array.target != record->element_type ||
            element_type.kind != NL_TYPE_SUM)
            return 0;
        printf("R869_PUBLIC index=%zu name=%.*s symbol=%zu array=%zu "
               "element=%zu binding=%zu constructor=%zu root=%zu "
               "incarnation=%zu\n",
               i, (int)name.length, (const char *)name.bytes, record->symbol,
               record->array_type, record->element_type, record->binding,
               record->constructor, record->root, record->incarnation);
        NLArray4Construction forged = *record;
        forged.elements[0].index = 4;
        if (nl_checked_array4_valid_at(body, i, &forged))
            return 0;
        forged = *record;
        forged.elements[1].place = forged.elements[0].place;
        if (nl_checked_array4_valid_at(body, i, &forged))
            return 0;
        forged = *record;
        forged.elements[0].incarnation = 0;
        if (nl_checked_array4_valid_at(body, i, &forged))
            return 0;
        forged = *record;
        forged.array_type = forged.element_type;
        if (nl_checked_array4_valid_at(body, i, &forged))
            return 0;
        forged = *record;
        forged.root = 0;
        if (nl_checked_array4_valid_at(body, i, &forged))
            return 0;
    }
    if (expected == 2) {
        if (records[0].binding == records[1].binding ||
            records[0].constructor == records[1].constructor)
            return 0;
        NLArray4Construction forged = records[1];
        forged.constructor = records[0].constructor;
        if (nl_checked_array4_valid_at(body, 1, &forged))
            return 0;
        forged = records[1];
        forged.elements[2] = records[0].elements[2];
        if (nl_checked_array4_valid_at(body, 1, &forged))
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    char *end = NULL;
    unsigned long n = strtoul(argv[2], &end, 10);
    if (*end != '\0' || n < 1 || n > 2)
        return 2;

    NLSource *source = NULL;
    NLSource *call_source = NULL;
    NLSyntaxTree *unit = NULL;
    NLSyntaxTree *call_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *call = NULL;
    int ok = 0;
    static const char expression[] = "main()";
    NLFunctionUnitDiagnostic unit_diag = {0};
    NLCheckDiagnostic check_diag = {0};

    if (nl_source_load(argv[1], &source) != NL_SOURCE_OK ||
        !parse(source, 1, &unit))
        goto cleanup;
    if (nl_source_create(expression, sizeof(expression) - 1, "r869-call",
                         &call_source) != NL_SOURCE_OK ||
        !parse(call_source, 0, &call_tree))
        goto cleanup;
    if (nl_semantic_create(&context) != NL_CHECK_OK)
        goto cleanup;
    const NLSyntaxTree *inputs[] = {unit};
    if (nl_semantic_register_function_unit(context, inputs, 1, &unit_diag) !=
        NL_CHECK_OK)
        goto cleanup;
    if (nl_semantic_check_expression(context, call_tree, &call, &check_diag) !=
        NL_CHECK_OK)
        goto cleanup;
    const NLCheckedFragment *body = registered_body(call);
    if (body == NULL || !validate(body, (size_t)n))
        goto cleanup;
    ok = 1;

cleanup:
    nl_checked_destroy(call);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(call_tree);
    nl_syntax_tree_destroy(unit);
    nl_source_destroy(call_source);
    nl_source_destroy(source);
    if (!ok)
        fprintf(stderr, "R869 PUBLIC CHECK/VALIDATOR FAILED: %s (%lu roots)\n",
                argv[1], n);
    return ok ? 0 : 1;
}
