#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdio.h>
#include <string.h>

typedef const NLCheckedFragment Frag;
typedef const NLCheckedNodeView Node;
typedef NLCheckedNodeId Id;

static const char *check_name(NLCheckStatus s)
{
    switch (s) {
    case NL_CHECK_OK:
        return "OK";
    case NL_CHECK_SEMANTIC_ERROR:
        return "SEMANTIC_ERROR";
    case NL_CHECK_SEMANTIC_UNSUPPORTED:
        return "UNSUPPORTED";
    case NL_CHECK_ANALYSIS_PRECISION_LIMIT:
        return "PRECISION";
    case NL_CHECK_OUT_OF_MEMORY:
        return "OOM";
    case NL_CHECK_RESOURCE_LIMIT:
        return "RESOURCE";
    case NL_CHECK_INTERNAL_ERROR:
        return "INTERNAL";
    }
    return "INVALID_STATUS";
}

static const char *parse_name(NLParseStatus s)
{
    switch (s) {
    case NL_PARSE_OK:
        return "OK";
    case NL_PARSE_LEXICALLY_UNSUPPORTED:
        return "LEXICALLY_UNSUPPORTED";
    case NL_PARSE_SYNTAX_UNSUPPORTED:
        return "SYNTAX_UNSUPPORTED";
    case NL_PARSE_SYNTAX_ERROR:
        return "SYNTAX_ERROR";
    case NL_PARSE_OUT_OF_MEMORY:
        return "OOM";
    case NL_PARSE_RESOURCE_LIMIT:
        return "RESOURCE";
    case NL_PARSE_INTERNAL_ERROR:
        return "INTERNAL";
    }
    return "INVALID_STATUS";
}

static bool text_at(const NLSource *s, NLSourceSpan span, char *out,
                    size_t capacity)
{
    NLSourceView v;
    if (!nl_source_view(s, span, &v) || v.length >= capacity) {
        return false;
    }
    if (v.length != 0) {
        memcpy(out, v.bytes, v.length);
    }
    out[v.length] = '\0';
    return true;
}

static bool child_path(char *out, size_t capacity, const char *parent,
                       const char *name)
{
    int n = snprintf(out, capacity, "%s/%s", parent, name);
    return n >= 0 && (size_t)n < capacity;
}

static void scalar(Frag *f, NLCheckedResult r, const char *path,
                   const char *tag)
{
    NLSemanticValueView v;
    NLSemanticTypeView t;
    const NLSemanticContext *c = nl_checked_context(f);
    if (r.value == 0 || !nl_semantic_value_view(c, r.value, &v) ||
        !nl_semantic_type_view(c, v.type, &t) || t.kind != NL_TYPE_USIZE) {
        return;
    }
    printf("%s\t%s\t%d\t%zu\t%zu\t%zu\n", tag, path, v.scalar_known ? 1 : 0,
           v.scalar_value, r.type, r.value);
}

static bool arithmetic(Frag *f, Id id, const char *path, NLCheckedKind k)
{
    bool nk, rk;
    size_t n, divisor, result, maximum;
    char op;
    if (k == NL_CHECKED_USIZE_DIVISION) {
        NLUsizeDivisionView v;
        if (!nl_checked_usize_division_view(f, id, &v) ||
            !nl_checked_usize_division_valid(f, id, NULL) ||
            !nl_checked_usize_division_valid(f, id, &v) || v.origin != f ||
            v.node != id) {
            printf("PUBLIC_ERROR\t%s\tdivision_validation\n", path);
            return false;
        }
        nk = v.numerator_known;
        rk = v.quotient_known;
        n = v.numbers[0];
        divisor = v.numbers[1];
        result = v.quotient;
        maximum = v.target_max;
        op = '/';
    } else {
        NLUsizeRemainderView v;
        if (!nl_checked_usize_remainder_view(f, id, &v) ||
            !nl_checked_usize_remainder_valid(f, id, NULL) ||
            !nl_checked_usize_remainder_valid(f, id, &v) || v.origin != f ||
            v.node != id || v.source != nl_checked_source(f)) {
            printf("PUBLIC_ERROR\t%s\tremainder_validation\n", path);
            return false;
        }
        nk = v.numerator_known;
        rk = v.remainder_known;
        n = v.numbers[0];
        divisor = v.numbers[1];
        result = v.remainder;
        maximum = v.target_max;
        op = '%';
    }
    printf("ARITH\t%s\t%c\t%d\t%d\t%zu\t%zu\t%zu\t%zu\n", path, op, nk ? 1 : 0,
           rk ? 1 : 0, n, divisor, result, maximum);
    return true;
}

