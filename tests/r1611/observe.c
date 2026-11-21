#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdio.h>
#include <string.h>

/* R1601 TEXT DRAFT: no execution has been authorized by this file.
 * No assert(): all error handling remains active in Release.
 * Only the original registered main call is entered. No helper is reinvoked.
 */
enum {
    R_MAX_FRAMES = 16,
    R_MAX_NODES = 512,
    R_MAX_SOURCE = 4096
};

typedef struct {
    size_t frame;
    NLCheckedNodeId node;
} Seen;

typedef struct {
    size_t frames, visits, seen_count;
    Seen seen[R_MAX_NODES];
    bool failed, limited;
} Walk;

static const char *bit(bool value)
{
    return value ? "true" : "false";
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
    return "UNCLASSIFIED";
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
    return "UNCLASSIFIED";
}

static bool ordinary_parse_refusal(NLParseStatus status)
{
    return status == NL_PARSE_LEXICALLY_UNSUPPORTED ||
           status == NL_PARSE_SYNTAX_UNSUPPORTED ||
           status == NL_PARSE_SYNTAX_ERROR;
}

static bool ordinary_check_refusal(NLCheckStatus status)
{
    return status == NL_CHECK_SEMANTIC_ERROR ||
           status == NL_CHECK_SEMANTIC_UNSUPPORTED ||
           status == NL_CHECK_ANALYSIS_PRECISION_LIMIT;
}

static const char *kind_name(NLCheckedKind kind)
{
    switch (kind) {
    case NL_CHECKED_REGISTERED_CALL:
        return "CALL";
    case NL_CHECKED_BINDING:
        return "BINDING";
    case NL_CHECKED_BLOCK:
        return "BLOCK";
    case NL_CHECKED_STATEMENT:
        return "STATEMENT";
    case NL_CHECKED_IDENTIFIER:
        return "IDENTIFIER";
    case NL_CHECKED_UNIT:
        return "UNIT";
    case NL_CHECKED_USIZE_LITERAL:
        return "USIZE_LITERAL";
    case NL_CHECKED_USIZE_ADDITION:
        return "USIZE_ADDITION";
    case NL_CHECKED_USIZE_SUBTRACTION:
        return "USIZE_SUBTRACTION";
    case NL_CHECKED_USIZE_DIVISION:
        return "USIZE_DIVISION";
    case NL_CHECKED_USIZE_REMAINDER:
        return "USIZE_REMAINDER";
    default:
        return "OTHER_PUBLIC_KIND";
    }
}

static const char *use_name(NLValueUse use)
{
    switch (use) {
    case NL_VALUE_USE_NONE:
        return "NONE";
    case NL_VALUE_COPIED:
        return "COPIED";
    case NL_VALUE_CONSUMED:
        return "CONSUMED";
    case NL_VALUE_REBORROWED:
        return "REBORROWED";
    case NL_VALUE_RECEIVED:
        return "RECEIVED";
    }
    return "UNCLASSIFIED";
}

static void hex(const unsigned char *bytes, size_t length)
{
    putchar('"');
    for (size_t i = 0; i < length; ++i) {
        printf("%02x", (unsigned)bytes[i]);
    }
    putchar('"');
}

static bool span_hex(const NLSource *source, NLSourceSpan span)
{
    NLSourceView view = {0};
    if (!source || !nl_source_view(source, span, &view) ||
        view.length > R_MAX_SOURCE) {
        fputs("null", stdout);
        return false;
    }
    hex(view.bytes, view.length);
    return true;
}

static void source_record(const NLSource *source, size_t frame,
                          const char *role)
{
    printf("{\"record\":\"source\",\"frame\":%zu,\"role\":\"%s\",", frame,
           role);
    if (!source) {
        fputs("\"available\":false,\"length\":0,\"hex\":null}\n", stdout);
        return;
    }
    size_t length = nl_source_length(source);
    printf("\"length\":%zu,\"available\":%s,\"hex\":", length,
           bit(length <= R_MAX_SOURCE));
    if (length <= R_MAX_SOURCE) {
        NLSourceSpan all = {0, length};
        (void)span_hex(source, all);
    } else {
        fputs("null", stdout);
    }
    fputs("}\n", stdout);
}

