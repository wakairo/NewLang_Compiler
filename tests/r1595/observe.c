/* R1595 V2 apparatus draft. PREP ONLY: not compiled or executed.
 * Public getters only after authentic source registration and one synthetic
 * main() entry. The traversal is serialization, never a runtime trace.
 */
#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdio.h>
#include <stdlib.h>

enum {
    MAX_OBJECTS = 128,
    MAX_VISITS = 8192,
    MAX_DEPTH = 64
};

typedef struct {
    const NLCheckedFragment *fragment;
    NLCheckedNodeId node;
} Visit;

typedef struct {
    const NLCheckedFragment *fragments[MAX_OBJECTS];
    const NLSemanticContext *contexts[MAX_OBJECTS];
    const NLSource *sources[MAX_OBJECTS];
    Visit visited[MAX_VISITS];
    size_t fragment_count, context_count, source_count, visit_count;
    bool failed;
} Observer;

static const char *yes(bool value)
{
    return value ? "true" : "false";
}

static void problem(Observer *observer, const char *message)
{
    observer->failed = true;
    printf("{\"event\":\"apparatus_or_public_contract_failure\","
           "\"message\":\"%s\"}\n",
           message);
}

static void hex_bytes(const unsigned char *bytes, size_t length)
{
    putchar('"');
    for (size_t i = 0; i < length; ++i)
        printf("%02x", (unsigned)bytes[i]);
    putchar('"');
}

static void span_hex(const NLSource *source, NLSourceSpan span)
{
    NLSourceView view;
    if (!source || !nl_source_view(source, span, &view)) {
        fputs("null", stdout);
        return;
    }
    hex_bytes(view.bytes, view.length);
}

static const char *parse_name(NLParseStatus status)
{
    switch (status) {
    case NL_PARSE_OK:
        return "OK";
    case NL_PARSE_LEXICALLY_UNSUPPORTED:
        return "LEXICALLY_UNSUPPORTED";
    case NL_PARSE_SYNTAX_UNSUPPORTED:
        return "SYNTAX_UNSUPPORTED";
    case NL_PARSE_SYNTAX_ERROR:
        return "SYNTAX_ERROR";
    case NL_PARSE_OUT_OF_MEMORY:
        return "OUT_OF_MEMORY";
    case NL_PARSE_RESOURCE_LIMIT:
        return "RESOURCE_LIMIT";
    case NL_PARSE_INTERNAL_ERROR:
        return "INTERNAL_ERROR";
    }
    return "UNKNOWN_STATUS";
}

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
    case NL_CHECK_INTERNAL_ERROR:
        return "INTERNAL_ERROR";
    }
    return "UNKNOWN_STATUS";
}

static bool parse_host_failure(NLParseStatus status)
{
    return status == NL_PARSE_OUT_OF_MEMORY ||
           status == NL_PARSE_RESOURCE_LIMIT ||
           status == NL_PARSE_INTERNAL_ERROR;
}

static bool check_host_failure(NLCheckStatus status)
{
    return status == NL_CHECK_OUT_OF_MEMORY ||
           status == NL_CHECK_RESOURCE_LIMIT ||
           status == NL_CHECK_INTERNAL_ERROR;
}

static const char *kind_name(NLCheckedKind kind)
{
    switch (kind) {
    case NL_CHECKED_REGISTERED_CALL:
        return "CALL";
    case NL_CHECKED_BLOCK:
        return "BLOCK";
    case NL_CHECKED_BINDING:
        return "BINDING";
    case NL_CHECKED_IDENTIFIER:
        return "IDENTIFIER";
    case NL_CHECKED_STATEMENT:
        return "STATEMENT";
    case NL_CHECKED_IF:
        return "IF";
    case NL_CHECKED_IF_ARM:
        return "IF_ARM";
    case NL_CHECKED_UNIT:
        return "UNIT";
    case NL_CHECKED_USIZE_LITERAL:
        return "USIZE_LITERAL";
    case NL_CHECKED_USIZE_LESS_THAN:
        return "USIZE_LESS_THAN";
    case NL_CHECKED_USIZE_DIVISION:
        return "USIZE_DIVISION";
    case NL_CHECKED_USIZE_REMAINDER:
        return "USIZE_REMAINDER";
    default:
        return "OTHER";
    }
}

