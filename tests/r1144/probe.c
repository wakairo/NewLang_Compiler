#include "linear_break_internal.h"
#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"

#include <stdio.h>
#include <string.h>

static const char *check_name(NLCheckStatus status)
{
    switch (status) {
    case NL_CHECK_OK:
        return "OK";
    case NL_CHECK_SEMANTIC_ERROR:
        return "SEMANTIC_ERROR";
    case NL_CHECK_SEMANTIC_UNSUPPORTED:
        return "SEMANTIC_UNSUPPORTED";
    case NL_CHECK_ANALYSIS_PRECISION_LIMIT:
        return "ANALYSIS_PRECISION_LIMIT";
    case NL_CHECK_OUT_OF_MEMORY:
        return "OUT_OF_MEMORY";
    case NL_CHECK_RESOURCE_LIMIT:
        return "RESOURCE_LIMIT";
    default:
        return "INTERNAL_ERROR";
    }
}

static void quoted(const char *text)
{
    putchar('"');
    if (text != NULL) {
        for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
            if (*p == '"' || *p == '\\')
                putchar('\\');
            if (*p >= 32 && *p < 127)
                putchar(*p);
            else
                printf("\\u%04x", (unsigned)*p);
        }
    }
    putchar('"');
}

static void rejected(const char *stage, const char *status, const char *code)
{
    printf("{\"accepted\":false,\"stage\":");
    quoted(stage);
    printf(",\"status\":");
    quoted(status);
    printf(",\"code\":");
    quoted(code);
    puts("}");
}

static bool named(const NLCheckedFragment *fragment,
                  const NLCheckedNodeView *view, const char *name)
{
    NLSourceView bytes;
    return nl_source_view(nl_checked_source(fragment), view->name, &bytes) &&
           bytes.length == strlen(name) &&
           memcmp(bytes.bytes, name, bytes.length) == 0;
}

static const NLCheckedNodeView *binding(const NLCheckedFragment *fragment,
                                        const char *name)
{
    NLSymbolId symbol = 0;
    for (size_t id = 1; id <= nl_checked_node_count(fragment); ++id) {
        const NLCheckedNodeView *view = nl_checked_node_view(fragment, id);
        if (view != NULL &&
            (view->kind == NL_CHECKED_RECEIVER ||
             view->kind == NL_CHECKED_BINDING) &&
            named(fragment, view, name))
            symbol = view->symbol;
    }
    if (symbol == 0)
        return NULL;
    for (size_t id = 1; id <= nl_checked_node_count(fragment); ++id) {
        const NLCheckedNodeView *view = nl_checked_node_view(fragment, id);
        if (view != NULL && view->kind == NL_CHECKED_BINDING &&
            view->symbol == symbol)
            return view;
    }
    return NULL;
}

static bool typed_binding(const NLCheckedFragment *body,
                          const NLCheckedNodeView *bind, NLTypeId expected)
{
    if (bind == NULL || bind->kind != NL_CHECKED_BINDING ||
        bind->type != nl_semantic_unit_type(nl_checked_context(body)) ||
        bind->argument_count != 1 || bind->symbol == 0)
        return false;
    const NLCheckedNodeView *receiver =
        nl_checked_node_view(body, bind->first_argument);
    const NLCheckedNodeView *initializer =
        nl_checked_node_view(body, bind->initializer);
    return receiver != NULL && receiver->kind == NL_CHECKED_RECEIVER &&
           receiver->symbol == bind->symbol && receiver->type == expected &&
           initializer != NULL && initializer->type == expected &&
           initializer->result_count == 1 &&
           initializer->results[0].type == expected && !initializer->terminates;
}