static const char *severity_name(NLDiagnosticSeverity severity)
{
    switch (severity) {
    case NL_DIAG_NOTE:
        return "NOTE";
    case NL_DIAG_WARNING:
        return "WARNING";
    case NL_DIAG_ERROR:
        return "ERROR";
    }
    return "UNCLASSIFIED";
}

/* These are actual borrowed diagnostic fields, not expected spellings.
 * A capped/unavailable field makes the record incomplete, never a match. */
static bool diagnostic_text(const char *text)
{
    if (!text) {
        fputs("null", stdout);
        return true;
    }
    size_t length = 0;
    while (length <= R_MAX_SOURCE && text[length] != '\0') {
        ++length;
    }
    if (length > R_MAX_SOURCE) {
        fputs("null", stdout);
        return false;
    }
    hex((const unsigned char *)text, length);
    return true;
}

static bool diagnostic_range(const NLSourceRange *range)
{
    if (!range) {
        fputs("null", stdout);
        return true;
    }
    fputs("{\"path_hex\":", stdout);
    bool complete = diagnostic_text(range->path);
    printf(",\"start_line\":%zu,\"start_column\":%zu,"
           "\"end_line\":%zu,\"end_column\":%zu}",
           range->start_line, range->start_column, range->end_line,
           range->end_column);
    return complete;
}

static bool diagnostic_record(const NLSource *source,
                              const NLDiagnostic *diagnostic, NLSourceSpan span,
                              const char *phase, const char *role,
                              size_t input_index)
{
    bool complete = true;
    bool valid = source && nl_source_span_valid(source, span);
    printf("{\"record\":\"diagnostic\",\"phase\":\"%s\","
           "\"source_role\":\"%s\",\"input_index\":%zu,"
           "\"source_name_hex\":",
           phase, role, input_index);
    if (!diagnostic_text(source ? nl_source_name(source) : NULL)) {
        complete = false;
    }
    printf(",\"severity\":\"%s\",\"category_hex\":",
           severity_name(diagnostic->severity));
    if (!diagnostic_text(diagnostic->category)) {
        complete = false;
    }
    fputs(",\"code_hex\":", stdout);
    if (!diagnostic_text(diagnostic->code)) {
        complete = false;
    }
    fputs(",\"message_hex\":", stdout);
    if (!diagnostic_text(diagnostic->message)) {
        complete = false;
    }
    printf(",\"span\":[%zu,%zu],\"span_valid\":%s,\"span_hex\":",
           span.start_byte, span.end_byte, bit(valid));
    if (!span_hex(source, span)) {
        complete = false;
    }
    fputs(",\"range\":", stdout);
    if (!diagnostic_range(diagnostic->range)) {
        complete = false;
    }
    printf(",\"note_count\":%zu,\"notes_present\":%s,\"notes\":[",
           diagnostic->note_count, bit(diagnostic->notes != NULL));
    if (diagnostic->note_count > R_MAX_FRAMES ||
        (diagnostic->note_count != 0 && !diagnostic->notes)) {
        complete = false;
    } else {
        for (size_t i = 0; i < diagnostic->note_count; ++i) {
            if (i != 0) {
                putchar(',');
            }
            fputs("{\"message_hex\":", stdout);
            if (!diagnostic_text(diagnostic->notes[i].message)) {
                complete = false;
            }
            fputs(",\"range\":", stdout);
            if (!diagnostic_range(diagnostic->notes[i].range)) {
                complete = false;
            }
            putchar('}');
        }
    }
    printf("],\"complete\":%s}\n", bit(complete && valid));
    return complete && valid;
}

static bool render_parse(const NLSource *source,
                         const NLParseDiagnostic *diagnostic, const char *phase,
                         const char *role)
{
    bool complete = diagnostic_record(source, &diagnostic->diagnostic,
                                      diagnostic->span, phase, role, 0);
    bool rendered =
        complete && nl_parse_diagnostic_render(stderr, source, diagnostic);
    printf("{\"record\":\"diagnostic_render\",\"phase\":\"%s\","
           "\"source_role\":\"%s\",\"input_index\":0,"
           "\"attempted\":%s,\"ok\":%s}\n",
           phase, role, bit(complete), bit(rendered));
    return complete && rendered;
}

