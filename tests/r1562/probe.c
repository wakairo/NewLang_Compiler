#include "newlang/checked.h"
#include "newlang/copy_body.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/syntax.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *label;
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *tree, *entry_tree;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
} Unit;

typedef struct {
    const char *unit;
    size_t call_line;
    bool remainder;
    const NLCheckedFragment *body;
    NLCheckedNodeId node;
    NLUsizeDivisionView division;
    NLUsizeRemainderView residue;
} Event;

static Event events[16];
static size_t event_count;
static bool failed;

static void string(const char *s)
{
    putchar('"');
    for (const unsigned char *p = (const unsigned char *)(s ? s : ""); *p != 0;
         ++p) {
        if (*p == '"' || *p == '\\')
            printf("\\%c", *p);
        else if (*p < 32)
            printf("\\u%04x", (unsigned)*p);
        else
            putchar(*p);
    }
    putchar('"');
}

static size_t line_of(const NLSource *source, NLSourceSpan span)
{
    NLSourceView view;
    if (!source ||
        !nl_source_view(source, (NLSourceSpan){0, span.start_byte}, &view))
        return 0;
    size_t line = 1;
    for (size_t i = 0; i < view.length; ++i)
        if (view.bytes[i] == '\n')
            ++line;
    return line;
}

static void name_of(const NLSource *source, NLSourceSpan span, char *out,
                    size_t capacity)
{
    NLSourceView view;
    out[0] = 0;
    if (!nl_source_view(source, span, &view) || view.length >= capacity)
        return;
    memcpy(out, view.bytes, view.length);
    out[view.length] = 0;
}

static void value(const NLSemanticContext *context, NLValueId id)
{
    NLSemanticValueView v = {0};
    bool present = id != 0 && nl_semantic_value_view(context, id, &v);
    printf("{\"id\":%zu,\"present\":%s,\"type\":%zu,\"known\":%s,"
           "\"number\":%zu}",
           id, present ? "true" : "false", present ? v.type : 0,
           present && v.scalar_known ? "true" : "false",
           present ? v.scalar_value : 0);
}

static void prefix(const char *kind, const char *unit)
{
    printf("{\"kind\":");
    string(kind);
    printf(",\"unit\":");
    string(unit);
}

static void diagnostic(const char *kind, const char *unit, int status,
                       const NLSource *source, const NLCheckDiagnostic *d)
{
    prefix(kind, unit);
    printf(",\"status\":%d,\"line\":%zu,\"start\":%zu,\"end\":%zu,"
           "\"code\":",
           status, line_of(source, d->span), d->span.start_byte,
           d->span.end_byte);
    string(d->diagnostic.code);
    printf(",\"message\":");
    string(d->diagnostic.message);
    puts("}");
}

static bool open_unit(Unit *u, const char *path, bool positive)
{
    if (nl_source_load(path, &u->source) != NL_SOURCE_OK ||
        nl_parser_create(u->source, &u->parser) != NL_PARSE_OK) {
        prefix("APPARATUS_ERROR", u->label);
        puts(",\"message\":\"source/parser allocation\"}");
        return false;
    }
    NLParseDiagnostic pd = {0};
    NLParseStatus parsed =
        nl_parser_parse_function_unit(u->parser, &u->tree, &pd);
    prefix("PARSE", u->label);
    printf(",\"status\":%d,\"line\":%zu,\"code\":", (int)parsed,
           line_of(u->source, pd.span));
    string(pd.diagnostic.code);
    printf(",\"message\":");
    string(pd.diagnostic.message);
    puts("}");
    if (parsed != NL_PARSE_OK)
        return false;
    if (nl_semantic_create(&u->context) != NL_CHECK_OK)
        return false;
    const NLSyntaxTree *inputs[] = {u->tree};
    NLFunctionUnitDiagnostic fd = {0};
    NLCheckStatus registered =
        nl_semantic_register_function_unit(u->context, inputs, 1, &fd);
    diagnostic("REGISTER", u->label, (int)registered, u->source,
               &fd.diagnostic);
    if (!positive)
        return registered == NL_CHECK_SEMANTIC_ERROR;
    if (registered != NL_CHECK_OK)
        return false;
    static const char call[] = "main()";
    if (nl_source_create(call, sizeof(call) - 1, "<R1562-main-entry>",
                         &u->entry_source) != NL_SOURCE_OK ||
        nl_parser_create(u->entry_source, &u->entry_parser) != NL_PARSE_OK)
        return false;
    parsed = nl_parser_parse_expression_fragment(u->entry_parser,
                                                 &u->entry_tree, &pd);
    if (parsed != NL_PARSE_OK)
        return false;
    NLCheckDiagnostic cd = {0};
    NLCheckStatus checked =
        nl_semantic_check_expression(u->context, u->entry_tree, &u->entry, &cd);
    diagnostic("ENTRY", u->label, (int)checked, u->entry_source, &cd);
    return checked == NL_CHECK_OK;
}

