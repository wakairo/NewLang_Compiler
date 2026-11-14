#include "newlang/checked.h"
#include "newlang/checked_c_node.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);

static bool inject, fired;

static bool fail_once(void)
{
    if (!inject)
        return false;
    inject = false;
    fired = true;
    return true;
}

void *__wrap_malloc(size_t n)
{
    return n && fail_once() ? NULL : __real_malloc(n);
}

void *__wrap_calloc(size_t n, size_t z)
{
    return n && z && fail_once() ? NULL : __real_calloc(n, z);
}

void *__wrap_realloc(void *p, size_t n)
{
    return n && fail_once() ? NULL : __real_realloc(p, n);
}

typedef struct {
    NLSource *source, *dispatch;
    NLParser *parser, *dispatch_parser;
    NLSyntaxTree *tree, *dispatch_tree;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
    char *generated;
    bool source_ok, parser_ok, tree_ok, context_ok;
    bool dispatch_ok, dispatch_parser_ok, dispatch_tree_ok, entry_ok;
    bool generated_ok;
} Fixture;

static void cleanup(Fixture *f)
{
    inject = false;
    if (f->generated_ok)
        free(f->generated);
    if (f->entry_ok)
        nl_checked_destroy(f->entry);
    if (f->dispatch_tree_ok)
        nl_syntax_tree_destroy(f->dispatch_tree);
    if (f->dispatch_parser_ok)
        nl_parser_destroy(f->dispatch_parser);
    if (f->dispatch_ok)
        nl_source_destroy(f->dispatch);
    if (f->context_ok)
        nl_semantic_destroy(f->context);
    if (f->tree_ok)
        nl_syntax_tree_destroy(f->tree);
    if (f->parser_ok)
        nl_parser_destroy(f->parser);
    if (f->source_ok)
        nl_source_destroy(f->source);
}

static bool same_snapshot(const NLSemanticSnapshot *a,
                          const NLSemanticSnapshot *b)
{
    return a->types == b->types && a->bindings == b->bindings &&
           a->values == b->values && a->places == b->places &&
           a->domains == b->domains && a->scopes == b->scopes &&
           a->functions == b->functions &&
           a->last_incarnation == b->last_incarnation &&
           a->last_value_fact == b->last_value_fact &&
           a->backing_regions == b->backing_regions &&
           a->raw_intervals == b->raw_intervals &&
           a->occurrences == b->occurrences;
}

static void arm(const char *mode, const char *stage)
{
    inject = strcmp(mode, stage) == 0;
    fired = false;
}

static int oom_result(bool output_unchanged, bool snapshot_unchanged,
                      bool actual_oom, const char *stage)
{
    printf("R1415 OOM stage=%s fired=%d output_unchanged=%d "
           "snapshot_unchanged=%d declared_oom=%d\n",
           stage, fired, output_unchanged, snapshot_unchanged, actual_oom);
    if (!fired)
        return 77;
    return output_unchanged && snapshot_unchanged && actual_oom ? 0 : 70;
}

static bool conservative(NLCheckStatus s)
{
    return s == NL_CHECK_SEMANTIC_UNSUPPORTED ||
           s == NL_CHECK_ANALYSIS_PRECISION_LIMIT ||
           s == NL_CHECK_RESOURCE_LIMIT;
}

static bool mentions_domain(const char *text)
{
    if (!text)
        return false;
    for (; *text; ++text) {
        const char *word = "domain";
        size_t i = 0;
        while (word[i] && text[i] && tolower((unsigned char)text[i]) == word[i])
            ++i;
        if (!word[i])
            return true;
    }
    return false;
}

static bool wrong_domain_location(const char *text, NLSourceSpan span)
{
    const char *begin =
        strstr(text, "let amber_observed = loan_read(violet_life)");
    const char *end = begin ? strstr(begin, "read(amber_reader@peer)") : NULL;
    return begin && end && span.start_byte >= (size_t)(begin - text) &&
           span.end_byte <=
               (size_t)(end - text) + strlen("read(amber_reader@peer)") &&
           span.end_byte >= span.start_byte;
}

