#include "../../src/semantic_internal.h"
#include "../support/function_body.h"
#include <stdint.h>
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}
static const char *const structural[] = {"fn", "let", "return", "match"};
static const char *const code = "P12-RESERVED-STRUCTURAL-NAME";

static bool source_header_reject(NLSemanticContext *c, const char *text,
                                 const char *name, size_t start)
{
    TestState before;
    CHECK(test_state(c, &before));
    NLSource *source = NULL;
    CHECK(nl_source_create(text, strlen(text), "reserved-header", &source) ==
          NL_SOURCE_OK);
    NLParser *parser = NULL;
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    for (size_t i = 0; i < 2; ++i) {
        NLSyntaxTree *tree = NULL;
        NLParseDiagnostic d = {0};
        CHECK(nl_parser_parse_function_unit(parser, &tree, &d) ==
                  NL_PARSE_SYNTAX_ERROR &&
              tree == NULL);
        CHECK(strcmp(d.diagnostic.code, code) == 0 &&
              strstr(d.diagnostic.message, "ordinary lexical namespace") !=
                  NULL &&
              strstr(d.diagnostic.message, "unit") == NULL);
        NLSourceView bytes;
        CHECK(d.span.start_byte == start &&
              nl_source_view(source, d.span, &bytes) &&
              bytes.length == strlen(name) &&
              memcmp(bytes.bytes, name, bytes.length) == 0);
        CHECK(test_unchanged(c, &before));
    }
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool ingress(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    NLSemanticBindingView bx;
    CHECK(body_binding(f.context, "x", &bx));
    NLPlaceId root;
    NLValueId value;
    CHECK(nl_semantic_seed_root(f.context, f.linear, f.domain, true,
                                NL_DEPENDENCY_FREE, &root,
                                &value) == NL_CHECK_OK);
    NLSymbolId ptr, ending;
    CHECK(test_reference(f.context, "p", root, NL_TYPE_PTR, NL_ACCESS_READ,
                         false, &ptr, NULL));
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending, NULL));
    NLTypeId option;
    const NLSumVariant variants[] = {{"Some", f.copy}, {"None", 0}};
    CHECK(nl_semantic_register_sum(f.context, "Option", variants, 2, &option) ==
          NL_CHECK_OK);
    CHECK(body_ok(f.context, "let option=Option.Some(x)"));
    NLSemanticBindingView ob;
    CHECK(body_binding(f.context, "option", &ob));
    NLSymbolId read;
    CHECK(test_reference(f.context, "read", ob.place, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &read, NULL));
    char text[512];
    for (size_t i = 0; i < 4; ++i) {
        const char *name = structural[i];
        CHECK(nl_ordinary_name_class(name, strlen(name)) ==
              NL_NAME_RESERVED_STRUCTURAL);
        (void)snprintf(text, sizeof(text), "fn %s()->unit{unit}", name);
        CHECK(source_header_reject(f.context, text, name, 3));
        /* Header admission must win over invalid types and body syntax. */
        (void)snprintf(text, sizeof(text), "fn f(%s:Unknown)->unit{match()}",
                       name);
        CHECK(source_header_reject(f.context, text, name, 5));
        (void)snprintf(text, sizeof(text), "let %s=x", name);
        CHECK(test_rejected(f.context, text, TEST_BINDING,
                            NL_CHECK_SEMANTIC_ERROR, code));
        CHECK(test_rejected(f.context, text, TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, code));
        (void)snprintf(text, sizeof(text), "{let %s=unit;}", name);
        CHECK(test_rejected(f.context, text, TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, code));
        (void)snprintf(text, sizeof(text), "loan read x as %s {}", name);
        CHECK(test_rejected(f.context, text, TEST_LOAN, NL_CHECK_SEMANTIC_ERROR,
                            code));
        (void)snprintf(text, sizeof(text), "let(%s,rest)=take(p,ending)", name);
        CHECK(test_rejected(f.context, text, TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, code));
        (void)snprintf(text, sizeof(text), "let(first,%s)=take(p,ending)",
                       name);
        CHECK(test_rejected(f.context, text, TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, code));
        CHECK(nl_semantic_find_binding(f.context, "first") == 0 &&
              nl_semantic_find_binding(f.context, "rest") == 0);
        NLSemanticPlaceView live;
        CHECK(nl_semantic_place_view(f.context, root, &live) && live.live);
        NLSemanticBindingView authority;
        CHECK(body_binding(f.context, "ending", &authority) &&
              authority.availability == NL_AVAILABLE);
        char type_name[32], record_name[32];
        (void)snprintf(type_name, sizeof(type_name), "Record%zu", i);
        (void)snprintf(record_name, sizeof(record_name), "record%zu", i);
        const NLAggregateField fields[] = {{"ok", f.copy}, {name, f.copy}};
        NLTypeId record;
        CHECK(nl_semantic_register_aggregate(f.context, type_name, fields, 2,
                                             &record) == NL_CHECK_OK);
        (void)snprintf(text, sizeof(text), "let %s=%s{ok:x,%s:x}", record_name,
                       type_name, name);
        CHECK(body_ok(f.context, text));
        (void)snprintf(text, sizeof(text), "let %s{ok,%s}=%s", type_name, name,
                       record_name);
        CHECK(test_rejected(f.context, text, TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, code));
        CHECK(nl_semantic_find_binding(f.context, "ok") == 0);
        (void)snprintf(text, sizeof(text),
                       "match option{Some(%s)=>{unit},None=>{unit}}", name);
        CHECK(test_rejected(f.context, text, TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, code));
        (void)snprintf(text, sizeof(text),
                       "match read{Some(%s)=>{unit},None=>{unit}}", name);
        CHECK(test_rejected(f.context, text, TEST_SOURCE,
                            NL_CHECK_SEMANTIC_ERROR, code));
        NLFunctionParameter p[] = {{"s", option}};
        (void)snprintf(
            text, sizeof(text),
            "{match s{Some(%s)=>{return unit;},None=>{return unit;}};}", name);
        CHECK(register_body(f.context, "bad_arm", p, 1, 1, text,
                            NL_CHECK_SEMANTIC_ERROR, code));
        NLFunctionParameter param[] = {{name, f.copy}};
        CHECK(register_body(f.context, "bad_parameter", param, 1, 1, "{unit}",
                            NL_CHECK_SEMANTIC_ERROR, code));
        CHECK(register_body(f.context, name, NULL, 0, 1, "{unit}",
                            NL_CHECK_SEMANTIC_ERROR, code));
        TestState before;
        CHECK(test_state(f.context, &before));
        NLSymbolId output = SIZE_MAX;
        NLDomainId domain = SIZE_MAX;
        NLPlaceId place = SIZE_MAX;
        NLTypeId type = SIZE_MAX;
        CHECK(nl_semantic_seed_value(f.context, name, f.copy,
                                     NL_DEPENDENCY_FREE,
                                     &output) == NL_CHECK_SEMANTIC_ERROR &&
              output == SIZE_MAX);
        CHECK(test_unchanged(f.context, &before));
        CHECK(nl_semantic_seed_domain(f.context, name, &output, &domain) ==
                  NL_CHECK_SEMANTIC_ERROR &&
              output == SIZE_MAX && domain == SIZE_MAX);
        CHECK(test_unchanged(f.context, &before));
        CHECK(nl_semantic_seed_slot(f.context, name, f.copy, &output, &place) ==
                  NL_CHECK_SEMANTIC_ERROR &&
              output == SIZE_MAX && place == SIZE_MAX);
        CHECK(test_unchanged(f.context, &before));
        CHECK(nl_semantic_seed_scalar(
                  f.context, name,
                  (NLScalarValue){
                      nl_semantic_core_type(f.context, NL_TYPE_BYTE), true, 7},
                  &output) == NL_CHECK_SEMANTIC_ERROR &&
              output == SIZE_MAX);
        CHECK(test_unchanged(f.context, &before));
        NLSemanticBindingView pb;
        NLSemanticValueView pv;
        CHECK(body_binding(f.context, "p", &pb) &&
              nl_semantic_value_view(f.context, pb.value, &pv));
        CHECK(nl_semantic_seed_reference(f.context, name, pb.type, pv.reference,
                                         &output) == NL_CHECK_SEMANTIC_ERROR &&
              output == SIZE_MAX);
        CHECK(test_unchanged(f.context, &before));
        CHECK(nl_semantic_register_function(f.context, name, NULL, 0, 1, false,
                                            false) == NL_CHECK_SEMANTIC_ERROR);
        CHECK(test_unchanged(f.context, &before));
        CHECK(nl_semantic_nominal(f.context, name, true, true, &type) ==
                  NL_CHECK_SEMANTIC_ERROR &&
              type == SIZE_MAX);
        CHECK(
            nl_semantic_register_aggregate(f.context, name, fields, 2, &type) ==
                NL_CHECK_SEMANTIC_ERROR &&
            type == SIZE_MAX);
        CHECK(nl_semantic_register_sum(f.context, name, variants, 2, &type) ==
                  NL_CHECK_SEMANTIC_ERROR &&
              type == SIZE_MAX);
        CHECK(test_unchanged(f.context, &before));
        TestChecked a = {0};
        CHECK(test_run(f.context, "x", TEST_SOURCE, NL_CHECK_OK, NULL, &a));
        CHECK(test_state(f.context, &before));
        CHECK(nl_semantic_bind_result(f.context, name,
                                      test_root(&a)->results[0].value,
                                      &output) == NL_CHECK_SEMANTIC_ERROR &&
              output == SIZE_MAX);
        CHECK(nl_sem_bind_in_scope(f.context, name,
                                   test_root(&a)->results[0].value, 0,
                                   &output) == NL_CHECK_SEMANTIC_ERROR &&
              output == SIZE_MAX);
        CHECK(test_unchanged(f.context, &before));
        test_checked_destroy(&a);
        CHECK(nl_semantic_find_binding(f.context, name) == 0);
    }
    CHECK(nl_ordinary_name_class("unit", 4) == NL_NAME_CORE_UNIT);
    CHECK(test_rejected(f.context, "let unit=x", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P10-RESERVED-NAME"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool header(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    CHECK(source_header_reject(
        f.context, "fn match()->unit{unit} fn caller()->unit{match()}", "match",
        3));
    char text[512];
    for (size_t i = 0; i < 4; ++i) {
        const char *name = structural[i];
        const char *prefix = "fn valid()->unit{unit} fn ";
        (void)snprintf(text, sizeof(text), "%s%s()->unit{unit}", prefix, name);
        CHECK(source_header_reject(f.context, text, name, strlen(prefix)));
        prefix = "fn valid()->unit{unit} fn invalid(ok:CopyT,";
        (void)snprintf(text, sizeof(text), "%s%s:CopyT)->unit{unit}", prefix,
                       name);
        CHECK(source_header_reject(f.context, text, name, strlen(prefix)));
    }
    const char *malformed[] = {"fn()",     "{fn();}",   "let()",
                               "{let();}", "return()",  "{return();}",
                               "match()",  "{match();}"};
    for (size_t i = 0; i < sizeof(malformed) / sizeof(malformed[0]); ++i) {
        NLSource *source = NULL;
        NLParser *parser = NULL;
        NLSyntaxTree *tree = NULL;
        CHECK(nl_source_create(malformed[i], strlen(malformed[i]),
                               "no-fallback", &source) == NL_SOURCE_OK);
        CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
        CHECK(nl_parser_parse_source_fragment(parser, &tree, NULL) !=
                  NL_PARSE_OK &&
              tree == NULL);
        nl_parser_destroy(parser);
        nl_source_destroy(source);
    }
    nl_semantic_destroy(f.context);
    return true;
}
static bool register_unit_ok(NLSemanticContext *c, const char *text)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_create(text, strlen(text), "ordinary-unit", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    const NLSyntaxTree *inputs[] = {tree};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus status = nl_semantic_register_function_unit(c, inputs, 1, &d);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "%s: %d %s\n", text, status,
                d.diagnostic.diagnostic.code);
    CHECK(status == NL_CHECK_OK);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool member(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    const NLSumVariant variants[] = {
        {"fn", 0}, {"let", 0}, {"return", 0}, {"match", 0}, {"unit", 0}};
    NLTypeId code_type, record;
    CHECK(nl_semantic_register_sum(f.context, "Code", variants, 5,
                                   &code_type) == NL_CHECK_OK);
    const NLAggregateField fields[] = {{"fn", f.copy},
                                       {"let", f.copy},
                                       {"return", f.copy},
                                       {"match", f.copy},
                                       {"unit", f.copy}};
    CHECK(nl_semantic_register_aggregate(f.context, "Record", fields, 5,
                                         &record) == NL_CHECK_OK);
    CHECK(body_ok(f.context,
                  "let record=Record{fn:x,let:x,return:x,match:x,unit:x}"));
    CHECK(register_unit_ok(
        f.context, "fn run(s:Code)->unit{let copy=s;match copy{fn=>{return "
                   "unit;},let=>{return unit;},return=>{return "
                   "unit;},match=>{return unit;},unit=>{return unit;}};}"));
    const char *labels[] = {"fn", "let", "return", "match", "unit"};
    char text[512];
    for (size_t i = 0; i < 5; ++i) {
        (void)snprintf(text, sizeof(text), "let chosen%zu=Code.%s", i,
                       labels[i]);
        CHECK(body_ok(f.context, text));
        (void)snprintf(text, sizeof(text), "run(chosen%zu)", i);
        CHECK(body_ok(f.context, text));
        (void)snprintf(text, sizeof(text),
                       "match "
                       "chosen%zu{fn=>{unit},let=>{unit},return=>{unit},match=>"
                       "{unit},unit=>{unit}}",
                       i);
        CHECK(body_ok(f.context, text));
        CHECK(nl_semantic_find_binding(f.context, labels[i]) == 0);
    }
    const char *near[] = {"Fn",    "Let",         "Return",  "Match",
                          "fn_",   "let_",        "return_", "match_",
                          "fn2",   "match_value", "unit_",   "units",
                          "Unit",  "ptr",         "ref",     "read",
                          "write", "exclusive",   "using"};
    for (size_t i = 0; i < sizeof(near) / sizeof(near[0]); ++i) {
        CHECK(nl_ordinary_name_class(near[i], strlen(near[i])) ==
              NL_NAME_ADMISSIBLE);
        (void)snprintf(text, sizeof(text), "let %s=x", near[i]);
        CHECK(body_ok(f.context, text));
        TestSemantic g = {0};
        CHECK(test_semantic_create(&g));
        CHECK(nl_semantic_seed_value(g.context, "input", g.copy,
                                     NL_DEPENDENCY_FREE, &x) == NL_CHECK_OK);
        (void)snprintf(text, sizeof(text), "fn %s(%s:CopyT)->CopyT{%s}",
                       near[i], near[i], near[i]);
        CHECK(register_unit_ok(g.context, text));
        (void)snprintf(text, sizeof(text), "%s(input)", near[i]);
        CHECK(body_ok(g.context, text));
        nl_semantic_destroy(g.context);
    }
    /* Length governs borrowed views, without NUL termination or overread. */
    const unsigned char view[] = {'m', 'a', 't', 'c', 'h', '_'};
    CHECK(nl_ordinary_name_class(view, 5) == NL_NAME_RESERVED_STRUCTURAL &&
          nl_ordinary_name_class(view, 6) == NL_NAME_ADMISSIBLE &&
          nl_ordinary_name_class(view, 4) == NL_NAME_ADMISSIBLE);
    nl_semantic_destroy(f.context);
    return true;
}
static bool check_faults(NLSemanticContext *c, const char *text, bool unit)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    NLParser *parser = NULL;
    CHECK(nl_source_create(text, strlen(text), "semantic-oom", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    if (unit)
        CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) ==
              NL_PARSE_OK);
    else
        CHECK(nl_parser_parse_source_fragment(parser, &tree, NULL) ==
              NL_PARSE_OK);
    nl_parser_destroy(parser);
    TestState before;
    CHECK(test_state(c, &before));
    bool complete = false;
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        NLCheckDiagnostic d = {0};
        NLFunctionUnitDiagnostic u = {0};
        const NLSyntaxTree *inputs[] = {tree};
        injecting = true;
        allocation_index = 0;
        NLCheckStatus status =
            unit ? nl_semantic_register_function_unit(c, inputs, 1, &u)
                 : nl_semantic_check_source_fragment(c, tree, &artifact, &d);
        injecting = false;
        if (status == NL_CHECK_SEMANTIC_ERROR) {
            const char *diagnostic =
                unit ? u.diagnostic.diagnostic.code : d.diagnostic.code;
            CHECK(strcmp(diagnostic, code) == 0 && artifact == NULL &&
                  test_unchanged(c, &before));
            complete = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(c, &before));
    }
    CHECK(complete && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
static bool failure(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x, r;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    NLSemanticBindingView bx;
    CHECK(body_binding(f.context, "x", &bx));
    CHECK(test_reference(f.context, "rw", bx.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &r, NULL));
    const NLAggregateField fields[] = {{"ok", f.copy}, {"match", f.copy}};
    NLTypeId record, option;
    CHECK(nl_semantic_register_aggregate(f.context, "Record", fields, 2,
                                         &record) == NL_CHECK_OK);
    CHECK(body_ok(f.context, "let record=Record{ok:x,match:x}"));
    const NLSumVariant variants[] = {{"Some", f.copy}, {"None", 0}};
    CHECK(nl_semantic_register_sum(f.context, "Option", variants, 2, &option) ==
          NL_CHECK_OK);
    CHECK(body_ok(f.context, "let option=Option.Some(x)"));
    CHECK(check_faults(f.context, "{store(rw,x);let fn=unit;}", false));
    CHECK(check_faults(f.context, "let(first,return)=unknown()", false));
    CHECK(check_faults(f.context, "let Record{ok,match}=record", false));
    CHECK(check_faults(f.context,
                       "match option{Some(let)=>{unit},None=>{unit}}", false));
    CHECK(check_faults(
        f.context,
        "fn valid()->unit{unit} fn invalid(x:CopyT)->unit{let match=x;}",
        true));
    const char *headers[] = {
        "fn valid()->unit{unit} fn match()->unit{unit}",
        "fn valid()->unit{unit} fn invalid(ok:CopyT,return:CopyT)->unit{unit}"};
    for (size_t i = 0; i < 2; ++i) {
        NLSource *source = NULL;
        NLParser *parser = NULL;
        CHECK(nl_source_create(headers[i], strlen(headers[i]), "header-oom",
                               &source) == NL_SOURCE_OK);
        CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
        bool complete = false;
        for (fail_at = 0; fail_at < 1000; ++fail_at) {
            NLSyntaxTree *tree = NULL;
            NLParseDiagnostic d = {0};
            injecting = true;
            allocation_index = 0;
            NLParseStatus status =
                nl_parser_parse_function_unit(parser, &tree, &d);
            injecting = false;
            CHECK(tree == NULL);
            if (status == NL_PARSE_SYNTAX_ERROR) {
                CHECK(strcmp(d.diagnostic.code, code) == 0);
                complete = true;
                break;
            }
            CHECK(status == NL_PARSE_OUT_OF_MEMORY);
        }
        CHECK(complete && fail_at > 0);
        nl_parser_destroy(parser);
        nl_source_destroy(source);
    }
    /* Seed/binder may allocate before admission inside a private candidate. */
    TestState before;
    CHECK(test_state(f.context, &before));
    bool complete = false;
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        NLSymbolId out = SIZE_MAX;
        injecting = true;
        allocation_index = 0;
        NLCheckStatus status = nl_semantic_seed_value(
            f.context, "match", f.copy, NL_DEPENDENCY_FREE, &out);
        injecting = false;
        CHECK(out == SIZE_MAX && test_unchanged(f.context, &before));
        if (status == NL_CHECK_SEMANTIC_ERROR) {
            complete = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY);
    }
    CHECK(complete && fail_at > 0);
    nl_semantic_destroy(f.context);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "ingress") == 0)
        return ingress() ? 0 : 1;
    if (strcmp(argv[1], "header") == 0)
        return header() ? 0 : 1;
    if (strcmp(argv[1], "member") == 0)
        return member() ? 0 : 1;
    if (strcmp(argv[1], "failure") == 0)
        return failure() ? 0 : 1;
    return 2;
}
