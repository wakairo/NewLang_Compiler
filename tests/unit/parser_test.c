#include "../support/syntax_check.h"
#include "newlang/parser.h"

#include <stdlib.h>

typedef NLParseStatus (*Parse)(NLParser *, NLSyntaxTree **,
                               NLParseDiagnostic *);

/* Test-only fault injection; no production allocator hooks. */
static size_t allocation_count;
static size_t fail_allocation;
void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size)
{
    if (fail_allocation != 0 && ++allocation_count == fail_allocation) {
        return NULL;
    }
    return __real_malloc(size);
}

static bool create(const char *text, NLSource **source, NLParser **parser)
{
    CHECK(nl_source_create(text, strlen(text), "fragment", source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(*source, parser) == NL_PARSE_OK);
    return true;
}

static bool positive_type(void)
{
    const struct {
        const char *text;
        NLSyntaxKind kind;
        NLAccessSyntax access;
        bool exclusive;
        size_t target_start;
        size_t target_end;
    } cases[] = {
        {"T", NL_SYNTAX_TYPE_NAME, NL_ACCESS_READ, false, 0, 0},
        {"LifetimeDomain", NL_SYNTAX_TYPE_NAME, NL_ACCESS_READ, false, 0, 0},
        {"ptr<T>", NL_SYNTAX_TYPE_PTR, NL_ACCESS_READ, false, 4, 5},
        {"ptr<ptr<T>>", NL_SYNTAX_TYPE_PTR, NL_ACCESS_READ, false, 4, 10},
        {"ref<read,T>", NL_SYNTAX_TYPE_REF, NL_ACCESS_READ, false, 9, 10},
        {"ref<write,T>", NL_SYNTAX_TYPE_REF, NL_ACCESS_WRITE, false, 10, 11},
        {"ref<read,ptr<T>>", NL_SYNTAX_TYPE_REF, NL_ACCESS_READ, false, 9, 15},
        {"exclusive ref<read,T>", NL_SYNTAX_TYPE_REF, NL_ACCESS_READ, true, 19,
         20},
        {"exclusive ref<write,T>", NL_SYNTAX_TYPE_REF, NL_ACCESS_WRITE, true,
         20, 21}};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        NLSource *source = NULL;
        NLParser *parser = NULL;
        CHECK(create(cases[i].text, &source, &parser));
        for (size_t repeat = 0; repeat < 2; ++repeat) {
            NLSyntaxTree *tree = NULL;
            CHECK(nl_parser_parse_type_fragment(parser, &tree, NULL) ==
                  NL_PARSE_OK);
            CHECK(test_tree(tree));
            const NLSyntaxView *const root =
                nl_syntax_node_view(nl_syntax_tree_root(tree));
            CHECK(root->kind == cases[i].kind);
            CHECK(root->span.start_byte == 0 &&
                  root->span.end_byte == strlen(cases[i].text));
            if (root->kind == NL_SYNTAX_TYPE_NAME) {
                CHECK(test_name(source, root->data.name, cases[i].text));
            } else {
                const NLSyntaxNode *target;
                if (root->kind == NL_SYNTAX_TYPE_PTR) {
                    target = root->data.ptr_type.target;
                } else {
                    target = root->data.ref_type.target;
                    CHECK(root->data.ref_type.access == cases[i].access);
                    CHECK(root->data.ref_type.is_exclusive ==
                          cases[i].exclusive);
                }
                const NLSyntaxView *const child = nl_syntax_node_view(target);
                CHECK(child->span.start_byte == cases[i].target_start);
                CHECK(child->span.end_byte == cases[i].target_end);
                if (strcmp(cases[i].text, "ptr<ptr<T>>") == 0 ||
                    strcmp(cases[i].text, "ref<read,ptr<T>>") == 0) {
                    CHECK(child->kind == NL_SYNTAX_TYPE_PTR);
                    CHECK(test_name(
                        source,
                        nl_syntax_node_view(child->data.ptr_type.target)
                            ->data.name,
                        "T"));
                } else {
                    CHECK(child->kind == NL_SYNTAX_TYPE_NAME &&
                          test_name(source, child->data.name, "T"));
                }
            }
            nl_syntax_tree_destroy(tree);
        }
        nl_parser_destroy(parser);
        nl_source_destroy(source);
    }
    NLSource *source = NULL;
    NLParser *parser = NULL;
    CHECK(create(" \tptr <\r\n T >\n", &source, &parser));
    NLSyntaxTree *tree = NULL;
    CHECK(nl_parser_parse_type_fragment(parser, &tree, NULL) == NL_PARSE_OK);
    const NLSyntaxView *const root =
        nl_syntax_node_view(nl_syntax_tree_root(tree));
    CHECK(root->span.start_byte == 2 && root->span.end_byte == 13);
    CHECK(test_name(source,
                    nl_syntax_node_view(root->data.ptr_type.target)->data.name,
                    "T"));
    CHECK(test_tree(tree));
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}