static void close_unit(Unit *u)
{
    nl_checked_destroy(u->entry);
    nl_syntax_tree_destroy(u->entry_tree);
    nl_parser_destroy(u->entry_parser);
    nl_source_destroy(u->entry_source);
    nl_semantic_destroy(u->context);
    nl_syntax_tree_destroy(u->tree);
    nl_parser_destroy(u->parser);
    nl_source_destroy(u->source);
}

static bool save_source(const NLCheckedFragment *body, const char *directory,
                        const char *label)
{
    const NLSource *source = nl_checked_source(body);
    NLSourceView view;
    char path[1024];
    int n = snprintf(path, sizeof(path), "%s/public-source-%s.nl", directory,
                     label);
    if (!source || n < 0 || (size_t)n >= sizeof(path) ||
        !nl_source_view(source, (NLSourceSpan){0, nl_source_length(source)},
                        &view))
        return false;
    FILE *file = fopen(path, "wb");
    if (!file)
        return false;
    bool ok = fwrite(view.bytes, 1, view.length, file) == view.length;
    if (fclose(file) != 0)
        ok = false;
    return ok;
}

static void arithmetic(const char *unit, const char *owner, size_t call_line,
                       const NLCheckedFragment *body, NLCheckedNodeId id,
                       const NLCheckedNodeView *node)
{
    if (event_count == sizeof(events) / sizeof(events[0])) {
        failed = true;
        return;
    }
    Event *e = &events[event_count++];
    e->unit = unit;
    e->call_line = call_line;
    e->body = body;
    e->node = id;
    e->remainder = node->kind == NL_CHECKED_USIZE_REMAINDER;
    bool observed, genuine, detached;
    size_t a, b, result, maximum;
    bool a_known, result_known;
    if (e->remainder) {
        observed = nl_checked_usize_remainder_view(body, id, &e->residue);
        genuine = nl_checked_usize_remainder_valid(body, id, NULL);
        detached =
            observed && nl_checked_usize_remainder_valid(body, id, &e->residue);
        a = e->residue.numbers[0];
        b = e->residue.numbers[1];
        result = e->residue.remainder;
        maximum = e->residue.target_max;
        a_known = e->residue.numerator_known;
        result_known = e->residue.remainder_known;
    } else {
        observed = nl_checked_usize_division_view(body, id, &e->division);
        genuine = nl_checked_usize_division_valid(body, id, NULL);
        detached =
            observed && nl_checked_usize_division_valid(body, id, &e->division);
        a = e->division.numbers[0];
        b = e->division.numbers[1];
        result = e->division.quotient;
        maximum = e->division.target_max;
        a_known = e->division.numerator_known;
        result_known = e->division.quotient_known;
    }
    prefix("ARITH", unit);
    printf(",\"owner\":");
    string(owner);
    printf(",\"call_line\":%zu,\"node\":%zu,\"line\":%zu,\"op\":", call_line,
           id, line_of(nl_checked_source(body), node->span));
    string(e->remainder ? "%" : "/");
    printf(",\"body\":\"%p\",\"context\":\"%p\",\"observed\":%s,"
           "\"genuine\":%s,\"detached\":%s,\"numerator_known\":%s,"
           "\"numerator\":%zu,\"divisor\":%zu,\"result_known\":%s,"
           "\"result\":%zu,\"target_max\":%zu",
           (const void *)body, (const void *)nl_checked_context(body),
           observed ? "true" : "false", genuine ? "true" : "false",
           detached ? "true" : "false", a_known ? "true" : "false", a, b,
           result_known ? "true" : "false", result, maximum);
    const NLCheckedNodeId *operands =
        e->remainder ? e->residue.operands : e->division.operands;
    const NLValueId *values =
        e->remainder ? e->residue.values : e->division.values;
    const NLSymbolId *symbols =
        e->remainder ? e->residue.symbols : e->division.symbols;
    const NLPlaceId *places =
        e->remainder ? e->residue.places : e->division.places;
    const NLIncarnationId *incarnations =
        e->remainder ? e->residue.incarnations : e->division.incarnations;
    const NLValueFactId *facts =
        e->remainder ? e->residue.facts : e->division.facts;
    NLSourceSpan operator_span =
        e->remainder ? e->residue.operator_span : e->division.operator_span;
    const NLCheckedFragment *origin =
        e->remainder ? e->residue.origin : e->division.origin;
    const NLSource *receipt_source =
        e->remainder ? e->residue.source : nl_checked_source(body);
    NLValueId result_id = e->remainder ? e->residue.result : e->division.result;
    printf(",\"origin\":\"%p\",\"source\":\"%p\",\"operands\":[%zu,%zu],"
           "\"values\":[%zu,%zu],\"result_id\":%zu,"
           "\"symbols\":[%zu,%zu],\"places\":[%zu,%zu],"
           "\"incarnations\":[%zu,%zu],\"facts\":[%zu,%zu],"
           "\"operator_span\":[%zu,%zu]}\n",
           (const void *)origin, (const void *)receipt_source, operands[0],
           operands[1], values[0], values[1], result_id, symbols[0], symbols[1],
           places[0], places[1], incarnations[0], incarnations[1], facts[0],
           facts[1], operator_span.start_byte, operator_span.end_byte);
    if (!observed || !genuine || !detached)
        failed = true;
}

