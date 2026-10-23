/* R756 public API registration/type-identity/rollback/OOM probe.
 * BLIND source bytes and expected results frozen in fb1d6ea59617.
 * No forged semantic root, ptr, Allocation, Domain, or public proof.
 */
#include "../support/node_checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool inject, did_fail;
static size_t seen, target;
void *__wrap_malloc(size_t n)
{
    if (inject && seen++ == target) {
        did_fail = true;
        return NULL;
    }
    return __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t k)
{
    if (inject && seen++ == target) {
        did_fail = true;
        return NULL;
    }
    return __real_calloc(n, k);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (inject && seen++ == target) {
        did_fail = true;
        return NULL;
    }
    return __real_realloc(p, n);
}

typedef struct {
    NLSource *source;
    NLParser *parser;
    NLSyntaxTree *unit;
    NLParseStatus parse;
} Input;
static bool input_open(const char *path, Input *i)
{
    CHECK(nl_source_load(path, &i->source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(i->source, &i->parser) == NL_PARSE_OK);
    i->parse = nl_parser_parse_function_unit(i->parser, &i->unit, NULL);
    printf("R756 %s parse=%d\n", path, (int)i->parse);
    return true;
}
static void input_close(Input *i)
{
    nl_syntax_tree_destroy(i->unit);
    nl_parser_destroy(i->parser);
    nl_source_destroy(i->source);
    *i = (Input){0};
}
static bool same(const NLSemanticSnapshot *a, const NLSemanticSnapshot *b)
{
#define EQ(k) (a->k == b->k)
    return EQ(types) && EQ(bindings) && EQ(values) && EQ(places) &&
           EQ(domains) && EQ(scopes) && EQ(functions) &&
           EQ(last_incarnation) && EQ(last_value_fact) &&
           EQ(backing_regions) && EQ(raw_intervals) && EQ(occurrences);
#undef EQ
}
static bool type_inspect(const NLSemanticContext *ctx, const char *link,
                         const char *a, const char *b)
{
    NLSemanticSnapshot ss = {0};
    CHECK(nl_semantic_snapshot(ctx, &ss));
    CHECK(ss.functions >= 3);
    const NLTypeId u8 = nl_semantic_core_type(ctx, NL_TYPE_U8);
    CHECK(u8 != 0);
    NLTypeId h = 0, t = 0;
    for (NLTypeId id = 1; id <= ss.types; ++id) {
        NLSemanticTypeView v = {0};
        if (!nl_semantic_type_view(ctx, id, &v) ||
            v.kind != NL_TYPE_NOMINAL || v.field_count != 2)
            continue;
        NLAggregateField f0 = {0}, f1 = {0};
        if (!nl_semantic_aggregate_field_view(ctx, id, 0, &f0) ||
            !nl_semantic_aggregate_field_view(ctx, id, 1, &f1) ||
            !f0.name || !f1.name)
            continue;
        if (strcmp(f0.name, a) == 0 && strcmp(f1.name, b) == 0) {
            CHECK(t == 0 && f0.type == u8 && f1.type == u8 &&
                  v.is_copy && v.is_discardable);
            t = id;
        }
        if (strcmp(f0.name, link) == 0 &&
            strcmp(f1.name, "payload") == 0) {
            CHECK(h == 0 && f1.type == u8);
            h = id;
        }
    }
    CHECK(h && t && h != t);
    CHECK(nl_semantic_recursive_local_type(ctx, h));
    NLTypeId p = 0;
    CHECK(nl_semantic_recursive_link_types(ctx, h, &p));
    NLSemanticTypeView pv = {0};
    CHECK(nl_semantic_type_view(ctx, p, &pv));
    CHECK(pv.kind == NL_TYPE_PTR && pv.target == h);
    printf("R756 distinct source-issued types H=%zu T=%zu ptrH=%zu "
           "T declared [0]=%s [1]=%s functions=%zu\n",
           (size_t)h, (size_t)t, (size_t)p, a, b, ss.functions);
    return true;
}
static bool positive(const char *source, const char *link,
                     const char *a, const char *b)
{
    Input i = {0};
    CHECK(input_open(source, &i) && i.parse == NL_PARSE_OK);
    NLSemanticContext *ctx = NULL;
    CHECK(nl_semantic_create(&ctx) == NL_CHECK_OK);
    const NLSyntaxTree *unit[] = {i.unit};
    NLCheckStatus result = nl_semantic_register_function_unit(ctx, unit, 1, NULL);
    printf("R756 positive REGISTER %s status=%d\n", source, (int)result);
    CHECK(result == NL_CHECK_OK);
    CHECK(type_inspect(ctx, link, a, b));
    nl_semantic_destroy(ctx);
    input_close(&i);
    TestNode n = {0};
    CHECK(node_checked_load(source, &n));
    const NLCheckedFragment *main_body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(main_body != NULL);
    printf("R756 genuine registered main artifact nodes=%zu "
           "(not joined mixed public certificate)\n",
           nl_checked_node_count(main_body));
    node_checked_destroy(&n);
    return true;
}
static bool negative(const char *bad_path, const char *good_path)
{
    Input bad = {0}, good = {0};
    CHECK(input_open(bad_path, &bad) && input_open(good_path, &good));
    CHECK(good.parse == NL_PARSE_OK);
    NLSemanticContext *ctx = NULL;
    CHECK(nl_semantic_create(&ctx) == NL_CHECK_OK);
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(ctx, &before));
    if (bad.parse == NL_PARSE_OK) {
        const NLSyntaxTree *items[] = {bad.unit};
        NLCheckStatus result =
            nl_semantic_register_function_unit(ctx, items, 1, NULL);
        printf("R756 negative %s REGISTER=%d\n", bad_path, (int)result);
        CHECK(result != NL_CHECK_OK);
    } else {
        printf("R756 negative %s PARSER rejection; "
               "cannot count as semantic CHECK rejection\n", bad_path);
    }
    CHECK(nl_semantic_snapshot(ctx, &after) && same(&before, &after));
    const NLSyntaxTree *repair[] = {good.unit};
    CHECK(nl_semantic_register_function_unit(ctx, repair, 1, NULL) ==
          NL_CHECK_OK);
    printf("R756 rollback and fresh same-context reuse PASS %s\n", bad_path);
    nl_semantic_destroy(ctx);
    input_close(&bad);
    input_close(&good);
    return true;
}
static bool fault_sweep(const char *source)
{
    Input i = {0};
    CHECK(input_open(source, &i) && i.parse == NL_PARSE_OK);
    const NLSyntaxTree *items[] = {i.unit};
    size_t hits = 0;
    for (size_t j = 0; j < 24; ++j) {
        NLSemanticContext *ctx = NULL;
        CHECK(nl_semantic_create(&ctx) == NL_CHECK_OK);
        NLSemanticSnapshot before, after;
        CHECK(nl_semantic_snapshot(ctx, &before));
        inject = true;
        did_fail = false;
        seen = 0;
        target = j;
        NLCheckStatus status =
            nl_semantic_register_function_unit(ctx, items, 1, NULL);
        inject = false;
        CHECK(nl_semantic_snapshot(ctx, &after));
        if (did_fail) {
            ++hits;
            CHECK(status != NL_CHECK_OK && same(&before, &after));
            CHECK(nl_semantic_register_function_unit(ctx, items, 1, NULL) ==
                  NL_CHECK_OK);
            printf("R756 OOM fail_at=%zu status=%d rollback/retry=OK\n",
                   j, (int)status);
        } else {
            CHECK(status == NL_CHECK_OK);
        }
        nl_semantic_destroy(ctx);
    }
    CHECK(hits >= 8);
    input_close(&i);
    return true;
}
static bool gate(int argc, char **argv)
{
    CHECK(argc == 3);
    CHECK(positive(argv[1], "successor", "incoming", "outgoing"));
    CHECK(positive(argv[2], "child", "top", "base"));
    static const char *cases[] = {
        "n_bad_second_t_type.nl", "n_bad_second_h_target.nl",
        "n_h_t_name_collision.nl", "n_two_h_nominals.nl",
        "n_two_t_nominals.nl", "n_unknown_t_field.nl",
        "n_bad_uncalled_body.nl", "n_duplicate_function.nl",
        "n_read_write_escalation.nl", "n_double_consume_owner.nl",
        "n_missing_failure_cleanup.nl"
    };
    const char *slash = strrchr(argv[1], '/');
    CHECK(slash != NULL);
    char full[4096];
    size_t prefix = (size_t)(slash - argv[1] + 1);
    for (size_t k = 0; k < sizeof(cases) / sizeof(cases[0]); ++k) {
        CHECK(prefix + strlen(cases[k]) < sizeof(full));
        memcpy(full, argv[1], prefix);
        strcpy(full + prefix, cases[k]);
        CHECK(negative(full, argv[1]));
    }
    CHECK(fault_sweep(argv[1]) && fault_sweep(argv[2]));
    puts("R756 public TypeId, registration, failure boundaries: limited PASS");
    return true;
}
int main(int argc, char **argv)
{
    return gate(argc, argv) ? 0 : 1;
}
