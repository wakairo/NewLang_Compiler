#include "newlang/parser.h"
#include "newlang/lexer.h"
#include "syntax_internal.h"

#include <stdlib.h>
#include <string.h>

struct NLParser {
    const NLSource *source;
    NLLexer lexer;
    NLToken token;
    bool have_token;
    size_t depth;
    NLSyntaxTree *tree; /* Owned only while a fragment call is active. */
    NLParseStatus status;
    NLParseDiagnostic failure;
};

static void fail(NLParser *parser, NLParseStatus status, NLSourceSpan span,
                 const char *code, const char *message)
{
    if (parser->status != NL_PARSE_OK) {
        return;
    }
    const char *category = "syntax";
    if (status == NL_PARSE_LEXICALLY_UNSUPPORTED ||
        status == NL_PARSE_SYNTAX_UNSUPPORTED) {
        category = "unsupported";
    } else if (status == NL_PARSE_OUT_OF_MEMORY ||
               status == NL_PARSE_RESOURCE_LIMIT) {
        category = "host";
    } else if (status == NL_PARSE_INTERNAL_ERROR) {
        category = "internal";
    }
    parser->status = status;
    parser->failure = (NLParseDiagnostic){
        {NL_DIAG_ERROR, category, code, message, NULL, NULL, 0}, span};
}

static bool peek(NLParser *parser)
{
    if (parser->status != NL_PARSE_OK) {
        return false;
    }
    if (!parser->have_token) {
        const NLLexResult result =
            nl_lexer_next(&parser->lexer, &parser->token);
        if (result == NL_LEX_INTERNAL_ERROR) {
            fail(parser, NL_PARSE_INTERNAL_ERROR, (NLSourceSpan){0, 0},
                 "P2-INTERNAL", "invalid lexer state");
            return false;
        }
        parser->have_token = true;
        if (result == NL_LEX_UNSUPPORTED) {
            fail(parser, NL_PARSE_LEXICALLY_UNSUPPORTED, parser->token.span,
                 "P2-LEXICAL-UNSUPPORTED",
                 "source spelling unsupported by P1 lexer");
            return false;
        }
    }
    return true;
}

static bool word(NLParser *parser, const char *spelling)
{
    if (!peek(parser) || parser->token.kind != NL_TOKEN_WORD) {
        return false;
    }
    NLSourceView view;
    if (!nl_source_view(parser->source, parser->token.span, &view)) {
        fail(parser, NL_PARSE_INTERNAL_ERROR, parser->token.span, "P2-INTERNAL",
             "invalid token span");
        return false;
    }
    const size_t length =
        strlen(spelling); /* Static grammar word, not lexeme. */
    return view.length == length && memcmp(view.bytes, spelling, length) == 0;
}

static bool punct(NLParser *parser, unsigned char spelling)
{
    if (!peek(parser) || parser->token.kind != NL_TOKEN_PUNCTUATION) {
        return false;
    }
    NLSourceView view;
    if (!nl_source_view(parser->source, parser->token.span, &view) ||
        view.length != 1) {
        fail(parser, NL_PARSE_INTERNAL_ERROR, parser->token.span, "P2-INTERNAL",
             "invalid punctuation span");
        return false;
    }
    return view.bytes[0] == spelling;
}

static void consume(NLParser *parser)
{
    parser->have_token = false;
}

static bool expect_punct(NLParser *parser, unsigned char spelling,
                         const char *code, const char *message)
{
    if (punct(parser, spelling)) {
        consume(parser);
        return true;
    }
    if (peek(parser)) {
        fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span, code, message);
    }
    return false;
}

static bool expect_word(NLParser *parser, const char *spelling,
                        const char *code, const char *message)
{
    if (word(parser, spelling)) {
        consume(parser);
        return true;
    }
    if (peek(parser)) {
        fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span, code, message);
    }
    return false;
}

static NLSyntaxNode *node(NLParser *parser, NLSyntaxKind kind,
                          NLSourceSpan span)
{
    if (parser->tree->node_count >= NL_PARSER_MAX_NODES) {
        fail(parser, NL_PARSE_RESOURCE_LIMIT, span, "P2-NODE-LIMIT",
             "P2 implementation syntax node budget exceeded");
        return NULL;
    }
    NLSyntaxNode *const result =
        nl_syntax_node_create(parser->tree, kind, span);
    if (result == NULL) {
        fail(parser, NL_PARSE_OUT_OF_MEMORY, span, "P2-OUT-OF-MEMORY",
             "host allocation failed while constructing syntax");
    }
    return result;
}