static bool failure(Parse parse, const char *text, NLParseStatus expected,
                    const char *code)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    CHECK(create(text, &source, &parser));
    for (size_t repeat = 0; repeat < 2; ++repeat) {
        NLSyntaxTree *tree = NULL;
        NLParseDiagnostic diagnostic;
        CHECK(parse(parser, &tree, &diagnostic) == expected);
        CHECK(tree == NULL && nl_source_span_valid(source, diagnostic.span));
        CHECK(diagnostic.diagnostic.severity == NL_DIAG_ERROR);
        CHECK(diagnostic.diagnostic.code != NULL &&
              diagnostic.diagnostic.message != NULL);
        if (code != NULL) {
            CHECK(strcmp(diagnostic.diagnostic.code, code) == 0);
        }
        CHECK(strcmp(diagnostic.diagnostic.category,
                     expected == NL_PARSE_SYNTAX_ERROR ? "syntax"
                     : expected == NL_PARSE_RESOURCE_LIMIT
                         ? "host"
                         : "unsupported") == 0);
    }
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}

static bool type_tests(void)
{
    CHECK(positive_type());
    const char *const errors[] = {"",
                                  "ptr<>",
                                  "ptr<T",
                                  "ref<T>",
                                  "ref<read>",
                                  "ref<read,T",
                                  "ref<exclusive,T>",
                                  "ref<read,write,T>",
                                  "exclusive ptr<T>",
                                  "ref<exclusive,read,T>",
                                  "ref<read,exclusive,T>",
                                  "ptr<ref<read,T>>>",
                                  "ptr<T)"};
    for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i) {
        const NLParseStatus status = strcmp(errors[i], "ptr<ref<read,T>>>") == 0
                                         ? NL_PARSE_SYNTAX_UNSUPPORTED
                                         : NL_PARSE_SYNTAX_ERROR;
        CHECK(failure(nl_parser_parse_type_fragment, errors[i], status, NULL));
    }
    CHECK(failure(nl_parser_parse_type_fragment, "Foo<T>",
                  NL_PARSE_SYNTAX_UNSUPPORTED, NULL));
    CHECK(failure(nl_parser_parse_type_fragment, "ptr<Foo<T>>",
                  NL_PARSE_SYNTAX_UNSUPPORTED, NULL));
    CHECK(failure(nl_parser_parse_type_fragment, "T;",
                  NL_PARSE_SYNTAX_UNSUPPORTED, NULL));
    return true;
}