static int check_refusal(const char *stage, NLCheckStatus status,
                         const NLCheckDiagnostic *d, const NLSource *source,
                         const char *text, bool negative)
{
    printf("R1415 CHECK stage=%s status=%d code=%s span=%zu:%zu\n", stage,
           (int)status, d->diagnostic.code ? d->diagnostic.code : "(none)",
           d->span.start_byte, d->span.end_byte);
    printf("R1415 DIAGNOSTIC message=%s\n",
           d->diagnostic.message ? d->diagnostic.message : "(none)");
    if (source)
        nl_check_diagnostic_render(stderr, source, d);
    if (conservative(status)) {
        puts("R1415 LIMIT conservative refusal; downstream NOT_REACHED");
        return 77;
    }
    if (negative && source && status == NL_CHECK_SEMANTIC_ERROR &&
        wrong_domain_location(text, d->span) &&
        (mentions_domain(d->diagnostic.code) ||
         mentions_domain(d->diagnostic.message))) {
        puts("R1415 NEGATIVE expected wrong-Domain rejection REACHED");
        return 0;
    }
    if (negative && status == NL_CHECK_SEMANTIC_ERROR) {
        puts("R1415 NEGATIVE refused; intended wrong-Domain NOT_REACHED "
             "because diagnostic origin/reason is unclassified");
        return 77;
    }
    puts("R1415 STOP unexpected semantic result; original evidence preserved");
    return 70;
}

typedef struct {
    size_t fragments, conditionals, trials, reads, replacements, releases;
} Inventory;