static bool enter(NLParser *parser)
{
    if (!peek(parser)) {
        return false;
    }
    if (parser->depth >= NL_PARSER_MAX_DEPTH) {
        fail(parser, NL_PARSE_RESOURCE_LIMIT, parser->token.span,
             "P2-DEPTH-LIMIT",
             "P2 implementation recursive syntax depth exceeded");
        return false;
    }
    ++parser->depth;
    return true;
}

static bool mode(NLParser *parser, NLAccessSyntax *out_mode)
{
    if (word(parser, "read")) {
        *out_mode = NL_ACCESS_READ;
    } else if (word(parser, "write")) {
        *out_mode = NL_ACCESS_WRITE;
    } else {
        if (peek(parser)) {
            fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
                 "P2-EXPECTED-MODE", "expected read or write access spelling");
        }
        return false;
    }
    consume(parser);
    return true;
}

static NLSyntaxNode *type(NLParser *parser)
{
    if (!enter(parser)) {
        return NULL;
    }
    NLSyntaxNode *result = NULL;
    if (parser->token.kind != NL_TOKEN_WORD) {
        fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
             "P2-EXPECTED-TYPE",
             "expected a type in the closed P2 type grammar");
        goto done;
    }
    const size_t start = parser->token.span.start_byte;
    const bool is_exclusive = word(parser, "exclusive");
    if (is_exclusive) {
        consume(parser);
        if (!word(parser, "ref")) {
            if (peek(parser)) {
                fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
                     "P2-EXPECTED-REF", "expected ref after exclusive");
            }
            goto done;
        }
    }
    const NLSourceSpan name_span = parser->token.span;
    const bool is_ptr = word(parser, "ptr");
    const bool is_ref = word(parser, "ref");
    consume(parser);
    if (!peek(parser)) {
        goto done;
    }
    if (!is_ptr && !is_ref && punct(parser, '<')) {
        fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
             "P2-SYNTAX-UNSUPPORTED",
             "general generic type syntax is outside P2");
        goto done;
    }
    if (!is_exclusive && (!(is_ptr || is_ref) || !punct(parser, '<'))) {
        result = node(parser, NL_SYNTAX_TYPE_NAME, name_span);
        if (result != NULL) {
            result->view.data.name = name_span;
        }
        goto done;
    }
    if (!expect_punct(parser, '<', "P2-EXPECTED-OPEN-TYPE",
                      "expected < after capability type name")) {
        goto done;
    }
    NLAccessSyntax access = NL_ACCESS_READ;
    if (is_ref && (!mode(parser, &access) ||
                   !expect_punct(parser, ',', "P2-EXPECTED-TYPE-COMMA",
                                 "expected comma after ref access mode"))) {
        goto done;
    }
    NLSyntaxNode *const target = type(parser);
    if (target == NULL || !peek(parser)) {
        goto done;
    }
    const size_t end = parser->token.span.end_byte;
    if (!expect_punct(parser, '>', "P2-EXPECTED-CLOSE-TYPE",
                      "expected > to close capability type")) {
        goto done;
    }
    result = node(parser, is_ptr ? NL_SYNTAX_TYPE_PTR : NL_SYNTAX_TYPE_REF,
                  (NLSourceSpan){start, end});
    if (result != NULL) {
        if (is_ptr) {
            result->view.data.ptr_type.target = target;
        } else {
            result->view.data.ref_type.access = access;
            result->view.data.ref_type.is_exclusive = is_exclusive;
            result->view.data.ref_type.target = target;
        }
    }
done:
    --parser->depth;
    return result;
}

static bool expression_extension(NLParser *parser)
{
    return parser->token.kind == NL_TOKEN_DIGITS || punct(parser, '.') ||
           punct(parser, '<') || punct(parser, '+') || punct(parser, '-') ||
           punct(parser, '*') || punct(parser, '/') || punct(parser, '%') ||
           punct(parser, '=') || punct(parser, '!') || punct(parser, '&') ||
           punct(parser, '|') || punct(parser, '?') || punct(parser, '[') ||
           punct(parser, '{') || punct(parser, '(') || punct(parser, '>') ||
           punct(parser, ':') || punct(parser, ';');
}