static void walk(const char *unit, const NLCheckedFragment *body,
                 const char *owner, size_t parent_line, const char *label,
                 const char *directory, unsigned depth)
{
    if (depth > 3 || !body || !save_source(body, directory, label)) {
        failed = true;
        return;
    }
    const NLSource *source = nl_checked_source(body);
    const NLSemanticContext *context = nl_checked_context(body);
    size_t count = nl_checked_node_count(body);
    prefix("BODY", unit);
    printf(",\"label\":");
    string(label);
    printf(",\"owner\":");
    string(owner);
    printf(",\"body\":\"%p\",\"context\":\"%p\",\"source\":\"%p\","
           "\"root\":%zu,\"nodes\":%zu}\n",
           (const void *)body, (const void *)context, (const void *)source,
           nl_checked_root(body), count);
    for (size_t id = 1; id <= count && !failed; ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, id);
        if (!n) {
            failed = true;
            break;
        }
        char name[128];
        name_of(source, n->name, name, sizeof(name));
        prefix("NODE", unit);
        printf(",\"label\":");
        string(label);
        printf(",\"owner\":");
        string(owner);
        printf(",\"id\":%zu,\"tag\":%d,\"line\":%zu,\"start\":%zu,"
               "\"end\":%zu,\"name\":",
               id, (int)n->kind, line_of(source, n->span), n->span.start_byte,
               n->span.end_byte);
        string(name);
        printf(",\"symbol\":%zu,\"function\":%zu,\"use\":%d,"
               "\"initializer\":%zu,\"first_argument\":%zu,"
               "\"next_argument\":%zu,\"arguments\":%zu,"
               "\"result_count\":%zu,\"result\":",
               n->symbol, n->function, (int)n->value_use, n->initializer,
               n->first_argument, n->next_argument, n->argument_count,
               n->result_count);
        value(context, n->result_count ? n->results[0].value : 0);
        printf(",\"returned\":");
        value(context, n->returned.value);
        puts("}");
        if (n->kind == NL_CHECKED_BINDING) {
            NLSemanticBindingView bv = {0};
            bool got = nl_semantic_binding_view(context, n->symbol, &bv);
            const NLCheckedNodeView *init =
                nl_checked_node_view(body, n->initializer);
            prefix("BIND", unit);
            printf(",\"owner\":");
            string(owner);
            printf(",\"call_line\":%zu,\"name\":", parent_line);
            string(name);
            printf(",\"symbol\":%zu,\"binding_present\":%s,\"binding\":",
                   n->symbol, got ? "true" : "false");
            value(context, got ? bv.value : 0);
            printf(",\"initializer_result\":");
            value(context,
                  init && init->result_count ? init->results[0].value : 0);
            puts("}");
        }
        if (n->kind == NL_CHECKED_USIZE_DIVISION ||
            n->kind == NL_CHECKED_USIZE_REMAINDER)
            arithmetic(unit, owner, parent_line, body, id, n);
        if (n->kind == NL_CHECKED_REGISTERED_CALL && n->body_backed) {
            const NLCheckedFragment *callee = nl_checked_call_body(body, id);
            size_t call_line = line_of(source, n->span);
            prefix("CALL", unit);
            printf(",\"caller\":");
            string(owner);
            printf(",\"name\":");
            string(name);
            printf(",\"line\":%zu,\"node\":%zu,\"function\":%zu,"
                   "\"body\":\"%p\",\"caller_context\":\"%p\","
                   "\"callee_context\":\"%p\",\"result\":",
                   call_line, id, n->function, (const void *)callee,
                   (const void *)context,
                   (const void *)(callee ? nl_checked_context(callee) : NULL));
            value(context, n->result_count ? n->results[0].value : 0);
            puts("}");
            NLCopyBodyView view = {0};
            bool got = nl_checked_copy_body_view(body, id, &view);
            int actual =
                got ? (int)nl_checked_copy_body_validate(body, id, NULL) : -1;
            int copied =
                got ? (int)nl_checked_copy_body_validate(body, id, &view) : -1;
            prefix("COPY_BODY_SURFACE", unit);
            printf(",\"line\":%zu,\"observed\":%s,\"actual_status\":%d,"
                   "\"detached_status\":%d}\n",
                   call_line, got ? "true" : "false", actual, copied);
            if (got && (actual != NL_CHECK_OK || copied != NL_CHECK_OK))
                failed = true;
            char child[256];
            int written = snprintf(child, sizeof(child), "%s-n%zu", label, id);
            if (written < 0 || (size_t)written >= sizeof(child)) {
                failed = true;
                break;
            }
            walk(unit, callee, name, call_line, child, directory, depth + 1);
        }
    }
}