static size_t source_id(Observer *observer, const NLSource *source)
{
    if (!source)
        return 0;
    for (size_t i = 0; i < observer->source_count; ++i)
        if (observer->sources[i] == source)
            return i + 1;
    if (observer->source_count == MAX_OBJECTS) {
        problem(observer, "source observation budget");
        return 0;
    }
    size_t id = ++observer->source_count;
    observer->sources[id - 1] = source;
    NLSourceSpan all = {0, nl_source_length(source)};
    printf("{\"event\":\"source\",\"id\":%zu,\"length\":%zu,"
           "\"bytes_hex\":",
           id, all.end_byte);
    span_hex(source, all);
    fputs("}\n", stdout);
    return id;
}

static size_t context_id(Observer *observer, const NLSemanticContext *context)
{
    if (!context)
        return 0;
    for (size_t i = 0; i < observer->context_count; ++i)
        if (observer->contexts[i] == context)
            return i + 1;
    if (observer->context_count == MAX_OBJECTS) {
        problem(observer, "context observation budget");
        return 0;
    }
    size_t id = ++observer->context_count;
    observer->contexts[id - 1] = context;
    return id;
}

static void scalar_value(const NLSemanticContext *context,
                         NLCheckedResult result)
{
    NLSemanticValueView value = {0};
    NLSemanticTypeView type = {0};
    bool present = context != NULL && result.value != 0 &&
                   nl_semantic_value_view(context, result.value, &value);
    bool type_present = context != NULL && result.type != 0 &&
                        nl_semantic_type_view(context, result.type, &type);
    printf("{\"type\":%zu,\"value_id\":%zu,\"view_present\":%s,"
           "\"view_type\":%zu,\"known\":%s,\"number\":%zu,"
           "\"type_view_present\":%s,\"type_is_usize\":%s,"
           "\"type_copy\":%s,\"type_discardable\":%s,\"types_match\":%s}",
           result.type, result.value, yes(present), present ? value.type : 0,
           yes(present && value.scalar_known), present ? value.scalar_value : 0,
           yes(type_present), yes(type_present && type.kind == NL_TYPE_USIZE),
           yes(type_present && type.is_copy),
           yes(type_present && type.is_discardable),
           yes(present && value.type == result.type));
}

static void
arithmetic_relation(NLTypeId type, const NLCheckedNodeId operands[2],
                    const NLValueId values[2], NLValueId result,
                    const NLSymbolId symbols[2], const NLPlaceId places[2],
                    const NLIncarnationId incarnations[2],
                    const NLValueFactId facts[2], NLSourceSpan span,
                    NLSourceSpan operation, const NLSourceSpan operand_spans[2])
{
    printf(",\"type\":%zu,\"operands\":[%zu,%zu],"
           "\"values\":[%zu,%zu],\"result_id\":%zu,"
           "\"symbols\":[%zu,%zu],\"places\":[%zu,%zu],"
           "\"incarnations\":[%zu,%zu],\"facts\":[%zu,%zu],"
           "\"span\":[%zu,%zu],\"operator_span\":[%zu,%zu],"
           "\"operand_spans\":[[%zu,%zu],[%zu,%zu]]}\n",
           type, operands[0], operands[1], values[0], values[1], result,
           symbols[0], symbols[1], places[0], places[1], incarnations[0],
           incarnations[1], facts[0], facts[1], span.start_byte, span.end_byte,
           operation.start_byte, operation.end_byte,
           operand_spans[0].start_byte, operand_spans[0].end_byte,
           operand_spans[1].start_byte, operand_spans[1].end_byte);
}

