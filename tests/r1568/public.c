#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    NLSource *source, *selector;
    NLParser *parser, *selector_parser;
    NLSyntaxTree *tree, *selector_tree;
    NLSemanticContext *context;
    NLCheckedFragment *checked;
} Unit;

typedef struct {
    unsigned cohort;
    bool division;
    const NLCheckedFragment *fragment;
    NLCheckedNodeId node;
    NLUsizeDivisionView div;
    NLUsizeRemainderView rem;
} Event;

static Event events[128];
static size_t event_count;
static unsigned errors;
static size_t fragments;

static void string(const unsigned char *bytes, size_t length)
{
    putchar('"');
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = bytes[i];
        if (c == '"' || c == '\\') {
            putchar('\\');
            putchar(c);
        } else if (c < 32 || c >= 127) {
            printf("\\u%04x", (unsigned)c);
        } else {
            putchar(c);
        }
    }
    putchar('"');
}

static void text(const char *value)
{
    string((const unsigned char *)value, strlen(value));
}

static void span_text(const NLSource *source, NLSourceSpan span)
{
    NLSourceView view;
    if (nl_source_view(source, span, &view))
        string(view.bytes, view.length);
    else
        text("<unavailable>");
}

static void name_copy(const NLSource *source, NLSourceSpan span, char out[128])
{
    NLSourceView view;
    if (!nl_source_view(source, span, &view) || view.length >= 128) {
        strcpy(out, "<unavailable>");
        return;
    }
    if (view.length)
        memcpy(out, view.bytes, view.length);
    out[view.length] = '\0';
}

static const char *kind_name(NLCheckedKind kind)
{
    switch (kind) {
    case NL_CHECKED_BINDING:
        return "binding";
    case NL_CHECKED_IDENTIFIER:
        return "identifier";
    case NL_CHECKED_REGISTERED_CALL:
        return "call";
    case NL_CHECKED_USIZE_DIVISION:
        return "division";
    case NL_CHECKED_USIZE_REMAINDER:
        return "remainder";
    case NL_CHECKED_RETURN:
        return "return";
    case NL_CHECKED_BLOCK:
        return "block";
    case NL_CHECKED_UNIT:
        return "unit";
    case NL_CHECKED_USIZE_LITERAL:
        return "literal";
    default:
        return "other";
    }
}

static void cleanup(Unit *unit)
{
    nl_checked_destroy(unit->checked);
    nl_semantic_destroy(unit->context);
    nl_syntax_tree_destroy(unit->selector_tree);
    nl_syntax_tree_destroy(unit->tree);
    nl_parser_destroy(unit->selector_parser);
    nl_parser_destroy(unit->parser);
    nl_source_destroy(unit->selector);
    nl_source_destroy(unit->source);
}