static Event *find_event(const char *unit, size_t line)
{
    for (size_t i = 0; i < event_count; ++i)
        if (!strcmp(events[i].unit, unit) && events[i].call_line == line)
            return &events[i];
    return NULL;
}

static void challenge(const char *label, Event *target, Event *foreign,
                      unsigned mode)
{
    bool accepted = true;
    if (!target || !foreign || target->remainder != foreign->remainder) {
        prefix("APPARATUS_ERROR", "AUTH");
        printf(",\"message\":");
        string(label);
        puts("}");
        failed = true;
        return;
    }
    if (target->remainder) {
        NLUsizeRemainderView candidate = foreign->residue;
        if (mode != 0) {
            candidate = target->residue;
            if (mode == 1)
                candidate.origin = foreign->body;
            else
                candidate.source = foreign->residue.source;
        }
        accepted = nl_checked_usize_remainder_valid(target->body, target->node,
                                                    &candidate);
    } else {
        NLUsizeDivisionView candidate = foreign->division;
        if (mode == 1) {
            candidate = target->division;
            candidate.origin = foreign->body;
        }
        accepted = nl_checked_usize_division_valid(target->body, target->node,
                                                   &candidate);
    }
    prefix("AUTH", target->unit);
    printf(",\"challenge\":");
    string(label);
    printf(",\"accepted\":%s,\"target_call_line\":%zu,"
           "\"foreign_unit\":",
           accepted ? "true" : "false", target->call_line);
    string(foreign->unit);
    printf(",\"foreign_call_line\":%zu}\n", foreign->call_line);
    if (accepted)
        failed = true;
}

int main(int argc, char **argv)
{
    if (argc != 8) {
        fputs("usage: r1562_probe OUT P1 P2 N1 N2 N3 N4\n", stderr);
        return 2;
    }
    printf("{\"kind\":\"HOST\",\"size_t_bytes\":%zu,\"size_max\":%zu}\n",
           sizeof(size_t), (size_t)SIZE_MAX);
    Unit units[7] = {{.label = "P1"}, {.label = "P2"}, {.label = "P2DUP"},
                     {.label = "N1"}, {.label = "N2"}, {.label = "N3"},
                     {.label = "N4"}};
    const char *paths[] = {argv[2], argv[3], argv[3], argv[4],
                           argv[5], argv[6], argv[7]};
    for (size_t i = 0; i < 7 && !failed; ++i) {
        if (!open_unit(&units[i], paths[i], i < 3)) {
            failed = true;
            break;
        }
        if (i < 3)
            walk(units[i].label, units[i].entry, "<entry>", 0, units[i].label,
                 argv[1], 0);
    }
    if (!failed) {
        challenge("P1_same_value_repeated_call", find_event("P1", 5),
                  find_event("P1", 6), 0);
        challenge("P1_different_leaf", find_event("P1", 3), find_event("P1", 5),
                  0);
        challenge("P2_equal_remainders_distinct_actual", find_event("P2", 10),
                  find_event("P2", 11), 0);
        challenge("P2_other_actual_origin", find_event("P2", 10),
                  find_event("P2", 11), 1);
        challenge("P2_identical_bytes_foreign_context", find_event("P2", 10),
                  find_event("P2DUP", 10), 0);
        challenge("P2_foreign_source_only", find_event("P2", 10),
                  find_event("P2DUP", 10), 2);
    }
    for (size_t i = 7; i > 0; --i)
        close_unit(&units[i - 1]);
    printf("{\"kind\":\"END\",\"failed\":%s}\n", failed ? "true" : "false");
    return failed ? 1 : 0;
}
