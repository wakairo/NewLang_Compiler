#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SOURCES 32
#define MAX_FRAGMENTS 32
#define MAX_NODES 512
#define MAX_DEPTH 12
#define MAX_BYTES 4096
#define MAX_DECLS 8

static const NLCheckedFragment *fragments[MAX_FRAGMENTS];
static const NLSemanticContext *worlds[MAX_FRAGMENTS];
static const NLSource *sources[MAX_SOURCES];
static size_t fragment_count, world_count, source_count, total_nodes;
static const NLSource *original_source, *adapter_source;
static NLSourceSpan body_spans[MAX_DECLS];
static size_t body_count;
static const NLCheckedFragment *entry_fragment;
static int fatal_problem, limited;
static const char *jb(bool v)
{
    return v ? "true" : "false";
}

static void problem(const char *why, bool fatal)
{
    printf("{\"record\":\"problem\",\"reason\":\"%s\",\"fatal\":%s}\n", why,
           jb(fatal));
    if (fatal)
        fatal_problem = 1;
    else
        limited = 1;
}

static void hex(const unsigned char *p, size_t n)
{
    putchar('"');
    for (size_t i = 0; i < n; ++i)
        printf("%02x", (unsigned)p[i]);
    putchar('"');
}

static void text_hex(const char *p)
{
    if (p == NULL) {
        fputs("null", stdout);
        return;
    }
    size_t n = 0;
    while (n < MAX_BYTES && p[n] != '\0')
        ++n;
    if (n == MAX_BYTES) {
        fputs("null", stdout);
        limited = 1;
        return;
    }
    hex((const unsigned char *)p, n);
}

static bool view_bytes(const NLSource *s, NLSourceSpan span, NLSourceView *out)
{
    return s != NULL && nl_source_span_valid(s, span) &&
           nl_source_view(s, span, out) && out->length <= MAX_BYTES;
}

static size_t world_id(const NLSemanticContext *w)
{
    if (w == NULL) {
        problem("missing_context", false);
        return 0;
    }
    for (size_t i = 0; i < world_count; ++i)
        if (worlds[i] == w)
            return i + 1;
    if (world_count == MAX_FRAGMENTS) {
        problem("context_limit", false);
        return 0;
    }
    worlds[world_count++] = w;
    return world_count;
}

static size_t source_id(const NLSource *s)
{
    if (s == NULL) {
        problem("missing_source", false);
        return 0;
    }
    for (size_t i = 0; i < source_count; ++i)
        if (sources[i] == s)
            return i + 1;
    if (source_count == MAX_SOURCES) {
        problem("source_limit", false);
        return 0;
    }
    size_t id = ++source_count;
    sources[id - 1] = s;
    NLSourceView v = {0}, original = {0};
    NLSourceSpan whole = {0, nl_source_length(s)};
    if (!view_bytes(s, whole, &v)) {
        problem("source_byte_limit_or_invalid_view", false);
        return id;
    }
    bool same_full =
        view_bytes(original_source,
                   (NLSourceSpan){0, nl_source_length(original_source)},
                   &original) &&
        v.length == original.length &&
        (v.length == 0 || memcmp(v.bytes, original.bytes, v.length) == 0);
    size_t body_matches = 0;
    for (size_t i = 0; i < body_count; ++i) {
        NLSourceView b = {0};
        if (view_bytes(original_source, body_spans[i], &b) &&
            b.length == v.length &&
            (v.length == 0 || memcmp(b.bytes, v.bytes, v.length) == 0))
            ++body_matches;
    }
    printf("{\"record\":\"source\",\"source\":%zu,\"original_owner\":%s,"
           "\"adapter_owner\":%s,\"matches_full_original\":%s,"
           "\"matching_original_body_count\":%zu,\"bytes\":%zu,\"name_hex\":",
           id, jb(s == original_source), jb(s == adapter_source), jb(same_full),
           body_matches, v.length);
    text_hex(nl_source_name(s));
    fputs(",\"bytes_hex\":", stdout);
    hex(v.bytes, v.length);
    puts("}");
    if (s != adapter_source && !same_full && body_matches != 1)
        problem("retained_source_text_unreconciled", false);
    return id;
}