static NLSyntaxNode *expression(NLParser *parser)
{
    if (!enter(parser)) {
        return NULL;
    }
    NLSyntaxNode *result = NULL;
    if (parser->token.kind != NL_TOKEN_WORD) {
        const bool unsupported = expression_extension(parser);
        fail(parser,
             unsupported ? NL_PARSE_SYNTAX_UNSUPPORTED : NL_PARSE_SYNTAX_ERROR,
             parser->token.span,
             unsupported ? "P2-SYNTAX-UNSUPPORTED" : "P2-EXPECTED-EXPRESSION",
             unsupported ? "expression spelling outside the P2 subset"
                         : "expected name or free-function call");
        goto done;
    }
    const NLSourceSpan name_span = parser->token.span;
    consume(parser);
    if (!peek(parser)) {
        goto done;
    }
    if (!punct(parser, '(')) {
        result = node(parser, NL_SYNTAX_EXPR_NAME, name_span);
        if (result != NULL) {
            result->view.data.name = name_span;
        }
        goto done;
    }
    consume(parser);
    result = node(parser, NL_SYNTAX_EXPR_CALL, name_span);
    if (result == NULL || !peek(parser)) {
        goto done;
    }
    result->view.data.call.callee = name_span;
    NLSyntaxNode *tail = NULL;
    if (!punct(parser, ')')) {
        for (;;) {
            NLSyntaxNode *const argument = expression(parser);
            if (argument == NULL || !peek(parser)) {
                goto done;
            }
            if (tail == NULL) {
                result->view.data.call.arguments = argument;
            } else {
                tail->argument_next = argument;
            }
            tail = argument;
            ++result->view.data.call.argument_count;
            if (!punct(parser, ',')) {
                break;
            }
            consume(parser);
            if (punct(parser, ')')) {
                fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
                     "P2-TRAILING-COMMA",
                     "trailing call comma is outside the P2 subset");
                goto done;
            }
        }
    }
    if (!peek(parser)) {
        goto done;
    }
    if (expression_extension(parser) || punct(parser, '(')) {
        fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
             "P2-SYNTAX-UNSUPPORTED",
             "call argument extension outside the P2 subset");
        goto done;
    }
    const size_t end = parser->token.span.end_byte;
    if (!expect_punct(parser, ')', "P2-EXPECTED-CLOSE-CALL",
                      "expected comma or ) after call argument")) {
        goto done;
    }
    result->view.span.end_byte = end;
done:
    --parser->depth;
    return parser->status == NL_PARSE_OK ? result : NULL;
}

static NLSyntaxNode *name_expression(NLParser *parser, const char *message)
{
    if (!peek(parser)) {
        return NULL;
    }
    if (parser->token.kind != NL_TOKEN_WORD) {
        fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
             "P2-EXPECTED-LOAN-NAME", message);
        return NULL;
    }
    const NLSourceSpan span = parser->token.span;
    consume(parser);
    NLSyntaxNode *const result = node(parser, NL_SYNTAX_EXPR_NAME, span);
    if (result != NULL) {
        result->view.data.name = span;
    }
    return result;
}

static NLSyntaxNode *binding(NLParser *parser)
{
    if (!peek(parser)) {
        return NULL;
    }
    const size_t start = parser->token.span.start_byte;
    if (!expect_word(parser, "let", "P2-EXPECTED-LET",
                     "expected let at binding fragment start") ||
        !peek(parser)) {
        return NULL;
    }
    if (parser->token.kind != NL_TOKEN_WORD) {
        const bool pattern =
            punct(parser, '(') || punct(parser, '[') || punct(parser, '{');
        fail(parser,
             pattern ? NL_PARSE_SYNTAX_UNSUPPORTED : NL_PARSE_SYNTAX_ERROR,
             parser->token.span, "P2-EXPECTED-BINDING-NAME",
             "P2 bindings require a single name; patterns unsupported");
        return NULL;
    }
    const NLSourceSpan name = parser->token.span;
    consume(parser);
    if (punct(parser, ':')) {
        fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
             "P2-SYNTAX-UNSUPPORTED",
             "binding type annotations are outside P2");
        return NULL;
    }
    if (!expect_punct(parser, '=', "P2-EXPECTED-BINDING-EQUAL",
                      "expected = after binding name")) {
        return NULL;
    }
    NLSyntaxNode *const initializer = expression(parser);
    if (initializer == NULL) {
        return NULL;
    }
    NLSyntaxNode *const result =
        node(parser, NL_SYNTAX_BINDING,
             (NLSourceSpan){start, initializer->view.span.end_byte});
    if (result != NULL) {
        result->view.data.binding.name = name;
        result->view.data.binding.initializer = initializer;
    }
    return result;
}