static bool walk(Frag *f, Id id, const char *path, unsigned depth)
{
    if (depth > 128) {
        printf("APPARATUS_ERROR\tdepth\n");
        return false;
    }
    Node *v = nl_checked_node_view(f, id);
    if (v == NULL) {
        printf("PUBLIC_ERROR\t%s\tmissing_node\n", path);
        return false;
    }
    if (v->kind == NL_CHECKED_BINDING) {
        char name[128], next[1024];
        bool named = text_at(nl_checked_source(f), v->name, name, sizeof(name));
        if (!named || name[0] == '\0') {
            printf("NOT_OBSERVABLE\t%s\tbinding_name\n", path);
            if (snprintf(name, sizeof(name), "unmapped_%zu", id) < 0) {
                return false;
            }
        }
        if (!child_path(next, sizeof(next), path, name)) {
            return false;
        }
        if (v->initializer != 0 && !walk(f, v->initializer, next, depth + 1)) {
            return false;
        }
        return true;
    }
    if (v->kind == NL_CHECKED_BLOCK) {
        Id item = v->first_item;
        size_t steps = 0;
        while (item != 0) {
            Node *i = nl_checked_node_view(f, item);
            if (i == NULL || ++steps > nl_checked_node_count(f)) {
                return false;
            }
            Id next = i->next_item;
            if (!walk(f, item, path, depth + 1)) {
                return false;
            }
            item = next;
        }
        if (v->tail != 0) {
            char next[1024];
            if (!child_path(next, sizeof(next), path, "tail") ||
                !walk(f, v->tail, next, depth + 1)) {
                return false;
            }
        }
        return true;
    }
    Id arg = v->first_argument;
    size_t position = 0;
    while (arg != 0) {
        Node *a = nl_checked_node_view(f, arg);
        char suffix[40], next[1024];
        if (a == NULL || position >= nl_checked_node_count(f)) {
            return false;
        }
        if (snprintf(suffix, sizeof(suffix), "arg%zu", position) < 0 ||
            !child_path(next, sizeof(next), path, suffix) ||
            !walk(f, arg, next, depth + 1)) {
            return false;
        }
        arg = a->next_argument;
        ++position;
    }
    if (v->kind == NL_CHECKED_REGISTERED_CALL) {
        char callee[128], next[1024];
        if (!text_at(nl_checked_source(f), v->name, callee, sizeof(callee))) {
            callee[0] = '\0';
        }
        Frag *body = nl_checked_call_body(f, id);
        printf("CALL\t%s\t%s\t%zu\t%d\n", path, callee, v->function,
               v->body_backed ? 1 : 0);
        if (!v->body_backed || body == NULL) {
            printf("PUBLIC_ERROR\t%s\tmissing_owned_body\n", path);
            return false;
        }
        const NLSource *s = nl_checked_source(body);
        bool same = nl_checked_context(body) == nl_checked_context(f);
        Node *root = nl_checked_node_view(body, nl_checked_root(body));
        printf("BODY\t%s\t%zu\t%d\t%zu\t%zu\n", path,
               s == NULL ? 0 : nl_source_length(s), same ? 1 : 0,
               root == NULL ? 0 : root->span.start_byte,
               root == NULL ? 0 : root->span.end_byte);
        if (!same || s == NULL || root == NULL) {
            printf("PUBLIC_ERROR\t%s\tbody_context\n", path);
            return false;
        }
        if (!child_path(next, sizeof(next), path, "body") ||
            !walk(body, nl_checked_root(body), next, depth + 1)) {
            return false;
        }
    }
    if (v->kind == NL_CHECKED_USIZE_DIVISION ||
        v->kind == NL_CHECKED_USIZE_REMAINDER) {
        if (!arithmetic(f, id, path, v->kind)) {
            return false;
        }
    }
    if (v->result_count == 1) {
        scalar(f, v->results[0], path, "VALUE");
    }
    if (v->kind == NL_CHECKED_RETURN) {
        scalar(f, v->returned, path, "RETURN");
    }
    return true;
}