static bool expression_tests(void)
{
    const struct {
        const char *text;
        const char *callee;
        size_t count;
    } calls[] = {{"f()", "f", 0},
                 {"f(x)", "f", 1},
                 {"f(x, y)", "f", 2},
                 {"f(g(x), y)", "f", 2},
                 {"LifetimeDomain()", "LifetimeDomain", 0},
                 {"ptr_from_ref(r)", "ptr_from_ref", 1},
                 {"finalize_domain(d)", "finalize_domain", 1},
                 {"initialize(slot, value, stable)", "initialize", 3},
                 {"take(p, ending)", "take", 2},
                 {"destroy(p, ending)", "destroy", 2},
                 {"replace(dst, value)", "replace", 2},
                 {"store(dst, value)", "store", 2},
                 {"swap(a, b)", "swap", 2},
                 {"take()", "take", 0},
                 {"take(a,b,c)", "take", 3},
                 {"split(x,y)", "split", 2},
                 {"loan(read,write,exclusive,using,as)", "loan", 5}};
    for (size_t i = 0; i < sizeof(calls) / sizeof(calls[0]); ++i) {
        NLSource *source = NULL;
        NLParser *parser = NULL;
        CHECK(create(calls[i].text, &source, &parser));
        NLSyntaxTree *tree = NULL;
        CHECK(nl_parser_parse_expression_fragment(parser, &tree, NULL) ==
              NL_PARSE_OK);
        CHECK(test_tree(tree));
        const NLSyntaxView *const root =
            nl_syntax_node_view(nl_syntax_tree_root(tree));
        CHECK(root->kind == NL_SYNTAX_EXPR_CALL &&
              root->data.call.argument_count == calls[i].count);
        CHECK(test_name(source, root->data.call.callee, calls[i].callee));
        CHECK(root->span.start_byte == 0 &&
              root->span.end_byte == strlen(calls[i].text));
        if (strcmp(calls[i].text, "f(g(x), y)") == 0) {
            const NLSyntaxNode *const first = root->data.call.arguments;
            const NLSyntaxView *const nested = nl_syntax_node_view(first);
            CHECK(nested->kind == NL_SYNTAX_EXPR_CALL &&
                  nested->span.start_byte == 2 && nested->span.end_byte == 6);
            CHECK(test_name(source, nested->data.call.callee, "g"));
            CHECK(test_name(
                source,
                nl_syntax_node_view(nested->data.call.arguments)->data.name,
                "x"));
            const NLSyntaxView *const second =
                nl_syntax_node_view(nl_syntax_next_argument(first));
            CHECK(second->span.start_byte == 8 && second->span.end_byte == 9);
            CHECK(test_name(source, second->data.name, "y"));
        }
        nl_syntax_tree_destroy(tree);
        nl_parser_destroy(parser);
        nl_source_destroy(source);
    }
    const char *const bindings[] = {"let d = LifetimeDomain()", "let d2 = d",
                                    "let p = ptr_from_ref(r)",
                                    "let old = replace(dst, value)"};
    const char *const names[] = {"d", "d2", "p", "old"};
    for (size_t i = 0; i < 4; ++i) {
        NLSource *source = NULL;
        NLParser *parser = NULL;
        CHECK(create(bindings[i], &source, &parser));
        NLSyntaxTree *tree = NULL;
        CHECK(nl_parser_parse_binding_fragment(parser, &tree, NULL) ==
                  NL_PARSE_OK &&
              test_tree(tree));
        const NLSyntaxView *const root =
            nl_syntax_node_view(nl_syntax_tree_root(tree));
        CHECK(root->kind == NL_SYNTAX_BINDING &&
              test_name(source, root->data.binding.name, names[i]));
        CHECK(root->data.binding.name.start_byte == 4 &&
              root->data.binding.name.end_byte == 4 + strlen(names[i]));
        const NLSyntaxView *const init =
            nl_syntax_node_view(root->data.binding.initializer);
        CHECK(init->kind ==
              (i == 1 ? NL_SYNTAX_EXPR_NAME : NL_SYNTAX_EXPR_CALL));
        nl_syntax_tree_destroy(tree);
        nl_parser_destroy(parser);
        nl_source_destroy(source);
    }
    const char *const errors[] = {
        "",      "f(",      "f(x",     "f(x y)",    "f(,x)",  "f(g(x],y)",
        "f(x,)", "x.field", "x+y",     "f(x>y)",    "f(x:y)", "f(x;y)",
        "42",    "(x)",     "if x {}", "fn f() {}", "f()(x)"};
    for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i) {
        CHECK(failure(
            nl_parser_parse_expression_fragment, errors[i],
            i < 6 ? NL_PARSE_SYNTAX_ERROR : NL_PARSE_SYNTAX_UNSUPPORTED, NULL));
    }
    CHECK(failure(nl_parser_parse_binding_fragment,
                  "let (value, slot) = take(p, ending)",
                  NL_PARSE_SYNTAX_UNSUPPORTED, NULL));
    CHECK(failure(nl_parser_parse_binding_fragment, "let x:T = y",
                  NL_PARSE_SYNTAX_UNSUPPORTED, NULL));
    CHECK(failure(nl_parser_parse_binding_fragment, "let = x",
                  NL_PARSE_SYNTAX_ERROR, NULL));
    CHECK(failure(nl_parser_parse_binding_fragment, "let x",
                  NL_PARSE_SYNTAX_ERROR, NULL));
    CHECK(failure(nl_parser_parse_binding_fragment,
                  "let x =", NL_PARSE_SYNTAX_ERROR, NULL));
    CHECK(failure(nl_parser_parse_expression_fragment, "f(//x)",
                  NL_PARSE_LEXICALLY_UNSUPPORTED, "P2-LEXICAL-UNSUPPORTED"));
    return true;
}