static bool load(Unit *unit, const char *path, unsigned cohort)
{
    NLSourceStatus source_status = nl_source_load(path, &unit->source);
    if (source_status != NL_SOURCE_OK) {
        printf("{\"record\":\"source_error\",\"cohort\":%u,\"status\":%d}\n",
               cohort, (int)source_status);
        return false;
    }
    NLParseDiagnostic parse_diagnostic;
    NLParseStatus parse_status = nl_parser_create(unit->source, &unit->parser);
    if (parse_status == NL_PARSE_OK)
        parse_status = nl_parser_parse_function_unit(
            unit->parser, &unit->tree, &parse_diagnostic);
    printf("{\"record\":\"parse\",\"cohort\":%u,\"status\":%d}\n", cohort,
           (int)parse_status);
    if (parse_status != NL_PARSE_OK)
        return false;
    bool found_main = false;
    const NLSyntaxView *root =
        nl_syntax_node_view(nl_syntax_tree_root(unit->tree));
    if (!root || root->kind != NL_SYNTAX_FUNCTION_UNIT)
        return false;
    for (const NLSyntaxNode *node = root->data.function_unit.declarations; node;
         node = nl_syntax_next_argument(node)) {
        const NLSyntaxView *view = nl_syntax_node_view(node);
        char name[128];
        if (!view || view->kind != NL_SYNTAX_FUNCTION)
            continue;
        name_copy(unit->source, view->data.function.name, name);
        if (strcmp(name, "main") == 0) {
            const NLSyntaxView *result =
                nl_syntax_node_view(view->data.function.result);
            char result_name[128];
            if (!result || result->kind != NL_SYNTAX_TYPE_NAME)
                return false;
            name_copy(unit->source, result->data.name, result_name);
            if (view->data.function.count != 0 ||
                strcmp(result_name, "unit") != 0 || found_main)
                return false;
            found_main = true;
        }
    }
    if (!found_main || nl_semantic_create(&unit->context) != NL_CHECK_OK)
        return false;
    const NLSyntaxTree *inputs[] = {unit->tree};
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLCheckStatus check_status = nl_semantic_register_function_unit(
        unit->context, inputs, 1, &diagnostic);
    printf("{\"record\":\"registration\",\"cohort\":%u,\"status\":%d}\n",
           cohort, (int)check_status);
    if (check_status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, unit->source,
                                   &diagnostic.diagnostic);
        return false;
    }
    /* Driver selection only; never a substitute guest body or seed. */
    if (nl_source_create("main()", 6, "R1568-driver-selector",
                         &unit->selector) != NL_SOURCE_OK)
        return false;
    if (nl_parser_create(unit->selector, &unit->selector_parser) != NL_PARSE_OK)
        return false;
    if (nl_parser_parse_expression_fragment(unit->selector_parser,
                                           &unit->selector_tree,
                                           &parse_diagnostic) != NL_PARSE_OK)
        return false;
    NLCheckDiagnostic entry_diagnostic;
    check_status = nl_semantic_check_expression(
        unit->context, unit->selector_tree, &unit->checked, &entry_diagnostic);
    printf("{\"record\":\"entry\",\"cohort\":%u,\"status\":%d}\n", cohort,
           (int)check_status);
    return check_status == NL_CHECK_OK;
}

static void arithmetic(const NLCheckedFragment *fragment, NLCheckedNodeId id,
                       const NLCheckedNodeView *node, unsigned cohort)
{
    Event event = {0};
    event.cohort = cohort;
    event.fragment = fragment;
    event.node = id;
    event.division = node->kind == NL_CHECKED_USIZE_DIVISION;
    bool present, valid = false, known = false;
    bool numerator_known = false, quantity_known = false;
    size_t numerator = 0, divisor = 0, result = 0, maximum = 0;
    if (event.division) {
        present = nl_checked_usize_division_view(fragment, id, &event.div);
        if (present) {
            valid = nl_checked_usize_division_valid(fragment, id, NULL) &&
                    nl_checked_usize_division_valid(fragment, id, &event.div);
            numerator_known = event.div.numerator_known;
            quantity_known = event.div.quotient_known;
            known = numerator_known && quantity_known;
            numerator = event.div.numbers[0];
            divisor = event.div.numbers[1];
            result = event.div.quotient;
            maximum = event.div.target_max;
        }
    } else {
        present = nl_checked_usize_remainder_view(fragment, id, &event.rem);
        if (present) {
            valid = nl_checked_usize_remainder_valid(fragment, id, NULL) &&
                    nl_checked_usize_remainder_valid(fragment, id, &event.rem);
            numerator_known = event.rem.numerator_known;
            quantity_known = event.rem.remainder_known;
            known = numerator_known && quantity_known;
            numerator = event.rem.numbers[0];
            divisor = event.rem.numbers[1];
            result = event.rem.remainder;
            maximum = event.rem.target_max;
        }
    }
    printf("{\"record\":\"arithmetic\",\"cohort\":%u,\"origin\":\"%p\",",
           cohort, (const void *)fragment);
    printf("\"node\":%zu,\"kind\":", id);
    text(kind_name(node->kind));
    printf(",\"present\":%s,\"valid\":%s,\"known\":%s,",
           present ? "true" : "false", valid ? "true" : "false",
           known ? "true" : "false");
    printf("\"numerator\":%zu,\"divisor\":%zu,\"quantity\":%zu,",
           numerator, divisor, result);
    printf("\"target_max\":%zu,\"span\":[%zu,%zu],", maximum,
           node->span.start_byte, node->span.end_byte);
    printf("\"numerator_known\":%s,\"quantity_known\":%s",
           numerator_known ? "true" : "false",
           quantity_known ? "true" : "false");
    if (present) {
        const NLValueId *values =
            event.division ? event.div.values : event.rem.values;
        const NLSymbolId *symbols =
            event.division ? event.div.symbols : event.rem.symbols;
        const NLPlaceId *places =
            event.division ? event.div.places : event.rem.places;
        const NLValueFactId *facts =
            event.division ? event.div.facts : event.rem.facts;
        NLSourceSpan operation = event.division ? event.div.operator_span
                                               : event.rem.operator_span;
        NLValueId result_id =
            event.division ? event.div.result : event.rem.result;
        printf(",\"values\":[%zu,%zu],\"result\":%zu,", values[0], values[1],
               result_id);
        printf("\"symbols\":[%zu,%zu],\"places\":[%zu,%zu],", symbols[0],
               symbols[1], places[0], places[1]);
        printf("\"facts\":[%zu,%zu],\"operator_span\":[%zu,%zu]", facts[0],
               facts[1], operation.start_byte, operation.end_byte);
    }
    puts("}");
    if (present && !valid)
        ++errors;
    if (present && event_count < 128)
        events[event_count++] = event;
    else if (present)
        puts("{\"record\":\"event_capacity\",\"status\":\"NOT_OBSERVED\"}");
}