static bool render_check(const NLSource *source,
                         const NLCheckDiagnostic *diagnostic, const char *phase,
                         const char *role, size_t input_index)
{
    bool complete =
        diagnostic_record(source, &diagnostic->diagnostic, diagnostic->span,
                          phase, role, input_index);
    bool rendered =
        complete && nl_check_diagnostic_render(stderr, source, diagnostic);
    printf("{\"record\":\"diagnostic_render\",\"phase\":\"%s\","
           "\"source_role\":\"%s\",\"input_index\":%zu,"
           "\"attempted\":%s,\"ok\":%s}\n",
           phase, role, input_index, bit(complete), bit(rendered));
    return complete && rendered;
}

static void scalar(const NLSemanticContext *context, NLCheckedResult result)
{
    NLSemanticValueView value = {0};
    NLSemanticTypeView type = {0};
    bool have_value = result.value != 0 &&
                      nl_semantic_value_view(context, result.value, &value);
    bool have_type =
        result.type != 0 && nl_semantic_type_view(context, result.type, &type);
    bool quantity = have_value && have_type && type.kind == NL_TYPE_USIZE &&
                    value.type == result.type;
    bool known = quantity && value.scalar_known;
    printf("{\"type\":%zu,\"value\":%zu,\"value_view\":%s,"
           "\"type_view\":%s,\"usize\":%s,\"known\":%s,\"number\":",
           result.type, result.value, bit(have_value), bit(have_type),
           bit(quantity), bit(known));
    if (known) {
        printf("%zu", value.scalar_value);
    } else {
        fputs("null", stdout);
    }
    fputs("}", stdout);
}