static bool loan_tests(void)
{
    const struct {
        const char *text;
        NLAccessSyntax access;
        bool exclusive;
        bool using;
        const char *bound;
    } cases[] = {
        {"loan read x as r {}", NL_ACCESS_READ, false, false, "r"},
        {"loan write x as w {}", NL_ACCESS_WRITE, false, false, "w"},
        {"loan exclusive read x as r {}", NL_ACCESS_READ, true, false, "r"},
        {"loan exclusive write x as w {}", NL_ACCESS_WRITE, true, false, "w"},
        {"loan read p using stable as r {}", NL_ACCESS_READ, false, true, "r"},
        {"loan write p using stable as w {}", NL_ACCESS_WRITE, false, true,
         "w"},
        {"loan exclusive read p using stable as r {}", NL_ACCESS_READ, true,
         true, "r"},
        {"loan exclusive write p using stable as w {}", NL_ACCESS_WRITE, true,
         true, "w"}};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        NLSource *source = NULL;
        NLParser *parser = NULL;
        CHECK(create(cases[i].text, &source, &parser));
        NLSyntaxTree *tree = NULL;
        CHECK(nl_parser_parse_loan_fragment(parser, &tree, NULL) ==
                  NL_PARSE_OK &&
              test_tree(tree));
        const NLSyntaxView *const root =
            nl_syntax_node_view(nl_syntax_tree_root(tree));
        CHECK(root->kind == NL_SYNTAX_LOAN);
        CHECK(root->data.loan.access == cases[i].access &&
              root->data.loan.is_exclusive == cases[i].exclusive);
        CHECK(root->span.start_byte == 0 &&
              root->span.end_byte == strlen(cases[i].text));
        CHECK(test_name(source,
                        nl_syntax_node_view(root->data.loan.source)->data.name,
                        cases[i].using ? "p" : "x"));
        CHECK(test_name(source, root->data.loan.binding, cases[i].bound));
        CHECK((root->data.loan.stability != NULL) == cases[i].using);
        if (cases[i].using) {
            CHECK(test_name(
                source,
                nl_syntax_node_view(root->data.loan.stability)->data.name,
                "stable"));
        }
        CHECK(root->data.loan.body_interior.start_byte ==
              root->data.loan.body_interior.end_byte);
        nl_syntax_tree_destroy(tree);
        nl_parser_destroy(parser);
        nl_source_destroy(source);
    }
    NLSource *source = NULL;
    NLParser *parser = NULL;
    const char *const text = " \tloan    read\r\nx\nas r\n{ f({ g(r) }) {{} } "
                             "if x { return y; } }\r\n";
    CHECK(create(text, &source, &parser));
    NLSyntaxTree *tree = NULL;
    CHECK(nl_parser_parse_loan_fragment(parser, &tree, NULL) == NL_PARSE_OK &&
          test_tree(tree));
    const NLSyntaxView *root = nl_syntax_node_view(nl_syntax_tree_root(tree));
    CHECK(root->span.start_byte == 2 &&
          root->span.end_byte == strlen(text) - 2);
    CHECK(test_name(source, root->data.loan.body_interior,
                    " f({ g(r) }) {{} } if x { return y; } "));
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    CHECK(failure(nl_parser_parse_loan_fragment, "loan read f(x) as r {}",
                  NL_PARSE_SYNTAX_UNSUPPORTED, NULL));
    const char *const errors[] = {
        "loan x as r {}",
        "loan exclusive x as r {}",
        "loan read as r {}",
        "loan read x r {}",
        "loan read x using as r {}",
        "loan read x using stable r {}",
        "loan read x as r",
        "loan read x as r {",
        "loan read x using stable using other as r {}",
        "loan read x as {}"};
    for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i) {
        CHECK(failure(nl_parser_parse_loan_fragment, errors[i],
                      NL_PARSE_SYNTAX_ERROR, NULL));
    }
    CHECK(failure(nl_parser_parse_loan_fragment, "loan read x as r {} other",
                  NL_PARSE_SYNTAX_UNSUPPORTED, NULL));
    CHECK(failure(nl_parser_parse_loan_fragment, "loan read x as r { /* } */ }",
                  NL_PARSE_LEXICALLY_UNSUPPORTED, NULL));
    CHECK(failure(nl_parser_parse_loan_fragment, "loan read x as r { \"}\" }",
                  NL_PARSE_LEXICALLY_UNSUPPORTED, NULL));
    /* Contextual words remain usable as name operands in unambiguous positions.
     */
    source = NULL;
    parser = NULL;
    tree = NULL;
    CHECK(create("loan read using using as as exclusive {}", &source, &parser));
    CHECK(nl_parser_parse_loan_fragment(parser, &tree, NULL) == NL_PARSE_OK &&
          test_tree(tree));
    root = nl_syntax_node_view(nl_syntax_tree_root(tree));
    CHECK(test_name(source,
                    nl_syntax_node_view(root->data.loan.source)->data.name,
                    "using"));
    CHECK(test_name(source,
                    nl_syntax_node_view(root->data.loan.stability)->data.name,
                    "as"));
    CHECK(test_name(source, root->data.loan.binding, "exclusive"));
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}

