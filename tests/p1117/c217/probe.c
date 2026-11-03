#define main p1117_c217_driver_main
#include "main.c"
#undef main

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    NLSource *source;
    NLSource *entry_source;
    NLParser *parser;
    NLParser *entry_parser;
    NLSyntaxTree *unit;
    NLSyntaxTree *entry_tree;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
    NLCheckedFragment *owner;
    NLCheckedFragment *bodies[2];
    NLCheckedNodeId calls[2];
} Input;

typedef struct {
    bool private_consumer;
    bool call_validator;
    bool outer_validator;
} Admission;

static V0Program program(const NLSemanticContext *context)
{
    return (V0Program){
        .unit = nl_semantic_unit_type(context),
        .u8 = nl_semantic_core_type(context, NL_TYPE_U8),
    };
}

static Admission admission(const Input *input)
{
    V0Program private_program = program(input->context);
    V0Program call_program = program(input->context);
    V0Program outer_program = program(input->context);
    return (Admission){
        .private_consumer =
            predicate_call(input->owner, input->calls[0], &private_program),
        .call_validator =
            v0_validate_call(input->owner, input->calls[0], &call_program),
        .outer_validator = v0_validate_call(
            input->entry, nl_checked_root(input->entry), &outer_program),
    };
}

static void expect(const char *label, Admission actual, bool private_expected,
                   bool call_expected, bool outer_expected)
{
    printf("P1117-C217 %s private=%d call=%d outer=%d\n", label,
           actual.private_consumer, actual.call_validator,
           actual.outer_validator);
    assert(actual.private_consumer == private_expected);
    assert(actual.call_validator == call_expected);
    assert(actual.outer_validator == outer_expected);
}

static void dispose(Input *input)
{
    nl_checked_destroy(input->entry);
    nl_semantic_destroy(input->context);
    nl_syntax_tree_destroy(input->entry_tree);
    nl_parser_destroy(input->entry_parser);
    nl_source_destroy(input->entry_source);
    nl_syntax_tree_destroy(input->unit);
    nl_parser_destroy(input->parser);
    nl_source_destroy(input->source);
    *input = (Input){0};
}

static void prepare(const char *path, Input *input)
{
    assert(nl_source_load(path, &input->source) == NL_SOURCE_OK);
    assert(nl_parser_create(input->source, &input->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(input->parser, &input->unit, NULL) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&input->context) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {input->unit};
    assert(nl_semantic_register_function_unit(input->context, units, 1, NULL) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "<P1117-C217-entry>",
                            &input->entry_source) == NL_SOURCE_OK);
    assert(nl_parser_create(input->entry_source, &input->entry_parser) ==
           NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(
               input->entry_parser, &input->entry_tree, NULL) == NL_PARSE_OK);
    assert(nl_semantic_check_expression(input->context, input->entry_tree,
                                        &input->entry, NULL) == NL_CHECK_OK);
    input->owner = (NLCheckedFragment *)nl_checked_call_body(
        input->entry, nl_checked_root(input->entry));
    assert(input->owner != NULL);
    size_t count = 0;
    for (NLCheckedNodeId id = 1;
         id <= nl_checked_node_count(input->owner) && count < 2; ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(input->owner, id);
        if (node != NULL && node->kind == NL_CHECKED_REGISTERED_CALL)
            input->calls[count++] = id;
    }
    assert(count == 2);
    for (size_t i = 0; i < 2; ++i) {
        input->bodies[i] = (NLCheckedFragment *)nl_checked_call_body(
            input->owner, input->calls[i]);
        assert(input->bodies[i] != NULL);
        assert(nl_checked_context(input->bodies[i]) == input->context);
    }
    assert(nl_checked_context(input->owner) == input->context);
    expect("genuine", admission(input), true, true, true);
}

static size_t fresh_copy_count(const Input *input,
                               const NLCheckedFragment *body)
{
    size_t fresh = 0;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, id);
        if (node == NULL || node->kind != NL_CHECKED_IDENTIFIER ||
            node->value_use != NL_VALUE_COPIED || node->result_count != 1 ||
            !node->symbol || node->symbol > input->context->binding_count)
            continue;
        NLValueId origin =
            input->context->bindings[node->symbol - 1].view.value;
        for (NLCheckedNodeId declaration_id = 1;
             declaration_id <= nl_checked_node_count(body); ++declaration_id) {
            const NLCheckedNodeView *declaration =
                nl_checked_node_view(body, declaration_id);
            if (declaration == NULL ||
                declaration->kind != NL_CHECKED_BINDING ||
                declaration->symbol != node->symbol ||
                declaration->span.end_byte > node->span.start_byte)
                continue;
            const NLCheckedNodeView *initializer =
                nl_checked_node_view(body, declaration->initializer);
            assert(initializer != NULL && initializer->result_count == 1);
            origin = initializer->results[0].value;
        }
        assert(origin != 0);
        if (origin != node->results[0].value)
            ++fresh;
    }
    return fresh;
}