static void arithmetic(const NLCheckedFragment *fragment, size_t frame,
                       NLCheckedNodeId id, NLCheckedKind kind, Walk *walk)
{
    if (kind == NL_CHECKED_USIZE_DIVISION) {
        NLUsizeDivisionView view = {0};
        bool present = nl_checked_usize_division_view(fragment, id, &view);
        printf("{\"record\":\"arithmetic\",\"op\":\"division\","
               "\"frame\":%zu,\"node\":%zu,\"present\":%s",
               frame, id, bit(present));
        if (present) {
            bool valid = nl_checked_usize_division_valid(fragment, id, NULL);
            bool copy_valid =
                nl_checked_usize_division_valid(fragment, id, &view);
            printf(",\"actual_valid\":%s,\"copy_valid\":%s,"
                   "\"origin_is_fragment\":%s,\"event_node\":%zu,"
                   "\"operands\":[%zu,%zu],\"values\":[%zu,%zu],"
                   "\"result\":%zu,\"type\":%zu,"
                   "\"symbols\":[%zu,%zu],\"places\":[%zu,%zu],"
                   "\"incarnations\":[%zu,%zu],\"facts\":[%zu,%zu],"
                   "\"span\":[%zu,%zu],\"operator_span\":[%zu,%zu],"
                   "\"operand_spans\":[[%zu,%zu],[%zu,%zu]],"
                   "\"numerator_known\":%s,\"result_known\":%s,"
                   "\"raw_numerator\":%zu,\"divisor\":%zu,"
                   "\"raw_result\":%zu,\"target_max\":%zu",
                   bit(valid), bit(copy_valid), bit(view.origin == fragment),
                   view.node, view.operands[0], view.operands[1],
                   view.values[0], view.values[1], view.result, view.type,
                   view.symbols[0], view.symbols[1], view.places[0],
                   view.places[1], view.incarnations[0], view.incarnations[1],
                   view.facts[0], view.facts[1], view.span.start_byte,
                   view.span.end_byte, view.operator_span.start_byte,
                   view.operator_span.end_byte,
                   view.operand_spans[0].start_byte,
                   view.operand_spans[0].end_byte,
                   view.operand_spans[1].start_byte,
                   view.operand_spans[1].end_byte, bit(view.numerator_known),
                   bit(view.quotient_known), view.numbers[0], view.numbers[1],
                   view.quotient, view.target_max);
            if (!valid || !copy_valid || view.origin != fragment ||
                view.node != id) {
                walk->failed = true;
            }
        }
        fputs("}\n", stdout);
    } else if (kind == NL_CHECKED_USIZE_REMAINDER) {
        NLUsizeRemainderView view = {0};
        bool present = nl_checked_usize_remainder_view(fragment, id, &view);
        printf("{\"record\":\"arithmetic\",\"op\":\"remainder\","
               "\"frame\":%zu,\"node\":%zu,\"present\":%s",
               frame, id, bit(present));
        if (present) {
            bool valid = nl_checked_usize_remainder_valid(fragment, id, NULL);
            bool copy_valid =
                nl_checked_usize_remainder_valid(fragment, id, &view);
            printf(",\"actual_valid\":%s,\"copy_valid\":%s,"
                   "\"origin_is_fragment\":%s,\"event_node\":%zu,"
                   "\"source_is_fragment_source\":%s,"
                   "\"operands\":[%zu,%zu],\"values\":[%zu,%zu],"
                   "\"result\":%zu,\"type\":%zu,"
                   "\"symbols\":[%zu,%zu],\"places\":[%zu,%zu],"
                   "\"incarnations\":[%zu,%zu],\"facts\":[%zu,%zu],"
                   "\"span\":[%zu,%zu],\"operator_span\":[%zu,%zu],"
                   "\"operand_spans\":[[%zu,%zu],[%zu,%zu]],"
                   "\"numerator_known\":%s,\"result_known\":%s,"
                   "\"raw_numerator\":%zu,\"divisor\":%zu,"
                   "\"raw_result\":%zu,\"target_max\":%zu",
                   bit(valid), bit(copy_valid), bit(view.origin == fragment),
                   view.node, bit(view.source == nl_checked_source(fragment)),
                   view.operands[0], view.operands[1], view.values[0],
                   view.values[1], view.result, view.type, view.symbols[0],
                   view.symbols[1], view.places[0], view.places[1],
                   view.incarnations[0], view.incarnations[1], view.facts[0],
                   view.facts[1], view.span.start_byte, view.span.end_byte,
                   view.operator_span.start_byte, view.operator_span.end_byte,
                   view.operand_spans[0].start_byte,
                   view.operand_spans[0].end_byte,
                   view.operand_spans[1].start_byte,
                   view.operand_spans[1].end_byte, bit(view.numerator_known),
                   bit(view.remainder_known), view.numbers[0], view.numbers[1],
                   view.remainder, view.target_max);
            if (!valid || !copy_valid || view.origin != fragment ||
                view.node != id) {
                walk->failed = true;
            }
        }
        fputs("}\n", stdout);
    }
}

static void visit(const NLCheckedFragment *, size_t, NLCheckedNodeId,
                  NLCheckedNodeId, const char *, size_t, Walk *);

static void visit_chain(const NLCheckedFragment *fragment, size_t frame,
                        NLCheckedNodeId first, NLCheckedNodeId parent,
                        bool items, size_t depth, Walk *walk)
{
    NLCheckedNodeId id = first;
    size_t count = 0;
    while (id != 0 && !walk->failed && !walk->limited) {
        if (++count > R_MAX_NODES) {
            walk->limited = true;
            break;
        }
        const NLCheckedNodeView *view = nl_checked_node_view(fragment, id);
        if (!view) {
            walk->failed = true;
            break;
        }
        NLCheckedNodeId next = items ? view->next_item : view->next_argument;
        visit(fragment, frame, id, parent, items ? "item" : "argument",
              depth + 1, walk);
        id = next;
    }
}