static bool ownership_tests(void)
{
    NLSource *source = NULL;
    NLParser *first = NULL;
    NLParser *second = NULL;
    CHECK(create("f(g(x),y)", &source, &first));
    CHECK(nl_parser_create(source, &second) == NL_PARSE_OK);
    NLSyntaxTree *a = NULL;
    NLSyntaxTree *b = NULL;
    NLSyntaxTree *c = NULL;
    CHECK(nl_parser_parse_expression_fragment(first, &a, NULL) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(second, &b, NULL) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(first, &c, NULL) == NL_PARSE_OK);
    CHECK(a != b && a != c && test_tree(a) && test_tree(b) && test_tree(c));
    CHECK(test_equivalent(nl_syntax_tree_root(a), nl_syntax_tree_root(b)));
    CHECK(test_equivalent(nl_syntax_tree_root(a), nl_syntax_tree_root(c)));
    NLParseDiagnostic untouched = {
        {NL_DIAG_NOTE, NULL, "unchanged", "unchanged", NULL, NULL, 0},
        {99, 100}};
    CHECK(nl_parser_parse_type_fragment(first, &a, &untouched) ==
          NL_PARSE_INTERNAL_ERROR);
    CHECK(untouched.span.start_byte == 99 &&
          strcmp(untouched.diagnostic.code, "unchanged") == 0);
    CHECK(nl_parser_parse_type_fragment(first, NULL, &untouched) ==
          NL_PARSE_INTERNAL_ERROR);
    CHECK(nl_parser_create(source, &first) == NL_PARSE_INTERNAL_ERROR);
    nl_parser_destroy(first);
    nl_parser_destroy(second);
    CHECK(test_tree(a) && test_tree(b) && test_tree(c));
    nl_syntax_tree_destroy(a);
    nl_syntax_tree_destroy(b);
    nl_syntax_tree_destroy(c);
    first = NULL;
    CHECK(nl_parser_create(NULL, &first) == NL_PARSE_INTERNAL_ERROR &&
          first == NULL);
    CHECK(nl_parser_create(source, NULL) == NL_PARSE_INTERNAL_ERROR);
    a = NULL;
    CHECK(nl_parser_parse_expression_fragment(NULL, &a, &untouched) ==
              NL_PARSE_INTERNAL_ERROR &&
          a == NULL);
    fail_allocation = 1;
    allocation_count = 0;
    const NLParseStatus created = nl_parser_create(source, &first);
    fail_allocation = 0;
    CHECK(created == NL_PARSE_OUT_OF_MEMORY && first == NULL);
    CHECK(nl_parser_create(source, &first) == NL_PARSE_OK);
    const Parse parses[] = {
        nl_parser_parse_expression_fragment, nl_parser_parse_type_fragment,
        nl_parser_parse_binding_fragment, nl_parser_parse_loan_fragment};
    const char *const inputs[] = {
        "f(g(x),y)", "exclusive ref<read,ptr<T>>", "let x=f(g(y),z)",
        "loan exclusive write p using stable as w { { } }"};
    for (size_t i = 0; i < 4; ++i) {
        NLSource *input = NULL;
        NLParser *parser = NULL;
        CHECK(create(inputs[i], &input, &parser));
        bool saw_failure = false;
        bool succeeded = false;
        for (size_t nth = 1; nth <= 128; ++nth) {
            NLSyntaxTree *tree = NULL;
            NLParseDiagnostic diagnostic;
            fail_allocation = nth;
            allocation_count = 0;
            const NLParseStatus status = parses[i](parser, &tree, &diagnostic);
            fail_allocation = 0;
            if (status == NL_PARSE_OK) {
                CHECK(test_tree(tree));
                nl_syntax_tree_destroy(tree);
                succeeded = true;
                break;
            }
            CHECK(status == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
            CHECK(strcmp(diagnostic.diagnostic.code, "P2-OUT-OF-MEMORY") == 0);
            CHECK(strcmp(diagnostic.diagnostic.category, "host") == 0 &&
                  nl_source_span_valid(input, diagnostic.span));
            saw_failure = true;
        }
        CHECK(saw_failure && succeeded);
        nl_parser_destroy(parser);
        nl_source_destroy(input);
    }
    /* A failure with this parser does not affect an earlier independently owned
     * tree. */
    CHECK(nl_parser_parse_expression_fragment(first, &a, &untouched) ==
          NL_PARSE_OK);
    CHECK(untouched.span.start_byte == 99);
    b = NULL;
    CHECK(nl_parser_parse_type_fragment(first, &b, NULL) ==
              NL_PARSE_SYNTAX_UNSUPPORTED &&
          b == NULL);
    CHECK(test_tree(a));
    CHECK(nl_parser_parse_expression_fragment(first, &b, NULL) == NL_PARSE_OK &&
          test_tree(b));
    nl_syntax_tree_destroy(a);
    nl_syntax_tree_destroy(b);
    nl_parser_destroy(first);
    nl_source_destroy(source);
    nl_parser_destroy(NULL);
    nl_syntax_tree_destroy(NULL);
    CHECK(nl_syntax_node_view(NULL) == NULL &&
          nl_syntax_tree_root(NULL) == NULL);
    CHECK(nl_syntax_tree_source(NULL) == NULL &&
          nl_syntax_next_argument(NULL) == NULL);
    return true;
}

static bool accepted(Parse parse, const char *text)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    CHECK(create(text, &source, &parser));
    NLSyntaxTree *tree = NULL;
    CHECK(parse(parser, &tree, NULL) == NL_PARSE_OK && test_tree(tree));
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}