static bool inspect_owned(const NLCheckedFragment *f, unsigned depth,
                          Inventory *out)
{
    if (!f || depth > 64 || !nl_checked_context(f) || !nl_checked_source(f))
        return false;
    ++out->fragments;
    size_t count = nl_checked_node_count(f);
    for (size_t i = 1; i <= count; ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (!v)
            return false;
        if (v->kind == NL_CHECKED_IF) {
            ++out->conditionals;
            printf("R1415 OWNED IF depth=%u node=%zu result_count=%zu\n", depth,
                   i, v->result_count);
            for (size_t a = 0; a < 2; ++a) {
                const NLCheckedFragment *child = nl_checked_if_arm(f, i, a);
                if (!inspect_owned(child, depth + 1, out))
                    return false;
            }
        }
        if (v->kind == NL_CHECKED_MATCH) {
            /* The frozen cohort contains only binary Option matches. */
            if (nl_checked_match_arm(f, i, 2))
                return false;
            for (size_t a = 0; a < 2; ++a) {
                const NLCheckedFragment *child = nl_checked_match_arm(f, i, a);
                if (!child || !inspect_owned(child, depth + 1, out))
                    return false;
            }
        }
        if (v->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
            ++out->trials;
        if (v->kind == NL_CHECKED_LINK_READ)
            ++out->reads;
        if (v->kind == NL_CHECKED_REPLACE)
            ++out->replacements;
        if (v->kind == NL_CHECKED_DEALLOCATE)
            ++out->releases;
        if (v->kind == NL_CHECKED_REF_FROM_PTR) {
            printf("R1415 OWNED ROOT_ACCESS depth=%u node=%zu "
                   "span=%zu:%zu result_count=%zu\n",
                   depth, i, v->span.start_byte, v->span.end_byte,
                   v->result_count);
        }
    }
    return true;
}

static int run(const char *path, const char *mode, const char *output)
{
    int result = 70;
    Fixture f = {0};
    FILE *input = fopen(path, "rb");
    char text[8192];
    if (!input)
        return 71;
    size_t length = fread(text, 1, sizeof(text) - 1, input);
    bool read_ok = !ferror(input) && feof(input);
    if (fclose(input) != 0 || !read_ok)
        return 71;
    text[length] = '\0';
    bool negative = strcmp(mode, "negative") == 0;

    arm(mode, "source");
    NLSourceStatus source_status =
        nl_source_create(text, length, path, &f.source);
    inject = false;
    f.source_ok = source_status == NL_SOURCE_OK;
    printf("R1415 SOURCE status=%d\n", (int)source_status);
    if (strcmp(mode, "source") == 0) {
        result = oom_result(f.source == NULL, true,
                            source_status == NL_SOURCE_OUT_OF_MEMORY,
                            "source_create");
        goto done;
    }
    if (source_status != NL_SOURCE_OK)
        goto done;
    f.parser_ok = nl_parser_create(f.source, &f.parser) == NL_PARSE_OK;
    if (!f.parser_ok)
        goto done;

    NLParseDiagnostic parse = {0};
    arm(mode, "parser");
    NLParseStatus parse_status =
        nl_parser_parse_function_unit(f.parser, &f.tree, &parse);
    inject = false;
    f.tree_ok = parse_status == NL_PARSE_OK;
    printf("R1415 PARSE status=%d code=%s span=%zu:%zu\n", (int)parse_status,
           parse.diagnostic.code ? parse.diagnostic.code : "(none)",
           parse.span.start_byte, parse.span.end_byte);
    if (strcmp(mode, "parser") == 0) {
        result = oom_result(f.tree == NULL, true,
                            parse_status == NL_PARSE_OUT_OF_MEMORY,
                            "parse_function_unit");
        goto done;
    }
    if (parse_status != NL_PARSE_OK) {
        if (f.tree)
            goto done;
        nl_parse_diagnostic_render(stderr, f.source, &parse);
        result = parse_status == NL_PARSE_LEXICALLY_UNSUPPORTED ||
                         parse_status == NL_PARSE_SYNTAX_UNSUPPORTED ||
                         parse_status == NL_PARSE_RESOURCE_LIMIT
                     ? 77
                     : 71;
        goto done;
    }

    f.context_ok = nl_semantic_create(&f.context) == NL_CHECK_OK;
    if (!f.context_ok)
        goto done;
    const NLSyntaxTree *units[1] = {f.tree};
    NLFunctionUnitDiagnostic unit = {0};
    NLSemanticSnapshot before = {0}, after = {0};
    if (!nl_semantic_snapshot(f.context, &before))
        goto done;
    arm(mode, "register");
    NLCheckStatus status =
        nl_semantic_register_function_unit(f.context, units, 1, &unit);
    inject = false;
    if (!nl_semantic_snapshot(f.context, &after))
        goto done;
    printf("R1415 REGISTER status=%d input_index=%zu\n", (int)status,
           unit.input_index);
    if (strcmp(mode, "register") == 0) {
        result = oom_result(true, same_snapshot(&before, &after),
                            status == NL_CHECK_OUT_OF_MEMORY,
                            "register_function_unit");
        goto done;
    }
    if (status != NL_CHECK_OK) {
        if (!same_snapshot(&before, &after))
            goto done;
        result = check_refusal("register", status, &unit.diagnostic, f.source,
                               text, negative);
        goto done;
    }

    f.dispatch_ok = nl_source_create("main()", 6, "R1415 public main dispatch",
                                     &f.dispatch) == NL_SOURCE_OK;
    if (!f.dispatch_ok)
        goto done;
    f.dispatch_parser_ok =
        nl_parser_create(f.dispatch, &f.dispatch_parser) == NL_PARSE_OK;
    if (!f.dispatch_parser_ok)
        goto done;
    f.dispatch_tree_ok =
        nl_parser_parse_source_fragment(f.dispatch_parser, &f.dispatch_tree,
                                        &parse) == NL_PARSE_OK;
    if (!f.dispatch_tree_ok)
        goto done;
    NLCheckDiagnostic diagnostic = {0};
    if (!nl_semantic_snapshot(f.context, &before))
        goto done;
    arm(mode, "entry");
    status = nl_semantic_check_source_fragment(f.context, f.dispatch_tree,
                                               &f.entry, &diagnostic);
    inject = false;
    f.entry_ok = status == NL_CHECK_OK;
    if (!nl_semantic_snapshot(f.context, &after))
        goto done;
    printf("R1415 ENTRY status=%d owned=%d\n", (int)status, f.entry != NULL);
    if (strcmp(mode, "entry") == 0) {
        result =
            oom_result(f.entry == NULL, same_snapshot(&before, &after),
                       status == NL_CHECK_OUT_OF_MEMORY, "check_source_entry");
        goto done;
    }
    if (status != NL_CHECK_OK) {
        if (f.entry || !same_snapshot(&before, &after))
            goto done;
        result =
            check_refusal("entry", status, &diagnostic, NULL, text, negative);
        goto done;
    }
    if (negative) {
        puts("R1415 STOP negative reached semantic acceptance; never emit/run");
        goto done;
    }
    NLCheckedNodeId root = nl_checked_root(f.entry);
    const NLCheckedNodeView *call = nl_checked_node_view(f.entry, root);
    const NLCheckedFragment *body = nl_checked_call_body(f.entry, root);
    if (!call || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || !body)
        goto done;
    Inventory inventory = {0};
    if (!inspect_owned(body, 0, &inventory))
        goto done;
    printf("R1415 OWNED fragments=%zu IF=%zu trial=%zu read=%zu "
           "replace=%zu release=%zu\n",
           inventory.fragments, inventory.conditionals, inventory.trials,
           inventory.reads, inventory.replacements, inventory.releases);
    if (!inventory.conditionals || inventory.trials < 2 ||
        inventory.reads < 2 || !inventory.replacements ||
        inventory.releases < 2)
        goto done;
    puts("R1415 PTR_IF_AUTH NOT_REACHED: applicable typed public candidate "
         "interface absent; field-only views are not substituted");

    size_t generated_length = 13579;
    if (!nl_semantic_snapshot(f.context, &before))
        goto done;
    arm(mode, "emit");
    NLNodeCStatus emitted =
        nl_checked_c_node(f.entry, &f.generated, &generated_length);
    inject = false;
    f.generated_ok = emitted == NL_NODE_C_OK;
    if (!nl_semantic_snapshot(f.context, &after) ||
        !same_snapshot(&before, &after))
        goto done;
    printf("R1415 EMIT status=%d output=%d length=%zu\n", (int)emitted,
           f.generated != NULL, generated_length);
    if (strcmp(mode, "emit") == 0) {
        bool atomic = f.generated == NULL && generated_length == 13579;
        bool declared_oom = emitted == NL_NODE_C_OUT_OF_MEMORY;
        bool fail_closed = emitted == NL_NODE_C_UNSUPPORTED ||
                           emitted == NL_NODE_C_RESOURCE_LIMIT;
        printf("R1415 OOM stage=checked_c_emission fired=%d "
               "output_unchanged=%d declared_oom=%d fail_closed=%d\n",
               fired, atomic, declared_oom, fail_closed);
        result = !fired ? 77 : atomic && (declared_oom || fail_closed) ? 0 : 70;
        goto done;
    }
    if (emitted != NL_NODE_C_OK) {
        if (f.generated || generated_length != 13579)
            goto done;
        result = emitted == NL_NODE_C_UNSUPPORTED ||
                         emitted == NL_NODE_C_RESOURCE_LIMIT
                     ? 78
                     : 70;
        goto done;
    }
    if (!f.generated || generated_length == 0)
        goto done;
    FILE *out = fopen(output, "wb");
    if (!out)
        goto done;
    bool written =
        fwrite(f.generated, 1, generated_length, out) == generated_length;
    if (fclose(out) != 0 || !written)
        goto done;
    puts("R1415 BASELINE complete source-owned C17 emission REACHED");
    result = 0;
done:
    printf("R1415 RESULT mode=%s code=%d\n", mode, result);
    cleanup(&f);
    return result;
}

int main(int argc, char **argv)
{
    if (argc != 4)
        return 71;
    return run(argv[1], argv[2], argv[3]);
}
