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
    bool match_scrutinee;
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

/* P5 uses a separate closed entry so P2's unsupported classifications remain
 * stable. Lists use the existing intrusive, tree-owned source-order link. */
static NLSyntaxNode *source_expression(NLParser *parser);
static NLSyntaxNode *source_binding(NLParser *parser);

static void link_node(NLSyntaxNode **head, NLSyntaxNode **tail,
                      NLSyntaxNode *item)
{
    if (*tail == NULL) {
        *head = item;
    } else {
        (*tail)->argument_next = item;
    }
    *tail = item;
}

static NLSyntaxNode *source_name(NLParser *parser, NLSyntaxKind kind,
                                 bool reject_wildcard)
{
    if (!peek(parser))
        return NULL;
    if (punct(parser, '(') || punct(parser, '[') || punct(parser, '{') ||
        punct(parser, '.') || (reject_wildcard && word(parser, "_"))) {
        fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
             "P5-PATTERN-UNSUPPORTED",
             "nested/rest/wildcard pattern outside P5");
        return NULL;
    }
    if (parser->token.kind != NL_TOKEN_WORD) {
        fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
             "P5-EXPECTED-NAME", "expected receiver or field name");
        return NULL;
    }
    const NLSourceSpan span = parser->token.span;
    consume(parser);
    NLSyntaxNode *result = node(parser, kind, span);
    if (result != NULL)
        result->view.data.name = span;
    return result;
}

static NLSyntaxNode *source_block(NLParser *parser)
{
    const NLSourceSpan start = parser->token.span;
    consume(parser);
    NLSyntaxNode *result = node(parser, NL_SYNTAX_BLOCK, start);
    NLSyntaxNode *head = NULL, *tail = NULL;
    if (result == NULL)
        return NULL;
    while (peek(parser) && !punct(parser, '}')) {
        const bool is_binding = word(parser, "let");
        const bool is_return = word(parser, "return");
        const NLSourceSpan return_start = parser->token.span;
        if (is_return)
            consume(parser);
        if (is_return && punct(parser, ';')) {
            fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
                 "P9-BARE-RETURN", "return requires an expression");
            return NULL;
        }
        NLSyntaxNode *item =
            is_binding ? source_binding(parser) : source_expression(parser);
        if (item == NULL)
            return NULL;
        if (!punct(parser, ';')) {
            if (!is_binding && !is_return && punct(parser, '}')) {
                result->view.data.block.tail = item;
                break;
            }
            fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
                 is_return ? "P9-RETURN-SEMICOLON" : "P5-EXPECTED-ITEM-END",
                 "non-tail block item requires ;");
            return NULL;
        }
        const size_t end = parser->token.span.end_byte;
        consume(parser);
        if (!is_binding) {
            NLSyntaxNode *statement =
                node(parser, is_return ? NL_SYNTAX_RETURN : NL_SYNTAX_STATEMENT,
                     (NLSourceSpan){is_return ? return_start.start_byte
                                              : item->view.span.start_byte,
                                    end});
            if (statement == NULL)
                return NULL;
            statement->view.data.statement.expression = item;
            item = statement;
        }
        link_node(&head, &tail, item);
        ++result->view.data.block.item_count;
    }
    result->view.data.block.items = head;
    const size_t end = parser->token.span.end_byte;
    if (!expect_punct(parser, '}', "P5-EXPECTED-BLOCK-END", "expected }"))
        return NULL;
    result->view.span.end_byte = end;
    return result;
}