static void value_record(const NLSemanticContext *w, size_t fid,
                         NLCheckedNodeId node, NLValueId value)
{
    if (value == 0)
        return;
    NLSemanticValueView v = {0};
    bool ok = nl_semantic_value_view(w, value, &v);
    printf("{\"record\":\"value\",\"fragment\":%zu,\"node\":%zu,"
           "\"world\":%zu,\"value\":%zu,\"view\":%s",
           fid, node, world_id(w), value, jb(ok));
    if (ok) {
        printf(",\"type\":%zu,\"scalar_known\":%s,\"scalar_value\":", v.type,
               jb(v.scalar_known));
        if (v.scalar_known)
            printf("%zu", v.scalar_value);
        else
            fputs("null", stdout);
        printf(",\"carrier\":%u,\"dependencies\":%u", (unsigned)v.carrier,
               (unsigned)v.dependencies);
    }
    puts("}");
    if (!ok)
        problem("value_view_unavailable", false);
}

static void division_record(const NLCheckedFragment *f, size_t fid,
                            NLCheckedNodeId id)
{
    NLUsizeDivisionView v = {0};
    if (!nl_checked_usize_division_view(f, id, &v)) {
        printf("{\"record\":\"division\",\"fragment\":%zu,\"node\":%zu,"
               "\"view\":false}\n",
               fid, id);
        limited = 1;
        return;
    }
    bool actual = nl_checked_usize_division_valid(f, id, NULL);
    bool copied = nl_checked_usize_division_valid(f, id, &v);
    NLUsizeDivisionView wrong = v;
    wrong.numbers[1] = 0;
    bool wrong_zero = nl_checked_usize_division_valid(f, id, &wrong);
    bool foreign_checked = entry_fragment != NULL && entry_fragment != f;
    bool foreign_accepted = false;
    if (foreign_checked) {
        wrong = v;
        wrong.origin = entry_fragment;
        foreign_accepted = nl_checked_usize_division_valid(f, id, &wrong);
    }
    printf("{\"record\":\"division\",\"fragment\":%zu,\"node\":%zu,"
           "\"view\":true,\"origin_same\":%s,\"event_valid\":%s,"
           "\"copy_valid\":%s,\"wrong_zero_accepted\":%s,"
           "\"foreign_origin_checked\":%s,\"foreign_origin_accepted\":%s,"
           "\"type\":%zu,\"operands\":[%zu,%zu],\"values\":[%zu,%zu],"
           "\"result\":%zu,\"symbols\":[%zu,%zu],\"places\":[%zu,%zu],"
           "\"incarnations\":[%zu,%zu],\"facts\":[%zu,%zu],"
           "\"span\":[%zu,%zu],\"operator_span\":[%zu,%zu],"
           "\"operand_spans\":[[%zu,%zu],[%zu,%zu]],"
           "\"numerator_known\":%s,\"result_known\":%s,"
           "\"numbers\":[%zu,%zu],\"quantity\":%zu,\"target_max\":%zu}\n",
           fid, id, jb(v.origin == f), jb(actual), jb(copied), jb(wrong_zero),
           jb(foreign_checked), jb(foreign_accepted), v.type, v.operands[0],
           v.operands[1], v.values[0], v.values[1], v.result, v.symbols[0],
           v.symbols[1], v.places[0], v.places[1], v.incarnations[0],
           v.incarnations[1], v.facts[0], v.facts[1], v.span.start_byte,
           v.span.end_byte, v.operator_span.start_byte,
           v.operator_span.end_byte, v.operand_spans[0].start_byte,
           v.operand_spans[0].end_byte, v.operand_spans[1].start_byte,
           v.operand_spans[1].end_byte, jb(v.numerator_known),
           jb(v.quotient_known), v.numbers[0], v.numbers[1], v.quotient,
           v.target_max);
    if (v.origin != f || !actual || !copied || wrong_zero || foreign_accepted)
        problem("division_authenticity_contradiction", true);
}