static void visit(const NLCheckedFragment *fragment, size_t frame,
                  NLCheckedNodeId id, NLCheckedNodeId parent, const char *edge,
                  size_t depth, Walk *walk)
{
    if (id == 0 || walk->failed || walk->limited) {
        return;
    }
    if (depth > 32 || walk->visits >= R_MAX_NODES) {
        walk->limited = true;
        return;
    }
    for (size_t i = 0; i < walk->seen_count; ++i) {
        if (walk->seen[i].frame == frame && walk->seen[i].node == id) {
            printf("{\"record\":\"revisit\",\"frame\":%zu,\"node\":%zu,"
                   "\"parent\":%zu,\"edge\":\"%s\"}\n",
                   frame, id, parent, edge);
            return;
        }
    }
    walk->seen[walk->seen_count++] = (Seen){frame, id};
    ++walk->visits;
    const NLCheckedNodeView *view = nl_checked_node_view(fragment, id);
    const NLSource *source = nl_checked_source(fragment);
    const NLSemanticContext *context = nl_checked_context(fragment);
    if (!view || !source || !context) {
        walk->failed = true;
        return;
    }
    printf("{\"record\":\"node\",\"frame\":%zu,\"node\":%zu,"
           "\"parent\":%zu,\"edge\":\"%s\",\"kind\":\"%s\","
           "\"kind_number\":%d,\"span\":[%zu,%zu],\"name_hex\":",
           frame, id, parent, edge, kind_name(view->kind), (int)view->kind,
           view->span.start_byte, view->span.end_byte);
    (void)span_hex(source, view->name);
    fputs(",\"text_hex\":", stdout);
    (void)span_hex(source, view->span);
    printf(",\"function\":%zu,\"symbol\":%zu,\"body_backed\":%s,"
           "\"value_use\":\"%s\",\"initializer\":%zu,\"tail\":%zu,"
           "\"argument_count\":%zu,\"item_count\":%zu,"
           "\"result_count\":%zu,\"results\":[",
           view->function, view->symbol, bit(view->body_backed),
           use_name(view->value_use), view->initializer, view->tail,
           view->argument_count, view->item_count, view->result_count);
    if (view->result_count > 2) {
        walk->failed = true;
    } else {
        for (size_t i = 0; i < view->result_count; ++i) {
            if (i != 0) {
                putchar(',');
            }
            scalar(context, view->results[i]);
        }
    }
    fputs("]}\n", stdout);
    arithmetic(fragment, frame, id, view->kind, walk);

    switch (view->kind) {
    case NL_CHECKED_BINDING:
        visit(fragment, frame, view->initializer, id, "initializer", depth + 1,
              walk);
        break;
    case NL_CHECKED_BLOCK:
        visit_chain(fragment, frame, view->first_item, id, true, depth, walk);
        visit(fragment, frame, view->tail, id, "tail", depth + 1, walk);
        break;
    case NL_CHECKED_STATEMENT:
        /* This selected suite has no expression-statement child. Do not
         * invent a field interpretation for an unneeded public category. */
        printf("{\"record\":\"limit\",\"frame\":%zu,\"node\":%zu,"
               "\"relation\":\"statement_child_NOT_OBSERVABLE\"}\n",
               frame, id);
        break;
    case NL_CHECKED_REGISTERED_CALL: {
        visit_chain(fragment, frame, view->first_argument, id, false, depth,
                    walk);
        const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
        if (!body) {
            printf("{\"record\":\"body\",\"frame\":%zu,\"call\":%zu,"
                   "\"available\":false}\n",
                   frame, id);
            break;
        }
        if (walk->frames >= R_MAX_FRAMES) {
            walk->limited = true;
            break;
        }
        size_t child = walk->frames++;
        bool same_context = nl_checked_context(body) == context;
        printf("{\"record\":\"body\",\"frame\":%zu,\"call\":%zu,"
               "\"available\":true,\"child_frame\":%zu,"
               "\"same_context\":%s,\"root\":%zu,\"node_count\":%zu}\n",
               frame, id, child, bit(same_context), nl_checked_root(body),
               nl_checked_node_count(body));
        if (!same_context) {
            walk->failed = true;
            break;
        }
        source_record(nl_checked_source(body), child, "registered_call_body");
        visit(body, child, nl_checked_root(body), 0, "root", depth + 1, walk);
        break;
    }
    case NL_CHECKED_USIZE_ADDITION:
    case NL_CHECKED_USIZE_SUBTRACTION:
    case NL_CHECKED_USIZE_DIVISION:
    case NL_CHECKED_USIZE_REMAINDER:
        visit_chain(fragment, frame, view->first_argument, id, false, depth,
                    walk);
        break;
    case NL_CHECKED_IDENTIFIER:
    case NL_CHECKED_UNIT:
    case NL_CHECKED_USIZE_LITERAL:
        break;
    default:
        printf("{\"record\":\"limit\",\"frame\":%zu,\"node\":%zu,"
               "\"relation\":\"unread_kind_children_NOT_OBSERVABLE\"}\n",
               frame, id);
        break;
    }
}