static NLSyntaxNode *source_match(NLParser *parser)
{
    const NLSourceSpan start = parser->token.span;
    consume(parser);
    const bool previous = parser->match_scrutinee;
    parser->match_scrutinee = true;
    NLSyntaxNode *scrutinee = source_expression(parser);
    parser->match_scrutinee = previous;
    if (scrutinee == NULL ||
        !expect_punct(parser, '{', "P6-MATCH-OPEN", "expected match {"))
        return NULL;
    NLSyntaxNode *result = node(parser, NL_SYNTAX_MATCH, start);
    if (result == NULL)
        return NULL;
    result->view.data.match.scrutinee = scrutinee;
    NLSyntaxNode *head = NULL, *tail = NULL;
    while (peek(parser) && !punct(parser, '}')) {
        if (word(parser, "_") || parser->token.kind == NL_TOKEN_DIGITS) {
            fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
                 "P6-PATTERN-UNSUPPORTED",
                 "wildcard/literal arms are outside the closed pattern forms");
            return NULL;
        }
        NLSyntaxNode *pattern = source_name(parser, NL_SYNTAX_MATCH_ARM, true);
        if (pattern == NULL)
            return NULL;
        pattern->view.data.arm.variant = pattern->view.data.name;
        if (punct(parser, '(')) {
            consume(parser);
            pattern->view.data.arm.payload = true;
            pattern->view.data.arm.wildcard = word(parser, "_");
            if (peek(parser) && parser->token.kind != NL_TOKEN_WORD &&
                !punct(parser, ')')) {
                fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
                     "P6-PATTERN-UNSUPPORTED",
                     "general payload patterns are outside P6");
                return NULL;
            }
            NLSyntaxNode *binding =
                source_name(parser, NL_SYNTAX_RECEIVER, false);
            if (binding == NULL)
                return NULL;
            pattern->view.data.arm.binding = binding->view.data.name;
            if (punct(parser, '(') || punct(parser, '{') ||
                punct(parser, '.')) {
                fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
                     "P6-PATTERN-UNSUPPORTED",
                     "nested/qualified patterns are outside P6");
                return NULL;
            }
            if (!expect_punct(parser, ')', "P6-PATTERN-END",
                              "expected single binding or _"))
                return NULL;
        }
        if (!punct(parser, '=')) {
            bool general =
                punct(parser, '.') || punct(parser, '|') || word(parser, "if");
            fail(parser,
                 general ? NL_PARSE_SYNTAX_UNSUPPORTED : NL_PARSE_SYNTAX_ERROR,
                 parser->token.span,
                 general ? "P6-PATTERN-UNSUPPORTED" : "P6-ARROW",
                 general
                     ? "only unqualified closed variant patterns are supported"
                     : "match requires =>");
            return NULL;
        }
        size_t arrow_end = parser->token.span.end_byte;
        consume(parser);
        if (!punct(parser, '>') || parser->token.span.start_byte != arrow_end) {
            fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span, "P6-ARROW",
                 "match requires adjacent =>");
            return NULL;
        }
        consume(parser);
        if (!punct(parser, '{')) {
            fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
                 "P6-ARM-BLOCK", "match arm requires lexical block");
            return NULL;
        }
        NLSyntaxNode *body = source_expression(parser);
        if (body == NULL)
            return NULL;
        pattern->view.data.arm.body = body;
        pattern->view.span.end_byte = body->view.span.end_byte;
        link_node(&head, &tail, pattern);
        ++result->view.data.match.arm_count;
        if (!punct(parser, ','))
            break;
        consume(parser);
    }
    result->view.data.match.arms = head;
    const size_t end = parser->token.span.end_byte;
    if (!expect_punct(parser, '}', "P6-ARM-SEPARATOR",
                      "expected comma or match }"))
        return NULL;
    result->view.span.end_byte = end;
    return result;
}