static void arithmetic(Observer *observer, const NLCheckedFragment *fragment,
                       size_t fid, NLCheckedNodeId node, NLCheckedKind kind)
{
    if (kind == NL_CHECKED_USIZE_DIVISION) {
        NLUsizeDivisionView view = {0};
        bool present = nl_checked_usize_division_view(fragment, node, &view);
        bool valid = nl_checked_usize_division_valid(fragment, node, NULL);
        bool math_ok = !present || !view.numerator_known ||
                       !view.quotient_known ||
                       (view.numbers[1] != 0 &&
                        view.quotient == view.numbers[0] / view.numbers[1]);
        printf("{\"event\":\"arithmetic\",\"fragment\":%zu,\"node\":%zu,"
               "\"operation\":\"division\",\"view_present\":%s,"
               "\"valid\":%s,\"origin_same\":%s,\"numerator_known\":%s,"
               "\"result_known\":%s,\"numerator\":%zu,\"divisor\":%zu,"
               "\"result\":%zu,\"target_max\":%zu,\"math_ok\":%s",
               fid, node, yes(present), yes(valid),
               yes(present && view.origin == fragment),
               yes(view.numerator_known), yes(view.quotient_known),
               view.numbers[0], view.numbers[1], view.quotient, view.target_max,
               yes(math_ok));
        arithmetic_relation(view.type, view.operands, view.values, view.result,
                            view.symbols, view.places, view.incarnations,
                            view.facts, view.span, view.operator_span,
                            view.operand_spans);
        if (present && (!valid || view.origin != fragment || !math_ok))
            problem(observer, "division evidence contradiction");
    } else if (kind == NL_CHECKED_USIZE_REMAINDER) {
        NLUsizeRemainderView view = {0};
        bool present = nl_checked_usize_remainder_view(fragment, node, &view);
        bool valid = nl_checked_usize_remainder_valid(fragment, node, NULL);
        bool math_ok = !present || !view.numerator_known ||
                       !view.remainder_known ||
                       (view.numbers[1] != 0 &&
                        view.remainder == view.numbers[0] % view.numbers[1]);
        printf("{\"event\":\"arithmetic\",\"fragment\":%zu,\"node\":%zu,"
               "\"operation\":\"remainder\",\"view_present\":%s,"
               "\"valid\":%s,\"origin_same\":%s,\"source_same\":%s,"
               "\"numerator_known\":%s,\"result_known\":%s,"
               "\"numerator\":%zu,\"divisor\":%zu,\"result\":%zu,"
               "\"target_max\":%zu,\"math_ok\":%s",
               fid, node, yes(present), yes(valid),
               yes(present && view.origin == fragment),
               yes(present && view.source == nl_checked_source(fragment)),
               yes(view.numerator_known), yes(view.remainder_known),
               view.numbers[0], view.numbers[1], view.remainder,
               view.target_max, yes(math_ok));
        arithmetic_relation(view.type, view.operands, view.values, view.result,
                            view.symbols, view.places, view.incarnations,
                            view.facts, view.span, view.operator_span,
                            view.operand_spans);
        if (present && (!valid || view.origin != fragment ||
                        view.source != nl_checked_source(fragment) || !math_ok))
            problem(observer, "remainder evidence contradiction");
    }
}

static size_t visit_fragment(Observer *, const NLCheckedFragment *, size_t);

static void visit_node(Observer *observer, const NLCheckedFragment *fragment,
                       size_t fid, NLCheckedNodeId node, size_t depth)
{
    if (node == 0 || observer->failed)
        return;
    if (depth > MAX_DEPTH) {
        problem(observer, "node traversal depth");
        return;
    }
    for (size_t i = 0; i < observer->visit_count; ++i)
        if (observer->visited[i].fragment == fragment &&
            observer->visited[i].node == node)
            return;
    if (observer->visit_count == MAX_VISITS) {
        problem(observer, "node observation budget");
        return;
    }
    observer->visited[observer->visit_count++] = (Visit){fragment, node};
    const NLCheckedNodeView *view = nl_checked_node_view(fragment, node);
    if (!view) {
        problem(observer, "published edge has no node view");
        return;
    }
    const NLSource *source = nl_checked_source(fragment);
    const NLSemanticContext *context = nl_checked_context(fragment);
    printf("{\"event\":\"node\",\"fragment\":%zu,\"id\":%zu,"
           "\"kind\":\"%s\",\"kind_number\":%d,\"span\":[%zu,%zu],"
           "\"name_hex\":",
           fid, node, kind_name(view->kind), (int)view->kind,
           view->span.start_byte, view->span.end_byte);
    span_hex(source, view->name);
    printf(",\"symbol\":%zu,\"function\":%zu,\"body_backed\":%s,"
           "\"first_argument\":%zu,\"next_argument\":%zu,"
           "\"argument_count\":%zu,\"initializer\":%zu,"
           "\"first_item\":%zu,\"next_item\":%zu,\"tail\":%zu,"
           "\"item_count\":%zu,\"parameter_type\":%zu,"
           "\"value_use\":%d,\"result_count\":%zu,\"results\":[",
           view->symbol, view->function, yes(view->body_backed),
           view->first_argument, view->next_argument, view->argument_count,
           view->initializer, view->first_item, view->next_item, view->tail,
           view->item_count, view->parameter_type, (int)view->value_use,
           view->result_count);
    size_t count = view->result_count <= 2 ? view->result_count : 2;
    for (size_t i = 0; i < count; ++i) {
        if (i != 0)
            putchar(',');
        scalar_value(context, view->results[i]);
    }
    printf("],\"has_scalar_result\":%s,\"scalar_known\":%s,"
           "\"scalar_number\":%zu,\"terminates\":%s,"
           "\"normal_arms\":%zu}\n",
           yes(view->has_scalar_result),
           yes(view->has_scalar_result && view->scalar_result.known),
           view->has_scalar_result ? view->scalar_result.value : 0,
           yes(view->terminates), view->normal_arms);
    if (view->result_count > 2) {
        problem(observer, "result count exceeds public array contract");
        return;
    }
    arithmetic(observer, fragment, fid, node, view->kind);
    NLCheckedNodeId edges[] = {view->first_argument, view->next_argument,
                               view->initializer,    view->first_item,
                               view->next_item,      view->tail};
    for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i)
        visit_node(observer, fragment, fid, edges[i], depth + 1);
    if (view->kind == NL_CHECKED_REGISTERED_CALL) {
        const NLCheckedFragment *body = nl_checked_call_body(fragment, node);
        if (body && nl_checked_context(body) != context)
            problem(observer, "call body context differs from public contract");
        size_t child = visit_fragment(observer, body, depth + 1);
        printf("{\"event\":\"edge\",\"relation\":\"call_body\","
               "\"parent_fragment\":%zu,\"parent_node\":%zu,"
               "\"child_fragment\":%zu}\n",
               fid, node, child);
    } else if (view->kind == NL_CHECKED_IF) {
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedFragment *arm = nl_checked_if_arm(fragment, node, i);
            size_t child = visit_fragment(observer, arm, depth + 1);
            printf("{\"event\":\"edge\",\"relation\":\"if_arm\","
                   "\"parent_fragment\":%zu,\"parent_node\":%zu,"
                   "\"index\":%zu,\"child_fragment\":%zu,"
                   "\"hypothetical_owned_evidence\":true}\n",
                   fid, node, i, child);
        }
    }
}