static void walk_fragment(const NLCheckedFragment *fragment, unsigned cohort,
                          const char *scope, unsigned depth);

static void walk_node(const NLCheckedFragment *fragment, NLCheckedNodeId id,
                      unsigned cohort, const char *scope, unsigned depth,
                      NLCheckedNodeId seen[4096], size_t *seen_count)
{
    if (!id)
        return;
    for (size_t i = 0; i < *seen_count; ++i)
        if (seen[i] == id)
            return;
    if (*seen_count >= 4096) {
        puts("{\"record\":\"walk_capacity\",\"status\":\"NOT_OBSERVED\"}");
        return;
    }
    seen[(*seen_count)++] = id;
    const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
    if (!node) {
        puts("{\"record\":\"edge_unavailable\",\"status\":\"NOT_OBSERVED\"}");
        return;
    }
    const NLSource *source = nl_checked_source(fragment);
    const NLSemanticContext *context = nl_checked_context(fragment);
    NLValueId value = node->result_count == 1 ? node->results[0].value : 0;
    const char *role = "node_result";
    if (node->kind == NL_CHECKED_BINDING && node->initializer) {
        const NLCheckedNodeView *initializer =
            nl_checked_node_view(fragment, node->initializer);
        if (initializer && initializer->result_count == 1) {
            value = initializer->results[0].value;
            role = "binding_initializer";
        }
    }
    NLSemanticValueView value_view;
    bool observed =
        value && nl_semantic_value_view(context, value, &value_view);
    printf("{\"record\":\"node\",\"cohort\":%u,\"origin\":\"%p\",", cohort,
           (const void *)fragment);
    printf("\"context\":\"%p\",\"scope\":", (const void *)context);
    text(scope);
    printf(",\"id\":%zu,\"kind\":", id);
    text(kind_name(node->kind));
    fputs(",\"name\":", stdout);
    span_text(source, node->name);
    printf(",\"span\":[%zu,%zu],\"symbol\":%zu,\"function\":%zu,",
           node->span.start_byte, node->span.end_byte, node->symbol,
           node->function);
    printf("\"value_use\":%d,\"body_backed\":%s,", (int)node->value_use,
           node->body_backed ? "true" : "false");
    printf("\"initializer\":%zu,\"tail\":%zu,\"returned\":%zu,",
           node->initializer, node->tail, node->returned.value);
    printf("\"first_argument\":%zu,\"argument_count\":%zu,",
           node->first_argument, node->argument_count);
    printf("\"value\":%zu,\"value_role\":", value);
    text(role);
    printf(",\"observed\":%s,\"known\":%s,\"number\":%zu}\n",
           observed ? "true" : "false",
           observed && value_view.scalar_known ? "true" : "false",
           observed && value_view.scalar_known ? value_view.scalar_value : 0);
    if (node->kind == NL_CHECKED_USIZE_DIVISION ||
        node->kind == NL_CHECKED_USIZE_REMAINDER)
        arithmetic(fragment, id, node, cohort);
    if (node->kind == NL_CHECKED_REGISTERED_CALL && depth < 16) {
        const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
        if (body) {
            char callee[128];
            name_copy(source, node->name, callee);
            walk_fragment(body, cohort, callee, depth + 1);
        }
    }
    NLCheckedNodeId edges[] = {node->initializer, node->first_item,
                              node->next_item, node->tail,
                              node->first_argument, node->next_argument};
    for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i)
        walk_node(fragment, edges[i], cohort, scope, depth, seen, seen_count);
}