static bool limits_tests(void)
{
    const size_t depth = NL_PARSER_MAX_DEPTH + 1;
    char *const text = malloc(depth * 5 + 64);
    CHECK(text != NULL);
    size_t used = 0;
    for (size_t i = 0; i < depth; ++i) {
        memcpy(text + used, "ptr<", 4);
        used += 4;
    }
    text[used++] = 'T';
    for (size_t i = 0; i < depth; ++i) {
        text[used++] = '>';
    }
    text[used] = 0;
    CHECK(failure(nl_parser_parse_type_fragment, text, NL_PARSE_RESOURCE_LIMIT,
                  "P2-DEPTH-LIMIT"));
    text[used - 2] = 0; /* Two fewer wrappers: 128 total recursive levels. */
    CHECK(accepted(nl_parser_parse_type_fragment, text + 8));
    used = 0;
    for (size_t i = 0; i < depth; ++i) {
        memcpy(text + used, "f(", 2);
        used += 2;
    }
    text[used++] = 'x';
    for (size_t i = 0; i < depth; ++i) {
        text[used++] = ')';
    }
    text[used] = 0;
    CHECK(failure(nl_parser_parse_expression_fragment, text,
                  NL_PARSE_RESOURCE_LIMIT, "P2-DEPTH-LIMIT"));
    text[used - 2] = 0;
    CHECK(accepted(nl_parser_parse_expression_fragment, text + 4));
    used = 0;
    memcpy(text, "loan read x as r ", 17);
    used += 17;
    for (size_t i = 0; i < depth; ++i) {
        text[used++] = '{';
    }
    for (size_t i = 0; i < depth; ++i) {
        text[used++] = '}';
    }
    text[used] = 0;
    CHECK(failure(nl_parser_parse_loan_fragment, text, NL_PARSE_RESOURCE_LIMIT,
                  "P2-BODY-DEPTH-LIMIT"));
    used = 17;
    for (size_t i = 0; i < NL_PARSER_MAX_DEPTH; ++i) {
        text[used++] = '{';
    }
    for (size_t i = 0; i < NL_PARSER_MAX_DEPTH; ++i) {
        text[used++] = '}';
    }
    text[used] = 0;
    CHECK(accepted(nl_parser_parse_loan_fragment, text));
    free(text);
    const size_t arguments = NL_PARSER_MAX_NODES;
    char *const wide = malloc(arguments * 2 + 4);
    CHECK(wide != NULL);
    memcpy(wide, "f(", 2);
    used = 2;
    for (size_t i = 0; i < arguments; ++i) {
        wide[used++] = 'x';
        wide[used++] = i + 1 == arguments ? ')' : ',';
    }
    wide[used] = 0;
    CHECK(failure(nl_parser_parse_expression_fragment, wide,
                  NL_PARSE_RESOURCE_LIMIT, "P2-NODE-LIMIT"));
    wide[used - 3] =
        ')'; /* Root plus 4095 arguments exactly fits node budget. */
    wide[used - 2] = 0;
    CHECK(accepted(nl_parser_parse_expression_fragment, wide));
    free(wide);
    return true;
}