static void remainder_record(const NLCheckedFragment *f, size_t fid,
                             NLCheckedNodeId id)
{
    NLUsizeRemainderView v = {0};
    if (!nl_checked_usize_remainder_view(f, id, &v)) {
        printf("{\"record\":\"remainder\",\"fragment\":%zu,\"node\":%zu,"
               "\"view\":false}\n",
               fid, id);
        limited = 1;
        return;
    }
    bool actual = nl_checked_usize_remainder_valid(f, id, NULL);
    bool copied = nl_checked_usize_remainder_valid(f, id, &v);
    NLUsizeRemainderView wrong = v;
    wrong.numbers[1] = 0;
    bool wrong_zero = nl_checked_usize_remainder_valid(f, id, &wrong);
    bool foreign_checked = adapter_source != NULL && adapter_source != v.source;
    bool foreign_accepted = false;
    if (foreign_checked) {
        wrong = v;
        wrong.source = adapter_source;
        foreign_accepted = nl_checked_usize_remainder_valid(f, id, &wrong);
    }
    bool source_same = v.source == nl_checked_source(f);
    printf("{\"record\":\"remainder\",\"fragment\":%zu,\"node\":%zu,"
           "\"view\":true,\"origin_same\":%s,\"source_same\":%s,"
           "\"event_valid\":%s,\"copy_valid\":%s,\"wrong_zero_accepted\":%s,"
           "\"foreign_source_checked\":%s,\"foreign_source_accepted\":%s,"
           "\"type\":%zu,\"operands\":[%zu,%zu],\"values\":[%zu,%zu],"
           "\"result\":%zu,\"symbols\":[%zu,%zu],\"places\":[%zu,%zu],"
           "\"incarnations\":[%zu,%zu],\"facts\":[%zu,%zu],"
           "\"span\":[%zu,%zu],\"operator_span\":[%zu,%zu],"
           "\"operand_spans\":[[%zu,%zu],[%zu,%zu]],"
           "\"numerator_known\":%s,\"result_known\":%s,"
           "\"numbers\":[%zu,%zu],\"quantity\":%zu,\"target_max\":%zu}\n",
           fid, id, jb(v.origin == f), jb(source_same), jb(actual), jb(copied),
           jb(wrong_zero), jb(foreign_checked), jb(foreign_accepted), v.type,
           v.operands[0], v.operands[1], v.values[0], v.values[1], v.result,
           v.symbols[0], v.symbols[1], v.places[0], v.places[1],
           v.incarnations[0], v.incarnations[1], v.facts[0], v.facts[1],
           v.span.start_byte, v.span.end_byte, v.operator_span.start_byte,
           v.operator_span.end_byte, v.operand_spans[0].start_byte,
           v.operand_spans[0].end_byte, v.operand_spans[1].start_byte,
           v.operand_spans[1].end_byte, jb(v.numerator_known),
           jb(v.remainder_known), v.numbers[0], v.numbers[1], v.remainder,
           v.target_max);
    if (v.origin != f || !source_same || !actual || !copied || wrong_zero ||
        foreign_accepted)
        problem("remainder_authenticity_contradiction", true);
}

static void enqueue(NLCheckedNodeId *ids, size_t *count, NLCheckedNodeId id)
{
    if (id == 0)
        return;
    for (size_t i = 0; i < *count; ++i)
        if (ids[i] == id)
            return;
    if (*count == MAX_NODES) {
        problem("node_queue_limit", false);
        return;
    }
    ids[(*count)++] = id;
}

