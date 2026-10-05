#include "../support/semantic_check.h"

#include <stdint.h>
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t size)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_malloc(size);
}
void *__wrap_realloc(void *p, size_t size)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_realloc(p, size);
}

static bool parse_case(const char *text, NLParseStatus expected)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLParseDiagnostic d = {0}, first = {0};
    CHECK(nl_source_create(text, strlen(text), "p5", &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    for (size_t i = 0; i < 2; ++i) {
        const NLParseStatus status =
            nl_parser_parse_source_fragment(parser, &tree, &d);
        if (status != expected)
            fprintf(stderr, "parse %s: %d, expected %d (%s)\n", text, status,
                    expected, d.diagnostic.code);
        CHECK(status == expected);
        if (expected == NL_PARSE_OK) {
            CHECK(tree != NULL && nl_syntax_tree_source(tree) == source);
            nl_syntax_tree_destroy(tree);
            tree = NULL;
        } else {
            CHECK(tree == NULL && nl_source_span_valid(source, d.span));
            if (i == 0)
                first = d;
            else
                CHECK(first.span.start_byte == d.span.start_byte &&
                      first.span.end_byte == d.span.end_byte &&
                      strcmp(first.diagnostic.code, d.diagnostic.code) == 0);
        }
    }
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}

static bool parser_tests(void)
{
    const char *valid[] = {
        "{}",
        "{ x; x }",
        "{ let x = y; x }",
        "{let(a,b)=take(p,d); let Pair{a,b,}=pair; Pair{b:b,a:a,}}",
        "let(a,b,c)=f()",
        "let _=x",
        "Pair{_:x}",
        "let Pair{_}=x",
        "let Pair{a,b} = pair;",
        "f({let x=y;x},Pair{a:x,b:y})"};
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i)
        CHECK(parse_case(valid[i], NL_PARSE_OK));
    const char *invalid[] = {"{let x=y\nx}", "{let x=y}",     "{x;y",
                             "let (a)=f()",  "let(a,b,)=f()", "let(a b)=f()",
                             "let(a,b)f()",  "Pair{}",        "Pair{a:}",
                             "Pair{a:x b:y}"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        CHECK(parse_case(invalid[i], NL_PARSE_SYNTAX_ERROR));
    const char *unsupported[] = {"let(a,_)=f()",
                                 "let(a,(b,c))=f()",
                                 "let Pair{a:renamed,b}=p",
                                 "x+y",
                                 "Option<u32>.None",
                                 "match x {None=>{x}}",
                                 "struct Pair{a:T}",
                                 "{ |x:T| x }",
                                 "let [a,b]=x",
                                 "Pair{a,b}",
                                 "let Pair{a,..}=p",
                                 "let Pair{a{b},c}=p"};
    for (size_t i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); ++i)
        CHECK(parse_case(unsupported[i], NL_PARSE_SYNTAX_UNSUPPORTED));
    /* Source order, structural roles and precise name spans. */
    const char *text = "{ let (v, s) = take(p,d); Pair { b: v, a: s } }";
    NLSource *src = NULL;
    NLParser *p = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_create(text, strlen(text), "roles", &src) == NL_SOURCE_OK);
    CHECK(nl_parser_create(src, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_source_fragment(p, &tree, NULL) == NL_PARSE_OK);
    const NLSyntaxView *block = nl_syntax_node_view(nl_syntax_tree_root(tree));
    CHECK(block->kind == NL_SYNTAX_BLOCK && block->data.block.item_count == 1);
    const NLSyntaxView *binding = nl_syntax_node_view(block->data.block.items);
    CHECK(binding->kind == NL_SYNTAX_MULTI_BINDING &&
          binding->data.multi_binding.count == 2);
    const NLSyntaxNode *r = binding->data.multi_binding.receivers;
    CHECK(nl_syntax_node_view(r)->data.name.start_byte == 7);
    CHECK(
        nl_syntax_node_view(nl_syntax_next_argument(r))->data.name.start_byte ==
        10);
    CHECK(nl_syntax_node_view(block->data.block.tail)->kind ==
          NL_SYNTAX_AGGREGATE);
    nl_parser_destroy(p); /* Tree survives parser. */
    CHECK(nl_syntax_node_view(nl_syntax_tree_root(tree))->span.end_byte ==
          strlen(text));
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(src);
    return true;
}

