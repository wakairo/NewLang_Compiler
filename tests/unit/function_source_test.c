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
static bool unit_tree(const char *text, const char *path, NLSource **source,
                      NLSyntaxTree **tree)
{
    CHECK(nl_source_create(text, strlen(text), path, source) == NL_SOURCE_OK);
    NLParser *parser = NULL;
    CHECK(nl_parser_create(*source, &parser) == NL_PARSE_OK);
    NLParseDiagnostic d = {0};
    NLParseStatus status = nl_parser_parse_function_unit(parser, tree, &d);
    if (status != NL_PARSE_OK)
        fprintf(stderr, "parse %s: %s\n", text, d.diagnostic.code);
    CHECK(status == NL_PARSE_OK);
    nl_parser_destroy(parser);
    return true;
}
static bool unit_register(NLSemanticContext *c, const char *text,
                          NLCheckStatus expected, const char *code)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(unit_tree(text, "function-unit", &source, &tree));
    TestState before;
    CHECK(test_state(c, &before));
    NLFunctionUnitDiagnostic d = {0};
    const NLSyntaxTree *inputs[] = {tree};
    NLCheckStatus status = nl_semantic_register_function_unit(c, inputs, 1, &d);
    if (status != expected)
        fprintf(stderr, "%s: got %d expected %d (%s)\n", text, status, expected,
                d.diagnostic.diagnostic.code);
    CHECK(status == expected);
    if (status != NL_CHECK_OK) {
        CHECK(test_unchanged(c, &before) && d.input_index == 0 &&
              nl_source_span_valid(source, d.diagnostic.span));
        if (code != NULL) {
            if (strcmp(code, d.diagnostic.diagnostic.code) != 0)
                fprintf(stderr, "wanted %s got %s\n", code,
                        d.diagnostic.diagnostic.code);
            CHECK(strcmp(code, d.diagnostic.diagnostic.code) == 0);
        }
    }
    /* Calls below must survive the destruction of every input owner. */
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
static bool malformed(const char *text, NLParseStatus expected,
                      const char *code)
{
    NLSource *source = NULL;
    CHECK(nl_source_create(text, strlen(text), "malformed-fn", &source) ==
          NL_SOURCE_OK);
    NLParser *parser = NULL;
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    NLSyntaxTree *tree = NULL;
    NLParseDiagnostic d = {0};
    for (size_t i = 0; i < 2; ++i) {
        NLParseStatus status = nl_parser_parse_function_unit(parser, &tree, &d);
        if (status != expected || strcmp(d.diagnostic.code, code) != 0)
            fprintf(stderr, "%s: parse %d %s wanted %d %s\n", text, status,
                    d.diagnostic.code, expected, code);
        CHECK(status == expected && tree == NULL &&
              strcmp(d.diagnostic.code, code) == 0 &&
              nl_source_span_valid(source, d.span));
    }
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool grammar(void)
{
    const char *errors[][2] = {
        {"", "P11-DECLARATION"},
        {"fn f(x:CopyT,) -> unit {}", "P11-TRAILING-COMMA"},
        {"fn f(x:CopyT y:CopyT) -> unit {}", "P11-PARAMETERS"},
        {"fn f(x CopyT) -> unit {}", "P11-PARAMETER-COLON"},
        {"fn f(x=CopyT) -> unit {}", "P11-PARAMETER-COLON"},
        {"fn f() -> {}", "P2-EXPECTED-TYPE"},
        {"fn f() unit {}", "P11-RESULT-ARROW"},
        {"fn f() - > unit {}", "P11-RESULT-ARROW"},
        {"fn f() -> unit {};", "P11-DECLARATION-SEMICOLON"},
        {"fn f() -> unit", "P11-BODY"},
        {"fn valid()->unit{} fn invalid(x CopyT)->unit{}",
         "P11-PARAMETER-COLON"}};
    for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i)
        CHECK(malformed(errors[i][0], NL_PARSE_SYNTAX_ERROR, errors[i][1]));
    CHECK(malformed("fn f<T>(x:T)->T{x}", NL_PARSE_SYNTAX_UNSUPPORTED,
                    "P11-GENERIC-DECLARATION"));
    CHECK(malformed("fn outer()->unit{fn inner()->unit{}}",
                    NL_PARSE_SYNTAX_UNSUPPORTED, "P11-LOCAL-DECLARATION"));
    CHECK(malformed("fn f(x:List<CopyT>)->unit{}", NL_PARSE_SYNTAX_UNSUPPORTED,
                    "P2-SYNTAX-UNSUPPORTED"));
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    CHECK(unit_register(
        f.context,
        "fn ping ( ) -> unit {return unit;} fn id(x : CopyT)->CopyT{x} "
        "fn multi(a:CopyT,b:CopyT)->CopyT{a; b}",
        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "ping()"));
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(unit_tree("fn one(a:ptr<CopyT>,b:ref<read,CopyT>)->unit{}", "inspect",
                    &source, &tree));
    const NLSyntaxView *root = nl_syntax_node_view(nl_syntax_tree_root(tree));
    const NLSyntaxView *decl =
        nl_syntax_node_view(root->data.function_unit.declarations);
    CHECK(root->kind == NL_SYNTAX_FUNCTION_UNIT &&
          root->data.function_unit.count == 1 &&
          decl->kind == NL_SYNTAX_FUNCTION && decl->data.function.count == 2);
    const NLSyntaxNode *first = decl->data.function.parameters;
    CHECK(nl_syntax_node_view(nl_syntax_node_view(first)->data.parameter.type)
              ->kind == NL_SYNTAX_TYPE_PTR);
    CHECK(
        nl_syntax_node_view(nl_syntax_node_view(nl_syntax_next_argument(first))
                                ->data.parameter.type)
            ->kind == NL_SYNTAX_TYPE_REF);
    CHECK(nl_syntax_node_view(decl->data.function.body)->kind ==
          NL_SYNTAX_BLOCK);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    /* fn remains contextual and can be an ordinary function call name. */
    CHECK(unit_register(f.context, "fn fn()->unit{} fn use_fn()->unit{fn();}",
                        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "use_fn()"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool names(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    const char *duplicates[] = {"fn same()->unit{} fn same()->unit{}",
                                "fn same()->unit{} fn same(x:CopyT)->CopyT{x}",
                                "fn CopyT()->unit{}", "fn life()->unit{}",
                                "fn store()->unit{}"};
    for (size_t i = 0; i < sizeof(duplicates) / sizeof(duplicates[0]); ++i)
        CHECK(unit_register(f.context, duplicates[i], NL_CHECK_SEMANTIC_ERROR,
                            "P11-DUPLICATE-FUNCTION"));
    CHECK(unit_register(f.context, "fn f(a:CopyT,a:CopyT)->CopyT{a}",
                        NL_CHECK_SEMANTIC_ERROR, "P11-DUPLICATE-PARAMETER"));
    CHECK(unit_register(f.context, "fn unit()->unit{}", NL_CHECK_SEMANTIC_ERROR,
                        "P10-RESERVED-NAME"));
    CHECK(unit_register(f.context, "fn f(unit:CopyT)->unit{}",
                        NL_CHECK_SEMANTIC_ERROR, "P10-RESERVED-NAME"));
    CHECK(unit_register(f.context, "fn f()->unit{let unit=unit;}",
                        NL_CHECK_SEMANTIC_ERROR, "P10-RESERVED-NAME"));
    CHECK(unit_register(f.context, "fn f(x:Unknown)->unit{}",
                        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-TYPE"));
    CHECK(unit_register(f.context, "fn f(x:CopyT)->ref<read,CopyT>{x}",
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P8-SIGNATURE-PRECISION"));
    CHECK(unit_register(f.context, "fn a()->unit{} fn z()->CopyT{unit}",
                        NL_CHECK_SEMANTIC_ERROR, "P8-BODY-RESULT"));
    CHECK(unit_register(f.context,
                        "fn a()->unit{} fn z(x:CopyT)->CopyT{return unit;}",
                        NL_CHECK_SEMANTIC_ERROR, "P9-RETURN-TYPE"));
    CHECK(unit_register(f.context, "fn valid()->unit{}", NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "valid()"));
    CHECK(test_rejected(f.context, "CopyT.valid()", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P6-SUM-QUALIFIER"));
    NLTypeId labels;
    const NLSumVariant member[] = {{"valid", 0}};
    CHECK(nl_semantic_register_sum(f.context, "Labels", member, 1, &labels) ==
          NL_CHECK_OK);
    CHECK(body_ok(f.context, "let member=Labels.valid"));
    CHECK(unit_register(f.context, "fn valid()->unit{}",
                        NL_CHECK_SEMANTIC_ERROR, "P11-DUPLICATE-FUNCTION"));
    nl_semantic_destroy(f.context);
    return true;
}
static size_t checked_function(const TestChecked *a)
{
    return test_root(a)->function;
}
static bool visibility(void)
{
    const char *parts[] = {"fn first(x:LinearT)->LinearT{second(x)}",
                           "fn second(x:LinearT)->LinearT{third(x)}",
                           "fn third(x:LinearT)->LinearT{x}"};
    size_t ids[4] = {0};
    for (size_t order = 0; order < 4; ++order) {
        TestSemantic f = {0};
        CHECK(test_semantic_create(&f));
        if (order < 2) {
            char text[512] = "";
            for (size_t i = 0; i < 3; ++i)
                strcat(text, parts[order == 0 ? i : 2 - i]);
            CHECK(unit_register(f.context, text, NL_CHECK_OK, NULL));
        } else {
            NLSource *sources[3] = {0};
            NLSyntaxTree *trees[3] = {0};
            const NLSyntaxTree *inputs[3];
            for (size_t i = 0; i < 3; ++i) {
                CHECK(unit_tree(parts[i], i == 0 ? "z.nl" : "a.nl", &sources[i],
                                &trees[i]));
                inputs[order == 2 ? i : 2 - i] = trees[i];
            }
            CHECK(nl_semantic_register_function_unit(f.context, inputs, 3,
                                                     NULL) == NL_CHECK_OK);
            for (size_t i = 0; i < 3; ++i) {
                nl_syntax_tree_destroy(trees[i]);
                nl_source_destroy(sources[i]);
            }
        }
        NLSymbolId x;
        CHECK(nl_semantic_seed_value(f.context, "input", f.linear,
                                     NL_DEPENDENCY_FREE, &x) == NL_CHECK_OK);
        NLSemanticBindingView original, result, consumed;
        CHECK(body_binding(f.context, "input", &original));
        TestChecked a = {0};
        CHECK(test_run(f.context, "first(input)", TEST_SOURCE, NL_CHECK_OK,
                       NULL, &a));
        ids[order] = checked_function(&a);
        CHECK(test_root(&a)->kind == NL_CHECKED_REGISTERED_CALL &&
              test_root(&a)->body_backed &&
              test_root(&a)->results[0].value == original.value);
        CHECK(nl_semantic_bind_result(f.context, "output", original.value,
                                      &x) == NL_CHECK_OK);
        CHECK(body_binding(f.context, "output", &result) &&
              result.value == original.value);
        CHECK(body_binding(f.context, "input", &consumed) &&
              consumed.availability == NL_CONSUMED);
        const NLCheckedFragment *body =
            nl_checked_call_body(a.artifact, nl_checked_root(a.artifact));
        const NLCheckedNodeView *block =
            nl_checked_node_view(body, nl_checked_root(body));
        CHECK(nl_checked_call_body(body, block->tail) != NULL);
        CHECK(nl_semantic_find_binding(f.context, "x") == 0);
        nl_semantic_destroy(f.context);
        /* Plans retained by nested checked bodies outlive public context. */
        NLSourceView bytes;
        CHECK(nl_source_view(nl_checked_source(body), (NLSourceSpan){0, 1},
                             &bytes) &&
              bytes.bytes[0] == '{');
        test_checked_destroy(&a);
    }
    CHECK(ids[0] == ids[1] && ids[0] == ids[2] && ids[0] == ids[3]);
    return true;
}
static bool precision(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    const char *cycles[] = {
        "fn recur(x:LinearT)->LinearT{recur(x)}",
        "fn a(x:LinearT)->LinearT{b(x)} fn b(x:LinearT)->LinearT{a(x)}",
        "fn b(x:LinearT)->LinearT{a(x)} fn a(x:LinearT)->LinearT{b(x)}"};
    for (size_t i = 0; i < sizeof(cycles) / sizeof(cycles[0]); ++i)
        CHECK(unit_register(f.context, cycles[i],
                            NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                            "P11-RECURSIVE-ANALYSIS-PRECISION"));
    CHECK(unit_register(f.context, "fn unknown(x:LinearT)->LinearT{missing(x)}",
                        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-CALLEE"));
    CHECK(unit_register(f.context, "fn lose(x:LinearT)->unit{}",
                        NL_CHECK_SEMANTIC_ERROR, "P5-SCOPE-OBLIGATION"));
    CHECK(unit_register(f.context, "fn borrow(r:ref<read,CopyT>)->CopyT{r}",
                        NL_CHECK_SEMANTIC_ERROR, "P8-BODY-RESULT"));
    CHECK(nl_semantic_register_function(f.context, "hidden", &f.copy, 1, 1,
                                        false, true) == NL_CHECK_OK);
    CHECK(unit_register(f.context, "fn bad(x:CopyT)->unit{hidden(x);}",
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P3-DEPENDENCIES-UNSUPPORTED"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool behavior(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    CHECK(unit_register(
        f.context,
        "fn exchange(a:ref<write,CopyT>,b:ref<write,CopyT>)->unit{worker(a,b);}"
        "fn worker(a:ref<write,CopyT>,b:ref<write,CopyT>)->unit{swap(a,b);}"
        "fn install(dst:ref<write,CopyT>,v:CopyT)->CopyT{replace(dst,v)}"
        "fn early(x:LinearT)->LinearT{{return x;};x}"
        "fn copied(x:CopyT)->CopyT{x}",
        NL_CHECK_OK, NULL));
    NLSymbolId x, y, ra, rb;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f.context, "y", f.copy, NL_DEPENDENCY_FREE,
                                 &y) == NL_CHECK_OK);
    NLSemanticBindingView bx, by;
    CHECK(body_binding(f.context, "x", &bx) &&
          body_binding(f.context, "y", &by));
    CHECK(test_reference(f.context, "ra", bx.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &ra, NULL));
    CHECK(test_reference(f.context, "rb", by.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &rb, NULL));
    CHECK(body_ok(f.context, "exchange(ra,rb)"));
    NLSemanticPlaceView ax, ay, noop;
    CHECK(nl_semantic_place_view(f.context, bx.place, &ax) &&
          ax.current_value == by.value);
    CHECK(nl_semantic_place_view(f.context, by.place, &ay) &&
          ay.current_value == bx.value);
    CHECK(body_ok(f.context, "exchange(ra,ra)"));
    CHECK(nl_semantic_place_view(f.context, bx.place, &noop) &&
          test_place_equal(ax, noop));
    CHECK(body_ok(f.context, "let old=install(ra,y)"));
    NLSemanticBindingView old;
    CHECK(body_binding(f.context, "old", &old) &&
          old.value == ax.current_value);
    CHECK(body_ok(f.context, "let copy=copied(y)"));
    NLSemanticBindingView copy;
    CHECK(body_binding(f.context, "copy", &copy) && copy.value != by.value);
    CHECK(body_binding(f.context, "y", &by) && by.availability == NL_AVAILABLE);
    CHECK(nl_semantic_seed_value(f.context, "linear", f.linear,
                                 NL_DEPENDENCY_FREE, &x) == NL_CHECK_OK);
    NLSemanticBindingView linear;
    CHECK(body_binding(f.context, "linear", &linear));
    CHECK(body_ok(f.context, "let returned=early(linear)"));
    CHECK(body_binding(f.context, "returned", &old) &&
          old.value == linear.value);
    CHECK(body_binding(f.context, "linear", &linear) &&
          linear.availability == NL_CONSUMED);
    /* Actual may-set failure remains body-sensitive and atomic across calls. */
    NLSemanticBindingView ref;
    CHECK(body_binding(f.context, "rb", &ref));
    NLValueId joined;
    CHECK(nl_semantic_join_references(f.context, &ref.value, 1, &joined) ==
          NL_CHECK_OK);
    CHECK(nl_semantic_bind_result(f.context, "joined", joined, &x) ==
          NL_CHECK_OK);
    CHECK(unit_register(f.context,
                        "fn "
                        "late(a:ref<write,CopyT>,b:ref<write,CopyT>,v:CopyT)->"
                        "unit{store(a,v);worker(a,b);}",
                        NL_CHECK_OK, NULL));
    CHECK(test_rejected(f.context, "late(ra,joined,y)", TEST_SOURCE,
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P7-SINGULAR-REF-PRECISION"));
    /* Definition flag must propagate across a body chain with return/match. */
    NLTypeId choice;
    const NLSumVariant variants[] = {{"Fail", 0}, {"Good", 0}};
    CHECK(nl_semantic_register_sum(f.context, "Choice", variants, 2, &choice) ==
          NL_CHECK_OK);
    CHECK(unit_register(f.context,
                        "fn caller(s:Choice,x:CopyT)->CopyT{decoder(s,x)}"
                        "fn decoder(s:Choice,x:CopyT)->CopyT{match "
                        "s{Fail=>{return x;},Good=>{x}}}",
                        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "let fail=Choice.Fail"));
    CHECK(body_ok(f.context, "let good=Choice.Good"));
    CHECK(body_ok(f.context, "let result_fail=caller(fail,y)"));
    CHECK(body_ok(f.context, "let result_good=caller(good,y)"));
    CHECK(unit_register(f.context, "fn exit_unit()->unit{return unit;}",
                        NL_CHECK_OK, NULL));
    CHECK(nl_semantic_seed_value(f.context, "dependent", f.copy,
                                 NL_HIDDEN_DEPENDENCIES, &x) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "exit_unit()", TEST_SOURCE,
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P3-DEPENDENCIES-UNSUPPORTED"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool semantic_faults(NLSemanticContext *c, const char *text,
                            NLCheckStatus expected)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(unit_tree(text, "fault-injection", &source, &tree));
    const NLSyntaxTree *inputs[] = {tree};
    TestState before;
    CHECK(test_state(c, &before));
    bool complete = false;
    for (fail_at = 0; fail_at < 30000; ++fail_at) {
        NLFunctionUnitDiagnostic d = {0};
        injecting = true;
        allocation_index = 0;
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, &d);
        injecting = false;
        if (status == expected) {
            if (expected != NL_CHECK_OK)
                CHECK(test_unchanged(c, &before));
            complete = true;
            break;
        }
        if (status != NL_CHECK_OUT_OF_MEMORY)
            fprintf(stderr, "fault %zu got %d %s\n", fail_at, status,
                    d.diagnostic.diagnostic.code);
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && test_unchanged(c, &before));
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
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    const char *text = "fn first(x:CopyT)->CopyT{second(x)} fn "
                       "second(x:CopyT)->CopyT{return x;}";
    CHECK(nl_source_create(text, strlen(text), "parse-oom", &source) ==
          NL_SOURCE_OK);
    NLParser *parser = NULL;
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    bool complete = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        injecting = true;
        allocation_index = 0;
        NLParseStatus status =
            nl_parser_parse_function_unit(parser, &tree, NULL);
        injecting = false;
        if (status == NL_PARSE_OK) {
            complete = true;
            break;
        }
        CHECK(status == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
    }
    CHECK(complete && fail_at > 0);
    nl_parser_destroy(parser);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    CHECK(semantic_faults(f.context,
                          "fn a()->unit{} fn z(x:CopyT)->CopyT{unit}",
                          NL_CHECK_SEMANTIC_ERROR));
    CHECK(semantic_faults(f.context, "fn a()->unit{} fn a(x:CopyT)->CopyT{x}",
                          NL_CHECK_SEMANTIC_ERROR));
    CHECK(semantic_faults(f.context, text, NL_CHECK_OK));
    NLSymbolId x;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    source = NULL;
    tree = NULL;
    CHECK(body_tree("let output=first(x)", &source, &tree));
    TestState before;
    CHECK(test_state(f.context, &before));
    complete = false;
    for (fail_at = 0; fail_at < 30000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        injecting = true;
        allocation_index = 0;
        NLCheckStatus status =
            nl_semantic_check_source_fragment(f.context, tree, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(artifact);
            complete = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(f.context, &before));
    }
    CHECK(complete && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    /* Diagnostic source index/span belongs to the failing physical input. */
    NLSource *sources[2] = {0};
    NLSyntaxTree *trees[2] = {0};
    CHECK(unit_tree("fn unique()->unit{}", "valid.nl", &sources[0], &trees[0]));
    CHECK(unit_tree("\n fn invalid(x:CopyT)->CopyT{return unit;}", "invalid.nl",
                    &sources[1], &trees[1]));
    const NLSyntaxTree *inputs[] = {trees[0], trees[1]};
    NLFunctionUnitDiagnostic d = {0};
    CHECK(test_state(f.context, &before));
    CHECK(nl_semantic_register_function_unit(f.context, inputs, 2, &d) ==
          NL_CHECK_SEMANTIC_ERROR);
    CHECK(d.input_index == 1 &&
          nl_source_span_valid(sources[1], d.diagnostic.span) &&
          test_unchanged(f.context, &before));
    NLSourceView bytes;
    CHECK(nl_source_view(sources[1], d.diagnostic.span, &bytes) &&
          bytes.length >= 6 && memcmp(bytes.bytes, "return", 6) == 0);
    for (size_t i = 0; i < 2; ++i) {
        nl_syntax_tree_destroy(trees[i]);
        nl_source_destroy(sources[i]);
    }
    nl_semantic_destroy(f.context);
    return true;
}
static bool limits(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(unit_tree("fn empty()->unit{}", "budget", &source, &tree));
    const NLSyntaxTree *inputs[NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS + 1];
    for (size_t i = 0; i < NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS + 1; ++i)
        inputs[i] = tree;
    TestState before;
    CHECK(test_state(f.context, &before));
    NLFunctionUnitDiagnostic d = {0};
    CHECK(nl_semantic_register_function_unit(
              f.context, inputs, NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS + 1,
              &d) == NL_CHECK_RESOURCE_LIMIT);
    CHECK(test_unchanged(f.context, &before));
    CHECK(nl_semantic_register_function_unit(NULL, inputs, 1, NULL) ==
          NL_CHECK_INTERNAL_ERROR);
    CHECK(nl_semantic_register_function_unit(f.context, NULL, 1, NULL) ==
          NL_CHECK_INTERNAL_ERROR);
    CHECK(nl_semantic_register_function_unit(f.context, inputs, 0, NULL) ==
          NL_CHECK_INTERNAL_ERROR);
    /* Cross-input duplicate detection is unit-wide, not file-local. */
    CHECK(nl_semantic_register_function_unit(f.context, inputs, 2, &d) ==
          NL_CHECK_SEMANTIC_ERROR);
    CHECK(strcmp(d.diagnostic.diagnostic.code, "P11-DUPLICATE-FUNCTION") == 0 &&
          test_unchanged(f.context, &before));
    d.input_index = SIZE_MAX;
    CHECK(nl_semantic_register_function_unit(f.context, inputs, 1, &d) ==
              NL_CHECK_OK &&
          d.input_index == SIZE_MAX);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    char chain[8192] = "";
    for (size_t i = 0; i < NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS; ++i) {
        char declaration[80];
        if (i + 1 == NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS)
            (void)snprintf(declaration, sizeof(declaration),
                           "fn f%zu()->unit{{{unit}}}", i);
        else
            (void)snprintf(declaration, sizeof(declaration),
                           "fn f%zu()->unit{f%zu()}", i, i + 1);
        strcat(chain, declaration);
    }
    CHECK(unit_register(f.context, chain, NL_CHECK_RESOURCE_LIMIT,
                        "P3-DEPTH-LIMIT"));
    chain[0] = 0;
    for (size_t i = 0; i < 14; ++i) {
        char declaration[100];
        if (i == 13)
            (void)snprintf(declaration, sizeof(declaration),
                           "fn w%zu()->unit{}", i);
        else
            (void)snprintf(declaration, sizeof(declaration),
                           "fn w%zu()->unit{w%zu();w%zu();}", i, i + 1, i + 1);
        strcat(chain, declaration);
    }
    CHECK(unit_register(f.context, chain, NL_CHECK_RESOURCE_LIMIT,
                        "P11-BODY-WORK-LIMIT"));
    nl_semantic_destroy(f.context);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "grammar") == 0)
        return grammar() ? 0 : 1;
    if (strcmp(argv[1], "names") == 0)
        return names() ? 0 : 1;
    if (strcmp(argv[1], "visibility") == 0)
        return visibility() ? 0 : 1;
    if (strcmp(argv[1], "precision") == 0)
        return precision() ? 0 : 1;
    if (strcmp(argv[1], "behavior") == 0)
        return behavior() ? 0 : 1;
    if (strcmp(argv[1], "limits") == 0)
        return limits() ? 0 : 1;
    if (strcmp(argv[1], "failure") == 0)
        return failure() ? 0 : 1;
    return 2;
}