static NLSyntaxNode *source_expression(NLParser *parser)
{
    if (!enter(parser))
        return NULL;
    NLSyntaxNode *result = NULL;
    if (word(parser, "return")) {
        fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
             "P9-RETURN-ITEM",
             "return is a terminating block item, not an expression");
        goto done;
    }
    if (word(parser, "match")) {
        result = source_match(parser);
        goto done;
    }
    if (punct(parser, '{')) {
        result = source_block(parser);
        goto done;
    }
    if (parser->token.kind != NL_TOKEN_WORD) {
        fail(parser,
             expression_extension(parser) ? NL_PARSE_SYNTAX_UNSUPPORTED
                                          : NL_PARSE_SYNTAX_ERROR,
             parser->token.span, "P5-EXPECTED-EXPRESSION",
             "expected name, call, registered aggregate or lexical block");
        goto done;
    }
    const NLSourceSpan name = parser->token.span;
    consume(parser);
    if (punct(parser, '.')) {
        consume(parser);
        NLSyntaxNode *variant = source_name(parser, NL_SYNTAX_RECEIVER, false);
        if (variant == NULL)
            goto done;
        result = node(parser, NL_SYNTAX_SUM_CONSTRUCTOR, name);
        if (result == NULL)
            goto done;
        result->view.data.constructor.qualifier = name;
        result->view.data.constructor.variant = variant->view.data.name;
        result->view.span.end_byte = variant->view.span.end_byte;
        if (punct(parser, '(')) {
            result->view.data.constructor.parentheses = true;
            consume(parser);
            const bool previous = parser->match_scrutinee;
            parser->match_scrutinee = false;
            NLSyntaxNode *head = NULL, *tail = NULL;
            if (!punct(parser, ')')) {
                for (;;) {
                    NLSyntaxNode *arg = source_expression(parser);
                    if (arg == NULL)
                        break;
                    link_node(&head, &tail, arg);
                    ++result->view.data.constructor.argument_count;
                    if (!punct(parser, ','))
                        break;
                    consume(parser);
                    if (punct(parser, ')')) {
                        fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
                             "P6-CONSTRUCTOR-COMMA",
                             "constructor has no trailing comma");
                        break;
                    }
                }
            }
            parser->match_scrutinee = previous;
            result->view.data.constructor.arguments = head;
            size_t end = parser->token.span.end_byte;
            if (!expect_punct(parser, ')', "P6-CONSTRUCTOR-END",
                              "expected constructor )"))
                goto done;
            result->view.span.end_byte = end;
        }
    } else if (punct(parser, '{') && !parser->match_scrutinee) {
        consume(parser);
        result = node(parser, NL_SYNTAX_AGGREGATE, name);
        if (result == NULL)
            goto done;
        result->view.data.aggregate.type_name = name;
        NLSyntaxNode *head = NULL, *tail = NULL;
        if (punct(parser, '}')) {
            fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
                 "P5-EMPTY-AGGREGATE", "aggregate requires at least one field");
            goto done;
        }
        for (;;) {
            NLSyntaxNode *field = source_name(parser, NL_SYNTAX_FIELD, false);
            if (field == NULL)
                goto done;
            const NLSourceSpan field_name = field->view.data.name;
            if (punct(parser, ',') || punct(parser, '}')) {
                fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
                     "P5-FIELD-SHORTHAND-UNSUPPORTED",
                     "field shorthand construction outside P5");
                goto done;
            }
            if (!expect_punct(parser, ':', "P5-EXPECTED-FIELD-COLON",
                              "field initializer requires :"))
                goto done;
            NLSyntaxNode *value = source_expression(parser);
            if (value == NULL)
                goto done;
            field->view.data.binding.name = field_name;
            field->view.data.binding.initializer = value;
            field->view.span.end_byte = value->view.span.end_byte;
            link_node(&head, &tail, field);
            ++result->view.data.aggregate.count;
            if (!punct(parser, ','))
                break;
            consume(parser);
            if (punct(parser, '}'))
                break;
        }
        result->view.data.aggregate.fields = head;
        const size_t end = parser->token.span.end_byte;
        if (!expect_punct(parser, '}', "P5-EXPECTED-AGGREGATE-END",
                          "expected , or }"))
            goto done;
        result->view.span.end_byte = end;
    } else if (punct(parser, '(')) {
        consume(parser);
        result = node(parser, NL_SYNTAX_EXPR_CALL, name);
        if (result == NULL)
            goto done;
        result->view.data.call.callee = name;
        NLSyntaxNode *head = NULL, *tail = NULL;
        if (!punct(parser, ')')) {
            for (;;) {
                const bool previous = parser->match_scrutinee;
                parser->match_scrutinee = false;
                NLSyntaxNode *arg = source_expression(parser);
                parser->match_scrutinee = previous;
                if (arg == NULL)
                    goto done;
                link_node(&head, &tail, arg);
                ++result->view.data.call.argument_count;
                if (!punct(parser, ','))
                    break;
                consume(parser);
                if (punct(parser, ')')) {
                    fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED,
                         parser->token.span, "P2-TRAILING-COMMA",
                         "trailing call comma outside selected subset");
                    goto done;
                }
            }
        }
        result->view.data.call.arguments = head;
        if (expression_extension(parser) && !punct(parser, ')')) {
            fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
                 "P5-EXPRESSION-UNSUPPORTED",
                 "call argument extension outside P5");
            goto done;
        }
        const size_t end = parser->token.span.end_byte;
        if (!expect_punct(parser, ')', "P5-EXPECTED-CALL-END",
                          "expected , or )"))
            goto done;
        result->view.span.end_byte = end;
    } else {
        result = node(parser, NL_SYNTAX_EXPR_NAME, name);
        if (result != NULL)
            result->view.data.name = name;
    }
    if (peek(parser) && expression_extension(parser) && !punct(parser, ';') &&
        !(parser->match_scrutinee && punct(parser, '{'))) {
        fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
             "P5-EXPRESSION-UNSUPPORTED", "expression extension outside P5");
    }
done:
    --parser->depth;
    return parser->status == NL_PARSE_OK ? result : NULL;
}