static bool diagnostic_tests(void)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    CHECK(create("\r\n\tloan read x r {}", &source, &parser));
    NLSyntaxTree *tree = NULL;
    NLParseDiagnostic diagnostic;
    CHECK(nl_parser_parse_loan_fragment(parser, &tree, &diagnostic) ==
          NL_PARSE_SYNTAX_ERROR);
    CHECK(strcmp(diagnostic.diagnostic.code, "P2-EXPECTED-AS") == 0);
    CHECK(diagnostic.span.start_byte == 15 && diagnostic.span.end_byte == 16);
    CHECK(diagnostic.diagnostic.range == NULL);
    FILE *const file = tmpfile();
    CHECK(file != NULL);
    CHECK(nl_parse_diagnostic_render(file, source, &diagnostic));
    CHECK(fflush(file) == 0 && fseek(file, 0, SEEK_SET) == 0);
    char buffer[256] = {0};
    const size_t count = fread(buffer, 1, sizeof(buffer) - 1, file);
    CHECK(!ferror(file) && count > 0);
    CHECK(strstr(buffer, "fragment:2:14-2:15: error(syntax)[P2-EXPECTED-AS]") !=
          NULL);
    CHECK(fclose(file) == 0);
    CHECK(!nl_parse_diagnostic_render(NULL, source, &diagnostic));
    diagnostic.span.end_byte = nl_source_length(source) + 1;
    CHECK(!nl_parse_diagnostic_render(NULL, source, &diagnostic));
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    const unsigned char raw[] = {'f', '(', 0, ')', 0xff};
    source = NULL;
    parser = NULL;
    CHECK(nl_source_create(raw, sizeof(raw), "binary", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &tree, &diagnostic) ==
              NL_PARSE_LEXICALLY_UNSUPPORTED &&
          tree == NULL);
    CHECK(diagnostic.span.start_byte == 2 && diagnostic.span.end_byte == 3);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    CHECK(failure(nl_parser_parse_type_fragment, "ptr<\xc3\xa9>",
                  NL_PARSE_LEXICALLY_UNSUPPORTED, NULL));
    CHECK(failure(nl_parser_parse_expression_fragment,
                  "\xef\xbb\xbf"
                  "f()",
                  NL_PARSE_LEXICALLY_UNSUPPORTED, NULL));
    CHECK(failure(nl_parser_parse_expression_fragment, "f()\r",
                  NL_PARSE_LEXICALLY_UNSUPPORTED, NULL));
    return true;
}