static void walk_fragment(const NLCheckedFragment *fragment, unsigned cohort,
                          const char *scope, unsigned depth)
{
    ++fragments;
    const NLSource *source = nl_checked_source(fragment);
    printf("{\"record\":\"fragment\",\"cohort\":%u,\"origin\":\"%p\",",
           cohort, (const void *)fragment);
    printf("\"context\":\"%p\",\"root\":%zu,\"node_count\":%zu,",
           (const void *)nl_checked_context(fragment),
           nl_checked_root(fragment),
           nl_checked_node_count(fragment));
    fputs("\"source\":", stdout);
    NLSourceSpan whole = {0, source ? nl_source_length(source) : 0};
    span_text(source, whole);
    puts("}");
    NLCheckedNodeId seen[4096];
    size_t seen_count = 0;
    walk_node(fragment, nl_checked_root(fragment), cohort, scope, depth, seen,
              &seen_count);
}

static void challenges(void)
{
    const Event *local = NULL, *foreign = NULL;
    for (size_t i = 0; i < event_count; ++i) {
        if (events[i].division && events[i].cohort == 0 && !local)
            local = &events[i];
        if (events[i].division && events[i].cohort == 2 && !foreign)
            foreign = &events[i];
    }
    if (local && foreign) {
        bool accepted = nl_checked_usize_division_valid(
            local->fragment, local->node, &foreign->div);
        printf("{\"record\":\"foreign_context\",\"accepted\":%s}\n",
               accepted ? "true" : "false");
        if (accepted)
            ++errors;
    } else {
        puts("{\"record\":\"foreign_context\",\"status\":\"NOT_OBSERVED\"}");
    }
    bool challenged = false;
    for (size_t i = 0; i < event_count && !challenged; ++i) {
        for (size_t j = i + 1; j < event_count && !challenged; ++j) {
            Event *a = &events[i], *b = &events[j];
            if (!a->division || !b->division || a->cohort != 0 ||
                b->cohort != 0 || !a->div.quotient_known ||
                !b->div.quotient_known ||
                a->div.quotient != b->div.quotient ||
                a->fragment == b->fragment)
                continue;
            bool accepted = nl_checked_usize_division_valid(
                a->fragment, a->node, &b->div);
            printf("{\"record\":\"same_value\",\"quantity\":%zu,",
                   a->div.quotient);
            printf("\"accepted\":%s}\n", accepted ? "true" : "false");
            if (accepted)
                ++errors;
            challenged = true;
        }
    }
    if (!challenged)
        puts("{\"record\":\"same_value\",\"status\":\"NOT_OBSERVED\"}");
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Unit units[3] = {{0}};
    bool ready = true;
    printf("{\"record\":\"host_target\",\"size_max\":%zu,\"size_bytes\":%zu}\n",
           SIZE_MAX, sizeof(size_t));
    for (unsigned i = 0; i < 3; ++i) {
        const char *path = i == 1 ? argv[2] : argv[1];
        if (!load(&units[i], path, i)) {
            ready = false;
            break;
        }
        walk_fragment(units[i].checked, i, "driver", 0);
    }
    if (ready)
        challenges();
    printf("{\"record\":\"summary\",\"ready\":%s,\"errors\":%u,",
           ready ? "true" : "false", errors);
    printf("\"events\":%zu,\"fragments\":%zu}\n", event_count, fragments);
    for (size_t i = 0; i < 3; ++i)
        cleanup(&units[i]);
    return ready && !errors ? 0 : 1;
}