static bool seed(TestSemantic *f, const char *name, NLTypeId type,
                 NLSymbolId *out)
{
    return nl_semantic_seed_value(f->context, name, type, NL_DEPENDENCY_FREE,
                                  out) == NL_CHECK_OK;
}
static bool run_ok(TestSemantic *f, const char *text)
{
    TestChecked a = {0};
    CHECK(test_run(f->context, text, TEST_SOURCE, NL_CHECK_OK, NULL, &a));
    test_checked_destroy(&a);
    return true;
}
static bool reject(TestSemantic *f, const char *text, const char *code)
{
    return test_rejected(f->context, text, TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                         code);
}

static bool binding_tests(void)
{
    TestSemantic f = {0};
    NLSymbolId x, y;
    CHECK(test_semantic_create(&f));
    CHECK(seed(&f, "x", f.copy, &x) && seed(&f, "y", f.linear, &y));
    CHECK(nl_semantic_register_function(f.context, "consume", &f.linear, 1, 1,
                                        false, false) == NL_CHECK_OK);
    CHECK(
        reject(&f, "{let moved=y; consume(moved); y}", "P3-USE-AFTER-CONSUME"));
    CHECK(reject(&f, "{let z=x; let z=x;}", "P5-DUPLICATE-BINDING"));
    CHECK(reject(&f, "{let z=y;}", "P5-SCOPE-OBLIGATION"));
    CHECK(reject(&f, "{y;}", "P5-DISCARDABLE-REQUIRED"));
    CHECK(run_ok(&f, "{let z=x; z; x}"));
    CHECK(nl_semantic_find_binding(f.context, "z") == 0);
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(f.context, x, &b) &&
          b.availability == NL_AVAILABLE);
    CHECK(run_ok(&f, "{let x=x; {let x=x; x}; x}"));
    CHECK(nl_semantic_find_binding(f.context, "x") == x);
    CHECK(run_ok(&f, "{let moved=y; consume(moved);}"));
    CHECK(nl_semantic_binding_view(f.context, y, &b) &&
          b.availability == NL_CONSUMED);
    CHECK(nl_semantic_find_binding(f.context, "moved") == 0);
    CHECK(run_ok(&f, "let u = {}"));
    CHECK(nl_semantic_find_binding(f.context, "u") != 0);
    CHECK(test_rejected(f.context, "let lost = take(x,x)", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
    nl_semantic_destroy(f.context);
    return true;
}

static bool receiving_fixture(TestSemantic *f, NLPlaceId *root,
                              NLSymbolId *ending)
{
    NLValueId value;
    NLSymbolId p;
    CHECK(test_semantic_create(f));
    CHECK(nl_semantic_seed_root(f->context, f->linear, f->domain, true,
                                NL_DEPENDENCY_FREE, root,
                                &value) == NL_CHECK_OK);
    CHECK(test_reference(f->context, "p", *root, NL_TYPE_PTR, NL_ACCESS_READ,
                         false, &p, NULL));
    CHECK(test_domain_ref(f, "ending", NL_ACCESS_READ, true, ending, NULL));
    return true;
}
static bool receiving_tests(void)
{
    TestSemantic f = {0};
    NLPlaceId root;
    NLSymbolId ending;
    CHECK(receiving_fixture(&f, &root, &ending));
    CHECK(reject(&f, "let (value,empty,extra)=take(p,ending)",
                 "P5-RESULT-ARITY"));
    CHECK(reject(&f, "let (v,v)=take(p,ending)", "P5-DUPLICATE-RECEIVER"));
    CHECK(reject(&f, "let (p,s)=take(p,ending)", "P5-DUPLICATE-BINDING"));
    CHECK(reject(&f, "let v=take(p,ending)", "P5-RESULT-ARITY"));
    CHECK(reject(&f, "{take(p,ending);}", "P5-DISCARDABLE-REQUIRED"));
    CHECK(reject(&f, "{let(v,s)=take(p,ending); v}", "P5-SCOPE-OBLIGATION"));
    CHECK(reject(&f, "let(v,s)=take(v,ending)", "P3-UNKNOWN-BINDING"));
    CHECK(
        reject(&f, "{let(v,s)=take(p,ending); missing}", "P3-UNKNOWN-BINDING"));
    TestChecked a = {0};
    CHECK(test_run(f.context, "let(v,s)=take(p,ending)", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &a));
    const NLCheckedNodeView *op =
        nl_checked_node_view(a.artifact, test_root(&a)->initializer);
    CHECK(op->kind == NL_CHECKED_TAKE && op->result_count == 2);
    NLSemanticBindingView v, s, b;
    CHECK(nl_semantic_binding_view(
        f.context, nl_semantic_find_binding(f.context, "v"), &v));
    CHECK(nl_semantic_binding_view(
        f.context, nl_semantic_find_binding(f.context, "s"), &s));
    CHECK(nl_checked_node_view(a.artifact, test_root(&a)->first_argument)
              ->value_use == NL_VALUE_RECEIVED);
    CHECK(v.type == f.linear && v.value == op->results[0].value &&
          s.value == op->results[1].value);
    NLSemanticTypeView st;
    CHECK(nl_semantic_type_view(f.context, s.type, &st) &&
          st.kind == NL_TYPE_SLOT && !st.is_discardable);
    CHECK(nl_semantic_binding_view(f.context, ending, &b) &&
          b.availability == NL_AVAILABLE);
    NLSemanticPlaceView place;
    CHECK(nl_semantic_place_view(f.context, root, &place) && !place.live);
    test_checked_destroy(&a);
    CHECK(reject(&f, "let(v2,s2)=take(p,ending)", "P3-STALE-POINTER"));
    CHECK(nl_semantic_register_function(f.context, "consume", &f.linear, 1, 1,
                                        false, false) == NL_CHECK_OK);
    CHECK(reject(&f, "{let copy=s; let again=s;}", "P3-USE-AFTER-CONSUME"));
    CHECK(run_ok(&f, "consume(v)"));
    nl_semantic_destroy(f.context);
    return true;
}

static bool aggregate_fixture(TestSemantic *f, NLTypeId *pair)
{
    NLSymbolId x, y;
    CHECK(test_semantic_create(f));
    const NLAggregateField fields[] = {{"a", f->linear}, {"b", f->copy}};
    CHECK(nl_semantic_register_aggregate(f->context, "Pair", fields, 2, pair) ==
          NL_CHECK_OK);
    CHECK(seed(f, "x", f->linear, &x) && seed(f, "y", f->copy, &y));
    CHECK(nl_semantic_register_function(f->context, "consume", &f->linear, 1, 1,
                                        false, false) == NL_CHECK_OK);
    return true;
}
static bool aggregate_tests(void)
{
    TestSemantic f = {0};
    NLTypeId pair;
    CHECK(aggregate_fixture(&f, &pair));
    CHECK(reject(&f, "Pair{a:x,a:x}", "P5-AGGREGATE-FIELD"));
    CHECK(reject(&f, "Pair{a:x}", "P5-AGGREGATE-FIELD-COUNT"));
    CHECK(reject(&f, "Pair{c:x,b:y}", "P5-AGGREGATE-FIELD"));
    CHECK(reject(&f, "Pair{a:x,b:x}", "P3-USE-AFTER-CONSUME"));
    CHECK(reject(&f, "Pair{a:y,b:y}", "P5-FIELD-TYPE"));
    CHECK(reject(&f, "{let pair=Pair{a:x,b:y}; pair;}",
                 "P5-DISCARDABLE-REQUIRED"));
    CHECK(reject(&f, "{let pair=Pair{a:x,b:y}; let Pair{a}=pair;}",
                 "P5-AGGREGATE-FIELD-COUNT"));
    CHECK(reject(&f, "{let pair=Pair{a:x,b:y}; let Pair{a,b}=pair; pair}",
                 "P3-USE-AFTER-CONSUME"));
    CHECK(reject(&f, "{let pair=Pair{a:x,b:y}; let Pair{a,b}=pair; b}",
                 "P5-SCOPE-OBLIGATION"));
    TestChecked a = {0};
    CHECK(
        test_run(f.context,
                 "{let pair=Pair{b:y,a:x}; let Pair{b,a}=pair; consume(a); b}",
                 TEST_SOURCE, NL_CHECK_OK, NULL, &a));
    size_t builds = 0, destructures = 0;
    for (size_t i = 1; i <= nl_checked_node_count(a.artifact); ++i) {
        const NLCheckedNodeView *node = nl_checked_node_view(a.artifact, i);
        if (node->kind == NL_CHECKED_AGGREGATE) {
            ++builds;
            const NLCheckedNodeView *first =
                nl_checked_node_view(a.artifact, node->first_argument);
            CHECK(first->field_index == 1);
            NLSemanticValueView value;
            CHECK(nl_semantic_value_view(f.context, node->results[0].value,
                                         &value));
            CHECK(value.field_count == 2 && value.carrier == NL_CARRIER_ENDED);
        }
        if (node->kind == NL_CHECKED_AGGREGATE_BINDING)
            ++destructures;
    }
    CHECK(builds == 1 && destructures == 1 && test_root(&a)->result_count == 1);
    CHECK(nl_semantic_find_binding(f.context, "pair") == 0 &&
          nl_semantic_find_binding(f.context, "a") == 0);
    test_checked_destroy(&a);
    nl_semantic_destroy(f.context);
    f = (TestSemantic){0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x;
    CHECK(seed(&f, "x", f.copy, &x));
    const NLAggregateField copy_fields[] = {{"a", f.copy}, {"b", f.copy}};
    CHECK(nl_semantic_register_aggregate(f.context, "CopyPair", copy_fields, 2,
                                         &pair) == NL_CHECK_OK);
    CHECK(run_ok(&f, "let original=CopyPair{a:x,b:x}"));
    NLSemanticBindingView original;
    NLSemanticValueView value;
    CHECK(nl_semantic_binding_view(
        f.context, nl_semantic_find_binding(f.context, "original"), &original));
    CHECK(nl_semantic_value_view(f.context, original.value, &value));
    CHECK(run_ok(&f, "{let CopyPair{a,b}=original; a; b;}"));
    NLSemanticValueView after, child;
    CHECK(nl_semantic_value_view(f.context, original.value, &after) &&
          after.carrier == NL_CARRIER_PLACE);
    for (size_t i = 0; i < 2; ++i) {
        CHECK(after.fields[i] == value.fields[i]);
        CHECK(nl_semantic_value_view(f.context, after.fields[i], &child) &&
              child.carrier == NL_CARRIER_AGGREGATE &&
              child.aggregate_owner == original.value);
    }
    CHECK(run_ok(&f,
                 "{original;}")); /* copy discard cannot end original members */
    CHECK(run_ok(&f, "original"));
    nl_semantic_destroy(f.context);
    return true;
}

static bool failure_tests(void)
{
    /* Every allocation in registration, parser and source check is injected,
     * until the first unfailed execution. Compare all observable semantic
     * state. */
    TestSemantic f = {0};
    NLTypeId pair;
    CHECK(aggregate_fixture(&f, &pair));
    const char *text = "{let p=Pair{a:x,b:y}; let Pair{a,b}=p; consume(a); b}";
    NLSource *src = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_create(text, strlen(text), "oom", &src) == NL_SOURCE_OK);
    CHECK(nl_parser_create(src, &parser) == NL_PARSE_OK);
    bool reached = false;
    for (size_t n = 0; n < 500; ++n) {
        injecting = true;
        fail_at = n;
        allocation_index = 0;
        NLParseStatus status =
            nl_parser_parse_source_fragment(parser, &tree, NULL);
        injecting = false;
        if (status == NL_PARSE_OK) {
            reached = true;
            break;
        }
        CHECK(status == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
    }
    CHECK(reached && tree != NULL);
    TestState before;
    CHECK(test_state(f.context, &before));
    reached = false;
    for (size_t n = 0; n < 500; ++n) {
        NLCheckedFragment *artifact = NULL;
        NLCheckDiagnostic d = {0};
        injecting = true;
        fail_at = n;
        allocation_index = 0;
        NLCheckStatus status =
            nl_semantic_check_source_fragment(f.context, tree, &artifact, &d);
        injecting = false;
        if (status == NL_CHECK_OK) {
            reached = true;
            nl_checked_destroy(artifact);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              strcmp(d.diagnostic.category, "host") == 0);
        CHECK(test_unchanged(f.context, &before));
        CHECK(nl_semantic_find_binding(f.context, "p") == 0);
    }
    CHECK(reached);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(src);
    const NLAggregateField fields[] = {{"left", f.copy}, {"right", f.copy}};
    CHECK(test_state(f.context, &before));
    reached = false;
    for (size_t n = 0; n < 500; ++n) {
        NLTypeId id = SIZE_MAX;
        injecting = true;
        fail_at = n;
        allocation_index = 0;
        NLCheckStatus status =
            nl_semantic_register_aggregate(f.context, "Other", fields, 2, &id);
        injecting = false;
        if (status == NL_CHECK_OK) {
            reached = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && id == SIZE_MAX);
        CHECK(test_unchanged(f.context, &before));
    }
    CHECK(reached);
    nl_semantic_destroy(f.context);
    f = (TestSemantic){0};
    NLPlaceId root;
    NLSymbolId ending;
    CHECK(receiving_fixture(&f, &root, &ending));
    text = "let(v,s)=take(p,ending)";
    src = NULL;
    parser = NULL;
    tree = NULL;
    CHECK(nl_source_create(text, strlen(text), "receiving-oom", &src) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(src, &parser) == NL_PARSE_OK &&
          nl_parser_parse_source_fragment(parser, &tree, NULL) == NL_PARSE_OK);
    CHECK(test_state(f.context, &before));
    reached = false;
    for (size_t n = 0; n < 500; ++n) {
        NLCheckedFragment *artifact = NULL;
        injecting = true;
        fail_at = n;
        allocation_index = 0;
        NLCheckStatus status =
            nl_semantic_check_source_fragment(f.context, tree, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            reached = true;
            nl_checked_destroy(artifact);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL);
        CHECK(test_unchanged(f.context, &before));
        CHECK(nl_semantic_find_binding(f.context, "v") == 0 &&
              nl_semantic_find_binding(f.context, "s") == 0);
    }
    CHECK(reached);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(src);
    nl_semantic_destroy(f.context);
    /* Copy aggregate member cloning and discard must also be OOM-atomic. */
    f = (TestSemantic){0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x;
    CHECK(seed(&f, "x", f.copy, &x));
    const NLAggregateField copy_fields[] = {{"a", f.copy}, {"b", f.copy}};
    CHECK(nl_semantic_register_aggregate(f.context, "CopyPair", copy_fields, 2,
                                         &pair) == NL_CHECK_OK);
    CHECK(run_ok(&f, "let original=CopyPair{a:x,b:x}"));
    text = "{let CopyPair{a,b}=original; original; a; b;}";
    src = NULL;
    parser = NULL;
    tree = NULL;
    CHECK(nl_source_create(text, strlen(text), "copy-oom", &src) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(src, &parser) == NL_PARSE_OK &&
          nl_parser_parse_source_fragment(parser, &tree, NULL) == NL_PARSE_OK);
    CHECK(test_state(f.context, &before));
    reached = false;
    for (size_t n = 0; n < 500; ++n) {
        NLCheckedFragment *artifact = NULL;
        injecting = true;
        fail_at = n;
        allocation_index = 0;
        NLCheckStatus status =
            nl_semantic_check_source_fragment(f.context, tree, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            reached = true;
            nl_checked_destroy(artifact);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL);
        CHECK(test_unchanged(f.context, &before));
    }
    CHECK(reached);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(src);
    nl_semantic_destroy(f.context);
    return true;
}

static bool diagnostic_tests(void)
{
    TestSemantic f = {0};
    NLSymbolId x;
    CHECK(test_semantic_create(&f) && seed(&f, "x", f.copy, &x));
    CHECK(nl_semantic_register_function(f.context, "effect", NULL, 0, 1, true,
                                        false) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "hidden", NULL, 0, 1, false,
                                        true) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "{effect();}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P3-EFFECT-SUMMARY-UNSUPPORTED"));
    CHECK(test_rejected(f.context, "{hidden();}", TEST_SOURCE,
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P3-DEPENDENCIES-UNSUPPORTED"));
    TestChecked a = {0};
    const char *text = "{\n let a=x;\n unknown\n}";
    CHECK(test_run(f.context, text, TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                   "P3-UNKNOWN-BINDING", &a));
    CHECK(a.diagnostic.span.start_byte == 13 &&
          a.diagnostic.span.end_byte == 20);
    FILE *stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(nl_check_diagnostic_render(stream, a.source, &a.diagnostic));
    fclose(stream);
    test_checked_destroy(&a);
    NLSource *invalid_source = NULL;
    NLParser *invalid_parser = NULL;
    NLSyntaxTree *invalid_tree = NULL;
    CHECK(nl_source_create("CopyT", 5, "invalid-entry", &invalid_source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(invalid_source, &invalid_parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_type_fragment(invalid_parser, &invalid_tree, NULL) ==
          NL_PARSE_OK);
    NLCheckedFragment *invalid_output = NULL;
    NLCheckDiagnostic sentinel = {.span = {42, 43}};
    CHECK(nl_semantic_check_source_fragment(f.context, invalid_tree,
                                            &invalid_output, &sentinel) ==
          NL_CHECK_INTERNAL_ERROR);
    CHECK(invalid_output == NULL && sentinel.span.start_byte == 42 &&
          sentinel.span.end_byte == 43);
    nl_syntax_tree_destroy(invalid_tree);
    nl_parser_destroy(invalid_parser);
    nl_source_destroy(invalid_source);
    /* Host limits, never language invalidity. */
    const NLAggregateField many[NL_SEMANTIC_MAX_FIELDS + 1] = {{"x", f.copy}};
    NLTypeId out = SIZE_MAX;
    CHECK(nl_semantic_register_aggregate(f.context, "Large", many,
                                         NL_SEMANTIC_MAX_FIELDS + 1,
                                         &out) == NL_CHECK_RESOURCE_LIMIT &&
          out == SIZE_MAX);
    char nested[2 * (NL_PARSER_MAX_DEPTH + 1) + 2];
    size_t len = 0;
    for (size_t i = 0; i <= NL_PARSER_MAX_DEPTH; ++i)
        nested[len++] = '{';
    for (size_t i = 0; i <= NL_PARSER_MAX_DEPTH; ++i)
        nested[len++] = '}';
    nested[len] = 0;
    CHECK(parse_case(nested, NL_PARSE_RESOURCE_LIMIT));
    CHECK(test_rejected(f.context, "let(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)=x",
                        TEST_SOURCE, NL_CHECK_RESOURCE_LIMIT,
                        "P5-RECEIVER-LIMIT"));
    const NLAggregateField copy_fields[] = {{"a", f.copy}, {"b", f.copy}};
    NLTypeId pair;
    CHECK(nl_semantic_register_aggregate(f.context, "CopyPair", copy_fields, 2,
                                         &pair) == NL_CHECK_OK);
    CHECK(run_ok(&f, "let original=CopyPair{a:x,b:x}"));
    const size_t repeats = 1400;
    char *large = malloc(repeats * 9 + 3);
    CHECK(large != NULL);
    size_t cursor = 0;
    large[cursor++] = '{';
    for (size_t i = 0; i < repeats; ++i) {
        memcpy(large + cursor, "original;", 9);
        cursor += 9;
    }
    large[cursor++] = '}';
    large[cursor] = 0;
    CHECK(test_rejected(f.context, large, TEST_SOURCE, NL_CHECK_RESOURCE_LIMIT,
                        "P3-RESOURCE-LIMIT"));
    free(large);
    nl_semantic_destroy(f.context);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "parser") == 0)
        return parser_tests() ? 0 : 1;
    if (strcmp(argv[1], "binding") == 0)
        return binding_tests() ? 0 : 1;
    if (strcmp(argv[1], "receiving") == 0)
        return receiving_tests() ? 0 : 1;
    if (strcmp(argv[1], "aggregate") == 0)
        return aggregate_tests() ? 0 : 1;
    if (strcmp(argv[1], "failure") == 0)
        return failure_tests() ? 0 : 1;
    if (strcmp(argv[1], "diagnostic") == 0)
        return diagnostic_tests() ? 0 : 1;
    return 2;
}