static void inspect(const NLCheckedFragment *f, size_t parent,
                    NLCheckedNodeId via, const char *edge, size_t depth)
{
    if (f == NULL) {
        problem("missing_borrowed_fragment", false);
        return;
    }
    for (size_t i = 0; i < fragment_count; ++i) {
        if (fragments[i] == f) {
            printf("{\"record\":\"fragment_repeat\",\"fragment\":%zu,"
                   "\"parent\":%zu,\"via\":%zu,\"edge\":\"%s\"}\n",
                   i + 1, parent, via, edge);
            return;
        }
    }
    if (depth > MAX_DEPTH || fragment_count == MAX_FRAGMENTS) {
        problem("fragment_depth_or_count_limit", false);
        return;
    }
    size_t fid = ++fragment_count;
    fragments[fid - 1] = f;
    const NLSemanticContext *w = nl_checked_context(f);
    const NLSource *s = nl_checked_source(f);
    size_t wid = world_id(w), sid = source_id(s);
    size_t count = nl_checked_node_count(f);
    printf("{\"record\":\"fragment\",\"fragment\":%zu,\"world\":%zu,"
           "\"source\":%zu,\"parent\":%zu,\"via\":%zu,\"edge\":\"%s\","
           "\"reported_nodes\":%zu,\"root\":%zu}\n",
           fid, wid, sid, parent, via, edge, count, nl_checked_root(f));
    if (w == NULL || s == NULL || count > MAX_NODES) {
        problem("fragment_observation_boundary", false);
        return;
    }
    NLCheckedNodeId queue[MAX_NODES] = {0};
    size_t queued = 0;
    enqueue(queue, &queued, nl_checked_root(f));
    for (size_t at = 0; at < queued && !fatal_problem; ++at) {
        if (++total_nodes > 2048) {
            problem("total_node_limit", false);
            return;
        }
        NLCheckedNodeId id = queue[at];
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (v == NULL) {
            problem("missing_reachable_node", false);
            continue;
        }
        printf("{\"record\":\"node\",\"fragment\":%zu,\"world\":%zu,"
               "\"source\":%zu,\"node\":%zu,\"kind\":%u,"
               "\"span\":[%zu,%zu],\"name_span\":[%zu,%zu],"
               "\"type\":%zu,\"symbol\":%zu,\"function\":%zu,"
               "\"body_backed\":%s,\"value_use\":%u,\"normal_arms\":%zu,"
               "\"terminates\":%s,\"arguments\":%zu,\"items\":%zu,"
               "\"edges\":[%zu,%zu,%zu,%zu,%zu,%zu],"
               "\"result_count\":%zu,\"results\":[[%zu,%zu],[%zu,%zu]],"
               "\"has_scalar_result\":%s}\n",
               fid, wid, sid, id, (unsigned)v->kind, v->span.start_byte,
               v->span.end_byte, v->name.start_byte, v->name.end_byte, v->type,
               v->symbol, v->function, jb(v->body_backed),
               (unsigned)v->value_use, v->normal_arms, jb(v->terminates),
               v->argument_count, v->item_count, v->first_argument,
               v->next_argument, v->initializer, v->first_item, v->next_item,
               v->tail, v->result_count,
               v->result_count > 0 ? v->results[0].type : 0,
               v->result_count > 0 ? v->results[0].value : 0,
               v->result_count > 1 ? v->results[1].type : 0,
               v->result_count > 1 ? v->results[1].value : 0,
               jb(v->has_scalar_result));
        if (v->has_scalar_result) {
            printf("{\"record\":\"node_scalar\",\"fragment\":%zu,"
                   "\"node\":%zu,\"known\":%s,\"value\":",
                   fid, id, jb(v->scalar_result.known));
            if (v->scalar_result.known)
                printf("%zu", v->scalar_result.value);
            else
                fputs("null", stdout);
            puts("}");
        }
        if (!nl_source_span_valid(s, v->span) || v->result_count > 2)
            problem("node_span_or_result_count_invalid", true);
        for (size_t k = 0; k < v->result_count && k < 2; ++k)
            value_record(w, fid, id, v->results[k].value);
        if (v->kind == NL_CHECKED_USIZE_DIVISION)
            division_record(f, fid, id);
        if (v->kind == NL_CHECKED_USIZE_REMAINDER)
            remainder_record(f, fid, id);
        if (v->kind == NL_CHECKED_IF) {
            for (size_t k = 0; k < 2; ++k) {
                const NLCheckedFragment *arm = nl_checked_if_arm(f, id, k);
                if (arm != NULL && nl_checked_context(arm) == w)
                    problem("if_arm_context_not_separate", true);
                inspect(arm, fid, id,
                        k == 0 ? "then_hypothetical" : "else_hypothetical",
                        depth + 1);
            }
        }
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(f, id);
            if (!v->body_backed || body == NULL)
                problem("call_body_unavailable", false);
            else {
                if (nl_checked_context(body) != w)
                    problem("call_body_context_mismatch", true);
                inspect(body, fid, id, "call_body_observation", depth + 1);
            }
        }
        enqueue(queue, &queued, v->first_argument);
        enqueue(queue, &queued, v->next_argument);
        enqueue(queue, &queued, v->initializer);
        enqueue(queue, &queued, v->first_item);
        enqueue(queue, &queued, v->next_item);
        enqueue(queue, &queued, v->tail);
    }
}