static bool declarations(const NLSyntaxTree *tree)
{
    const NLSyntaxView *root = nl_syntax_node_view(nl_syntax_tree_root(tree));
    if (!root || root->kind != NL_SYNTAX_FUNCTION_UNIT) {
        return false;
    }
    const NLSource *source = nl_syntax_tree_source(tree);
    const NLSyntaxNode *node = root->data.function_unit.declarations;
    size_t count = 0;
    while (node) {
        if (++count > NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS) {
            return false;
        }
        const NLSyntaxView *view = nl_syntax_node_view(node);
        if (!view || view->kind != NL_SYNTAX_FUNCTION) {
            return false;
        }
        printf("{\"record\":\"declaration\",\"ordinal\":%zu,\"name_hex\":",
               count);
        (void)span_hex(source, view->data.function.name);
        printf(",\"span\":[%zu,%zu],\"parameter_count\":%zu}\n",
               view->span.start_byte, view->span.end_byte,
               view->data.function.count);
        node = nl_syntax_next_argument(node);
    }
    return count == root->data.function_unit.count;
}

int main(int argc, char **argv)
{
    NLSource *original = NULL, *entry = NULL;
    NLParser *unit_parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit_tree = NULL, *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    NLParseDiagnostic pd = {0};
    NLFunctionUnitDiagnostic ud = {0};
    NLCheckDiagnostic cd = {0};
    int outcome = 20;
    const unsigned char adapter[] = "main()\n";

    if (argc != 3 ||
        (strcmp(argv[2], "invalid") != 0 && strcmp(argv[2], "lawful") != 0)) {
        fputs("R1601 observer usage: observe ORIGINAL_UNIT invalid|lawful\n",
              stderr);
        return 20;
    }
    bool invalid_original = strcmp(argv[2], "invalid") == 0;
    /* This frozen host-side law flag is not an input fact to any compiler API.
     */
    printf("{\"record\":\"observer_config\",\"revision\":\"V2\","
           "\"source_law\":\"%s\"}\n",
           invalid_original ? "INVALID" : "LAWFUL");
    NLSourceStatus source_status = nl_source_load(argv[1], &original);
    printf("{\"record\":\"phase\",\"phase\":\"original_source_load\","
           "\"status_number\":%d,\"ok\":%s}\n",
           (int)source_status, bit(source_status == NL_SOURCE_OK));
    if (source_status != NL_SOURCE_OK) {
        goto cleanup;
    }
    source_record(original, 0, "original_unit");
    NLParseStatus ps = nl_parser_create(original, &unit_parser);
    printf("{\"record\":\"phase\",\"phase\":\"unit_parser_create\","
           "\"status\":\"%s\"}\n",
           parse_name(ps));
    if (ps != NL_PARSE_OK) {
        goto cleanup;
    }
    ps = nl_parser_parse_function_unit(unit_parser, &unit_tree, &pd);
    printf("{\"record\":\"phase\",\"phase\":\"original_unit_parse\","
           "\"status\":\"%s\"}\n",
           parse_name(ps));
    if (ps != NL_PARSE_OK) {
        outcome = ordinary_parse_refusal(ps) ? 10 : 20;
        if (!render_parse(original, &pd, "original_unit_parse",
                          "original_unit")) {
            outcome = 20;
        }
        goto cleanup;
    }
    if (!declarations(unit_tree)) {
        fputs("R1601 unexpected public declaration traversal\n", stderr);
        goto cleanup;
    }
    NLCheckStatus cs = nl_semantic_create(&context);
    printf("{\"record\":\"phase\",\"phase\":\"context_create\","
           "\"status\":\"%s\"}\n",
           check_name(cs));
    if (cs != NL_CHECK_OK) {
        goto cleanup;
    }
    const NLSyntaxTree *inputs[] = {unit_tree};
    cs = nl_semantic_register_function_unit(context, inputs, 1, &ud);
    printf("{\"record\":\"phase\",\"phase\":\"original_unit_register\","
           "\"status\":\"%s\",\"input_index\":%zu}\n",
           check_name(cs), ud.input_index);
    if (cs != NL_CHECK_OK) {
        outcome = ordinary_check_refusal(cs) ? 11 : 20;
        if (!render_check(original, &ud.diagnostic, "original_unit_register",
                          "original_unit", ud.input_index)) {
            outcome = 20;
        }
        goto cleanup;
    }

    if (invalid_original) {
        /* Definition registration already admitted every original declaration.
         * No later adapter refusal may rescue this true-invalid admission. */
        fputs("{\"record\":\"admission_stop\","
              "\"phase\":\"original_unit_register\","
              "\"reason\":\"TRUE_INVALID_ORIGINAL_UNIT_REGISTERED\","
              "\"synthetic_entry\":\"NOT_STARTED\"}\n",
              stdout);
        outcome = 23;
        goto cleanup;
    }

    source_status = nl_source_create(adapter, sizeof(adapter) - 1,
                                     "R1601 synthetic main() adapter", &entry);
    printf("{\"record\":\"phase\",\"phase\":\"synthetic_entry_create\","
           "\"status_number\":%d,\"ok\":%s}\n",
           (int)source_status, bit(source_status == NL_SOURCE_OK));
    if (source_status != NL_SOURCE_OK) {
        goto cleanup;
    }
    ps = nl_parser_create(entry, &entry_parser);
    printf("{\"record\":\"phase\",\"phase\":\"entry_parser_create\","
           "\"status\":\"%s\"}\n",
           parse_name(ps));
    if (ps != NL_PARSE_OK) {
        goto cleanup;
    }
    pd = (NLParseDiagnostic){0};
    ps = nl_parser_parse_expression_fragment(entry_parser, &entry_tree, &pd);
    printf("{\"record\":\"phase\",\"phase\":\"synthetic_entry_parse\","
           "\"status\":\"%s\"}\n",
           parse_name(ps));
    if (ps != NL_PARSE_OK) {
        if (!render_parse(entry, &pd, "synthetic_entry_parse",
                          "synthetic_entry")) {
            fputs("R1601 adapter diagnostic render failed\n", stderr);
        }
        goto cleanup;
    }
    cs = nl_semantic_check_expression(context, entry_tree, &checked, &cd);
    printf("{\"record\":\"phase\",\"phase\":\"synthetic_entry_check\","
           "\"status\":\"%s\"}\n",
           check_name(cs));
    if (cs != NL_CHECK_OK) {
        outcome = ordinary_check_refusal(cs) ? 12 : 20;
        if (!render_check(entry, &cd, "synthetic_entry_check",
                          "synthetic_entry", 0)) {
            outcome = 20;
        }
        goto cleanup;
    }
    {
        Walk walk = {0};
        walk.frames = 1;
        source_record(nl_checked_source(checked), 0, "synthetic_entry");
        printf("{\"record\":\"entry_artifact\",\"root\":%zu,"
               "\"node_count\":%zu}\n",
               nl_checked_root(checked), nl_checked_node_count(checked));
        visit(checked, 0, nl_checked_root(checked), 0, "root", 0, &walk);
        printf("{\"record\":\"observation_end\",\"frames\":%zu,"
               "\"visits\":%zu,\"contract_failure\":%s,\"bounded_stop\":%s,"
               "\"whole_call_custody\":\"NOT_OBSERVABLE\","
               "\"runtime_quantity\":\"NOT_OBSERVABLE\","
               "\"runtime_order\":\"NOT_OBSERVABLE\"}\n",
               walk.frames, walk.visits, bit(walk.failed), bit(walk.limited));
        outcome = walk.failed ? 21 : (walk.limited ? 22 : 0);
    }

cleanup:
    /* Views are used only while every owner is live. No source/world setters,
     * fixture registrations, binding-result seeds, or callback hooks. */
    nl_checked_destroy(checked);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry);
    nl_syntax_tree_destroy(unit_tree);
    nl_parser_destroy(unit_parser);
    nl_source_destroy(original);
    printf("{\"record\":\"observer_end\",\"exit\":%d}\n", outcome);
    if (fflush(stdout) != 0 || fflush(stderr) != 0 || ferror(stdout) ||
        ferror(stderr)) {
        return 20;
    }
    return outcome;
}