static bool loan_operand_extension(NLParser *parser)
{
    return punct(parser, '(') || punct(parser, '.') || punct(parser, '[') ||
           punct(parser, '<') || punct(parser, '>') || punct(parser, '+') ||
           punct(parser, '-') || punct(parser, '*') || punct(parser, '/') ||
           punct(parser, '%') || punct(parser, '&') || punct(parser, '|');
}

static NLSyntaxNode *loan(NLParser *parser)
{
    if (!peek(parser)) {
        return NULL;
    }
    const size_t start = parser->token.span.start_byte;
    if (!expect_word(parser, "loan", "P2-EXPECTED-LOAN",
                     "expected loan at loan fragment start")) {
        return NULL;
    }
    const bool is_exclusive = word(parser, "exclusive");
    if (is_exclusive) {
        consume(parser);
    }
    NLAccessSyntax access;
    if (!mode(parser, &access)) {
        return NULL;
    }
    NLSyntaxNode *const source =
        name_expression(parser, "expected loan source name");
    if (source == NULL) {
        return NULL;
    }
    NLSyntaxNode *stability = NULL;
    if (word(parser, "using")) {
        consume(parser);
        stability =
            name_expression(parser, "expected stability name after using");
        if (stability == NULL) {
            return NULL;
        }
    }
    if (loan_operand_extension(parser)) {
        fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
             "P2-SYNTAX-UNSUPPORTED", "complex loan operands are outside P2");
        return NULL;
    }
    if (!expect_word(parser, "as", "P2-EXPECTED-AS",
                     "expected as after loan source/using clause") ||
        !peek(parser)) {
        return NULL;
    }
    if (parser->token.kind != NL_TOKEN_WORD) {
        fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
             "P2-EXPECTED-LOAN-BINDING", "expected loan bound name");
        return NULL;
    }
    const NLSourceSpan bound_name = parser->token.span;
    consume(parser);
    if (!peek(parser)) {
        return NULL;
    }
    const NLSourceSpan open = parser->token.span;
    if (!expect_punct(parser, '{', "P2-EXPECTED-BODY",
                      "expected { to open loan body")) {
        return NULL;
    }
    size_t depth = 1;
    NLSourceSpan close = {0, 0};
    while (depth != 0) {
        if (!peek(parser)) {
            return NULL;
        }
        if (parser->token.kind == NL_TOKEN_EOF) {
            fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
                 "P2-UNCLOSED-BODY", "expected } to close loan body");
            return NULL;
        }
        if (punct(parser, '{')) {
            if (depth >= NL_PARSER_MAX_DEPTH) {
                fail(parser, NL_PARSE_RESOURCE_LIMIT, parser->token.span,
                     "P2-BODY-DEPTH-LIMIT",
                     "P2 implementation body brace depth exceeded");
                return NULL;
            }
            ++depth;
        } else if (punct(parser, '}')) {
            --depth;
            if (depth == 0) {
                close = parser->token.span;
            }
        }
        consume(parser);
    }
    NLSyntaxNode *const result =
        node(parser, NL_SYNTAX_LOAN, (NLSourceSpan){start, close.end_byte});
    if (result != NULL) {
        result->view.data.loan.access = access;
        result->view.data.loan.is_exclusive = is_exclusive;
        result->view.data.loan.source = source;
        result->view.data.loan.stability = stability;
        result->view.data.loan.binding = bound_name;
        result->view.data.loan.body_open = open;
        result->view.data.loan.body_interior =
            (NLSourceSpan){open.end_byte, close.start_byte};
        result->view.data.loan.body_close = close;
    }
    return result;
}

typedef NLSyntaxNode *(*Fragment)(NLParser *parser);