static size_t visit_fragment(Observer *observer,
                             const NLCheckedFragment *fragment, size_t depth)
{
    if (!fragment || observer->failed)
        return 0;
    for (size_t i = 0; i < observer->fragment_count; ++i)
        if (observer->fragments[i] == fragment)
            return i + 1;
    if (depth > MAX_DEPTH || observer->fragment_count == MAX_OBJECTS) {
        problem(observer, "fragment observation budget");
        return 0;
    }
    size_t fid = ++observer->fragment_count;
    observer->fragments[fid - 1] = fragment;
    size_t sid = source_id(observer, nl_checked_source(fragment));
    size_t cid = context_id(observer, nl_checked_context(fragment));
    printf("{\"event\":\"fragment\",\"id\":%zu,\"source\":%zu,"
           "\"context\":%zu,\"root\":%zu,\"node_count\":%zu}\n",
           fid, sid, cid, nl_checked_root(fragment),
           nl_checked_node_count(fragment));
    visit_node(observer, fragment, fid, nl_checked_root(fragment), depth + 1);
    return fid;
}

static void declarations(const NLSyntaxTree *tree)
{
    const NLSource *source = nl_syntax_tree_source(tree);
    const NLSyntaxView *unit = nl_syntax_node_view(nl_syntax_tree_root(tree));
    if (!unit || unit->kind != NL_SYNTAX_FUNCTION_UNIT)
        return;
    for (const NLSyntaxNode *node = unit->data.function_unit.declarations; node;
         node = nl_syntax_next_argument(node)) {
        const NLSyntaxView *function = nl_syntax_node_view(node);
        if (!function || function->kind != NL_SYNTAX_FUNCTION)
            continue;
        const NLSyntaxView *body =
            nl_syntax_node_view(function->data.function.body);
        printf("{\"event\":\"declaration\",\"name_hex\":");
        span_hex(source, function->data.function.name);
        printf(",\"span\":[%zu,%zu],\"parameter_count\":%zu,"
               "\"body_span\":[%zu,%zu]}\n",
               function->span.start_byte, function->span.end_byte,
               function->data.function.count, body ? body->span.start_byte : 0,
               body ? body->span.end_byte : 0);
    }
}