static bool declarations(const NLSyntaxTree *tree)
{
    const NLSyntaxView *unit = nl_syntax_node_view(nl_syntax_tree_root(tree));
    if (unit == NULL || unit->kind != NL_SYNTAX_FUNCTION_UNIT ||
        unit->data.function_unit.count > MAX_DECLS)
        return false;
    const NLSyntaxNode *p = unit->data.function_unit.declarations;
    size_t seen = 0;
    while (p != NULL && seen < MAX_DECLS) {
        const NLSyntaxView *f = nl_syntax_node_view(p);
        if (f == NULL || f->kind != NL_SYNTAX_FUNCTION)
            return false;
        const NLSyntaxView *b = nl_syntax_node_view(f->data.function.body);
        NLSourceView name = {0};
        if (b == NULL || b->kind != NL_SYNTAX_BLOCK ||
            !view_bytes(original_source, f->data.function.name, &name) ||
            !nl_source_span_valid(original_source, b->span))
            return false;
        body_spans[body_count++] = b->span;
        fputs("{\"record\":\"declaration\",\"name_hex\":", stdout);
        hex(name.bytes, name.length);
        printf(",\"body_span\":[%zu,%zu]}\n", b->span.start_byte,
               b->span.end_byte);
        ++seen;
        p = nl_syntax_next_argument(p);
    }
    return p == NULL && seen == unit->data.function_unit.count;
}

static void diagnostic_record(const char *stage, const NLSource *source,
                              const NLDiagnostic *d, NLSourceSpan span,
                              size_t input, bool rendered)
{
    printf("{\"record\":\"diagnostic\",\"stage\":\"%s\","
           "\"input_index\":%zu,\"span\":[%zu,%zu],\"span_valid\":%s,"
           "\"severity\":%u,\"rendered\":%s,\"category_hex\":",
           stage, input, span.start_byte, span.end_byte,
           jb(nl_source_span_valid(source, span)), (unsigned)d->severity,
           jb(rendered));
    text_hex(d->category);
    fputs(",\"code_hex\":", stdout);
    text_hex(d->code);
    fputs(",\"message_hex\":", stdout);
    text_hex(d->message);
    puts("}");
}