static bool original_grant_tests(void)
{
    const char *text = "struct Packet { p:ptr<Node>, a:Allocation, "
                       "d:LifetimeDomain, } fn main()->unit{unit}";
#ifndef NEWLANG_EXPERIMENTAL_ORIGINAL_GRANT
    CHECK(failure(nl_parser_parse_function_unit, text,
                  NL_PARSE_SYNTAX_UNSUPPORTED, "AVS-DECL-PROFILE"));
#else
    NLSource *source = NULL;
    NLParser *parser = NULL;
    CHECK(create(text, &source, &parser));
    for (size_t repeat = 0; repeat < 2; ++repeat) {
        NLSyntaxTree *tree = NULL;
        CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) ==
              NL_PARSE_OK);
        const NLSyntaxView *root =
            nl_syntax_node_view(nl_syntax_tree_root(tree));
        CHECK(root->kind == NL_SYNTAX_FUNCTION_UNIT);
        const NLSyntaxView *record =
            nl_syntax_node_view(root->data.function_unit.declarations);
        CHECK(record->kind == NL_SYNTAX_EXPERIMENTAL_ROOT_STRUCT &&
              record->data.avs_struct.count == 3);
        CHECK(test_name(source, record->data.avs_struct.name, "Packet"));
        const NLSyntaxView *field =
            nl_syntax_node_view(record->data.avs_struct.fields);
        CHECK(test_name(source, field->data.parameter.name, "p") &&
              nl_syntax_node_view(field->data.parameter.type)->kind ==
                  NL_SYNTAX_TYPE_PTR);
        nl_syntax_tree_destroy(tree);
    }
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    CHECK(
        failure(nl_parser_parse_function_unit,
                "struct Packet{p:ptr<Node>,a:Allocation} fn main()->unit{unit}",
                NL_PARSE_SYNTAX_UNSUPPORTED, "AVS-DECL-PROFILE"));
    CHECK(failure(
        nl_parser_parse_function_unit,
        "struct Packet{p:ptr<Node>,a:Allocation,d:LifetimeDomain,extra:u8} fn "
        "main()->unit{unit}",
        NL_PARSE_SYNTAX_UNSUPPORTED, "AVS-DECL-PROFILE"));
    CHECK(failure(nl_parser_parse_function_unit,
                  "struct Packet{p:ptr<Node>,a:LifetimeDomain,d:Allocation} fn "
                  "main()->unit{unit}",
                  NL_PARSE_SYNTAX_UNSUPPORTED, "AVS-DECL-PROFILE"));
    CHECK(failure(
        nl_parser_parse_function_unit,
        "struct TreeTwo{root:Packet,child:Packet} fn main()->unit{unit}",
        NL_PARSE_SYNTAX_UNSUPPORTED, "AVS-DECL-PROFILE"));
    CHECK(failure(nl_parser_parse_function_unit,
                  "struct Packet{p:ptr<Node>,a:Allocation,d:",
                  NL_PARSE_SYNTAX_UNSUPPORTED, "AVS-DECL-PROFILE"));
#endif
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return EXIT_FAILURE;
    }
    const struct {
        const char *name;
        bool (*run)(void);
    } groups[] = {{"type", type_tests},
                  {"expression", expression_tests},
                  {"loan", loan_tests},
                  {"ownership", ownership_tests},
                  {"limits", limits_tests},
                  {"diagnostics", diagnostic_tests},
                  {"original_grant", original_grant_tests}};
    for (size_t i = 0; i < sizeof(groups) / sizeof(groups[0]); ++i) {
        if (strcmp(argv[1], groups[i].name) == 0) {
            if (!groups[i].run()) {
                return EXIT_FAILURE;
            }
            printf("parser %s: contracts passed\n", groups[i].name);
            return EXIT_SUCCESS;
        }
    }
    return EXIT_FAILURE;
}