int main(int argc, char **argv)
{
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *tree = NULL, *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    Observer *observer = NULL;
    int result = 3;
    if (argc != 3) {
        fputs("usage: r1595_public_observe ORIGINAL ENTRY_EXPRESSION\n",
              stderr);
        return 3;
    }
    NLSourceStatus loaded = nl_source_load(argv[1], &source);
    printf("{\"event\":\"status\",\"stage\":\"source_load\","
           "\"number\":%d}\n",
           (int)loaded);
    if (loaded != NL_SOURCE_OK)
        goto cleanup;
    NLParseStatus parsed = nl_parser_create(source, &parser);
    printf("{\"event\":\"status\",\"stage\":\"parser_create\","
           "\"status\":\"%s\"}\n",
           parse_name(parsed));
    if (parsed != NL_PARSE_OK)
        goto cleanup;
    NLParseDiagnostic parse_diagnostic = {0};
    parsed = nl_parser_parse_function_unit(parser, &tree, &parse_diagnostic);
    printf("{\"event\":\"status\",\"stage\":\"unit_parse\","
           "\"status\":\"%s\"}\n",
           parse_name(parsed));
    if (parsed != NL_PARSE_OK) {
        nl_parse_diagnostic_render(stderr, source, &parse_diagnostic);
        result = parse_host_failure(parsed) ? 3 : 0;
        goto cleanup;
    }
    declarations(tree);
    NLCheckStatus status = nl_semantic_create(&context);
    printf("{\"event\":\"status\",\"stage\":\"semantic_create\","
           "\"status\":\"%s\"}\n",
           check_name(status));
    if (status != NL_CHECK_OK)
        goto cleanup;
    const NLSyntaxTree *inputs[] = {tree};
    NLFunctionUnitDiagnostic unit_diagnostic = {0};
    status = nl_semantic_register_function_unit(context, inputs, 1,
                                                &unit_diagnostic);
    printf("{\"event\":\"status\",\"stage\":\"unit_register\","
           "\"status\":\"%s\",\"input_index\":%zu}\n",
           check_name(status), unit_diagnostic.input_index);
    if (status != NL_CHECK_OK) {
        if (unit_diagnostic.input_index == 0)
            nl_check_diagnostic_render(stderr, source,
                                       &unit_diagnostic.diagnostic);
        result = check_host_failure(status) ? 3 : 0;
        goto cleanup;
    }
    loaded = nl_source_load(argv[2], &entry_source);
    printf("{\"event\":\"status\",\"stage\":\"apparatus_entry_load\","
           "\"number\":%d}\n",
           (int)loaded);
    if (loaded != NL_SOURCE_OK)
        goto cleanup;
    parsed = nl_parser_create(entry_source, &entry_parser);
    printf("{\"event\":\"status\",\"stage\":\"apparatus_entry_parser_create\","
           "\"status\":\"%s\",\"number\":%d}\n",
           parse_name(parsed), (int)parsed);
    if (parsed != NL_PARSE_OK)
        goto cleanup;
    parsed = nl_parser_parse_expression_fragment(entry_parser, &entry_tree,
                                                 &parse_diagnostic);
    printf("{\"event\":\"status\",\"stage\":\"apparatus_entry_parse\","
           "\"status\":\"%s\"}\n",
           parse_name(parsed));
    if (parsed != NL_PARSE_OK) {
        nl_parse_diagnostic_render(stderr, entry_source, &parse_diagnostic);
        goto cleanup;
    }
    NLCheckDiagnostic check_diagnostic = {0};
    status = nl_semantic_check_expression(context, entry_tree, &checked,
                                          &check_diagnostic);
    printf("{\"event\":\"status\",\"stage\":\"apparatus_entry_check\","
           "\"status\":\"%s\"}\n",
           check_name(status));
    if (status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, entry_source, &check_diagnostic);
        result = check_host_failure(status) ? 3 : 0;
        goto cleanup;
    }
    observer = calloc(1, sizeof(*observer));
    printf("{\"event\":\"status\",\"stage\":\"observer_allocate\","
           "\"status\":\"%s\",\"requested_bytes\":%zu}\n",
           observer ? "OK" : "OUT_OF_MEMORY", sizeof(*observer));
    if (!observer)
        goto cleanup;
    size_t original_source = source_id(observer, source);
    printf("{\"event\":\"original_input\",\"source\":%zu}\n", original_source);
    size_t root_fragment = visit_fragment(observer, checked, 0);
    printf("{\"event\":\"observation_complete\",\"root_fragment\":%zu,"
           "\"public_scope\":\"PARTIAL\",\"complete_call_result_auth\":"
           "\"NOT_OBSERVABLE\",\"quantity_runtime\":\"NOT_OBSERVED\","
           "\"failed\":%s}\n",
           root_fragment, yes(observer->failed));
    result = observer->failed ? 4 : 0;

cleanup:
    printf("{\"event\":\"observer_exit\",\"status\":%d}\n", result);
    if (ferror(stdout) || fflush(stdout) != 0)
        result = 3;
    free(observer);
    nl_checked_destroy(checked);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    nl_semantic_destroy(context);
    return result;
}
