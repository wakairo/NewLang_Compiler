#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum { MAX_SOURCES = 64, MAX_WORLDS = 64, MAX_FRAGMENTS = 64,
       MAX_NODES = 1024, MAX_TOTAL_NODES = 4096, MAX_DEPTH = 32,
       MAX_SOURCE_BYTES = 8192 };
static const NLSource *sources[MAX_SOURCES];
static const NLSemanticContext *worlds[MAX_WORLDS];
static const NLCheckedFragment *fragments[MAX_FRAGMENTS];
static size_t source_count, world_count, fragment_count, syntax_count;
static size_t checked_count;
static bool evidence_error, observation_gap;

static void hex_bytes(const unsigned char *p, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        printf("%02x", (unsigned)p[i]);
    }
}
static void hex_text(const char *p) {
    if (p != NULL) {
        size_t n = strlen(p);
        if (n > MAX_SOURCE_BYTES) {
            evidence_error = true;
            n = MAX_SOURCE_BYTES;
        }
        hex_bytes((const unsigned char *)p, n);
    }
}
static size_t source_id(const NLSource *s) {
    if (s == NULL) {
        observation_gap = true;
        return 0;
    }
    for (size_t i = 0; i < source_count; ++i) {
        if (sources[i] == s) {
            return i + 1;
        }
    }
    if (source_count == MAX_SOURCES ||
        nl_source_length(s) > MAX_SOURCE_BYTES) {
        evidence_error = true;
        return 0;
    }
    size_t id = ++source_count;
    sources[id - 1] = s;
    NLSourceSpan span = {0, nl_source_length(s)};
    NLSourceView v = {0};
    if (!nl_source_view(s, span, &v)) {
        evidence_error = true;
        return 0;
    }
    printf("{\"record\":\"source\",\"source\":%zu,\"bytes\":%zu,\"hex\":\"",
           id, v.length);
    hex_bytes(v.bytes, v.length);
    printf("\"}\n");
    return id;
}
static size_t world_id(const NLSemanticContext *w) {
    if (w == NULL) {
        observation_gap = true;
        return 0;
    }
    for (size_t i = 0; i < world_count; ++i) {
        if (worlds[i] == w) {
            return i + 1;
        }
    }
    if (world_count == MAX_WORLDS) {
        evidence_error = true;
        return 0;
    }
    size_t id = ++world_count;
    worlds[id - 1] = w;
    NLSemanticSnapshot s = {0};
    bool ok = nl_semantic_snapshot(w, &s);
    printf("{\"record\":\"world\",\"world\":%zu,\"snapshot_available\":%s,"
           "\"types\":%zu,\"bindings\":%zu,\"values\":%zu,\"functions\":%zu}\n",
           id, ok ? "true" : "false", s.types, s.bindings, s.values, s.functions);
    return id;
}
static void span_hex(const NLSource *s, NLSourceSpan span) {
    NLSourceView v = {0};
    if (s == NULL || !nl_source_view(s, span, &v)) {
        observation_gap = true;
        return;
    }
    hex_bytes(v.bytes, v.length);
}
static const char *parse_status(NLParseStatus s) {
    switch (s) {
    case NL_PARSE_OK: return "OK";
    case NL_PARSE_LEXICALLY_UNSUPPORTED: return "LEXICALLY_UNSUPPORTED";
    case NL_PARSE_SYNTAX_UNSUPPORTED: return "SYNTAX_UNSUPPORTED";
    case NL_PARSE_SYNTAX_ERROR: return "SYNTAX_ERROR";
    case NL_PARSE_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
    case NL_PARSE_RESOURCE_LIMIT: return "RESOURCE_LIMIT";
    case NL_PARSE_INTERNAL_ERROR: return "INTERNAL_ERROR";
    }
    evidence_error = true;
    return "UNKNOWN_ENUM";
}
static const char *check_status(NLCheckStatus s) {
    switch (s) {
    case NL_CHECK_OK: return "OK";
    case NL_CHECK_SEMANTIC_ERROR: return "SEMANTIC_ERROR";
    case NL_CHECK_SEMANTIC_UNSUPPORTED: return "SEMANTIC_UNSUPPORTED";
    case NL_CHECK_ANALYSIS_PRECISION_LIMIT: return "ANALYSIS_PRECISION_LIMIT";
    case NL_CHECK_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
    case NL_CHECK_RESOURCE_LIMIT: return "RESOURCE_LIMIT";
    case NL_CHECK_INTERNAL_ERROR: return "INTERNAL_ERROR";
    }
    evidence_error = true;
    return "UNKNOWN_ENUM";
}
static void diagnostic(const char *phase, const NLDiagnostic *d,
                       NLSourceSpan span) {
    printf("{\"record\":\"diagnostic\",\"phase\":\"%s\",\"severity\":%d,"
           "\"span\":[%zu,%zu],\"code_hex\":\"",
           phase, (int)d->severity, span.start_byte, span.end_byte);
    hex_text(d->code);
    printf("\",\"category_hex\":\"");
    hex_text(d->category);
    printf("\",\"message_hex\":\"");
    hex_text(d->message);
    printf("\"}\n");
}
static size_t syntax_node(const NLSource *, const NLSyntaxNode *,
                          size_t, const char *, size_t, size_t);