static NLSyntaxNode *source_binding(NLParser *parser)
{
    const size_t start = parser->token.span.start_byte;
    consume(parser); /* contextual let */
    NLSyntaxNode *result = NULL, *head = NULL, *tail = NULL;
    size_t count = 0;
    if (punct(parser, '(')) {
        consume(parser);
        result =
            node(parser, NL_SYNTAX_MULTI_BINDING, (NLSourceSpan){start, start});
        if (result == NULL)
            return NULL;
        for (;;) {
            NLSyntaxNode *name = source_name(parser, NL_SYNTAX_RECEIVER, true);
            if (name == NULL)
                return NULL;
            link_node(&head, &tail, name);
            ++count;
            if (!punct(parser, ','))
                break;
            consume(parser);
        }
        if (count < 2) {
            fail(parser, NL_PARSE_SYNTAX_ERROR, parser->token.span,
                 "P5-RECEIVER-COUNT",
                 "receiver list requires at least two names");
            return NULL;
        }
        if (!expect_punct(parser, ')', "P5-EXPECTED-RECEIVER-END",
                          "expected , or )"))
            return NULL;
        result->view.data.multi_binding.receivers = head;
        result->view.data.multi_binding.count = count;
    } else {
        NLSyntaxNode *first = source_name(parser, NL_SYNTAX_RECEIVER, false);
        if (first == NULL)
            return NULL;
        const NLSourceSpan name = first->view.data.name;
        if (punct(parser, '{')) {
            consume(parser);
            result = node(parser, NL_SYNTAX_AGGREGATE_BINDING,
                          (NLSourceSpan){start, start});
            if (result == NULL)
                return NULL;
            for (;;) {
                NLSyntaxNode *field =
                    source_name(parser, NL_SYNTAX_RECEIVER, false);
                if (field == NULL)
                    return NULL;
                link_node(&head, &tail, field);
                ++count;
                if (punct(parser, ':') || punct(parser, '{') ||
                    punct(parser, '(') || punct(parser, '.')) {
                    fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED,
                         parser->token.span, "P5-PATTERN-UNSUPPORTED",
                         "renaming/nested pattern outside P5");
                    return NULL;
                }
                if (!punct(parser, ','))
                    break;
                consume(parser);
                if (punct(parser, '}'))
                    break;
            }
            if (!expect_punct(parser, '}', "P5-EXPECTED-PATTERN-END",
                              "expected , or }"))
                return NULL;
            result->view.data.aggregate.type_name = name;
            result->view.data.aggregate.fields = head;
            result->view.data.aggregate.count = count;
        } else {
            result =
                node(parser, NL_SYNTAX_BINDING, (NLSourceSpan){start, start});
            if (result == NULL)
                return NULL;
            result->view.data.binding.name = name;
        }
    }
    if (punct(parser, ':')) {
        fail(parser, NL_PARSE_SYNTAX_UNSUPPORTED, parser->token.span,
             "P5-ANNOTATION-UNSUPPORTED", "binding annotation outside P5");
        return NULL;
    }
    if (!expect_punct(parser, '=', "P5-EXPECTED-BINDING-EQUAL", "expected ="))
        return NULL;
    NLSyntaxNode *initializer = source_expression(parser);
    if (initializer == NULL)
        return NULL;
    result->view.span.end_byte = initializer->view.span.end_byte;
    if (result->view.kind == NL_SYNTAX_BINDING)
        result->view.data.binding.initializer = initializer;
    else if (result->view.kind == NL_SYNTAX_MULTI_BINDING)
        result->view.data.multi_binding.initializer = initializer;
    else
        result->view.data.aggregate.initializer = initializer;
    return result;
}

static NLSyntaxNode *source_fragment(NLParser *parser)
{
    if (!peek(parser))
        return NULL;
    const bool is_binding = word(parser, "let");
    NLSyntaxNode *result =
        is_binding ? source_binding(parser) : source_expression(parser);
    if (result != NULL && punct(parser, ';')) {
        const size_t end = parser->token.span.end_byte;
        consume(parser);
        if (!is_binding) {
            NLSyntaxNode *statement =
                node(parser, NL_SYNTAX_STATEMENT,
                     (NLSourceSpan){result->view.span.start_byte, end});
            if (statement == NULL)
                return NULL;
            statement->view.data.statement.expression = result;
            result = statement;
        }
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

NLParseStatus nl_parser_parse_source_fragment(NLParser *parser,
                                              NLSyntaxTree **out,
                                              NLParseDiagnostic *diagnostic)
{
    return fragment(parser, out, diagnostic, source_fragment);
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