int main(int argc, char **argv)
{
    NLSource *source = NULL, *entry = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *tree = NULL, *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    NLParseDiagnostic pd = {0};
    NLCheckDiagnostic cd = {0};
    NLFunctionUnitDiagnostic ud = {0};
    int result = 70;
    if (argc != 3 || nl_source_load(argv[1], &source) != NL_SOURCE_OK) {
        fprintf(stderr, "R1581 source load/argument failure\n");
        goto cleanup;
    }
    NLSourceView bytes;
    NLSourceSpan whole = {0, nl_source_length(source)};
    if (!nl_source_view(source, whole, &bytes)) {
        goto cleanup;
    }
    printf("INPUT_HEX\t");
    for (size_t i = 0; i < bytes.length; ++i) {
        printf("%02x", (unsigned)bytes.bytes[i]);
    }
    putchar('\n');
    NLParseStatus ps = nl_parser_create(source, &parser);
    if (ps == NL_PARSE_OK) {
        ps = nl_parser_parse_function_unit(parser, &tree, &pd);
    }
    printf("STAGE\tparse\t%s\n", parse_name(ps));
    if (ps != NL_PARSE_OK) {
        (void)nl_parse_diagnostic_render(stderr, source, &pd);
        result = 0;
        goto cleanup;
    }
    NLCheckStatus cs = nl_semantic_create(&context);
    if (cs != NL_CHECK_OK) {
        printf("STAGE\tcontext\t%s\n", check_name(cs));
        goto cleanup;
    }
    const NLSyntaxTree *inputs[] = {tree};
    cs = nl_semantic_register_function_unit(context, inputs, 1, &ud);
    printf("STAGE\tregister\t%s\n", check_name(cs));
    if (cs != NL_CHECK_OK) {
        (void)nl_check_diagnostic_render(stderr, source, &ud.diagnostic);
        result = 0;
        goto cleanup;
    }
    if (nl_source_create("main()", 6, "R1581-entry", &entry) != NL_SOURCE_OK) {
        goto cleanup;
    }
    ps = nl_parser_create(entry, &entry_parser);
    if (ps != NL_PARSE_OK) {
        goto cleanup;
    }
    ps = nl_parser_parse_expression_fragment(entry_parser, &entry_tree, &pd);
    if (ps != NL_PARSE_OK) {
        goto cleanup;
    }
    cs = nl_semantic_check_expression(context, entry_tree, &checked, &cd);
    printf("STAGE\tmain\t%s\n", check_name(cs));
    if (cs != NL_CHECK_OK) {
        (void)nl_check_diagnostic_render(stderr, entry, &cd);
        result = 0;
        goto cleanup;
    }
    if (strcmp(argv[2], "inspect") != 0) {
        puts("STAGE\tpublic\tNOT_RUN");
        result = 0;
        goto cleanup;
    }
    if (!walk(checked, nl_checked_root(checked), "entry", 0)) {
        goto cleanup;
    }
    puts("LIMIT\twhole_call_result_custody\tNOT_OBSERVABLE");
    puts("STAGE\tpublic\tOK");
    result = 0;
cleanup:
    nl_checked_destroy(checked);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return result;
}