int main(int argc, char **argv)
{
    if (argc != 5)
        return 64;
    bool invalid = strcmp(argv[4], "invalid") == 0;
    if (!invalid && strcmp(argv[4], "lawful") != 0)
        return 64;
    setvbuf(stdout, NULL, _IOLBF, 0);
    NLSource *source = NULL, *adapter = NULL;
    NLParser *parser = NULL, *ap = NULL;
    NLSyntaxTree *tree = NULL, *atree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    NLParseDiagnostic pd = {0};
    NLFunctionUnitDiagnostic ud = {0};
    NLCheckDiagnostic cd = {0};
    FILE *render = fopen(argv[3], "wbx");
    int result = 0, render_error = 0, render_close = 0;
    if (render == NULL) {
        problem("renderer_file_open", true);
        return 74;
    }
    NLSourceStatus ss = nl_source_load(argv[1], &source);
    printf("{\"record\":\"status\",\"stage\":\"original_source\","
           "\"value\":%u}\n",
           (unsigned)ss);
    if (ss != NL_SOURCE_OK) {
        result = 74;
        goto done;
    }
    original_source = source;
    if (nl_source_length(source) > MAX_BYTES) {
        problem("original_source_limit", false);
        result = 75;
        goto done;
    }
    source_id(source);
    NLParseStatus ps = nl_parser_create(source, &parser);
    printf("{\"record\":\"status\",\"stage\":\"original_parser\","
           "\"value\":%u}\n",
           (unsigned)ps);
    if (ps != NL_PARSE_OK) {
        result = 74;
        goto done;
    }
    ps = nl_parser_parse_function_unit(parser, &tree, &pd);
    printf("{\"record\":\"status\",\"stage\":\"original_parse\","
           "\"value\":%u}\n",
           (unsigned)ps);
    if (ps != NL_PARSE_OK) {
        bool ok = nl_source_span_valid(source, pd.span) &&
                  nl_parse_diagnostic_render(render, source, &pd);
        diagnostic_record("original_parse", source, &pd.diagnostic, pd.span, 0,
                          ok);
        if (!ok)
            result = 74;
        goto done;
    }
    if (!declarations(tree)) {
        problem("declaration_observation_boundary", false);
        result = 75;
        goto done;
    }
    source_id(source);
    NLCheckStatus cs = nl_semantic_create(&context);
    printf("{\"record\":\"status\",\"stage\":\"original_context\","
           "\"value\":%u}\n",
           (unsigned)cs);
    if (cs != NL_CHECK_OK) {
        result = 74;
        goto done;
    }
    const NLSyntaxTree *inputs[] = {tree};
    cs = nl_semantic_register_function_unit(context, inputs, 1, &ud);
    printf("{\"record\":\"status\",\"stage\":\"original_register\","
           "\"value\":%u}\n",
           (unsigned)cs);
    if (cs != NL_CHECK_OK) {
        bool ok = ud.input_index == 0 &&
                  nl_source_span_valid(source, ud.diagnostic.span) &&
                  nl_check_diagnostic_render(render, source, &ud.diagnostic);
        diagnostic_record("original_register", source,
                          &ud.diagnostic.diagnostic, ud.diagnostic.span,
                          ud.input_index, ok);
        if (!ok)
            result = 74;
        goto done;
    }
    if (invalid) {
        problem("true_invalid_original_admitted_stop_before_adapter", true);
        result = 80;
        goto done;
    }
    ss = nl_source_load(argv[2], &adapter);
    printf("{\"record\":\"status\",\"stage\":\"adapter_source\","
           "\"value\":%u}\n",
           (unsigned)ss);
    if (ss != NL_SOURCE_OK) {
        result = 74;
        goto done;
    }
    adapter_source = adapter;
    source_id(adapter);
    ps = nl_parser_create(adapter, &ap);
    printf("{\"record\":\"status\",\"stage\":\"adapter_parser\","
           "\"value\":%u}\n",
           (unsigned)ps);
    if (ps != NL_PARSE_OK) {
        result = 74;
        goto done;
    }
    ps = nl_parser_parse_expression_fragment(ap, &atree, &pd);
    printf("{\"record\":\"status\",\"stage\":\"adapter_parse\","
           "\"value\":%u}\n",
           (unsigned)ps);
    if (ps != NL_PARSE_OK) {
        bool ok = nl_source_span_valid(adapter, pd.span) &&
                  nl_parse_diagnostic_render(render, adapter, &pd);
        diagnostic_record("adapter_parse", adapter, &pd.diagnostic, pd.span, 0,
                          ok);
        if (!ok)
            result = 74;
        goto done;
    }
    cs = nl_semantic_check_expression(context, atree, &checked, &cd);
    printf("{\"record\":\"status\",\"stage\":\"adapter_check\","
           "\"value\":%u}\n",
           (unsigned)cs);
    if (cs != NL_CHECK_OK) {
        bool ok = nl_source_span_valid(adapter, cd.span) &&
                  nl_check_diagnostic_render(render, adapter, &cd);
        diagnostic_record("adapter_check", adapter, &cd.diagnostic, cd.span, 0,
                          ok);
        if (!ok)
            result = 74;
        goto done;
    }
    entry_fragment = checked;
    inspect(checked, 0, 0, "entry_adapter", 0);
done:
    render_error = ferror(render);
    render_close = fclose(render);
    if (render_error || render_close != 0) {
        problem("renderer_flush_close", true);
        result = 74;
    }
    printf("{\"record\":\"end\",\"fatal\":%s,\"limited\":%s,"
           "\"fragments\":%zu,\"worlds\":%zu,\"nodes\":%zu,"
           "\"runtime_selection_observed\":false,"
           "\"whole_call_theorem_available\":false}\n",
           jb(fatal_problem != 0), jb(limited != 0), fragment_count,
           world_count, total_nodes);
    nl_checked_destroy(checked);
    nl_syntax_tree_destroy(atree);
    nl_parser_destroy(ap);
    nl_source_destroy(adapter);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    if (fatal_problem)
        result = 80;
    if (fflush(stdout) != 0 || ferror(stdout))
        result = 74;
    return result;
}