static void syntax_list(const NLSource *s, const NLSyntaxNode *p, size_t count,
                        size_t parent, const char *role, size_t depth) {
    if (count > MAX_NODES) {
        evidence_error = true;
        return;
    }
    for (size_t i = 0; i < count; ++i) {
        if (p == NULL) {
            evidence_error = true;
            return;
        }
        syntax_node(s, p, parent, role, i, depth);
        p = nl_syntax_next_argument(p);
    }
    if (p != NULL) {
        evidence_error = true;
    }
}
static size_t syntax_node(const NLSource *s, const NLSyntaxNode *p,
                          size_t parent, const char *role, size_t position,
                          size_t depth) {
    if (p == NULL) {
        return 0;
    }
    if (depth > MAX_DEPTH || syntax_count == MAX_NODES) {
        evidence_error = true;
        return 0;
    }
    const NLSyntaxView *v = nl_syntax_node_view(p);
    if (v == NULL) {
        evidence_error = true;
        return 0;
    }
    size_t id = ++syntax_count;
    printf("{\"record\":\"syntax\",\"node\":%zu,\"parent\":%zu,"
           "\"role\":\"%s\",\"position\":%zu,\"kind\":%d,\"span\":[%zu,%zu]}\n",
           id, parent, role, position, (int)v->kind,
           v->span.start_byte, v->span.end_byte);
    switch (v->kind) {
    case NL_SYNTAX_FUNCTION_UNIT:
        syntax_list(s, v->data.function_unit.declarations,
                    v->data.function_unit.count, id, "declaration", depth + 1);
        break;
    case NL_SYNTAX_FUNCTION:
        printf("{\"record\":\"function_syntax\",\"node\":%zu,"
               "\"parameter_count\":%zu,\"name_hex\":\"",
               id, v->data.function.count);
        span_hex(s, v->data.function.name);
        printf("\"}\n");
        syntax_list(s, v->data.function.parameters, v->data.function.count,
                    id, "formal_declaration", depth + 1);
        syntax_node(s, v->data.function.result, id, "result_type", 0, depth + 1);
        syntax_node(s, v->data.function.body, id, "body", 0, depth + 1);
        break;
    case NL_SYNTAX_PARAMETER:
        printf("{\"record\":\"formal_syntax\",\"node\":%zu,\"name_hex\":\"", id);
        span_hex(s, v->data.parameter.name);
        printf("\"}\n");
        syntax_node(s, v->data.parameter.type, id, "formal_type", 0, depth + 1);
        break;
    case NL_SYNTAX_EXPR_CALL:
        printf("{\"record\":\"call_syntax\",\"node\":%zu,"
               "\"argument_count\":%zu,\"callee_hex\":\"",
               id, v->data.call.argument_count);
        span_hex(s, v->data.call.callee);
        printf("\"}\n");
        syntax_list(s, v->data.call.arguments, v->data.call.argument_count,
                    id, "actual_source_order", depth + 1);
        break;
    case NL_SYNTAX_BINDING:
        printf("{\"record\":\"binding_syntax\",\"node\":%zu,\"name_hex\":\"", id);
        span_hex(s, v->data.binding.name);
        printf("\"}\n");
        syntax_node(s, v->data.binding.initializer, id, "initializer", 0,
                    depth + 1);
        break;
    case NL_SYNTAX_BLOCK:
        syntax_list(s, v->data.block.items, v->data.block.item_count,
                    id, "block_item", depth + 1);
        syntax_node(s, v->data.block.tail, id, "tail", 0, depth + 1);
        break;
    case NL_SYNTAX_STATEMENT:
    case NL_SYNTAX_RETURN:
        syntax_node(s, v->data.statement.expression, id, "expression", 0,
                    depth + 1);
        break;
    case NL_SYNTAX_IF:
        syntax_node(s, v->data.conditional.condition, id, "condition", 0,
                    depth + 1);
        syntax_node(s, v->data.conditional.then_block, id, "then", 0, depth + 1);
        syntax_node(s, v->data.conditional.else_block, id, "else", 1, depth + 1);
        break;
    case NL_SYNTAX_USIZE_EQUALITY:
    case NL_SYNTAX_USIZE_LESS_THAN:
    case NL_SYNTAX_USIZE_ADDITION:
    case NL_SYNTAX_USIZE_SUBTRACTION:
    case NL_SYNTAX_USIZE_DIVISION:
    case NL_SYNTAX_USIZE_REMAINDER:
        syntax_node(s, v->data.equality.left, id, "left", 0, depth + 1);
        syntax_node(s, v->data.equality.right, id, "right", 1, depth + 1);
        break;
    case NL_SYNTAX_TYPE_NAME:
    case NL_SYNTAX_EXPR_NAME:
    case NL_SYNTAX_USIZE_LITERAL:
        break;
    default:
        observation_gap = true;
        printf("{\"record\":\"unwalked_syntax_kind\",\"node\":%zu}\n", id);
        break;
    }
    return id;
}
static void value_record(const NLSemanticContext *w, size_t wid, NLValueId id) {
    if (id == 0) {
        return;
    }
    NLSemanticValueView v = {0};
    bool ok = nl_semantic_value_view(w, id, &v);
    printf("{\"record\":\"value\",\"world\":%zu,\"value\":%zu,"
           "\"available\":%s,\"type\":%zu,\"carrier\":%d,"
           "\"owner_place\":%zu,\"scalar_known\":%s,\"scalar\":%zu}\n",
           wid, id, ok ? "true" : "false", v.type, (int)v.carrier,
           v.owner_place, v.scalar_known ? "true" : "false", v.scalar_value);
    if (!ok) {
        observation_gap = true;
    }
}
static void binding_record(const NLSemanticContext *w, size_t wid,
                           NLSymbolId id) {
    if (id == 0) {
        return;
    }
    NLSemanticBindingView b = {0};
    bool ok = nl_semantic_binding_view(w, id, &b);
    printf("{\"record\":\"binding\",\"world\":%zu,\"symbol\":%zu,"
           "\"view_available\":%s,\"availability\":%d,\"type\":%zu,"
           "\"value\":%zu,\"place\":%zu}\n",
           wid, id, ok ? "true" : "false", (int)b.availability,
           b.type, b.value, b.place);
    if (ok) {
        value_record(w, wid, b.value);
    } else {
        observation_gap = true;
    }
}
static void queue_node(NLCheckedNodeId *ids, size_t *n, NLCheckedNodeId id) {
    if (id == 0) {
        return;
    }
    for (size_t i = 0; i < *n; ++i) {
        if (ids[i] == id) {
            return;
        }
    }
    if (*n == MAX_NODES) {
        evidence_error = true;
        return;
    }
    ids[(*n)++] = id;
}
static void checked_fragment(const NLCheckedFragment *f, size_t parent,
                             NLCheckedNodeId owner, const char *role,
                             size_t position, size_t depth) {
    if (f == NULL) {
        observation_gap = true;
        printf("{\"record\":\"missing_fragment\",\"parent\":%zu,"
               "\"owner\":%zu,\"role\":\"%s\",\"position\":%zu}\n",
               parent, owner, role, position);
        return;
    }
    size_t fid = 0;
    for (size_t i = 0; i < fragment_count; ++i) {
        if (fragments[i] == f) {
            fid = i + 1;
        }
    }
    bool seen = fid != 0;
    if (!seen) {
        if (depth > MAX_DEPTH || fragment_count == MAX_FRAGMENTS) {
            evidence_error = true;
            return;
        }
        fid = ++fragment_count;
        fragments[fid - 1] = f;
    }
    printf("{\"record\":\"fragment_edge\",\"parent\":%zu,\"owner\":%zu,"
           "\"role\":\"%s\",\"position\":%zu,\"fragment\":%zu}\n",
           parent, owner, role, position, fid);
    if (seen) {
        return;
    }
    const NLSemanticContext *w = nl_checked_context(f);
    const NLSource *s = nl_checked_source(f);
    size_t wid = world_id(w), sid = source_id(s);
    size_t declared = nl_checked_node_count(f);
    NLCheckedNodeId root = nl_checked_root(f);
    printf("{\"record\":\"fragment\",\"fragment\":%zu,\"world\":%zu,"
           "\"source\":%zu,\"root\":%zu,\"reported_node_count\":%zu}\n",
           fid, wid, sid, root, declared);
    if (w == NULL || root == 0 || declared > MAX_NODES) {
        evidence_error = true;
        return;
    }
    NLCheckedNodeId ids[MAX_NODES] = {0};
    size_t n = 0;
    queue_node(ids, &n, root);
    for (size_t i = 0; i < n && !evidence_error; ++i) {
        if (++checked_count > MAX_TOTAL_NODES) {
            evidence_error = true;
            break;
        }
        NLCheckedNodeId id = ids[i];
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (v == NULL || v->result_count > 2) {
            evidence_error = true;
            break;
        }
        printf("{\"record\":\"checked_node\",\"fragment\":%zu,\"node\":%zu,"
               "\"kind\":%d,\"span\":[%zu,%zu],\"name_hex\":\"",
               fid, id, (int)v->kind, v->span.start_byte, v->span.end_byte);
        span_hex(s, v->name);
        printf("\",\"function\":%zu,\"body_backed\":%s,\"symbol\":%zu,"
               "\"type\":%zu,\"parameter_type\":%zu,\"value_use\":%d,"
               "\"argument_count\":%zu,\"first_argument\":%zu,"
               "\"next_argument\":%zu,\"initializer\":%zu,"
               "\"first_item\":%zu,\"next_item\":%zu,\"tail\":%zu,"
               "\"item_count\":%zu,\"result_count\":%zu,\"terminates\":%s,"
               "\"normal_arms\":%zu,\"has_scalar_result\":%s,"
               "\"scalar_known\":%s,\"scalar\":%zu,\"results\":[",
               v->function, v->body_backed ? "true" : "false", v->symbol,
               v->type, v->parameter_type, (int)v->value_use,
               v->argument_count, v->first_argument, v->next_argument,
               v->initializer, v->first_item, v->next_item, v->tail,
               v->item_count, v->result_count,
               v->terminates ? "true" : "false", v->normal_arms,
               v->has_scalar_result ? "true" : "false",
               v->has_scalar_result && v->scalar_result.known ? "true" : "false",
               v->has_scalar_result && v->scalar_result.known
                   ? v->scalar_result.value
                   : 0);
        for (size_t j = 0; j < v->result_count; ++j) {
            printf("%s{\"type\":%zu,\"value\":%zu}", j == 0 ? "" : ",",
                   v->results[j].type, v->results[j].value);
        }
        if (v->kind == NL_CHECKED_RETURN) {
            printf("],\"returned\":{\"type\":%zu,\"value\":%zu}}\n",
                   v->returned.type, v->returned.value);
        } else {
            printf("],\"returned\":null}\n");
        }
        binding_record(w, wid, v->symbol);
        for (size_t j = 0; j < v->result_count; ++j) {
            value_record(w, wid, v->results[j].value);
        }
        if (v->kind == NL_CHECKED_RETURN) {
            value_record(w, wid, v->returned.value);
        }
        queue_node(ids, &n, v->first_argument);
        queue_node(ids, &n, v->next_argument);
        queue_node(ids, &n, v->initializer);
        queue_node(ids, &n, v->first_item);
        queue_node(ids, &n, v->next_item);
        queue_node(ids, &n, v->tail);
        if (v->kind == NL_CHECKED_USIZE_DIVISION) {
            NLUsizeDivisionView d = {0};
            bool available = nl_checked_usize_division_view(f, id, &d);
            bool valid = nl_checked_usize_division_valid(f, id, NULL);
            printf("{\"record\":\"division\",\"fragment\":%zu,\"node\":%zu,"
                   "\"view_available\":%s,\"valid\":%s,"
                   "\"origin_matches\":%s,\"operands\":[%zu,%zu],"
                   "\"values\":[%zu,%zu],\"result\":%zu,"
                   "\"symbols\":[%zu,%zu],\"numbers\":[%zu,%zu],"
                   "\"numerator_known\":%s,\"quotient_known\":%s,"
                   "\"quotient\":%zu,\"target_max\":%zu}\n",
                   fid, id, available ? "true" : "false",
                   valid ? "true" : "false", d.origin == f ? "true" : "false",
                   d.operands[0], d.operands[1], d.values[0], d.values[1],
                   d.result, d.symbols[0], d.symbols[1], d.numbers[0],
                   d.numbers[1], d.numerator_known ? "true" : "false",
                   d.quotient_known ? "true" : "false", d.quotient, d.target_max);
            if (!available || !valid) {
                observation_gap = true;
            }
        }
        if (v->kind == NL_CHECKED_USIZE_REMAINDER) {
            NLUsizeRemainderView r = {0};
            bool available = nl_checked_usize_remainder_view(f, id, &r);
            bool valid = nl_checked_usize_remainder_valid(f, id, NULL);
            printf("{\"record\":\"remainder\",\"fragment\":%zu,\"node\":%zu,"
                   "\"view_available\":%s,\"valid\":%s,"
                   "\"origin_matches\":%s,\"operands\":[%zu,%zu],"
                   "\"values\":[%zu,%zu],\"result\":%zu,"
                   "\"symbols\":[%zu,%zu],\"numbers\":[%zu,%zu],"
                   "\"numerator_known\":%s,\"remainder_known\":%s,"
                   "\"remainder\":%zu,\"target_max\":%zu}\n",
                   fid, id, available ? "true" : "false",
                   valid ? "true" : "false", r.origin == f ? "true" : "false",
                   r.operands[0], r.operands[1], r.values[0], r.values[1],
                   r.result, r.symbols[0], r.symbols[1], r.numbers[0],
                   r.numbers[1], r.numerator_known ? "true" : "false",
                   r.remainder_known ? "true" : "false", r.remainder,
                   r.target_max);
            if (!available || !valid) {
                observation_gap = true;
            }
        }
        if (v->kind == NL_CHECKED_REGISTERED_CALL && v->body_backed) {
            checked_fragment(nl_checked_call_body(f, id), fid, id,
                             "retained_call_body", 0, depth + 1);
        }
        if (v->kind == NL_CHECKED_IF) {
            checked_fragment(nl_checked_if_arm(f, id, 0), fid, id,
                             "hypothetical_then", 0, depth + 1);
            checked_fragment(nl_checked_if_arm(f, id, 1), fid, id,
                             "hypothetical_else", 1, depth + 1);
        }
    }
    printf("{\"record\":\"fragment_walk_end\",\"fragment\":%zu,"
           "\"reachable_nodes\":%zu,\"reported_node_count\":%zu,"
           "\"enumeration\":\"public_root_edges_only\"}\n",
           fid, n, declared);
}