static bool inspect(const NLCheckedFragment *entry)
{
    const NLCheckedFragment *body =
        nl_checked_call_body(entry, nl_checked_root(entry));
    if (body == NULL)
        return false;
    const NLCheckedNodeView *after = binding(body, "after");
    const NLCheckedNodeView *copy = binding(body, "mirrored");
    const NLCheckedNodeView *selected = binding(body, "selected");
    if (after == NULL || copy == NULL || selected == NULL)
        return false;
    const NLCheckedNodeView *loop =
        nl_checked_node_view(body, after->initializer);
    const NLCheckedNodeView *copy_value =
        nl_checked_node_view(body, copy->initializer);
    const NLCheckedNodeView *call =
        nl_checked_node_view(body, selected->initializer);
    const NLTypeId quantity =
        nl_semantic_core_type(nl_checked_context(body), NL_TYPE_USIZE);
    const NLTypeId octet =
        nl_semantic_core_type(nl_checked_context(body), NL_TYPE_U8);
    if (loop == NULL || loop->kind != NL_CHECKED_LOOP ||
        !loop->header_inductive || loop->result_count != 1 ||
        loop->results[0].type != quantity || loop->continue_edges != 1 ||
        loop->break_edges != 1 || loop->return_edges != 0 ||
        nl_checked_loop_body(body, after->initializer) == NULL ||
        !typed_binding(body, after, quantity) ||
        !typed_binding(body, copy, quantity) || copy_value == NULL ||
        copy_value->kind != NL_CHECKED_IDENTIFIER ||
        copy_value->symbol != after->symbol ||
        copy_value->value_use != NL_VALUE_COPIED || call == NULL ||
        call->kind != NL_CHECKED_REGISTERED_CALL || call->argument_count != 1 ||
        call->result_count != 1 || call->results[0].type != octet ||
        !typed_binding(body, selected, octet))
        return false;
    const NLCheckedNodeView *actual =
        nl_checked_node_view(body, call->first_argument);
    if (actual == NULL || actual->kind != NL_CHECKED_IDENTIFIER ||
        actual->symbol != copy->symbol ||
        actual->value_use != NL_VALUE_COPIED || actual->type != quantity)
        return false;
    const NLCheckedFragment *callee =
        nl_checked_call_body(body, selected->initializer);
    if (callee == NULL)
        return false;
    NLSymbolId parameter = 0;
    for (size_t id = 1; id <= nl_checked_node_count(callee); ++id) {
        const NLCheckedNodeView *view = nl_checked_node_view(callee, id);
        if (view != NULL && view->kind == NL_CHECKED_IDENTIFIER &&
            named(callee, view, "amount") && view->type == quantity)
            parameter = view->symbol;
    }
    NLLinearBreakView proof;
    if (parameter == 0 || nl_linear_break_owner_validate(body) != NL_CHECK_OK ||
        nl_linear_break_validate(body, after->initializer, &proof) !=
            NL_CHECK_OK ||
        !proof.scalar_loop || !proof.computed_carry ||
        proof.result.value != loop->results[0].value)
        return false;
    printf("{\"accepted\":true,\"stage\":\"owned\","
           "\"status\":\"OK\",\"source_issued\":true,"
           "\"header_inductive\":true,\"continue_edges\":%zu,"
           "\"break_edges\":%zu,\"return_edges\":%zu,"
           "\"ordinary_copy\":true,\"actual_copy\":true,"
           "\"binding_unit_and_typed_receivers\":true,"
           "\"private_relation_valid\":true,"
           "\"direct_break_provenance\":%s,"
           "\"entry\":%zu,\"loop\":%zu,\"after\":%zu,\"mirrored\":%zu,"
           "\"selected\":%zu,\"call\":%zu,\"callee\":%zu,"
           "\"parameter\":%zu,\"initial\":%zu,\"step\":%zu,"
           "\"limit\":%zu,\"subtract\":%s}\n",
           loop->continue_edges, loop->break_edges, loop->return_edges,
           nl_checked_loop_break_provenance_valid(body, after->initializer)
               ? "true"
               : "false",
           nl_checked_node_view(entry, nl_checked_root(entry))->function,
           after->initializer, after->symbol, copy->symbol, selected->symbol,
           selected->initializer, call->function, parameter,
           proof.initial_phase, proof.next_phase, proof.limit,
           proof.subtract_carry ? "true" : "false");
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3 ||
        (strcmp(argv[1], "owned") != 0 && strcmp(argv[1], "check") != 0))
        return 3;
    NLSource *source = NULL;
    NLSource *entry_source = NULL;
    NLParser *parser = NULL;
    NLParser *entry_parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSyntaxTree *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *entry = NULL;
    NLParseDiagnostic parse = {0};
    NLFunctionUnitDiagnostic registration = {0};
    NLCheckDiagnostic diagnostic = {0};
    int result = 3;
    if (nl_source_load(argv[2], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK) {
        rejected("source", "HOST_ERROR", "");
        goto done;
    }
    const NLParseStatus parsed =
        nl_parser_parse_function_unit(parser, &unit, &parse);
    if (parsed != NL_PARSE_OK) {
        rejected("parse",
                 parsed >= NL_PARSE_OUT_OF_MEMORY ? "HOST_ERROR"
                                                  : "PARSE_REJECT",
                 parse.diagnostic.code);
        nl_parse_diagnostic_render(stderr, source, &parse);
        result = parsed >= NL_PARSE_OUT_OF_MEMORY ? 3 : 2;
        goto done;
    }
    if (nl_semantic_create(&context) != NL_CHECK_OK) {
        rejected("context", "HOST_ERROR", "");
        goto done;
    }
    const NLSyntaxTree *inputs[] = {unit};
    const NLCheckStatus registered =
        nl_semantic_register_function_unit(context, inputs, 1, &registration);
    if (registered != NL_CHECK_OK) {
        rejected("registration", check_name(registered),
                 registration.diagnostic.diagnostic.code);
        nl_check_diagnostic_render(stderr, source, &registration.diagnostic);
        result = 2;
        goto done;
    }
    if (nl_source_create("main()", 6, "<r1144-entry>", &entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(entry_parser, &entry_tree,
                                            &parse) != NL_PARSE_OK) {
        rejected("entry", "HOST_ERROR", "");
        goto done;
    }
    const NLCheckStatus checked =
        nl_semantic_check_expression(context, entry_tree, &entry, &diagnostic);
    if (checked != NL_CHECK_OK) {
        rejected("CHECK", check_name(checked), diagnostic.diagnostic.code);
        nl_check_diagnostic_render(stderr, entry_source, &diagnostic);
        result = 2;
        goto done;
    }
    if (strcmp(argv[1], "check") == 0) {
        puts("{\"accepted\":true,\"stage\":\"CHECK\",\"status\":\"OK\"}");
        result = 0;
    } else if (inspect(entry)) {
        result = 0;
    } else {
        rejected("owned-observation", "SHAPE_UNOBSERVED", "");
    }
done:
    nl_checked_destroy(entry);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(entry_tree);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(entry_parser);
    nl_parser_destroy(parser);
    nl_source_destroy(entry_source);
    nl_source_destroy(source);
    return result;
}