static NLCheckedNodeView *return_node(NLCheckedFragment *body)
{
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(body); ++id) {
        NLCheckedNodeView *node = &body->nodes[id - 1];
        if (node->kind == NL_CHECKED_RETURN)
            return node;
    }
    return NULL;
}

static void apply_foreign(NLCheckedNodeView *call, NLCheckedNodeView *root,
                          NLCheckedNodeView *ret, NLCheckedNodeView *operand,
                          NLCheckedResult foreign, bool include_operand)
{
    call->results[0] = foreign;
    if (ret != NULL)
        ret->returned = foreign;
    if (root->terminates)
        root->returned = foreign;
    else
        root->results[0] = foreign;
    if (include_operand)
        operand->results[0] = foreign;
}

int main(int argc, char **argv)
{
    assert(setvbuf(stdout, NULL, _IONBF, 0) == 0);
    assert(argc == 4);
    const bool original = strcmp(argv[1], "original") == 0;
    assert(original || strcmp(argv[1], "repair") == 0);
    const bool expect_return = strstr(argv[3], "return-") == argv[3];
    const bool expect_alias = strstr(argv[3], "-alias") != NULL;

    Input input = {0};
    prepare(argv[2], &input);
    NLCheckedNodeView *call = &input.owner->nodes[input.calls[0] - 1];
    const NLCheckedNodeView *other =
        nl_checked_node_view(input.owner, input.calls[1]);
    assert(call->result_count == 1 && other != NULL &&
           other->result_count == 1 &&
           call->results[0].type == other->results[0].type &&
           call->results[0].value != other->results[0].value);

    NLCheckedFragment *body = input.bodies[0];
    const NLCheckedFragment *other_body = input.bodies[1];
    NLCheckedNodeView *root = &body->nodes[nl_checked_root(body) - 1];
    const NLCheckedNodeView *other_root =
        nl_checked_node_view(other_body, nl_checked_root(other_body));
    NLCheckedNodeView *ret = return_node(body);
    assert((ret != NULL) == expect_return);
    NLCheckedNodeId operand_id = ret != NULL ? ret->initializer : root->tail;
    assert(operand_id != 0);
    NLCheckedNodeView *operand = &body->nodes[operand_id - 1];
    assert(operand->result_count == 1);
    assert((operand->kind == NL_CHECKED_IDENTIFIER) == expect_alias);
    assert(operand->kind == NL_CHECKED_IDENTIFIER ||
           operand->kind == NL_CHECKED_IF);

    const NLCheckedResult foreign = other->results[0];
    const NLCheckedResult other_body_result =
        other_root->terminates ? other_root->returned : other_root->results[0];
    assert(foreign.type == other_body_result.type &&
           foreign.value == other_body_result.value);
    NLSemanticValueView local_value, foreign_value;
    assert(nl_semantic_value_view(input.context, operand->results[0].value,
                                  &local_value));
    assert(
        nl_semantic_value_view(input.context, foreign.value, &foreign_value));
    assert(local_value.type == foreign_value.type &&
           local_value.dependencies == NL_DEPENDENCY_FREE &&
           foreign_value.dependencies == NL_DEPENDENCY_FREE &&
           !local_value.scalar_known && !local_value.scalar_value &&
           !foreign_value.scalar_known && !foreign_value.scalar_value);

    const size_t fresh = fresh_copy_count(&input, body);
    assert(fresh >= (expect_alias ? 2u : 1u));
    printf("P1117-C217 shape=%s local=%zu foreign=%zu fresh_copies=%zu\n",
           argv[3], operand->results[0].value, foreign.value, fresh);

    const NLCheckedNodeView saved_call = *call;
    const NLCheckedNodeView saved_root = *root;
    const NLCheckedNodeView saved_operand = *operand;
    const NLCheckedNodeView saved_return =
        ret != NULL ? *ret : (NLCheckedNodeView){0};

    call->results[0] = foreign;
    expect("call-only", admission(&input), false, false, false);
    *call = saved_call;

    apply_foreign(call, root, ret, operand, foreign, false);
    expect("endpoints-only", admission(&input), false, false, false);
    *call = saved_call;
    *root = saved_root;
    if (ret != NULL)
        *ret = saved_return;

    apply_foreign(call, root, ret, operand, foreign, true);
    Admission coherent = admission(&input);
    printf("P1117-C217 coherent private=%d call=%d outer=%d phase=%s\n",
           coherent.private_consumer, coherent.call_validator,
           coherent.outer_validator, argv[1]);
    assert(coherent.private_consumer == original);
    assert(coherent.call_validator == original);
    assert(coherent.outer_validator == original);

    *operand = saved_operand;
    if (ret != NULL)
        *ret = saved_return;
    *root = saved_root;
    *call = saved_call;
    expect("restored", admission(&input), true, true, true);
    puts("P1117-C217 altered graphs were not emitted or executed");
    dispose(&input);
    return 0;
}