static NLParseStatus fragment(NLParser *parser, NLSyntaxTree **out_tree,
                              NLParseDiagnostic *out_diagnostic, Fragment parse)
{
    if (parser == NULL || out_tree == NULL || *out_tree != NULL) {
        return NL_PARSE_INTERNAL_ERROR;
    }
    (void)nl_lexer_init(&parser->lexer, parser->source);
    parser->have_token = false;
    parser->depth = 0;
    parser->status = NL_PARSE_OK;
    parser->tree = nl_syntax_tree_create(parser->source);
    if (parser->tree == NULL) {
        fail(parser, NL_PARSE_OUT_OF_MEMORY, (NLSourceSpan){0, 0},
             "P2-OUT-OF-MEMORY",
             "host allocation failed while creating syntax owner");
    } else {
        parser->tree->root = parse(parser);
        if (parser->status == NL_PARSE_OK && peek(parser) &&
            parser->token.kind != NL_TOKEN_EOF) {
            fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
                 "P2-TRAILING-SYNTAX",
                 "additional syntax outside one P2 fragment");
        }
        if (parser->status == NL_PARSE_OK && parser->tree->root == NULL) {
            fail(parser, NL_PARSE_INTERNAL_ERROR, (NLSourceSpan){0, 0},
                 "P2-INTERNAL", "parser did not produce a root");
        }
    }
    if (parser->status == NL_PARSE_OK) {
        *out_tree = parser->tree; /* Transfer owner on success only. */
    } else {
        nl_syntax_tree_destroy(parser->tree);
        if (out_diagnostic != NULL) {
            *out_diagnostic = parser->failure;
        }
    }
    parser->tree = NULL;
    return parser->status;
}

NLParseStatus nl_parser_create(const NLSource *source, NLParser **out_parser)
{
    if (source == NULL || out_parser == NULL || *out_parser != NULL) {
        return NL_PARSE_INTERNAL_ERROR;
    }
    NLParser *const parser = malloc(sizeof(*parser));
    if (parser == NULL) {
        return NL_PARSE_OUT_OF_MEMORY;
    }
    *parser = (NLParser){.source = source};
    *out_parser = parser;
    return NL_PARSE_OK;
}

void nl_parser_destroy(NLParser *parser)
{
    free(parser);
}

NLParseStatus nl_parser_parse_type_fragment(NLParser *parser,
                                            NLSyntaxTree **out_tree,
                                            NLParseDiagnostic *out_diagnostic)
{
    return fragment(parser, out_tree, out_diagnostic, type);
}

NLParseStatus
nl_parser_parse_expression_fragment(NLParser *parser, NLSyntaxTree **out_tree,
                                    NLParseDiagnostic *out_diagnostic)
{
    return fragment(parser, out_tree, out_diagnostic, expression);
}

NLParseStatus
nl_parser_parse_binding_fragment(NLParser *parser, NLSyntaxTree **out_tree,
                                 NLParseDiagnostic *out_diagnostic)
{
    return fragment(parser, out_tree, out_diagnostic, binding);
}

NLParseStatus nl_parser_parse_loan_fragment(NLParser *parser,
                                            NLSyntaxTree **out_tree,
                                            NLParseDiagnostic *out_diagnostic)
{
    return fragment(parser, out_tree, out_diagnostic, loan);
}

bool nl_parse_diagnostic_render(FILE *stream, const NLSource *source,
                                const NLParseDiagnostic *diagnostic)
{
    if (diagnostic == NULL || !nl_source_span_valid(source, diagnostic->span)) {
        return false;
    }
    NLSourceView view;
    if (!nl_source_view(source, (NLSourceSpan){0, diagnostic->span.end_byte},
                        &view)) {
        return false;
    }
    NLSourceRange range = {nl_source_name(source), 1, 1, 1, 1};
    size_t line = 1;
    size_t column = 1;
    for (size_t offset = 0; offset <= view.length; ++offset) {
        if (offset == diagnostic->span.start_byte) {
            range.start_line = line;
            range.start_column = column;
        }
        if (offset == diagnostic->span.end_byte) {
            range.end_line = line;
            range.end_column = column;
            break;
        }
        if (view.bytes[offset] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
    }
    NLDiagnostic rendered = diagnostic->diagnostic;
    rendered.range = &range;
    return nl_diagnostic_render(stream, &rendered);
}