static void phase_record(const char *name, const char *status, size_t sid) {
    printf("{\"record\":\"phase\",\"phase\":\"%s\",\"status\":\"%s\","
           "\"source\":%zu}\n", name, status, sid);
    if (fflush(stdout) != 0) {
        evidence_error = true;
    }
}
static void admission_record(const char *name, size_t sid) {
    printf("{\"record\":\"admission\",\"phase\":\"%s\",\"source\":%zu,"
           "\"status\":\"OK\",\"admitted\":true}\n", name, sid);
    if (fflush(stdout) != 0) {
        evidence_error = true;
    }
}
static bool rendered_diagnostic(const char *phase, const NLSource *source,
                                size_t sid, const NLParseDiagnostic *parse,
                                const NLCheckDiagnostic *check) {
    enum { MAX_RENDER_BYTES = 32768 };
    unsigned char bytes[MAX_RENDER_BYTES + 1];
    size_t count = 0, forwarded = 0;
    bool renderer_ok = false, capture_ok = true, truncated = false;
    bool stderr_ok = true;
    FILE *temporary = tmpfile();
    if (temporary == NULL) {
        capture_ok = false;
    } else {
        if (parse != NULL && check == NULL) {
            renderer_ok = nl_parse_diagnostic_render(temporary, source, parse);
        } else if (check != NULL && parse == NULL) {
            renderer_ok = nl_check_diagnostic_render(temporary, source, check);
        } else {
            capture_ok = false;
        }
        if (fflush(temporary) != 0 || ferror(temporary) ||
            fseek(temporary, 0, SEEK_SET) != 0) {
            capture_ok = false;
        } else {
            count = fread(bytes, 1, sizeof(bytes), temporary);
            if (ferror(temporary)) {
                capture_ok = false;
            }
            if (count > MAX_RENDER_BYTES) {
                truncated = true;
                count = MAX_RENDER_BYTES;
            }
        }
        if (fclose(temporary) != 0) {
            capture_ok = false;
        }
    }
    if (count != 0) {
        forwarded = fwrite(bytes, 1, count, stderr);
    }
    if (forwarded != count || fflush(stderr) != 0 || ferror(stderr)) {
        stderr_ok = false;
    }
    printf("{\"record\":\"renderer\",\"phase\":\"%s\",\"source\":%zu,"
           "\"kind\":\"%s\",\"public_renderer_return\":%s,"
           "\"capture_ok\":%s,\"truncated\":%s,\"bytes\":%zu,\"hex\":\"",
           phase, sid, parse != NULL ? "parse" : "check",
           renderer_ok ? "true" : "false", capture_ok ? "true" : "false",
           truncated ? "true" : "false", count);
    hex_bytes(bytes, count);
    printf("\",\"stderr_forwarded_bytes\":%zu,\"stderr_write_ok\":%s}\n",
           forwarded, stderr_ok ? "true" : "false");
    if (fflush(stdout) != 0) {
        evidence_error = true;
    }
    if (!capture_ok || truncated || !stderr_ok) {
        evidence_error = true;
    }
    return renderer_ok && capture_ok && !truncated && stderr_ok && count != 0;
}
int main(int argc, char **argv) {
    NLSource *source = NULL, *entry = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *tree = NULL, *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    NLParseDiagnostic pd = {0};
    NLFunctionUnitDiagnostic ud = {0};
    NLCheckDiagnostic cd = {0};
    size_t original_sid = 0, entry_sid = 0;
    int rc = 20;
    const char *terminal = "HEALTH_FAILURE";
    if (argc != 3 || (strcmp(argv[2], "registration") != 0 &&
                      strcmp(argv[2], "entry") != 0)) {
        return 20;
    }
    printf("{\"record\":\"begin\",\"schema\":\"r1617-observer-v2\","
           "\"mode\":\"%s\"}\n", argv[2]);
    NLSourceStatus ss = nl_source_load(argv[1], &source);
    printf("{\"record\":\"source_load\",\"status\":%d}\n", (int)ss);
    phase_record("source_load", ss == NL_SOURCE_OK ? "OK" : "SOURCE_FAILURE", 0);
    if (ss != NL_SOURCE_OK || source == NULL ||
        nl_source_length(source) > MAX_SOURCE_BYTES || evidence_error) {
        goto done;
    }
    original_sid = source_id(source);
    NLParseStatus ps = nl_parser_create(source, &parser);
    phase_record("parser_create", parse_status(ps), original_sid);
    if (ps != NL_PARSE_OK || parser == NULL || evidence_error) {
        goto done;
    }
    ps = nl_parser_parse_function_unit(parser, &tree, &pd);
    phase_record("original_parse", parse_status(ps), original_sid);
    if (ps != NL_PARSE_OK) {
        diagnostic("original_parse", &pd.diagnostic, pd.span);
        if (!rendered_diagnostic("original_parse", source, original_sid, &pd, NULL)) {
            goto done;
        }
        if (ps == NL_PARSE_LEXICALLY_UNSUPPORTED ||
            ps == NL_PARSE_SYNTAX_UNSUPPORTED || ps == NL_PARSE_SYNTAX_ERROR) {
            rc = 10;
            terminal = "ORIGINAL_PARSE_REFUSAL";
        }
        goto done;
    }
    if (tree == NULL || evidence_error) {
        goto done;
    }
    syntax_node(source, nl_syntax_tree_root(tree), 0, "original_root", 0, 0);
    if (evidence_error) {
        goto done;
    }
    NLCheckStatus cs = nl_semantic_create(&context);
    phase_record("semantic_create", check_status(cs), original_sid);
    if (cs != NL_CHECK_OK || context == NULL || evidence_error) {
        goto done;
    }
    const NLSyntaxTree *inputs[] = {tree};
    cs = nl_semantic_register_function_unit(context, inputs, 1, &ud);
    phase_record("original_register", check_status(cs), original_sid);
    if (cs != NL_CHECK_OK) {
        diagnostic("original_register", &ud.diagnostic.diagnostic,
                   ud.diagnostic.span);
        printf("{\"record\":\"unit_diagnostic_input\",\"index\":%zu}\n",
               ud.input_index);
        if (ud.input_index != 0 ||
            !rendered_diagnostic("original_register", source, original_sid,
                                 NULL, &ud.diagnostic)) {
            goto done;
        }
        if (cs == NL_CHECK_SEMANTIC_ERROR ||
            cs == NL_CHECK_SEMANTIC_UNSUPPORTED ||
            cs == NL_CHECK_ANALYSIS_PRECISION_LIMIT) {
            rc = 11;
            terminal = "ORIGINAL_REGISTRATION_REFUSAL";
        }
        goto done;
    }
    /* Preserve the actual original admission before any observation or cleanup. */
    admission_record("original_register", original_sid);
    if (evidence_error) {
        goto done;
    }
    world_id(context);
    if (strcmp(argv[2], "registration") == 0) {
        rc = 0;
        terminal = "REGISTRATION_ONLY_ENTRY_NOT_RUN";
        goto done;
    }
    static const char entry_bytes[] = "main()\n";
    ss = nl_source_create(entry_bytes, sizeof(entry_bytes) - 1,
                          "r1617-entry-call.nl", &entry);
    printf("{\"record\":\"entry_source_create\",\"status\":%d}\n", (int)ss);
    phase_record("entry_source_create",
                 ss == NL_SOURCE_OK ? "OK" : "SOURCE_FAILURE", 0);
    if (ss != NL_SOURCE_OK || entry == NULL || evidence_error) {
        goto done;
    }
    entry_sid = source_id(entry);
    ps = nl_parser_create(entry, &entry_parser);
    phase_record("entry_parser_create", parse_status(ps), entry_sid);
    if (ps != NL_PARSE_OK || entry_parser == NULL || evidence_error) {
        goto done;
    }
    ps = nl_parser_parse_expression_fragment(entry_parser, &entry_tree, &pd);
    phase_record("entry_parse", parse_status(ps), entry_sid);
    if (ps != NL_PARSE_OK) {
        diagnostic("entry_parse", &pd.diagnostic, pd.span);
        (void)rendered_diagnostic("entry_parse", entry, entry_sid, &pd, NULL);
        goto done;
    }
    if (entry_tree == NULL || evidence_error) {
        goto done;
    }
    cs = nl_semantic_check_expression(context, entry_tree, &checked, &cd);
    phase_record("entry_check", check_status(cs), entry_sid);
    if (cs != NL_CHECK_OK) {
        diagnostic("entry_check", &cd.diagnostic, cd.span);
        if (!rendered_diagnostic("entry_check", entry, entry_sid, NULL, &cd)) {
            goto done;
        }
        if (cs == NL_CHECK_SEMANTIC_ERROR ||
            cs == NL_CHECK_SEMANTIC_UNSUPPORTED ||
            cs == NL_CHECK_ANALYSIS_PRECISION_LIMIT) {
            rc = 12;
            terminal = "ENTRY_REFUSAL";
        }
        goto done;
    }
    admission_record("entry_check", entry_sid);
    if (checked == NULL || evidence_error) {
        evidence_error = true;
        goto done;
    }
    checked_fragment(checked, 0, 0, "entry_call", 0, 0);
    rc = 0;
    terminal = "PUBLIC_OBSERVATION_COMPLETE";
done:
    if (evidence_error) {
        rc = 21;
        terminal = "EVIDENCE_OR_LIMIT_FAILURE";
    }
    printf("{\"record\":\"observation_limit\","
           "\"actual_to_formal_identity\":\"NOT_EXPOSED_BY_SELECTED_HEADERS\","
           "\"native_temporal_order\":\"NOT_OBSERVED\","
           "\"synthetic_guest_state\":false,\"oracle_read\":false}\n");
    printf("{\"record\":\"terminal\",\"state\":\"%s\",\"rc\":%d,"
           "\"observation_gap\":%s,\"evidence_error\":%s}\n",
           terminal, rc, observation_gap ? "true" : "false",
           evidence_error ? "true" : "false");
    /* Destructors are after the admission prefix; their errors cannot erase it. */
    nl_checked_destroy(checked);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    if (ferror(stdout) || fflush(stdout) != 0 ||
        ferror(stderr) || fflush(stderr) != 0) {
        return 22;
    }
    return rc;
}
